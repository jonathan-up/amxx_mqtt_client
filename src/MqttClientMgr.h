//
// Created by jonathan on 9/2/25.
//

#ifndef MQTT_CLIENT_MGR_H
#define MQTT_CLIENT_MGR_H

#include <cstdint>
#include <memory>
#include <thread>
#include <unordered_map>

#include "AmxxMqttClient.h"

// Owned by the game thread. Network callbacks may only enqueue events; they
// must never look up, create, or destroy clients (including AMXX forwards).
class MqttClientMgr {
    std::unordered_map<int, std::unique_ptr<AmxxMqttClient>> m_clients;
    const std::thread::id m_gameThread = std::this_thread::get_id();
    // Never reset or wrap: a delayed event/stale Pawn handle must not identify
    // another client, even across map changes. Zero remains the invalid handle.
    std::uint64_t m_nextHandle = 1;
    static constexpr size_t maxClients = 10000;

    void assertGameThread() const;

public:
    int make(const std::string &broker, const std::string &clientId);

    // Borrowed only until destroy/reset on the game thread. Callers must not
    // retain or access this pointer after invoking Pawn (which can destroy it).
    AmxxMqttClient *getClient(int handle) const;
    void destroy(int handle);
    void reset();
};

extern MqttClientMgr g_mqttClientMgr;

#endif
