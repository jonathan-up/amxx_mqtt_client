//
// Created by jonathan on 2025/9/4.
//

#include "AmxxMqttClient.h"

#include "sdk/amxxmodule.h"

AmxxMqttClient::AmxxMqttClient(const std::string &blocker, const std::string &client) : MqttClient(blocker, client) {
}

AmxxMqttClient::~AmxxMqttClient() {
    // Unregister forwards if exists
    if (this->getOnConnectedForwardId() != -1) {
        MF_UnregisterSPForward(this->getOnConnectedForwardId());
    }
    if (this->getOnMessageForwardId() != -1) {
        MF_UnregisterSPForward(this->getOnMessageForwardId());
    }
    if (this->getOnConnectionLostForwardId() != -1) {
        MF_UnregisterSPForward(this->getOnConnectionLostForwardId());
    }
    if (this->getDisconnectForwardId() != -1) {
        MF_UnregisterSPForward(this->getDisconnectForwardId());
    }
}

int AmxxMqttClient::getOnConnectedForwardId() const {
    return m_iOnConnectedForwardId;
}

void AmxxMqttClient::setOnConnectedForwardId(int m_i_on_connected_forward_id) {
    m_iOnConnectedForwardId = m_i_on_connected_forward_id;
}

int AmxxMqttClient::getOnMessageForwardId() const {
    return m_iOnMessageForwardId;
}

void AmxxMqttClient::setOnMessageForwardId(int m_i_on_message_forward_id) {
    m_iOnMessageForwardId = m_i_on_message_forward_id;
}

int AmxxMqttClient::getOnConnectionLostForwardId() const {
    return m_iOnConnectionLostForwardId;
}

void AmxxMqttClient::setOnConnectionLostForwardId(int m_i_on_connection_lost_forward_id) {
    m_iOnConnectionLostForwardId = m_i_on_connection_lost_forward_id;
}

int AmxxMqttClient::getDisconnectForwardId() const {
    return m_iOnDisconnectForwardId;
}

void AmxxMqttClient::setDisconnectForwardId(int m_i_disconnect_forward_id) {
    m_iOnDisconnectForwardId = m_i_disconnect_forward_id;
}
