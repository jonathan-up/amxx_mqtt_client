//
// Created by jonathan on 2025/9/4.
//

#ifndef CALLBACK_DISPATCHER_H
#define CALLBACK_DISPATCHER_H

#include "AmxxMqttClient.h"

// Callback dispatchers
void ConnectedCallbackDispatcher( int handle);

void MessageCallbackDispatcher( int handle, const mqtt::const_message_ptr &msg);

void ConnectionLostCallbackDispatcher(int handle, const std::string &reason);

void DisconnectedCallbackDispatcher(int handle, mqtt::ReasonCode reasonCode);

#endif //CALLBACK_DISPATCHER_H
