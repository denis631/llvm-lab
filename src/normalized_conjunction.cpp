#include "normalized_conjunction.h"
#include <algorithm>
#include <set>

namespace pcpo {

using namespace llvm;

// MARK: Initializers

NormalizedConjunction::NormalizedConjunction(Constant const& constant) {
    if (ConstantInt const* c = dyn_cast<ConstantInt>(&constant)) {
        state = NORMAL;
        // Watch out for signed/unsigend APInts in future
        Equality eq = {&constant, APInt(64, 0), nullptr, c->getValue()};
        equalaties = {{&constant,eq}};
    } else {
        state = TOP;
    }
}

NormalizedConjunction::NormalizedConjunction(std::map<Value const*, Equality> equalaties) {
    this->equalaties = equalaties;
    state = NORMAL;
}

// MARK: AbstractDomain interface

NormalizedConjunction NormalizedConjunction::interpret(
    Instruction const& inst, std::vector<NormalizedConjunction> const& operands
) {
    // FIXME: maybe use nonDeterministic assignment here.
    if (operands.size() != 2) return NormalizedConjunction {true};

    // We only deal with integer types
    IntegerType const* type = dyn_cast<IntegerType>(inst.getType());
    // FIXME: maybe use nonDeterministic assignment here.
    if (not type) return NormalizedConjunction {true};
    
    type = dyn_cast<IntegerType>(inst.getOperand(0)->getType());
    // FIXME: maybe use nonDeterministic assignment here.
    if (not type) return NormalizedConjunction {true};
    
    type = dyn_cast<IntegerType>(inst.getOperand(1)->getType());
    // FIXME: maybe use nonDeterministic assignment here.
    if (not type) return NormalizedConjunction {true};
    
    unsigned bitWidth = inst.getOperand(0)->getType()->getIntegerBitWidth();
    assert(bitWidth == inst.getOperand(1)->getType()->getIntegerBitWidth());

    NormalizedConjunction a = operands[0];
    NormalizedConjunction b = operands[1];
        
    // Abstract Effect of statements:
            
    // [xi := ?]
    // default
    
    // [xi := b]
    // This will never occur due to SSA Form.
    
    // [xi := xi]
    // [xi := xj]
    // Those will never occur due to SSA Form.
    
    // [xi := xi + b]   [xi := xi - b]
    // Cannot occur due to SSA
    
    // [xi := xj + b]   [xi := xj - b]
    
    // [xi := a * xi]   [xi := a / xi]
    // Cannot occur due to SSA
    
    // [xi := a * xj]   [xi := a / xj]
    
    // [xi := a * xi + b]   [xi := a * xi - b]   [xi := a / xi + b]  [xi := a / xi - b]
    // Cannot occur due to SSA
    
    // [xi := a * xj + b]   [xi := a * xj - b]   [xi := a / xj + b]  [xi := a / xj - b]
    // Those will never occur due to SSA Form.
    
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
            return nonDeterminsticAssignment(inst, a, b);
    }
}

NormalizedConjunction NormalizedConjunction::refineBranch(CmpInst::Predicate pred, Value const& lhs, Value const& rhs, NormalizedConjunction a, NormalizedConjunction b) {

    // Do nothing
    return a;
}


NormalizedConjunction NormalizedConjunction::merge(Merge_op::Type op, NormalizedConjunction a, NormalizedConjunction b) {
    if (a.isBottom()) return b;
    if (b.isBottom()) return a;
    if (a.isTop() || b.isTop()) return NormalizedConjunction {true};

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

    // TODO: getter / setter for equlaties to keep equalaties normalized


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
        Equality eq = {d, APInt(64,0), d, APInt(64,0)};
        E1.insert(eq);
    }
    
    // extend E2 by trivial equalities
    for (auto d: dX2) {
        Equality eq = {d, APInt(64,0), d, APInt(64,0)};
        E2.insert(eq);
    }
    
    // XO / E'0: set of variables where the right hand side in E1 and E2 coincide
    std::set<Equality> X0;
    {
    std::set_intersection(E1.begin(), E1.end(), E2.begin(), E2.end(), std::inserter(X0, X0.end()));
    
    // Remove trivial equalities
    std::set<Equality> filteredX0;
    auto filterTrivialEqualaties = [](Equality eq){return eq.y != eq.x;};
    copy_if(X0, std::inserter(filteredX0, filteredX0.end()), filterTrivialEqualaties);
    X0 = filteredX0;
    
    }
    // X1 / E'1: set of variables where the right hand side is constant but does not coincide in E1 and E2
    std::set<Equality> X1;
    {
    std::set<std::pair<Equality, Equality>> differentConstants;
    
    assert(E1.size() == E2.size() && "E1 and E2 should have the same set of variables in the same order");

        for (itE1 = E1.begin(), itE2 = E2.begin(); itE1 != E1.end() && itE2 != E2.end(); ++itE1, ++itE2) {
            auto eq1 = *itE1;
            auto eq2 = *itE2;
            assert(eq1.y == eq2.y && "left hand side of equations should be the same");
            if (eq1.isConstant() && eq2.isConstant() && eq1.b != eq2.b) {
                differentConstants.insert({eq1,eq2});
            }
        }
    }
    
    // pop first element
    std::pair<Equality, Equality> h = *differentConstants.begin();
    differentConstants.erase(differentConstants.begin());
    
    for (auto i: differentConstants) {
        // find a linear equation that contains both points P1(c(1)i, c(1)h) and P2(c(2)i, c(2)h)
        // y = m * x + b
        auto y = i.first.y;
        APInt m = (h.second.b - h.first.b).sdiv((i.second.b - i.first.b));
        auto x = h.first.x;
        APInt b = -m * h.first.b + i.first.b;
        Equality eq = {y, m, x, b};
        X1.insert(eq);
    }
    }
    // X2 / E'2: set of variables where the right hand side of E1 is constant but the rhs of E2 contains a variable.
    std::set<Equality> X2;
    {
    std::set<std::pair<Equality, Equality>> differentConstants;

    assert(E1.size() == E2.size() && "E1 and E2 should have the same set of variables in the same order");

    for (auto eq1: E1) {
        for (auto eq2: E2) {
            assert(eq1.y == eq2.y && "left hand side of equations should be the same");
            if (eq1.isConstant() && !eq2.isConstant()) {
                differentConstants.insert({eq1,eq2});
            }
        }
    }
    
    std::vector<std::set<std::pair<Equality, Equality>>> Pi2;
    
        // partition differentConstants
        for (auto iterator = differentConstants.begin(); iterator != differentConstants.end();) {
            std::set<std::pair<Equality, Equality>> equivalenceClass;
            std::pair<Equality, Equality> ipair = *iterator;
            equivalenceClass.insert(ipair);
            iterator++;
            // FIXME: Make sure this doesnt casue any trouble!
            for (auto tempit = iterator; tempit != differentConstants.end(); tempit++) {
                std::pair<Equality, Equality> jpair = *tempit;
                bool condition1 = ipair.second.x == jpair.second.x;
                bool condition2 = (ipair.first.b - ipair.second.b).sdiv(ipair.second.a) == (jpair.first.b - jpair.second.b).sdiv(jpair.second.a);
                if (condition1 && condition2) {
                    equivalenceClass.insert(jpair);
                    differentConstants.erase(tempit);
                }
            }
            Pi2.push_back(equivalenceClass);
    }

    // form equaltites for partitions in Pi2
    for (auto q: Pi2) {
        std::pair<Equality, Equality> h = *q.begin();
        q.erase(q.begin());
        for (auto i: q) {
            // xi = ai/ah * xh + ( bi - (ai * bh) / ah)
            auto y = i.first.y;
            auto m = i.second.a.sdiv(h.second.b);
            auto x = h.first.y;
            auto b = i.second.b - (i.second.a * h.second.b).sdiv(h.second.a);
            Equality eq = {y, m, x, b};
            X2.insert(eq);
        }
    }
    
    }
    // X3 / E'3: set of variables where the right hand side of E1 contains a variable and E2 contains a constant.
    std::set<Equality> X3;
    {
    std::set<std::pair<Equality, Equality>> differentConstants;

    assert(E1.size() == E2.size() && "E1 and E2 should have the same set of variables in the same order");

    for (auto eq1: E1) {
        for (auto eq2: E2) {
            assert(eq1.y == eq2.y && "left hand side of equations should be the same");
            if (!eq1.isConstant() && eq2.isConstant()) {
                differentConstants.insert({eq1,eq2});
            }
        }
    }
    
    std::vector<std::set<std::pair<Equality, Equality>>> Pi3;
    
        // partition differentConstants
        for (auto iterator = differentConstants.begin(); iterator != differentConstants.end();) {
            std::set<std::pair<Equality, Equality>> equivalenceClass;
            std::pair<Equality, Equality> ipair = *iterator;
            equivalenceClass.insert(ipair);
            iterator++;
            // FIXME: Make sure this doesnt casue any trouble!
            for (auto tempit = iterator; tempit != differentConstants.end(); tempit++) {
                std::pair<Equality, Equality> jpair = *tempit;
                bool condition1 = ipair.second.x == jpair.second.x;
                bool condition2 = (ipair.first.b - ipair.second.b).sdiv(ipair.second.a) == (jpair.first.b - jpair.second.b).sdiv(jpair.second.a);
                if (condition1 && condition2) {
                    equivalenceClass.insert(jpair);
                    differentConstants.erase(tempit);
                }
            }
            Pi3.push_back(equivalenceClass);
    }

    // form equaltites for partitions in Pi3
    for (auto q: Pi3) {
        std::pair<Equality, Equality> h = *q.begin();
        q.erase(q.begin());
        for (auto i: q) {
            // xi = ai/ah * xh + ( bi - (ai * bh) / ah)
            auto y = i.first.y;
            auto m = i.second.a.sdiv(h.second.b);
            auto x = h.first.y;
            auto b = i.second.b - (i.second.a * h.second.b).sdiv(h.second.a);
            Equality eq = {y, m, x, b};
            X3.insert(eq);
        }
    }
    
    }
    
    
    // X4 / E'4:
    std::set<Equality> X4;
    {
        std::set<std::pair<Equality, Equality>> differentConstants;

        assert(E1.size() == E2.size() && "E1 and E2 should have the same set of variables in the same order");

        for (auto eq1: E1) {
            for (auto eq2: E2) {
                assert(eq1.y == eq2.y && "left hand side of equations should be the same");
                if (!eq1.isConstant() && !eq2.isConstant()) {
                    differentConstants.insert({eq1,eq2});
                }
            }
        }
        
        std::vector<std::set<std::pair<Equality, Equality>>> Pi4;
        
        // partition differentConstants
        for (auto iterator = differentConstants.begin(); iterator != differentConstants.end();) {
            std::set<std::pair<Equality, Equality>> equivalenceClass;
            std::pair<Equality, Equality> ipair = *iterator;
            equivalenceClass.insert(ipair);
            iterator++;
            // FIXME: Make sure this doesnt casue any trouble!
            for (auto tempit = iterator; tempit != differentConstants.end(); tempit++) {
                std::pair<Equality, Equality> jpair = *tempit;
                bool condition1 = ipair.first.x == jpair.first.x && ipair.second.x == jpair.second.x;
                bool condition2 = ipair.second.a.sdiv(ipair.first.a) == jpair.second.a.sdiv(jpair.first.a);
                bool condition3 = (ipair.first.b - ipair.second.b).sdiv(ipair.first.a) == (jpair.first.b - jpair.second.b).sdiv(jpair.first.a);
                if (condition1 && condition2 && condition3) {
                    equivalenceClass.insert(jpair);
                    differentConstants.erase(tempit);
                }
            }
            Pi4.push_back(equivalenceClass);
        }
        
        // form equaltites for partitions in Pi4
        for (auto q: Pi4) {
            std::pair<Equality, Equality> h = *q.begin();
            q.erase(q.begin());
            for (auto i: q) {
                // xi = ai/ah * xh + ( bi - (ai * bh) / ah)
                auto y = i.first.y;
                auto m = i.second.a.sdiv(h.second.b);
                auto x = h.first.y;
                auto b = i.second.b - (i.second.a * h.second.b).sdiv(h.second.a);
                Equality eq = {y, m, x, b};
                X3.insert(eq);
            }
        }
        
        
    }
    
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

// MARK: Abstract Operations

// [xi := ?]# E = Exists# xi in E
    auto result = leastUpperBound(lhs, rhs);APInt(64,0);
    auto i = result.equalaties[&inst];
    if (i.x != &inst && i.b != APInt(64,0)) {
        result.equalaties[&inst] = {&inst, APInt(64,1), &inst, APInt(64,0)};
    } else {
        // find all equations using xi
        auto predicate = [&i](std::pair<Value const*, Equality> p){ return p.second.x = i.y;};
        auto it = std::find_if(result.equalaties.begin(), result.equalaties.end(), predicate);
        if (it != result.equalaties.end()) {
            Equality k = (*it).second;
            for (it = std::find_if(++it, result.equalaties.end(), predicate);
                 it != result.equalaties.end();
                 it = std::find_if(++it, result.equalaties.end(), predicate)) {
                auto& l = it->second;
                result.equalaties[l.y] = {l.y, APInt(64,1), k.y, l.b - k.b};
            }
            result.equalaties[k.y] = {k.y, APInt(64,1), k.y, APInt(64,0)};
        }
        result.equalaties[&inst] = {&inst, APInt(64,1), &inst, APInt(64,0)};
    }
    return result;
}

// TODO: [xi := xj + x]
NormalizedConjunction NormalizedConjunction::Add(Instruction const& inst,  NormalizedConjunction lhs, NormalizedConjunction rhs) {
    auto result = leastUpperBound(lhs, rhs);
    auto i = result.equalaties[&inst];
    
    auto op1 = inst.getOperand(0);
    auto op2 = inst.getOperand(1);
    
    Value const* j;
    ConstantInt const* b;
    
    if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
        b = dyn_cast<ConstantInt>(op1);
        j = op2;
    } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
        b = dyn_cast<ConstantInt>(op2);
        j = op1;
    } else {
        assert(false && "One operand has to be constant");
    }
    
    auto jj = result.equalaties[j];
    
    if (i > jj) {
        result.equalaties[i.y] = {i.y, APInt(64,1), jj.x, i.b + jj.b};;
    } else {
        // Filter results
            result.equalaties[k.y] = {k.y, APInt(64,1), i.y, k.b - i.b - jj.b};
        auto pred = [&jj](std::pair<Value const*,Equality> p){ return p.second.x == jj.y && p.second.y != jj.y;};
        
        for (auto kpair: make_filter_range(result.equalaties, pred)) {
            auto k = kpair.second;
        }
        result.equalaties[jj.y] = {jj.y, APInt(64,1), i.y, -i.b - jj.b};
    }
    
    return result;
}

// TODO: [xi := xj - x]
NormalizedConjunction NormalizedConjunction::Sub(Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs) {
    auto result = leastUpperBound(lhs, rhs);
    auto i = result.equalaties[&inst];
    
    auto op1 = inst.getOperand(0);
    auto op2 = inst.getOperand(1);
    
    Value const* j;
    ConstantInt const* b;
    
    if (isa<ConstantInt>(op1) && isa<Value>(op2)) {
        b = dyn_cast<ConstantInt>(op1);
        j = op2;
    } else if (isa<ConstantInt>(op2) && isa<Value>(op1)) {
        b = dyn_cast<ConstantInt>(op2);
        j = op1;
    } else {
        assert(false && "One operand has to be constant");
    }
    
    auto jj = result.equalaties[j];
    
    if (i > jj) {
        result.equalaties[i.y] = {i.y, APInt(64,1), jj.x, jj.b - i.b};;
    } else {
        // Filter results
            result.equalaties[k.y] = {k.y, APInt(64,1), i.y, k.b + i.b - jj.b};
        auto pred = [&jj](std::pair<Value const*,Equality> p){ return p.second.x == jj.y && p.second.y != jj.y;};
        for (auto kpair: make_filter_range(result.equalaties, pred)) {
            auto& k = kpair.second;
        }
        result.equalaties[jj.y] = {jj.y, APInt(64,1), i.y, i.b - jj.b};
    }
    
    return result;
}

// [xi := a * xj]
NormalizedConjunction NormalizedConjunction::Mul(Instruction const& inst, NormalizedConjunction lhs, NormalizedConjunction rhs) {
    auto result = leastUpperBound(lhs, rhs);

    // TODO
    assert(false && "not implemented");
    
    return result;
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
    return state == NORMAL
        ? rhs.state == NORMAL and equalaties == rhs.equalaties
        : state == rhs.state;
}

raw_ostream& operator<<(raw_ostream& os, NormalizedConjunction a) {
    if (a.isBottom()) {
        os << "[]";
    } else if(a.isTop()) {
        os << "T";
    } else {
        for (auto eq: a.equalaties) {
        os << eq.first << " = " << eq.second.a << " * " << eq.second.x << " + " << eq.second.b;
        }
    }
    return os;
}


}


