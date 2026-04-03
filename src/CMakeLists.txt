# Module
add_library(amxx_mqtt_client SHARED
    sdk/amxxmodule.cpp
    module.cpp
    natives.cpp

    CallbackDispatcher.cpp
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

target_link_libraries(amxx_mqtt_client paho-mqttpp3-static)
