#pragma once

#include "llvm/IR/BasicBlock.h"
#include "llvm/IR/Constant.h"
#include "llvm/IR/CFG.h"
#include "llvm/IR/InstrTypes.h"
#include "llvm/IR/Instructions.h"

#include "global.h"
#include "simple_interval.h"

namespace pcpo {

	void InterpretCall(llvm::CallInst const* call, std::vector<SimpleInterval>& operands);

}