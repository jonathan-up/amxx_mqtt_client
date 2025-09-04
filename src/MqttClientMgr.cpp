//
// Created by jonathan on 9/2/25.
//

#include "MqttClientMgr.h"

MqttClientMgr::~MqttClientMgr() {
    for (auto &[k, v]: this->m_clients) {
        delete v;
    }
}

int MqttClientMgr::make(const std::string &blocker, const std::string &clientId) {
    if (const int index = this->makeIndex(); index != 0) {
        this->m_clients[index] = new AmxxMqttClient{blocker, clientId};
        return index;
    }
    return 0;
}

const AmxxMqttClient *MqttClientMgr::getClient(const int index) {
    return this->m_clients[index];
}

void MqttClientMgr::destroy(const int index) {
    if (const auto it = this->m_clients.find(index); it != m_clients.end()) {
        delete it->second;
        this->m_indexUsed.erase(it->first);
        this->m_clients.erase(it);
    }
}

int MqttClientMgr::makeIndex() {
    std::lock_guard lock(this->m_mutex);
    for (int i = 1; i <= MAX_INDEX; i++) {
        if (this->m_indexUsed.find(i) == this->m_indexUsed.end()) {
            this->m_indexUsed.insert(i);
            return i;
        }
    }

    return 0;
}

MqttClientMgr g_mqttClientMgr;
