#pragma once

#include "CoreMinimal.h"
#include "UObject/WeakObjectPtr.h"
#include "UObject/StrongObjectPtr.h"
#include "overkill/core.hpp"

class UAudioComponent;
class USoundWave;
class USoundConcurrency;

// A consumer of committed events only. Audio never owns clocks, randomness,
// costs or combat state. Optional mixer capture is isolated from game rules.
class FFoundryAudio
{
public:
    explicit FFoundryAudio(UObject* InWorldContext);
    ~FFoundryAudio();
    void Tick(float DeltaSeconds);
    void Present(const std::vector<overkill::Event>& Events);
    void SetActive(bool bActive);
    void SetVolume(float InVolume);
    void PlayCue(FName Name, uint64 EventId = 0);
    void StopAll();
    bool IsReady() const;
    int32 GetPlayedCueCount() const { return PlayedCues; }

private:
    TWeakObjectPtr<UObject> WorldContext;
    TMap<FName, TStrongObjectPtr<USoundWave>> Waves;
    TStrongObjectPtr<USoundConcurrency> ActionConcurrency, FireConcurrency, RoomConcurrency;
    TArray<TWeakObjectPtr<UAudioComponent>> Voices;
    TWeakObjectPtr<UAudioComponent> Room;
    TMap<FName, double> LastCueTime;
    float Volume = 0.55f;
    bool bEnabled = true, bActive = false, bProbe = false, bRecording = false, bProbeFinished = false;
    float ProbeClock = 0, FinishedClock = 0;
    int32 ProbeStep = 0, PlayedCues = 0;
    int32 PreviousNeverDisableSubmixes = -1;
};
