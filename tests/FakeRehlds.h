#ifndef AMXX_MQTT_FAKE_REHLDS_H
#define AMXX_MQTT_FAKE_REHLDS_H

#ifdef _WIN32
#ifdef FAKE_REHLDS_EXPORTS
#define FAKE_ENGINE_API extern "C" __declspec(dllexport)
#else
#define FAKE_ENGINE_API extern "C" __declspec(dllimport)
#endif
#else
#define FAKE_ENGINE_API extern "C" __attribute__((visibility("default")))
#endif

enum FakeRehldsMode { Compatible, NoInterface, WrongMajor, OldMinor, NoHookchains, NoFrameRegistry };
FAKE_ENGINE_API void FakeEngineSetMode(FakeRehldsMode mode);
FAKE_ENGINE_API void FakeEngineSetPaused(bool paused);
FAKE_ENGINE_API void FakeEngineFrame();
FAKE_ENGINE_API int FakeEnginePhysicsFrames();
FAKE_ENGINE_API int FakeEngineServerFrames();
FAKE_ENGINE_API int FakeEngineHookCount();

#endif
