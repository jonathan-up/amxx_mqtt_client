//
// Created by jonathan on 2025/9/4.
//

#ifndef AMXX_MQTT_CLIENT_H
#define AMXX_MQTT_CLIENT_H

#include "MqttClient.h"

class AmxxMqttClient : public MqttClient {
    int m_iOnConnectedForwardId = -1;
    int m_iOnMessageForwardId = -1;
    int m_iOnConnectionLostForwardId = -1;
    int m_iOnDisconnectForwardId = -1;
    int m_iOnErrorForwardId = -1;

public:
    AmxxMqttClient(const std::string &blocker, const std::string &client);

    ~AmxxMqttClient() override;

    [[nodiscard]] int getOnConnectedForwardId() const;

    void setOnConnectedForwardId(int forwardId);

    [[nodiscard]] int getOnMessageForwardId() const;

    void setOnMessageForwardId(int forwardId);

    [[nodiscard]] int getOnConnectionLostForwardId() const;

    void setOnConnectionLostForwardId(int forwardId);

    [[nodiscard]] int getDisconnectForwardId() const;

    void setDisconnectForwardId(int forwardId);
    [[nodiscard]] int getErrorForwardId() const;
    void setErrorForwardId(int forwardId);
};

#endif //AMXX_MQTT_CLIENT_H
