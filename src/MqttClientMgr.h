//
// Created by jonathan on 9/2/25.
//

#ifndef MQTT_CLIENT_MGR_H
#define MQTT_CLIENT_MGR_H

#include <unordered_map>

#include "MqttClient.h"

class MqttClientMgr {
    std::unordered_map<int, MqttClient*> clients;
public:
    MqttClientMgr() = delete;

    ~MqttClientMgr();
};

#endif
