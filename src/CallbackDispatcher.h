//
// Created by jonathan on 2025/9/4.
//

#ifndef CALLBACK_DISPATCHER_H
#define CALLBACK_DISPATCHER_H

#include "AmxxMqttClient.h"

// Callback dispatchers
void ConnectedCallbackDispatcher(const AmxxMqttClient *client);

void MessageCallbackDispatcher(const AmxxMqttClient *client, const mqtt::const_message_ptr &msg);

void ConnectionLostCallbackDispatcher(const AmxxMqttClient *client, const std::string &reason);

void DisconnectedCallbackDispatcher(const AmxxMqttClient *client, mqtt::ReasonCode reasonCode);

#endif //CALLBACK_DISPATCHER_H
