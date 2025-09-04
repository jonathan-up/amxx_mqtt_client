#include <amxmodx>
#include "include/MqttClient"

new MqttHandle:g_client = MqttHandle_Invalid;

public plugin_init()
{
    register_plugin("Mqtt client test", "BETA", "H0meV");

    register_srvcmd("mq_cre", "cmd_create");
    register_srvcmd("mq_con", "cmd_connect");
}

public plugin_end()
{
    // TODO destroy
    mqtt_destroy(g_client);
}

public cmd_create()
{
    new clientId[32];
    read_argv(1, clientId, charsmax(clientId));

    g_client = mqtt_create("mqtt://202.189.6.222:1883", clientId);
    server_print("g_client: %d -> %s", g_client, clientId);

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

    return PLUGIN_HANDLED;
}

public on_connected(const MqttHandle:h)
{
    mqtt_subscribe(h, "cs/server");
    server_print("%d -> subscribe to: cs/server", h);
}

public on_message(const MqttHandle:h, const message[])
{
    server_print("%d -> %s",h , message);
}
