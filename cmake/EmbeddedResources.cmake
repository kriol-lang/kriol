set(GENERATED_DIR ${CMAKE_CURRENT_BINARY_DIR})
set(KRIOL_EMBED_FILE_SCRIPT ${CMAKE_CURRENT_LIST_DIR}/EmbedFile.cmake)

set(KRIOL_WASI_TARGET "wasm32-wasi" CACHE STRING "Target triple used for Kriol WASI output")
set(KRIOL_WASI_SYSROOT "/usr" CACHE PATH "WASI sysroot used for Kriol WASI output")

set(RUNTIME_NATIVE_GC_BC     ${GENERATED_DIR}/kriol_runtime_native_gc.bc)
set(RUNTIME_NATIVE_GC_HEADER ${GENERATED_DIR}/kriol_runtime_native_gc.bc.h)
set(GC_NATIVE_HEADER         ${GENERATED_DIR}/libgc_native.h)
set(KRIOL_EMBEDDED_RESOURCE_HEADERS
    ${RUNTIME_NATIVE_GC_HEADER}
    ${GC_NATIVE_HEADER}
)

if(KRIOL_ENABLE_WASM)
    include(ExternalProject)

    if(KRIOL_WASI_ENABLE_GC)
        file(READ "${BDWGC_DIR}/include/private/gcconfig.h" KRIOL_BDWGC_GCCONFIG)
        string(FIND "${KRIOL_BDWGC_GCCONFIG}" "__wasi__" KRIOL_BDWGC_HAS_WASI_CONFIG)
        if(KRIOL_BDWGC_HAS_WASI_CONFIG EQUAL -1)
            message(FATAL_ERROR
                "KRIOL_WASI_ENABLE_GC=ON requires a Boehm GC checkout with wasm32-wasi support. "
                "The current stable Boehm GC checkout does not support wasm32-wasi; use the default "
                "WASI no-GC runtime or switch Boehm GC to a WASI-capable release/branch."
            )
        endif()

        set(RUNTIME_WASM32_WASI_BC     ${GENERATED_DIR}/kriol_runtime_wasm32_wasi_gc.bc)
        set(RUNTIME_WASM32_WASI_HEADER ${GENERATED_DIR}/kriol_runtime_wasm32_wasi_gc.bc.h)
        set(GC_WASM32_WASI_HEADER      ${GENERATED_DIR}/libgc_wasm32_wasi.h)
    else()
        set(RUNTIME_WASM32_WASI_BC     ${GENERATED_DIR}/kriol_runtime_wasm32_wasi_nogc.bc)
        set(RUNTIME_WASM32_WASI_HEADER ${GENERATED_DIR}/kriol_runtime_wasm32_wasi_nogc.bc.h)
    endif()

    set(WASI_CRT1_COMMAND_HEADER      ${GENERATED_DIR}/wasi_crt1_command.o.h)
    set(WASI_LIBC_HEADER              ${GENERATED_DIR}/wasi_libc.a.h)
    set(WASI_LIBM_HEADER              ${GENERATED_DIR}/wasi_libm.a.h)
    set(WASI_BUILTINS_HEADER          ${GENERATED_DIR}/wasi_builtins.a.h)
    list(APPEND KRIOL_EMBEDDED_RESOURCE_HEADERS
        ${RUNTIME_WASM32_WASI_HEADER}
        ${WASI_CRT1_COMMAND_HEADER}
        ${WASI_LIBC_HEADER}
        ${WASI_LIBM_HEADER}
        ${WASI_BUILTINS_HEADER}
    )

    if(KRIOL_WASI_ENABLE_GC)
        list(APPEND KRIOL_EMBEDDED_RESOURCE_HEADERS
            ${GC_WASM32_WASI_HEADER}
        )
    endif()
endif()

add_custom_command(
    OUTPUT ${RUNTIME_NATIVE_GC_BC}

    COMMAND
        ${CLANG_PROGRAM}
        -emit-llvm
        -O3
        -c
        ${CMAKE_SOURCE_DIR}/runtime/kriol_runtime.c
        -o
        ${RUNTIME_NATIVE_GC_BC}
        -I${BDWGC_DIR}/include

    DEPENDS
        ${CMAKE_SOURCE_DIR}/runtime/kriol_runtime.c
)

# Embeds INPUT_FILE as a C array named after SYMBOL (e.g. libgc_native.a ->
# libgc_native_a / libgc_native_a_len). Extra arguments are added as
# dependencies, for inputs produced by targets rather than files.
function(kriol_embed_file INPUT_FILE OUTPUT_HEADER SYMBOL)
    add_custom_command(
        OUTPUT ${OUTPUT_HEADER}

        COMMAND
            ${CMAKE_COMMAND}
            -DINPUT=${INPUT_FILE}
            -DOUTPUT=${OUTPUT_HEADER}
            -DSYMBOL=${SYMBOL}
            -P ${KRIOL_EMBED_FILE_SCRIPT}

        DEPENDS
            ${INPUT_FILE}
            ${KRIOL_EMBED_FILE_SCRIPT}
            ${ARGN}

        VERBATIM
    )
endfunction()

kriol_embed_file(${RUNTIME_NATIVE_GC_BC} ${RUNTIME_NATIVE_GC_HEADER} kriol_runtime_native_gc.bc)

kriol_embed_file($<TARGET_FILE:gc> ${GC_NATIVE_HEADER} libgc_native.a gc)

if(KRIOL_ENABLE_WASM)
    set(WASI_LIB_DIR ${KRIOL_WASI_SYSROOT}/lib/wasm32-wasi)
    set(WASI_CRT1_COMMAND ${WASI_LIB_DIR}/crt1-command.o)
    set(WASI_LIBC ${WASI_LIB_DIR}/libc.a)
    set(WASI_LIBM ${WASI_LIB_DIR}/libm.a)

    execute_process(
        COMMAND
            ${CLANG_PROGRAM}
            --target=${KRIOL_WASI_TARGET}
            --sysroot=${KRIOL_WASI_SYSROOT}
            --print-libgcc-file-name
        OUTPUT_VARIABLE WASI_BUILTINS
        OUTPUT_STRIP_TRAILING_WHITESPACE
        RESULT_VARIABLE WASI_BUILTINS_RESULT
    )

    if(NOT WASI_BUILTINS_RESULT EQUAL 0 OR NOT EXISTS "${WASI_BUILTINS}")
        message(FATAL_ERROR "Could not locate WASI compiler-rt builtins with ${CLANG_PROGRAM}")
    endif()

    foreach(_KRIOL_WASI_INPUT IN ITEMS
        "${WASI_CRT1_COMMAND}"
        "${WASI_LIBC}"
        "${WASI_LIBM}"
    )
        if(NOT EXISTS "${_KRIOL_WASI_INPUT}")
            message(FATAL_ERROR "Required WASI input not found: ${_KRIOL_WASI_INPUT}")
        endif()
    endforeach()

    add_custom_command(
        OUTPUT ${RUNTIME_WASM32_WASI_BC}

        COMMAND
            ${CLANG_PROGRAM}
            --target=${KRIOL_WASI_TARGET}
            --sysroot=${KRIOL_WASI_SYSROOT}
            -emit-llvm
            -O3
            -c
            ${CMAKE_SOURCE_DIR}/runtime/kriol_runtime.c
            -o
            ${RUNTIME_WASM32_WASI_BC}
            $<$<BOOL:${KRIOL_WASI_ENABLE_GC}>:-I${BDWGC_DIR}/include>
            -DKRIOL_RUNTIME_NO_GC=$<NOT:$<BOOL:${KRIOL_WASI_ENABLE_GC}>>

        DEPENDS
            ${CMAKE_SOURCE_DIR}/runtime/kriol_runtime.c
    )

    get_filename_component(_KRIOL_WASI_RUNTIME_NAME ${RUNTIME_WASM32_WASI_BC} NAME)
    kriol_embed_file(${RUNTIME_WASM32_WASI_BC} ${RUNTIME_WASM32_WASI_HEADER} ${_KRIOL_WASI_RUNTIME_NAME})

    kriol_embed_file(${WASI_CRT1_COMMAND} ${WASI_CRT1_COMMAND_HEADER} wasi_crt1_command.o)

    kriol_embed_file(${WASI_LIBC} ${WASI_LIBC_HEADER} wasi_libc.a)

    kriol_embed_file(${WASI_LIBM} ${WASI_LIBM_HEADER} wasi_libm.a)

    kriol_embed_file(${WASI_BUILTINS} ${WASI_BUILTINS_HEADER} wasi_builtins.a)

    if(KRIOL_WASI_ENABLE_GC)
        set(WASI_GC_BUILD_DIR ${GENERATED_DIR}/_bdwgc_wasm32_wasi_cross)
        set(WASI_GC_LIB ${WASI_GC_BUILD_DIR}/libgc.a)

        ExternalProject_Add(
            kriol_bdwgc_wasm32_wasi
            SOURCE_DIR ${BDWGC_DIR}
            BINARY_DIR ${WASI_GC_BUILD_DIR}
            CMAKE_ARGS
                -DCMAKE_SYSTEM_NAME=WASI
                -DCMAKE_SYSTEM_PROCESSOR=wasm32
                -DCMAKE_C_COMPILER=${CLANG_PROGRAM}
                -DCMAKE_C_COMPILER_TARGET=${KRIOL_WASI_TARGET}
                -DCMAKE_SYSROOT=${KRIOL_WASI_SYSROOT}
                -DCMAKE_TRY_COMPILE_TARGET_TYPE=STATIC_LIBRARY
                -DBUILD_SHARED_LIBS=OFF
                -Denable_threads=OFF
                -Denable_docs=OFF
                -Dbuild_cord=OFF
                -Denable_cplusplus=OFF
                -Denable_gcj_support=OFF
                -Denable_java_finalization=OFF
                -Dbuild_tests=OFF
            BUILD_BYPRODUCTS ${WASI_GC_LIB}
            INSTALL_COMMAND ""
        )

        kriol_embed_file(${WASI_GC_LIB} ${GC_WASM32_WASI_HEADER} libgc_wasm32_wasi.a kriol_bdwgc_wasm32_wasi)
    endif()
endif()

if(KRIOL_ENABLE_WINDOWS_TARGET)
    # x86_64-windows output links MinGW programs against msvcrt.dll, which
    # every Windows release ships, using CRT pieces from an llvm-mingw
    # (msvcrt variant) toolchain. They are embedded here in link order.
    set(KRIOL_MINGW_TARGET "x86_64-w64-windows-gnu")
    set(_KRIOL_MINGW_LIB_DIR "${KRIOL_MINGW_SYSROOT}/x86_64-w64-mingw32/lib")

    file(GLOB _KRIOL_MINGW_BUILTINS
        "${KRIOL_MINGW_SYSROOT}/lib/clang/*/lib/windows/libclang_rt.builtins-x86_64.a")
    list(LENGTH _KRIOL_MINGW_BUILTINS _KRIOL_MINGW_BUILTINS_COUNT)
    if(NOT _KRIOL_MINGW_BUILTINS_COUNT EQUAL 1)
        message(FATAL_ERROR
            "Expected one libclang_rt.builtins-x86_64.a under ${KRIOL_MINGW_SYSROOT}/lib/clang, "
            "found ${_KRIOL_MINGW_BUILTINS_COUNT}. KRIOL_MINGW_SYSROOT must be an llvm-mingw root.")
    endif()

    set(_KRIOL_MINGW_STARTUP_FILES
        ${_KRIOL_MINGW_LIB_DIR}/crt2.o
        ${_KRIOL_MINGW_LIB_DIR}/crtbegin.o
    )
    set(_KRIOL_MINGW_LIBRARY_FILES
        ${_KRIOL_MINGW_LIB_DIR}/libmingw32.a
        ${_KRIOL_MINGW_BUILTINS}
        ${_KRIOL_MINGW_LIB_DIR}/libmoldname.a
        ${_KRIOL_MINGW_LIB_DIR}/libmingwex.a
        ${_KRIOL_MINGW_LIB_DIR}/libmsvcrt.a
        ${_KRIOL_MINGW_LIB_DIR}/libadvapi32.a
        ${_KRIOL_MINGW_LIB_DIR}/libshell32.a
        ${_KRIOL_MINGW_LIB_DIR}/libuser32.a
        ${_KRIOL_MINGW_LIB_DIR}/libkernel32.a
        ${_KRIOL_MINGW_LIB_DIR}/crtend.o
    )

    set(RUNTIME_X86_64_WINDOWS_BC     ${GENERATED_DIR}/kriol_runtime_x86_64_windows.bc)
    set(RUNTIME_X86_64_WINDOWS_HEADER ${GENERATED_DIR}/kriol_runtime_x86_64_windows.bc.h)
    set(GC_X86_64_WINDOWS_OBJ         ${GENERATED_DIR}/libgc_x86_64_windows.o)
    set(GC_X86_64_WINDOWS_HEADER      ${GENERATED_DIR}/libgc_x86_64_windows.o.h)
    set(MINGW_INPUTS_HEADER           ${GENERATED_DIR}/mingw_link_inputs.h)

    # Use the C99 printf family from mingwex so output matches other targets
    # (msvcrt.dll prints three-digit exponents, for example).
    add_custom_command(
        OUTPUT ${RUNTIME_X86_64_WINDOWS_BC}
        COMMAND
            ${CLANG_PROGRAM}
            --target=${KRIOL_MINGW_TARGET}
            --sysroot=${KRIOL_MINGW_SYSROOT}
            -D__USE_MINGW_ANSI_STDIO=1
            -emit-llvm
            -O3
            -c
            ${CMAKE_SOURCE_DIR}/runtime/kriol_runtime.c
            -o
            ${RUNTIME_X86_64_WINDOWS_BC}
            -I${BDWGC_DIR}/include
        DEPENDS
            ${CMAKE_SOURCE_DIR}/runtime/kriol_runtime.c
        VERBATIM
    )

    # Boehm GC's single-file build, with the definitions its CMake build uses
    # for Kriol's configuration (static, no threads).
    add_custom_command(
        OUTPUT ${GC_X86_64_WINDOWS_OBJ}
        COMMAND
            ${CLANG_PROGRAM}
            --target=${KRIOL_MINGW_TARGET}
            --sysroot=${KRIOL_MINGW_SYSROOT}
            -O2
            -c
            ${BDWGC_DIR}/extra/gc.c
            -o
            ${GC_X86_64_WINDOWS_OBJ}
            -I${BDWGC_DIR}/include
            -DALL_INTERIOR_POINTERS
            -DNO_EXECUTE_PERMISSION
            -DENABLE_DISCLAIM
            -DGC_ATOMIC_UNCOLLECTABLE
            -DGC_NOT_DLL
            -DUSE_MMAP
            -DUSE_MUNMAP
        DEPENDS
            ${BDWGC_DIR}/extra/gc.c
        VERBATIM
    )

    kriol_embed_file(${RUNTIME_X86_64_WINDOWS_BC} ${RUNTIME_X86_64_WINDOWS_HEADER} kriol_runtime_x86_64_windows.bc)
    kriol_embed_file(${GC_X86_64_WINDOWS_OBJ} ${GC_X86_64_WINDOWS_HEADER} libgc_x86_64_windows.o)

    set(_KRIOL_MINGW_INPUTS_CONTENT "// Generated by cmake/EmbeddedResources.cmake.\n")
    set(_KRIOL_MINGW_INPUT_HEADERS)
    foreach(_KRIOL_GROUP IN ITEMS STARTUP LIBRARY)
        set(_KRIOL_TABLE "")
        foreach(_KRIOL_INPUT IN LISTS _KRIOL_MINGW_${_KRIOL_GROUP}_FILES)
            if(NOT EXISTS "${_KRIOL_INPUT}")
                message(FATAL_ERROR "Required MinGW input not found: ${_KRIOL_INPUT}")
            endif()
            get_filename_component(_KRIOL_NAME "${_KRIOL_INPUT}" NAME)
            string(MAKE_C_IDENTIFIER "mingw_${_KRIOL_NAME}" _KRIOL_SYMBOL)
            set(_KRIOL_HEADER ${GENERATED_DIR}/${_KRIOL_SYMBOL}.h)
            kriol_embed_file(${_KRIOL_INPUT} ${_KRIOL_HEADER} ${_KRIOL_SYMBOL})
            list(APPEND _KRIOL_MINGW_INPUT_HEADERS ${_KRIOL_HEADER})
            string(APPEND _KRIOL_MINGW_INPUTS_CONTENT "#include \"${_KRIOL_SYMBOL}.h\"\n")
            string(APPEND _KRIOL_TABLE "    {\"${_KRIOL_NAME}\", ${_KRIOL_SYMBOL}, ${_KRIOL_SYMBOL}_len},\n")
        endforeach()
        string(TOLOWER ${_KRIOL_GROUP} _KRIOL_GROUP_NAME)
        string(APPEND _KRIOL_MINGW_INPUTS_CONTENT
            "\nstatic const KriolEmbeddedFile kriol_mingw_${_KRIOL_GROUP_NAME}_inputs[] = {\n${_KRIOL_TABLE}};\n")
    endforeach()
    file(CONFIGURE OUTPUT ${MINGW_INPUTS_HEADER} CONTENT "${_KRIOL_MINGW_INPUTS_CONTENT}" @ONLY)

    list(APPEND KRIOL_EMBEDDED_RESOURCE_HEADERS
        ${RUNTIME_X86_64_WINDOWS_HEADER}
        ${GC_X86_64_WINDOWS_HEADER}
        ${_KRIOL_MINGW_INPUT_HEADERS}
    )
endif()

add_custom_target(
    kriol_embedded_resources
    DEPENDS
        ${KRIOL_EMBEDDED_RESOURCE_HEADERS}
)
