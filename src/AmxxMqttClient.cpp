//
// Created by jonathan on 2025/9/4.
//

#include "AmxxMqttClient.h"

#include "sdk/amxxmodule.h"

namespace {
void ReplaceForward(int &currentId, int forwardId) {
    if (currentId != -1 && currentId != forwardId) {
        MF_UnregisterSPForward(currentId);
    }
    currentId = forwardId;
}
} // namespace

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
    if (m_iOnErrorForwardId != -1) {
        MF_UnregisterSPForward(m_iOnErrorForwardId);
    }
}

int AmxxMqttClient::getOnConnectedForwardId() const {
    return m_iOnConnectedForwardId;
}

void AmxxMqttClient::setOnConnectedForwardId(int forwardId) {
    ReplaceForward(m_iOnConnectedForwardId, forwardId);
}

int AmxxMqttClient::getOnMessageForwardId() const {
    return m_iOnMessageForwardId;
}

void AmxxMqttClient::setOnMessageForwardId(int forwardId) {
    ReplaceForward(m_iOnMessageForwardId, forwardId);
}

int AmxxMqttClient::getOnConnectionLostForwardId() const {
    return m_iOnConnectionLostForwardId;
}

void AmxxMqttClient::setOnConnectionLostForwardId(int forwardId) {
    ReplaceForward(m_iOnConnectionLostForwardId, forwardId);
}

int AmxxMqttClient::getDisconnectForwardId() const {
    return m_iOnDisconnectForwardId;
}

void AmxxMqttClient::setDisconnectForwardId(int forwardId) {
    ReplaceForward(m_iOnDisconnectForwardId, forwardId);
}

int AmxxMqttClient::getErrorForwardId() const {
    return m_iOnErrorForwardId;
}

void AmxxMqttClient::setErrorForwardId(int forwardId) {
    ReplaceForward(m_iOnErrorForwardId, forwardId);
}
