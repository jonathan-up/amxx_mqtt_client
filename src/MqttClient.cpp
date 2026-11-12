//
// Created by jonathan on 9/2/25.
//

#include "MqttClient.h"

MqttClient::MqttClient(const std::string &blocker, const std::string &client)
    : m_pClient(std::make_unique<mqtt::async_client>(blocker, client)) {
    // Install Paho's callbacks once, before connecting. Replacing these while
    // Paho is delivering an event races with its callback function objects.
    m_pClient->set_connected_handler([this](const mqtt::string &msg) {
        m_state = State::Connected;
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
        m_state = State::Disconnected;
        if (const auto handler = copyHandler(m_connectionLostHandler)) {
            handler(this, msg);
        }
    });
    m_pClient->set_disconnected_handler([this](const mqtt::properties &props, mqtt::ReasonCode rc) {
        m_state = State::Disconnected;
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

void MqttClient::setErrorHandler(const ErrorHandler &handler) {
    std::lock_guard lock(m_handlerMutex);
    m_errorHandler = handler;
}

void MqttClient::connect() {
    const auto opts = mqtt::connect_options_builder::v5()
            .clean_start(true)
            .keep_alive_interval(std::chrono::seconds(20))
            .finalize();

    this->connect(opts);
}

void MqttClient::connect(const mqtt::connect_options &options) {
    auto expected = State::Disconnected;
    if (!m_state.compare_exchange_strong(expected, State::Connecting)) {
        throw mqtt::exception(MQTTASYNC_OPERATION_INCOMPLETE,
                              "Client is connected or a connection operation is pending");
    }
    try {
        m_pClient->connect(options, nullptr, *this);
    } catch (...) {
        m_state = State::Disconnected;
        throw;
    }
}

void MqttClient::disconnect() {
    auto expected = State::Connected;
    if (!m_state.compare_exchange_strong(expected, State::Disconnecting)) {
        throw mqtt::exception(expected == State::Disconnected ? MQTTASYNC_DISCONNECTED
                                                             : MQTTASYNC_OPERATION_INCOMPLETE,
                              "Client is not connected or an operation is pending; destroy the client to cancel a pending connect");
    }
    try {
        // Paho owns the pending action and calls this listener on completion.
        // No detached waiter can outlive the client or the module.
        m_pClient->disconnect(nullptr, *this);
    } catch (...) {
        // A concurrent connection-lost callback may already have set Disconnected.
        expected = State::Disconnecting;
        m_state.compare_exchange_strong(expected, State::Connected);
        throw;
    }
}

void MqttClient::notifyDisconnect() const {
    if (const auto handler = copyHandler(m_disconnectedHandler)) {
        const mqtt::properties props;
        handler(this, props, mqtt::NORMAL_DISCONNECTION);
    }
}

void MqttClient::on_success(const mqtt::token &token) {
    // Paho also invokes this listener for CONNECT (sometimes twice). The
    // installed connected handler owns that state transition and notification.
    if (token.get_type() != mqtt::token::Type::DISCONNECT) {
        return;
    }
    // Paho has completed the local disconnect. Publish the state before the
    // notification, even if no handler is registered. A request/failure alone
    // does not prove the connection has closed.
    m_state = State::Disconnected;
    notifyDisconnect();
}

void MqttClient::on_failure(const mqtt::token &token) {
    const bool connecting = token.get_type() == mqtt::token::Type::CONNECT;
    if (connecting) {
        m_state = State::Disconnected;
    } else {
        auto expected = State::Disconnecting;
        m_state.compare_exchange_strong(expected, State::Connected);
    }
    if (const auto handler = copyHandler(m_errorHandler)) {
        const mqtt::exception error(token.get_return_code(), token.get_reason_code(), token.get_error_message());
        handler(connecting ? Operation::Connect : Operation::Disconnect,
                token.get_return_code(), token.get_reason_code(), error.what());
    }
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
    const auto state = m_state.load();
    return state == State::Connected || state == State::Disconnecting;
}
