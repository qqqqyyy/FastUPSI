include_directories(${PROJECT_SOURCE_DIR}/upsi)


add_executable(setup setup.cpp)
target_link_libraries(setup upsi)


add_executable(main main.cpp)
target_link_libraries(main upsi)