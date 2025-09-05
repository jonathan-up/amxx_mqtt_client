//
// Created by jonathan on 8/29/25.
//

#include <string>
#include "natives.h"

#include "MqttClientMgr.h"
#include "CallbackDispatcher.h"

#define ARG_COUNT (params[0] / sizeof(cell))

cell AMX_NATIVE_CALL mqtt_create(AMX *amx, cell *params) {
    enum { arg_count, arg_blocker, arg_client_id };

    // check args
    if (params[arg_count] < 3) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    // get blocker & client id
    const std::string blocker{MF_GetAmxString(amx, params[arg_blocker], arg_blocker - 1, nullptr)};
    const std::string clientId{MF_GetAmxString(amx, params[arg_client_id], arg_client_id - 1, nullptr)};

    // make it
    return g_mqttClientMgr.make(blocker, clientId);
}

cell AMX_NATIVE_CALL mqtt_destroy(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    // check args
    if (params[arg_count] < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    // destroy it
    g_mqttClientMgr.destroy(params[arg_handle]);
    return 1;
}

cell AMX_NATIVE_CALL mqtt_connect(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    // check args
    if (params[arg_count] < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    const int handle = params[arg_handle];

    // check handle
    const AmxxMqttClient *client = g_mqttClientMgr.getClient(handle);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", handle);
        return 0;
    }

    // set handlers for client
    client->setConnectedHandler([handle](const mqtt::string &) {
        ConnectedCallbackDispatcher(handle);
    });
    client->setMessageHandler([handle](const mqtt::const_message_ptr &msg) {
        MessageCallbackDispatcher(handle, msg);
    });
    client->setConnectioLostHandler([handle](const mqtt::string &msg) {
        ConnectionLostCallbackDispatcher(handle, msg);
    });
    client->setDisconnectHandler([handle](const mqtt::properties &, const mqtt::ReasonCode code) {
        DisconnectedCallbackDispatcher(handle, code);
    });

    // connect
    client->connect();
    return 1;
}

cell AMX_NATIVE_CALL mqtt_is_connect(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    // check args
    if (params[arg_count] < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    //check clients
    const AmxxMqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }

    // return it
    return client->isConnected();
}

cell AMX_NATIVE_CALL mqtt_subscribe(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_topic };

    // check args
    if (params[arg_count] < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    // check handle
    const AmxxMqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }

    // get topic name & check it
    const std::string topicName{MF_GetAmxString(amx, params[arg_topic], arg_topic - 1, nullptr)};
    if (topicName.empty()) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Topic name could not be empty");
        return 0;
    }

    // subscribe
    client->subscribe(topicName.c_str());

    return 1;
}

cell AMX_NATIVE_CALL mqtt_publish(AMX *amx, cell *params) {

    enum { arg_count, arg_handle, arg_topic, arg_data };

    // check args
    if (ARG_COUNT < 3) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
        return 0;
    }

    // check handle
    const AmxxMqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }

    // get topic name & check it
    const std::string topicName{MF_GetAmxString(amx, params[arg_topic], arg_topic - 1, nullptr)};
    if (topicName.empty()) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Topic name could not be empty");
        return 0;
    }

    // get data
    const std::string data{MF_GetAmxString(amx, params[arg_data], arg_data - 1, nullptr)};

    // subscribe
    client->publish(topicName, data);

    return 1;
}

cell AMX_NATIVE_CALL mqtt_set_connected_callback(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_callback };

    // check args
    if (params[arg_count] < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    // check handle
    AmxxMqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }

    // get callback
    const std::string callback{MF_GetAmxString(amx, params[arg_callback], arg_callback - 1, nullptr)};

    // check callback
    const int forwardId = MF_RegisterSPForwardByName(amx, callback.c_str(), FP_CELL, FP_DONE);
    if (forwardId == -1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Callback function \"%s\" is not exists", callback.c_str());
        return 0;
    }

    // set it
    client->setOnConnectedForwardId(forwardId);
    return 1;
}

cell AMX_NATIVE_CALL mqtt_set_message_callback(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_callback };

    // check args
    if (params[arg_count] < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to function, the inc file incorrect?");
        return 0;
    }

    // check handle
    AmxxMqttClient *client = g_mqttClientMgr.getClient(params[arg_handle]);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_handle]);
        return 0;
    }

    // get callback
    const std::string callback{MF_GetAmxString(amx, params[arg_callback], arg_callback - 1, nullptr)};

    // check callback
    const int forwardId = MF_RegisterSPForwardByName(amx, callback.c_str(), FP_CELL, FP_STRING, FP_DONE);
    if (forwardId == -1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Callback function \"%s\" is not exists", callback.c_str());
        return 0;
    }

    // set it
    client->setOnMessageForwardId(forwardId);
    return 1;
}

AMX_NATIVE_INFO g_natives[] =
{
    {"mqtt_create", mqtt_create},
    {"mqtt_destroy", mqtt_destroy},
    {"mqtt_connect", mqtt_connect},
    {"mqtt_is_connect", mqtt_is_connect},
    {"mqtt_subscribe", mqtt_subscribe},
    {"mqtt_publish", mqtt_publish},
    {"mqtt_set_connected_callback", mqtt_set_connected_callback},
    {"mqtt_set_message_callback", mqtt_set_message_callback},
    {nullptr, nullptr}
};
