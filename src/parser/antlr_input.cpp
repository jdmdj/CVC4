/*********************                                                        */
/*! \file antlr_input.cpp
 ** \verbatim
 ** Original author: Christopher L. Conway
 ** Major contributors: Morgan Deters
 ** Minor contributors (to current version): Francois Bobot
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2013  New York University and The University of Iowa
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief A super-class for ANTLR-generated input language parsers.
 **
 ** A super-class for ANTLR-generated input language parsers
 **/

#include <limits.h>
#include <antlr3.h>
#include <stdint.h>

#include "parser/antlr_input.h"
#include "parser/input.h"
#include "parser/bounded_token_buffer.h"
#include "parser/bounded_token_factory.h"
#include "parser/antlr_line_buffered_input.h"
#include "parser/memory_mapped_input_buffer.h"
#include "parser/parser_exception.h"
#include "parser/parser.h"

#include "expr/command.h"
#include "expr/type.h"
#include "parser/cvc/cvc_input.h"
#include "parser/smt1/smt1_input.h"
#include "parser/smt2/smt2_input.h"
#include "parser/tptp/tptp_input.h"
#include "util/output.h"

using namespace std;
using namespace CVC4;
using namespace CVC4::parser;
using namespace CVC4::kind;

namespace CVC4 {
namespace parser {

AntlrInputStream::AntlrInputStream(std::string name, 
                                   pANTLR3_INPUT_STREAM input,
                                   bool fileIsTemporary) :
  InputStream(name, fileIsTemporary),
  d_input(input) {
  assert( input != NULL );
  input->fileName = input->strFactory->newStr8(input->strFactory, (pANTLR3_UINT8)name.c_str());
}

AntlrInputStream::~AntlrInputStream() {
  d_input->free(d_input);
}

pANTLR3_INPUT_STREAM AntlrInputStream::getAntlr3InputStream() const {
  return d_input;
}

AntlrInputStream*
AntlrInputStream::newFileInputStream(const std::string& name,
                                     bool useMmap)
  throw (InputStreamException) {
#ifdef _WIN32
  if(useMmap) {
    useMmap = false;
  }
#endif
  pANTLR3_INPUT_STREAM input = NULL;
  if(useMmap) {
    input = MemoryMappedInputBufferNew(name);
  } else {
    // libantlr3c v3.2 isn't source-compatible with v3.4
#ifdef CVC4_ANTLR3_OLD_INPUT_STREAM
    input = antlr3AsciiFileStreamNew((pANTLR3_UINT8) name.c_str());
#else /* CVC4_ANTLR3_OLD_INPUT_STREAM */
    input = antlr3FileStreamNew((pANTLR3_UINT8) name.c_str(), ANTLR3_ENC_8BIT);
#endif /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  }
  if(input == NULL) {
    throw InputStreamException("Couldn't open file: " + name);
  }
  return new AntlrInputStream( name, input );
}

AntlrInputStream*
AntlrInputStream::newStreamInputStream(std::istream& input,
                                       const std::string& name,
                                       bool lineBuffered)
  throw (InputStreamException) {

  pANTLR3_INPUT_STREAM inputStream = NULL;

  if(lineBuffered) {
#ifdef CVC4_ANTLR3_OLD_INPUT_STREAM
    inputStream =
      antlr3LineBufferedStreamNew(input,
                                  0,
                                  (pANTLR3_UINT8) strdup(name.c_str()));
#else /* CVC4_ANTLR3_OLD_INPUT_STREAM */
    inputStream =
      antlr3LineBufferedStreamNew(input,
                                  ANTLR3_ENC_8BIT,
                                  (pANTLR3_UINT8) strdup(name.c_str()));
#endif /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  } else {

    // Since these are all NULL on entry, realloc will be called
    char *basep = NULL, *boundp = NULL, *cp = NULL;
    /* 64KB seems like a reasonable default size. */
    size_t bufSize = 0x10000;

    /* Keep going until we can't go no more. */
    while( !input.eof() && !input.fail() ) {

      if( cp == boundp ) {
        /* We ran out of room in the buffer. Realloc at double the size. */
        ptrdiff_t offset = cp - basep;
        basep = (char *) realloc(basep, bufSize);
        if( basep == NULL ) {
          throw InputStreamException("Failed buffering input stream: " + name);
        }
        cp = basep + offset;
        boundp = basep + bufSize;
        bufSize *= 2;
      }

      /* Read as much as we have room for. */
      input.read( cp, boundp - cp );
      cp += input.gcount();
    }

    /* Make sure the fail bit didn't get set. */
    if( !input.eof() ) {
      throw InputStreamException("Stream input failed: " + name);
    }

    /* Create an ANTLR input backed by the buffer. */
#ifdef CVC4_ANTLR3_OLD_INPUT_STREAM
    inputStream =
      antlr3NewAsciiStringInPlaceStream((pANTLR3_UINT8) basep,
                                        cp - basep,
                                        (pANTLR3_UINT8) strdup(name.c_str()));
#else /* CVC4_ANTLR3_OLD_INPUT_STREAM */
    inputStream =
      antlr3StringStreamNew((pANTLR3_UINT8) basep,
                            ANTLR3_ENC_8BIT,
                            cp - basep,
                            (pANTLR3_UINT8) strdup(name.c_str()));
#endif /* CVC4_ANTLR3_OLD_INPUT_STREAM */

  }

  if( inputStream == NULL ) {
    throw InputStreamException("Couldn't initialize input: " + name);
  }

  return new AntlrInputStream( name, inputStream );
}


AntlrInputStream*
AntlrInputStream::newStringInputStream(const std::string& input,
                                       const std::string& name)
  throw (InputStreamException) {
  char* inputStr = strdup(input.c_str());
  char* nameStr = strdup(name.c_str());
  assert( inputStr!=NULL && nameStr!=NULL );
#ifdef CVC4_ANTLR3_OLD_INPUT_STREAM
  pANTLR3_INPUT_STREAM inputStream =
      antlr3NewAsciiStringInPlaceStream((pANTLR3_UINT8) inputStr,
                                        input.size(),
                                        (pANTLR3_UINT8) nameStr);
#else /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  pANTLR3_INPUT_STREAM inputStream =
      antlr3StringStreamNew((pANTLR3_UINT8) inputStr,
                            ANTLR3_ENC_8BIT,
                            input.size(),
                            (pANTLR3_UINT8) nameStr);
#endif /* CVC4_ANTLR3_OLD_INPUT_STREAM */
  if( inputStream==NULL ) {
    throw InputStreamException("Couldn't initialize string input: '" + input + "'");
  }
  return new AntlrInputStream( name, inputStream );
}

AntlrInput* AntlrInput::newInput(InputLanguage lang, AntlrInputStream& inputStream) {
  using namespace language::input;

  AntlrInput* input;

  switch(lang) {
  case LANG_CVC4: {
    input = new CvcInput(inputStream);
    break;
  }
  case LANG_SMTLIB_V1:
    input = new Smt1Input(inputStream);
    break;

  case LANG_SMTLIB_V2:
    input = new Smt2Input(inputStream);
    break;

  case LANG_TPTP:
    input = new TptpInput(inputStream);
    break;

  default:
    std::stringstream ss;
    ss << "internal error: unhandled language " << lang << " in AntlrInput::newInput";
    throw InputStreamException(ss.str());
  }

  return input;
}

AntlrInput::AntlrInput(AntlrInputStream& inputStream, unsigned int lookahead) :
    Input(inputStream),
    d_lookahead(lookahead),
    d_lexer(NULL),
    d_parser(NULL),
    d_antlr3InputStream( inputStream.getAntlr3InputStream() ),
    d_tokenBuffer(NULL) {
}

/*
AntlrParser::AntlrParser(ExprManager* exprManager, std::istream& input, const std::string& name, unsigned int lookahead)
  Parser(exprManager,name),
  d_lookahead(lookahead) {

}
*/

/*
AntlrInput::Input(ExprManager* exprManager, const std::string& input, const std::string& name, unsigned int lookahead) :
  Input(exprManager,name),
  d_lookahead(lookahead),
  d_lexer(NULL),
  d_parser(NULL),
  d_tokenStream(NULL) {

  char* inputStr = strdup(input.c_str());
  char* nameStr = strdup(name.c_str());
  if( inputStr==NULL || nameStr==NULL ) {
    throw ParserException("Couldn't initialize string input: '" + input + "'");
  }
  d_inputStream = antlr3NewAsciiStringInPlaceStream((pANTLR3_UINT8)inputStr,input.size(),(pANTLR3_UINT8)nameStr);
  if( d_inputStream == NULL ) {
    throw ParserException("Couldn't create input stream for string: '" + input + "'");
  }

}
*/

AntlrInput::~AntlrInput() {
  BoundedTokenBufferFree(d_tokenBuffer);
}

pANTLR3_COMMON_TOKEN_STREAM AntlrInput::getTokenStream() {
  return d_tokenBuffer->commonTstream;
}

void AntlrInput::lexerError(pANTLR3_BASE_RECOGNIZER recognizer) {
  pANTLR3_LEXER lexer = (pANTLR3_LEXER)(recognizer->super);
  assert(lexer!=NULL);
  Parser *parser = (Parser*)(lexer->super);
  assert(parser!=NULL);
  AntlrInput *input = (AntlrInput*) parser->getInput();
  assert(input!=NULL);

  /* Call the error display routine *if* there's not already a 
   * parse error pending.  If a parser error is pending, this
   * error is probably less important, so we just drop it. */
  if( input->d_parser->rec->state->error == ANTLR3_FALSE ) {
    input->parseError("Error finding next token.");
  }
}

void AntlrInput::warning(const std::string& message) {
  Warning() << getInputStream()->getName() << ':' << d_lexer->getLine(d_lexer) << '.' << d_lexer->getCharPositionInLine(d_lexer) << ": " << message << endl;
}

void AntlrInput::parseError(const std::string& message, bool eofException)
  throw (ParserException) {
  Debug("parser") << "Throwing exception: "
      << (const char*)d_lexer->rec->state->tokSource->fileName->chars << ":"
      << d_lexer->getLine(d_lexer) << "."
      << d_lexer->getCharPositionInLine(d_lexer) << ": "
      << message << endl;
  if(eofException) {
    throw ParserEndOfFileException(message,
                                   (const char*)d_lexer->rec->state->tokSource->fileName->chars,
                                   d_lexer->getLine(d_lexer),
                                   d_lexer->getCharPositionInLine(d_lexer));
  } else {
    throw ParserException(message,
                          (const char*)d_lexer->rec->state->tokSource->fileName->chars,
                          d_lexer->getLine(d_lexer),
                          d_lexer->getCharPositionInLine(d_lexer));
  }
}


void AntlrInput::setAntlr3Lexer(pANTLR3_LEXER pLexer) {
  d_lexer = pLexer;

  pANTLR3_TOKEN_FACTORY pTokenFactory = d_lexer->rec->state->tokFactory;
  if( pTokenFactory != NULL ) {
    pTokenFactory->close(pTokenFactory);
  }

  /* 2*lookahead should be sufficient, but we give ourselves some breathing room. */
  pTokenFactory = BoundedTokenFactoryNew(d_antlr3InputStream, 2*d_lookahead);
  if( pTokenFactory == NULL ) {
    throw InputStreamException("Couldn't create token factory.");
  }
  d_lexer->rec->state->tokFactory = pTokenFactory;

  pBOUNDED_TOKEN_BUFFER buffer = BoundedTokenBufferSourceNew(d_lookahead, d_lexer->rec->state->tokSource);
  if( buffer == NULL ) {
    throw InputStreamException("Couldn't create token buffer.");
  }

  d_tokenBuffer = buffer;

  // Override default lexer error reporting
  d_lexer->rec->reportError = &lexerError;
  // Override default nextToken function, just to prevent exceptions escaping.
  d_lexer->rec->state->tokSource->nextToken = &nextToken;
}

void AntlrInput::setParser(Parser& parser) {
  // ANTLR isn't using super in the lexer or the parser, AFAICT.
  // We could also use @lexer/parser::context to add a field to the generated
  // objects, but then it would have to be declared separately in every
  // language's grammar and we'd have to in the address of the field anyway.
  d_lexer->super = &parser;
  d_parser->super = &parser;
}

void AntlrInput::setAntlr3Parser(pANTLR3_PARSER pParser) {
  d_parser = pParser;
//  d_parser->rec->match = &match;
  d_parser->rec->reportError = &reportError;
  /* Don't try to recover from a parse error. */
  // [chris 4/5/2010] Not clear on why this cast is necessary, but I get an error if I remove it.
  d_parser->rec->recoverFromMismatchedToken =
      (void* (*)(ANTLR3_BASE_RECOGNIZER_struct*, ANTLR3_UINT32, ANTLR3_BITSET_LIST_struct*))
      d_parser->rec->mismatch;
}

}/* CVC4::parser namespace */
}/* CVC4 namespace */
