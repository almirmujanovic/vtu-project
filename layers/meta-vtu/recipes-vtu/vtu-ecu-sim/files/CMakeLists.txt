cmake_minimum_required(VERSION 3.14)
project(vtu-ecu-sim VERSION 1.0.0 LANGUAGES C)

# Find our libvtu-common library
# The headers and library are in the sysroot during cross-compilation
find_path(VTU_INCLUDE_DIR vtu/can_defs.h)
find_library(VTU_LIBRARY vtu-common)

# Create executable
add_executable(vtu-ecu-sim
    src/ecu_sim.c
)

# Include directories
target_include_directories(vtu-ecu-sim PRIVATE
    ${VTU_INCLUDE_DIR}
)

# Link against our library
# Also link pthread for threading (timer implementation)
target_link_libraries(vtu-ecu-sim PRIVATE
    ${VTU_LIBRARY}
    pthread
    m
)

# Install the executable
install(TARGETS vtu-ecu-sim
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)