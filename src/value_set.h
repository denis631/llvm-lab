#pragma once

#include <unordered_map>
#include <vector>

#include "llvm/IR/CFG.h"
#include "llvm/IR/Instructions.h"

#include "global.h"

namespace pcpo {

class AbstractDomainDummy {
public:
    // This has to initialise to either top or bottom, depending on the flagxs
    AbstractDomainDummy(bool isTop = false)
        { assert(false); };

    // Copy constructor. We need to be able to copy this type. (To be precise, we also need copy
    // assignment.) Probably you can just ignore this and leave the default, compiler-generated copy
    // constructor.
    AbstractDomainDummy(AbstractDomainDummy const&) = default;
    
    // Initialise from a constant
    AbstractDomainDummy(llvm::Constant const& constant): AbstractDomainDummy(true) {}

    // This method does the actual abstract interpretation by executing the instruction on the
    // abstract domain, return (an upper bound of) the result. Relevant instructions are mostly the
    // arithmetic ones (like add, sub, mul, etc.). Comparisons are handled mostly using
    // refineBranch, however you can still give bounds on their results here. (They just are not as
    // useful for branching.) Control-flowy instructions, like phi nodes, are also handled above.
    static AbstractDomainDummy interpret (
        llvm::Instruction const& inst, std::vector<AbstractDomainDummy> const& operands
    ) { return AbstractDomainDummy(true); }

    // Return whether the two values represent the same thing
    bool operator== (AbstractDomainDummy o) const
        { assert(false); return false; }
    
    // Refine a by using the information that the value has to fulfill the predicate w.r.t. b. For
    // example, if the domain is an interval domain:
    //     refineBranch( ULE, [5, 10], [7, 8] ) => [5, 8]
    // or
    //     refineBranch( ULE, [5, 10], [7, 8] ) => [5, 10]
    // would be valid implementations, though the former is more precise. In this case the only
    // relevant information is that the number has to be less than or equal to 8.
    //  The following properties have to be fulfilled, if c = refineBranch(~, a, b):
    //     1. c <= a
    //     2. For all x in a, y in b with x ~ y we have x in c.
    // For more or less stupid reasons this function also gets the corresponding llvm values.
    static AbstractDomainDummy refineBranch (
        llvm::CmpInst::Predicate pred, llvm::Value const& a_value, llvm::Value const& b_value,
        AbstractDomainDummy a, AbstractDomainDummy b
    ) { return a; }

    // This mirrors the AbstractState::merge method (documented in AbstractStateDummy), so please
    // refer to that for a description of the different operations. Instead of modifying the object,
    // this returns a new one containing the result of the operation.
    static AbstractDomainDummy merge(Merge_op::Type op, AbstractDomainDummy a, AbstractDomainDummy b)
        { return AbstractDomainDummy(true); }
};

// Return a human readable version of the predicate. Because I have to differentiate between signed
// and unsigned, there is a 'u' or 's' prefix, so 'u<=' means 'unsigned lesser than or equal'.
char const* get_predicate_name(llvm::CmpInst::Predicate pred);

template <typename AbstractDomain>
class AbstractStateValueSet {
public:
    std::unordered_map<llvm::Value const*, AbstractDomain> values;

    // We need an additional boolean, as there is a difference between an empty AbstractState and
    // one that is bottom.
    bool isBottom = true;

public:
    AbstractStateValueSet() = default;
    AbstractStateValueSet(AbstractStateValueSet const& state) = default;

    explicit AbstractStateValueSet(llvm::Function const& f) {
        // We need to initialise the arguments to T
        for (llvm::Argument const& arg: f.args()) {
            values[&arg] = AbstractDomain {true};
        }
        isBottom = false;
    }

    // This constructor is used to initialize the state of a function call, to which parameters are passed.
    // This is the "enter" function as described in "Compiler Design: Analysis and Transformation"
    explicit AbstractStateValueSet(llvm::Function const* callee_func, AbstractStateValueSet const& state,
                                   llvm::CallInst const* call) {
        assert(callee_func->arg_size() == call->getNumArgOperands());
        for (llvm::Argument const& arg: callee_func->args()) {
            llvm::Value* value = call->getArgOperand(arg.getArgNo());
            if (value->getType()->isIntegerTy()) {
                if (llvm::Constant const* c = llvm::dyn_cast<llvm::Constant>(value)) {
                    values[&arg] = AbstractDomain {*c};
                } else {
                    values[&arg] = state.values.at(value);
                }
            } else {
                values[&arg] = AbstractDomain {true};
            }
        }
        isBottom = false;
    }

    // Handles the evaluation of merging points
    void applyPHINode(llvm::BasicBlock const& bb, std::vector<AbstractStateValueSet> const& pred_values,
                      llvm::Instruction const& inst) {
        std::vector<AbstractDomain> operands;
        AbstractDomain inst_result;

        llvm::PHINode const* phi = llvm::dyn_cast<llvm::PHINode>(&inst);

        // Phi nodes are handled here, to get the precise values of the predecessors
        
        for (unsigned i = 0; i < phi->getNumIncomingValues(); ++i) {
            // Find the predecessor corresponding to the block of the phi node
            unsigned block = 0;
            for (llvm::BasicBlock const* pred_bb: llvm::predecessors(&bb)) {
                if (pred_bb == phi->getIncomingBlock(i)) break;
                ++block;
            }

            // Take the union of the values
            AbstractDomain pred_value = pred_values[block].getAbstractValue(*phi->getIncomingValue(i));
            inst_result = AbstractDomain::merge(Merge_op::UPPER_BOUND, inst_result, pred_value);

            operands.push_back(pred_value); // Keep the debug output happy
        }
        
        values[&inst] = inst_result;

        debug_output(inst, operands);
    }

    // Handles the evaluation of function calls
    // This is the "combine" function as described in "Compiler Design: Analysis and Transformation"
    void applyCallInst(llvm::Instruction const& inst, llvm::BasicBlock const* end_block,
                       AbstractStateValueSet const& callee_state) {
        std::vector<AbstractDomain> operands;

        // Keep the debug output happy
        for (llvm::Value const* value : inst.operand_values()) {
            operands.push_back(getAbstractValue(*value));
        }

        //iterate through all instructions of it till we find a return statement
        for (llvm::Instruction const& iter_inst: *end_block) {
            if (llvm::ReturnInst const* ret_inst = llvm::dyn_cast<llvm::ReturnInst>(&iter_inst)) {
                llvm::Value const* ret_val = ret_inst->getReturnValue();
                dbgs(4) << "      Found return instruction\n";

                if (callee_state.values.find(ret_val) != callee_state.values.end()) {
                    dbgs(4) << "      Return evaluated, merging parameters\n";
                    values[&inst] = callee_state.values.at(ret_val);
                } else {
                    dbgs(4) << "      Return not evaluated, setting to bottom\n";
                    values[&inst] = AbstractDomain{}; // Initializes the return value to Bottom
                }
            }
        }
        
        debug_output(inst, operands);
    }

    // Handles the evaluation of return instructions
    void applyReturnInst(llvm::Instruction const& inst) {
        llvm::Value const* ret_val = llvm::dyn_cast<llvm::ReturnInst>(&inst)->getReturnValue();
        if (ret_val && ret_val->getType()->isIntegerTy()) {
            if (llvm::Constant const* c = llvm::dyn_cast<llvm::Constant>(ret_val)) {
                values[&inst] = AbstractDomain{ *c };
            } else if (values.find(ret_val) != values.end()) {
                values[&inst] = values.at(ret_val);
            } else {
                values[&inst] = AbstractDomain{}; // Initializes the return value to Bottom
            }
        } else {
            values[&inst] = AbstractDomain{ true };
        }
    }

    // Handles the evaluation of all other instructions
    void applyDefault(llvm::Instruction const& inst) {
        std::vector<AbstractDomain> operands;

        for (llvm::Value const* value: inst.operand_values()) {
            operands.push_back(getAbstractValue(*value));
        }

        // Compute the result of the operation
        values[&inst] = AbstractDomain::interpret(inst, operands);

        debug_output(inst, operands);
    }

    // Used for debug output
    void debug_output(llvm::Instruction const& inst, std::vector<AbstractDomain> operands) {
        dbgs(3).indent(2) << inst << " // " << values.at(&inst) << ", args ";
        {int i = 0;
        for (llvm::Value const* value: inst.operand_values()) {
            if (i) dbgs(3) << ", ";
            if (value->getName().size()) dbgs(3) << '%' << value->getName() << " = ";
            dbgs(3) << operands[i];
            ++i;
        }}
        dbgs(3) << '\n';
    }

	// Checks whether at least one of the operands is bottom -- in such a case we
	// set the result to bottom as well
	bool checkOperandsForBottom(llvm::Instruction const& inst) {
		std::vector<AbstractDomain> operands;

		for (llvm::Value const* value : inst.operand_values()) {
			operands.push_back(getAbstractValue(*value));
		}

		for (llvm::Value const* value : inst.operand_values())
			if (llvm::Constant const* c = llvm::dyn_cast<llvm::Constant>(value)) {

			} else
				if (values[value].isBottom()) {
					values[&inst] = AbstractDomain{};

					dbgs(3).indent(2) << inst << " // " << values.at(&inst) << ", args ";
					{int i = 0;
					for (llvm::Value const* value : inst.operand_values()) {
						if (i) dbgs(3) << ", ";
						if (value->getName().size()) dbgs(3) << '%' << value->getName() << " = ";
						dbgs(3) << operands[i];
						++i;
					}}
					dbgs(3) << '\n';

					operands.clear();

					return true;
				}
		return false;
	}

 
    bool merge(Merge_op::Type op, AbstractStateValueSet const& other) {
        bool changed = false;

        if (isBottom > other.isBottom) {
            changed = true;
            isBottom = false;
        }
        
        for (std::pair<llvm::Value const*, AbstractDomain> i: other.values) {
            // If our value did not exist before, it will be implicitly initialised as bottom,
            // which works just fine.
            AbstractDomain v = AbstractDomain::merge(op, values[i.first], i.second);

            // No change, nothing to do here
            if (v == values[i.first]) continue;

            if (i.first->getName().size())
                dbgs(3) << "    %" << i.first->getName() << " set to " << v << ", " << Merge_op::name[op] << " "
                        << values[i.first] << " and " << i.second << '\n';
                
            values[i.first] = v;
            changed = true;
			if (checkValueForBottom(4, i.first)) return changed;
        }

        //if (changed) checkForBottom(4);
        
        return changed;
    }
    
    void branch(llvm::BasicBlock const& from, llvm::BasicBlock const& towards) {
        llvm::Instruction const* terminator = from.getTerminator();
        assert(terminator /* from is not a well-formed basic block! */);
        assert(terminator->isTerminator());

        llvm::BranchInst const* branch = llvm::dyn_cast<llvm::BranchInst>(terminator);

        // If the terminator is not a simple branch, we are not interested
        if (not branch) return;

        // In the case of an unconditional branch, there is nothing to do
        if (branch->isUnconditional()) return;

        llvm::Value const& condition = *branch->getCondition();

        llvm::ICmpInst const* cmp = llvm::dyn_cast<llvm::ICmpInst>(&condition);

        // We only deal with integer compares here. If you want to do floating points operations as
        // well, you need to adjust the following lines of code a bit.
        if (not cmp) return;

        // We need to find out whether the towards block is on the true or the false branch
        llvm::CmpInst::Predicate pred;
        if (branch->getOperand(2) == &towards) {
            pred = cmp->getPredicate();
        } else if (branch->getOperand(1) == &towards) {
            // We are the false block, so do the negation of the predicate
            pred = cmp->getInversePredicate();
        } else {
            assert(false /* we were not passed the right 'from' block? */);
        }
        llvm::CmpInst::Predicate pred_s = llvm::CmpInst::getSwappedPredicate(pred);

        dbgs(3) << "      Detected branch from " << from.getName() << " towards " << towards.getName()
                << " using compare in %" << cmp->getName() << '\n';
        
        llvm::Value const& lhs = *cmp->getOperand(0);
        llvm::Value const& rhs = *cmp->getOperand(1);

        AbstractDomain lhs_new;
        AbstractDomain rhs_new;
        
        // Constrain the values if they exist.
        if (values.count(&lhs)) {
            dbgs(3) << "      Deriving constraint %" << lhs.getName()  << ' ' << get_predicate_name(pred) << ' ';
            (rhs.getName().size() ? dbgs(3) << "%" << rhs.getName() : dbgs(3) << rhs)
                    << ", with %" << lhs.getName() << " = " << values[&lhs];
            if (values.count(&rhs)) dbgs(3) << " and %" << rhs.getName() << " = " << values[&rhs];
            dbgs(3) << '\n';

            // For the lhs we say that 'lhs pred rhs' has to hold
            lhs_new = AbstractDomain::refineBranch(pred, lhs, rhs, values[&lhs], getAbstractValue(rhs));
        }
        if (values.count(&rhs)) {
            dbgs(3) << "      Deriving constraint %" << rhs.getName() << ' ' << get_predicate_name(pred_s) << ' ';
            (lhs.getName().size() ? dbgs(3) << "%" << lhs.getName() : dbgs(3) << lhs)
                    << ", with %" << rhs.getName() << " = " << values[&rhs];
            if (values.count(&lhs)) dbgs(3) << " and %" << lhs.getName() << " = " << values[&lhs];
            dbgs(3) << '\n';

            // Here, we take the swapped predicate and assert 'rhs pred_s lhs'
            rhs_new = AbstractDomain::refineBranch(pred_s, rhs, lhs, values[&rhs], getAbstractValue(lhs));
        }

        // The control flow is like this so that the previous ifs do not conflict with one another.
        if (values.count(&lhs)) values[&lhs] = lhs_new;
        if (values.count(&rhs)) values[&rhs] = rhs_new;
        
        if (values.count(&lhs) && values.count(&rhs)) {
            dbgs(3) << "      Values restricted to %" << lhs.getName() << " = " << values[&lhs] << " and %"
                    << rhs.getName() << " = " << values[&rhs] << '\n';
			if (!checkValueForBottom(6, &lhs)) checkValueForBottom(6, &rhs);
        } else if (values.count(&lhs)) {
            dbgs(3) << "      Value restricted to %" << lhs.getName() << " = " << values[&lhs]  << '\n';
			checkValueForBottom(6, &lhs);
        } else if (values.count(&rhs)) {
            dbgs(3) << "      Value restricted to %" << rhs.getName() << " = " << values[&rhs]  << '\n';
			checkValueForBottom(6, &rhs);
        } else {
            dbgs(3) << "      No restrictions were derived.\n";
        }

        // This cannot happen when doing UPPER_BOUND or WIDEN, but for NARROW it is possible, so
        // check just in case.

		//This was created before interprocedural analysis was added, therefore it might work wrong
		//Perhaps, one should check each value for bottom separately, see bool checkValueForBottom()
        //if (!checkValueForBottom(6, &lhs)) checkValueForBottom(6, &rhs);
		//checkForBottom(4);
    }

    void printIncoming(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {
        // @Speed: This is quadratic, could be linear
        bool nothing = true;
        for (std::pair<llvm::Value const*, AbstractDomain> const& i: values) {
            bool read    = false;
            bool written = false;
            for (llvm::Instruction const& inst: bb) {
                if (&inst == i.first) written = true;
                for (llvm::Value const* v: inst.operand_values()) {
                    if (v == i.first) read = true;
                }
            }

            if (read and not written) {
                out.indent(indentation) << '%' << i.first->getName() << " = " << i.second << '\n';
                nothing = false;
            }
        }
        if (nothing) {
            out.indent(indentation) << "<nothing>\n";
        }
    }
    void printOutgoing(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {
        for (auto const& i: values) {
            if (llvm::ReturnInst::classof(i.first)) {
                out.indent(indentation) << "<ret> = " << i.second << '\n';
            } else {
                out.indent(indentation) << '%' << i.first->getName() << " = " << i.second << '\n';
            }
        }
        if (isBottom) {
            out.indent(indentation) << "bottom\n";
        } else if (values.size() == 0) {
            out.indent(indentation) << "<nothing>\n";
        }
    };
    
public:
    AbstractDomain getAbstractValue(llvm::Value const& value) const {
        if (llvm::Constant const* c = llvm::dyn_cast<llvm::Constant>(&value)) {
            return AbstractDomain {*c};
        } else if (values.count(&value)) {
            return values.at(&value);
        } else if (isBottom) {
            // If we are at bottom, there are no values
            return AbstractDomain {};
        } else {
            // This branch should only catch 'weird' values, like things we are not handling
            return AbstractDomain {true};
        }
    }

    // If any of our values is bottom, then we are bottom as well. So this function checks that and
    // normalises our value. Returns whether this changed our value (i.e. we are now bottom).

	// Actually the statement above is correct if we have only one function. If not, then we can
	// have values that are bottom but do not belong to the current function => the function
	// is not bottom. Use bool checkValueForBottom() if you want to check the values separately.
    bool checkForBottom(int indent = 0) {
        if (isBottom) return false;

        for (auto const& i: values) {
            if (i.second == AbstractDomain {}) {
                dbgs(3).indent(indent) << "Variable %" << i.first->getName() << " is bottom, so the state is as well.\n";
                
                values.clear();
                isBottom = true;
                
                return true;
            }
        }
        return false;
    }

	// For a specific value checks whether it is a bottom. See bool checkForBottom for more
	// information
	bool checkValueForBottom(int indent, llvm::Value const* value) {
		if (isBottom) return false;
		if (values[value] == AbstractDomain{}) {
			dbgs(3).indent(indent) << "Variable %" << value->getName() << " is bottom, so the state is as well.\n";
			
			values.clear();
			isBottom = true;

			return true;
		}
		return false;
	}
};

} /* end of namespace pcpo */
