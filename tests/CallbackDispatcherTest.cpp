#include "CallbackDispatcher.h"
#include "MqttClientMgr.h"
#include "sdk/amxxmodule.h"
#include "RehldsFrameHook.h"
#include "FakeRehlds.h"

#include <atomic>
#include <cstdarg>
#include <cstdlib>
#include <functional>
#include <iostream>
#include <thread>
#include <vector>

namespace {
constexpr int connectedId = 10;
constexpr int messageId = 11;
constexpr int lostId = 12;
constexpr int disconnectedId = 13;
const auto gameThread = std::this_thread::get_id();

struct Invocation {
    int id;
    int handle;
    std::string text;
    int reason = 0;
};
std::vector<Invocation> invocations;
std::function<void(int)> onForward;

void Require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::abort();
    }
}

int ExecuteForward(int id, ...) {
    Require(std::this_thread::get_id() == gameThread, "Pawn executed on a worker thread");
    va_list args;
    va_start(args, id);
    Invocation invocation{id, va_arg(args, int), {}};
    if (id == messageId || id == lostId) {
        invocation.text = va_arg(args, const char *);
    } else if (id == disconnectedId) {
        invocation.reason = va_arg(args, int);
    }
    va_end(args);
    invocations.push_back(std::move(invocation));
    const auto action = onForward;
    if (action) {
        action(id);
    }
    return 0;
}

void UnregisterForward(int) {
    Require(std::this_thread::get_id() == gameThread, "Forward unregistered on a worker thread");
}

int AddNatives(const AMX_NATIVE_INFO *) { return 1; }

void Frame() {
    const int before = FakeEngineServerFrames();
    FakeEngineFrame();
    Require(FakeEngineServerFrames() == before + 1, "SV_Frame did not call the next hook exactly once");
}

int CreateClient() {
    // Creating a Paho client does not open a connection; no broker is needed.
    const int handle = g_mqttClientMgr.make("tcp://127.0.0.1:1883", "dispatcher-test");
    auto *client = g_mqttClientMgr.getClient(handle);
    client->setOnConnectedForwardId(connectedId);
    client->setOnMessageForwardId(messageId);
    client->setOnConnectionLostForwardId(lostId);
    client->setDisconnectForwardId(disconnectedId);
    return handle;
}
} // namespace

// Replace only the host-provided AMXX entry points. The queue, client manager,
// lifecycle callbacks, API discovery, and SV_Frame hook are the production code.
PFN_EXECUTE_FORWARD g_fn_ExecuteForward = ExecuteForward;
PFN_UNREGISTER_SPFORWARD g_fn_UnregisterSPForward = UnregisterForward;
PFN_ADD_NATIVES g_fn_AddNatives = AddNatives;
AMX_NATIVE_INFO g_natives[] = {{nullptr, nullptr}};
void MF_Log(const char *, ...) {}

int main() {
    for (auto mode : {NoInterface, WrongMajor, OldMinor, NoHookchains, NoFrameRegistry}) {
        FakeEngineSetMode(mode);
        std::string error;
        Require(!RegisterRehldsFrameHook(error) && !error.empty(), "Unsupported API was accepted");
        Require(OnAmxxCheckGame("cstrike") == AMXX_GAME_BAD, "Module accepted an unsupported engine");
        Require(FakeEngineHookCount() == 0, "Invalid API installed a frame hook");
    }
    FakeEngineSetMode(Compatible);
    Require(OnAmxxCheckGame("cstrike") == AMXX_GAME_OK, "API 3.7 was rejected");
    OnAmxxAttach();
    Require(FakeEngineHookCount() == 1, "Module did not register SV_Frame");
    std::string error;
    Require(RegisterRehldsFrameHook(error) && FakeEngineHookCount() == 1, "Duplicate hook registered");
    const int handle = CreateClient();
    ConnectedCallbackDispatcher(handle);
    Frame();
    Require(invocations.empty(), "Callbacks ran before plugins were loaded");
    OnPluginsLoaded();

    // All four event kinds must wait for SV_Frame, preserve their arguments,
    // and own the strings after the producer's objects are modified/destroyed.
    std::thread producer([handle] {
        auto message = mqtt::make_message("topic", "original payload");
        std::string reason = "original reason";
        ConnectedCallbackDispatcher(handle);
        MessageCallbackDispatcher(handle, message);
        ConnectionLostCallbackDispatcher(handle, reason);
        DisconnectedCallbackDispatcher(handle, mqtt::SERVER_BUSY);
        message->set_payload("changed payload");
        reason = "changed reason";
    });
    producer.join();
    Require(invocations.empty(), "A producer executed a callback before SV_Frame");
    FakeEngineSetPaused(true);
    const int physicsBeforePause = FakeEnginePhysicsFrames();
    Frame();
    Require(FakeEnginePhysicsFrames() == physicsBeforePause, "Paused engine unexpectedly ran physics");
    Require(invocations.size() == 4, "Not all event kinds were dispatched");
    Require(invocations[0].id == connectedId && invocations[0].handle == handle, "Connected event mismatch");
    Require(invocations[1].id == messageId && invocations[1].text == "original payload", "Payload was not copied");
    Require(invocations[2].id == lostId && invocations[2].text == "original reason", "Reason was not copied");
    Require(invocations[3].id == disconnectedId && invocations[3].reason == mqtt::SERVER_BUSY, "Disconnect reason mismatch");

    // An MQTT callback can unpause the game without needing GameDLL StartFrame.
    invocations.clear();
    onForward = [](int) { FakeEngineSetPaused(false); };
    ConnectedCallbackDispatcher(handle);
    Frame();
    Require(invocations.size() == 1, "Unpause callback stalled while the game was paused");
    onForward = {};
    Frame();
    Require(FakeEnginePhysicsFrames() == physicsBeforePause + 1, "Game did not resume after MQTT callback");

    // A callback can wait for a producer without deadlocking on the queue lock.
    // Its newly queued event is delivered on the following frame.
    invocations.clear();
    onForward = [handle](int id) {
        if (id == connectedId) {
            std::thread nested([handle] { ConnectionLostCallbackDispatcher(handle, "next frame"); });
            nested.join();
        }
    };
    ConnectedCallbackDispatcher(handle);
    Frame();
    Require(invocations.size() == 1, "Reentrant events ran in the same frame");
    Frame();
    Require(invocations.size() == 2 && invocations.back().text == "next frame", "Reentrant event was lost");
    onForward = {};

    // Multiple producers enqueue while the game thread drains. Every producer's
    // messages must arrive once and in order, across bounded frame batches.
    invocations.clear();
    std::atomic<int> finished{0};
    std::vector<std::thread> producers;
    for (int source = 0; source < 4; ++source) {
        producers.emplace_back([&, source] {
            for (int sequence = 0; sequence < 400; ++sequence) {
                ConnectionLostCallbackDispatcher(handle, std::to_string(source) + ":" + std::to_string(sequence));
            }
            ++finished;
        });
    }
    while (finished.load() != 4) {
        Frame();
        std::this_thread::yield();
    }
    for (auto &thread : producers) {
        thread.join();
    }
    for (int frame = 0; frame < 10; ++frame) {
        Frame();
    }
    Require(invocations.size() == 1600, "Concurrent events were lost or duplicated");
    int nextSequence[4]{};
    for (const auto &invocation : invocations) {
        const auto separator = invocation.text.find(':');
        const int source = std::stoi(invocation.text.substr(0, separator));
        const int sequence = std::stoi(invocation.text.substr(separator + 1));
        Require(source >= 0 && source < 4, "Invalid producer id");
        Require(sequence == nextSequence[source]++, "Producer event ordering changed");
    }

    invocations.clear();
    for (int i = 0; i < 512; ++i) {
        ConnectedCallbackDispatcher(handle);
    }
    Frame();
    Require(invocations.size() == 256, "Frame callback budget was not enforced");
    Frame();
    Require(invocations.size() == 512, "Events beyond the frame budget were lost");

    // A Pawn callback may destroy its client. Later queued events for that
    // missing client must be skipped without accessing the freed object.
    invocations.clear();
    const int destroyedHandle = CreateClient();
    onForward = [destroyedHandle](int) { g_mqttClientMgr.destroy(destroyedHandle); };
    ConnectedCallbackDispatcher(destroyedHandle);
    ConnectionLostCallbackDispatcher(destroyedHandle, "discard");
    Frame();
    Require(invocations.size() == 1, "Destroyed client still received callbacks");
    onForward = {};

    // Shutdown from inside a callback cancels the rest of the frame; late
    // worker events while stopped are discarded and cannot cross into reload.
    invocations.clear();
    onForward = [](int) { OnPluginsUnloading(); };
    ConnectedCallbackDispatcher(handle);
    ConnectionLostCallbackDispatcher(handle, "discard at shutdown");
    Frame();
    Require(invocations.size() == 1, "Callbacks continued after plugin shutdown");
    onForward = {};
    std::thread lateProducer([handle] { ConnectedCallbackDispatcher(handle); });
    lateProducer.join();
    OnPluginsLoaded();
    Frame();
    Require(invocations.size() == 1, "Old queued events survived plugin reload");
    ConnectedCallbackDispatcher(handle);
    Frame();
    Require(invocations.size() == 2, "Dispatch did not resume after plugin reload");

    ConnectedCallbackDispatcher(handle);
    OnAmxxDetach();
    Require(FakeEngineHookCount() == 0, "Module detach left its hook registered");
    UnregisterRehldsFrameHook();
    Require(FakeEngineHookCount() == 0, "Repeated detach unregistered twice");
    Frame();
    Require(invocations.size() == 2, "Callbacks ran after module detach");
    std::cout << "PASS: API validation, paused SV_Frame dispatch, unpause, main-thread ordering, reentrancy, and detach" << std::endl;
}
