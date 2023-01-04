cmake_minimum_required(VERSION 3.0)

project(curveball LANGUAGES C)

add_executable(curveball main.c)

install(TARGETS curveball RUNTIME DESTINATION bin)
