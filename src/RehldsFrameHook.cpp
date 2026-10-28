#include "RehldsFrameHook.h"
#include "sdk/rehlds.h"
#include "CallbackDispatcher.h"

namespace {
// SV_Frame was added in ReHLDS API 3.7. Later 3.x APIs append compatible entries.
constexpr int minimumApiMinor = 7;
static_assert(REHLDS_API_VERSION_MAJOR == 3 && REHLDS_API_VERSION_MINOR >= minimumApiMinor);
IRehldsHookRegistry_SV_Frame *frameRegistry = nullptr;

CreateInterfaceFn FindEngineFactory() {
#ifdef _WIN32
    // Same dedicated/listen-server candidates used by ReAPI.
    for (const char *name : {"swds.dll", "sw.dll", "hw.dll"}) {
        if (const HMODULE engine = GetModuleHandleA(name)) {
            if (const auto factory = reinterpret_cast<CreateInterfaceFn>(
                    GetProcAddress(engine, CREATEINTERFACE_PROCNAME))) {
                return factory;
            }
        }
    }
#else
    // Only inspect the already loaded engine. Never load another engine copy.
    for (const char *name : {"engine_i486.so", "./engine_i486.so"}) {
        if (void *engine = dlopen(name, RTLD_NOW | RTLD_NOLOAD)) {
            const auto factory = reinterpret_cast<CreateInterfaceFn>(
                dlsym(engine, CREATEINTERFACE_PROCNAME));
            dlclose(engine); // Balance our lookup reference; the host owns the engine.
            if (factory) {
                return factory;
            }
        }
    }
#endif
    return nullptr;
}

IRehldsHookRegistry_SV_Frame *FindFrameRegistry(std::string &error) {
    error.clear();
    const auto factory = FindEngineFactory();
    if (!factory) {
        error = "ReHLDS API 3.7+ is required: engine CreateInterface was not found.";
        return nullptr;
    }

    auto *api = reinterpret_cast<IRehldsApi *>(factory(VREHLDS_HLDS_API_VERSION, nullptr));
    if (!api) {
        error = "ReHLDS API 3.7+ is required: VREHLDS_HLDS_API_VERSION001 is unavailable.";
        return nullptr;
    }

    const int major = api->GetMajorVersion();
    const int minor = api->GetMinorVersion();
    if (major != REHLDS_API_VERSION_MAJOR || minor < minimumApiMinor) {
        error = "ReHLDS API version mismatch: need 3.7+, found "
            + std::to_string(major) + "." + std::to_string(minor) + ".";
        return nullptr;
    }

    auto *hooks = api->GetHookchains();
    auto *registry = hooks ? hooks->SV_Frame() : nullptr;
    if (!registry) {
        error = "ReHLDS API does not provide the SV_Frame hook registry.";
    }
    return registry;
}

void OnServerFrame(IRehldsHook_SV_Frame *chain) {
    chain->callNext();
    // SV_Frame keeps running during game pause, unlike the GameDLL StartFrame.
    ProcessPendingCallbacks();
}
} // namespace

bool CheckRehldsApi(std::string &error) {
    return FindFrameRegistry(error) != nullptr;
}

bool RegisterRehldsFrameHook(std::string &error) {
    error.clear();
    if (frameRegistry) {
        return true;
    }
    frameRegistry = FindFrameRegistry(error);
    if (!frameRegistry) {
        return false;
    }
    frameRegistry->registerHook(OnServerFrame);
    return true;
}

void UnregisterRehldsFrameHook() {
    if (frameRegistry) {
        frameRegistry->unregisterHook(OnServerFrame);
        frameRegistry = nullptr;
    }
}
