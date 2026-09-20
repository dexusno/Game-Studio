#include "FoundryHost.h"
#include "FoundryRobot.h"
#include "FoundryMara.h"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformMisc.h"

DEFINE_LOG_CATEGORY_STATIC(LogFoundryArtProbe, Log, All);

// Timed actions pass through the same session/core adapter as physical controls.
// This verifies rendering/event delivery, not human input handling or visual approval.
void AFoundryStage::TickArtProbe(float DeltaSeconds)
{
    ArtElapsed += DeltaSeconds;
    ReturnCameraAfter = 0; // Keep the probe framing stable while clips play.
    const auto Check = [&](bool bCondition, const TCHAR* Name)
    {
        if (!bCondition) bArtProbeOk = false;
        UE_LOG(LogFoundryArtProbe, Display, TEXT("ART_CHECK ok=%d name=%s"), bCondition, Name);
    };
    const auto Submit = [&](const overkill::Action& Action)
    {
        if (!Session->Submit(Action, TEXT("rendered art probe"))) bArtProbeOk = false;
        PresentCommittedEvents();
    };
    const auto Craft = [&](const std::string& Recipe)
    {
        for (const auto& Copy : Session->State.memory) if (Copy.recipe == Recipe) { Submit(overkill::Action::craft(Copy.id)); return; }
        Check(false, TEXT("probe recipe present"));
    };
    const auto Shoot = [&](uint64 Target)
    {
        std::vector<overkill::Id> Parts;
        for (const auto& Part : Session->State.parts)
            if (Part.place == overkill::Place::Reserve && Part.kind == overkill::Kind::Ammo) Parts.push_back(Part.id);
        Submit(overkill::Action::load(Parts));
        const auto BeforePreview = overkill::stateHash(Session->State);
        const auto Preview = Session->Preview(overkill::Action::fire(Target));
        Check(overkill::stateHash(Session->State) == BeforePreview, TEXT("preview leaves live state unchanged"));
        Submit(overkill::Action::fire(Target));
        Check(Preview.result.ok && overkill::stateHash(Preview.state) == overkill::stateHash(Session->State), TEXT("preview equals committed shot"));
    };
    const uint64 Mite = Session->State.enemies[0].id;
    const uint64 Ram = Session->State.enemies[1].id;
    // Exercise visible collection/load/unload before restoring the exact combat
    // fixture. These are legal actions, independent of the source/import probes.
    static const float MaraAt[] = {.30f, .79f, 1.60f, 1.80f, 2.06f, 2.80f, 3.04f, 3.50f};
    if (MaraProbeStep < UE_ARRAY_COUNT(MaraAt) && ArtElapsed >= MaraAt[MaraProbeStep])
    {
        switch (MaraProbeStep)
        {
        case 0: Submit(overkill::Action::collect(3)); break;
        case 1:
            Check(Mara->GetClawCue() == TEXT("collect") && Mara->HasPayload(), TEXT("rear claw grab from committed collection"));
            CaptureNamed(TEXT("mara-collect-grab.png")); break;
        case 2:
            Check(Mara->GetClawCue() == TEXT("collect") && !Mara->HasPayload(), TEXT("rear claw dump clears cosmetic payload"));
            CaptureNamed(TEXT("mara-collect-dump.png")); break;
        case 3:
        {
            Craft("SH001");
            std::vector<overkill::Id> Parts;
            for (const auto& Part : Session->State.parts) if (Part.place == overkill::Place::Reserve && Part.kind == overkill::Kind::Ammo) Parts.push_back(Part.id);
            Submit(overkill::Action::load(Parts)); break;
        }
        case 4:
            Check(Mara->GetGunCue() == TEXT("load"), TEXT("committed Load opens original breech rig"));
            SetActionView(true, true);
            CaptureNamed(TEXT("mara-load.png")); break;
        case 5:
        {
            overkill::Action Unload;
            Unload.type = overkill::ActionType::Unload;
            Submit(Unload); break;
        }
        case 6:
            Check(Mara->GetGunCue() == TEXT("unload"), TEXT("committed Unload reverses loading mechanism"));
            CaptureNamed(TEXT("mara-unload.png")); break;
        case 7:
            Control(TEXT("restart"));
            Session->bShowPanels = false;
            Check(Mara->IsReset(), TEXT("Mara restart clears cosmetic state")); break;
        }
        ++MaraProbeStep;
    }
    if (ArtElapsed >= 12.60f && !ArtCaptures.Contains(TEXT("art-ram-steam.png")))
    {
        ArtCaptures.Add(TEXT("art-ram-steam.png"));
        CaptureNamed(TEXT("art-ram-steam.png"));
    }
    // Capture both collapse and all-slot dissolve as they actually run, even if
    // a future core version changes which legal shot finishes the encounter.
    for (AFoundryRobot* Robot : Robots)
    {
        if (!IsValid(Robot) || Robot->GetCue() != TEXT("death")) continue;
        for (const auto& Sample : {TPair<float, FString>(.26f, TEXT("burst")), TPair<float, FString>(.90f, TEXT("collapse")), TPair<float, FString>(1.45f, TEXT("dissolve"))})
        {
            const FString Name = FString::Printf(TEXT("art-%llu-death-%s.png"), Robot->GetCoreId(), *Sample.Value);
            if (Robot->GetCueElapsed() >= Sample.Key && !ArtCaptures.Contains(Name))
            {
                ArtCaptures.Add(Name);
                CaptureNamed(Name);
                break;
            }
        }
    }
    static const float At[] = {4, 5, 5.14f, 6, 6.16f, 7, 7.22f, 8, 8.36f, 9.6f, 10, 10.22f, 10.9f, 11.15f, 12, 12.34f, 14, 16.2f, 17, 19, 21.3f, 22.3f, 23.3f, 24.3f};
    if (ArtStep >= UE_ARRAY_COUNT(At) || ArtElapsed < At[ArtStep]) return;
    switch (ArtStep)
    {
    case 0: CaptureNamed(TEXT("art-preparation.png")); break;
    case 1:
        SetActionView(true, true);
        Submit(overkill::Action::collect(3)); Craft("SH003"); Shoot(Ram); break;
    case 2: CaptureNamed(TEXT("art-deflection.png")); break;
    case 3: Craft("SH004"); Shoot(Ram); break;
    case 4:
        Check(Mara->GetGunCue() == TEXT("fire"), TEXT("committed shot plays original gun recoil"));
        CaptureNamed(TEXT("art-hit-light.png")); break;
    case 5: Craft("SH001"); Shoot(Ram); break;
    case 6: CaptureNamed(TEXT("art-hit-medium.png")); break;
    case 7: Submit(overkill::Action::endTurn()); break;
    case 8: CaptureNamed(TEXT("art-mite-attack-ram-charge.png")); break;
    case 9:
        for (AFoundryRobot* Robot : Robots) if (IsValid(Robot) && Robot->GetCoreId() == Ram)
            Check(Robot->IsCharged() && Robot->GetCue() == TEXT("charge_hold"), TEXT("Ram holds final charged pose"));
        CaptureNamed(TEXT("art-charge-held.png")); break;
    case 10: Submit(overkill::Action::collect(3)); Craft("SH004"); Shoot(Ram); break;
    case 11: CaptureNamed(TEXT("art-hit-heavy.png")); break;
    case 12: Craft("SH003"); Shoot(Mite); break;
    case 13: CaptureNamed(TEXT("art-mite-hit-heavy.png")); break;
    case 14: Submit(overkill::Action::endTurn()); break;
    case 15: CaptureNamed(TEXT("art-ram-blast.png")); break;
    case 16: Submit(overkill::Action::collect(3)); Craft("SH004"); Shoot(Mite); break;
    case 17:
        Check(Session->State.enemies[0].dead && !IsValid(Robots[0]), TEXT("Mite core death and actor cleanup"));
        Craft("SH001"); Craft("SH003"); Shoot(Ram); break;
    case 18:
        if (Session->State.phase == overkill::Phase::Preparation) Submit(overkill::Action::endTurn());
        break;
    case 19:
        if (Session->State.phase == overkill::Phase::Collection)
        {
            Submit(overkill::Action::collect(3)); Craft("SH004"); Craft("SH001"); Shoot(Ram);
        }
        break;
    case 20:
    {
        int32 Remaining = 0;
        for (TActorIterator<AFoundryRobot> Robot(GetWorld()); Robot; ++Robot) if (IsValid(*Robot)) ++Remaining;
        Check(Session->State.phase == overkill::Phase::Victory && Remaining == 0, TEXT("both core deaths; zero robot actors or attached presentation remains"));
        CaptureNamed(TEXT("art-cleanup.png"));
        UE_LOG(LogFoundryArtProbe, Display, TEXT("ART_FINAL hash=%s hp=%d round=%d shots=%d actors=%d"), UTF8_TO_TCHAR(overkill::stateHash(Session->State).c_str()), Session->State.hp, Session->State.round, Session->State.shots, Remaining);
        break;
    }
    case 21:
        Control(TEXT("restart"));
        Session->bShowPanels = true;
        Check(Robots.Num() == 2 && IsValid(Robots[0]) && IsValid(Robots[1]) && Robots[0]->AreMaterialsSolid() && Robots[1]->AreMaterialsSolid(), TEXT("restart recreates both complete robot actors with all 15 solid slots"));
        Check(Mara->IsReset(), TEXT("Mara final restart clears payload and muzzle effects"));
        break;
    case 22:
        CaptureNamed(TEXT("art-restart-hud.png"));
        break;
    case 23:
        UE_LOG(LogFoundryArtProbe, Display, TEXT("FOUNDRY_ART_PROBE_COMPLETE ok=%d snapshots=%d actual_core_actions=1"), bArtProbeOk, RequestedCaptures.Num());
        FPlatformMisc::RequestExit(false);
        break;
    }
    ++ArtStep;
}
