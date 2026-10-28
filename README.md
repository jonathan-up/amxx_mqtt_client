AmxxMqttClient
-------
[English](README.md) | [简体中文](README.zh-CN.md)

A mqtt client module for AmxModx. Requires ReHLDS API 3.x, version 3.7 or later.
The ReAPI AMXX module is not required; this module accesses the engine API directly.

MQTT events are queued by the network threads and delivered to Pawn on the game
thread via ReHLDS's `SV_Frame` hook, including while the game is paused. Each
server frame processes at most 256 events
in queue order; remaining events wait for subsequent frames. Callbacks run
without holding the queue lock. Pending events are discarded while plugins are
unloading or the module is detaching.

Client handles are opaque positive integers and are never reused during a module
load, including across map changes. Destroyed clients' queued or late events are
discarded. Up to 10,000 clients may be alive at once; exhausting the positive
32-bit handle range fails creation instead of recycling old handles. Client
creation, lookup, destruction, and AMXX forward access stay on the game thread.
Disconnect completion uses Paho's action listener, with no detached wait thread.
Plugin unload also destroys clients and unused connection options before AMXX
releases their forwards. Create new clients when the next map's plugins load.

`mqtt_disconnect()` is asynchronous. On successful completion the connection
state becomes false before the disconnected callback is queued, even without a
registered callback. A pending or failed request alone does not mark a connected
client as disconnected. `mqtt_is_connect()` reports the latest known state; a
subsequent reconnect may have completed before an older queued callback runs.

The module checks the engine API before loading and unregisters its frame hook
on detach. A paused AMXX plugin still cannot receive Pawn callbacks, and suspending
the entire server process also suspends MQTT processing. The event queue currently
has no capacity limit, so sustained overload or a stalled main thread can still
cause queued messages to accumulate.

QoS and message callbacks
-------

Subscriptions and publishes accept QoS 0, 1, or 2; omitted QoS defaults to 0:

```c
mqtt_subscribe(h, "cs/server/commands", 1);
mqtt_publish(h, "cs/server/status", "ready", false, 2);
```

For publishing, `retained` remains the fourth argument and `qos` is fifth.
Invalid QoS values produce a native error. A return value of 1 means the
asynchronous request was submitted, not that the broker has acknowledged it.

Message callbacks now receive **`(handle, topic, payload)`**, using the same
`mqtt_set_message_callback()` registration native. This changes the old
`(handle, payload)` signature: update and recompile plugins using message
callbacks before replacing the module. An old two-argument callback would read
the topic as its payload. See the updated example below.

Example
-------

```c
#include <amxmodx>
#include "include/MqttClient"

new MqttHandle:g_client = MqttHandle_Invalid;
new g_lastSub[32];

public plugin_init()
{
    register_plugin("Mqtt client test", "BETA", "H0meV");

    register_srvcmd("mq_cre", "cmd_create");
    register_srvcmd("mq_con", "cmd_connect");
    register_srvcmd("mq_sub", "cmd_subscribe");
    register_srvcmd("mq_pub", "cmd_publish");
    register_srvcmd("mq_isc", "cmd_isConnected");
    register_srvcmd("mq_dis", "cmd_disconnect");
}

public plugin_end()
{
    mqtt_destroy(g_client);
}

public cmd_create()
{
    new clientId[32];
    read_argv(1, clientId, charsmax(clientId));

    g_client = mqtt_create("mqtt://202.189.6.222:1883", clientId);
    server_print("%d -> create client: %s", g_client, clientId);

    return PLUGIN_HANDLED;
}

public cmd_connect()
{
    new MqttHandle:h = MqttHandle:read_argv_int(1);

    if (h == MqttHandle_Invalid)
    {
        server_print("invalid handle: %d", h);
        return PLUGIN_HANDLED;
    }

    server_print("cmd_connect: %d", h);

    new MqttConnectOptions:opts = mqtt_create_connect_options();
    mqtt_set_options(opts, MQTT_OPTION_NAMES_USERNAME, "cs_server");
    mqtt_set_options(opts, MQTT_OPTION_NAMES_PASSWORD, "123123");
    mqtt_set_options(opts, MQTT_OPTIONS_CLEAN_START, false);
    mqtt_set_options(opts, MQTT_OPTION_NAMES_SESSION_EXPIRY, 30 /**60*60*24*30**/);

    server_print("opts: %d", opts);

    mqtt_connect(h, opts);
    mqtt_set_connected_callback(h, "on_connected");
    mqtt_set_message_callback(h, "on_message");
    mqtt_set_connection_lost_callback(h, "on_connection_lost");
    mqtt_set_disconnected_callback(h, "on_disconnected");

    return PLUGIN_HANDLED;
}

public cmd_disconnect()
{
    new MqttHandle:h = MqttHandle:read_argv_int(1);

    if (h == MqttHandle_Invalid)
    {
        server_print("invalid handle: %d", h);
        return PLUGIN_HANDLED;
    }

    server_print("cmd_disconnect: %d", h);

    mqtt_disconnect(h);

    return PLUGIN_HANDLED;
}

public cmd_subscribe()
{
    new MqttHandle:h = MqttHandle:read_argv_int(1);

    if (h == MqttHandle_Invalid)
    {
        server_print("invalid handle: %d", h);
        return PLUGIN_HANDLED;
    }

    if (g_lastSub[0] != 0)
    {
        mqtt_unsubscribe(h, g_lastSub);
    }

    new newTopic[32];
    read_argv(2, newTopic, charsmax(newTopic));

    mqtt_subscribe(h, newTopic, 1);

    copy(g_lastSub, charsmax(g_lastSub), newTopic);

    return PLUGIN_HANDLED;
}

public cmd_publish()
{
    new MqttHandle:h = MqttHandle:read_argv_int(1);

    if (h == MqttHandle_Invalid)
    {
        server_print("invalid handle: %d", h);
        return PLUGIN_HANDLED;
    }

    new data[32];
    read_argv(2, data, charsmax(data));

    if (mqtt_is_connect(h))
    {
        new args = mqtt_publish(h, "cs/server", data, false, 1);
        server_print("%d", args);
    }

    return PLUGIN_HANDLED;
}

public cmd_isConnected()
{
    new MqttHandle:h = MqttHandle:read_argv_int(1);

    if (h == MqttHandle_Invalid)
    {
        server_print("invalid handle: %d", h);
        return PLUGIN_HANDLED;
    }

    new const bool:connected = mqtt_is_connect(h);
    server_print("Connection status: %s", connected ? "connected" : "disconnected");

    return PLUGIN_HANDLED;
}

public on_connected(const MqttHandle:h)
{
    // mqtt_subscribe(h, "cs/server");
    server_print("%d -> connected", h);
}

public on_message(const MqttHandle:h, const topic[], const message[])
{
    server_print("%d -> [%s] %s", h, topic, message);
    client_print(0, print_chat, "[MQTT(%d) %s]: %s", h, topic, message);
}
public on_connection_lost(const MqttHandle:h, const reason[])
{
    server_print("%d -> connection lost: %s",h , reason);
}
public on_disconnected(const MqttHandle:h, const MqttReasonCode:reasonCode)
{
    server_print("%d -> disconnected: %d",h , reasonCode);
}
```

Building
-------
Initialize the Paho and ReHLDS SDK submodules before building:

```shell
git submodule update --init --recursive
```

Linux
-------
```shell
mkdir builda && cd builda
cmake .. \
      -DPAHO_WITH_MQTT_C=TRUE \
      -DPAHO_BUILD_STATIC=TRUE \
      -DPAHO_WITH_SSL=FALSE \
      -DPAHO_ENABLE_TESTING=FALSE

make -j8
```

Or

```shell
./build.sh
```

To build and run the regression tests, install Python 3 as well. Tests provide a
fake ReHLDS API and a minimal MQTT peer on an ephemeral loopback port; no game
server or external broker is required. They cover dispatch ordering, stale handles,
plugin reload, disconnect completion and state, reconnecting, and immediate
destruction of pending actions. Native API tests also check subscription/publish
QoS on the wire, QoS 2 acknowledgements, retained flags, validation, and topic/payload order:

```shell
cmake -S . -B build -DPAHO_WITH_SSL=FALSE -DPAHO_ENABLE_TESTING=FALSE -DAMXX_MQTT_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Windows
-------
No supports yet
