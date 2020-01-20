#pragma once
#include "llvm/ADT/Hashing.h"

// C++ unordered_maps don't support tuples as keys, which is why one has to define the hash function for said tuple.
// If weird behaviours are observed, chances are high, that this hash function is not working properly, as we don't know
// if the llvm::hash_combine function works for all cases. So far it did (hopefully in the future as well).
namespace std { 
  
  template <> 
  struct hash<std::tuple<llvm::BasicBlock const*, llvm::BasicBlock const*>> 
  { 
    std::size_t operator()(const std::tuple<llvm::BasicBlock const*, llvm::BasicBlock const*>& k) const 
    { 
      llvm::hash_code hash_code = llvm::hash_combine(std::get<0>(k), std::get<1>(k)); 
      return hash_code; 
    } 
  }; 
 
}

namespace pcpo {

  // The definition of the tuple, for ease of writing. First element is the basic block we currently evaluate.
  // Second element is the callstring, i.e. the basic block from which the function is called, which the
  // first element is part of. (It's complicated to put in words).
  // The callstring is normally represented by the caller function, as described in "Compiler Design: Analysis and Transformation",
  // but since this code operates on BasicBlocks instead of the whole function, we set the callstring to the BasicBlock,
  // from which the function call is performed.
  typedef std::tuple<llvm::BasicBlock const*, llvm::BasicBlock const*> bb_key;

  // Helper functions for debug output, pretty much self explanatory.
  static std::string _bb_to_str(llvm::BasicBlock const* bb) {
      std::string str = "%";
      if (llvm::Function const* f = bb->getParent()) {
          str.append(f->getName());
          str.append(".");
      }
      str.append(bb->getName());
      return str;
  }

  static std::string _bb_key_to_str(bb_key key) {
      std::string str = "[";
      str.append(_bb_to_str(std::get<0>(key)));
      str.append(", ");
      str.append(_bb_to_str(std::get<1>(key)));
      str.append("]");
      return str;
  }
}
