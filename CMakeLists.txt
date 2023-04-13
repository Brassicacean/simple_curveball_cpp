cmake_minimum_required(VERSION 3.0)

project(curveball LANGUAGES C CXX)

add_executable(curveball main.cpp)

set_target_properties(curveball PROPERTIES LINKER_LANGUAGE CXX)

install(TARGETS curveball RUNTIME DESTINATION bin)
