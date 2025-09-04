//
// Created by jonathan on 2025/9/4.
//

#include "CallbackDispatcher.h"

#include <mutex>
#include "sdk/amxxmodule.h"
#include "MqttClientMgr.h"

static std::mutex mutex_connected;
static std::mutex mutex_message;
static std::mutex mutex_connection_lost;
static std::mutex mutex_disconnected;

void ConnectedCallbackDispatcher(const int handle) {
    std::lock_guard lock(mutex_connected);
    // Call forwards
}

void MessageCallbackDispatcher(const int handle, const mqtt::const_message_ptr &msg) {
    std::lock_guard lock(mutex_message);
    // Call forwards if exists
    const AmxxMqttClient *client = g_mqttClientMgr.getClient(handle);
    if (const int forwardId = client->getOnMessageForwardId(); forwardId != -1) {
        const std::string payload = msg->get_payload_str();
        MF_ExecuteForward(forwardId, handle, payload.c_str());
    }
}

void ConnectionLostCallbackDispatcher(const int handle, const std::string &reason) {
    std::lock_guard lock(mutex_connection_lost);
    // Call forwards
}

void DisconnectedCallbackDispatcher(const int handle, mqtt::ReasonCode reasonCode) {
    std::lock_guard lock(mutex_disconnected);
    // Call forwards
}
