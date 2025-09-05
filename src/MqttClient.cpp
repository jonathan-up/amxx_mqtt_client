//
// Created by jonathan on 9/2/25.
//

#include "MqttClient.h"

MqttClient::MqttClient(const std::string &blocker, const std::string &client) {
    this->m_pClient = new mqtt::async_client(blocker, client);
}

MqttClient::~MqttClient() {
    delete this->m_pClient;
}

void MqttClient::setConnectedHandler(const mqtt::async_client::connection_handler &cb) const {
    this->m_pClient->set_connected_handler(cb);
}

void MqttClient::setConnectioLostHandler(const mqtt::async_client::connection_handler &cb) const {
    this->m_pClient->set_connection_lost_handler(cb);
}

void MqttClient::setDisconnectHandler(const mqtt::async_client::disconnected_handler &cb) const {
    this->m_pClient->set_disconnected_handler(cb);
}

void MqttClient::setMessageHandler(const mqtt::async_client::message_handler &cb) const {
    this->m_pClient->set_message_callback(cb);
}

void MqttClient::connect() const {
    try {
        const auto opts = mqtt::connect_options_builder::v5()
                .properties({{mqtt::property::SESSION_EXPIRY_INTERVAL, 604800}})
                .clean_start(true)
                .keep_alive_interval(std::chrono::seconds(20))
                .finalize();

        this->m_pClient->connect(opts);
    } catch (const mqtt::exception &e) {
        std::cout << "ERR: " << e.what() << std::endl;
    }
}

void MqttClient::subscribe(const char *topicName, const int qos) const {
    this->m_pClient->subscribe(topicName, qos);
}

void MqttClient::publish(const std::string &topicName, const std::string &payload, const int qos) const {
    const mqtt::message_ptr msg = mqtt::make_message(topicName, payload);
    msg->set_qos(qos);
    this->m_pClient->publish(msg);
}

bool MqttClient::isConnected() const {
    return this->m_pClient->is_connected();
}
