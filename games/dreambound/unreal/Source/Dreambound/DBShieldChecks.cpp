#include "DBShieldChecks.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "DBGameMode.h"
#include "DBProjectile.h"
#include "DBThrownShield.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/EngineVersion.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

ADBShieldCheckRunner::ADBShieldCheckRunner()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    PrimaryActorTick.TickGroup = TG_PostUpdateWork;
}

bool DBStartPhysicalShieldChecks(ADBGameMode& InMode)
{
    if (!InMode.GetWorld()) return false;
    ADBShieldCheckRunner* Runner = InMode.GetWorld()->SpawnActor<ADBShieldCheckRunner>();
    return Runner && Runner->Initialize(InMode);
}

bool ADBShieldCheckRunner::Check(bool bGood, const TCHAR* Id, const FString& Detail)
{
    Checks.Add({Scenario, Id, bGood, Detail});
    UE_LOG(LogTemp, Display, TEXT("DB_SHIELD_QA %s %s / %s"), bGood ? TEXT("PASS") : TEXT("FAIL"), Id, *Detail);
    return bGood;
}

bool ADBShieldCheckRunner::Initialize(ADBGameMode& InMode)
{
    Mode = &InMode;
    Player = Mode->Player;
    StartedAt = PhaseStartedAt = FPlatformTime::Seconds();
    OriginalSeed = Mode->Seed;
    FString ExplicitSlot;
    const bool bExplicitSlot = FParse::Value(FCommandLine::Get(), TEXT("DBSaveSlot="), ExplicitSlot);
    const bool bSafe = FParse::Param(FCommandLine::Get(), TEXT("DBVerify")) && bExplicitSlot
        && ExplicitSlot == Mode->SlotBase
        && (ExplicitSlot.StartsWith(TEXT("DreamboundQA")) || ExplicitSlot.StartsWith(TEXT("DBQA_")))
        && !ExplicitSlot.Contains(TEXT("/")) && !ExplicitSlot.Contains(TEXT("\\")) && !ExplicitSlot.Contains(TEXT(".."));
    if (!Check(bSafe, TEXT("isolated_slot"), TEXT("Requires -DBVerify and an explicit matching QA save-slot prefix.")))
    { Finish(true); return false; }
    if (!Check(Mode->bRecoverySlice && Mode->Rooms.Num() == 3 && IsValid(Player)
        && Player->GetController() && Player->ViewCamera, TEXT("recovery_world"), TEXT("Actual world, possessed character, camera and three courtyard phases.")))
    { Finish(true); return false; }

    for (const TCHAR* Suffix : {TEXT("_0"), TEXT("_1"), TEXT("_settings")})
    {
        FJournalCopy Copy;
        Copy.Slot = ExplicitSlot + Suffix;
        Copy.bExisted = UGameplayStatics::DoesSaveGameExist(Copy.Slot, 0);
        if (Copy.bExisted && !UGameplayStatics::LoadDataFromSlot(Copy.Bytes, Copy.Slot, 0))
        { Check(false, TEXT("journal_backup"), Copy.Slot); Finish(true); return false; }
        JournalCopies.Add(MoveTemp(Copy));
    }
    bHaveJournalCopies = true;
    bool bRemoved = true;
    for (const FJournalCopy& Copy : JournalCopies)
        if (Copy.bExisted) bRemoved &= UGameplayStatics::DeleteGameInSlot(Copy.Slot, 0);
    if (!Check(bRemoved, TEXT("journal_backup"), TEXT("Prior QA bytes backed up; empty isolated profile established.")))
    { Finish(true); return false; }
    Mode->StoredSave = nullptr;
    Mode->SaveRevision = 0;
    Mode->LearnedPatterns.Reset();
    Mode->StartingPattern = NAME_None;
    Mode->bBossWon = false;
    Mode->bCanResume = Mode->bSaveFailed = false;
    Mode->StartNewRun(true);
    MakeBox(Origin + FVector(0, 0, -25), FVector(3000, 3000, 25));
    ResetPlayer(Origin + FVector(-900, 0, 93));
    Target = MakeTarget(Origin + FVector(-720, 0, 88));
    if (!Check(IsValid(Target), TEXT("collision_fixture"), TEXT("Stationary real enemy and separate QA floor, 100 m above courtyard.")))
    { Finish(true); return false; }
    bInitialized = true;
    WriteResult(false);
    return true;
}

void ADBShieldCheckRunner::Go(EStep Next)
{
    Step = Next;
    PhaseAge = 0.f;
    PhaseStartedAt = FPlatformTime::Seconds();
}

void ADBShieldCheckRunner::Aim(FRotator Rotation)
{
    Player->GetController()->SetControlRotation(Rotation);
    Player->ViewCamera->SetWorldRotation(Rotation);
}

void ADBShieldCheckRunner::PlacePlayer(FVector Location)
{
    Player->GetCharacterMovement()->StopMovementImmediately();
    Player->SetActorLocation(Location, false, nullptr, ETeleportType::TeleportPhysics);
}

void ADBShieldCheckRunner::ResetPlayer(FVector Location, bool bClearUpgrades)
{
    Mode->bTitle = Mode->bPaused = Mode->bChoosingReward = Mode->bShowingBuild = Mode->bDefeated = Mode->bWon = false;
    Mode->SetMenuInput(false);
    if (bClearUpgrades) { Player->Upgrades.Reset(); Player->CurrentElement = EDBElement::Neutral; }
    Player->OnRunReset();
    // Position is explicitly staged. The later route uses swept movement, not OS controls.
    Player->GetCharacterMovement()->DisableMovement();
    PlacePlayer(Location);
    Aim(FRotator::ZeroRotator);
}

AActor* ADBShieldCheckRunner::MakeBox(FVector Center, FVector Extent)
{
    AActor* Actor = GetWorld()->SpawnActor<AActor>();
    if (!Actor) return nullptr;
    UBoxComponent* Box = NewObject<UBoxComponent>(Actor);
    Actor->SetRootComponent(Box);
    Actor->AddInstanceComponent(Box);
    Box->SetBoxExtent(Extent);
    Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Box->SetCollisionObjectType(ECC_WorldStatic);
    Box->SetCollisionResponseToAllChannels(ECR_Block);
    Box->RegisterComponent();
    Actor->SetActorLocation(Center);
    FixtureActors.Add(Actor);
    return Actor;
}

ADBEnemy* ADBShieldCheckRunner::MakeTarget(FVector Location)
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ADBEnemy* Enemy = GetWorld()->SpawnActor<ADBEnemy>(Location, FRotator(0, 180, 0), Params);
    if (!Enemy) return nullptr;
    Enemy->Configure(EDBEnemyKind::Melee, Mode->CurrentRoomId, 1.f);
    Enemy->SetActorLocation(Location);
    Enemy->Health = Enemy->MaxHealth = 2000.f; // Prevent death/progression during contact checks.
    Enemy->SetActorTickEnabled(false);
    Enemy->GetCharacterMovement()->StopMovementImmediately();
    Enemy->GetCharacterMovement()->DisableMovement();
    Enemy->GetCharacterMovement()->SetComponentTickEnabled(false);
    FixtureActors.Add(Enemy);
    return Enemy;
}

ADBProjectile* ADBShieldCheckRunner::FireBolt(FVector Start, FVector Direction)
{
    FActorSpawnParameters Params;
    Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ADBProjectile* Projectile = GetWorld()->SpawnActor<ADBProjectile>(Start, Direction.Rotation(), Params);
    if (Projectile) Projectile->Initialize(Direction, 1000.f, 17.f, false, nullptr, FLinearColor::Red);
    return Projectile;
}

void ADBShieldCheckRunner::BeginReturn()
{
    Flight = Player->ThrownShield.Get();
    LastFlightPoint = Flight.IsValid() ? Flight->GetActorLocation() : FVector::ZeroVector;
    MaxFlightStep = 0.f;
    LastCatchDistance = MAX_flt;
    bSawReturnTravel = false;
    Player->UseSpecial(); // Ordinary Q action, not a direct state mutation.
}

void ADBShieldCheckRunner::SampleReturn()
{
    if (!Flight.IsValid() || Flight->IsActorBeingDestroyed()) return;
    const FVector Point = Flight->GetActorLocation();
    const float Distance = FVector::Distance(Point, LastFlightPoint);
    MaxFlightStep = FMath::Max(MaxFlightStep, Distance);
    bSawReturnTravel |= Distance > 1.f;
    LastCatchDistance = FVector::Distance(Point, Player->GetShieldCatchLocation());
    LastFlightPoint = Point;
}

bool ADBShieldCheckRunner::CheckCatch(const TCHAR* Id)
{
    return Check(Player->ShieldState == EDBShieldState::Held && !IsValid(Player->ThrownShield)
        && bSawReturnTravel && LastCatchDistance <= 215.f && MaxFlightStep <= 145.f && PhaseAge < 4.4f,
        Id, FString::Printf(TEXT("return=%.3fs last catch gap=%.1fcm max sampled travel=%.1fcm; excludes timeout reconstitution"),
            PhaseAge, LastCatchDistance, MaxFlightStep));
}

bool ADBShieldCheckRunner::StartEncounter(int32 Index, int32 ExpectedCount)
{
    if (!Mode->Rooms.IsValidIndex(Index) || !IsValid(Mode->Rooms[Index].Altar)) return false;
    PlacePlayer(Mode->Rooms[Index].Altar->GetActorLocation() + FVector(-180, 0, 35));
    Mode->Interact(); // Must start via ward interaction, not SpawnWave or cleared flags.
    int32 Count = 0;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It)
        if (!It->bDead && It->RoomId == Index)
        {
            ++Count;
            It->SetActorTickEnabled(false);
            It->GetCharacterMovement()->DisableMovement();
        }
    Mode->Interact();
    int32 Again = 0;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It) if (!It->bDead && It->RoomId == Index) ++Again;
    return Check(!Mode->bSliceAwaitingStart && Count == ExpectedCount && Again == Count
        && !Mode->bChoosingReward && !Mode->ClaimedRooms.Contains(Index),
        *FString::Printf(TEXT("encounter_%d_starts_once"), Index),
        FString::Printf(TEXT("ward interaction spawned %d actual threats; repeated E leaves %d; no unearned reward"), Count, Again));
}

bool ADBShieldCheckRunner::ClearEncounter(int32 Index)
{
    TArray<ADBEnemy*> Enemies;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It) if (!It->bDead && It->RoomId == Index) Enemies.Add(*It);
    const int32 BeforeKills = Mode->Kills;
    for (ADBEnemy* Enemy : Enemies)
    {
        FDBHit Hit;
        Hit.Damage = 100000.f;
        Hit.Source = Player->GetActorLocation();
        Hit.Direction = FVector::ForwardVector;
        Hit.InstigatorActor = Player;
        Enemy->ApplyCombatHit(Hit); // Staged lethal damage tests progression, not player combat success.
        Enemy->ApplyCombatHit(Hit); // Dead actors must not notify/award twice.
    }
    return Check(!Enemies.IsEmpty() && Mode->ClearedRooms.Contains(Index) && Mode->Kills == BeforeKills + Enemies.Num(),
        *FString::Printf(TEXT("encounter_%d_clear"), Index),
        FString::Printf(TEXT("%d spawned enemies defeated with staged lethal hits; kill notices remain unique"), Enemies.Num()));
}

bool ADBShieldCheckRunner::TakeReward(FName Id, int32 ExpectedNextPhase)
{
    Mode->Interact();
    int32 Choice = INDEX_NONE;
    for (int32 I = 0; I < Mode->Offers.Num(); ++I) if (Mode->Offers[I].Id == Id) Choice = I;
    const bool bOfferedAfterClear = Mode->bChoosingReward && Choice != INDEX_NONE;
    const int32 Before = Player->GetUpgradeRank(Id);
    Mode->ChooseReward(Choice);
    Mode->ChooseReward(Choice);
    return Check(bOfferedAfterClear && Player->GetUpgradeRank(Id) == Before + 1
        && Mode->LearnedPatterns.Contains(Id) && Mode->ClaimedRooms.Contains(ExpectedNextPhase - 1)
        && Mode->CurrentRoomId == ExpectedNextPhase && Mode->bSliceAwaitingStart && !Mode->bChoosingReward,
        *FString::Printf(TEXT("earned_%s"), *Id.ToString()), TEXT("Clear, ward, ordinary reward choice, installation, retained pattern and next ready phase; duplicate input ignored."));
}

void ADBShieldCheckRunner::DestroyFixtures()
{
    for (AActor* Actor : FixtureActors) if (IsValid(Actor)) Actor->Destroy();
    FixtureActors.Reset();
    Target = nullptr;
    Cover = nullptr;
}

void ADBShieldCheckRunner::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized || bFinished) return;
    if (FPlatformTime::Seconds() - StartedAt > 55.0)
    { Check(false, TEXT("bounded_timeout"), FString::Printf(TEXT("step=%d"), int32(Step))); Finish(true); return; }
    if (!IsValid(Mode) || !IsValid(Player))
    { Check(false, TEXT("runtime_actor_lost")); Finish(true); return; }
    PhaseAge += DeltaSeconds;
    switch (Step)
    {
    case EStep::Prepare:
        if (PhaseAge < .08f) break;
        Scenario = TEXT("held_attack_defense_tradeoff");
        Player->PressGuard();
        HealthBefore = Player->Health;
        Player->ReceiveAttack(10.f, Player->ViewCamera->GetComponentLocation() + FVector(300, 0, 0));
        Check(Player->bGuarding && Player->Health == HealthBefore && Player->ParryFlashTime > 0.f,
            TEXT("held_front_guard"), TEXT("Fresh raised shield protects from the staged frontal attack."));
        Player->ReleaseGuard();
        TargetBefore = Target->Health;
        Player->PressFire(); Player->ReleaseFire();
        Player->PressGuard();
        Player->ReceiveAttack(10.f, Player->ViewCamera->GetComponentLocation() + FVector(300, 0, 0));
        Check(!Player->bGuarding && Player->AttackRecovery > 0.f && Player->Health < HealthBefore,
            TEXT("strike_commits_protection"), TEXT("Tap attacks; held guard cannot cancel its exposed recovery."));
        Go(EStep::Strike);
        break;
    case EStep::Strike:
        if (PhaseAge < .65f) break;
        Check(Target->Health < TargetBefore && Player->bGuarding && Player->AttackRecovery <= 0.f,
            TEXT("rim_contact_and_guard_recovery"), FString::Printf(TEXT("real nearby target health %.1f -> %.1f; held guard restored after recovery"), TargetBefore, Target->Health));
        Player->ReleaseGuard();
        Target->SetActorLocation(Origin + FVector(-100, 0, 88));
        TargetBefore = Target->Health;
        Player->PressFire();
        HealthBefore = Player->Health;
        Player->ReceiveAttack(10.f, Player->ViewCamera->GetComponentLocation() + FVector(300, 0, 0));
        Check(Player->ShieldState == EDBShieldState::Charging && !Player->bGuarding && Player->Health < HealthBefore,
            TEXT("charge_exposes_player"));
        Go(EStep::Charge);
        break;
    case EStep::Charge:
        if (PhaseAge < .45f) break;
        Scenario = TEXT("launch_exposure_and_pause");
        Player->ReleaseFire();
        Flight = Player->ThrownShield.Get();
        if (!Check(Flight.IsValid() && Player->ShieldState == EDBShieldState::Outbound, TEXT("release_launches_physical_actor")))
        { Finish(true); return; }
        HealthBefore = Player->Health;
        Player->PressGuard();
        Player->ReceiveAttack(10.f, Player->ViewCamera->GetComponentLocation() + FVector(300, 0, 0));
        Check(!Player->bGuarding && Player->Health < HealthBefore, TEXT("away_has_no_guard"));
        PausedPoint = Flight->GetActorLocation();
        Mode->TogglePause();
        Go(EStep::PausedFlight);
        break;
    case EStep::PausedFlight:
        if (FPlatformTime::Seconds() - PhaseStartedAt < .15) break;
        HealthBefore = Player->Health;
        Player->PressFire(); Player->UseSpecial(); Player->PressGuard();
        Player->ReceiveAttack(10.f, Player->ViewCamera->GetComponentLocation());
        Check(Flight.IsValid() && Mode->bPaused && Player->ShieldState == EDBShieldState::Outbound
            && Flight->GetActorLocation().Equals(PausedPoint, .01f) && Player->Health == HealthBefore,
            TEXT("pause_preserves_deployment"), TEXT("Paused real actor stops; attack/recall/damage calls cannot recover it."));
        Mode->TogglePause();
        Go(EStep::Outward);
        break;
    case EStep::Outward:
        if (Player->ShieldState != EDBShieldState::Lodged && PhaseAge < 3.2f) break;
        Scenario = TEXT("outward_return_contacts_and_catch");
        if (!Check(Flight.IsValid() && Player->ShieldState == EDBShieldState::Lodged && Target->Health < TargetBefore,
            TEXT("outward_swept_hit"), FString::Printf(TEXT("target %.1f -> %.1f through tick-driven physical travel"), TargetBefore, Target->Health)))
        { Finish(true); return; }
        OutwardHealth = Target->Health;
        Go(EStep::Lodged);
        break;
    case EStep::Lodged:
        if (PhaseAge < .2f) break;
        Check(Target->Health == OutwardHealth, TEXT("lodged_no_repeat_damage"));
        BeginReturn();
        Check(Flight.IsValid() && Flight->IsReturning() && Player->ShieldState == EDBShieldState::Returning,
            TEXT("q_recalls_same_actor"));
        Go(EStep::Return);
        break;
    case EStep::Return:
        SampleReturn();
        if (Player->ShieldState != EDBShieldState::Held && PhaseAge < 5.f) break;
        CheckCatch(TEXT("physical_return_catch"));
        Check(Target->Health < OutwardHealth, TEXT("return_swept_hit"), FString::Printf(TEXT("same target %.1f -> %.1f on return pass"), OutwardHealth, Target->Health));
        Go(EStep::CatchRecovery);
        break;
    case EStep::CatchRecovery:
        if (PhaseAge < .4f) break;
        Player->PressGuard();
        Check(Player->bGuarding && Player->bShieldReady, TEXT("caught_shield_usable"));
        Scenario = TEXT("physical_recall_around_cover");
        Target->Destroy(); Target = nullptr;
        ResetPlayer(Origin + FVector(-800, -700, 93));
        Cover = MakeBox(Origin + FVector(0, 0, 200), FVector(90, 450, 200));
        Player->PressFire();
        Go(EStep::CoverCharge);
        break;
    case EStep::CoverCharge:
        if (PhaseAge < .45f) break;
        Player->ReleaseFire(); Flight = Player->ThrownShield.Get();
        Go(EStep::CoverOutward);
        break;
    case EStep::CoverOutward:
        if (Player->ShieldState != EDBShieldState::Lodged && PhaseAge < 3.2f) break;
        if (!Check(Flight.IsValid() && Player->ShieldState == EDBShieldState::Lodged, TEXT("cover_route_deployed")))
        { Finish(true); return; }
        MoveDestination = Origin + FVector(-800, 150, 93);
        MoveSamples = 0;
        Go(EStep::CoverMove);
        break;
    case EStep::CoverMove:
    {
        const FVector Before = Player->GetActorLocation();
        const FVector Delta = MoveDestination - Before;
        FHitResult MovementHit;
        Player->SetActorLocation(Before + Delta.GetClampedToMaxSize(FMath::Min(35.f, 650.f * DeltaSeconds)), true, &MovementHit);
        ++MoveSamples;
        if (FVector::Distance(Player->GetActorLocation(), MoveDestination) > 5.f && PhaseAge < 3.f) break;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(DBQACover), false, this);
        Params.AddIgnoredActor(Player); Params.AddIgnoredActor(Flight.Get());
        FHitResult Obstruction;
        const bool bBlocked = Flight.IsValid() && GetWorld()->SweepSingleByChannel(Obstruction,
            Flight->GetActorLocation(), Player->GetShieldCatchLocation(), FQuat::Identity, ECC_Visibility,
            FCollisionShape::MakeSphere(20.f), Params);
        if (!Check(bBlocked && Obstruction.GetActor() == Cover && MoveSamples > 5
            && FVector::Distance(Player->GetActorLocation(), MoveDestination) <= 5.f,
            TEXT("moved_behind_real_cover"), TEXT("Swept lateral route recorded by deployed shield; its direct catch line hits the QA wall.")))
        { Finish(true); return; }
        BeginReturn(); Go(EStep::CoverReturn);
        break;
    }
    case EStep::CoverReturn:
        SampleReturn();
        if (Player->ShieldState != EDBShieldState::Held && PhaseAge < 5.f) break;
        CheckCatch(TEXT("recall_recovers_around_cover"));
        Scenario = TEXT("anchor_front_projectile_crossing");
        if (IsValid(Cover)) Cover->Destroy();
        ResetPlayer(Origin + FVector(-800, 0, 93));
        Player->ApplyUpgrade(TEXT("Anchor")); Player->ApplyUpgrade(TEXT("Anchor"));
        Player->PressFire(); Go(EStep::AnchorCharge);
        break;
    case EStep::AnchorCharge:
        if (PhaseAge < .45f) break;
        Player->ReleaseFire(); Flight = Player->ThrownShield.Get();
        Go(EStep::AnchorOutward);
        break;
    case EStep::AnchorOutward:
        if (Player->ShieldState != EDBShieldState::Lodged && PhaseAge < 3.2f) break;
        if (!Check(Flight.IsValid() && Flight->IsAnchored(), TEXT("physical_anchor_deploys"), TEXT("Rank 2 is granted only for this deterministic mechanics fixture.")))
        { Finish(true); return; }
        PlacePlayer(Flight->GetActorLocation() + FVector(-450, 0, 0));
        IntegrityBefore = Flight->GetAnchorIntegrity(); HealthBefore = Player->Health; BankBefore = Player->StoredShots;
        Bolt = FireBolt(Flight->GetActorLocation() + FVector(300, 0, 0), -FVector::ForwardVector);
        Go(EStep::AnchorFront);
        break;
    case EStep::AnchorFront:
        if (PhaseAge < 1.05f) break;
        if (!Flight.IsValid()) { Check(false, TEXT("anchor_actor_lost")); Finish(true); return; }
        Check(Flight.IsValid() && FMath::IsNearlyEqual(Flight->GetAnchorIntegrity(), IntegrityBefore - 17.f)
            && Player->Health == HealthBefore && Player->StoredShots == BankBefore + 1
            && (!Bolt.IsValid() || Bolt->IsActorBeingDestroyed()),
            TEXT("anchor_stops_crossing_projectile"), TEXT("Actual front-crossing bolt spent integrity and banked force; player behind it took no damage."));
        Scenario = TEXT("anchor_flank_and_back_exposure");
        PlacePlayer(Flight->GetActorLocation() + FVector(-450, 150, 0));
        IntegrityBefore = Flight->GetAnchorIntegrity(); HealthBefore = Player->Health; BankBefore = Player->StoredShots;
        Bolt = FireBolt(Flight->GetActorLocation() + FVector(300, 150, 0), -FVector::ForwardVector);
        Go(EStep::AnchorFlank);
        break;
    case EStep::AnchorFlank:
        if (PhaseAge < 1.05f) break;
        if (!Flight.IsValid()) { Check(false, TEXT("anchor_actor_lost")); Finish(true); return; }
        Check(Flight.IsValid() && Flight->GetAnchorIntegrity() == IntegrityBefore
            && FMath::IsNearlyEqual(Player->Health, HealthBefore - 17.f) && Player->StoredShots == BankBefore,
            TEXT("anchor_flank_remains_exposed"), TEXT("Parallel real bolt 150 cm outside the disk reaches the unguarded player."));
        PlacePlayer(Flight->GetActorLocation() + FVector(450, 0, 0));
        IntegrityBefore = Flight->GetAnchorIntegrity(); HealthBefore = Player->Health;
        Bolt = FireBolt(Flight->GetActorLocation() + FVector(-300, 0, 0), FVector::ForwardVector);
        Go(EStep::AnchorRear);
        break;
    case EStep::AnchorRear:
        if (PhaseAge < 1.05f) break;
        if (!Flight.IsValid()) { Check(false, TEXT("anchor_actor_lost")); Finish(true); return; }
        Check(Flight.IsValid() && Flight->GetAnchorIntegrity() == IntegrityBefore
            && FMath::IsNearlyEqual(Player->Health, HealthBefore - 17.f),
            TEXT("anchor_back_face_remains_exposed"), TEXT("Actual reverse crossing is not omnidirectional protection."));
        Player->OnRunReset(); DestroyFixtures();
        Mode->LearnedPatterns.Reset(); Mode->StartingPattern = NAME_None;
        Mode->StartNewRun(true);
        Player->GetCharacterMovement()->DisableMovement();
        Go(EStep::FirstEncounter);
        break;
    case EStep::FirstEncounter:
        Scenario = TEXT("first_earned_attachment");
        Mode->ChooseReward(0);
        Check(Player->Upgrades.IsEmpty() && Mode->ClaimedRooms.IsEmpty() && Mode->bSliceAwaitingStart,
            TEXT("fresh_run_has_no_free_reward"));
        if (!StartEncounter(0, 1) || !ClearEncounter(0) || !TakeReward(TEXT("Anchor"), 1))
        { Finish(true); return; }
        Check(Mode->StartingPattern == TEXT("Anchor"), TEXT("first_earned_pattern_selected"));
        Go(EStep::SecondEncounter);
        break;
    case EStep::SecondEncounter:
        Scenario = TEXT("second_earned_attachment");
        if (!StartEncounter(1, 3) || !ClearEncounter(1) || !TakeReward(TEXT("Frost"), 2))
        { Finish(true); return; }
        Check(Player->GetUpgradeRank(TEXT("Anchor")) == 1 && Player->GetUpgradeRank(TEXT("Frost")) == 1
            && Player->CurrentElement == EDBElement::Frost, TEXT("two_earned_attachments_combine"));
        Player->Health = 73.f;
        Player->PressFire(); Go(EStep::SaveCharge);
        break;
    case EStep::SaveCharge:
        if (PhaseAge < .35f) break;
        Scenario = TEXT("fresh_checkpoint_restores_physical_build");
        Player->ReleaseFire(); Flight = Player->ThrownShield.Get();
        if (!Check(Flight.IsValid() && Player->IsShieldAway(), TEXT("checkpoint_with_deployed_shield")))
        { Finish(true); return; }
        Mode->SaveProgress(true);
        Mode->StoredSave = nullptr;
        Mode->LoadProgress(); Mode->ResumeRun();
        Check(!Mode->bSaveFailed && Mode->CurrentRoomId == 2 && Mode->bSliceAwaitingStart
            && Mode->ClaimedRooms.Contains(0) && Mode->ClaimedRooms.Contains(1)
            && Player->GetUpgradeRank(TEXT("Anchor")) == 1 && Player->GetUpgradeRank(TEXT("Frost")) == 1
            && FMath::IsNearlyEqual(Player->Health, 73.f) && Player->CurrentElement == EDBElement::Frost
            && Player->ShieldState == EDBShieldState::Held && !IsValid(Player->ThrownShield)
            && (!Flight.IsValid() || Flight->IsActorBeingDestroyed()),
            TEXT("fresh_journal_resume_restores_build"), TEXT("Cache cleared before actual load/resume; checkpoint/build restored, deployed actor removed."));
        Player->GetCharacterMovement()->DisableMovement();
        Go(EStep::Journal);
        break;
    case EStep::Journal:
    {
        Scenario = TEXT("crc_checkpoint_recovery");
        Player->Health = 73.f; Mode->SaveProgress(true);
        const int32 ExpectedRevision = Mode->SaveRevision;
        Player->Health = 61.f; Player->ApplyUpgrade(TEXT("Ram")); Mode->LearnedPatterns.AddUnique(TEXT("Ram"));
        Mode->SaveProgress(true);
        const FString NewestSlot = Mode->SlotBase + FString::Printf(TEXT("_%d"), Mode->SaveRevision % 2);
        TArray<uint8> Bytes;
        bool bMutated = UGameplayStatics::LoadDataFromSlot(Bytes, NewestSlot, 0) && Bytes.Num() >= 44;
        if (bMutated) { Bytes[8] ^= 1; bMutated = UGameplayStatics::SaveDataToSlot(Bytes, NewestSlot, 0); }
        Mode->StoredSave = nullptr; Mode->LoadProgress();
        const bool bPrevious = Mode->StoredSave && Mode->StoredSave->Revision == ExpectedRevision;
        if (bPrevious) Mode->ResumeRun();
        Check(bMutated && bPrevious && Mode->CurrentRoomId == 2 && FMath::IsNearlyEqual(Player->Health, 73.f)
            && Player->GetUpgradeRank(TEXT("Anchor")) == 1 && Player->GetUpgradeRank(TEXT("Frost")) == 1
            && !Player->HasUpgrade(TEXT("Ram")) && !Mode->LearnedPatterns.Contains(TEXT("Ram")),
            TEXT("crc_fallback_preserves_earned_build"), FString::Printf(TEXT("checksum mutation=%d, previous revision=%d, selected=%d, bytes=%d"),
                bMutated, ExpectedRevision, Mode->StoredSave ? Mode->StoredSave->Revision : -1, Bytes.Num()));
        Player->GetCharacterMovement()->DisableMovement();
        Go(EStep::FinalEncounter);
        break;
    }
    case EStep::FinalEncounter:
        Scenario = TEXT("courtyard_completion");
        if (!StartEncounter(2, 2) || !ClearEncounter(2)) { Finish(true); return; }
        Check(!Mode->bWon && Mode->bBossWon && Mode->LearnedPatterns.Contains(TEXT("Capacitor")),
            TEXT("final_clear_allows_catch_before_results"));
        Go(EStep::Victory);
        break;
    case EStep::Victory:
        if (!Mode->bWon && PhaseAge < 2.5f) break;
        Check(Mode->bWon && !Mode->bCanResume && !Player->CanAct(), TEXT("courtyard_success_results"));
        Scenario = TEXT("retry_and_death_recovery");
        Mode->StartNewRun(true);
        Check(Mode->Seed == OriginalSeed && Mode->CurrentRoomId == 0 && Mode->bSliceAwaitingStart
            && Mode->ClaimedRooms.IsEmpty() && Mode->ClearedRooms.IsEmpty() && !Mode->bWon
            && Player->Upgrades.Num() == 1 && Player->GetUpgradeRank(TEXT("Anchor")) == 1
            && Player->ShieldState == EDBShieldState::Held && Player->StoredShots == 0
            && Mode->LearnedPatterns.Contains(TEXT("Frost")) && Mode->bBossWon,
            TEXT("same_seed_retry_resets_attempt_retains_patterns"));
        Player->ReceiveAttack(100000.f, Player->GetActorLocation(), true);
        Check(Player->bDead && Mode->bDefeated && !Mode->bCanResume, TEXT("death_records_inactive_attempt"));
        Mode->StartNewRun(true);
        Check(!Player->bDead && !Mode->bDefeated && Player->CanAct() && Player->Health == Player->MaxHealth
            && Player->ShieldState == EDBShieldState::Held && Player->AttackRecovery == 0.f
            && Player->GetUpgradeRank(TEXT("Anchor")) == 1 && Mode->LearnedPatterns.Contains(TEXT("Frost")),
            TEXT("death_retry_restores_usable_shield"));
        Finish();
        break;
    default: break;
    }
}

void ADBShieldCheckRunner::Finish(bool bAbort)
{
    if (bFinished) return;
    bFinished = true;
    bAborted = bAbort;
    if (IsValid(Mode))
    {
        Mode->bTitle = true; Mode->bPaused = true;
        Mode->bChoosingReward = false;
        if (IsValid(Player)) Player->SuspendCombatInput();
    }
    DestroyFixtures();
    if (bHaveJournalCopies)
    {
        Scenario = TEXT("qa_save_restoration");
        bool bRestored = true;
        for (const FJournalCopy& Copy : JournalCopies)
        {
            if (Copy.bExisted)
            {
                bRestored &= UGameplayStatics::SaveDataToSlot(Copy.Bytes, Copy.Slot, 0);
                TArray<uint8> Restored;
                bRestored &= UGameplayStatics::LoadDataFromSlot(Restored, Copy.Slot, 0) && Restored == Copy.Bytes;
            }
            else if (UGameplayStatics::DoesSaveGameExist(Copy.Slot, 0))
                bRestored &= UGameplayStatics::DeleteGameInSlot(Copy.Slot, 0);
        }
        Check(bRestored, TEXT("qa_journals_restored"), TEXT("Previous QA bytes restored exactly; newly created slots removed; end-play saving suppressed."));
    }
    WriteResult(true);
    Step = EStep::Finished;
    // The JSON outcome is authoritative even where platform exit status is unavailable.
    FGenericPlatformMisc::RequestExit(false);
}

void ADBShieldCheckRunner::WriteResult(bool bComplete) const
{
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("suite"), TEXT("physical-shield-courtyard-v1"));
    Report->SetStringField(TEXT("scope"), TEXT("Staged actual-world ticks, real sweeps/input APIs and artificial positions/lethal progression hits. No OS input, normal run or fun evidence."));
    Report->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
    Report->SetStringField(TEXT("executable"), FPlatformProcess::ExecutableName());
    Report->SetStringField(TEXT("platform"), TEXT("Windows"));
    Report->SetStringField(TEXT("save_slot"), IsValid(Mode) ? Mode->SlotBase : TEXT("unavailable"));
    Report->SetNumberField(TEXT("seed"), OriginalSeed);
    Report->SetBoolField(TEXT("null_rhi"), FParse::Param(FCommandLine::Get(), TEXT("NullRHI")));
    Report->SetBoolField(TEXT("complete"), bComplete);
    Report->SetBoolField(TEXT("aborted"), bAborted);
    Report->SetNumberField(TEXT("last_step"), int32(Step));
    Report->SetNumberField(TEXT("elapsed_wall_seconds"), FPlatformTime::Seconds() - StartedAt);
    TArray<TSharedPtr<FJsonValue>> Rows;
    TMap<FString, TSharedPtr<FJsonObject>> Scenarios;
    for (const FCheck& Result : Checks)
    {
        TSharedPtr<FJsonObject>& ScenarioRow = Scenarios.FindOrAdd(Result.Scenario);
        if (!ScenarioRow)
        {
            ScenarioRow = MakeShared<FJsonObject>();
            ScenarioRow->SetStringField(TEXT("id"), Result.Scenario);
            ScenarioRow->SetStringField(TEXT("status"), TEXT("passed"));
            ScenarioRow->SetArrayField(TEXT("observations"), {});
            Rows.Add(MakeShared<FJsonValueObject>(ScenarioRow));
        }
        if (!Result.bPassed) ScenarioRow->SetStringField(TEXT("status"), TEXT("failed"));
        TSharedRef<FJsonObject> Observation = MakeShared<FJsonObject>();
        Observation->SetStringField(TEXT("id"), Result.Id);
        Observation->SetBoolField(TEXT("passed"), Result.bPassed);
        Observation->SetStringField(TEXT("detail"), Result.Detail);
        TArray<TSharedPtr<FJsonValue>> Observations = ScenarioRow->GetArrayField(TEXT("observations"));
        Observations.Add(MakeShared<FJsonValueObject>(Observation));
        ScenarioRow->SetArrayField(TEXT("observations"), Observations);
    }
    int32 Passed = 0, Failed = 0;
    for (const auto& Pair : Scenarios)
        Pair.Value->GetStringField(TEXT("status")) == TEXT("passed") ? ++Passed : ++Failed;
    Report->SetNumberField(TEXT("passed"), Passed);
    Report->SetNumberField(TEXT("failed"), Failed);
    Report->SetArrayField(TEXT("checks"), Rows);
    Report->SetStringField(TEXT("not_run"), bAborted ? TEXT("Remaining steps after last_step; ordinary inputs, graphics/audio/feel and an unstaged full journey.")
        : TEXT("Ordinary inputs, graphics/audio/feel, performance and an unstaged full journey."));
    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Report, Writer);
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("QA");
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Path = Directory / TEXT("physical-shield-checks.json");
    const bool bWritten = FFileHelper::SaveStringToFile(Json, *Path);
    UE_LOG(LogTemp, Display, TEXT("DB_SHIELD_QA_RESULT complete=%d aborted=%d passed=%d failed=%d written=%d path=%s"),
        bComplete, bAborted, Passed, Failed, bWritten, *Path);
}
