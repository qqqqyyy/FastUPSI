include_directories(${PROJECT_SOURCE_DIR}/upsi)


add_executable(vole_test VoleTest.cpp)
target_link_libraries(vole_test oc::libOTe)

add_executable(vole_sender VoleSender.cpp)
target_link_libraries(vole_sender oc::libOTe)

add_executable(vole_receiver VoleReceiver.cpp)
target_link_libraries(vole_receiver oc::libOTe)


add_executable(poly_test PolyTest.cpp)
target_link_libraries(poly_test oc::libOTe tree)