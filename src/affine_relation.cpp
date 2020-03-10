#include "affine_relation.h"
#include "llvm/IR/CFG.h"

using namespace llvm;
using namespace std;

namespace pcpo {

// MARK: - Initializers

AffineRelation::AffineRelation(Function const& f) {
    numberOfVariables = 999; // FIXME: figure this out.
    matrix = Matrix<int>(numberOfVariables);
    for (Argument const& arg: f.args()) {
        index[&arg] = ++lastIndex;
    }
    isBottom = f.arg_empty();
}

AffineRelation::AffineRelation(Function const* callee_func, AffineRelation const& state, CallInst const* call) {
    // TODO
}

// MARK: - AbstractState Interface

void AffineRelation::applyPHINode(BasicBlock const& bb, vector<AffineRelation> pred_values, Instruction const& phi) {
    // TODO
}

void AffineRelation::applyCallInst(Instruction const& inst, BasicBlock const* end_block, AffineRelation const& callee_state) {
    // TODO
}

void AffineRelation::applyReturnInst(Instruction const& inst) {
    // TODO
}

void AffineRelation::applyDefault(Instruction const& inst) {
//    std::vector<LinearEquality> operands;

    if (inst.getNumOperands() != 2) return nonDeterminsticAssignment(&inst);

    // We only deal with integer types
    IntegerType const* type = dyn_cast<IntegerType>(inst.getType());
    if (not type) return nonDeterminsticAssignment(&inst);

    type = dyn_cast<IntegerType>(inst.getOperand(0)->getType());
    if (not type) return nonDeterminsticAssignment(&inst);

    type = dyn_cast<IntegerType>(inst.getOperand(1)->getType());
    if (not type) return nonDeterminsticAssignment(&inst);

//    for (llvm::Value const* value: inst.operand_values()) {
//        operands.push_back(getAbstractValue(*value));
//    }

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

//    debug_output(inst, operands);
}

//Matrix<int> AffineRelation::getAbstractValue(Value const& value) const {
//    if (llvm::Constant const* c = llvm::dyn_cast<llvm::Constant>(&value)) {
//        return Matrix(1,1,c->getSExtValue);
//    } else if (values.count(&value)) {
//        return values.at(&value);
//    } else if (isBottom) {
//        // If we are at bottom, there are no values
//        return Matrix();
//    } else {
//        // This branch should only catch 'weird' values, like things we are not handling
//        return AbstractDomain {true};
//    }
//}

bool AffineRelation::merge(Merge_op::Type op, AffineRelation const& other) {
    // TODO
}

// MARK: - Lattice Operations

bool AffineRelation::leastUpperBound(AffineRelation rhs) {
    // TODO
}

// MARK: - Assignments

void AffineRelation::affineAssignment(Value const* xi, int64_t a, Value const* xj, int64_t b) {
    Matrix<int> affineTransformer = Matrix<int>(numberOfVariables);
    affineTransformer(0,index[xi]) = b;
    affineTransformer(index[xi],index[xi]) = 0;
    if (xj != nullptr) {
        affineTransformer(index[xj],index[xi]) = a;
    }
    matrix *= affineTransformer;
}

void AffineRelation::nonDeterminsticAssignment(Value const* xi) {
    // TODO
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
        assert(false);
        return nonDeterminsticAssignment(&inst);
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
        assert(false);
        return nonDeterminsticAssignment(&inst);
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
        assert(false);
        return nonDeterminsticAssignment(&inst);
    }
}

// MARK: - debug output

void AffineRelation::printIncoming(BasicBlock const& bb, raw_ostream& out, int indentation) const {
    // TODO
}

void AffineRelation::printOutgoing(BasicBlock const& bb, raw_ostream& out, int indentation) const {
    // TODO
}

void AffineRelation::debug_output(Instruction const& inst, Matrix<int> operands) {
    // TODO
}

}
