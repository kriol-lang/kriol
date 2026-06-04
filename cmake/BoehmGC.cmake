set(BDWGC_DIR "${CMAKE_SOURCE_DIR}/include/external/bdwgc")

if(NOT EXISTS "${BDWGC_DIR}/CMakeLists.txt")
    message(FATAL_ERROR
        "Missing Boehm GC submodule.\n"
        "Run:\n"
        "git submodule update --init include/external/bdwgc"
    )
endif()

if(KRIOL_STATIC AND CMAKE_BUILD_TYPE STREQUAL "Release")
    set(_GC_SHARED OFF)
else()
    set(_GC_SHARED ON)
endif()

set(_OLD_GC_BUILD_SHARED_LIBS ${GC_BUILD_SHARED_LIBS})

set(GC_BUILD_SHARED_LIBS ${_GC_SHARED} CACHE BOOL "" FORCE)

set(enable_docs              OFF CACHE BOOL "" FORCE)
set(enable_gcj_support       OFF CACHE BOOL "" FORCE)
set(enable_java_finalization OFF CACHE BOOL "" FORCE)
set(enable_threads           ON  CACHE BOOL "" FORCE)

add_subdirectory(
    ${BDWGC_DIR}
    _bdwgc
    EXCLUDE_FROM_ALL
)

set(
    GC_BUILD_SHARED_LIBS
    ${_OLD_GC_BUILD_SHARED_LIBS}
    CACHE BOOL "" FORCE
)

message(STATUS "Boehm GC shared=${_GC_SHARED}")