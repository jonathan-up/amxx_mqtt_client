# AmxxMqttClient

[English](README.md) | [简体中文](README.zh-CN.md)

适用于 AMX Mod X 的 MQTT 客户端模块。需要 ReHLDS API 3.x，且版本不低于 3.7。
本模块直接访问引擎 API，无需安装 ReAPI AMXX 模块。

## 回调分发机制

网络线程将 MQTT 事件加入队列，再通过 ReHLDS 的 `SV_Frame` 钩子在游戏主线程中
调用 Pawn 回调；游戏暂停期间也能继续分发事件。每个服务器帧按入队顺序最多
处理 256 个事件，其余事件留到后续帧处理。执行回调时不持有队列锁。
插件卸载或模块分离时，会停止接收新事件并丢弃尚未处理的事件。

客户端句柄为不透明的正整数，在模块本次加载期间不会复用，换图也不会重置。
客户端销毁后，其排队中或迟到的事件会被丢弃，不会调用新客户端的回调。
最多允许同时存在 10,000 个客户端；若耗尽 32 位正整数句柄范围，创建会失败，
不会回收旧编号。客户端创建、查找、销毁及 AMXX forward 操作均在游戏主线程执行。
主动断开的完成通知使用 Paho 的 action listener，不再创建独立的等待线程。
插件卸载时还会销毁客户端和未使用的连接选项，并在 AMXX 清理 forward 前注销回调。
下一张地图加载插件时，需要重新创建客户端。

`mqtt_disconnect()` 是异步操作：断开成功后，先将连接状态更新为 false，再把断开
回调加入队列；即使没有注册回调，状态也会更新。请求尚未完成或失败时，不会仅因
调用了断开操作就把已连接状态改为 false。`mqtt_is_connect()` 返回最近已知的连接
状态；如果已重新连接，执行较早入队的断开回调时，查询结果也可能已经是 true。

模块在加载前检查引擎 API，并在分离时注销帧钩子。需要注意：

- 暂停 AMXX 插件后，该插件仍无法接收 Pawn 回调。
- 挂起整个服务器进程时，MQTT 处理也会暂停。
- 事件队列目前没有容量上限，持续过载或主线程停滞可能导致消息不断积压。

## 使用示例

以下 Pawn 示例演示客户端创建、连接、订阅、发布、连接状态查询、断开连接及回调注册。
使用前请将 MQTT 服务器地址、用户名和密码替换为自己的配置。

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

## 构建

构建前，先初始化 Paho 和 ReHLDS SDK 子模块：

```shell
git submodule update --init --recursive
```

### Linux

项目使用 CMake 3.20 或更新版本及支持 C++17 的 GCC，目标为 32 位 AMXX 模块。
在 Ubuntu 等 64 位系统上，需要安装 `gcc-multilib` 和 `g++-multilib`。

在仓库根目录执行：

```shell
cmake -S . -B build \
      -DPAHO_WITH_MQTT_C=TRUE \
      -DPAHO_BUILD_STATIC=TRUE \
      -DPAHO_WITH_SSL=FALSE \
      -DPAHO_ENABLE_TESTING=FALSE

cmake --build build --parallel 8
```

生成的模块为 `build/mqtt_client_amxx_i386.so`。

也可以运行仓库提供的脚本，构建结果位于 `build_sh/`：

```shell
./build.sh
```

### 回归测试

运行测试还需安装 Python 3。测试使用模拟的 ReHLDS API，并在本机回环地址的临时端口
启动最小 MQTT 测试服务，无需启动游戏服务器或外部 MQTT 消息代理（Broker）。
在配置时启用 `AMXX_MQTT_BUILD_TESTS`，然后构建并运行 CTest：

```shell
cmake -S . -B build \
      -DPAHO_WITH_SSL=FALSE \
      -DPAHO_ENABLE_TESTING=FALSE \
      -DAMXX_MQTT_BUILD_TESTS=ON
cmake --build build --parallel
ctest --test-dir build --output-on-failure
```

测试覆盖 API 校验、暂停期间的回调分发、主线程执行、事件顺序、回调重入、
单帧事件处理上限、客户端销毁重建、旧句柄与迟到事件隔离、插件重载、模块分离，
以及真实 Paho 连接的断开完成通知、连接状态更新、重复断开、重连、未注册回调，
和发起断开后立即销毁等场景。

### Windows（Win32）

仓库的 [CI 配置](.github/workflows/build.yml) 使用 Visual Studio 2022 的 Win32 工具链
构建模块并运行回归测试。安装 Visual Studio 2022 的 C++ 构建工具后，
可在仓库根目录使用以下命令：

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

以上多行命令使用适用于 Bash 的续行符 `\`。在 PowerShell 或命令提示符中，
请将 `cmake` 配置命令合并为一行执行。

生成的模块为 `build-win32/Release/mqtt_client_amxx.dll`。
