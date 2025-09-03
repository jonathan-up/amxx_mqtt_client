#include "MqttClient.h"

MqttClient::MqttClient(const std::string &blocker, const std::string &client)
{
    this->m_pClient = new mqtt::async_client(blocker, client);
}
MqttClient::~MqttClient()
{
    if (this->m_pClient != nullptr)
    {
        delete this->m_pClient;
    }
}

void MqttClient::setConnectedHandler(mqtt::async_client::connection_handler cb)
{
    this->m_pClient->set_connected_handler(cb);
}
void MqttClient::setConnectioLostHandler(mqtt::async_client::connection_handler cb)
{
    this->m_pClient->set_connection_lost_handler(cb);
}
void MqttClient::setDisconnectHandler(mqtt::async_client::disconnected_handler cb)
{
    this->m_pClient->set_disconnected_handler(cb);
}
void MqttClient::setMessageHandler(mqtt::async_client::message_handler cb)
{
    this->m_pClient->set_message_callback(cb);
}

void MqttClient::connect()
{
    try
    {
        auto opts = mqtt::connect_options_builder::v5()
                        .properties({{mqtt::property::SESSION_EXPIRY_INTERVAL, 604800}})
                        .clean_start(true)
                        .keep_alive_interval(std::chrono::seconds(20))
                        .finalize();
        this->m_pToken = this->m_pClient->connect(opts);
        this->m_pToken->wait();
    }
    catch (const mqtt::exception &e)
    {
        std::cout << "ERR: " << e.what() << std::endl;
    }
}

void MqttClient::subscribe(const char *topicName, int qos = 0) const
{
    // mqtt::topic topic{*this->m_pClient, topicName, qos};
    // auto token = topic.subscribe(mqtt::subscribe_options(true));
    // token->wait();
    if (this->m_pClient->is_connected())
    {
        this->m_pClient
            ->subscribe(topicName, qos)
            ->wait();
    }
}

void MqttClient::publish(const std::string &topicName, const std::string &payload, int qos)
{
    // mqtt::topic topic{*this->m_pClient, topicName, qos};
    // topic.publish()
    if (this->m_pClient->is_connected())
    {
        this->m_pClient->publish(mqtt::make_message(topicName, payload));
    }
}