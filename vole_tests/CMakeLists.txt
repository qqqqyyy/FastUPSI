
add_executable(vole_test VoleTest.cpp)
target_link_libraries(vole_test oc::libOTe)

add_executable(vole_test_sender Sender.cpp)
target_link_libraries(vole_test_sender oc::libOTe)

add_executable(vole_test_receiver Receiver.cpp)
target_link_libraries(vole_test_receiver oc::libOTe)