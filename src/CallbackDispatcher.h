//
// Created by jonathan on 2025/9/4.
//

#ifndef CALLBACK_DISPATCHER_H
#define CALLBACK_DISPATCHER_H

#include "AmxxMqttClient.h"

// Producer entry points: safe to call from Paho's worker threads. They only
// enqueue owned event data; AMXX and client handles are accessed on SV_Frame.
void ConnectedCallbackDispatcher(int handle);

void MessageCallbackDispatcher(int handle, const mqtt::const_message_ptr &msg);

void ConnectionLostCallbackDispatcher(int handle, const std::string &reason);

void DisconnectedCallbackDispatcher(int handle, mqtt::ReasonCode reasonCode);

// Lifecycle and draining must be called on the game thread.
void StartCallbackDispatcher();
void StopCallbackDispatcher();
void ProcessPendingCallbacks();

#endif //CALLBACK_DISPATCHER_H
