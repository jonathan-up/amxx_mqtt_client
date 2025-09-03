//
// Created by jonathan on 9/2/25.
//

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <mqtt/async_client.h>
#include <string>

class MqttClient {
    mqtt::async_client *m_pClient = nullptr;

public:
    MqttClient(const std::string &blocker, const std::string &client);

    ~MqttClient();

    void setConnectedHandler(const mqtt::async_client::connection_handler &cb) const;

    void setConnectioLostHandler(const mqtt::async_client::connection_handler &cb) const;

    void setDisconnectHandler(const mqtt::async_client::disconnected_handler &cb) const;

    void setMessageHandler(const mqtt::async_client::message_handler &cb) const;

    void connect() const;

    void subscribe(const char *topicName, int qos = 0) const;

    void publish(const std::string &topicName, const std::string &payload, int qos = 0) const;

    [[nodiscard]] mqtt::async_client *getClient() const {
        return this->m_pClient;
    }
};

#endif
