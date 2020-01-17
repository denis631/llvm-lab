#include "normalized_conjunction.h"
#include <algorithm>
#include <set>

namespace pcpo {

using namespace llvm;

// MARK: Initializers

NormalizedConjunction::NormalizedConjunction(Constant const& constant) {
    if (ConstantInt const* c = dyn_cast<ConstantInt>(&constant)) {
        state = NORMAL;
        Equality eq = {&constant, 1, nullptr, c->getValue().getSExtValue()};
        equalaties = {{&constant,eq}};
    } else {
        state = TOP;
    }
}

NormalizedConjunction::NormalizedConjunction(std::map<Value const*, Equality> equalaties) {
    this->equalaties = equalaties;
    state = equalaties.empty() ? TOP : NORMAL;
}

// MARK: AbstractDomain interface

NormalizedConjunction NormalizedConjunction::interpret(
    Instruction const& inst, std::vector<NormalizedConjunction> const& operands
) {
    NormalizedConjunction a = operands[0];
    NormalizedConjunction b = operands[1];
    
    if (operands.size() != 2) return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(a,b), &inst);

    // We only deal with integer types
    IntegerType const* type = dyn_cast<IntegerType>(inst.getType());
    // FIXME: maybe use nonDeterministic assignment here.
    if (not type) return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(a,b), &inst);
    
    type = dyn_cast<IntegerType>(inst.getOperand(0)->getType());
    // FIXME: maybe use nonDeterministic assignment here.
    if (not type) return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(a,b), &inst);
    
    type = dyn_cast<IntegerType>(inst.getOperand(1)->getType());
    // FIXME: maybe use nonDeterministic assignment here.
    if (not type) return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(a,b), &inst);
    
    unsigned bitWidth = inst.getOperand(0)->getType()->getIntegerBitWidth();
    assert(bitWidth == inst.getOperand(1)->getType()->getIntegerBitWidth());
    
    switch (inst.getOpcode()) {
        case Instruction::Add:
            return NormalizedConjunction::Add(inst, a, b);
        case Instruction::Sub:
            return NormalizedConjunction::Sub(inst, a, b);
        case Instruction::Mul:
            return NormalizedConjunction::Mul(inst, a, b);
        case Instruction::SDiv:
        case Instruction::UDiv:
        default:
            return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(a,b), &inst);
    }
}

NormalizedConjunction NormalizedConjunction::refineBranch(CmpInst::Predicate pred, Value const& lhs, Value const& rhs, NormalizedConjunction a, NormalizedConjunction b) {
    // Do nothing
    return a;
}


NormalizedConjunction NormalizedConjunction::merge(Merge_op::Type op, NormalizedConjunction a, NormalizedConjunction b) {
//    if (a.isBottom()) return b;
//    if (b.isBottom()) return a;
//    if (a.isTop() || b.isTop()) return NormalizedConjunction {true};

    // Note that T is handled above, so no need to convert the inputs
    
    switch (op) {
    case Merge_op::UPPER_BOUND:
        return leastUpperBound(a, b);
    case Merge_op::WIDEN:
        assert(false && "not implemented");
    case Merge_op::NARROW:
        assert(false && "not implmented");
    default:
        assert(false && "invalid op value");
        return NormalizedConjunction {true};
    }
}

// MARK: Helpers

/// XO / E'0: set of variables where the right hand side in E1 and E2 coincide
std::set<NormalizedConjunction::Equality> NormalizedConjunction::computeX0(std::set<Equality> const& E1, std::set<Equality> const& E2) {
    std::set<Equality> X0;
    std::set_intersection(E1.begin(), E1.end(), E2.begin(), E2.end(), std::inserter(X0, X0.end()));
    // Remove trivial equalities
    std::set<Equality> filteredX0;
    auto filterTrivialEqualaties = [](Equality eq){ return eq.y != eq.x;};
    copy_if(X0, std::inserter(filteredX0, filteredX0.end()), filterTrivialEqualaties);
    
    return filteredX0;
}

/// X1 / E'1: set of variables where the right hand side is constant but does not coincide in E1 and E2
std::set<NormalizedConjunction::Equality> NormalizedConjunction::computeX1(std::set<Equality> const& E1, std::set<Equality> const& E2) {
    std::set<Equality> X1;
    std::set<std::pair<Equality, Equality>> differentConstants;
    
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
        std::pair<Equality, Equality> h = *differentConstants.begin();
        differentConstants.erase(differentConstants.begin());
        
        for (auto i: differentConstants) {
            // find a linear equation that contains both points P1(c(1)i, c(1)h) and P2(c(2)i, c(2)h)
            // y = a * x + b
            auto y = i.first.y;
            int64_t a = ((i.second.b - i.first.b)) / (h.second.b - h.first.b);
            auto x = h.first.y;
            int64_t b = -a * h.first.b + i.first.b;
            Equality eq = {y, a, x, b};
            X1.insert(eq);
        }
    }
    return X1;
}

/// X2 / E'2: set of variables where the right hand side of E1 is constant but the rhs of E2 contains a variable.
std::set<NormalizedConjunction::Equality> NormalizedConjunction::computeX2(std::set<Equality> const& E1, std::set<Equality> const& E2) {
    std::set<Equality> X2;
    std::set<std::pair<Equality, Equality>> differentConstants;
    
    assert(E1.size() == E2.size() && "E1 and E2 should have the same set of variables in the same order");
    
    for (auto itE1 = E1.begin(), itE2 = E2.begin(); itE1 != E1.end() && itE2 != E2.end(); ++itE1, ++itE2) {
        auto eq1 = *itE1;
        auto eq2 = *itE2;
        assert(eq1.y == eq2.y && "left hand side of equations should be the same");
        if (eq1.isConstant() && !eq2.isConstant()) {
            differentConstants.insert({eq1,eq2});
        }
    }
    
    std::vector<std::set<std::pair<Equality, Equality>>> Pi2;
    
    for (auto jt = differentConstants.begin(); jt != differentConstants.end();) {
        std::set<std::pair<Equality, Equality>> equivalenceClass;
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
            Equality eq = {y, a, x, b};
            X2.insert(eq);
        }
    }
    return X2;
}

std::set<NormalizedConjunction::Equality> NormalizedConjunction::computeX4(std::set<Equality> const& E1, std::set<Equality> const& E2) {
    std::set<Equality> X4;
    std::set<std::pair<Equality, Equality>> differentConstants;
    
    assert(E1.size() == E2.size() && "E1 and E2 should have the same set of variables in the same order");
    
    for (auto itE1 = E1.begin(), itE2 = E2.begin(); itE1 != E1.end() && itE2 != E2.end(); ++itE1, ++itE2) {
        auto eq1 = *itE1;
        auto eq2 = *itE2;
        assert(eq1.y == eq2.y && "left hand side of equations should be the same");
        if (!eq1.isConstant() && !eq2.isConstant() && eq1 != eq2) {
            differentConstants.insert({eq1,eq2});
        }
    }
    
    std::vector<std::set<std::pair<Equality, Equality>>> Pi4;
    
    // partition differentConstants
    for (auto it = differentConstants.begin(); it != differentConstants.end();) {
        std::set<std::pair<Equality, Equality>> equivalenceClass;
        std::pair<Equality, Equality> i = *it;
        equivalenceClass.insert(i);
        it = differentConstants.erase(it);
        
        for (auto jt = it; jt != differentConstants.end(); ) {
            std::pair<Equality, Equality> j = *jt;
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
            Equality eq = {y, a, x, b};
            X4.insert(eq);
        }
    }
    return X4;
}

// MARK: Lattice Operations
    
NormalizedConjunction NormalizedConjunction::leastUpperBound(NormalizedConjunction lhs, NormalizedConjunction rhs) {
    // set of all occuring variables in E1 and E2
    std::set<Value const*> vars, varsE1, varsE2;
    std::set<Equality> E1, E2;

    auto mapToSeccond = [](std::pair<Value const*,Equality> p){ return p.second; };
    transform(lhs.equalaties, std::inserter(E1, E1.end()), mapToSeccond);
    transform(rhs.equalaties, std::inserter(E2, E2.end()), mapToSeccond);
    
    auto mapToY = [](Equality eq){ return eq.y; };
    transform(E1, std::inserter(varsE1, varsE1.end()), mapToY);
    transform(E2, std::inserter(varsE2, varsE2.end()), mapToY);
    std::set_union(varsE1.begin(), varsE1.end(), varsE2.begin(), varsE2.end(), std::inserter(vars, vars.end()));
    
    std::set<Value const*> dX1, dX2;
    
    std::set_difference(vars.begin(), vars.end(), varsE1.begin(), varsE1.end(), std::inserter(dX1, dX1.end()));
    std::set_difference(vars.begin(), vars.end(), varsE2.begin(), varsE2.end(), std::inserter(dX2, dX2.end()));
        
    // extend E1 by trivial equalities
    for (auto d: dX1) {
        Equality eq = {d, 1, d, 0};
        E1.insert(eq);
    }
    
    // extend E2 by trivial equalities
    for (auto d: dX2) {
        Equality eq = {d, 1, d, 0};
        E2.insert(eq);
    }
    
    std::set<Equality> X0 = computeX0(E1, E2);
    std::set<Equality> X1 = computeX1(E1, E2);
    std::set<Equality> X2 = computeX2(E1, E2);
    std::set<Equality> X3 = computeX2(E2, E1);
    std::set<Equality> X4 = computeX4(E1, E2);
    
    // E1 U E2 = E'0 AND E'1 AND E'2 AND E'3 AND E'4
    
    std::set<Equality> leastUpperBound;
    leastUpperBound.insert(X0.begin(), X0.end());
    leastUpperBound.insert(X1.begin(), X1.end());
    leastUpperBound.insert(X2.begin(), X2.end());
    leastUpperBound.insert(X3.begin(), X3.end());
    leastUpperBound.insert(X4.begin(), X4.end());
    
    std::map<Value const*, Equality> result;
    
    auto addMapping = [](Equality eq){ return std::make_pair(eq.y,eq); };
    transform(leastUpperBound, std::inserter(result, result.end()), addMapping);

    return NormalizedConjunction(result);
}

// MARK: Abstract Assignments

/// [xi := ?]
NormalizedConjunction NormalizedConjunction::nonDeterminsticAssignment(NormalizedConjunction E, Value const* xi) {
    assert(xi != nullptr && "xi cannot be NULL");
    auto xj = E.equalaties[xi].x;
    
    if (xi != xj && xj != 0) {
        E.equalaties[xi] = {xi, 1, xi, 0};
    } else {
        // find all equations using xi
        auto predicate = [&xi](std::pair<Value const*, Equality> p){ return p.second.x == xi && p.second.y != xi ;};
        auto it = std::find_if(E.equalaties.begin(), E.equalaties.end(), predicate);
        if (it != E.equalaties.end()) {
            auto xk = (*it).second;
            for (it = std::find_if(++it, E.equalaties.end(), predicate);
                 it != E.equalaties.end();
                 it = std::find_if(++it, E.equalaties.end(), predicate)) {
                auto& xl = it->second;
                E.equalaties[xl.y] = {xl.y, 1, xk.y, xl.b - xk.b};
            }
            E.equalaties[xk.y] = {xk.y, 1, xk.y, 0};
        }
        E.equalaties[xi] = {xi, 1, xi, 0};
    }
    return E;
}

/// [xi := a * xj + b]
NormalizedConjunction NormalizedConjunction::linearAssignment(NormalizedConjunction E, Value const* xi, int64_t a, Value const* xj, int64_t b) {
    assert(xi != nullptr && "xi cannot be NULL");

    E = nonDeterminsticAssignment(E, xi);
    
    // make sure xj exists
    auto xjS = E.equalaties.find(xj) != E.equalaties.end() ? E.equalaties[xj].x : nullptr;
    auto bS = E.equalaties.find(xj) != E.equalaties.end() ? E.equalaties[xj].b : 0;
    auto aS = E.equalaties.find(xj) != E.equalaties.end() ? E.equalaties[xj].a : 1;
    
    if (!(a % aS == 0 && (-bS - b) % aS == 0)) {
        // Precison loss due to int division! Abort
        return E;
    }
    
    if (xi > xjS) {
        E.equalaties[xi] = {xi, aS * a, xjS, a * bS + b};
        return E;
    } else {
        auto pred = [&xjS](std::pair<Value const*, Equality> p){ return p.second.x == xjS && p.second.y != xjS; };
        for (auto xk: make_filter_range(E.equalaties, pred)) {
            E.equalaties[xk.second.y] = {xk.second.y, xk.second.a * a/aS, xi, (-bS - b) / aS + xk.second.b};
        }
        E.equalaties[xjS] = {xjS, a/aS, xi, (-bS - b) / aS};
    }
    return E;
}

// MARK: Abstract Operations

// [xi := xj + b]
// [xi := xj + xk]
// [xi := bj + bk]
NormalizedConjunction NormalizedConjunction::Add(Instruction const& inst,  NormalizedConjunction lhs, NormalizedConjunction rhs) {
    auto result = leastUpperBound(lhs, rhs);
    auto op1 = inst.getOperand(0);
    auto op2 = inst.getOperand(1);
    
    // [xi := b + xj]
    if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
        auto b = dyn_cast<ConstantInt>(op1);
        return linearAssignment(result, &inst, 1, op2, b->getSExtValue());
    // [xi := xj + b]
    } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
        auto b = dyn_cast<ConstantInt>(op2);
        return linearAssignment(result, &inst, 1, op1, b->getSExtValue());
    // [xi := xj + xk]
    } else if (isa<Value>(op1) && isa<Value>(op2)) {
        // [xi := bj + xk]
        if (result.equalaties[op1].isConstant()) {
            return linearAssignment(result, &inst, 1, op2, result.equalaties[op1].b);
        // [xi := xj + bk]
        } else if (result.equalaties[op2].isConstant()) {
            return linearAssignment(result, &inst, 1, op1, result.equalaties[op2].b);
        // [xi := xj + xk]
        } else {
            return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(lhs,rhs), &inst);
        }
    // [xi := bj + bk]
    } else if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
        return linearAssignment(result, &inst, 1, nullptr, result.equalaties[op1].b + result.equalaties[op2].b);
    } else {
        assert(false);
        return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(lhs,rhs), &inst);
    }
}

// TODO: [xi := xj - x]
NormalizedConjunction NormalizedConjunction::Sub(Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs) {
   auto result = leastUpperBound(lhs, rhs);
   auto op1 = inst.getOperand(0);
   auto op2 = inst.getOperand(1);
   
   // [xi := b - xj]
   if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
       auto b = dyn_cast<ConstantInt>(op1);
       return linearAssignment(result, &inst, 1, op2, -b->getSExtValue());
   // [xi := xj - b]
   } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
       auto b = dyn_cast<ConstantInt>(op2);
       return linearAssignment(result, &inst, 1, op1, -b->getSExtValue());
   // [xi := xj - xk]
   } else if (isa<Value>(op1) && isa<Value>(op2)) {
       // [xi := bj - xk]
       if (result.equalaties[op1].isConstant()) {
           return linearAssignment(result, &inst, 1, op2, -result.equalaties[op1].b);
       // [xi := xj - bk]
       } else if (result.equalaties[op2].isConstant()) {
           return linearAssignment(result, &inst, 1, op1, -result.equalaties[op2].b);
       // [xi := xj - xk]
       } else {
           return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(lhs,rhs), &inst);
       }
   // [xi := bj + bk]
   } else if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
       return linearAssignment(result, &inst, 1, nullptr, result.equalaties[op1].b - result.equalaties[op2].b);
   } else {
       assert(false);
       return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(lhs,rhs), &inst);
   }
}

// [xi := a * xj]
NormalizedConjunction NormalizedConjunction::Mul(Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs) {
    auto result = leastUpperBound(lhs, rhs);
    auto op1 = inst.getOperand(0);
    auto op2 = inst.getOperand(1);
    
    // [xi := a * xj]
    if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
        auto a = dyn_cast<ConstantInt>(op1);
        return linearAssignment(result, &inst, a->getSExtValue(), op2, 0);
    // [xi := xj * a]
    } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
        auto a = dyn_cast<ConstantInt>(op2);
        return linearAssignment(result, &inst, a->getSExtValue(), op1, 0);
    // [xi := xj * xk]
    } else if (isa<Value>(op1) && isa<Value>(op2)) {
        // [xi := aj * xk]
        if (result.equalaties[op1].isConstant()) {
            return linearAssignment(result, &inst, result.equalaties[op1].b, op2, 0);
        // [xi := xj * ak]
        } else if (result.equalaties[op2].isConstant()) {
            return linearAssignment(result, &inst, result.equalaties[op1].b, op1, 0);
        // [xi := xj * xk]
        } else {
            return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(lhs,rhs), &inst);
        }
    // [xi := aj * ak]
    } else if (isa<ConstantInt>(op1) && (isa<ConstantInt>(op2))) {
        return linearAssignment(result, &inst, 1, nullptr, result.equalaties[op1].b * result.equalaties[op2].b);
    } else {
        assert(false);
        return nonDeterminsticAssignment(NormalizedConjunction::leastUpperBound(lhs,rhs), &inst);
    }
}

// MARK: Utils

bool NormalizedConjunction::isNormalized() const {
    bool result = true;
    for (auto eq: equalaties) {
        if (eq.second.isConstant()) {
            auto containsY = [&eq](std::pair<Value const*, Equality> pair){ return pair.second.x == eq.second.y; };
            result &= none_of(equalaties, containsY);
        } else {
            auto occursElseWhere = [&eq](std::pair<Value const*, Equality> pair){ return eq.second.y != pair.second.y && eq.second.y == pair.second.x; };
            result &= none_of(equalaties, occursElseWhere);
        }
    }
    
    auto jGreaterI = [](std::pair<Value const*, Equality> pair){ return pair.second.y > pair.second.x;};
    result &= all_of(equalaties, jGreaterI);
    return result;
}

// MARK: Operators

bool NormalizedConjunction::operator==(NormalizedConjunction rhs) const {
    return getState() == NORMAL
        ? rhs.getState() == NORMAL and equalaties == rhs.equalaties
        : getState() == rhs.getState();
}

raw_ostream& operator<<(raw_ostream& os, NormalizedConjunction a) {
    if (a.isBottom()) {
        os << "[]";
    } else if(a.isTop()) {
        os << "T";
    } else {
        if (a.equalaties.size() > 0) {
            os << "{ ";
        } else {
            os << "{ }";
        }
        for (auto it = a.equalaties.begin(); it != a.equalaties.end(); it++) {
            auto eq = *it;
            if (eq.second.y != nullptr && eq.second.y->hasName()) {
                os << eq.second.y->getName() << " = ";
            } else if (eq.second.y != nullptr) {
                os << eq.second.y << " = ";
            } else {
                os << "<null> = ";
            }
            
            if (eq.second.x != nullptr) {
                if (eq.second.x->hasName()) {
                    os << eq.second.a << " * " << eq.second.x->getName();
                } else {
                    os << eq.second.a << " * " << eq.second.x;
                }
                if (eq.second.b > 0) {
                    os << " + " << eq.second.b;
                } else if (eq.second.b < 0) {
                    os << eq.second.b;
                }
            } else {
                os << eq.second.b;
            }
        
            if (std::next(it) == a.equalaties.end()) {
                os << " }";
            } else {
                os << ", ";
            }
        }
    }
    return os;
}

}
