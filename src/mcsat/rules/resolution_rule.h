#pragma once

#include "mcsat/rules/proof_rule.h"

namespace CVC4 {
namespace mcsat {
namespace rules {

/**
 * Boolean resolution rule. To be used in sequence for one resolution proof.
 */
class BooleanResolutionRule : public ProofRule {

  /** The literals of the current clause */
  LiteralHashSet d_literals;

  /** Initial clause */
  CRef d_initialClause;

  /** Number of resolution steps in this round */
  unsigned d_stepsCount;

public:

  /** Create a new Boolean resolution starting from the given initial clause */
  BooleanResolutionRule(ClauseDatabase& clauseDB, const SolverTrail& trail);

  /** Start the resolution */
  void start(CRef initialClause);

  /**
   * Resolve with given clause. Optionally, you can give the index of the literal to be resolved.
   */
  void resolve(CRef cRef, unsigned literalIndex);

  /** Finish the derivation */
  CRef finish();
};


}
}
}


