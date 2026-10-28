#ifndef AMXX_MQTT_REHLDS_FRAME_HOOK_H
#define AMXX_MQTT_REHLDS_FRAME_HOOK_H

#include <string>

// Called only by the engine/AMXX main thread.
bool CheckRehldsApi(std::string &error);
bool RegisterRehldsFrameHook(std::string &error);
void UnregisterRehldsFrameHook();

#endif
