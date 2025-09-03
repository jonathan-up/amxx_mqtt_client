//
// Created by jonathan on 2/25/25.
//

#include "sdk/amxxmodule.h"
#include "natives.h"

#include "MqttClient.h"

#define BLOCKER "mqtt://202.189.6.222:1883"
#define CLIENTID "CS_SERVER"
#define TOPIC "cs/server"

MqttClient *mqttClient = nullptr;

void onMsg(mqtt::const_message_ptr msg_ptr)
{
    MF_Log("OO_MSG: %s", msg_ptr->get_payload_str().c_str());
}

void OnAmxxAttach()
{
    MF_AddNatives(g_natives);
    MF_Log("==================OnAmxxAttach==================");

    mqttClient = new MqttClient{BLOCKER, CLIENTID};
    mqttClient->setMessageHandler(onMsg);
    mqttClient->connect();

    if (mqttClient->getClient()->is_connected())
    {
        mqttClient->subscribe(TOPIC, 0);
        MF_Log("Connected to %s", BLOCKER);
        MF_Log("Subscribe topic `%s`", TOPIC);
    }
    MF_Log("==================OnAmxxAttach==================");
}

void OnAmxxDetach()
{
    MF_Log("==================OnAmxxDetach==================");
    if (mqttClient != nullptr)
    {
        delete mqttClient;
    }
    MF_Log("==================OnAmxxDetach==================");
}
