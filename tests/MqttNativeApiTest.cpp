#include "CallbackDispatcher.h"
#include "ConnectOptionMgr.h"
#include "MqttClientMgr.h"
#include "natives.h"

#include <chrono>
#include <cstdarg>
#include <cstdio>
#include <cstdlib>
#include <cstring>
#include <iostream>
#include <memory>
#include <thread>
#include <unordered_map>
#include <vector>

using namespace std::chrono_literals;

namespace {
const auto gameThread = std::this_thread::get_id();
std::unordered_map<cell, std::string> strings;
std::unordered_map<int, std::vector<int>> forwards;
std::vector<std::string> errors;
int nextForward = 1;

struct Message {
    int forward;
    int handle;
    std::string topic;
    std::string payload;
};
std::vector<Message> messages;

void Require(bool condition, const char *message) {
    if (!condition) {
        std::cerr << "FAIL: " << message << std::endl;
        std::abort();
    }
}

cell Text(const std::string &text) {
    const cell address = static_cast<cell>(strings.size() + 1);
    strings.emplace(address, text);
    return address;
}

char *GetAmxString(AMX *, cell address, int, int *length) {
    auto &text = strings.at(address);
    if (length) {
        *length = static_cast<int>(text.size());
    }
    return text.data();
}

int RegisterForward(AMX *, const char *name, ...) {
    Require(std::this_thread::get_id() == gameThread, "Forward registered off the game thread");
    if (std::strcmp(name, "missing") == 0) {
        return -1;
    }
    std::vector<int> types;
    va_list args;
    va_start(args, name);
    for (int type = va_arg(args, int); type != FP_DONE; type = va_arg(args, int)) {
        Require(types.size() < 3, "Too many message callback arguments");
        types.push_back(type);
    }
    va_end(args);
    Require(types == std::vector<int>{FP_CELL, FP_STRING, FP_STRING}, "Incorrect message callback signature");
    forwards.emplace(nextForward, std::move(types));
    return nextForward++;
}

void UnregisterForward(int id) {
    Require(std::this_thread::get_id() == gameThread, "Forward unregistered off the game thread");
    Require(forwards.erase(id) == 1, "Invalid or duplicate forward release");
}

int ExecuteForward(int id, ...) {
    Require(std::this_thread::get_id() == gameThread, "Pawn ran off the game thread");
    Require(forwards.count(id) == 1, "Executed a stale forward");
    va_list args;
    va_start(args, id);
    const int handle = va_arg(args, int);
    const std::string topic = va_arg(args, const char *);
    const std::string payload = va_arg(args, const char *);
    va_end(args);
    messages.push_back({id, handle, topic, payload});
    return 0;
}

cell Call(const char *name, std::initializer_list<cell> arguments) {
    // Match Pawn's native ABI, including exact-sized older argument frames.
    auto params = std::make_unique<cell[]>(arguments.size() + 1);
    params[0] = static_cast<cell>(arguments.size() * sizeof(cell));
    size_t index = 1;
    for (const auto argument : arguments) {
        params[index++] = argument;
    }
    for (const auto *native = g_natives; native->name; ++native) {
        if (std::strcmp(native->name, name) == 0) {
            return native->func(nullptr, params.get());
        }
    }
    Require(false, "Native is not exported");
    return 0;
}

template <typename Predicate>
void WaitFor(Predicate predicate) {
    const auto deadline = std::chrono::steady_clock::now() + 5s;
    while (!predicate() && std::chrono::steady_clock::now() < deadline) {
        ProcessPendingCallbacks();
        std::this_thread::sleep_for(1ms);
    }
    Require(predicate(), "Timed out waiting for MQTT activity");
}

void ExpectMessage(int handle, const std::string &topic, const std::string &payload) {
    WaitFor([] { return !messages.empty(); });
    Require(messages.size() == 1, "Message callback was duplicated");
    const auto &message = messages.front();
    Require(message.handle == handle && message.topic == topic && message.payload == payload,
            "Unexpected topic, payload, QoS, retained flag, or callback argument order");
    messages.clear();
}
} // namespace

PFN_GET_AMXSTRING g_fn_GetAmxString = GetAmxString;
PFN_GET_AMXADDR g_fn_GetAmxAddr = nullptr; // Only option setters use this host API.
PFN_REGISTER_SPFORWARD_BYNAME g_fn_RegisterSPForwardByName = RegisterForward;
PFN_UNREGISTER_SPFORWARD g_fn_UnregisterSPForward = UnregisterForward;
PFN_EXECUTE_FORWARD g_fn_ExecuteForward = ExecuteForward;

void MF_LogError(AMX *, int, const char *format, ...) {
    char message[512];
    va_list args;
    va_start(args, format);
    std::vsnprintf(message, sizeof(message), format, args);
    va_end(args);
    errors.emplace_back(message);
}

int main(int argc, char **argv) {
    Require(argc == 2, "Expected a loopback broker URI");
    StartCallbackDispatcher();
    const int handle = Call("mqtt_create", {Text(argv[1]), Text("native-api")});
    Require(handle > 0, "Client creation failed");
    Require(Call("mqtt_set_message_callback", {handle, Text("on_message")}) == 1, "Callback registration failed");
    const int options = Call("mqtt_create_connect_options", {});
    Require(Call("mqtt_connect", {handle, options}) == 1, "Connect was rejected");
    WaitFor([handle] { return Call("mqtt_is_connect", {handle}) != 0; });

    // Missing QoS keeps its default, and retained stays in argument four.
    Require(Call("mqtt_subscribe", {handle, Text("native/default")}) == 1, "Default subscribe failed");
    ExpectMessage(handle, "native/default", "subscribe:0");
    Require(Call("mqtt_publish", {handle, Text("native/default"), Text("data")}) == 1, "Default publish failed");
    ExpectMessage(handle, "native/default", "publish:0:0:data");
    Require(Call("mqtt_publish", {handle, Text("native/retained"), Text("data"), 1}) == 1, "Retained publish failed");
    ExpectMessage(handle, "native/retained", "publish:0:1:data");

    for (int qos = 0; qos <= 2; ++qos) {
        const std::string topic = "native/qos/" + std::to_string(qos);
        Require(Call("mqtt_subscribe", {handle, Text(topic), qos}) == 1, "QoS subscribe failed");
        ExpectMessage(handle, topic, "subscribe:" + std::to_string(qos));
        Require(Call("mqtt_publish", {handle, Text(topic), Text("data"), 0, qos}) == 1, "QoS publish failed");
        ExpectMessage(handle, topic, "publish:" + std::to_string(qos) + ":0:data");
    }
    Require(errors.empty(), "Valid native call reported an error");

    for (const int qos : {-1, 3}) {
        Require(Call("mqtt_subscribe", {handle, Text("invalid"), qos}) == 0, "Invalid subscribe QoS accepted");
        Require(Call("mqtt_publish", {handle, Text("invalid"), Text("data"), 0, qos}) == 0, "Invalid publish QoS accepted");
    }
    Require(errors.size() == 4, "QoS validation did not report native errors");
    for (const auto &error : errors) {
        Require(error.find("Invalid MQTT QoS") != std::string::npos, "QoS validation returned an unrelated error");
    }

    const int oldForward = g_mqttClientMgr.getClient(handle)->getOnMessageForwardId();
    Require(Call("mqtt_set_message_callback", {handle, Text("missing")}) == 0, "Missing callback accepted");
    Require(g_mqttClientMgr.getClient(handle)->getOnMessageForwardId() == oldForward,
            "Failed registration discarded the existing callback");
    Require(Call("mqtt_set_message_callback", {handle, Text("replacement")}) == 1, "Callback replacement failed");
    Require(forwards.size() == 1 && forwards.count(oldForward) == 0, "Callback replacement leaked a forward");
    Require(Call("mqtt_publish", {handle, Text("native/after-replace"), Text("data"), 1, 2}) == 1, "Publish after replacement failed");
    ExpectMessage(handle, "native/after-replace", "publish:2:1:data");
    WaitFor([handle] { return g_mqttClientMgr.getClient(handle)->getClient()->get_pending_delivery_tokens().empty(); });

    Require(Call("mqtt_destroy", {handle}) == 1, "Client destruction failed");
    Require(forwards.empty(), "Client destruction leaked a forward");
    StopCallbackDispatcher();
    g_connectOptionMgr.reset();
    std::cout << "PASS: native QoS defaults/0/1/2/validation, retained, topic/payload order, and forward replacement" << std::endl;
}
