//
// Created by jonathan on 8/29/25.
//

#include <string>
#include "natives.h"

#include "MqttClientMgr.h"
#include "ConnectOptionMgr.h"
#include "CallbackDispatcher.h"

#define ARG_COUNT (params[0] / sizeof(cell))

cell AMX_NATIVE_CALL mqtt_create(AMX *amx, cell *params) {
    enum { arg_count, arg_blocker, arg_client_id };

    // check args
    if (ARG_COUNT < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
        return 0;
    }

    // get blocker & client id
    const std::string blocker{MF_GetAmxString(amx, params[arg_blocker], arg_blocker - 1, nullptr)};
    const std::string clientId{MF_GetAmxString(amx, params[arg_client_id], arg_client_id - 1, nullptr)};

    try {
        // make it
        return g_mqttClientMgr.make(blocker, clientId);
    } catch (const mqtt::exception &err) {
        MF_LogError(amx, AMX_ERR_NATIVE, err.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL mqtt_destroy(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    // check args
    if (ARG_COUNT < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
        return 0;
    }

    // destroy it
    g_mqttClientMgr.destroy(params[arg_handle]);
    return 1;
}

cell AMX_NATIVE_CALL mqtt_connect(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_options };

    // check args
    if (ARG_COUNT < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
        return 0;
    }

    const int handle = params[arg_handle];
    const int optionsHandle = params[arg_options];

    // check handle
    AmxxMqttClient *client = g_mqttClientMgr.getClient(handle);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", handle);
        return 0;
    }

    ConnectOption *options = g_connectOptionMgr.getOptions(optionsHandle);

    // set handlers for client
    client->setConnectedHandler([handle](const MqttClient *, const std::string &) {
        ConnectedCallbackDispatcher(handle);
    });
    client->setMessageHandler([handle](const MqttClient *, const mqtt::const_message_ptr &msg) {
        MessageCallbackDispatcher(handle, msg);
    });
    client->setConnectionLostHandler([handle](const MqttClient *, const mqtt::string &msg) {
        ConnectionLostCallbackDispatcher(handle, msg);
    });
    client->setDisconnectHandler([handle](const MqttClient *, const mqtt::properties &, const mqtt::ReasonCode code) {
        DisconnectedCallbackDispatcher(handle, code);
    });

    try {
        // connect
        client->connect(*options->getOption());
        g_connectOptionMgr.destroy(optionsHandle);
        return 1;
    } catch (const mqtt::exception &err) {
        g_connectOptionMgr.destroy(optionsHandle);
        MF_LogError(amx, AMX_ERR_NATIVE, err.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL mqtt_disconnect(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    // check args
    if (ARG_COUNT < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
        return 0;
    }

    const int handle = params[arg_handle];

    // check handle
    const AmxxMqttClient *client = g_mqttClientMgr.getClient(handle);
    if (client == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", handle);
        return 0;
    }

    try {
        // connect
        client->disconnect();
        return 1;
    } catch (const mqtt::exception &err) {
        MF_LogError(amx, AMX_ERR_NATIVE, err.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL mqtt_is_connect(AMX *amx, cell *params) {
    enum { arg_count, arg_handle };

    // check args
    if (ARG_COUNT < 1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
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
    if (ARG_COUNT < 2) {
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

    try {
        // subscribe
        client->subscribe(topicName.c_str());
        return 1;
    } catch (const mqtt::exception &err) {
        MF_LogError(amx, AMX_ERR_NATIVE, err.what());
        return 0;
    }
}

cell AMX_NATIVE_CALL mqtt_unsubscribe(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_topic };

    // check args
    if (ARG_COUNT < 2) {
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

    try {
        // unsubscribe
        client->unsubscribe(topicName);
        return 1;
    } catch (const mqtt::exception &err) {
        MF_LogError(amx, AMX_ERR_NATIVE, err.what());
        return 0;
    }
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

    try {
        // publish
        client->publish(topicName, data);
        return 1;
    } catch (const mqtt::exception &err) {
        MF_LogError(amx, AMX_ERR_NATIVE, err.what());
        return 0;
    }

    return 1;
}

cell AMX_NATIVE_CALL mqtt_set_connected_callback(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_callback };

    // check args
    if (ARG_COUNT < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
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
    if (ARG_COUNT < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
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

cell AMX_NATIVE_CALL mqtt_set_connection_lost_callback(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_callback };

    // check args
    if (ARG_COUNT < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
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
    client->setOnConnectionLostForwardId(forwardId);
    return 1;
}

cell AMX_NATIVE_CALL mqtt_set_disconnected_callback(AMX *amx, cell *params) {
    enum { arg_count, arg_handle, arg_callback };

    // check args
    if (ARG_COUNT < 2) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
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
    const int forwardId = MF_RegisterSPForwardByName(amx, callback.c_str(), FP_CELL, FP_CELL, FP_DONE);
    if (forwardId == -1) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Callback function \"%s\" is not exists", callback.c_str());
        return 0;
    }

    // set it
    client->setDisconnectForwardId(forwardId);
    return 1;
}

cell AMX_NATIVE_CALL mqtt_create_connect_options(AMX *amx, cell *) {
    // no args
    try {
        return g_connectOptionMgr.make();
    } catch (const mqtt::exception &err) {
        MF_LogError(amx, AMX_ERR_NATIVE, err.what());
        return 0;
    }
}

enum MqttConnectOptionNames {
    MQTT_OPTIONS_USERNAME,
    MQTT_OPTIONS_PASSWORD,
    MQTT_OPTIONS_SESSION_EXPIRY,
};

cell AMX_NATIVE_CALL mqtt_set_options(AMX *amx, cell *params) {
    enum { arg_count, arg_options, arg_name, arg_value };

    // check args
    if (ARG_COUNT < 3) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Too few arguments to %s, the inc file incorrect?", __FUNCTION__);
        return 0;
    }

    // check options
    const ConnectOption *options = g_connectOptionMgr.getOptions(params[arg_options]);
    if (options == nullptr) {
        MF_LogError(amx, AMX_ERR_NATIVE, "Invalid mqtt client handle: %d", params[arg_options]);
        return 0;
    }

    switch (params[arg_name]) {
        case MQTT_OPTIONS_USERNAME: {
            const std::string value{MF_GetAmxString(amx, params[arg_value], arg_value - 1, nullptr)};
            options->getOption()->set_user_name(value);
            return 1;
        }
        case MQTT_OPTIONS_PASSWORD: {
            const std::string value{MF_GetAmxString(amx, params[arg_value], arg_value - 1, nullptr)};
            options->getOption()->set_password(value);
            return 1;
        }
        case MQTT_OPTIONS_SESSION_EXPIRY: {
            const cell *value = MF_GetAmxAddr(amx, params[arg_value]);
            options->getProperties()->clear();
            options->getProperties()->add({mqtt::property::SESSION_EXPIRY_INTERVAL, *value});
            options->getOption()->set_properties(*options->getProperties());
            return 1;
        }
        default:
            MF_LogError(amx, AMX_ERR_NATIVE, "Invalid option name");
            return 0;
    }
}

AMX_NATIVE_INFO g_natives[] =
{
    {"mqtt_create", mqtt_create},
    {"mqtt_destroy", mqtt_destroy},
    {"mqtt_connect", mqtt_connect},
    {"mqtt_disconnect", mqtt_disconnect},
    {"mqtt_is_connect", mqtt_is_connect},
    {"mqtt_subscribe", mqtt_subscribe},
    {"mqtt_unsubscribe", mqtt_unsubscribe},
    {"mqtt_publish", mqtt_publish},
    {"mqtt_set_connected_callback", mqtt_set_connected_callback},
    {"mqtt_set_message_callback", mqtt_set_message_callback},
    {"mqtt_set_connection_lost_callback", mqtt_set_connection_lost_callback},
    {"mqtt_set_disconnected_callback", mqtt_set_disconnected_callback},

    {"mqtt_create_connect_options", mqtt_create_connect_options},
    {"mqtt_set_options", mqtt_set_options},
    {nullptr, nullptr}
};
