cmake_minimum_required(VERSION 3.30)
project(amxx_mqtt_client)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_CXX_FLAGS "${CMAKE_C_FLAGS} -m32")
set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")
set(CMAKE_SHARED_LINKER_FLAGS "${CMAKE_SHARED_LINKER_FLAGS} -m32")

# Global include directories
include_directories(
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR/dep/include}
)

#Global definitions
add_compile_definitions()

set(PAHO_WITH_MQTT_C TRUE)
set(PAHO_BUILD_STATIC TRUE)
set(PAHO_ENABLE_TESTING FALSE)
set(PAHO_WITH_SSL FALSE)
add_subdirectory(dep/mqtt-cpp)

add_subdirectory(src)
