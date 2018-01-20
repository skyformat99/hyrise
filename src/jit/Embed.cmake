cmake_minimum_required(VERSION 3.7)

enable_language(ASM)

function(EMBED_FILES ASM_FILES)
    cmake_parse_arguments(llvm "" "EXPORT_MACRO" "" ${ARGN})
    set(INPUT_FILES "${llvm_UNPARSED_ARGUMENTS}")

    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

    set(${ASM_FILES})

    foreach(FILE ${INPUT_FILES})
        get_filename_component(FILE_EXTENSION ${FILE} EXT)
        get_filename_component(CPP_FILE ${FILE} ABSOLUTE)
        get_filename_component(FILENAME ${FILE} NAME_WE)
        get_filename_component(DIRECTORY ${FILE} DIRECTORY)

        if(NOT ${FILE_EXTENSION} MATCHES ".cpp")
            continue()
        endif()

        set(FLAGS -std=c++1z -O3 -fwhole-program-vtables -flto ${CMAKE_CXX_FLAGS})
        set(LLVM_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/${FILENAME}.ll")
        set(HPP_FILE ${DIRECTORY}/${FILENAME}.hpp)
        add_custom_command(
                OUTPUT ${LLVM_FILE}
                COMMAND clang++-5.0 ${FLAGS} -DSOURCE_PATH_SIZE -I /mnt/hgfs/vm-code/hyrise/src/lib -emit-llvm -S -o ${LLVM_FILE} ${CPP_FILE}
                DEPENDS ${CPP_FILE} ${HPP_FILE})
        embed_file(${FILENAME} ${LLVM_FILE} ${ASM_FILES})
    endforeach()

    set_source_files_properties(${ASM_FILES} PROPERTIES GENERATED TRUE)
    set(${ASM_FILES} ${${ASM_FILES}} PARENT_SCOPE)
endfunction()

function(EMBED_FILE SYMBOL FILE ASM_FILES)
    file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated")

    set(ASM_FILE "${CMAKE_CURRENT_BINARY_DIR}/generated/${SYMBOL}.s")
    mangle_symbol(${SYMBOL} MANGLED_SYMBOL)
    mangle_symbol(${SYMBOL}_size MANGLED_SYMBOL_SIZE)

    add_custom_command(
            OUTPUT ${ASM_FILE}
            COMMAND echo \".global ${MANGLED_SYMBOL}\\n.global ${MANGLED_SYMBOL_SIZE}\\n${MANGLED_SYMBOL}:\\n.incbin \\"${FILE}\\"\\n1:\\n${MANGLED_SYMBOL_SIZE}:\\n.8byte 1b - ${MANGLED_SYMBOL}\\n\" > ${ASM_FILE}
            DEPENDS ${FILE})

    list(APPEND ${ASM_FILES} ${ASM_FILE})
    set(${ASM_FILES} "${${ASM_FILES}}" PARENT_SCOPE)
endfunction()

function(MANGLE_SYMBOL SYMBOL MANGLED_SYMBOL)
    string(LENGTH ${SYMBOL} LENGTH)
    set(${MANGLED_SYMBOL} _ZN7opossum${LENGTH}${SYMBOL}E PARENT_SCOPE)
endfunction()
