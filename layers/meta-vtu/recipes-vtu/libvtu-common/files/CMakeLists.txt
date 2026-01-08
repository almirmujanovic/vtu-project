cmake_minimum_required(VERSION 3.14)
project(vtu-common VERSION 1.0.0 LANGUAGES C)

# Create shared library
add_library(vtu-common SHARED
    src/vtu_common.c
)

# Set library version
set_target_properties(vtu-common PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
)

# Include directories
target_include_directories(vtu-common
    PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
        $<INSTALL_INTERFACE:include>
)

# Install the library
install(TARGETS vtu-common
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Install header files
install(DIRECTORY include/vtu
    DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)