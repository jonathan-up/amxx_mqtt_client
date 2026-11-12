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

Connection option handles are also never reused during a module load, including
across map changes, with at most 10,000 live options. Once `mqtt_connect()` validates
both handles, it consumes the options even if Paho immediately rejects the request.
Create fresh options for each connection attempt; a consumed handle is invalid.
Replacing any registered callback releases its old forward. If registration fails,
the previous callback remains active.

`mqtt_disconnect()` is asynchronous. On successful completion the connection
state becomes false before the disconnected callback is queued, even without a
registered callback. A pending or failed request alone does not mark a connected
client as disconnected. `mqtt_is_connect()` reports the latest known state; a
subsequent reconnect may have completed before an older queued callback runs.
An offline client or a pending connect/disconnect operation makes
`mqtt_disconnect()` return 0 with a native error, without a disconnected callback.
To cancel a pending connection immediately, call `mqtt_destroy()`; a rejected
`mqtt_disconnect()` does not cancel it. Overlapping connect requests are rejected.
Asynchronous connect/disconnect failures are logged on the game thread and sent
to the optional `mqtt_set_error_callback()` callback; they never masquerade as a
normal disconnection. Synchronous failures use native errors instead.

The module checks the engine API before loading and unregisters its frame hook
on detach. A paused AMXX plugin still cannot receive Pawn callbacks, and suspending
the entire server process also suspends MQTT processing.

The queue is limited to 4,096 events and 8 MiB of queued topic/payload/error text.
An incoming message's topic plus payload must fit within 64 KiB. Oversized or
excess incoming messages are dropped. Control events (connection changes and
errors) evict the oldest queued message when needed; a queue containing only
control events evicts its oldest event. Retained events preserve FIFO order.
Drop counts are logged on the game thread at most once every five seconds.
These application-level drops can occur even with MQTT QoS 1/2; QoS does not
acknowledge Pawn callback execution. Destroying a client immediately removes its
queued events and rejects late arrivals. Limits apply to the module's callback
queue, not Paho's network receive buffers.

Failure notifications
-------

Register before connecting:

```c
mqtt_set_error_callback(h, "on_mqtt_error");

public on_mqtt_error(const MqttHandle:h, MqttOperation:operation,
                     code, MqttReasonCode:reason, const message[])
{
    server_print("MQTT %d operation %d failed: code=%d reason=%d %s",
                 h, operation, code, reason, message);
}
```

`operation` is `MQTT_OPERATION_CONNECT` (1) or `MQTT_OPERATION_DISCONNECT` (2).
`code` is Paho's return code and `reason` is the MQTT broker reason code. Inspect
both: a broker rejection can have `code == 0` and a failure reason such as 135
(not authorized). The callback follows the same queue, replacement and lifetime
rules as other callbacks. Update the include and recompile to use this new native.
Existing plugins must account for the corrected disconnect error behavior.

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
QoS on the wire, QoS 2 acknowledgements, retained flags, validation, topic/payload
order, all five callback replacements, and rejection of consumed connection options.
Options manager tests cover stale handles, map-change reset, and the live-options limit.
Additional regressions cover queue count/byte limits, oversized messages, control
event priority, immediate purge and late-event rejection after destruction,
authentication rejection, connect timeout, early transport closure, retries,
failure logging without a callback, rejected disconnects while connecting, and
destruction of pending connections:

```shell
cmake -S . -B build -DPAHO_WITH_SSL=FALSE -DPAHO_ENABLE_TESTING=FALSE -DAMXX_MQTT_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

Windows (Win32)
-------
The [CI workflow](.github/workflows/build.yml) builds the module and runs regression
tests using Visual Studio 2022's Win32 toolchain. Install the Visual Studio 2022 C++
build tools and Python 3, then run these commands from the repository root:

```shell
cmake -S . -B build-win32 \
      -G "Visual Studio 17 2022" -A Win32 \
      -DPAHO_WITH_MQTT_C=TRUE \
      -DPAHO_WITH_SSL=FALSE \
      -DPAHO_ENABLE_TESTING=FALSE \
      -DAMXX_MQTT_BUILD_TESTS=ON
cmake --build build-win32 --config Release --parallel
ctest --test-dir build-win32 -C Release --output-on-failure
```

The multiline configuration command uses Bash's `\` line continuation. In
PowerShell or Command Prompt, write the configuration command on a single line.

The resulting module is `build-win32/Release/mqtt_client_amxx.dll`.
