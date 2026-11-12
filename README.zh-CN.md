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

连接选项句柄同样在模块本次加载期间不会复用，换图也不会重置，最多允许同时存在
10,000 个连接选项。`mqtt_connect()` 校验两个句柄有效后会消耗连接选项，即使 Paho
立即拒绝连接请求也是如此。每次尝试连接都应创建新的选项，已消耗的句柄不可再次使用。
替换任意类型的回调都会释放旧 forward；如果新回调注册失败，则保留原回调。

`mqtt_disconnect()` 是异步操作：断开成功后，先将连接状态更新为 false，再把断开
回调加入队列；即使没有注册回调，状态也会更新。请求尚未完成或失败时，不会仅因
调用了断开操作就把已连接状态改为 false。`mqtt_is_connect()` 返回最近已知的连接
状态；如果已重新连接，执行较早入队的断开回调时，查询结果也可能已经是 true。
客户端未连接，或连接/断开操作尚未完成时，`mqtt_disconnect()` 返回 0 并报告 native
错误，不再发送正常断开通知。需要立即取消正在进行的连接时，调用 `mqtt_destroy()`；
被拒绝的断开请求不会取消该连接。重复连接请求同样会被拒绝。
连接或断开的异步失败会在游戏主线程写入日志，并通知可选的错误回调；同步失败通过
native 错误报告。旧插件需要注意这一断开错误行为的修正。

模块在加载前检查引擎 API，并在分离时注销帧钩子。需要注意：

- 暂停 AMXX 插件后，该插件仍无法接收 Pawn 回调。
- 挂起整个服务器进程时，MQTT 处理也会暂停。

队列最多保留 4,096 个事件，排队中的主题、正文和错误文本总计最多 8 MiB。单条消息的
主题加正文最多 64 KiB；超过大小或队列容量限制时，丢弃新消息。连接状态及错误通知
优先于消息，必要时淘汰最旧消息；若队列全部为控制事件，则淘汰最旧控制事件。
保留事件仍按入队顺序执行，丢弃数量每五秒最多记录一次日志。
即使使用 QoS 1/2，也可能发生上述应用层丢弃：MQTT 确认不等于 Pawn 回调已执行。
销毁客户端时立即清除其排队事件，并拒绝迟到事件。这些限制针对模块回调队列，
不限制 Paho 自身的网络接收缓冲区。

## 错误回调

连接前可以注册：

```c
mqtt_set_error_callback(h, "on_mqtt_error");

public on_mqtt_error(const MqttHandle:h, MqttOperation:operation,
                     code, MqttReasonCode:reason, const message[])
{
    server_print("MQTT %d operation %d failed: code=%d reason=%d %s",
                 h, operation, code, reason, message);
}
```

`operation` 为 `MQTT_OPERATION_CONNECT`（1）或 `MQTT_OPERATION_DISCONNECT`（2）。
`code` 为 Paho 返回码，`reason` 为 Broker 的 MQTT 原因码，需要同时检查：认证拒绝
可能返回 `code == 0`、`reason == 135`。即使没有注册错误回调，异步失败也会写日志。
错误回调遵循同样的有界队列、替换和销毁规则；使用新增 native 需要更新 include 并重新编译。

## QoS 与消息回调

订阅和发布均支持 QoS 0、1、2，省略时默认为 0：

```c
mqtt_subscribe(h, "cs/server/commands", 1);
mqtt_publish(h, "cs/server/status", "ready", false, 2);
```

发布函数的第四个参数仍为 `retained`，第五个参数是 `qos`。无效 QoS 会触发 native
错误。返回 1 只表示异步请求已提交，不表示 Broker 已完成确认。

消息回调统一为 **`(handle, topic, payload)`**，仍通过 `mqtt_set_message_callback()`
注册。这改变了旧的 `(handle, payload)` 签名：使用消息回调的插件需要修改源码，
重新编译后再更新模块；旧的两参数回调会把 topic 当作 payload。下方示例已更新。

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
Native 接口测试还会检查实际报文中的订阅/发布 QoS、QoS 2 确认流程、retained 标记、
无效 QoS 参数处理、消息回调的 topic/payload 顺序、五类回调的替换，以及拒绝已消耗的
连接选项句柄。连接选项管理器测试还覆盖旧句柄隔离、换图重置及有效选项数量上限。
新增回归场景包括队列条数/字节限制、超大消息、过载时的控制事件优先级、销毁后立即
清队列及拒绝迟到消息、Broker 认证拒绝、连接超时、握手前断线、失败后的重试、
无错误回调时的日志、连接中的断开拒绝，以及销毁尚未完成连接的客户端。

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
