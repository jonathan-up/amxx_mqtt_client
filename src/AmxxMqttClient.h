//
// Created by jonathan on 2025/9/4.
//

#ifndef AMXX_MQTT_CLIENT_H
#define AMXX_MQTT_CLIENT_H

#include "MqttClient.h"

class AmxxMqttClient : public MqttClient {
public:
    int m_iOnMessageForwardId = 0;
    int m_iOnConnectedForwardId = 0;
    int m_iOnConnectionLostForwardId = 0;
    int m_iDisconnectForwardId = 0;

    AmxxMqttClient(const std::string &blocker, const std::string &client) :MqttClient(blocker, client) {};
};

#endif //AMXX_MQTT_CLIENT_H
