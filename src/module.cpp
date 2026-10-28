//
// Created by jonathan on 9/2/25.
//

#include "sdk/amxxmodule.h"
#include "natives.h"

#include "MqttClientMgr.h"
#include "ConnectOptionMgr.h"
#include "CallbackDispatcher.h"
#include "RehldsFrameHook.h"
#include <cstdio>

int OnAmxxCheckGame(const char *) {
    std::string error;
    if (!CheckRehldsApi(error)) {
        // AMXX's logging API is not yet available during CheckGame.
        std::fprintf(stderr, "[AmxxMqttClient] %s\n", error.c_str());
        return AMXX_GAME_BAD;
    }
    return AMXX_GAME_OK;
}

void OnAmxxAttach() {
    std::string error;
    if (!RegisterRehldsFrameHook(error)) {
        MF_Log("%s", error.c_str());
        return;
    }
    MF_AddNatives(g_natives);
    MF_Log("==================OnAmxxAttach==================");
}

void OnAmxxDetach() {
    StopCallbackDispatcher();
    UnregisterRehldsFrameHook();
    g_mqttClientMgr.reset();
    g_connectOptionMgr.reset();
    MF_Log("==================OnAmxxDetach==================");
}

void OnPluginsLoaded() {
    StartCallbackDispatcher();
}

void OnPluginsUnloading() {
    StopCallbackDispatcher();
    // AMXX still owns the forwards here. Release them before it unloads Pawn
    // programs or reuses forward IDs for the next map's plugins.
    g_mqttClientMgr.reset();
    g_connectOptionMgr.reset();
}
