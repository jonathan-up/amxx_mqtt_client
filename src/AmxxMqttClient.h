//
// Created by jonathan on 2025/9/4.
//

#ifndef AMXX_MQTT_CLIENT_H
#define AMXX_MQTT_CLIENT_H

#include "MqttClient.h"

class AmxxMqttClient : public MqttClient {
    int m_iOnConnectedForwardId = 0;
    int m_iOnMessageForwardId = 0;
    int m_iOnConnectionLostForwardId = 0;
    int m_iOnDisconnectForwardId = 0;

public:
    AmxxMqttClient(const std::string &blocker, const std::string &client) : MqttClient(blocker, client) {
    }

    [[nodiscard]] int getOnConnectedForwardId() const;

    void setOnConnectedForwardId(int m_i_on_connected_forward_id);

    [[nodiscard]] int getOnMessageForwardId() const;

    void setOnMessageForwardId(int m_i_on_message_forward_id);

    [[nodiscard]] int getOnConnectionLostForwardId() const;

    void setOnConnectionLostForwardId(int m_i_on_connection_lost_forward_id);

    [[nodiscard]] int getDisconnectForwardId() const;

    void setDisconnectForwardId(int m_i_disconnect_forward_id);
};

#endif //AMXX_MQTT_CLIENT_H
