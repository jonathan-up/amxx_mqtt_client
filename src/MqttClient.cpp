//
// Created by jonathan on 9/2/25.
//

#include "MqttClient.h"

MqttClient::MqttClient(const std::string &blocker, const std::string &client)
    : m_pClient(std::make_unique<mqtt::async_client>(blocker, client)) {
    // Install Paho's callbacks once, before connecting. Replacing these while
    // Paho is delivering an event races with its callback function objects.
    m_pClient->set_connected_handler([this](const mqtt::string &msg) {
        m_bIsConnected = true;
        if (const auto handler = copyHandler(m_connectedHandler)) {
            handler(this, msg);
        }
    });
    m_pClient->set_message_callback([this](const mqtt::const_message_ptr &msg) {
        if (const auto handler = copyHandler(m_messageHandler)) {
            handler(this, msg);
        }
    });
    m_pClient->set_connection_lost_handler([this](const mqtt::string &msg) {
        m_bIsConnected = false;
        if (const auto handler = copyHandler(m_connectionLostHandler)) {
            handler(this, msg);
        }
    });
    m_pClient->set_disconnected_handler([this](const mqtt::properties &props, mqtt::ReasonCode rc) {
        m_bIsConnected = false;
        if (const auto handler = copyHandler(m_disconnectedHandler)) {
            handler(this, props, rc);
        }
    });
}

MqttClient::~MqttClient() {
    {
        std::lock_guard lock(m_handlerMutex);
        m_stopping = true;
    }
    // MQTTAsync_destroy synchronizes with Paho callbacks and cancels pending
    // actions. Their listener and callback state must stay alive until it returns.
    // Never hold m_handlerMutex here: cancellation may invoke on_failure.
    m_pClient.reset();
}

void MqttClient::setConnectedHandler(const ConnectedHandler &handler) {
    std::lock_guard lock(m_handlerMutex);
    m_connectedHandler = handler;
}

void MqttClient::setMessageHandler(const MessageHandler &handler) {
    std::lock_guard lock(m_handlerMutex);
    m_messageHandler = handler;
}

void MqttClient::setConnectionLostHandler(const ConnectionLostHandler &handler) {
    std::lock_guard lock(m_handlerMutex);
    m_connectionLostHandler = handler;
}

void MqttClient::setDisconnectHandler(const DisconnectedHandler &handler) {
    std::lock_guard lock(m_handlerMutex);
    m_disconnectedHandler = handler;
}

void MqttClient::connect() {
    const auto opts = mqtt::connect_options_builder::v5()
            .clean_start(true)
            .keep_alive_interval(std::chrono::seconds(20))
            .finalize();

    this->connect(opts);
}

void MqttClient::connect(const mqtt::connect_options &options) {
    m_pClient->connect(options);
}

void MqttClient::disconnect() {
    try {
        // Paho owns the pending action and calls this listener on completion.
        // No detached waiter can outlive the client or the module.
        m_pClient->disconnect(nullptr, *this);
    } catch (const mqtt::exception &) {
        // Preserve the existing notification for synchronous disconnect errors.
        notifyDisconnect();
    }
}

void MqttClient::notifyDisconnect() const {
    if (const auto handler = copyHandler(m_disconnectedHandler)) {
        const mqtt::properties props;
        handler(nullptr, props, mqtt::NORMAL_DISCONNECTION);
    }
}

void MqttClient::on_success(const mqtt::token &) {
    notifyDisconnect();
}

void MqttClient::on_failure(const mqtt::token &) {
    notifyDisconnect();
}

void MqttClient::subscribe(const char *topicName, const int qos) const {
    this->m_pClient->subscribe(topicName, qos);
}

void MqttClient::unsubscribe(const std::string &topicName) const {
    this->m_pClient->unsubscribe(topicName);
}

void MqttClient::publish(const std::string &topicName, const std::string &payload, const int qos, const bool retained) const {
    const mqtt::message_ptr msg = mqtt::make_message(topicName, payload);
    msg->set_qos(qos);
    msg->set_retained(retained);
    this->m_pClient->publish(msg);
}

bool MqttClient::isConnected() const {
    return this->m_bIsConnected;
}
