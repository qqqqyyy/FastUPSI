cmake_minimum_required(VERSION 3.12)
project(rbokvs_gf2_test CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

add_executable(test_rbokvs_gf2
    test/test_rbokvs_gf2.cpp
    rb_okvs_gf2.cpp
    utils_gf2.cpp
)
add_executable(test_rb_psi
    test/test_rb_psi.cpp
    rb_psi.cpp
    rb_okvs_gf2.cpp
    utils_gf2.cpp
)

target_include_directories(test_rbokvs_gf2 PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(test_rb_psi PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
