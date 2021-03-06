/*********************                                                        */
/*! \file cdo_black.h
 ** \verbatim
 ** Original author: Morgan Deters
 ** Major contributors: none
 ** Minor contributors (to current version): none
 ** This file is part of the CVC4 project.
 ** Copyright (c) 2009-2013  New York University and The University of Iowa
 ** See the file COPYING in the top-level source directory for licensing
 ** information.\endverbatim
 **
 ** \brief Black box testing of CVC4::context::CDO<>.
 **
 ** Black box testing of CVC4::context::CDO<>.
 **/

#include <cxxtest/TestSuite.h>

#include <vector>
#include <iostream>
#include "context/context.h"
#include "context/cdlist.h"
#include "context/cdo.h"
#include "util/cvc4_assert.h"

using namespace std;
using namespace CVC4;
using namespace CVC4::context;

class CDOBlack : public CxxTest::TestSuite {
private:

  Context* d_context;

public:

  void setUp() {
    d_context = new Context;
  }

  void tearDown() {
    delete d_context;
  }

  void testIntCDO() {
    // Test that push/pop maintains the original value
    CDO<int> a1(d_context);
    a1 = 5;
    TS_ASSERT(d_context->getLevel() == 0);
    d_context->push();
    a1 = 10;
    TS_ASSERT(d_context->getLevel() == 1);
    TS_ASSERT(a1 == 10);
    d_context->pop();
    TS_ASSERT(d_context->getLevel() == 0);
    TS_ASSERT(a1 == 5);
  }
};
