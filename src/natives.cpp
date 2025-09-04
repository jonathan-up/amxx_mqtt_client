//
// Created by jonathan on 8/29/25.
//

#include <string>
#include "natives.h"

#include "MqttClientMgr.h"

cell AMX_NATIVE_CALL mqtt_create(AMX *amx, cell *params) {
    enum { arg_count, arg_blocker, arg_client_id };

    if (params[arg_count] < 3) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    const std::string blocker{MF_GetAmxString(amx, params[arg_blocker], arg_blocker - 1, nullptr)};
    const std::string clientId{MF_GetAmxString(amx, params[arg_client_id], arg_client_id - 1, nullptr)};

    MF_Log("%s|%s", blocker.c_str(), clientId.c_str());

    return g_mqttClientMgr.make(blocker, clientId);
}

cell AMX_NATIVE_CALL mqtt_destroy(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    if (params[arg_count] < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    g_mqttClientMgr.destroy(params[arg_handle]);
    return 1;
}

cell AMX_NATIVE_CALL mqtt_connect(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    if (params[arg_count] < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    const MqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }
    client->connect();
    return 1;
}

cell AMX_NATIVE_CALL mqtt_is_connect(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    if (params[arg_count] < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    const MqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }
    return client->isConnected();
}

cell AMX_NATIVE_CALL mqtt_set_msg_callback(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_handler };

    if (params[arg_count] < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    const MqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }

    client->setMessageHandler([&] (const mqtt::const_message_ptr& msg) {
        client->connect();
    });

    const std::string handler{MF_GetAmxString(amx, params[arg_handler], arg_handler - 1, nullptr)};

    return 1;
}


AMX_NATIVE_INFO g_natives[] =
{
    {"mqtt_create", mqtt_create},
    {"mqtt_destroy", mqtt_destroy},
    {"mqtt_connect", mqtt_connect},
    {"mqtt_is_connect", mqtt_is_connect},
    {nullptr, nullptr}
};
