#pragma once

#include <vector>
#include <tuple>

#include "llvm/ADT/Hashing.h"

namespace std {

template <typename ... T>
struct hash<tuple<T...>> {
    size_t operator()(tuple<T...> const& t) const {
        size_t seed = 0;
        apply([&seed](const auto&... item) {(( seed = llvm::hash_combine(seed, item) ), ...);}, t);
        return seed;
    }
};

template <typename T, typename U>
struct hash<pair<T, U>> {
    size_t operator()(pair<T, U> const& in) const {
        return llvm::hash_value(in);
    }
};


template <typename T>
struct hash<vector<T>> {
    size_t operator()(vector<T> const& in) const {
        return llvm::hash_combine_range(in.begin(), in.end());
    }
};

}

namespace llvm {

template <typename T>
static hash_code hash_value(std::vector<T> const& in) {
    return hash_combine_range(in.begin(), in.end());
}

}
