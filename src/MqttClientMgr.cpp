//
// Created by jonathan on 9/2/25.
//

#include "MqttClientMgr.h"
#include "CallbackDispatcher.h"

#include <cassert>
#include <limits>

void MqttClientMgr::assertGameThread() const {
    assert(std::this_thread::get_id() == m_gameThread);
}

int MqttClientMgr::make(const std::string &broker, const std::string &clientId) {
    assertGameThread();
    if (m_clients.size() >= maxClients) {
        throw mqtt::exception{0, "too many mqtt clients"};
    }
    if (m_nextHandle > static_cast<std::uint64_t>(std::numeric_limits<std::int32_t>::max())) {
        throw mqtt::exception{0, "mqtt client handles exhausted"};
    }

    const int handle = static_cast<int>(m_nextHandle);
    auto client = std::make_unique<AmxxMqttClient>(broker, clientId);
    // Bind handlers once, before any connection can start. Workers capture only
    // the permanent handle, never a manager entry or AMXX forward.
    client->setConnectedHandler([handle](const MqttClient *, const std::string &) {
        ConnectedCallbackDispatcher(handle);
    });
    client->setMessageHandler([handle](const MqttClient *, const mqtt::const_message_ptr &msg) {
        MessageCallbackDispatcher(handle, msg);
    });
    client->setConnectionLostHandler([handle](const MqttClient *, const std::string &reason) {
        ConnectionLostCallbackDispatcher(handle, reason);
    });
    client->setDisconnectHandler([handle](const MqttClient *, const mqtt::properties &, mqtt::ReasonCode code) {
        DisconnectedCallbackDispatcher(handle, code);
    });

    // Commit only after construction succeeds; failed creation consumes no slot.
    m_clients.emplace(handle, std::move(client));
    ++m_nextHandle;
    return handle;
}

AmxxMqttClient *MqttClientMgr::getClient(const int handle) const {
    assertGameThread();
    const auto it = m_clients.find(handle);
    return it == m_clients.end() ? nullptr : it->second.get();
}

void MqttClientMgr::destroy(const int handle) {
    assertGameThread();
    // Invalidate the handle before Paho teardown can enqueue a final event.
    const auto removed = m_clients.extract(handle);
}

void MqttClientMgr::reset() {
    assertGameThread();
    decltype(m_clients) removed;
    removed.swap(m_clients);
    // Destruction happens on this thread with all handles already invalidated.
}

MqttClientMgr g_mqttClientMgr;
