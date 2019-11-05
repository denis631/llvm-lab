# If we don't need RTTI or EH, there's no reason to export anything
# from the hello plugin.
#if( NOT LLVM_REQUIRES_RTTI )
#  if( NOT LLVM_REQUIRES_EH )
#    set(LLVM_EXPORTED_SYMBOL_FILE ${CMAKE_CURRENT_SOURCE_DIR}/Hello.exports)
#  endif()
#endif()
cmake_minimum_required(VERSION 3.10)

find_package(LLVM REQUIRED CONFIG)
add_definitions(${LLVM_DEFINITIONS})
include_directories(${LLVM_INCLUDE_DIRS})

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")
message(STATUS "Using LLVMConfig.cmake in: ${LLVM_DIR}")

list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")
message(STATUS "${LLVM_CMAKE_DIR}")

include(AddLLVM)

# For older LLVM < 8.0 replace first line with this
# add_llvm_loadable_module(llvm-pain
add_llvm_library(llvm-pain MODULE
  src/fixpoint.cpp
  src/fixpoint.h
  src/value_set.cpp
  src/value_set.h
  src/simple_interval.cpp
  src/simple_interval.h
  src/callstring.cpp
  src/callstring.h
  DEPENDS
  intrinsics_gen
  PLUGIN_TOOL
  opt
  )
