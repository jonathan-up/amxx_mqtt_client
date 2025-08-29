cmake_minimum_required(VERSION 3.30)
project(amxx_mqtt_client)

set(CMAKE_CXX_STANDARD 17)
SET(CMAKE_CXX_STANDARD_REQUIRED ON)

# Global include directories
include_directories(
    ${PROJECT_SOURCE_DIR}
)

#Global definitions
add_compile_definitions()

# Module
include(flags.cmake)
add_library(amxx_mqtt_client SHARED
    src/sdk/amxxmodule.cpp
    src/module.cpp
    src/natives.cpp
)
set_target_properties(amxx_mqtt_client PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mqtt_client_amxx$<$<BOOL:${UNIX}>:_i386>"
    PDB_NAME amxx_mqtt_client
    COMPILE_FLAGS ${COMPILE_FLAGS}
    LINK_FLAGS ${LINK_FLAGS}
)
