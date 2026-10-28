#include "MqttClient.h"

#include <atomic>
#include <chrono>
#include <cstdlib>
#include <future>
#include <iostream>
#include <memory>
#include <vector>

using namespace std::chrono_literals;

namespace {
void Require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::abort();
    }
}

struct Observations {
    std::atomic<bool> destroyed{false};
    std::atomic<int> lateCallbacks{0};
    std::atomic<int> disconnects{0};
    std::promise<void> connected;
    std::promise<void> disconnected;

    void observe() {
        if (destroyed.load()) {
            ++lateCallbacks;
        }
    }
};
} // namespace

int main(int argc, char **argv) {
    Require(argc == 2, "Expected a loopback broker URI");
    std::vector<std::shared_ptr<Observations>> observations;
    {
        // Keep Paho initialized between cases so its threads can overlap client
        // destruction/creation instead of stopping after every single client.
        MqttClient keeper(argv[1], "lifetime-keeper");
        for (int i = 0; i < 50; ++i) {
            const auto state = std::make_shared<Observations>();
            observations.push_back(state);
            auto connected = state->connected.get_future();
            auto disconnected = state->disconnected.get_future();
            {
                MqttClient client(argv[1], "lifetime-" + std::to_string(i));
                client.setConnectedHandler([state](const MqttClient *, const std::string &) {
                    state->observe();
                    state->connected.set_value();
                });
                client.setDisconnectHandler([state](const MqttClient *, const mqtt::properties &, mqtt::ReasonCode) {
                    state->observe();
                    if (++state->disconnects == 1) {
                        state->disconnected.set_value();
                    }
                });
                client.connect();
                Require(connected.wait_for(5s) == std::future_status::ready, "Connect did not complete");
                client.disconnect();
                if (i == 0) {
                    Require(disconnected.wait_for(5s) == std::future_status::ready,
                            "Paho disconnect completion did not notify the handler");
                    Require(state->disconnects == 1, "Disconnect completion was duplicated");
                }
                // Remaining cases destroy immediately, including repeated
                // requests that may fail synchronously or still be pending.
                else if (i % 2 == 0) {
                    client.disconnect();
                }
            }
            state->destroyed = true;
        }
    }
    // All Paho clients and workers are gone. No module-owned waiters may remain.
    for (const auto &state : observations) {
        Require(state->lateCallbacks == 0, "A disconnect callback outlived its client");
    }
    std::cout << "PASS: live disconnect completion and immediate teardown of pending actions" << std::endl;
}
