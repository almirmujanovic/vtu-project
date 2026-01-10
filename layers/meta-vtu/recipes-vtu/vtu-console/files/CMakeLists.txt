cmake_minimum_required(VERSION 3.14)
project(vtu-console VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

add_executable(vtu-console src/console_main.c)

install(TARGETS vtu-console RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})