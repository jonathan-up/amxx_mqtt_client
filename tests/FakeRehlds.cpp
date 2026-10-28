#include "sdk/rehlds.h"
#include "FakeRehlds.h"
#include <cstring>

namespace {
FakeRehldsMode mode = Compatible;
bool paused = false;
int physicsFrames = 0;
int serverFrames = 0;

class FrameChain : public IRehldsHook_SV_Frame {
public:
    void callNext() override { callOriginal(); }
    void callOriginal() override {
        ++serverFrames;
        // Mirrors ReHLDS: SV_Frame runs during pause, SV_Physics does not.
        if (!paused) {
            ++physicsFrames;
        }
    }
};

class FrameRegistry : public IRehldsHookRegistry_SV_Frame {
public:
    hookfunc_t callback = nullptr;
    int count = 0;
    void registerHook(hookfunc_t hook, int) override {
        callback = hook;
        ++count;
    }
    void unregisterHook(hookfunc_t hook) override {
        if (callback == hook) {
            callback = nullptr;
            --count;
        }
    }
} frameRegistry;

class Hookchains : public IRehldsHookchains {
public:
    IRehldsHookRegistry_SV_Frame *SV_Frame() override {
        return mode == NoFrameRegistry ? nullptr : &frameRegistry;
    }

    // The remaining API slots are deliberately unused by this module.
#define UNUSED_HOOK(name) IRehldsHookRegistry_##name *name() override { return nullptr; }
    UNUSED_HOOK(Steam_NotifyClientConnect)
    UNUSED_HOOK(SV_ConnectClient)
    UNUSED_HOOK(SV_GetIDString)
    UNUSED_HOOK(SV_SendServerinfo)
    UNUSED_HOOK(SV_CheckProtocol)
    UNUSED_HOOK(SVC_GetChallenge_mod)
    UNUSED_HOOK(SV_CheckKeyInfo)
    UNUSED_HOOK(SV_CheckIPRestrictions)
    UNUSED_HOOK(SV_FinishCertificateCheck)
    UNUSED_HOOK(Steam_NotifyBotConnect)
    UNUSED_HOOK(SerializeSteamId)
    UNUSED_HOOK(SV_CompareUserID)
    UNUSED_HOOK(Steam_NotifyClientDisconnect)
    UNUSED_HOOK(PreprocessPacket)
    UNUSED_HOOK(ValidateCommand)
    UNUSED_HOOK(ClientConnected)
    UNUSED_HOOK(HandleNetCommand)
    UNUSED_HOOK(Mod_LoadBrushModel)
    UNUSED_HOOK(Mod_LoadStudioModel)
    UNUSED_HOOK(ExecuteServerStringCmd)
    UNUSED_HOOK(SV_EmitEvents)
    UNUSED_HOOK(EV_PlayReliableEvent)
    UNUSED_HOOK(SV_StartSound)
    UNUSED_HOOK(PF_Remove_I)
    UNUSED_HOOK(PF_BuildSoundMsg_I)
    UNUSED_HOOK(SV_WriteFullClientUpdate)
    UNUSED_HOOK(SV_CheckConsistencyResponse)
    UNUSED_HOOK(SV_DropClient)
    UNUSED_HOOK(SV_ActivateServer)
    UNUSED_HOOK(SV_WriteVoiceCodec)
    UNUSED_HOOK(Steam_GSGetSteamID)
    UNUSED_HOOK(SV_TransferConsistencyInfo)
    UNUSED_HOOK(Steam_GSBUpdateUserData)
    UNUSED_HOOK(Cvar_DirectSet)
    UNUSED_HOOK(SV_EstablishTimeBase)
    UNUSED_HOOK(SV_Spawn_f)
    UNUSED_HOOK(SV_CreatePacketEntities)
    UNUSED_HOOK(SV_EmitSound2)
    UNUSED_HOOK(CreateFakeClient)
    UNUSED_HOOK(SV_CheckConnectionLessRateLimits)
    UNUSED_HOOK(SV_ShouldSendConsistencyList)
    UNUSED_HOOK(GetEntityInit)
    UNUSED_HOOK(SV_EmitPings)
    UNUSED_HOOK(ED_Alloc)
    UNUSED_HOOK(ED_Free)
    UNUSED_HOOK(Con_Printf)
    UNUSED_HOOK(SV_CheckUserInfo)
    UNUSED_HOOK(PF_precache_generic_I)
    UNUSED_HOOK(PF_precache_model_I)
    UNUSED_HOOK(PF_precache_sound_I)
    UNUSED_HOOK(EV_Precache)
    UNUSED_HOOK(SV_AddResource)
    UNUSED_HOOK(SV_ClientPrintf)
    UNUSED_HOOK(SV_AllowPhysent)
    UNUSED_HOOK(SV_SendResources)
#undef UNUSED_HOOK
} hookchains;

class RehldsApi : public IRehldsApi {
public:
    int GetMajorVersion() override { return mode == WrongMajor ? 4 : 3; }
    int GetMinorVersion() override { return mode == OldMinor ? 6 : 7; }
    const RehldsFuncs_t *GetFuncs() override { return nullptr; }
    IRehldsHookchains *GetHookchains() override { return mode == NoHookchains ? nullptr : &hookchains; }
    IRehldsServerStatic *GetServerStatic() override { return nullptr; }
    IRehldsServerData *GetServerData() override { return nullptr; }
    IRehldsFlightRecorder *GetFlightRecorder() override { return nullptr; }
    IMessageManager *GetMessageManager() override { return nullptr; }
} api;
} // namespace

extern "C" EXPORT_FUNCTION IBaseInterface *CreateInterface(const char *name, int *code) {
    const bool found = mode != NoInterface && std::strcmp(name, VREHLDS_HLDS_API_VERSION) == 0;
    if (code) {
        *code = found ? IFACE_OK : IFACE_FAILED;
    }
    return found ? reinterpret_cast<IBaseInterface *>(&api) : nullptr;
}

void FakeEngineSetMode(FakeRehldsMode value) { mode = value; }
void FakeEngineSetPaused(bool value) { paused = value; }
int FakeEnginePhysicsFrames() { return physicsFrames; }
int FakeEngineServerFrames() { return serverFrames; }
int FakeEngineHookCount() { return frameRegistry.count; }
void FakeEngineFrame() {
    FrameChain chain;
    if (frameRegistry.callback) {
        frameRegistry.callback(&chain);
    } else {
        chain.callNext();
    }
}
