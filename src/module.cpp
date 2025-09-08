//
// Created by jonathan on 9/2/25.
//

#include "sdk/amxxmodule.h"
#include "natives.h"

#include "MqttClientMgr.h"
#include "ConnectOptionMgr.h"

void OnAmxxAttach() {
    MF_AddNatives(g_natives);
    MF_Log("==================OnAmxxAttach==================");
}

void OnAmxxDetach() {
    g_mqttClientMgr.reset();
    g_connectOptionMgr.reset();
    MF_Log("==================OnAmxxDetach==================");
}
