#include "linear_equality.h"


namespace pcpo {

using namespace llvm;

LinearEquality::LinearEquality(Value const* y) {
    this->y = y;
    this->a = 1;
    this->x = y;
    this->b = 0;
}
    
LinearEquality::LinearEquality(Value const* y, int64_t a, Value const* x, int64_t b) {
    this->y = y;
    this->a = a;
    this->x = x;
    this->b = b;
}

LinearEquality::LinearEquality(ConstantInt const* y) {
    this->y = y;
    this->a = 1;
    this->x = nullptr;
    this->b = y->getSExtValue();
}
    
raw_ostream& operator<<(raw_ostream& os, LinearEquality a) {
    os << "{ ";
    if (a.y != nullptr && a.y->hasName()) {
        os << a.y->getName() << " = ";
    } else if (a.y != nullptr) {
        os << a.y << " = ";
    } else {
        os << "<null> = ";
    }
        
    if (a.x != nullptr) {
        if (a.x->hasName()) {
            os << a.a << " * " << a.x->getName();
        } else {
            os << a.a << " * " << a.x;
        }
        
        if (a.b > 0) {
            os << " + " << a.b;
        } else if (a.b < 0) {
            os << a.b;
        }
    } else {
        os << a.b;
    }
        
    os << " }";
        
    return os;
}

}

