cmake_minimum_required(VERSION 3.20)

# Normalize install destinations such as "./licenses" (the flat release layout).
if(POLICY CMP0177)
    cmake_policy(SET CMP0177 NEW)
endif()

# -----------------------------------------------------------------------------
# Compiler selection
# -----------------------------------------------------------------------------

if(NOT CMAKE_TOOLCHAIN_FILE)
    if(CMAKE_HOST_WIN32)
        # LLVM's exported Windows targets carry link.exe-style flags
        # (-INCLUDE:, -delayload:), which only the MSVC-style driver accepts.
        find_program(_CLANG NAMES clang-cl)
        set(_CLANGPP "${_CLANG}")
    else()
        find_program(_CLANG NAMES clang-20 clang-19 clang)
        find_program(_CLANGPP NAMES clang++-20 clang++-19 clang++)
    endif()

    if(_CLANG)
        set(CMAKE_C_COMPILER "${_CLANG}")
    endif()

    if(_CLANGPP)
        set(CMAKE_CXX_COMPILER "${_CLANGPP}")
    endif()
endif()

project(kriol LANGUAGES C CXX)

# -----------------------------------------------------------------------------
# Build configuration
# -----------------------------------------------------------------------------

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Debug CACHE STRING "Build type" FORCE)
endif()

option(KRIOL_STATIC "Statically link the kriol binary in Release builds" OFF)
# WASI output needs a WASI sysroot at build time; on Windows it is opt-in until
# the sysroot and linker discovery are wired up there.
if(WIN32)
    set(_KRIOL_ENABLE_WASM_DEFAULT OFF)
else()
    set(_KRIOL_ENABLE_WASM_DEFAULT ON)
endif()

option(KRIOL_ENABLE_WASM "Build the kriol compiler with wasm32-wasi output support" ${_KRIOL_ENABLE_WASM_DEFAULT})
option(KRIOL_WASI_ENABLE_GC "Build and embed Boehm GC for wasm32-wasi output" OFF)
option(KRIOL_USE_EMBEDDED_LLD "Use LLD as an embedded library for wasm32-wasi final linking" OFF)
option(KRIOL_ENABLE_FUZZING "Build libFuzzer targets for the Kriol compiler" OFF)

set(KRIOL_MINGW_SYSROOT "" CACHE PATH
    "Root of an llvm-mingw (msvcrt) toolchain; enables x86_64-windows output")
if(KRIOL_MINGW_SYSROOT)
    set(KRIOL_ENABLE_WINDOWS_TARGET ON)
else()
    set(KRIOL_ENABLE_WINDOWS_TARGET OFF)
endif()

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Unix toolchains need libm spelled out; the Windows C runtimes include it.
set(KRIOL_SYSTEM_LIBS)
if(UNIX)
    list(APPEND KRIOL_SYSTEM_LIBS m)
endif()

# -----------------------------------------------------------------------------
# Tools
# -----------------------------------------------------------------------------

find_program(CLANG_PROGRAM NAMES clang-20 clang-19 clang REQUIRED)
find_program(VALGRIND valgrind)

# -----------------------------------------------------------------------------
# LLVM
# -----------------------------------------------------------------------------

find_package(LLVM REQUIRED CONFIG)

message(STATUS "Found LLVM ${LLVM_PACKAGE_VERSION}")

# The Windows LLVM release archives record the DIA SDK library by the absolute
# path it had on LLVM's build machine; point it at the local Visual Studio copy.
if(WIN32 AND TARGET LLVMDebugInfoPDB)
    get_target_property(_KRIOL_PDB_LIBS LLVMDebugInfoPDB INTERFACE_LINK_LIBRARIES)

    if(_KRIOL_PDB_LIBS MATCHES "([^;]*diaguids\\.lib)" AND NOT EXISTS "${CMAKE_MATCH_1}")
        set(_KRIOL_STALE_DIAGUIDS "${CMAKE_MATCH_1}")

        set(_KRIOL_VS_DIRS "$ENV{VSINSTALLDIR}")
        set(_KRIOL_PROGRAM_FILES_X86 "ProgramFiles(x86)")
        set(_KRIOL_VSWHERE "$ENV{${_KRIOL_PROGRAM_FILES_X86}}/Microsoft Visual Studio/Installer/vswhere.exe")
        if(EXISTS "${_KRIOL_VSWHERE}")
            execute_process(
                COMMAND "${_KRIOL_VSWHERE}" -latest -products * -property installationPath
                OUTPUT_VARIABLE _KRIOL_VSWHERE_DIR
                OUTPUT_STRIP_TRAILING_WHITESPACE
            )
            list(APPEND _KRIOL_VS_DIRS "${_KRIOL_VSWHERE_DIR}")
        endif()

        set(_KRIOL_DIA_HINTS)
        foreach(_KRIOL_VS_DIR IN LISTS _KRIOL_VS_DIRS)
            list(APPEND _KRIOL_DIA_HINTS "${_KRIOL_VS_DIR}/DIA SDK/lib/amd64")
        endforeach()

        find_library(KRIOL_DIAGUIDS_LIBRARY diaguids HINTS ${_KRIOL_DIA_HINTS})
        if(NOT KRIOL_DIAGUIDS_LIBRARY)
            message(FATAL_ERROR
                "LLVM links against '${_KRIOL_STALE_DIAGUIDS}', which does not exist here.\n"
                "Install the Visual Studio C++ tools (they include the DIA SDK) or pass "
                "-DKRIOL_DIAGUIDS_LIBRARY=<path to diaguids.lib>."
            )
        endif()

        list(TRANSFORM _KRIOL_PDB_LIBS REPLACE ".*diaguids\\.lib$" "${KRIOL_DIAGUIDS_LIBRARY}")
        set_target_properties(LLVMDebugInfoPDB PROPERTIES INTERFACE_LINK_LIBRARIES "${_KRIOL_PDB_LIBS}")
        message(STATUS "Using DIA SDK: ${KRIOL_DIAGUIDS_LIBRARY}")
    endif()
endif()

set(KRIOL_LLVM_COMPONENTS
    core
    native
    linker
    bitreader
    passes
)

if(KRIOL_ENABLE_WASM)
    list(APPEND KRIOL_LLVM_COMPONENTS
        WebAssemblyCodeGen
        WebAssemblyDesc
        WebAssemblyInfo
        WebAssemblyUtils
    )
endif()

llvm_map_components_to_libnames(
    LLVM_LIBS
    ${KRIOL_LLVM_COMPONENTS}
)

separate_arguments(
    LLVM_DEFINITIONS_LIST
    UNIX_COMMAND
    "${LLVM_DEFINITIONS}"
)

list(FILTER LLVM_DEFINITIONS_LIST EXCLUDE REGEX "^-fno-exceptions$")
list(FILTER LLVM_DEFINITIONS_LIST EXCLUDE REGEX "^-std=")

set(KRIOL_LLD_LIBS)
if(KRIOL_USE_EMBEDDED_LLD)
    if(NOT KRIOL_ENABLE_WASM)
        message(FATAL_ERROR "KRIOL_USE_EMBEDDED_LLD requires KRIOL_ENABLE_WASM=ON")
    endif()

    string(REGEX MATCH "^[0-9]+" LLVM_VERSION_MAJOR "${LLVM_PACKAGE_VERSION}")

    find_path(
        LLD_INCLUDE_DIR
        lld/Common/Driver.h
        HINTS ${LLVM_INCLUDE_DIRS}
        REQUIRED
    )

    find_library(
        LLD_COMMON_LIBRARY
        NAMES lldCommon
        HINTS ${LLVM_LIBRARY_DIRS}
        REQUIRED
    )

    find_library(
        LLD_WASM_LIBRARY
        NAMES lldWasm
        HINTS ${LLVM_LIBRARY_DIRS}
        REQUIRED
    )

    find_library(
        LLVM_MONOLITHIC_LIBRARY
        NAMES LLVM-${LLVM_VERSION_MAJOR} LLVM
        HINTS ${LLVM_LIBRARY_DIRS}
        REQUIRED
    )

    set(LLVM_LIBS ${LLVM_MONOLITHIC_LIBRARY})

    list(APPEND KRIOL_LLD_LIBS
        ${LLD_WASM_LIBRARY}
        ${LLD_COMMON_LIBRARY}
    )

    message(STATUS "Using embedded LLD wasm linker")
    message(STATUS "  LLD include: ${LLD_INCLUDE_DIR}")
    message(STATUS "  LLD wasm lib: ${LLD_WASM_LIBRARY}")
    message(STATUS "  LLD common lib: ${LLD_COMMON_LIBRARY}")
    message(STATUS "  LLVM lib: ${LLVM_MONOLITHIC_LIBRARY}")
endif()

# -----------------------------------------------------------------------------
# Boehm GC
# -----------------------------------------------------------------------------

include(cmake/BoehmGC.cmake)

# -----------------------------------------------------------------------------
# Bison / Flex
# -----------------------------------------------------------------------------

find_package(BISON)
find_package(FLEX)

if(BISON_FOUND AND FLEX_FOUND)

    BISON_TARGET(
        KriolParser
        ${CMAKE_SOURCE_DIR}/rules/parser.y
        ${CMAKE_BINARY_DIR}/parser.cc
        DEFINES_FILE ${CMAKE_BINARY_DIR}/parser.hh
        COMPILE_FLAGS "-dt"
    )

    FLEX_TARGET(
        KriolScanner
        ${CMAKE_SOURCE_DIR}/rules/scanner.l
        ${CMAKE_BINARY_DIR}/scanner.cc
    )

    ADD_FLEX_BISON_DEPENDENCY(KriolScanner KriolParser)

    set(PARSER_SRC ${BISON_KriolParser_OUTPUTS})
    set(SCANNER_SRC ${FLEX_KriolScanner_OUTPUTS})

else()

    message(STATUS "Using pre-generated parser/scanner")

    set(PARSER_SRC  ${CMAKE_SOURCE_DIR}/parser.cc)
    set(SCANNER_SRC ${CMAKE_SOURCE_DIR}/scanner.cc)

endif()

# -----------------------------------------------------------------------------
# Sources
# -----------------------------------------------------------------------------

set(KRIOL_CORE_SOURCES
    src/kriol/sema.cc
    src/kriol/type_rules.cc
    ${PARSER_SRC}
    ${SCANNER_SRC}
)

set(KRIOL_LLVM_SOURCES
    src/kriol/cli.cc
    src/kriol/codegen.cc
    src/kriol/codegen_prelude.cc
)

# -----------------------------------------------------------------------------
# Embedded resources
# -----------------------------------------------------------------------------

include(cmake/EmbeddedResources.cmake)

# -----------------------------------------------------------------------------
# Executable
# -----------------------------------------------------------------------------

# The compiler itself, shared by kriol and the test drivers so its sources
# (codegen.cc embeds several MB of runtime libraries) compile only once.
add_library(
    kriol_core
    STATIC
    ${KRIOL_CORE_SOURCES}
    ${KRIOL_LLVM_SOURCES}
)

add_executable(
    kriol
    main.cc
)

# Test drivers build only for `check` (or when named), not by default.
add_executable(
    kriol_api_compile_memory_test
    EXCLUDE_FROM_ALL
    tests/api/compile_memory.cc
)

add_executable(
    kriol_type_model_test
    EXCLUDE_FROM_ALL
    tests/api/type_model.cc
)

add_executable(
    kriol_gc_runtime_test
    EXCLUDE_FROM_ALL
    tests/runtime/gc_stress.c
    runtime/kriol_runtime.c
)

add_executable(
    kriol_compile_fuzz_smoke
    EXCLUDE_FROM_ALL
    tests/fuzz/compile_fuzz_smoke.cc
)

add_executable(
    kriol_compile_random_fuzz
    EXCLUDE_FROM_ALL
    tests/fuzz/compile_random_fuzz.cc
)

if(KRIOL_ENABLE_FUZZING)
    if(NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
        message(FATAL_ERROR "KRIOL_ENABLE_FUZZING requires Clang with libFuzzer support")
    endif()

    # libFuzzer needs the compiler sources built with its instrumentation, so
    # this target compiles them itself instead of using kriol_core.
    add_executable(
        kriol_compile_fuzzer
        tests/fuzz/compile_fuzzer.cc
        ${KRIOL_CORE_SOURCES}
        ${KRIOL_LLVM_SOURCES}
    )
endif()

set_source_files_properties(
    ${KRIOL_LLVM_SOURCES}
    PROPERTIES COMPILE_OPTIONS "${LLVM_DEFINITIONS_LIST}"
)

add_dependencies(kriol_core kriol_embedded_resources)

if(KRIOL_ENABLE_FUZZING)
    add_dependencies(kriol_compile_fuzzer kriol_embedded_resources)
endif()

function(kriol_configure_target TARGET_NAME)
    target_include_directories(
        ${TARGET_NAME}
        BEFORE PRIVATE
        ${BDWGC_DIR}/include
    )

    target_include_directories(
        ${TARGET_NAME}
        PRIVATE
        ${CMAKE_SOURCE_DIR}
        ${CMAKE_BINARY_DIR}
        ${LLVM_INCLUDE_DIRS}
        ${LLD_INCLUDE_DIR}
    )

    if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
        target_compile_options(
            ${TARGET_NAME}
            PRIVATE
            $<$<CONFIG:Debug>:/W3>
        )
    else()
        target_compile_options(
            ${TARGET_NAME}
            PRIVATE
            $<$<CONFIG:Debug>:-g -Wall>
            $<$<CONFIG:Release>:-O2 -DNDEBUG>
        )
    endif()

    # The compiler recurses over the syntax tree; give it the 8 MiB stack it
    # gets on Linux instead of the 1 MiB Windows default.
    if(WIN32)
        if(CMAKE_CXX_COMPILER_FRONTEND_VARIANT STREQUAL "MSVC")
            target_link_options(${TARGET_NAME} PRIVATE /STACK:8388608)
        else()
            target_link_options(${TARGET_NAME} PRIVATE -Wl,--stack,8388608)
        endif()
    endif()

    target_compile_definitions(
        ${TARGET_NAME}
        PRIVATE
        KRIOL_ENABLE_WASM=$<BOOL:${KRIOL_ENABLE_WASM}>
        KRIOL_WASI_ENABLE_GC=$<BOOL:${KRIOL_WASI_ENABLE_GC}>
        KRIOL_USE_EMBEDDED_LLD=$<BOOL:${KRIOL_USE_EMBEDDED_LLD}>
        KRIOL_ENABLE_WINDOWS_TARGET=$<BOOL:${KRIOL_ENABLE_WINDOWS_TARGET}>
        KRIOL_WASI_TARGET="${KRIOL_WASI_TARGET}"
    )
endfunction()

kriol_configure_target(kriol_core)
kriol_configure_target(kriol)
kriol_configure_target(kriol_api_compile_memory_test)
kriol_configure_target(kriol_type_model_test)
kriol_configure_target(kriol_gc_runtime_test)
kriol_configure_target(kriol_compile_fuzz_smoke)
kriol_configure_target(kriol_compile_random_fuzz)

if(KRIOL_ENABLE_FUZZING)
    kriol_configure_target(kriol_compile_fuzzer)
    target_compile_options(
        kriol_compile_fuzzer
        PRIVATE
        -fsanitize=fuzzer,address,undefined
        -fno-omit-frame-pointer
    )
    target_link_options(
        kriol_compile_fuzzer
        PRIVATE
        -fsanitize=fuzzer,address,undefined
    )
endif()

# -----------------------------------------------------------------------------
# Linking
# -----------------------------------------------------------------------------

if(KRIOL_STATIC AND CMAKE_BUILD_TYPE STREQUAL "Release" AND NOT WIN32)
    list(FILTER LLVM_LIBS EXCLUDE REGEX "z3")
    target_link_options(
        kriol
        PRIVATE
        -static-libgcc
        -static-libstdc++
    )
endif()

target_link_libraries(
    kriol_core
    PUBLIC
    ${KRIOL_LLD_LIBS}
    ${LLVM_LIBS}
    ${KRIOL_SYSTEM_LIBS}
)

foreach(_KRIOL_COMPILER_TARGET IN ITEMS
    kriol
    kriol_api_compile_memory_test
    kriol_compile_fuzz_smoke
    kriol_compile_random_fuzz
)
    target_link_libraries(${_KRIOL_COMPILER_TARGET} PRIVATE kriol_core)
endforeach()

target_link_libraries(
    kriol_gc_runtime_test
    PRIVATE
    gc
)

if(KRIOL_ENABLE_FUZZING)
    target_link_libraries(
        kriol_compile_fuzzer
        PRIVATE
        ${KRIOL_LLD_LIBS}
        ${LLVM_LIBS}
        ${KRIOL_SYSTEM_LIBS}
    )
endif()

add_custom_target(
    random-fuzz
    COMMAND
        $<TARGET_FILE:kriol_compile_random_fuzz>
    DEPENDS kriol_compile_random_fuzz
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)

# -----------------------------------------------------------------------------
# Installation
# -----------------------------------------------------------------------------

# `cmake --install` lays out a complete distribution. The release archives are
# flat (configured with -DCMAKE_INSTALL_BINDIR=. -DCMAKE_INSTALL_DOCDIR=.).

include(GNUInstallDirs)

install(TARGETS kriol RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})

# kriol.exe links Windows programs with the ld.lld next to it.
if(WIN32 AND KRIOL_ENABLE_WINDOWS_TARGET)
    find_program(KRIOL_BUNDLED_LLD NAMES ld.lld HINTS ${LLVM_TOOLS_BINARY_DIR} NO_DEFAULT_PATH)
    if(NOT KRIOL_BUNDLED_LLD)
        message(FATAL_ERROR "ld.lld was not found in ${LLVM_TOOLS_BINARY_DIR}; it is installed next to kriol.exe")
    endif()
    install(PROGRAMS ${KRIOL_BUNDLED_LLD} DESTINATION ${CMAKE_INSTALL_BINDIR})
endif()

install(FILES LICENSE README.md DESTINATION ${CMAKE_INSTALL_DOCDIR})

# Licenses of everything kriol bundles or embeds into the programs it builds.
set(_KRIOL_LICENSE_DIR ${CMAKE_INSTALL_DOCDIR}/licenses)
install(FILES licenses/LLVM.txt DESTINATION ${_KRIOL_LICENSE_DIR})
install(FILES ${BDWGC_DIR}/README.QUICK DESTINATION ${_KRIOL_LICENSE_DIR} RENAME bdwgc.txt)

if(KRIOL_ENABLE_WINDOWS_TARGET)
    set(_KRIOL_MINGW_LICENSE
        ${KRIOL_MINGW_SYSROOT}/x86_64-w64-mingw32/share/mingw32/COPYING.MinGW-w64-runtime.txt)
    if(NOT EXISTS ${_KRIOL_MINGW_LICENSE})
        message(FATAL_ERROR "MinGW-w64 runtime license not found at ${_KRIOL_MINGW_LICENSE}")
    endif()
    install(FILES ${_KRIOL_MINGW_LICENSE} DESTINATION ${_KRIOL_LICENSE_DIR} RENAME MinGW-w64-runtime.txt)
endif()

if(KRIOL_ENABLE_WASM)
    # Debian and Ubuntu's wasi-libc package; other sysroots may not ship one.
    set(_KRIOL_WASI_LICENSE ${KRIOL_WASI_SYSROOT}/share/doc/wasi-libc/copyright)
    if(EXISTS ${_KRIOL_WASI_LICENSE})
        install(FILES ${_KRIOL_WASI_LICENSE} DESTINATION ${_KRIOL_LICENSE_DIR} RENAME wasi-libc.txt)
    else()
        message(WARNING "wasi-libc license not found at ${_KRIOL_WASI_LICENSE}; it will be missing from installs")
    endif()
endif()

# -----------------------------------------------------------------------------
# Utility targets
# -----------------------------------------------------------------------------

if(VALGRIND)

    add_custom_target(
        valgrind
        COMMAND ${VALGRIND}
                --leak-check=full
                --show-leak-kinds=all
                --error-exitcode=1
                $<TARGET_FILE:kriol>
                "$ENV{KRIOL_FILE}"
        DEPENDS kriol
    )

endif()

add_custom_target(
    check
    COMMAND
        $<TARGET_FILE:kriol_api_compile_memory_test>
    COMMAND
        $<TARGET_FILE:kriol_type_model_test>
    COMMAND
        $<TARGET_FILE:kriol_gc_runtime_test>
    COMMAND
        $<TARGET_FILE:kriol_compile_fuzz_smoke>
    COMMAND
        ${CMAKE_SOURCE_DIR}/run_tests.sh
        $<TARGET_FILE:kriol>
        ${CMAKE_SOURCE_DIR}
    DEPENDS kriol kriol_api_compile_memory_test kriol_type_model_test kriol_gc_runtime_test kriol_compile_fuzz_smoke
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
)
