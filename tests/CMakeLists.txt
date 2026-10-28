add_library(fake_rehlds SHARED FakeRehlds.cpp)
target_link_libraries(fake_rehlds PRIVATE amxx_module_sdk)
target_compile_definitions(fake_rehlds PRIVATE FAKE_REHLDS_EXPORTS)
set_target_properties(fake_rehlds PROPERTIES PREFIX "")
if(WIN32)
    set_target_properties(fake_rehlds PROPERTIES OUTPUT_NAME swds)
else()
    set_target_properties(fake_rehlds PROPERTIES OUTPUT_NAME engine_i486)
endif()

add_executable(callback_dispatcher_test
    CallbackDispatcherTest.cpp
    ../src/CallbackDispatcher.cpp
    ../src/RehldsFrameHook.cpp
    ../src/module.cpp
    ../src/MqttClient.cpp
    ../src/MqttClientMgr.cpp
    ../src/AmxxMqttClient.cpp
    ../src/ConnectOption.cpp
    ../src/ConnectOptionMgr.cpp
)
target_link_libraries(callback_dispatcher_test PRIVATE amxx_module_sdk paho-mqttpp3-static fake_rehlds ${CMAKE_DL_LIBS})
add_test(NAME callback_dispatcher COMMAND callback_dispatcher_test)
set_tests_properties(callback_dispatcher PROPERTIES TIMEOUT 30)

find_package(Python3 REQUIRED COMPONENTS Interpreter)
add_executable(mqtt_client_lifetime_test MqttClientLifetimeTest.cpp ../src/MqttClient.cpp)
target_link_libraries(mqtt_client_lifetime_test PRIVATE amxx_module_sdk paho-mqttpp3-static)
add_test(NAME mqtt_client_lifetime
    COMMAND ${Python3_EXECUTABLE} ${CMAKE_CURRENT_SOURCE_DIR}/LocalBrokerTest.py $<TARGET_FILE:mqtt_client_lifetime_test>)
set_tests_properties(mqtt_client_lifetime PROPERTIES TIMEOUT 60)
