//
// Created by jonathan on 2025/9/4.
//

#include "CallbackDispatcher.h"

#include <mutex>

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
    // Call forwards
}

void ConnectionLostCallbackDispatcher(const int handle, const std::string &reason) {
    std::lock_guard lock(mutex_connection_lost);
    // Call forwards
}

void DisconnectedCallbackDispatcher(const int handle, mqtt::ReasonCode reasonCode) {
    std::lock_guard lock(mutex_disconnected);
    // Call forwards
}
