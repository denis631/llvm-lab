#pragma once

#include "llvm/Pass.h"
#include "fixpoint.h"
#include "normalized_conjunction.h"

namespace pcpo {

template <typename AbstractState>
void executeFixpointAlgorithmTwoVarEq(llvm::Module const& M);

} /* end of namespace pcpo */
