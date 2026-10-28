//
// Created by jonathan on 2025/9/4.
//

#include "CallbackDispatcher.h"

#include <algorithm>
#include <deque>
#include <mutex>
#include <utility>
#include "sdk/amxxmodule.h"
#include "MqttClientMgr.h"

namespace {
enum class CallbackType { Connected, Message, ConnectionLost, Disconnected };

struct CallbackEvent {
    CallbackType type;
    int handle;
    std::string text;
    mqtt::ReasonCode reasonCode = mqtt::NORMAL_DISCONNECTION;
    std::string topic;
};

std::mutex callbackMutex;
std::deque<CallbackEvent> pendingCallbacks;
bool acceptingCallbacks = false;
constexpr size_t maxCallbacksPerFrame = 256;

void EnqueueCallback(CallbackEvent event) {
    std::lock_guard lock(callbackMutex);
    if (acceptingCallbacks) {
        pendingCallbacks.push_back(std::move(event));
    }
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
    }
}
} // namespace

void ConnectedCallbackDispatcher(const int handle) {
    EnqueueCallback({CallbackType::Connected, handle, {}});
}

void MessageCallbackDispatcher(const int handle, const mqtt::const_message_ptr &msg) {
    CallbackEvent event{CallbackType::Message, handle, msg->get_payload_str()};
    event.topic = msg->get_topic();
    EnqueueCallback(std::move(event));
}

void ConnectionLostCallbackDispatcher(const int handle, const std::string &reason) {
    EnqueueCallback({CallbackType::ConnectionLost, handle, reason});
}

void DisconnectedCallbackDispatcher(const int handle, const mqtt::ReasonCode reasonCode) {
    EnqueueCallback({CallbackType::Disconnected, handle, {}, reasonCode});
}

void StartCallbackDispatcher() {
    std::lock_guard lock(callbackMutex);
    acceptingCallbacks = true;
}

void StopCallbackDispatcher() {
    std::lock_guard lock(callbackMutex);
    acceptingCallbacks = false;
    pendingCallbacks.clear();
}

void ProcessPendingCallbacks() {
    size_t remaining;
    {
        std::lock_guard lock(callbackMutex);
        remaining = std::min(pendingCallbacks.size(), maxCallbacksPerFrame);
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
            pendingCallbacks.pop_front();
        }

        // Never hold the queue lock while running Pawn: a callback can invoke
        // MQTT natives, destroy a client, or trigger plugin shutdown.
        ExecuteCallback(event);
    }
}
