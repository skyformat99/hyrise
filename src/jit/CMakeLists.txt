include(./Embed.cmake)

add_definitions(-Qunused-arguments)

set(LLVM_DIR /usr/local/Cellar/llvm-5.0/5.0.0/lib/llvm-5.0/lib/cmake/llvm)
find_package(LLVM 5.0.0 REQUIRED)
message(STATUS ${LLVM_VERSION})
message(STATUS "Found llvm library: inc=${LLVM_INCLUDE_DIR}, lib=${LLVM_LIBRARY_DIR}/libLLVM.so")

include_directories(${LLVM_INCLUDE_DIR})
set(LLVM_LIB ${LLVM_LIBRARY_DIR}/libLLVM.so)

set(JIT_OPERATOR_FILES
    ../lib/operators/jit_operator/specialization/ir_repository.cpp
    ../lib/operators/jit_operator/specialization/ir_repository.hpp
    ../lib/operators/jit_operator/specialization/jit_compiler.cpp
    ../lib/operators/jit_operator/specialization/jit_compiler.hpp
    ../lib/operators/jit_operator/specialization/runtime_pointer.hpp
    ../lib/operators/jit_operator/specialization/module.cpp
    ../lib/operators/jit_operator/specialization/module.hpp
    ../lib/operators/jit_operator/specialization/utils/error_utils.hpp
    ../lib/operators/jit_operator/specialization/utils/llvm_utils.hpp)

set(FILES_TO_EMBED
    demo/demo.cpp
    demo/demo.hpp)

embed_files(EMBEDDED_FILES ${FILES_TO_EMBED})

add_executable(jitSpecialization jit_specialization.cpp ${JIT_OPERATOR_FILES} ${FILES_TO_EMBED} ${EMBEDDED_FILES})
target_link_libraries(jitSpecialization boost_filesystem boost_system ${LLVM_LIB})
