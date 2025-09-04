//
// Created by jonathan on 9/2/25.
//

#include "sdk/amxxmodule.h"
#include "natives.h"

void OnAmxxAttach() {
    MF_AddNatives(g_natives);
    MF_Log("==================OnAmxxAttach==================");
}

void OnAmxxDetach() {
    MF_Log("==================OnAmxxDetach==================");
}
