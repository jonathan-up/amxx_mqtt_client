cmake_minimum_required(VERSION 3.20)

# CMake policy
set(CMAKE_POLICY_DEFAULT_CMP0077 NEW)
set(CMAKE_POLICY_DEFAULT_CMP0117 NEW)

project(amxx_mqtt_client LANGUAGES CXX)

# Force all runtime targets (DLL/SO) into the build root, regardless of generator or source-directory layout
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Compiler check
if(NOT CMAKE_CXX_COMPILER_ID STREQUAL "GNU" AND NOT CMAKE_CXX_COMPILER_ID STREQUAL "MSVC")
    message(FATAL_ERROR "Unsupported compiler: ${CMAKE_CXX_COMPILER_ID}. Only GCC and MSVC are supported.")
endif()

if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
    set(AMXX_MQTT_GNU ON)
endif()

if(MSVC)
    set(AMXX_MQTT_MSVC ON)
    set(CMAKE_MSVC_RUNTIME_LIBRARY "MultiThreaded$<$<CONFIG:Debug>:Debug>")
    add_compile_options(/wd4819 /wd4201)
endif()

# GCC specific settings
if(AMXX_MQTT_GNU)
    add_compile_options(-m32)
    add_link_options(-m32 -static-libstdc++ -static-libgcc)
endif()

# MSVC specific settings
if(AMXX_MQTT_MSVC AND CMAKE_SIZEOF_VOID_P EQUAL 8)
    message(WARNING "AMXX modules are 32-bit native libraries. Use a 32-bit MSVC toolchain, e.g. cmake -G \"Visual Studio 17 2022\" -A Win32 ...")
endif()

set(PAHO_WITH_MQTT_C ON CACHE BOOL "Build Paho C from dep/mqtt-cpp/externals/paho.mqtt.c")
set(PAHO_BUILD_STATIC ON CACHE BOOL "Build static Paho MQTT C++ library")

add_subdirectory(dep/mqtt-cpp)
add_subdirectory(src)
