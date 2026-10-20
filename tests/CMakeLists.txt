cmake_minimum_required(VERSION 3.16)

add_executable(test_takuzu test_takuzu.cpp)
target_link_libraries(test_takuzu PRIVATE takuzu_core)
add_test(NAME test_takuzu COMMAND test_takuzu)

add_executable(test_vision test_vision.cpp)
target_link_libraries(test_vision PRIVATE vision_core)
add_test(NAME test_vision COMMAND test_vision)
