cmake_minimum_required(VERSION 3.4.3)

project(PAIN
  VERSION 1.0.0
  LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 17)

set (PROJECT_DESCRIPTION "Implements an LLVM analysis pass using abstract interpretation.")
set (PROJECT_HOMEPAGE_URL "https://versioncontrolseidl.in.tum.de/petter/llvm-abstractinterpretation")

if (NOT PATH_TO_LLVM)
  message(FATAL_ERROR " 
  The cmake is supposed to be called with PATH_TO_LLVM pointing to
 a precompiled version of LLVM or to to the source code of LLVM
 Examples:
 cmake -G \"${CMAKE_GENERATOR}\" -DPATH_TO_LLVM=/opt/llvm-9.0.1 ${CMAKE_SOURCE_DIR}
 cmake -G \"${CMAKE_GENERATOR}\" -DPATH_TO_LLVM=/llvm-project/llvm ${CMAKE_SOURCE_DIR}
")
endif()

if (NOT IS_ABSOLUTE ${PATH_TO_LLVM})
  # Convert relative path to absolute path
  get_filename_component(PATH_TO_LLVM
    "${PATH_TO_LLVM}" REALPATH BASE_DIR "${CMAKE_BINARY_DIR}")
endif()

set (BUILD_AGAINST_PRECOMPILED_LLVM TRUE)
if (EXISTS ${PATH_TO_LLVM}/CMakeLists.txt)
  set (BUILD_AGAINST_PRECOMPILED_LLVM FALSE)
endif()

# This enables assertions for Release builds.
# https://stackoverflow.com/questions/22140520/how-to-enable-assert-in-cmake-release-mode
string(REPLACE "-DNDEBUG" "" CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE}")

if (${BUILD_AGAINST_PRECOMPILED_LLVM})
  set (search_paths
    ${PATH_TO_LLVM}
    ${PATH_TO_LLVM}/lib/cmake
    ${PATH_TO_LLVM}/lib/cmake/llvm
    ${PATH_TO_LLVM}/lib/cmake/clang
    ${PATH_TO_LLVM}/share/clang/cmake/
    ${PATH_TO_LLVM}/share/llvm/cmake/
  )

  find_package(LLVM REQUIRED CONFIG PATHS ${search_paths} NO_DEFAULT_PATH)
  find_package(Clang REQUIRED CONFIG PATHS ${search_paths} NO_DEFAULT_PATH)

  list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

else()
  
  set (LLVM_ENABLE_PROJECTS "clang" CACHE BOOL "Build only Clang when building against monorepo" FORCE)
  set (LLVM_TARGETS_TO_BUILD "host" CACHE STRING "Only build targets for host architecture" FORCE)
  #set (BUILD_SHARED_LIBS "On" CACHE BOOL "Link LLVM libraries dynamically" FORCE)

  add_subdirectory(${PATH_TO_LLVM} llvm-build)

  if (NOT TARGET clangTooling)
    message(FATAL_ERROR " 
 Cannot find clangTooling target. Did you forget to clone clang sources?
 Clean CMake cache and make sure they are available at:
 ${PATH_TO_LLVM}/tools/clang")
  endif()

  # Normally, include paths provided by LLVMConfig.cmake
  # In this case we can 'steal' them from real targets
  get_target_property(llvm_support_includes LLVMSupport INCLUDE_DIRECTORIES)
  get_target_property(clang_tooling_includes clangTooling INCLUDE_DIRECTORIES)
  set(LLVM_INCLUDE_DIRS ${llvm_support_includes} ${clang_tooling_includes})
  list(REMOVE_DUPLICATES LLVM_INCLUDE_DIRS)

  # Manually include the llvm CMake modules
  list(APPEND CMAKE_MODULE_PATH
    "${PATH_TO_LLVM}/cmake"
    "${PATH_TO_LLVM}/cmake/modules"
  )

  set(LLVM_MAIN_SRC_DIR ${PATH_TO_LLVM})

endif()

include(LLVM-Config)
include(HandleLLVMOptions)
include(AddLLVM)

if ("LLVM" IN_LIST LLVM_AVAILABLE_LIBS)
  set (LLVM_AVAILABLE_LIBS
    LLVM
  )
else()
  set (LLVM_AVAILABLE_LIBS
    LLVMSupport
    LLVMCore  
    LLVMAnalysis
  )
endif()

set(PAIN_SOURCES
  src/fixpoint.cpp
  src/fixpoint_widening.cpp
  src/value_set.cpp
  src/simple_interval.cpp
  src/normalized_conjunction.cpp
  src/linear_equality.cpp
  src/linear_subspace.cpp
  src/constant_folding.cpp
)

set(PAIN_HEADERS
  src/fixpoint.h
  src/value_set.h
  src/simple_interval.h
  src/normalized_conjunction.h
  src/linear_equality.h
  src/general.h
  src/global.h
  src/abstract_state.h
  src/hash_utils.h
  src/linear_subspace.h
  src/simple_matrix.h
  src/sparse_matrix.h
  src/constant_folding.h
)

include_directories(${LLVM_INCLUDE_DIRS})

add_llvm_library(llvm-pain MODULE
  ${PAIN_SOURCES}
  ${PAIN_HEADERS}
  DEPENDS
  intrinsics_gen
  irgen
  PLUGIN_TOOL
  opt
)

#
# Tests
#

add_llvm_executable(simple_interval_test
   test/simple_interval_test.cpp
   ${PAIN_HEADERS}
   ${PAIN_SOURCES}
   DEPENDS
   irgen
   intrinsics_gen
)

target_link_libraries(simple_interval_test
   PRIVATE ${LLVM_AVAILABLE_LIBS}
)

add_llvm_executable(normalized_conjunction_test
   test/normalized_conjunction_test.cpp
   ${PAIN_HEADERS}
   ${PAIN_SOURCES}
   DEPENDS
   irgen
   intrinsics_gen
)

target_link_libraries(normalized_conjunction_test
   PRIVATE ${LLVM_AVAILABLE_LIBS}
)

add_llvm_executable(linear_subspace_test
   test/linear_subspace_test.cpp
   ${PAIN_HEADERS}
   ${PAIN_SOURCES}
   DEPENDS
   irgen
   intrinsics_gen
)

target_link_libraries(linear_subspace_test
   PRIVATE ${LLVM_AVAILABLE_LIBS}
)

add_llvm_executable(simple_matrix_test
   test/simple_matrix_test.cpp
   ${PAIN_HEADERS}
   ${PAIN_SOURCES}
)

target_link_libraries(simple_matrix_test
   PRIVATE ${LLVM_AVAILABLE_LIBS}
)

add_llvm_executable(sparse_matrix_test
   test/sparse_matrix_test.cpp
   ${PAIN_HEADERS}
   ${PAIN_SOURCES}
)

target_link_libraries(sparse_matrix_test
   PRIVATE ${LLVM_AVAILABLE_LIBS}
)

enable_testing()

add_test(NAME intervalAnalysisTest
  COMMAND opt --load $<TARGET_FILE:llvm-pain> --painpass -S ${CMAKE_SOURCE_DIR}/output/add-1.ll
)

add_test(NAME simpleIntervalTest
   COMMAND simple_interval_test
)

add_test(NAME normalizedConjunctionTest
   COMMAND normalized_conjunction_test
)

add_test(NAME linearSubspaceTest
   COMMAND linear_subspace_test
)

add_test(NAME simpleMatrixTest
   COMMAND simple_matrix_test
)

add_test(NAME sparseMatrixTest
   COMMAND sparse_matrix_test
)

#
# Samples
#

set(SAMPLES
  add-1-float
  add-1
  add-2
  add-3
  add-4
  basic_function
  branching
  cmp-two-variables-1
  cmp-two-variables-2
  euler-48
  euler
  example
  for-loop-1
  for
  func-test-1
  func-test-2
  func-test-3
  func-test-4
  func-test-5
  func-test-for
  func-test-rec-endless
  func-test-rec
  func-test-switch
  gcd
  goto
  if-and
  if-multiple-of-4
  if-then-else-2
  if-then-else-complicated
  if-then-else-two-constraints
  if-then-else
  multiple-functions
  ops
  switch-2
  switch-3
  switch-two-labels
  switch
  tmp
  while-1
  while-2
  while-bigger-steps
  while-neg
  example-1
  A
  B
  C
  D
)

# Older CMake version do not support list transformations
macro(list_transform_prepend in prefix)
    foreach(f ${${in}})
        list(APPEND temp "${prefix}${f}")
    endforeach()
    set(${in} "${temp}")
    unset(temp)
endmacro()

macro(list_transform_append in suffix out)
    foreach(f ${${in}})
        list(APPEND ${out} "${f}${suffix}")
    endforeach()
endmacro()

list_transform_append(SAMPLES ".c" SAMPLES_C)
list_transform_append(SAMPLES ".bc" SAMPLES_BC)
list_transform_append(SAMPLES ".ll" SAMPLES_LL)
list_transform_prepend(SAMPLES_LL "${CMAKE_SOURCE_DIR}/output/")
list_transform_prepend(SAMPLES_BC "${CMAKE_SOURCE_DIR}/output/")
list_transform_prepend(SAMOLES_C "${CMAKE_SOURCE_DIR}/samples/")

add_custom_target(irgen
  DEPENDS ${SAMPLES_LL} ${SAMPLES_BC}
)

file(MAKE_DIRECTORY ${CMAKE_SOURCE_DIR}/output)

if (APPLE)

foreach(src ${SAMPLES})
  add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/output/${src}.ll
    COMMAND clang --sysroot ${CMAKE_OSX_SYSROOT} -O0 -emit-llvm ${CMAKE_SOURCE_DIR}/samples/${src}.c -Xclang -disable-O0-optnone -c -o ${CMAKE_SOURCE_DIR}/output/${src}.bc
    COMMAND opt -S -mem2reg ${CMAKE_SOURCE_DIR}/output/${src}.bc -o ${CMAKE_SOURCE_DIR}/output/${src}.ll
    DEPENDS clang opt ${CMAKE_SOURCE_DIR}/samples/${src}.c
    COMMENT "Generating LLVM IR for example ${src}"
  )
endforeach(src)

else()

foreach(src ${SAMPLES})
  add_custom_command(
    OUTPUT ${CMAKE_SOURCE_DIR}/output/${src}.ll
    COMMAND clang -O0 -emit-llvm ${CMAKE_SOURCE_DIR}/samples/${src}.c -Xclang -disable-O0-optnone -c -o ${CMAKE_SOURCE_DIR}/output/${src}.bc
    COMMAND opt -S -mem2reg ${CMAKE_SOURCE_DIR}/output/${src}.bc -o ${CMAKE_SOURCE_DIR}/output/${src}.ll
    DEPENDS clang opt ${CMAKE_SOURCE_DIR}/samples/${src}.c
    COMMENT "Generating LLVM IR for example ${src}"
  )
endforeach(src)

endif()

configure_file(.lldbinit.in .lldbinit)
