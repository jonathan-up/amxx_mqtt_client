#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <mqtt/async_client.h>
#include <string>

class MqttClient
{
    mqtt::async_client *m_pClient = nullptr;
    mqtt::token_ptr m_pToken = nullptr;

public:
    MqttClient(const std::string &blocker, const std::string &client);
    ~MqttClient();

    void setConnectedHandler(mqtt::async_client::connection_handler cb);
    void setConnectioLostHandler(mqtt::async_client::connection_handler cb);
    void setDisconnectHandler(mqtt::async_client::disconnected_handler cb);
    void setMessageHandler(mqtt::async_client::message_handler cb);

    void connect();

    void subscribe(const char* topicName, int qos) const;
    void publish(const std::string& topicName, const std::string& payload, int qos = 0);

    mqtt::async_client* getClient() const {

        return this->m_pClient;
    }
};

#endif
