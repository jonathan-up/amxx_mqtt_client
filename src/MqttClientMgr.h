//
// Created by jonathan on 9/2/25.
//

#ifndef MQTT_CLIENT_MGR_H
#define MQTT_CLIENT_MGR_H

#include <unordered_map>
#include <mutex>
#include <set>

#include "AmxxMqttClient.h"

#define MAX_INDEX 10000

class MqttClientMgr {
    std::unordered_map<int, AmxxMqttClient *> m_clients;
    std::set<int> m_indexUsed;
    std::mutex m_mutex;

public:
    ~MqttClientMgr();

    /**
     * Make a mqtt client
     *
     * @param blocker block server uri
     * @param clientId client id
     *
     * @return index for mqtt client
     */
    int make(const std::string &blocker, const std::string &clientId);

    /**
     * Get a client by index
     *
     * @param index client index
     *
     * @return MqttClient*
     */
    AmxxMqttClient *getClient(int index);

    /**
     * Destroy a client by index
     *
     * @param index client index
     */
    void destroy(int index);

    /**
     * Destroy all client
     * Do not call this function in mqtt handler
     * it will be deadlock
     */
    void reset();

private:
    /**
     * make an index for mqtt client on making mqtt client
     * @return
     */
    int makeIndex();
};

extern MqttClientMgr g_mqttClientMgr;

#endif
