# Public headers for the ReHLDS SV_Frame hook. No engine binaries are linked.
if(NOT EXISTS "${PROJECT_SOURCE_DIR}/dep/rehlds/rehlds/public/rehlds/rehlds_api.h")
    message(FATAL_ERROR "Missing ReHLDS SDK. Run git submodule update --init --recursive.")
endif()

add_library(amxx_module_sdk INTERFACE)
target_include_directories(amxx_module_sdk INTERFACE ${CMAKE_CURRENT_SOURCE_DIR})
target_include_directories(amxx_module_sdk SYSTEM INTERFACE
    ${PROJECT_SOURCE_DIR}/dep/rehlds/rehlds/common
    ${PROJECT_SOURCE_DIR}/dep/rehlds/rehlds/dlls
    ${PROJECT_SOURCE_DIR}/dep/rehlds/rehlds/engine
    ${PROJECT_SOURCE_DIR}/dep/rehlds/rehlds/pm_shared
    ${PROJECT_SOURCE_DIR}/dep/rehlds/rehlds/public
    ${PROJECT_SOURCE_DIR}/dep/rehlds/rehlds/public/rehlds
)
if(WIN32)
    target_compile_definitions(amxx_module_sdk INTERFACE NOMINMAX)
endif()

# Module
add_library(amxx_mqtt_client SHARED
    sdk/amxxmodule.cpp
    module.cpp
    natives.cpp

    CallbackDispatcher.cpp
    RehldsFrameHook.cpp
    MqttClient.cpp
    AmxxMqttClient.cpp
    MqttClientMgr.cpp
    ConnectOption.cpp
    ConnectOptionMgr.cpp
)

target_include_directories(amxx_mqtt_client PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}
    ${PROJECT_SOURCE_DIR}/dep/include
)

set_target_properties(amxx_mqtt_client PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mqtt_client_amxx$<$<BOOL:${UNIX}>:_i386>"
    PDB_NAME amxx_mqtt_client
    RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
    LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}
)

if(AMXX_MQTT_GNU)
    include(../flags.cmake)
    separate_arguments(_amxx_gnu_compile_flags NATIVE_COMMAND "${COMPILE_FLAGS}")
    target_compile_options(amxx_mqtt_client PRIVATE ${_amxx_gnu_compile_flags})
    target_link_options(amxx_mqtt_client PRIVATE
        $<$<NOT:$<CONFIG:Debug>>:-s>
    )
elseif(AMXX_MQTT_MSVC)
    target_compile_definitions(amxx_mqtt_client PRIVATE
        NOMINMAX
        HAVE_STDINT_H
        NO_MSVC8_AUTO_COMPAT
        _WIN32
        _WIN32_WINNT=0x0600
        WIN32
        WINVER=0x0600
    )
    target_compile_options(amxx_mqtt_client PRIVATE
        /GR-
        /EHsc
        /W4
        $<$<CONFIG:Release>:/Zi>
    )
    target_link_options(amxx_mqtt_client PRIVATE
        $<$<CONFIG:Release>:/LTCG>
        $<$<CONFIG:Release>:/INCREMENTAL:NO>
        $<$<CONFIG:Release>:/DEBUG>
        $<$<CONFIG:Release>:/OPT:REF>
        $<$<CONFIG:Release>:/OPT:ICF>
    )
endif()

target_link_libraries(amxx_mqtt_client PRIVATE amxx_module_sdk paho-mqttpp3-static ${CMAKE_DL_LIBS})
