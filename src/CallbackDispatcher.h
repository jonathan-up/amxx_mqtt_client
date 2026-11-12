//
// Created by jonathan on 2025/9/4.
//

#ifndef CALLBACK_DISPATCHER_H
#define CALLBACK_DISPATCHER_H

#include "AmxxMqttClient.h"
#include <cstdint>

inline constexpr size_t MaxQueuedCallbacks = 4096;
inline constexpr size_t MaxQueuedCallbackBytes = 8 * 1024 * 1024;
inline constexpr size_t MaxMessageCallbackBytes = 64 * 1024; // topic + payload

struct CallbackQueueStats {
    size_t count;
    size_t bytes;
    std::uint64_t dropped;
};
CallbackQueueStats GetCallbackQueueStats();

// Producer entry points: safe to call from Paho's worker threads. They only
// enqueue owned event data; AMXX and client handles are accessed on SV_Frame.
void ConnectedCallbackDispatcher(int handle);

void MessageCallbackDispatcher(int handle, const mqtt::const_message_ptr &msg);

void ConnectionLostCallbackDispatcher(int handle, const std::string &reason);

void DisconnectedCallbackDispatcher(int handle, mqtt::ReasonCode reasonCode);
void ErrorCallbackDispatcher(int handle, MqttClient::Operation operation, int code,
                             mqtt::ReasonCode reason, const std::string &message);

// Lifecycle and draining must be called on the game thread.
void StartCallbackDispatcher();
void StopCallbackDispatcher();
void ProcessPendingCallbacks();
void RegisterCallbackClient(int handle);
void RemoveClientCallbacks(int handle);

#endif //CALLBACK_DISPATCHER_H
