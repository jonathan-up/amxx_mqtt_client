//
// Created by jonathan on 9/2/25.
//

#ifndef MQTT_CLIENT_H
#define MQTT_CLIENT_H

#include <mqtt/async_client.h>
#include <functional>
#include <string>

class MqttClient {
public:
    using ConnectedHandler = std::function<void(const MqttClient *mqtt_client, const std::string &msg)>;
    using MessageHandler = std::function<void (const MqttClient *mqtt_client, mqtt::const_message_ptr)>;
    using ConnectionLostHandler = std::function<void(const MqttClient *mqtt_client, const std::string &msg)>;
    using DisconnectedHandler = std::function<void(const MqttClient *mqtt_client, const mqtt::properties &props,
                                                   mqtt::ReasonCode)>;

private:
    mqtt::async_client *m_pClient;
    bool m_bIsConnected;

    // handlers
    ConnectedHandler m_connectedHandler;
    MessageHandler m_messageHandler;
    ConnectionLostHandler m_connectionLostHandler;
    DisconnectedHandler m_disconnectedHandler;

public:
    MqttClient(const std::string &blocker, const std::string &client);

    virtual ~MqttClient();

    void setConnectedHandler(const ConnectedHandler &handler);

    void setMessageHandler(const MessageHandler &handler);

    void setConnectionLostHandler(const ConnectionLostHandler &handler);

    void setDisconnectHandler(const DisconnectedHandler &handler);

    void connect();
    void connect(const mqtt::connect_options& options);
    void disconnect() const;

    void subscribe(const char *topicName, int qos = 0) const;

    void unsubscribe(const std::string &topicName) const;

    void publish(const std::string &topicName, const std::string &payload, int qos = 0) const;

    [[nodiscard]] bool isConnected() const;

    [[nodiscard]] mqtt::async_client *getClient() const {
        return this->m_pClient;
    }
};

#endif
