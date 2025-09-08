# AmxxMqttClient
-------
A mqtt client module for AmxModx

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

    mqtt_connect(h);
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

    mqtt_subscribe(h, newTopic);

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
        new args = mqtt_publish(h, "cs/server", data);
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

public on_message(const MqttHandle:h, const message[])
{
    server_print("%d -> %s",h , message);
    client_print(0, print_chat, "[MQTT(%d)]: %s", h, message);
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