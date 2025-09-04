cmake_minimum_required(VERSION 3.15)
project(FastUPSI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_FLAGS "-Wall -Wfatal-errors")
set(CMAKE_CXX_FLAGS "-O3 -DNDEBUG")
set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")

set(_LIBOTE_ROOT "${CMAKE_SOURCE_DIR}/libOTe")
if (EXISTS "${_LIBOTE_ROOT}/out/install")
  list(APPEND CMAKE_PREFIX_PATH "${_LIBOTE_ROOT}/out/install")
  file(GLOB _LIBOTE_PREFIXES "${_LIBOTE_ROOT}/out/install/*")
  list(APPEND CMAKE_PREFIX_PATH ${_LIBOTE_PREFIXES})
endif()
list(APPEND CMAKE_PREFIX_PATH "${_LIBOTE_ROOT}")


find_package(libOTe CONFIG REQUIRED)

add_subdirectory(rbokvs)
add_subdirectory(upsi)
add_subdirectory(tests)
