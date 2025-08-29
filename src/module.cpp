//
// Created by jonathan on 2/25/25.
//

#include "sdk/amxxmodule.h"
#include "natives.h"

void OnAmxxAttach() {
    MF_AddNatives(g_natives);
}

void OnAmxxDetach() {
}
