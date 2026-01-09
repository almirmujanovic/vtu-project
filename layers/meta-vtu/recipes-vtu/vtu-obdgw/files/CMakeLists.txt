cmake_minimum_required(VERSION 3.14)
project(vtu-obdgw VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find libvtu-common
find_library(VTU_COMMON_LIB vtu-common REQUIRED)
find_path(VTU_COMMON_INCLUDE vtu/can_defs.h REQUIRED)

add_executable(vtu-obdgw src/obdgw_main.c)

target_include_directories(vtu-obdgw PRIVATE ${VTU_COMMON_INCLUDE})
target_link_libraries(vtu-obdgw PRIVATE ${VTU_COMMON_LIB})

install(TARGETS vtu-obdgw RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})