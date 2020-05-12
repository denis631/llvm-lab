#include "linear_subspace.h"
#include "global.h"

#include "llvm/IR/CFG.h"

#include <set>

using namespace llvm;
using std::vector;
using std::unordered_map;
using std::set;

namespace pcpo {

// MARK: - Initializers

LinearSubspace::LinearSubspace(Function const& func) {
    index = createVariableIndexMap(func);
    basis = {MatrixType(getNumberOfVariables() + 1)};
    isBottom = true;
}

LinearSubspace::LinearSubspace(Function const* callee_func, LinearSubspace const& state, CallInst const* call) {
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
        }
    }
    isBottom = true;
}

// MARK: - AbstractState Interface

void LinearSubspace::applyPHINode(BasicBlock const& bb, vector<LinearSubspace> const& pred_values, Instruction const& phi) {
    PHINode const* phiNode = dyn_cast<PHINode>(&phi);
    int i = 0;

    for (BasicBlock const* pred_bb: llvm::predecessors(&bb)) {
        auto& incoming_value = *phiNode->getIncomingValueForBlock(pred_bb);
        auto& incoming_state = pred_values[i];
        // Predecessor states should have been merged before. This is just bookkeeping.
        if (llvm::ConstantInt const* c = llvm::dyn_cast<llvm::ConstantInt>(&incoming_value)) {
            LinearSubspace acc = *this;
            acc.affineAssignment(&phi, 1, nullptr, c->getSExtValue());
            merge(Merge_op::UPPER_BOUND, acc);
        } else {
            LinearSubspace acc = *this;
            for (auto m: incoming_state.basis) {
                acc.affineAssignment(&phi, 1, &incoming_value, 0);
            }
            merge(Merge_op::UPPER_BOUND, acc);
        }
        i++;
    }
}

void LinearSubspace::applyCallInst(Instruction const& inst, BasicBlock const* end_block, LinearSubspace const& callee_state) {
    if (callee_state.isBottom) {
        isBottom = true;
    } else {
        basis = callee_state.basis;
    }
}

void LinearSubspace::applyReturnInst(Instruction const& inst) {
    Value const* ret_val = dyn_cast<llvm::ReturnInst>(&inst)->getReturnValue();
    if (ret_val && ret_val->getType()->isIntegerTy()) {
        if (ConstantInt const* c = dyn_cast<ConstantInt>(ret_val)) {
            affineAssignment(&inst, 1, nullptr, c->getSExtValue());
        } else {
            affineAssignment(&inst, 1, ret_val, 0);
        }
    } else {
        nonDeterminsticAssignment(&inst);
    }
}

void LinearSubspace::applyDefault(Instruction const& inst) {
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

bool LinearSubspace::merge(Merge_op::Type op, LinearSubspace const& other) {
    index.insert(other.index.begin(), other.index.end());

    if (isBottom && other.isBottom) {
        basis = other.basis;
        return false;
    } else if (isBottom && !other.isBottom) {
        basis = other.basis;
        isBottom = false;
        return true;
    } else if (!isBottom && other.isBottom) {
        return false;
    } else if (!isBottom && !other.isBottom) {
        switch (op) {
            case Merge_op::UPPER_BOUND: return leastUpperBound(other);
            default: abort();
        }
    }
}

// MARK: - Lattice Operations

bool LinearSubspace::leastUpperBound(LinearSubspace const& rhs) {
    assert(getNumberOfVariables() == rhs.getNumberOfVariables());
    vector<MatrixType> before = basis;
    vector<vector<T>> vectors;
    vectors.reserve(basis.size() + rhs.basis.size());
    for (MatrixType m: basis) {
        vectors.push_back(m.toVector());
    }

    for (MatrixType m: rhs.basis) {
        vectors.push_back(m.toVector());
    }

    if (vectors.empty()) {
        return false;
    }

    MatrixType combined = MatrixType(vectors);
    MatrixType result = MatrixType::span(combined, true);

    basis = result.reshapeColumns(getHeight(), getWidth());
    // FIXME: figure out a better way to detect changes
    return before != basis;
}

// MARK: - Assignments

// xi = a1x1 + ... + anxn + a0
void LinearSubspace::affineAssignment(Value const* xi, unordered_map<Value const*,T> relations, T constant) {
    MatrixType Wr = MatrixType(getNumberOfVariables() + 1);
    Wr.setValue(index.at(xi),index.at(xi), 0);
    Wr.setValue(0,index.at(xi), constant);

    for (auto [variable, factor]: relations) {
        Wr.setValue(index.at(variable),index.at(xi), factor);
    }

    // FIXME: this seems quite inefficient
    MatrixType vector = MatrixType(Wr.toVector());
    MatrixType vectorSpan = MatrixType::span(vector, true);
    Wr = vectorSpan.reshape(Wr.getHeight(), Wr.getWidth());


    if (basis.empty()) {
        basis.push_back(Wr);
    }

    for (MatrixType& matrix: basis) {
        matrix *= Wr;
    }
}

// xi = a * xj + b
void LinearSubspace::affineAssignment(Value const* xi, T a, Value const* xj, T b) {
    if (xj == nullptr) {
        affineAssignment(xi, {}, b);
    } else {
        affineAssignment(xi, {{xj,a}}, b);
    }
}

// xi = ?
void LinearSubspace::nonDeterminsticAssignment(Value const* xi) {
    return;
    if (index.count(xi) == 0) return;

    MatrixType T0 = MatrixType(getNumberOfVariables() + 1);
    MatrixType T1 = MatrixType(getNumberOfVariables() + 1);

    T0.setValue(index.at(xi),index.at(xi), 0);
    T0.setValue(0,index.at(xi), 0);

    T1.setValue(index.at(xi),index.at(xi), 0);
    T1.setValue(0,index.at(xi), 1);

    vector<vector<T>> assignment_vectors;
    assignment_vectors.push_back(T0.toVector());
    assignment_vectors.push_back(T1.toVector());

    MatrixType combined = MatrixType(assignment_vectors);
    MatrixType result = MatrixType::span(combined, true);

    vector<MatrixType> span = result.reshapeColumns(T0.getHeight(), T0.getWidth());

    if (basis.empty()) {
        basis = span;
    }

    for (MatrixType& matrix_state: basis) {
        for (MatrixType const& matrix_assignment: span) {
            matrix_state *= matrix_assignment;
        }
    }
}

// MARK: - Abstract Operators

void LinearSubspace::Add(Instruction const& inst) {
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

void LinearSubspace::Sub(Instruction const& inst) {
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

void LinearSubspace::Mul(Instruction const& inst) {
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

unordered_map<Value const*, int> LinearSubspace::createVariableIndexMap(Function const& func) {
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

void LinearSubspace::print() const {
    dbgs(3) << *this;
}

void LinearSubspace::printIncoming(BasicBlock const& bb, raw_ostream& out, int indentation) const {
    out << *this;
}

void LinearSubspace::printOutgoing(BasicBlock const& bb, raw_ostream& out, int indentation) const {
    MatrixType nullspace = MatrixType::null(MatrixType(this->basis));

    auto reversed = reverseMap(index);
    for (int i = 1; i <= int(index.size()); i++) {
        auto val = reversed.at(i);
        if (val->hasName()) {
            out << left_justify(val->getName(), 6);
        } else {
            out << left_justify("<>", 6);
        }
    }
    
    out << "\n" << nullspace;
}

void LinearSubspace::debug_output(Instruction const& inst, MatrixType operands) {
    dbgs(3) << *this;
}


raw_ostream& operator<<(raw_ostream& os, LinearSubspace const& relation) {
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
