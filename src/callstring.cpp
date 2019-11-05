#include "callstring.h"
#include "simple_interval.h"

namespace pcpo {

	void InterpretCall(llvm::CallInst const* call, std::vector<SimpleInterval>& operands) {
		dbgs(3) << "Call performed\nArguments:\n";
		dbgs(3) << call->getArgOperand(0)->getName() << " ";
		dbgs(3) << operands[0] << "\n";

	}

}

