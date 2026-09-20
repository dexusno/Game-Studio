#include "FoundryHost.h"
#include "FoundryRobot.h"
#include "FoundryMara.h"
#include "overkill/robots.hpp"

#include "Engine/World.h"
#include "EngineUtils.h"
#include "HAL/PlatformMisc.h"

DEFINE_LOG_CATEGORY_STATIC(LogFoundryRosterProbe, Log, All);

namespace
{
struct FRosterCase
{
    const char* Formation;
    const char* Definition;
    const TCHAR* Name;
    int32 Cursor = 0;
    int32 Mode = 0; // 0 action; 1-3 reactions; 4 death; 5 escape; 6 tiles; 7-8 action then death; 9 cancelled summon; 10 Utility kill.
    bool bBossPhaseTwo = false;
    bool bBossRecover = false;
};

const TArray<FRosterCase>& RosterCases()
{
    static const TArray<FRosterCase> Cases = []
    {
        TArray<FRosterCase> Out = {
            {"C1-F-MITE-RAM", "C1-R01", TEXT("attack")},
            {"C1-F-RAM", "C1-R02", TEXT("charge")},
            {"C1-F-RAM", "C1-R02", TEXT("blast"), 1},
            {"C1-F-BINDER", "C1-R04", TEXT("spray")},
            {"C1-F-BINDER", "C1-R04", TEXT("foul"), 1},
            {"C1-F-BINDER", "C1-R04", TEXT("punch"), 2},
            {"C1-F-CASK", "C1-R05", TEXT("attack")},
            {"C1-F-CASK", "C1-R05", TEXT("pressurize"), 1},
            {"C1-F-NEST", "C1-R07", TEXT("deploy")},
            {"C1-F-NEST", "C1-R07", TEXT("attack"), 1},
            {"C1-F-PRESS", "C1-R08", TEXT("brace")},
            {"C1-F-PRESS", "C1-R08", TEXT("double_punch"), 1},
            {"C1-F-PRESS", "C1-R08", TEXT("slam"), 2},
            {"C1-F-PURSUER", "C1-O01", TEXT("strike")},
            {"C1-F-PURSUER", "C1-O01", TEXT("triple_strike"), 1},
            {"C1-F-WARDEN", "C1-O02", TEXT("attack_9")},
            {"C1-F-WARDEN", "C1-O02", TEXT("gain_drive"), 1},
            {"C1-F-WARDEN", "C1-O02", TEXT("attack_12"), 2},
            {"C1-F-CHASSIS", "C1-O03", TEXT("double_strike")},
            {"C1-F-CHASSIS", "C1-O03", TEXT("thermal_runaway"), 1},
            {"C1-F-CHASSIS", "C1-O03", TEXT("triple_strike"), 2},
            {"C1-F-GATEBREAKER", "C1-B01", TEXT("phase_one_strike")},
            {"C1-F-GATEBREAKER", "C1-B01", TEXT("recover"), 0, 0, true, true},
            {"C1-F-GATEBREAKER", "C1-B01", TEXT("quad_strike"), 0, 0, true},
            {"C1-F-GATEBREAKER", "C1-B01", TEXT("charge"), 1, 0, true},
            {"C1-F-GATEBREAKER", "C1-B01", TEXT("blast"), 2, 0, true}
        };
        const FRosterCase Bodies[] = {
            {"C1-F-MITE-RAM", "C1-R01", TEXT("")}, {"C1-F-RAM", "C1-R02", TEXT("")},
            {"C1-F-BINDER", "C1-R04", TEXT("")}, {"C1-F-CASK", "C1-R05", TEXT("")},
            {"C1-F-NEST", "C1-R07", TEXT("")}, {"C1-F-PRESS", "C1-R08", TEXT("")},
            {"C1-F-PURSUER", "C1-O01", TEXT("")}, {"C1-F-WARDEN", "C1-O02", TEXT("")},
            {"C1-F-CHASSIS", "C1-O03", TEXT("")}, {"C1-F-GATEBREAKER", "C1-B01", TEXT("")}
        };
        const TCHAR* Names[] = {TEXT(""), TEXT("hit_light"), TEXT("hit_medium"), TEXT("hit_heavy"), TEXT("death"), TEXT("escape")};
        for (const auto& Body : Bodies)
            for (int32 Mode = 1; Mode <= 5; ++Mode)
            {
                // With 7 HP, a positive integer loss cannot be below 5%.
                if (FString(Body.Definition) == TEXT("C1-R01") && Mode == 1) continue;
                auto Entry = Body; Entry.Name = Names[Mode]; Entry.Mode = Mode; Out.Add(Entry);
            }
        Out.Add({"C1-F-WARDEN", "C1-O02", TEXT("tiles_3_to_0"), 0, 6});
        Out.Add({"C1-F-CASK", "C1-R05", TEXT("attack_then_burn_death"), 0, 7});
        Out.Add({"C1-F-BINDER", "C1-R04", TEXT("one_spray_hit_then_counter_death"), 0, 8});
        Out.Add({"C1-F-NEST", "C1-R07", TEXT("cancelled_summon_reconciles"), 0, 9});
        Out.Add({"C1-F-CASK", "C1-R05", TEXT("utility_kill_holds_result"), 0, 10});
        return Out;
    }();
    return Cases;
}
}

// Controlled formation/phase/cursor and plain-ammunition fixtures isolate
// presentation. They are not naturally earned builds or city-balance evidence.
// Every displayed damage/death/escape/summon comes from Rules::apply. Animation
// must leave the already committed state hash unchanged throughout each case.
void AFoundryStage::TickRosterProbe(float DeltaSeconds)
{
    RosterElapsed += DeltaSeconds;
    const auto Check = [&](bool bCondition, const TCHAR* Name)
    {
        if (!bCondition) bRosterProbeOk = false;
        UE_LOG(LogFoundryRosterProbe, Display, TEXT("ROSTER_CHECK ok=%d case=%d name=%s"), bCondition, RosterCase, Name);
    };
    const auto& Cases = RosterCases();
    if (RosterCase >= Cases.Num())
    {
        for (AFoundryRobot* Robot : Robots) if (IsValid(Robot)) Robot->Destroy();
        Robots.Empty();
        UE_LOG(LogFoundryRosterProbe, Display, TEXT("FOUNDRY_ROSTER_PROBE_COMPLETE ok=%d cases=%d snapshots=%d controlled_fixtures=1 natural_campaign=0"), bRosterProbeOk, Cases.Num(), RequestedCaptures.Num());
        FPlatformMisc::RequestExit(false);
        return;
    }
    const FRosterCase& Case = Cases[RosterCase];
    if (Case.Mode != 7 && Case.Mode != 8 && Case.Mode != 10) ReturnCameraAfter = CameraTransitionRemaining = 0;
    const auto Focus = [&]() -> AFoundryRobot*
    {
        for (AFoundryRobot* Robot : Robots) if (IsValid(Robot) && Robot->GetDefinition() == UTF8_TO_TCHAR(Case.Definition)) return Robot;
        return nullptr;
    };
    const auto Capture = [&](const TCHAR* Suffix)
    {
        CaptureNamed(FString::Printf(TEXT("roster-%02d-%s-%s-%s.png"), RosterCase, UTF8_TO_TCHAR(Case.Definition), Case.Name, Suffix));
    };
    const auto Submit = [&](const overkill::Action& Action)
    {
        const auto Before = overkill::stateHash(Session->State);
        const auto Preview = Session->Preview(Action);
        Check(overkill::stateHash(Session->State) == Before, TEXT("preview is read only"));
        Check(Session->Submit(Action, TEXT("controlled roster probe")), TEXT("actual core command accepted"));
        Check(Preview.result.ok && overkill::stateHash(Preview.state) == overkill::stateHash(Session->State), TEXT("preview equals committed state"));
    };
    const auto Shoot = [&](int32 Damage)
    {
        overkill::Part Ammo;
        Ammo.id = Session->State.nextId++;
        Ammo.output = "Controlled presentation-test projectile";
        Ammo.effects.push_back({overkill::Op::FlatDamage, overkill::Timing::Assembly, Damage, 0});
        Session->State.parts.push_back(Ammo);
        Submit(overkill::Action::load({Ammo.id}));
        Submit(overkill::Action::fire(Session->Target));
    };
    if (RosterStep == 0)
    {
        Session->State = overkill::State{};
        Session->State.seed = 991;
        Session->State.encounter = "controlled-roster-probe";
        Session->State.rng = overkill::Rng::seeded(991, Session->State.encounter);
        Session->State.enemies = overkill::makeCinderwallFormation(Case.Formation, 991, Session->State.encounter, Session->State.nextId);
        auto& Enemy = Session->State.enemies.front();
        Enemy.patternCursor = Case.Cursor;
        Enemy.bossTransitioned = Case.bBossPhaseTwo;
        Enemy.recoveryPending = Case.bBossRecover;
        Enemy.intentRound = 0;
        if (Case.Mode >= 1 && Case.Mode <= 4) Enemy.tiles = 0;
        if (Case.Mode == 5) Enemy.departureRound = Session->State.round;
        if (Case.Mode == 7) { Enemy.hp = 1; Enemy.burn = 1; }
        if (Case.Mode == 8) Enemy.hp = 2;
        if (Case.Mode == 10)
        {
            Enemy.hp = 1;
            Session->State.heat = 5;
            Session->State.memory.push_back({Session->State.nextId++, "MA058", 0, 0, 0});
        }
        overkill::commitRobotIntent(Session->State, Enemy.id, {});
        Session->Target = Enemy.id;
        Session->Selection.clear();
        Session->CommittedEvents.clear();
        ResetActionPresentation(); Mara->ResetPresentation();
        SpawnRobots(); SetActionView(Case.Mode != 10, true); Session->bShowPanels = false;
        Check(Focus() && Focus()->AreMaterialsSolid(), TEXT("distinct original mesh with every material solid"));
        Check(Focus() && Focus()->HasAction(Case.Mode >= 6 ? TEXT("idle") : Case.Name), TEXT("exact authored action clip exists"));
        if (Case.Mode == 8)
            Check(Session->Rules.acquireUpgrade(Session->State, "UGS-019").ok, TEXT("controlled Counterpulse upgrade acquired through core"));
        UE_LOG(LogFoundryRosterProbe, Display, TEXT("ROSTER_CASE index=%d definition=%s visual=%s action=%s mode=%d controlled_cursor=%d max_hp=%d"),
            RosterCase, UTF8_TO_TCHAR(Case.Definition), Focus() ? *Focus()->GetVisualAssetName() : TEXT("MISSING"), Case.Name, Case.Mode, Case.Cursor, Enemy.maxHp);
        RosterElapsed = 0; RosterStep = 1; return;
    }
    if (RosterStep == 1 && RosterElapsed >= .45f)
    {
        Capture(TEXT("ready"));
        Submit(overkill::Action::collect(3));
        RosterExpectedHits = 0;
        if (Case.Mode == 10)
        {
            const uint64 CopyId = Session->State.memory.front().id;
            const auto Preview = Session->Preview(overkill::Action::craft(CopyId));
            Control(FString::Printf(TEXT("craft:%llu"), CopyId));
            Check(Preview.result.ok && overkill::stateHash(Preview.state) == overkill::stateHash(Session->State), TEXT("real Utility Use equals preview"));
            Check(Session->State.phase == overkill::Phase::Victory && Focus() && Focus()->IsTerminal(), TEXT("real Open Exhaust causes committed victory and death"));
            Check(!IsActionView() && IsPresentationBusy(), TEXT("Utility kill retains preparation view and holds results/input for death"));
        }
        else if (Case.Mode == 0 || Case.Mode == 5 || Case.Mode >= 7)
        {
            if (Case.Mode == 0) Check(UTF8_TO_TCHAR(Session->State.enemies.front().robotAction.c_str()) == FString(Case.Name), TEXT("core performs intended authored action"));
            if (Case.Mode == 7 || Case.Mode == 8)
            {
                const auto Preview = Session->Preview(overkill::Action::endTurn());
                for (const auto& Event : Preview.result.events) if (Event.type == "player_damage" && Event.subject == Session->Target) ++RosterExpectedHits;
                Control(TEXT("end"));
                Check(Preview.result.ok && overkill::stateHash(Preview.state) == overkill::stateHash(Session->State), TEXT("real End Turn control equals preview"));
                Check(IsActionView() && IsPresentationBusy(), TEXT("real End Turn control begins camera/input lock"));
            }
            else Submit(overkill::Action::endTurn());
        }
        else
        {
            const auto& Enemy = Session->State.enemies.front();
            const int32 Loss = Case.Mode == 1 ? 1 : Case.Mode == 2 ? FMath::CeilToInt(Enemy.maxHp * .10f) : Case.Mode == 3 ? FMath::CeilToInt(Enemy.maxHp * .25f) : Case.Mode == 4 ? Enemy.maxHp : 10;
            Shoot(Loss + Enemy.armor);
        }
        for (const auto& Event : Session->CommittedEvents) if (Event.type == "player_damage" && Event.subject == Session->Target) ++RosterExpectedHits;
        if (Case.Mode == 9)
        {
            // The same cancellation path used when a title overlay interrupts
            // the camera blend; no synthetic summon event is fabricated.
            QueuedEvents = std::move(Session->CommittedEvents);
            Session->CommittedEvents.clear();
            int32 Deployments = 0;
            for (const auto& Event : QueuedEvents) if (Event.type == "robot_deployed") ++Deployments;
            Check(Deployments == 1 && Session->State.enemies.size() == 2, TEXT("real deploy exists before cancellation"));
            SpawnMissingRobots();
            int32 Helpers = 0;
            for (const AFoundryRobot* Robot : Robots) if (IsValid(Robot) && Robot->GetCoreId() != Session->State.enemies.front().id)
            {
                ++Helpers;
                Check(Robot->IsHidden(), TEXT("queued deployment initially awaits camera/animation"));
            }
            Check(Helpers == 1, TEXT("one real helper exists before cancellation"));
            const auto BeforeReset = overkill::stateHash(Session->State);
            for (AFoundryRobot* Robot : Robots) if (IsValid(Robot)) Robot->SetSceneHidden(true);
            ResetActionPresentation();
            Check(BeforeReset == overkill::stateHash(Session->State), TEXT("cosmetic cancellation preserves committed state"));
            for (AFoundryRobot* Robot : Robots) if (IsValid(Robot))
            {
                Check(Robot->IsHidden(), TEXT("title overlay still hides reconciled actors"));
                Robot->SetSceneHidden(false);
                Check(!Robot->IsHidden(), TEXT("in-memory resume shows all committed actors"));
            }
        }
        else PresentCommittedEvents();
        RosterExpectedHash = UTF8_TO_TCHAR(overkill::stateHash(Session->State).c_str());
        if (Case.Mode >= 1 && Case.Mode <= 3) Check(Focus() && Focus()->GetCue() == Case.Name, TEXT("relative hit band uses actual resolved HP loss"));
        if (Case.Mode == 4) Check(Focus() && Focus()->IsTerminal(), TEXT("committed death owns terminal animation"));
        if (Case.Mode == 6) Check(Focus() && Focus()->GetVisibleTileCount() == 2, TEXT("first paid shot removes one rendered plate"));
        if (Session->State.enemies.size() > 1 && FString(Case.Definition) != TEXT("C1-R01") && Case.Mode != 9)
            for (const AFoundryRobot* Robot : Robots) if (IsValid(Robot) && Robot->GetCoreId() != Session->State.enemies.front().id)
                Check(Robot->IsHidden(), TEXT("committed helper waits for authored release cue"));
        RosterElapsed = 0; RosterStep = 2; return;
    }
    if (RosterStep == 2 && RosterElapsed >= .18f)
    {
        Capture(TEXT("early")); RosterStep = 3;
    }
    if (RosterStep == 3 && RosterElapsed >= .72f)
    {
        Capture(TEXT("action"));
        if (Case.Mode == 7 || Case.Mode == 8)
        {
            Check(Focus() && Focus()->IsTerminal() && Focus()->GetPresentedHitCount() == 1, TEXT("actual committed final hit appears before queued death"));
            Check(RosterExpectedHits == 1, TEXT("core emitted exactly one hit before death"));
        }
        if (Case.Mode == 6)
        {
            Shoot(10); PresentCommittedEvents();
            Check(Focus() && Focus()->GetVisibleTileCount() == 1, TEXT("second paid shot leaves one rendered plate"));
            Shoot(10); PresentCommittedEvents();
            Check(Focus() && Focus()->GetVisibleTileCount() == 0, TEXT("third paid shot exhausts rendered plates"));
            RosterExpectedHash = UTF8_TO_TCHAR(overkill::stateHash(Session->State).c_str());
        }
        RosterStep = 4;
    }
    if (RosterStep == 4 && RosterElapsed >= 1.40f)
    {
        if (Case.Mode == 4 || Case.Mode == 6 || Case.Mode == 7 || Case.Mode == 8) Capture(TEXT("late"));
        if (Case.Mode == 10) Check(IsPresentationBusy() && Focus(), TEXT("Utility result remains held during actual fade"));
        RosterStep = 5;
    }
    if (RosterStep == 5 && (Case.Mode == 7 || Case.Mode == 8) && RosterElapsed >= 2.4f)
    {
        Check(IsActionView() && IsPresentationBusy() && Focus(), TEXT("camera/input lock survives ordinary timer while terminal clip remains"));
        Capture(TEXT("terminal-lock"));
        RosterStep = 6;
    }
    if ((RosterStep == 5 || RosterStep == 6) && RosterElapsed >= ((Case.Mode == 7 || Case.Mode == 8) ? 4.f : Case.Mode == 10 ? 2.6f : 2.25f))
    {
        Check(RosterExpectedHash == UTF8_TO_TCHAR(overkill::stateHash(Session->State).c_str()), TEXT("all animation leaves committed core state unchanged"));
        if (Case.Mode == 4 || Case.Mode == 5 || Case.Mode == 7 || Case.Mode == 8 || Case.Mode == 10)
            Check(!Focus(), TEXT("terminal robot and its owned components are destroyed"));
        else
        {
            Check(Focus() && Focus()->GetPresentedHitCount() == RosterExpectedHits, TEXT("each committed hit is displayed once and no extra hits appear"));
            Check(Focus() && Focus()->AreMaterialsSolid(), TEXT("survivor materials remain solid"));
            if (FString(Case.Name) == TEXT("charge")) Check(Focus() && Focus()->IsCharged() && Focus()->GetCue() == TEXT("charge_hold"), TEXT("charge holds final pose until next action"));
            if (FString(Case.Name) == TEXT("deploy")) Check(Focus() && Focus()->GetPresentedSummonCount() == 1, TEXT("one committed deployment cue"));
        }
        if (Case.Mode == 7 || Case.Mode == 8 || Case.Mode == 10)
            Check(!IsActionView() && !IsPresentationBusy(), TEXT("real camera/input lock releases after terminal cleanup"));
        for (const auto& Enemy : Session->State.enemies) if (!Enemy.dead && !Enemy.escaped)
        {
            int32 Count = 0;
            for (const AFoundryRobot* Robot : Robots) if (IsValid(Robot) && Robot->GetCoreId() == Enemy.id && !Robot->IsHidden()) ++Count;
            Check(Count == 1, TEXT("each living core enemy has exactly one visible original actor"));
        }
        UE_LOG(LogFoundryRosterProbe, Display, TEXT("ROSTER_RESULT case=%d hash=%s expected_hits=%d actors=%d"), RosterCase, *RosterExpectedHash, RosterExpectedHits, Robots.Num());
        ++RosterCase; RosterStep = 0; RosterElapsed = 0;
    }
}
