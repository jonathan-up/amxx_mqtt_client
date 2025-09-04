//
// Created by jonathan on 8/29/25.
//

#include <string>
#include "natives.h"

#include "MqttClientMgr.h"
#include "CallbackDispatcher.h"

cell AMX_NATIVE_CALL mqtt_create(AMX *amx, cell *params) {
    enum { arg_count, arg_blocker, arg_client_id };

    if (params[arg_count] < 3) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    const std::string blocker{MF_GetAmxString(amx, params[arg_blocker], arg_blocker - 1, nullptr)};
    const std::string clientId{MF_GetAmxString(amx, params[arg_client_id], arg_client_id - 1, nullptr)};

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

    const AmxxMqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }

    client->setConnectedHandler([&client](const mqtt::string &) {
        ConnectedCallbackDispatcher(client);
    });
    client->setMessageHandler([&client](const mqtt::const_message_ptr &msg) {
        MessageCallbackDispatcher(client, msg);
    });
    client->setConnectioLostHandler([&client](const mqtt::string &msg) {
        ConnectionLostCallbackDispatcher(client, msg);
    });
    client->setDisconnectHandler([&client](const mqtt::properties &, const mqtt::ReasonCode code) {
        DisconnectedCallbackDispatcher(client, code);
    });

    client->connect();
    return 1;
}

cell AMX_NATIVE_CALL mqtt_is_connect(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    if (params[arg_count] < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    const AmxxMqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }
    return client->isConnected();
}

cell AMX_NATIVE_CALL mqtt_set_message_callback(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_handler };

    if (params[arg_count] < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    AmxxMqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }

    const std::string handler{MF_GetAmxString(amx, params[arg_handler], arg_handler - 1, nullptr)};

    const int forwardId = MF_RegisterSPForwardByName(amx, handler.c_str());
    if (forwardId != -1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Callback function \"%s\" is not exists", handler.c_str());
        return 0;
    }

    client->setOnMessageForwardId(forwardId);
    return 1;
}

AMX_NATIVE_INFO g_natives[] =
{
    {"mqtt_create", mqtt_create},
    {"mqtt_destroy", mqtt_destroy},
    {"mqtt_connect", mqtt_connect},
    {"mqtt_is_connect", mqtt_is_connect},
    {"mqtt_set_message_callback", mqtt_set_message_callback},
    {nullptr, nullptr}
};
