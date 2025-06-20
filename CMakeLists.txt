cmake_minimum_required(VERSION 3.15)
project(FastUPSI)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(COMMON_FLAGS "-Wall -Wfatal-errors")
set(CMAKE_CXX_FLAGS_RELEASE "-O3 -DNDEBUG")
set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -ggdb")

find_package(libOTe REQUIRED HINTS ${CMAKE_CURRENT_SOURCE_DIR}/libOTe)

add_executable(vole_test VoleTest.cpp)
target_link_libraries(vole_test oc::libOTe)


add_executable(sender Sender.cpp)
target_link_libraries(sender oc::libOTe)


add_executable(receiver Receiver.cpp)
target_link_libraries(receiver oc::libOTe)

