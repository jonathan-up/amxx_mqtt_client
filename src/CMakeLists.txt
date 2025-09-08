# Module
add_library(amxx_mqtt_client SHARED
    sdk/amxxmodule.cpp
    module.cpp
    natives.cpp

    CallbackDispatcher.cpp
    MqttClient.cpp
    AmxxMqttClient.cpp
    MqttClientMgr.cpp
    ConnectOptionMgr.cpp
)

include(../flags.cmake)
set_target_properties(amxx_mqtt_client PROPERTIES
    PREFIX ""
    OUTPUT_NAME "mqtt_client_amxx$<$<BOOL:${UNIX}>:_i386>"
    PDB_NAME amxx_mqtt_client
    COMPILE_FLAGS ${COMPILE_FLAGS}
    LINK_FLAGS ${LINK_FLAGS}
)

target_link_libraries(amxx_mqtt_client paho-mqttpp3-static)
