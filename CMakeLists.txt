cmake_minimum_required(VERSION 3.15)

project(FastUPSI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_FLAGS "-Wall -Wfatal-errors")
set(CMAKE_CXX_FLAGS "-O3 -DNDEBUG")

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} ${COMMON_FLAGS}")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${COMMON_FLAGS}")

find_package(libOTe REQUIRED HINTS ${CMAKE_CURRENT_SOURCE_DIR}/libOTe)

if (ENABLE_SOFTSPOKEN_OT)
    message(STATUS "SoftSpokenOT enabled")
endif()

if (ENABLE_SILENT_VOLE)
    message(STATUS "SlientVole enabled")
endif()

add_subdirectory(upsi)

add_subdirectory(tests)