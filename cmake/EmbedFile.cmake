# Portable replacement for `xxd -i`: turns a binary file into a C array header.
#
# Usage:
#   cmake -DINPUT=<file> -DOUTPUT=<header> [-DSYMBOL=<name>] -P EmbedFile.cmake
#
# The symbol defaults to the input file name made into a C identifier, which is
# the same naming `xxd -i` uses (e.g. libgc_native.a -> libgc_native_a and
# libgc_native_a_len).

if(NOT DEFINED INPUT OR NOT DEFINED OUTPUT)
    message(FATAL_ERROR "EmbedFile.cmake requires -DINPUT=<file> and -DOUTPUT=<header>")
endif()

if(NOT DEFINED SYMBOL)
    get_filename_component(SYMBOL "${INPUT}" NAME)
endif()
string(MAKE_C_IDENTIFIER "${SYMBOL}" SYMBOL)

file(READ "${INPUT}" _hex HEX)
string(LENGTH "${_hex}" _hex_len)
math(EXPR _len "${_hex_len} / 2")

if(_len EQUAL 0)
    set(_bytes "0x00")
else()
    # Break into rows of 16 bytes, then turn every byte into `0xNN,`.
    string(REGEX REPLACE "(................................)" "\\1\n  " _hex "${_hex}")
    string(REGEX REPLACE "([0-9a-f][0-9a-f])" "0x\\1," _bytes "${_hex}")
endif()

file(WRITE "${OUTPUT}.tmp"
    "const unsigned char ${SYMBOL}[] = {\n  ${_bytes}\n};\n"
    "const unsigned int ${SYMBOL}_len = ${_len};\n"
)
file(RENAME "${OUTPUT}.tmp" "${OUTPUT}")
