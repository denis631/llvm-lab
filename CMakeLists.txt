# If we don't need RTTI or EH, there's no reason to export anything
# from the hello plugin.
#if( NOT LLVM_REQUIRES_RTTI )
#  if( NOT LLVM_REQUIRES_EH )
#    set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Hello.exports)
#  endif()
#endif()

if(WIN32 OR CYGWIN)
  set(LLVM_LINK_COMPONENTS Core Support)
endif()

# For older LLVM < 8.0 replace first line with this
# add_llvm_loadable_module(llvm-pain
add_llvm_library(llvm-pain MODULE
  src/fixpoint.cpp
  src/fixpoint.h
  src/value_set.cpp
  src/value_set.h
  src/simple_interval.cpp
  src/simple_interval.h
  src/normalized_conjunction.cpp
  src/normalized_conjunction.h
  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
