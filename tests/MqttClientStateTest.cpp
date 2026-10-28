#include "MqttClient.h"

#include <chrono>
#include <condition_variable>
#include <cstdlib>
#include <iostream>
#include <mutex>
#include <thread>

using namespace std::chrono_literals;

namespace {
void Require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::abort();
    }
}

class Events {
    std::mutex mutex;
    std::condition_variable changed;
    int connected = 0;
    int disconnected = 0;

public:
    void onConnected() {
        std::lock_guard lock(mutex);
        ++connected;
        changed.notify_all();
    }

    void onDisconnected() {
        std::lock_guard lock(mutex);
        ++disconnected;
        changed.notify_all();
    }

    void waitFor(int connections, int disconnections) {
        std::unique_lock lock(mutex);
        Require(changed.wait_for(lock, 5s, [&] {
            return connected >= connections && disconnected >= disconnections;
        }), "Timed out waiting for connection/disconnection");
        Require(connected == connections && disconnected == disconnections,
                "Connection/disconnection callback was duplicated");
    }
};
} // namespace

int main(int argc, char **argv) {
    Require(argc == 2, "Expected a loopback broker URI");
    Events events;
    MqttClient client(argv[1], "connection-state");
    Require(!client.isConnected(), "A new client reported connected");

    client.setConnectedHandler([&](const MqttClient *, const std::string &) {
        Require(client.isConnected(), "Connected callback observed disconnected state");
        events.onConnected();
    });
    client.setDisconnectHandler([&](const MqttClient *, const mqtt::properties &, mqtt::ReasonCode reason) {
        Require(!client.isConnected(), "Disconnect callback observed connected state");
        Require(reason == mqtt::NORMAL_DISCONNECTION, "Unexpected disconnect reason");
        events.onDisconnected();
    });

    // An already-disconnected request fails synchronously in Paho. Its existing
    // notification must leave the client disconnected and usable for connecting.
    client.disconnect();
    events.waitFor(0, 1);
    Require(!client.isConnected(), "Offline disconnect changed the connection state");

    // Reuse the same client, so the test catches stale state across reconnects.
    for (int cycle = 1; cycle <= 3; ++cycle) {
        client.connect();
        events.waitFor(cycle, 2 * cycle - 1);
        Require(client.isConnected(), "Connected client reported disconnected");
        client.disconnect();
        events.waitFor(cycle, 2 * cycle);
        Require(!client.isConnected(), "Completed disconnect left stale connected state");
        Require(!client.getClient()->is_connected(), "Paho did not complete disconnect");

        client.disconnect();
        events.waitFor(cycle, 2 * cycle + 1);
        Require(!client.isConnected(), "Repeated disconnect restored connected state");
    }

    // State changes must not depend on a user registering a disconnect handler.
    client.setDisconnectHandler({});
    client.connect();
    events.waitFor(4, 7);
    client.disconnect();
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (client.isConnected() && std::chrono::steady_clock::now() < deadline) {
        std::this_thread::sleep_for(1ms);
    }
    Require(!client.isConnected(), "Disconnect without a handler left stale state");
    Require(!client.getClient()->is_connected(), "Disconnect without a handler did not complete");
    std::cout << "PASS: disconnect state, callback ordering, repeated disconnect, reconnect, and no handler" << std::endl;
}
