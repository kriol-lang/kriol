# Turns a binary file into a C array header.
#
# Usage:
#   cmake -DINPUT=<file> -DOUTPUT=<header> [-DSYMBOL=<name>] [-DUSE_EMBED=ON]
#         -P EmbedFile.cmake
#
# The symbol defaults to the input file name made into a C identifier, which is
# the same naming `xxd -i` uses (e.g. libgc_native.a -> libgc_native_a and
# libgc_native_a_len).
#
# With USE_EMBED the header uses the compiler's #embed, which compiles about
# ten times faster than a hex initializer. Otherwise, and for empty files
# (which #embed cannot turn into a valid array), it writes the bytes as hex.

if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "EmbedFile.cmake requires -DINPUT=<file> and -DOUTPUT=<header>")
endif()

if(NOT DEFINED SYMBOL)
    get_filename_component(SYMBOL "${INPUT}" NAME)
endif()
string(MAKE_C_IDENTIFIER "${SYMBOL}" SYMBOL)

file(SIZE "${INPUT}" _size)

if(USE_EMBED AND _size GREATER 0)
    file(TO_CMAKE_PATH "${INPUT}" _path)
    get_filename_component(_path "${_path}" ABSOLUTE)
    file(WRITE "${OUTPUT}.tmp"
        "#if defined(__clang__)\n"
        "#pragma clang diagnostic push\n"
        "#pragma clang diagnostic ignored \"-Wc23-extensions\"\n"
        "#endif\n"
        "const unsigned char ${SYMBOL}[] = {\n"
        "#embed \"${_path}\"\n"
        "};\n"
        "#if defined(__clang__)\n"
        "#pragma clang diagnostic pop\n"
        "#endif\n"
        "const unsigned int ${SYMBOL}_len = ${_size};\n"
    )
else()
    file(READ "${INPUT}" _hex HEX)

    if(_size EQUAL 0)
        set(_bytes "0x00")
    else()
        # Break into rows of 16 bytes, then turn every byte into `0xNN,`.
        string(REGEX REPLACE "(................................)" "\\1\n  " _hex "${_hex}")
        string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_hex}")
    endif()

    file(WRITE "${OUTPUT}.tmp"
        "const unsigned char ${SYMBOL}[] = {\n  ${_bytes}\n};\n"
        "const unsigned int ${SYMBOL}_len = ${_size};\n"
    )
endif()

# The header is rewritten whenever its input changes, so files that include it
# rebuild even where the compiler does not track #embed dependencies.
file(RENAME "${OUTPUT}.tmp" "${OUTPUT}")
