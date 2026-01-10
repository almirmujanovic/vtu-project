cmake_minimum_required(VERSION 3.14)
project(vtu-telemetry VERSION 1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)

# Find Paho MQTT C library
find_library(PAHO_MQTT_LIB paho-mqtt3c REQUIRED)

add_executable(vtu-telemetry src/telemetry_main.c)

target_link_libraries(vtu-telemetry PRIVATE ${PAHO_MQTT_LIB})

install(TARGETS vtu-telemetry RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})