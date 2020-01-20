//
//  normalized_conjunction.cpp
//  ADTTests
//
//  Created by Tim Gymnich on 17.1.20.
//

#include "normalized_conjunction.h"

using namespace llvm;

namespace pcpo {

// MARK: - Initializers

NormalizedConjunction::NormalizedConjunction(llvm::Function const& f) {
    for (llvm::Argument const& arg: f.args()) {
        values[&arg] = LinearEquality(&arg);
    }
}

NormalizedConjunction::NormalizedConjunction(llvm::Function const* callee_func, NormalizedConjunction const& state, llvm::CallInst const* call) {
    assert(callee_func->arg_size() == call->getNumArgOperands());
    for (llvm::Argument const& arg: callee_func->args()) {
        llvm::Value* value = call->getArgOperand(arg.getArgNo());
        if (value->getType()->isIntegerTy()) {
            if (llvm::ConstantInt const* c = llvm::dyn_cast<llvm::ConstantInt>(value)) {
                values[&arg] = LinearEquality(c);
            } else {
                values[&arg] = state.values.at(value);
            }
        }
    }
}

NormalizedConjunction::NormalizedConjunction(std::unordered_map<llvm::Value const*, LinearEquality> equalaties) {
    this->values = equalaties;
}


// MARK: - AbstractState Interface

/// Handles the evaluation of merging points
void NormalizedConjunction::applyPHINode(llvm::BasicBlock const& bb, std::vector<NormalizedConjunction> pred_values,
                  llvm::PHINode const *phi) {
    std::vector<NormalizedConjunction> operands;

    // Phi nodes are handled here, to get the precise values of the predecessors
    for (auto& incoming_state: pred_values) {
        merge(Merge_op::UPPER_BOUND, incoming_state);
        operands.push_back(incoming_state); // Keep the debug output happy
    }
}

void NormalizedConjunction::applyCallInst(llvm::Instruction const& inst, llvm::BasicBlock const* end_block, NormalizedConjunction const& callee_state) {
    std::vector<LinearEquality> operands;

//    // Keep the debug output happy
//    for (llvm::Value const* value : inst.operand_values()) {
//        operands.push_back(EqualityFoo(value));
//    }
    
    //iterate through all instructions of it till we find a return statement
    for (auto& iter_inst: *end_block) {
        if (llvm::ReturnInst const* ret_inst = llvm::dyn_cast<llvm::ReturnInst>(&iter_inst)) {
            llvm::Value const* ret_val = ret_inst->getReturnValue();
            dbgs(4) << "\t\tFound return instruction\n";
            if (callee_state.values.find(ret_val) != callee_state.values.end()) {
                dbgs(4) << "\t\tReturn evaluated, merging parameters\n";
                values[&inst] = callee_state.values.at(ret_val);
            } else {
                dbgs(4) << "\t\tReturn not evaluated, setting to bottom\n";
            }
        }
    }
//    debug_output(inst, operands);
}

void NormalizedConjunction::applyReturnInst(llvm::Instruction const& inst) {
    llvm::Value const* ret_val = llvm::dyn_cast<llvm::ReturnInst>(&inst)->getReturnValue();
    if (ret_val->getType()->isIntegerTy()) {
        if (llvm::ConstantInt const* c = llvm::dyn_cast<llvm::ConstantInt>(ret_val)) {
            values[&inst] = LinearEquality(c);
        } else if (values.find(ret_val) != values.end()) {
            values[&inst] = values.at(ret_val);
        }
    }
}

void NormalizedConjunction::applyDefault(llvm::Instruction const& inst) {
    std::vector<LinearEquality> operands;
    
    if (inst.getNumOperands() != 2) return nonDeterminsticAssignment(&inst);

    // We only deal with integer types
    IntegerType const* type = dyn_cast<IntegerType>(inst.getType());
    if (not type) return nonDeterminsticAssignment(&inst);
    
    type = dyn_cast<IntegerType>(inst.getOperand(0)->getType());
    if (not type) return nonDeterminsticAssignment(&inst);
    
    type = dyn_cast<IntegerType>(inst.getOperand(1)->getType());
    if (not type) return nonDeterminsticAssignment(&inst);
    
    for (llvm::Value const* value: inst.operand_values()) {
        operands.push_back(LinearEquality(value));
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

    debug_output(inst, operands);
}

bool NormalizedConjunction::merge(Merge_op::Type op, NormalizedConjunction const& other) {
    
    switch (op) {
        case Merge_op::UPPER_BOUND: return leastUpperBound(other);
        default: abort();
    }
}

// MARK: Lattice Operations

bool NormalizedConjunction::leastUpperBound(NormalizedConjunction rhs) {
    // set of all occuring variables in E1 and E2
    std::set<Value const*> vars, varsE1, varsE2;
    std::set<LinearEquality> E1, E2;

    auto mapToSeccond = [](std::pair<Value const*, LinearEquality> p){ return p.second; };
    transform(values, std::inserter(E1, E1.end()), mapToSeccond);
    transform(rhs.values, std::inserter(E2, E2.end()), mapToSeccond);
    
    auto mapToY = [](LinearEquality eq){ return eq.y; };
    transform(E1, std::inserter(varsE1, varsE1.end()), mapToY);
    transform(E2, std::inserter(varsE2, varsE2.end()), mapToY);
    std::set_union(varsE1.begin(), varsE1.end(), varsE2.begin(), varsE2.end(), std::inserter(vars, vars.end()));
    
    std::set<Value const*> dX1, dX2;
    
    std::set_difference(vars.begin(), vars.end(), varsE1.begin(), varsE1.end(), std::inserter(dX1, dX1.end()));
    std::set_difference(vars.begin(), vars.end(), varsE2.begin(), varsE2.end(), std::inserter(dX2, dX2.end()));
            
    // extend E1 by trivial equalities
    for (auto d: dX1) {
        LinearEquality eq = {d, 1, d, 0};
        E1.insert(eq);
    }
    
    // extend E2 by trivial equalities
    for (auto d: dX2) {
        LinearEquality eq = {d, 1, d, 0};
        E2.insert(eq);
    }
    
    std::set<LinearEquality> X0 = computeX0(E1, E2);
    std::set<LinearEquality> X1 = computeX1(E1, E2);
    std::set<LinearEquality> X2 = computeX2(E1, E2);
    std::set<LinearEquality> X3 = computeX2(E2, E1);
    std::set<LinearEquality> X4 = computeX4(E1, E2);
    
    // E1 U E2 = E'0 AND E'1 AND E'2 AND E'3 AND E'4
    
    std::set<LinearEquality> leastUpperBound;
    leastUpperBound.insert(X0.begin(), X0.end());
    leastUpperBound.insert(X1.begin(), X1.end());
    leastUpperBound.insert(X2.begin(), X2.end());
    leastUpperBound.insert(X3.begin(), X3.end());
    leastUpperBound.insert(X4.begin(), X4.end());
    
    std::unordered_map<Value const*, LinearEquality> result;
    
    auto addMapping = [](LinearEquality eq){ return std::make_pair(eq.y,eq); };
    transform(leastUpperBound, std::inserter(result, result.end()), addMapping);

    bool changed = values != result;
    
    values = result;

    return changed;
}

/// XO / E'0: set of variables where the right hand side in E1 and E2 coincide
std::set<LinearEquality> NormalizedConjunction::computeX0(std::set<LinearEquality> const& E1, std::set<LinearEquality> const& E2) {
    std::set<LinearEquality> X0;
    std::set_intersection(E1.begin(), E1.end(), E2.begin(), E2.end(), std::inserter(X0, X0.end()));
    // Remove trivial equalities
    std::set<LinearEquality> filteredX0;
    auto filterTrivialEqualaties = [](LinearEquality eq){ return eq.y != eq.x;};
    copy_if(X0, std::inserter(filteredX0, filteredX0.end()), filterTrivialEqualaties);
    
    return filteredX0;
}

/// X1 / E'1: set of variables where the right hand side is constant but does not coincide in E1 and E2
std::set<LinearEquality> NormalizedConjunction::computeX1(std::set<LinearEquality> const& E1, std::set<LinearEquality> const& E2) {
    std::set<LinearEquality> X1;
    std::set<std::pair<LinearEquality, LinearEquality>> differentConstants;
    
    assert(E1.size() == E2.size() && "E1 and E2 should have the same set of variables in the same order");
    
    for (auto itE1 = E1.begin(), itE2 = E2.begin(); itE1 != E1.end() && itE2 != E2.end(); ++itE1, ++itE2) {
        auto eq1 = *itE1;
        auto eq2 = *itE2;
        assert(eq1.y == eq2.y && "left hand side of equations should be the same");
        if (eq1.isConstant() && eq2.isConstant() && eq1.b != eq2.b) {
            differentConstants.insert({eq1,eq2});
        }
    }
    
    if (!differentConstants.empty()) {
        // pop first element
        std::pair<LinearEquality, LinearEquality> h = *differentConstants.begin();
        differentConstants.erase(differentConstants.begin());
        
        for (auto i: differentConstants) {
            // find a linear equation that contains both points P1(c(1)i, c(1)h) and P2(c(2)i, c(2)h)
            // y = a * x + b
            auto y = i.first.y;
            int64_t a = ((i.second.b - i.first.b)) / (h.second.b - h.first.b);
            auto x = h.first.y;
            int64_t b = -a * h.first.b + i.first.b;
            LinearEquality eq = {y, a, x, b};
            X1.insert(eq);
        }
    }
    return X1;
}

/// X2 / E'2: set of variables where the right hand side of E1 is constant but the rhs of E2 contains a variable.
std::set<LinearEquality> NormalizedConjunction::computeX2(std::set<LinearEquality> const& E1, std::set<LinearEquality> const& E2) {
    std::set<LinearEquality> X2;
    std::set<std::pair<LinearEquality, LinearEquality>> differentConstants;
    
    assert(E1.size() == E2.size() && "E1 and E2 should have the same set of variables in the same order");
    
    for (auto itE1 = E1.begin(), itE2 = E2.begin(); itE1 != E1.end() && itE2 != E2.end(); ++itE1, ++itE2) {
        auto eq1 = *itE1;
        auto eq2 = *itE2;
        assert(eq1.y == eq2.y && "left hand side of equations should be the same");
        if (eq1.isConstant() && !eq2.isConstant()) {
            differentConstants.insert({eq1,eq2});
        }
    }
    
    std::vector<std::set<std::pair<LinearEquality, LinearEquality>>> Pi2;
    
    for (auto jt = differentConstants.begin(); jt != differentConstants.end();) {
        std::set<std::pair<LinearEquality, LinearEquality>> equivalenceClass;
        auto j = *jt;
        equivalenceClass.insert(j);
        jt = differentConstants.erase(jt);
        
        // partition differentConstants
        for (auto it = jt; it != differentConstants.end(); ) {
            auto i = *it;
            bool condition1 = i.second.x == j.second.x;
            bool condition2 = (i.first.b - i.second.b) / (i.second.a) == (j.first.b - j.second.b) / (j.second.a);
            if (condition1 && condition2) {
                equivalenceClass.insert(i);
                it = differentConstants.erase(it);
                jt = differentConstants.begin();
            } else {
                it++;
            }
        }
                
        Pi2.push_back(equivalenceClass);
    }
    
    // form equaltites for partitions in Pi2
    for (auto q: Pi2) {
        auto h = *q.begin();
        q.erase(q.begin());
        for (auto i: q) {
            // xi = ai/ah * xh + ( bi - (ai * bh) / ah)
            auto y = i.first.y;
            auto a = i.second.a / h.second.a;
            auto x = h.first.y;
            auto b = i.second.b - (i.second.a * h.second.b) / h.second.a;
            LinearEquality eq = {y, a, x, b};
            X2.insert(eq);
        }
    }
    return X2;
}

std::set<LinearEquality> NormalizedConjunction::computeX4(std::set<LinearEquality> const& E1, std::set<LinearEquality> const& E2) {
    std::set<LinearEquality> X4;
    std::set<std::pair<LinearEquality, LinearEquality>> differentConstants;
    
    assert(E1.size() == E2.size() && "E1 and E2 should have the same set of variables in the same order");
    
    for (auto itE1 = E1.begin(), itE2 = E2.begin(); itE1 != E1.end() && itE2 != E2.end(); ++itE1, ++itE2) {
        auto eq1 = *itE1;
        auto eq2 = *itE2;
        assert(eq1.y == eq2.y && "left hand side of equations should be the same");
        if (!eq1.isConstant() && !eq2.isConstant() && eq1 != eq2) {
            differentConstants.insert({eq1,eq2});
        }
    }
    
    std::vector<std::set<std::pair<LinearEquality, LinearEquality>>> Pi4;
    
    // partition differentConstants
    for (auto it = differentConstants.begin(); it != differentConstants.end();) {
        std::set<std::pair<LinearEquality, LinearEquality>> equivalenceClass;
        std::pair<LinearEquality, LinearEquality> i = *it;
        equivalenceClass.insert(i);
        it = differentConstants.erase(it);
        
        for (auto jt = it; jt != differentConstants.end(); ) {
            std::pair<LinearEquality, LinearEquality> j = *jt;
            bool condition1 = i.first.x == j.first.x && i.second.x == j.second.x;
            bool condition2 = i.second.a / (i.first.a) == j.second.a / (j.first.a);
            bool condition3 = (i.first.b - i.second.b) / (i.first.a) == (j.first.b - j.second.b) / (j.first.a);
            if (condition1 && condition2 && condition3) {
                equivalenceClass.insert(j);
                jt = differentConstants.erase(jt);
                it = differentConstants.begin();
            } else {
                jt++;
            }
        }

        Pi4.push_back(equivalenceClass);
    }
    
    // form equaltites for partitions in Pi4
    for (auto q: Pi4) {
        auto h = *q.begin();
        q.erase(q.begin());
        for (auto i: q) {
            // xi = ai/ah * xh + ( bi - (ai * bh) / ah)
            auto y = i.first.y;
            auto a = i.second.a / h.second.a;
            auto x = h.first.y;
            auto b = i.second.b - (i.second.a * h.second.b) / (h.second.a);
            LinearEquality eq = {y, a, x, b};
            X4.insert(eq);
        }
    }
    return X4;
}

// MARK: Abstract Assignments

/// [xi := ?]
void NormalizedConjunction::nonDeterminsticAssignment( Value const* xi) {
    assert(xi != nullptr && "xi cannot be NULL");
    auto xj = values[xi].x;
    
    if (xi != xj && xj != 0) {
        values[xi] = {xi, 1, xi, 0};
    } else {
        // find all equations using xi
        auto predicate = [&xi](std::pair<Value const*, LinearEquality> p){ return p.second.x == xi && p.second.y != xi ;};
        auto it = std::find_if(values.begin(), values.end(), predicate);
        if (it != values.end()) {
            auto xk = (*it).second;
            for (it = std::find_if(++it, values.end(), predicate);
                 it != values.end();
                 it = std::find_if(++it, values.end(), predicate)) {
                auto& xl = it->second;
                values[xl.y] = {xl.y, 1, xk.y, xl.b - xk.b};
            }
            values[xk.y] = {xk.y, 1, xk.y, 0};
        }
        values[xi] = {xi, 1, xi, 0};
    }
}

/// [xi := a * xj + b]
void NormalizedConjunction::linearAssignment(Value const* xi, int64_t a, Value const* xj, int64_t b) {
    assert(xi != nullptr && "xi cannot be NULL");

    nonDeterminsticAssignment(xi);
    
    // make sure xj exists
    auto xjS = values.find(xj) != values.end() ? values[xj].x : nullptr;
    auto bS = values.find(xj) != values.end() ? values[xj].b : 0;
    auto aS = values.find(xj) != values.end() ? values[xj].a : 1;
    
    if (!(a % aS == 0 && (-bS - b) % aS == 0)) {
        // Precison loss due to int division! Abort
        return;
    }
    
    if (xi > xjS) {
        values[xi] = {xi, aS * a, xjS, a * bS + b};
        return;
    } else {
        auto pred = [&xjS](std::pair<Value const*, LinearEquality> p){ return p.second.x == xjS && p.second.y != xjS; };
        for (auto xk: make_filter_range(values, pred)) {
            values[xk.second.y] = {xk.second.y, xk.second.a * a/aS, xi, (-bS - b) / aS + xk.second.b};
        }
        values[xjS] = {xjS, a/aS, xi, (-bS - b) / aS};
    }
}

// MARK: Abstract Operations

// [xi := xj + b]
// [xi := xj + xk]
// [xi := bj + bk]
void NormalizedConjunction::Add(Instruction const& inst) {
    auto op1 = inst.getOperand(0);
    auto op2 = inst.getOperand(1);
    
    // [xi := bj + bk]
    if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
        auto b1 = dyn_cast<ConstantInt>(op1);
        auto b2 = dyn_cast<ConstantInt>(op2);
        return linearAssignment(&inst, 1, nullptr, b1->getSExtValue() + b2->getSExtValue() );
    // [xi := b + xj]
    }  else if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
        auto b = dyn_cast<ConstantInt>(op1);
        return linearAssignment(&inst, 1, op2, b->getSExtValue());
    // [xi := xj + b]
    } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
        auto b = dyn_cast<ConstantInt>(op2);
        return linearAssignment(&inst, 1, op1, b->getSExtValue());
    // [xi := xj + xk]
    } else if (isa<Value>(op1) && isa<Value>(op2)) {
        // [xi := bj + xk]
        if (values[op1].isConstant()) {
            return linearAssignment(&inst, 1, op2, values[op1].b);
        // [xi := xj + bk]
        } else if (values[op2].isConstant()) {
            return linearAssignment(&inst, 1, op1, values[op2].b);
        // [xi := xj + xk]
        } else {
            return nonDeterminsticAssignment(&inst);
        }
    // [xi := bj + bk]
    } else {
        assert(false);
        return nonDeterminsticAssignment(&inst);
    }
}

/// [xi := xj - x]
void NormalizedConjunction::Sub(Instruction const& inst) {
   auto op1 = inst.getOperand(0);
   auto op2 = inst.getOperand(1);
   
    // [xi := bj - bk]
    if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
        auto b1 = dyn_cast<ConstantInt>(op1);
        auto b2 = dyn_cast<ConstantInt>(op2);
        return linearAssignment(&inst, 1, nullptr, b1->getSExtValue() - b2->getSExtValue() );
    // [xi := b - xj]
    } else if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
       auto b = dyn_cast<ConstantInt>(op1);
       return linearAssignment(&inst, 1, op2, -b->getSExtValue());
   // [xi := xj - b]
   } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
       auto b = dyn_cast<ConstantInt>(op2);
       return linearAssignment(&inst, 1, op1, -b->getSExtValue());
   // [xi := xj - xk]
   } else if (isa<Value>(op1) && isa<Value>(op2)) {
       // [xi := bj - xk]
       if (values[op1].isConstant()) {
           return linearAssignment(&inst, 1, op2, -values[op1].b);
       // [xi := xj - bk]
       } else if (values[op2].isConstant()) {
           return linearAssignment(&inst, 1, op1, -values[op2].b);
       // [xi := xj - xk]
       } else {
           return nonDeterminsticAssignment(&inst);
       }
   } else {
       assert(false);
       return nonDeterminsticAssignment(&inst);
   }
}

// [xi := a * xj]
void NormalizedConjunction::Mul(Instruction const& inst) {
    auto op1 = inst.getOperand(0);
    auto op2 = inst.getOperand(1);
    
    // [xi := aj * ak]
    if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
        auto b1 = dyn_cast<ConstantInt>(op1);
        auto b2 = dyn_cast<ConstantInt>(op2);
        return linearAssignment(&inst, 1, nullptr, b1->getSExtValue() * b2->getSExtValue() );
    // [xi := a * xj]
    } else if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
        auto a = dyn_cast<ConstantInt>(op1);
        return linearAssignment(&inst, a->getSExtValue(), op2, 0);
    // [xi := xj * a]
    } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
        auto a = dyn_cast<ConstantInt>(op2);
        return linearAssignment(&inst, a->getSExtValue(), op1, 0);
    // [xi := xj * xk]
    } else if (isa<Value>(op1) && isa<Value>(op2)) {
        // [xi := aj * xk]
        if (values[op1].isConstant()) {
            return linearAssignment(&inst, values[op1].b, op2, 0);
        // [xi := xj * ak]
        } else if (values[op2].isConstant()) {
            return linearAssignment(&inst, values[op1].b, op1, 0);
        // [xi := xj * xk]
        } else {
            return nonDeterminsticAssignment(&inst);
        }
    } else {
        assert(false);
        return nonDeterminsticAssignment(&inst);
    }
}

// MARK: Debug

void NormalizedConjunction::debug_output(llvm::Instruction const& inst, std::vector<LinearEquality> operands) {
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

void NormalizedConjunction::printIncoming(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {
    // @Speed: This is quadratic, could be linear
    bool nothing = true;
    for (std::pair<llvm::Value const*, LinearEquality> const& i: values) {
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

void NormalizedConjunction::printOutgoing(llvm::BasicBlock const& bb, llvm::raw_ostream& out, int indentation = 0) const {
    for (auto const& i: values) {
        if (llvm::ReturnInst::classof(i.first)) {
            out.indent(indentation) << "<ret> = " << i.second << '\n';
        } else {
            out.indent(indentation) << '%' << i.first->getName() << " = " << i.second << '\n';
        }
    }
}

} /* end of namespace pcpo */
