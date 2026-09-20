#include "FoundryAudio.h"

#include "AudioMixerBlueprintLibrary.h"
#include "Components/AudioComponent.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "HAL/PlatformMisc.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/DateTime.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Sound/SoundConcurrency.h"
#include "Sound/SoundWave.h"

DEFINE_LOG_CATEGORY_STATIC(LogFoundryAudio, Log, All);

namespace
{
const TCHAR* const CueNames[] = {
    TEXT("cannon_light"), TEXT("cannon_heavy"), TEXT("metal_hit_light"), TEXT("metal_hit_heavy"),
    TEXT("robot_break"), TEXT("breech_load"), TEXT("breech_unload"), TEXT("forge"), TEXT("collect"),
    TEXT("shield_on"), TEXT("shield_hit"), TEXT("player_hit"), TEXT("robot_move"), TEXT("repair"),
    TEXT("upgrade"), TEXT("victory"), TEXT("defeat"), TEXT("precision_good"), TEXT("precision_miss"),
    TEXT("ui_ok"), TEXT("ui_error"), TEXT("turn"), TEXT("factory_room")
};

TStrongObjectPtr<USoundConcurrency> Concurrency(int32 Count)
{
    TStrongObjectPtr<USoundConcurrency> Result(NewObject<USoundConcurrency>());
    Result->Concurrency.MaxCount = Count;
    Result->Concurrency.ResolutionRule = EMaxConcurrentResolutionRule::StopOldest;
    Result->Concurrency.RetriggerTime = 0.025f;
    Result->Concurrency.VoiceStealReleaseTime = 0.012f;
    return Result;
}

float CueGain(FName Name)
{
    if (Name == TEXT("cannon_light") || Name == TEXT("cannon_heavy")) return 0.70f;
    if (Name == TEXT("robot_break")) return 0.80f;
    if (Name == TEXT("metal_hit_light") || Name == TEXT("metal_hit_heavy")) return 0.50f;
    return 0.65f;
}
}

FFoundryAudio::FFoundryAudio(UObject* InWorldContext) : WorldContext(InWorldContext)
{
    bEnabled = !FParse::Param(FCommandLine::Get(), TEXT("nosound"));
    bProbe = FParse::Param(FCommandLine::Get(), TEXT("FoundryAudioProbe"));
    if (bProbe)
    {
        // UE skips silent submix buffers even during output recording. For a
        // clocked diagnostic only, retain silence so each cue's capture has
        // its real timeline. Ordinary game auto-disable remains unchanged.
        if (IConsoleVariable* Flag=IConsoleManager::Get().FindConsoleVariable(TEXT("au.NeverDisableSubmixes")))
        {
            PreviousNeverDisableSubmixes=Flag->GetInt();
            Flag->Set(1,ECVF_SetByCode);
        }
    }
    ActionConcurrency = Concurrency(6);
    FireConcurrency = Concurrency(2);
    RoomConcurrency = Concurrency(1);
    for (const TCHAR* Name : CueNames)
    {
        const FString Path = FString::Printf(TEXT("/Game/FoundryAudio/S_%s.S_%s"), Name, Name);
        if (USoundWave* Wave = LoadObject<USoundWave>(nullptr, *Path))
            Waves.Add(FName(Name), TStrongObjectPtr<USoundWave>(Wave));
        else
            UE_LOG(LogFoundryAudio, Error, TEXT("Missing cue: %s. Regenerate/import FoundryAudio before packaging."), *Path);
    }
    UE_LOG(LogFoundryAudio, Display, TEXT("FOUNDRY_AUDIO_READY cues=%d expected=%d enabled=%d probe=%d"), Waves.Num(), UE_ARRAY_COUNT(CueNames), bEnabled, bProbe);
}

FFoundryAudio::~FFoundryAudio()
{
    StopAll();
    if (PreviousNeverDisableSubmixes >= 0)
        if (IConsoleVariable* Flag=IConsoleManager::Get().FindConsoleVariable(TEXT("au.NeverDisableSubmixes")))
            Flag->Set(PreviousNeverDisableSubmixes,ECVF_SetByCode);
}
bool FFoundryAudio::IsReady() const { return Waves.Num() == UE_ARRAY_COUNT(CueNames); }

void FFoundryAudio::StopAll()
{
    if (Room.IsValid()) Room->Stop();
    Room.Reset();
    for (const auto& Voice : Voices) if (Voice.IsValid()) Voice->Stop();
    Voices.Empty();
    LastCueTime.Empty();
    bActive = false;
}

void FFoundryAudio::SetVolume(float InVolume)
{
    const float Old = Volume;
    Volume = FMath::Clamp(InVolume, 0.0f, 1.0f);
    if (Room.IsValid()) Room->SetVolumeMultiplier(Volume);
    // A mute takes effect on active transients immediately. Later unmute starts
    // fresh cues, without replaying an event that happened while muted.
    if (Volume == 0 || Old == 0)
    {
        for (const auto& Voice : Voices) if (Voice.IsValid()) Voice->Stop();
        Voices.Empty();
    }
    else
        for (const auto& Voice : Voices) if (Voice.IsValid())
            Voice->SetVolumeMultiplier(Voice->VolumeMultiplier * Volume / Old);
}

void FFoundryAudio::SetActive(bool bInActive)
{
    if (bProbe || !bEnabled || !WorldContext.IsValid()) return;
    bActive = bInActive;
    if (!bActive)
    {
        StopAll();
        return;
    }
    if (Room.IsValid()) return;
    if (const auto* Wave = Waves.Find(TEXT("factory_room")))
        Room = UGameplayStatics::SpawnSound2D(WorldContext.Get(), Wave->Get(), Volume, 1.0f, 0, RoomConcurrency.Get());
}

void FFoundryAudio::PlayCue(FName Name, uint64 EventId)
{
    if (!bEnabled || !WorldContext.IsValid() || Volume <= 0) return;
    const auto* Wave = Waves.Find(Name);
    if (!Wave) return;
    const double Now = FPlatformTime::Seconds();
    if (const double* Last = LastCueTime.Find(Name); Last && Now - *Last < 0.035) return;
    LastCueTime.Add(Name, Now);
    const bool bFire = Name == TEXT("cannon_light") || Name == TEXT("cannon_heavy");
    const bool bRoom = Name == TEXT("factory_room");
    // Variation is a local cosmetic function of the immutable event ID. It
    // consumes none of the game's random streams and cannot alter a replay.
    const float Pitch = bRoom || EventId == 0 ? 1.0f : 0.985f + float(EventId % 7) * 0.005f;
    UAudioComponent* Voice = UGameplayStatics::SpawnSound2D(WorldContext.Get(), Wave->Get(),
        Volume * (bRoom ? 1.0f : CueGain(Name)), Pitch, 0,
        bRoom ? RoomConcurrency.Get() : (bFire ? FireConcurrency.Get() : ActionConcurrency.Get()));
    if (Voice)
    {
        if (bRoom) Room = Voice;
        else Voices.Add(Voice);
        ++PlayedCues;
        UE_LOG(LogFoundryAudio, Verbose, TEXT("cue=%s event=%llu"), *Name.ToString(), EventId);
    }
}

void FFoundryAudio::Present(const std::vector<overkill::Event>& Events)
{
    if (bProbe || !bEnabled) return;
    // Many pellets/robots can resolve together. One strongest impact, one rig
    // hit, and one break per committed batch keep important cues intelligible.
    TSet<FName> Used;
    overkill::Amount StrongestHit = 0;
    uint64 HitId = 0;
    auto Once = [&](FName Cue, uint64 Id) { if (!Used.Contains(Cue)) { Used.Add(Cue); PlayCue(Cue, Id); } };
    for (const auto& E : Events)
    {
        if (E.type == "fire") Once(E.amount >= 18 ? TEXT("cannon_heavy") : TEXT("cannon_light"), E.id);
        else if (E.type == "collected") Once(TEXT("collect"), E.id);
        else if (E.type == "loaded") Once(TEXT("breech_load"), E.id);
        else if (E.type == "unload") Once(TEXT("breech_unload"), E.id);
        else if (E.type == "recipe_used") Once(TEXT("forge"), E.id);
        else if (E.type == "first_install" || E.type == "reinstall") Once(TEXT("shield_on"), E.id);
        else if (E.type == "hit" || E.type == "status_damage")
        {
            if (E.amount > StrongestHit) { StrongestHit = E.amount; HitId = E.id; }
        }
        else if (E.type == "player_damage")
        {
            if (E.secondary > 0) Once(TEXT("shield_hit"), E.id);
            if (E.amount > 0) Once(TEXT("player_hit"), E.id);
        }
        else if (E.type == "enemy_death") Once(TEXT("robot_break"), E.id);
        else if (E.type == "enemy_action" || E.type.rfind("robot_action:", 0) == 0) Once(TEXT("robot_move"), E.id);
        else if (E.type == "heal" || E.type.rfind("upgrade_heal:", 0) == 0) Once(TEXT("repair"), E.id);
        else if (E.type == "upgrade_acquired") Once(TEXT("upgrade"), E.id);
        else if (E.type == "end_turn") Once(TEXT("turn"), E.id);
        else if (E.type == "victory" || E.type == "escape_complete") Once(TEXT("victory"), E.id);
        else if (E.type == "defeat") Once(TEXT("defeat"), E.id);
    }
    if (StrongestHit > 0 && !Used.Contains(TEXT("robot_break")))
        Once(StrongestHit >= 18 ? TEXT("metal_hit_heavy") : TEXT("metal_hit_light"), HitId);
}

void FFoundryAudio::Tick(float DeltaSeconds)
{
    Voices.RemoveAll([](const auto& Voice) { return !Voice.IsValid() || !Voice->IsPlaying(); });
    if (!bProbe) return;
    ProbeClock += DeltaSeconds;
    if (!bRecording && !bProbeFinished && ProbeClock > 0.8f)
    {
        StopAll();
        UAudioMixerBlueprintLibrary::StartRecordingOutput(WorldContext.Get(), 38.0f);
        bRecording = true;
        UE_LOG(LogFoundryAudio, Display, TEXT("FOUNDRY_AUDIO_PROBE_START enabled=%d ready=%d"), bEnabled, IsReady());
    }
    if (bRecording && ProbeStep < UE_ARRAY_COUNT(CueNames) && ProbeClock >= 1.0f + ProbeStep * 1.2f)
    {
        PlayCue(FName(CueNames[ProbeStep]));
        UE_LOG(LogFoundryAudio, Display, TEXT("FOUNDRY_AUDIO_PROBE_CUE index=%d name=%s"), ProbeStep, CueNames[ProbeStep]);
        ++ProbeStep;
    }
    if (bRecording && ProbeClock > 1.0f + (UE_ARRAY_COUNT(CueNames) - 1) * 1.2f + 6.1f)
    {
        StopAll();
        const FString Directory = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("AudioCapture"));
        IFileManager::Get().MakeDirectory(*Directory, true);
        const FString Name = TEXT("foundry-audio-") + FDateTime::Now().ToString(TEXT("%Y%m%d-%H%M%S"));
        UAudioMixerBlueprintLibrary::StopRecordingOutput(WorldContext.Get(), EAudioRecordingExportType::WavFile, Name, Directory);
        bRecording = false;
        bProbeFinished = true;
        UE_LOG(LogFoundryAudio, Display, TEXT("FOUNDRY_AUDIO_CAPTURE %s/%s.wav spawned=%d"), *Directory, *Name, PlayedCues);
    }
    if (bProbeFinished)
    {
        FinishedClock += DeltaSeconds;
        // Export runs asynchronously; leave several mixer frames and real
        // disk time before a bounded diagnostic exits. Validate the saved PCM.
        if (FinishedClock > 4.0f) FPlatformMisc::RequestExit(false);
    }
}
