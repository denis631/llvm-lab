#include "affine_relation.h"
#include "global.h"

#include "llvm/IR/CFG.h"

#include <set>

using namespace llvm;
using std::vector;
using std::unordered_map;
using std::set;

namespace pcpo {

// MARK: - Initializers

AffineRelation::AffineRelation(Function const& func) {
    index = createVariableIndexMap(func);
    basis = {Matrix<T>(getNumberOfVariables() + 1)};
    isBottom = basis.empty();
}

AffineRelation::AffineRelation(Function const* callee_func, AffineRelation const& state, CallInst const* call) {
    assert(callee_func->arg_size() == call->getNumArgOperands());
    index = state.index;
    basis = state.basis;

    for (Argument const& arg: callee_func->args()) {
        Value* value = call->getArgOperand(arg.getArgNo());
        if (value->getType()->isIntegerTy()) {
            if (ConstantInt const* c = dyn_cast<ConstantInt>(value)) {
                affineAssignment(&arg, 1, nullptr, c->getSExtValue());
            } else {
                affineAssignment(&arg, 1, value, 0);
            }
        } else {
            nonDeterminsticAssignment(&arg);
        }
    }
    isBottom = basis.empty();
}

// MARK: - AbstractState Interface

void AffineRelation::applyPHINode(BasicBlock const& bb, vector<AffineRelation> const& pred_values, Instruction const& phi) {
    PHINode const* phiNode = dyn_cast<PHINode>(&phi);
    int i = 0;

    for (BasicBlock const* pred_bb: llvm::predecessors(&bb)) {
        auto& incoming_value = *phiNode->getIncomingValueForBlock(pred_bb);
        auto& incoming_state = pred_values[i];
        // Predecessor states should have been merged before. This is just bookkeeping.
        if (llvm::ConstantInt const* c = llvm::dyn_cast<llvm::ConstantInt>(&incoming_value)) {
            AffineRelation acc = *this;
            acc.affineAssignment(&phi, 1, nullptr, c->getSExtValue());
            merge(Merge_op::UPPER_BOUND, acc);
        } else {
            AffineRelation acc = *this;
            for (auto m: incoming_state.basis) {
                auto val = m.column(index.at(&incoming_value));
                acc.affineAssignment(&phi, 1, &incoming_value, 0);
            }
            merge(Merge_op::UPPER_BOUND, acc);
        }
        i++;
    }
}

void AffineRelation::applyCallInst(Instruction const& inst, BasicBlock const* end_block, AffineRelation const& callee_state) {
    // State changes from function call were not merged with the oredecessors.
    // So we have to do more than just bookkeeping.

    //iterate through all instructions of it till we find a return statement
    for (Instruction const& iter_inst: *end_block) {
        if (ReturnInst const* ret_inst = llvm::dyn_cast<llvm::ReturnInst>(&iter_inst)) {
            Value const* ret_val = ret_inst->getReturnValue();
            dbgs(4) << "      Found return instruction\n";

            if (callee_state.index.find(ret_val) != callee_state.index.end()) {
                dbgs(4) << "      Return evaluated, merging parameters\n";
                affineAssignment(&inst, 1, ret_val, 0);
            } else {
                dbgs(4) << "      Return not evaluated, setting to bottom\n";
            }
        }
    }
}

void AffineRelation::applyReturnInst(Instruction const& inst) {
    Value const* ret_val = dyn_cast<llvm::ReturnInst>(&inst)->getReturnValue();
    if (ret_val && ret_val->getType()->isIntegerTy()) {
        if (ConstantInt const* c = dyn_cast<ConstantInt>(ret_val)) {
            affineAssignment(&inst, 1, nullptr, c->getSExtValue());
        } else {
            affineAssignment(&inst, 1, ret_val, 0);
        }
    }
}

void AffineRelation::applyDefault(Instruction const& inst) {
    if (inst.getNumOperands() != 2) return nonDeterminsticAssignment(&inst);

    // We only deal with integer types
    IntegerType const* type = dyn_cast<IntegerType>(inst.getType());
    if (not type) return nonDeterminsticAssignment(&inst);

    type = dyn_cast<IntegerType>(inst.getOperand(0)->getType());
    if (not type) return nonDeterminsticAssignment(&inst);

    type = dyn_cast<IntegerType>(inst.getOperand(1)->getType());
    if (not type) return nonDeterminsticAssignment(&inst);

    if (isa<UndefValue>(inst.getOperand(0)) || isa<UndefValue>(inst.getOperand(1))) {
        return nonDeterminsticAssignment(&inst);
    }

    switch (inst.getOpcode()) {
        case Instruction::Add:
            return Add(inst);
        case Instruction::Sub:
            return Sub(inst);
        case Instruction::Mul:
            return Mul(inst);
        case Instruction::SDiv:
        case Instruction::UDiv:
        default:
            return nonDeterminsticAssignment(&inst);
    }
}

bool AffineRelation::merge(Merge_op::Type op, AffineRelation const& other) {
    if (other.isBottom) {
        return false;
    } else if (isBottom) {
        // FIXME: is this correct?
        basis = other.basis;
        index = other.index;
        isBottom = false;
        return true;
    }

    switch (op) {
        case Merge_op::UPPER_BOUND: return leastUpperBound(other);
        default: abort();
    }
}

// MARK: - Lattice Operations

bool AffineRelation::leastUpperBound(AffineRelation const& rhs) {
    assert(getNumberOfVariables() == rhs.getNumberOfVariables());
    vector<Matrix<T>> before = basis;
    vector<vector<T>> vectors;
    vectors.reserve(basis.size() + rhs.basis.size());
    for (Matrix<T> m: basis) {
        vectors.push_back(m.toVector());
    }

    for (Matrix<T> m: rhs.basis) {
        vectors.push_back(m.toVector());
    }
    // FIXME: i think this is transposing it twice. Maybe create a fast path for this kind of thing.
    Matrix<T> combined = Matrix<T>(vectors).transpose();
    Matrix<T> result = Matrix<T>::span(combined);

    basis = result.reshapeColumns(basis.front().getHeight(), basis.front().getWidth());
    // FIXME: figure out a better way to detect changes
    return before != basis;
}

// MARK: - Assignments

void AffineRelation::affineAssignment(Value const* xi, unordered_map<Value const*,T> relations, T constant) {
    Matrix<T> Wr = Matrix<T>(getNumberOfVariables() + 1);
    Wr(index.at(xi),index.at(xi)) = 0;
    Wr(0,index.at(xi)) = constant;

    for (auto [variable, factor]: relations) {
        Wr(index.at(variable),index.at(xi)) = factor;
    }

    // FIXME: this seems quite inefficient
    Matrix<T> vector = Matrix(Wr.toVector()).transpose();
    Matrix<T> vectorSpan = Matrix<T>::span(vector);
    Wr = vectorSpan.reshape(Wr.getHeight(), Wr.getWidth());

    for (Matrix<T>& matrix: basis) {
        matrix *= Wr;
    }
}

void AffineRelation::affineAssignment(Value const* xi, T a, Value const* xj, T b) {
    if (xj == nullptr) {
        affineAssignment(xi, {}, b);
    } else {
        affineAssignment(xi, {{xj,a}}, b);
    }
}

void AffineRelation::nonDeterminsticAssignment(Value const* xi) {
    if (index.count(xi) == 0) return;

    Matrix<T> T0 = Matrix<T>(getNumberOfVariables() + 1);
    Matrix<T> T1 = Matrix<T>(getNumberOfVariables() + 1);

    T0(index.at(xi),index.at(xi)) = 0;
    T0(0,index.at(xi)) = 0;

    T1(index.at(xi),index.at(xi)) = 0;
    T1(0,index.at(xi)) = 1;

    vector<vector<T>> assignment_vectors;
    assignment_vectors.push_back(T0.toVector());
    assignment_vectors.push_back(T1.toVector());

    Matrix<T> combined = Matrix<T>(assignment_vectors).transpose();
    Matrix<T> result = Matrix<T>::span(combined);

    vector<Matrix<T>> span = result.reshapeColumns(T0.getHeight(), T0.getWidth());

    for (Matrix<T>& matrix_state: basis) {
        for (Matrix<T> const& matrix_assignment: span) {
            matrix_state *= matrix_assignment;
        }
    }
}

// MARK: - Abstract Operators

void AffineRelation::Add(Instruction const& inst) {
    auto op1 = inst.getOperand(0);
    auto op2 = inst.getOperand(1);

    // [xi := bj + bk]
    if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
        auto b1 = dyn_cast<ConstantInt>(op1);
        auto b2 = dyn_cast<ConstantInt>(op2);
        return affineAssignment(&inst, 1, nullptr, b1->getSExtValue() + b2->getSExtValue() );
        // [xi := b + xj]
    }  else if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
        auto b = dyn_cast<ConstantInt>(op1);
        return affineAssignment(&inst, 1, op2, b->getSExtValue());
        // [xi := xj + b]
    } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
        auto b = dyn_cast<ConstantInt>(op2);
        return affineAssignment(&inst, 1, op1, b->getSExtValue());
    } else {
        return affineAssignment(&inst, {{op1,1},{op2,1}}, 0);
    }
}

void AffineRelation::Sub(Instruction const& inst) {
    auto op1 = inst.getOperand(0);
    auto op2 = inst.getOperand(1);

    // [xi := bj - bk]
    if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
        auto b1 = dyn_cast<ConstantInt>(op1);
        auto b2 = dyn_cast<ConstantInt>(op2);
        return affineAssignment(&inst, 1, nullptr, b1->getSExtValue() - b2->getSExtValue() );
    // [xi := b - xj]
    } else if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
        auto b = dyn_cast<ConstantInt>(op1);
        return affineAssignment(&inst, 1, op2, -b->getSExtValue());
    // [xi := xj - b]
    } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
        auto b = dyn_cast<ConstantInt>(op2);
        return affineAssignment(&inst, 1, op1, -b->getSExtValue());
    } else {
        return affineAssignment(&inst, {{op1,1},{op2,1}}, 0);
    }
}

void AffineRelation::Mul(Instruction const& inst) {
    auto op1 = inst.getOperand(0);
    auto op2 = inst.getOperand(1);

    // [xi := aj * ak]
    if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
        auto b1 = dyn_cast<ConstantInt>(op1);
        auto b2 = dyn_cast<ConstantInt>(op2);
        return affineAssignment(&inst, 1, nullptr, b1->getSExtValue() * b2->getSExtValue() );
    // [xi := a * xj]
    } else if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
        auto a = dyn_cast<ConstantInt>(op1);
        return affineAssignment(&inst, a->getSExtValue(), op2, 0);
    // [xi := xj * a]
    } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
        auto a = dyn_cast<ConstantInt>(op2);
        return affineAssignment(&inst, a->getSExtValue(), op1, 0);
    } else {
        return nonDeterminsticAssignment(&inst);
    }
}

// MARK: - Helpers

unordered_map<Value const*, int> createVariableIndexMap_impl(Function const& func, int& count, set<Function const*>& visited_funcs) {
    unordered_map<Value const*, int> map;
    visited_funcs.insert(&func);
    for (BasicBlock const& basic_block: func) {
        for (Instruction const& inst: basic_block) {
            if (isa<IntegerType>(inst.getType()) || isa<ReturnInst>(&inst)) {
                count++;
                map[&inst] = count;
            }
            if (CallInst const* call = dyn_cast<CallInst>(&inst)) {
                Function const* callee_func = call->getCalledFunction();
                if (callee_func->empty()) {
                    continue;
                }
                if (visited_funcs.count(callee_func) == 0) {
                    for (Argument const& arg: callee_func->args()) {
                        if (isa<IntegerType>(arg.getType())) {
                            count++;
                            map[&arg] = count;
                        }
                    }
                    unordered_map<Value const*, int> callee_map = createVariableIndexMap_impl(*callee_func, count, visited_funcs);
                    map.merge(callee_map);
                }
            } 
        }
    }
    return map;
}

unordered_map<Value const*, int> AffineRelation::createVariableIndexMap(Function const& func) {
    int count = 0;
    set<Function const*> visited_funcs = {};
    return createVariableIndexMap_impl(func, count, visited_funcs);
}

// MARK: - debug output

unordered_map<int,Value const*> reverseMap(unordered_map<Value const*, int> const& map) {
    unordered_map<int,Value const*> reversed;
    for (auto [key, value]: map) {
        reversed[value] = key;
    }
    return reversed;
}

void AffineRelation::printIncoming(BasicBlock const& bb, raw_ostream& out, int indentation) const {
    out << *this;
}

void AffineRelation::printOutgoing(BasicBlock const& bb, raw_ostream& out, int indentation) const {
    out << *this;
}

void AffineRelation::debug_output(Instruction const& inst, Matrix<T> operands) {
    dbgs(3) << *this;
}


raw_ostream& operator<<(raw_ostream& os, AffineRelation const& relation) {
    auto reversed = reverseMap(relation.index);
    if (relation.basis.empty()) {
        return os << "[]\n";
    }
    for (auto m: relation.basis) {
        os << left_justify("", 8);
        for (int i = 1; i <= int(relation.index.size()); i++) {
            auto val = reversed.at(i);
            if (val->hasName()) {
                os << left_justify(val->getName(), 6);
            } else {
                os << left_justify("<>", 6);
            }
        }
        os << "\n" << m << "\n";
    }
    return os;
}

}
