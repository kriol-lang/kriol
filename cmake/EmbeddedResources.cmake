set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})

set(RUNTIME_BC
    ${GENERATED_DIR}/kriol_runtime.bc
)

set(RUNTIME_BC_HEADER
    ${GENERATED_DIR}/kriol_runtime.bc.h
)

set(GC_HEADER
    ${GENERATED_DIR}/libgc_static.h
)

add_custom_command(
    OUTPUT ${RUNTIME_BC}

    COMMAND
        ${CLANG_PROGRAM}
        -emit-llvm
        -O3
        -c
        ${CMAKE_SOURCE_DIR}/runtime/kriol_runtime.c
        -o
        ${RUNTIME_BC}
        -I${BDWGC_DIR}/include

    DEPENDS
        ${CMAKE_SOURCE_DIR}/runtime/kriol_runtime.c
)

add_custom_command(
    OUTPUT ${RUNTIME_BC_HEADER}

    COMMAND
        ${XXD_PROGRAM}
        -i
        kriol_runtime.bc
        > kriol_runtime.bc.h

    WORKING_DIRECTORY ${GENERATED_DIR}

    DEPENDS
        ${RUNTIME_BC}
)

add_custom_command(
    OUTPUT ${GC_HEADER}

    COMMAND
        ${CMAKE_COMMAND}
        -E copy
        $<TARGET_FILE:gc>
        ${GENERATED_DIR}/embedded_libgc.a

    COMMAND
        ${XXD_PROGRAM}
        -i
        embedded_libgc.a
        > libgc_static.h

    WORKING_DIRECTORY ${GENERATED_DIR}

    DEPENDS
        gc
)

add_custom_target(
    kriol_embedded_resources
    DEPENDS
        ${RUNTIME_BC_HEADER}
        ${GC_HEADER}
)