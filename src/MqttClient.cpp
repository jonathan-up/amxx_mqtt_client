//
// Created by jonathan on 9/2/25.
//

#include "MqttClient.h"

#include <thread>

MqttClient::MqttClient(const std::string &blocker, const std::string &client) {
    this->m_pClient = new mqtt::async_client(blocker, client);
    this->m_bIsConnected = false;

    this->m_connectedHandler = nullptr;
    this->m_messageHandler = nullptr;
    this->m_connectionLostHandler = nullptr;
    this->m_disconnectedHandler = nullptr;
}

MqttClient::~MqttClient() {
    delete this->m_pClient;
}

void MqttClient::setConnectedHandler(const ConnectedHandler &handler) {
    this->m_connectedHandler = handler;
}

void MqttClient::setMessageHandler(const MessageHandler &handler) {
    this->m_messageHandler = handler;
}

void MqttClient::setConnectionLostHandler(const ConnectionLostHandler &handler) {
    this->m_connectionLostHandler = handler;
}

void MqttClient::setDisconnectHandler(const DisconnectedHandler &handler) {
    this->m_disconnectedHandler = handler;
}

void MqttClient::connect() {
    const auto opts = mqtt::connect_options_builder::v5()
            .clean_start(true)
            .keep_alive_interval(std::chrono::seconds(20))
            .finalize();

    this->connect(opts);
}

void MqttClient::connect(const mqtt::connect_options &options) {
    // On connected
    this->m_pClient->set_connected_handler([this](const mqtt::string &msg) {
        this->m_bIsConnected = true;
        if (this->m_connectedHandler != nullptr) {
            this->m_connectedHandler(this, msg);
        }
    });

    // On message
    this->m_pClient->set_message_callback([this](const mqtt::const_message_ptr &msg) {
        if (this->m_messageHandler != nullptr) {
            this->m_messageHandler(this, msg);
        }
    });

    // On connection lost
    this->m_pClient->set_connection_lost_handler([this](const mqtt::string &msg) {
        this->m_bIsConnected = false;
        if (this->m_connectionLostHandler != nullptr) {
            this->m_connectionLostHandler(this, msg);
        }
    });

    // On connection lost
    this->m_pClient->set_disconnected_handler([this](const mqtt::properties &props, mqtt::ReasonCode rc) {
        this->m_bIsConnected = false;
        if (this->m_disconnectedHandler != nullptr) {
            this->m_disconnectedHandler(this, props, rc);
        }
    });

    this->m_pClient->connect(options);
}

void MqttClient::disconnect() const {
    mqtt::token_ptr token;
    try {
        token = this->m_pClient->disconnect();
    } catch (const mqtt::exception &) {
        // Already disconnected / disconnect failed synchronously.
        token = nullptr;
    }

    // Copy the handler by value so the detached thread never dereferences
    // `this`: on map change the client can be deleted (OnAmxxDetach -> reset)
    // while this thread is still waiting on the token. The handler captures
    // its handle and routes through the manager-checked dispatcher, so it is
    // safe on its own.
    const DisconnectedHandler handler = this->m_disconnectedHandler;

    std::thread asyncCall{
        [token, handler] {
            // token->wait() throws mqtt::exception (e.g. -11
            // OPERATION_INCOMPLETE) when the disconnect cannot complete.
            // This runs in a detached thread, so an uncaught exception would
            // call std::terminate and abort the whole server.
            try {
                if (token) {
                    token->wait();
                }
            } catch (const mqtt::exception &) {
                // Ignore: notify the handler regardless below.
            }

            if (handler != nullptr) {
                const mqtt::properties p;
                handler(nullptr, p, mqtt::NORMAL_DISCONNECTION);
            }
        }
    };
    asyncCall.detach();
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
