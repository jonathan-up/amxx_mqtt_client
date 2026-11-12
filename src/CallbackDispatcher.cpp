//
// Created by jonathan on 2025/9/4.
//

#include "CallbackDispatcher.h"

#include <algorithm>
#include <chrono>
#include <deque>
#include <mutex>
#include <utility>
#include <unordered_set>
#include "sdk/amxxmodule.h"
#include "MqttClientMgr.h"

namespace {
enum class CallbackType { Connected, Message, ConnectionLost, Disconnected, Error };

struct CallbackEvent {
    CallbackType type;
    int handle;
    std::string text;
    mqtt::ReasonCode reasonCode = mqtt::NORMAL_DISCONNECTION;
    std::string topic;
    MqttClient::Operation operation = MqttClient::Operation::Connect;
    int errorCode = 0;

    size_t bytes() const { return text.size() + topic.size(); }
};

std::mutex callbackMutex;
std::deque<CallbackEvent> pendingCallbacks;
std::unordered_set<int> activeClients;
size_t pendingBytes = 0;
std::uint64_t droppedCallbacks = 0;
std::uint64_t reportedDrops = 0;
std::chrono::steady_clock::time_point lastDropReport{};
bool acceptingCallbacks = false;
constexpr size_t maxCallbacksPerFrame = 256;

void EnqueueCallback(CallbackEvent event) {
    std::lock_guard lock(callbackMutex);
    if (!acceptingCallbacks || activeClients.count(event.handle) == 0) {
        return;
    }
    while (pendingCallbacks.size() >= MaxQueuedCallbacks ||
           event.bytes() > MaxQueuedCallbackBytes - pendingBytes) {
        if (event.type == CallbackType::Message) {
            ++droppedCallbacks; // Preserve already queued messages and their order.
            return;
        }
        // Connection/error notifications take priority over queued messages.
        // If only control events remain, replace the oldest one to stay bounded.
        auto victim = std::find_if(pendingCallbacks.begin(), pendingCallbacks.end(),
                                  [](const auto &item) { return item.type == CallbackType::Message; });
        if (victim == pendingCallbacks.end()) {
            victim = pendingCallbacks.begin();
        }
        pendingBytes -= victim->bytes();
        pendingCallbacks.erase(victim);
        ++droppedCallbacks;
    }
    const size_t bytes = event.bytes();
    pendingCallbacks.push_back(std::move(event));
    pendingBytes += bytes;
}

void ExecuteCallback(const CallbackEvent &event) {
    // Handles are never reused. Resolve only on the game thread immediately
    // before use, and never touch client after Pawn may have destroyed it.
    const AmxxMqttClient *client = g_mqttClientMgr.getClient(event.handle);
    if (client == nullptr) {
        return;
    }

    switch (event.type) {
        case CallbackType::Connected:
            if (const int id = client->getOnConnectedForwardId(); id != -1) {
                MF_ExecuteForward(id, event.handle);
            }
            break;
        case CallbackType::Message:
            if (const int id = client->getOnMessageForwardId(); id != -1) {
                MF_ExecuteForward(id, event.handle, event.topic.c_str(), event.text.c_str());
            }
            break;
        case CallbackType::ConnectionLost:
            if (const int id = client->getOnConnectionLostForwardId(); id != -1) {
                MF_ExecuteForward(id, event.handle, event.text.c_str());
            }
            break;
        case CallbackType::Disconnected:
            if (const int id = client->getDisconnectForwardId(); id != -1) {
                MF_ExecuteForward(id, event.handle, event.reasonCode);
            }
            break;
        case CallbackType::Error:
            MF_Log("MQTT client %d %s failed (code %d, reason %d): %s", event.handle,
                   event.operation == MqttClient::Operation::Connect ? "connect" : "disconnect",
                   event.errorCode, static_cast<int>(event.reasonCode), event.text.c_str());
            if (const int id = client->getErrorForwardId(); id != -1) {
                MF_ExecuteForward(id, event.handle, static_cast<int>(event.operation),
                                  event.errorCode, static_cast<int>(event.reasonCode), event.text.c_str());
            }
            break;
    }
}
} // namespace

void ConnectedCallbackDispatcher(const int handle) {
    EnqueueCallback({CallbackType::Connected, handle, {}});
}

void MessageCallbackDispatcher(const int handle, const mqtt::const_message_ptr &msg) {
    // Check lengths before copying network-owned data into the queue.
    if (msg->get_payload_str().size() > MaxMessageCallbackBytes ||
        msg->get_topic().size() > MaxMessageCallbackBytes - msg->get_payload_str().size()) {
        std::lock_guard lock(callbackMutex);
        if (acceptingCallbacks && activeClients.count(handle)) {
            ++droppedCallbacks;
        }
        return;
    }
    CallbackEvent event{CallbackType::Message, handle, msg->get_payload_str()};
    event.topic = msg->get_topic();
    EnqueueCallback(std::move(event));
}

void ConnectionLostCallbackDispatcher(const int handle, const std::string &reason) {
    EnqueueCallback({CallbackType::ConnectionLost, handle, reason.substr(0, 1024)});
}

void DisconnectedCallbackDispatcher(const int handle, const mqtt::ReasonCode reasonCode) {
    EnqueueCallback({CallbackType::Disconnected, handle, {}, reasonCode});
}

void ErrorCallbackDispatcher(int handle, MqttClient::Operation operation, int code,
                             mqtt::ReasonCode reason, const std::string &message) {
    CallbackEvent event{CallbackType::Error, handle, message.substr(0, 1024), reason};
    event.operation = operation;
    event.errorCode = code;
    EnqueueCallback(std::move(event));
}

void RegisterCallbackClient(int handle) {
    std::lock_guard lock(callbackMutex);
    activeClients.insert(handle);
}

void RemoveClientCallbacks(int handle) {
    std::lock_guard lock(callbackMutex);
    activeClients.erase(handle); // Late producers cannot refill the queue.
    for (auto it = pendingCallbacks.begin(); it != pendingCallbacks.end();) {
        if (it->handle == handle) {
            pendingBytes -= it->bytes();
            it = pendingCallbacks.erase(it);
        } else {
            ++it;
        }
    }
}

CallbackQueueStats GetCallbackQueueStats() {
    std::lock_guard lock(callbackMutex);
    return {pendingCallbacks.size(), pendingBytes, droppedCallbacks};
}

void StartCallbackDispatcher() {
    std::lock_guard lock(callbackMutex);
    acceptingCallbacks = true;
}

void StopCallbackDispatcher() {
    std::lock_guard lock(callbackMutex);
    acceptingCallbacks = false;
    pendingCallbacks.clear();
    pendingBytes = 0;
}

void ProcessPendingCallbacks() {
    size_t remaining;
    std::uint64_t dropsToReport = 0;
    {
        std::lock_guard lock(callbackMutex);
        remaining = std::min(pendingCallbacks.size(), maxCallbacksPerFrame);
        const auto now = std::chrono::steady_clock::now();
        if (droppedCallbacks != reportedDrops && now - lastDropReport >= std::chrono::seconds(5)) {
            dropsToReport = droppedCallbacks - reportedDrops;
            reportedDrops = droppedCallbacks;
            lastDropReport = now;
        }
    }
    if (dropsToReport) {
        MF_Log("MQTT callback queue dropped %llu events (capacity or message size limit)",
               static_cast<unsigned long long>(dropsToReport));
    }

    // Bound the work per frame. Newly produced events cannot extend this batch.
    while (remaining-- > 0) {
        CallbackEvent event;
        {
            std::lock_guard lock(callbackMutex);
            if (!acceptingCallbacks || pendingCallbacks.empty()) {
                return;
            }
            event = std::move(pendingCallbacks.front());
            pendingBytes -= event.bytes();
            pendingCallbacks.pop_front();
        }

        // Never hold the queue lock while running Pawn: a callback can invoke
        // MQTT natives, destroy a client, or trigger plugin shutdown.
        ExecuteCallback(event);
    }
}
