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
        && Player->GetController() && Player->ViewCamera, TEXT("recovery_world"), TEXT("Actual world, possessed character, camera and three connected rooms.")))
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
    Enemy->Configure(EDBEnemyKind::Melee, INDEX_NONE, 1.f);
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

int32 ADBShieldCheckRunner::FindPiece(EDBShieldPieceState State) const
{
    for (int32 I = 0; I < Player->GetShieldPieceCount(); ++I)
        if (Player->GetPieceState(I) == State) return I;
    return INDEX_NONE;
}

bool ADBShieldCheckRunner::CheckPieceOwnership(const TCHAR* Id)
{
    TSet<ADBThrownShield*> Actors;
    bool Good = Player->GetShieldPieceCount() == 6;
    for (int32 I = 0; I < 6; ++I)
    {
        const EDBShieldPieceState State = Player->GetPieceState(I);
        ADBThrownShield* Piece = Player->GetPieceFlight(I);
        const bool Away = State == EDBShieldPieceState::Outbound || State == EDBShieldPieceState::Lodged || State == EDBShieldPieceState::Returning;
        Good &= Away == IsValid(Piece);
        if (Piece) { Good &= Piece->GetPieceId() == I && !Actors.Contains(Piece); Actors.Add(Piece); }
    }
    Good &= Player->GetAttachedPieceCount() + Player->GetDeployedPieceCount() + Player->GetRegeneratingPieceCount() == 6;
    return Check(Good, Id, FString::Printf(TEXT("held=%d selected=%d deployed=%d rebuilding=%d unique flight actors=%d"),
        Player->GetAttachedPieceCount(), Player->GetSelectedPieceCount(), Player->GetDeployedPieceCount(), Player->GetRegeneratingPieceCount(), Actors.Num()));
}

bool ADBShieldCheckRunner::StartEncounter(int32 Index, int32 ExpectedCount)
{
    if (!Mode->Rooms.IsValidIndex(Index) || !IsValid(Mode->Rooms[Index].Altar)) return false;
    Mode->ClearRewardPractice();
    Mode->ActivateRoom(Index); // Explicit staging; corridor collision is checked separately, not a claimed walk-through.
    PlacePlayer(Mode->Rooms[Index].Altar->GetActorLocation() + FVector(-180, 0, 35));
    Mode->Interact();
    int32 Count = 0;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It)
        if (!It->bDead && It->RoomId == Index)
        {
            ++Count; It->SetActorTickEnabled(false);
            It->GetCharacterMovement()->StopMovementImmediately();
            It->GetCharacterMovement()->DisableMovement();
        }
    Mode->Interact();
    int32 Again = 0;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It) if (!It->bDead && It->RoomId == Index) ++Again;
    return Check(!Mode->bSliceAwaitingStart && Count == ExpectedCount && Again == Count
        && !Mode->bChoosingReward && !Mode->ClaimedRooms.Contains(Index),
        *FString::Printf(TEXT("encounter_%d_starts_once"), Index),
        FString::Printf(TEXT("ActivateRoom staging, actual ward interaction spawned %d threats; repeated E leaves %d."), Count, Again));
}

bool ADBShieldCheckRunner::ClearEncounter(int32 Index)
{
    TArray<ADBEnemy*> Enemies;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It) if (!It->bDead && It->RoomId == Index) Enemies.Add(*It);
    const int32 Before = Mode->Kills;
    for (ADBEnemy* Enemy : Enemies)
    {
        FDBHit Hit; Hit.Damage = 100000.f; Hit.Source = Player->GetActorLocation(); Hit.InstigatorActor = Player;
        Enemy->ApplyCombatHit(Hit); Enemy->ApplyCombatHit(Hit);
    }
    return Check(!Enemies.IsEmpty() && Mode->ClearedRooms.Contains(Index) && Mode->Kills == Before + Enemies.Num(),
        *FString::Printf(TEXT("encounter_%d_clear"), Index), TEXT("Actual spawned enemies; staged lethal combat hits; duplicate death notification rejected."));
}

bool ADBShieldCheckRunner::TakeReward(FName Id, int32 Room)
{
    Mode->Interact();
    int32 Choice = INDEX_NONE;
    for (int32 I = 0; I < Mode->Offers.Num(); ++I) if (Mode->Offers[I].Id == Id) Choice = I;
    const bool Offered = Mode->bChoosingReward && Choice != INDEX_NONE;
    const int32 Before = Player->GetUpgradeRank(Id);
    Mode->ChooseReward(Choice); Mode->ChooseReward(Choice);
    bool Open = Mode->Rooms.IsValidIndex(Room + 1) && !Mode->Rooms[Room + 1].Gates.IsEmpty();
    if (Open) for (AActor* Gate : Mode->Rooms[Room + 1].Gates) Open &= IsValid(Gate) && !Gate->GetActorEnableCollision();
    return Check(Offered && Player->GetUpgradeRank(Id) == Before + 1 && Mode->LearnedPatterns.Contains(Id)
        && Mode->ClaimedRooms.Contains(Room) && Mode->CurrentRoomId == Room && !Mode->bChoosingReward
        && Mode->PracticeReward == Id && !Mode->PracticeInstruction.IsEmpty() && Open,
        *FString::Printf(TEXT("earned_%s_stays_in_room"), *Id.ToString()),
        TEXT("Clear, ward, actual choice/claim: one installation, retained pattern, same room, practice instruction and next gate opened."));
}

bool ADBShieldCheckRunner::CheckPractice(FName Id, int32 Room)
{
    const int32 BeforeKills = Mode->Kills;
    const TArray<int32> BeforeCleared = Mode->ClearedRooms;
    int32 Count = 0; bool Safe = true;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It)
        if (It->bPracticeTarget)
        {
            ++Count; Safe &= It->RoomId == INDEX_NONE;
            FDBHit Hit; Hit.Damage = 100000.f; Hit.InstigatorActor = Player; Hit.Source = Player->GetActorLocation();
            It->ApplyCombatHit(Hit);
            Safe &= !It->bDead && It->Health > 0.f;
        }
    return Check(Count > 0 && Safe && Mode->PracticeReward == Id && Mode->CurrentRoomId == Room
        && Mode->Kills == BeforeKills && Mode->ClearedRooms == BeforeCleared && !Mode->bDefeated && Player->CanAct(),
        *FString::Printf(TEXT("%s_practice_is_safe"), *Id.ToString()),
        FString::Printf(TEXT("%d actual practice targets survive lethal contact; RoomId=-1; no kill/clear progression."), Count));
}

bool ADBShieldCheckRunner::CheckRouteGeometry()
{
    bool Connected = Mode->Rooms.Num() == 3, Clear = true, Floored = true, Closed = true;
    int32 Samples = 0; FString Failures;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBSegmentRouteQA), false, Player);
    for (const FDBRoom& Room : Mode->Rooms) for (AActor* Gate : Room.Gates) Params.AddIgnoredActor(Gate);
    for (int32 I = 1; I < Mode->Rooms.Num(); ++I)
    {
        const FVector A = Mode->Rooms[I - 1].Center, B = Mode->Rooms[I].Center;
        const FVector D = (B - A).GetSafeNormal2D();
        Connected &= Mode->Rooms[I].Parent == I - 1 && FMath::IsNearlyEqual(FVector::Distance(A, B), 4200.f, 1.f);
        const FVector Start = A + D * 1300.f + FVector(0, 0, 110), End = B - D * 1300.f + FVector(0, 0, 110);
        FHitResult Obstacle;
        if (GetWorld()->SweepSingleByChannel(Obstacle, Start, End, FQuat::Identity, ECC_Pawn,
            FCollisionShape::MakeCapsule(Player->GetCapsuleComponent()->GetScaledCapsuleRadius(), Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), Params))
        {
            Clear = false; Failures += FString::Printf(TEXT(" link%d blocked by %s;"), I, *GetNameSafe(Obstacle.GetActor()));
        }
        for (int32 S = 0; S <= 16; ++S)
        {
            const FVector P = FMath::Lerp(Start, End, S / 16.f); FHitResult Floor;
            Floored &= GetWorld()->LineTraceSingleByChannel(Floor, P, P - FVector(0, 0, 350), ECC_Visibility, Params)
                && Floor.ImpactNormal.Z > .65f;
            ++Samples;
        }
        Closed &= !Mode->Rooms[I].Gates.IsEmpty();
        for (AActor* Gate : Mode->Rooms[I].Gates) Closed &= IsValid(Gate) && Gate->GetActorEnableCollision();
    }
    return Check(Connected && Clear && Floored && Closed, TEXT("connected_collision_and_floor"),
        FString::Printf(TEXT("Three linked rooms; player-sized door/corridor sweeps (closed progression gates explicitly ignored), %d floor probes; gates initially closed.%s"), Samples, *Failures));
}

void ADBShieldCheckRunner::DestroyFixtures()
{
    for (AActor* Actor : FixtureActors) if (IsValid(Actor)) Actor->Destroy();
    FixtureActors.Reset(); Target = nullptr; OtherTarget = nullptr; Cover = nullptr;
}

void ADBShieldCheckRunner::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized || bFinished) return;
    if (FPlatformTime::Seconds() - StartedAt > 150.0)
    { Check(false, TEXT("bounded_timeout"), FString::Printf(TEXT("step=%d"), int32(Step))); Finish(true); return; }
    if (!IsValid(Mode) || !IsValid(Player)) { Check(false, TEXT("runtime_actor_lost")); Finish(true); return; }
    PhaseAge += DeltaSeconds;
    switch (Step)
    {
    case EStep::Prepare:
        if (PhaseAge < .08f) break;
        Scenario = TEXT("allocation_and_retained_guard");
        Target->Destroy(); Target = nullptr; Player->PressFire(); Go(EStep::HalfCharge); break;
    case EStep::HalfCharge:
        if (PhaseAge < .52f) break;
        Check(Player->GetSelectedPieceCount() == 3 && Player->GetAttachedPieceCount() == 6 && !Player->bGuarding,
            TEXT("hold_selects_three"), FString::Printf(TEXT("Ordinary hold observed %.4fs; expected thresholds .22/.36/.50."), PhaseAge));
        Player->ReleaseFire();
        Check(Player->GetDeployedPieceCount() == 3 && Player->GetAttachedPieceCount() == 3 && Player->GetSelectedPieceCount() == 0,
            TEXT("release_launches_selected_three"));
        CheckPieceOwnership(TEXT("launch_ownership")); Go(EStep::LaunchRecovery); break;
    case EStep::LaunchRecovery:
        if (PhaseAge < .25f) break;
        HealthBefore = Player->Health; Player->PressGuard();
        Player->ReceiveAttack(17.f, Player->ViewCamera->GetComponentLocation() + FVector(300, 0, 0), false, nullptr, 101);
        FirstLost = FindPiece(EDBShieldPieceState::Regenerating);
        Check(Player->Health == HealthBefore && Player->GetAttachedPieceCount() == 2 && Player->GetRegeneratingPieceCount() == 1,
            TEXT("remainder_blocks_without_health_loss"));
        Scenario = TEXT("attack_identity");
        Player->ReceiveAttack(17.f, Player->ViewCamera->GetComponentLocation() + FVector(300, 0, 0), false, nullptr, 101);
        Check(Player->GetRegeneratingPieceCount() == 1 && Player->Health == HealthBefore, TEXT("same_attack_id_spends_once"));
        Flight = Player->GetPieceFlight(0);
        PausedPoint = Flight.IsValid() ? Flight->GetActorLocation() : FVector::ZeroVector;
        PausedProgress = Player->GetPieceRegenerationProgress(FirstLost);
        Mode->TogglePause(); Go(EStep::Paused); break;
    case EStep::Paused:
        if (PhaseAge < .35f) break;
        Scenario = TEXT("pause_freezes_flight_and_rebuild");
        Check(Mode->bPaused && Flight.IsValid() && FVector::Dist(Flight->GetActorLocation(), PausedPoint) < .01f
            && FMath::IsNearlyEqual(Player->GetPieceRegenerationProgress(FirstLost), PausedProgress, .0001f),
            TEXT("pause_preserves_position_and_timer"), TEXT("Ordinary pause menu; runner advances while character/flight remain frozen."));
        Mode->TogglePause(); Player->PressGuard(); Go(EStep::SecondBlock); break;
    case EStep::SecondBlock:
        if (PhaseAge < .6f) break;
        Scenario = TEXT("attack_identity");
        Player->ReceiveAttack(17.f, Player->ViewCamera->GetComponentLocation() + FVector(300, 0, 0), false, nullptr, 102);
        for (int32 I = 0; I < 6; ++I) if (I != FirstLost && Player->GetPieceState(I) == EDBShieldPieceState::Regenerating) SecondLost = I;
        Check(SecondLost != INDEX_NONE && Player->GetRegeneratingPieceCount() == 2 && Player->Health == HealthBefore,
            TEXT("different_attack_id_spends_another"));
        Scenario = TEXT("independent_reconstruction");
        Check(Player->GetPieceRegenerationProgress(FirstLost) > .15f && Player->GetPieceRegenerationProgress(SecondLost) < .03f,
            TEXT("staggered_losses_have_separate_timers"));
        Player->ReleaseGuard(); Go(EStep::FirstRebuild); break;
    case EStep::FirstRebuild:
        if (Player->GetPieceState(FirstLost) == EDBShieldPieceState::Regenerating && PhaseAge < 3.f) break;
        Check(Player->GetPieceState(FirstLost) == EDBShieldPieceState::Attached
            && Player->GetPieceState(SecondLost) == EDBShieldPieceState::Regenerating && Player->GetDeployedPieceCount() == 3,
            TEXT("older_piece_rebuilds_without_waiting_for_newer")); Go(EStep::LastRebuild); break;
    case EStep::LastRebuild:
        if (Player->GetPieceState(SecondLost) == EDBShieldPieceState::Regenerating && PhaseAge < 1.f) break;
        Check(Player->GetRegeneratingPieceCount() == 0 && Player->GetAttachedPieceCount() == 3, TEXT("later_piece_finishes_own_timer"));
        ResetPlayer(Origin + FVector(-900, 0, 93));
        Scenario = TEXT("concurrent_throws_and_unselected_catch"); Player->PressFire(); Go(EStep::FirstPartial); break;
    case EStep::FirstPartial:
        if (PhaseAge < .24f) break;
        Player->ReleaseFire(); CaughtPiece = FindPiece(EDBShieldPieceState::Outbound);
        Check(Player->GetDeployedPieceCount() == 1, TEXT("first_partial_throw")); Go(EStep::SecondReady); break;
    case EStep::SecondReady:
        if (PhaseAge < .24f) break;
        Player->PressFire(); Player->RecallShield(); Go(EStep::CatchDuringCharge); break;
    case EStep::CatchDuringCharge:
        if (PhaseAge < .52f) break;
        Check(Player->GetPieceState(CaughtPiece) == EDBShieldPieceState::Attached && Player->GetSelectedPieceCount() == 3,
            TEXT("catch_during_hold_docks_unselected"), TEXT("One physical return docks during a second hold; original three lit pieces remain selected."));
        Player->ReleaseFire();
        Check(Player->GetDeployedPieceCount() == 3 && Player->GetAttachedPieceCount() == 3
            && Player->GetPieceState(CaughtPiece) == EDBShieldPieceState::Attached, TEXT("second_throw_launches_only_lit_pieces"));
        FirstLost = FindPiece(EDBShieldPieceState::Outbound);
        if (ADBThrownShield* Lost = Player->GetPieceFlight(FirstLost))
        {
            Lost->DestroyPiece();
            Player->OnShieldPieceDestroyed(FirstLost, Lost); Player->OnShieldPieceCaught(FirstLost, Lost);
        }
        Player->RecallShield(); Go(EStep::RecallSurvivors); break;
    case EStep::RecallSurvivors:
        if (Player->GetDeployedPieceCount() > 0 && PhaseAge < 1.5f) break;
        Scenario = TEXT("destroyed_piece_and_survivor_recall");
        Check(Player->GetDeployedPieceCount() == 0 && Player->GetAttachedPieceCount() == 5
            && Player->GetPieceState(FirstLost) == EDBShieldPieceState::Regenerating, TEXT("only_survivors_return_no_duplicate_replacement"));
        CheckPieceOwnership(TEXT("recall_ownership"));
        ResetPlayer(Origin + FVector(-900, 0, 93)); Target = MakeTarget(Origin + FVector(-720, 0, 88));
        Scenario = TEXT("zero_piece_core_offense"); Player->PressGuard(); HealthBefore = Player->Health;
        for (uint32 I = 0; I < 6; ++I) Player->ReceiveAttack(10.f, Player->ViewCamera->GetComponentLocation() + FVector(300, 0, 0), false, nullptr, 200 + I);
        Check(Player->GetAttachedPieceCount() == 0 && Player->GetRegeneratingPieceCount() == 6 && Player->Health == HealthBefore,
            TEXT("six_distinct_blocks_spend_six_pieces"));
        TargetBefore = Target->Health; Player->PressFire(); Player->ReleaseFire(); Go(EStep::CoreStrike); break;
    case EStep::CoreStrike:
        if (PhaseAge < .4f) break;
        Check(Target->Health < TargetBefore && Player->GetAttachedPieceCount() == 0 && !Player->bGuarding,
            TEXT("core_tap_hits_without_guard_pieces"));
        Target->Destroy(); ResetPlayer(Origin + FVector(-900, 0, 93)); Player->Health = 100000.f;
        Target = MakeTarget(Origin + FVector(-100, 0, 100)); Target->Configure(EDBEnemyKind::Boss, INDEX_NONE, 1.f);
        Target->SetArenaBounds(Origin, FVector2D(2500, 2500)); Target->Health = Target->MaxHealth = 2000.f;
        Target->SetActorTickEnabled(true); bSawEliteTell = false;
        Scenario = TEXT("elite_directional_interception"); Go(EStep::EliteWait); break;
    case EStep::EliteWait:
        bSawEliteTell |= Target->Phase == EDBEnemyPhase::Telegraph && Target->Telegraph.Contains(TEXT("INTERCEPT"));
        if (!Target->bShieldInterceptionReady && PhaseAge < 18.f) break;
        Check(bSawEliteTell && Target->bShieldInterceptionReady && Target->Phase == EDBEnemyPhase::Attack,
            TEXT("actual_ai_enters_signalled_counter"));
        // Freeze only after the real AI entered its stance; test direction and simultaneous piece consumption separately from timing difficulty.
        Target->SetActorTickEnabled(false); Target->GetCharacterMovement()->StopMovementImmediately();
        Check(!Target->TryInterceptShieldPiece(Target->GetActorForwardVector())
            && !Target->TryInterceptShieldPiece(FVector::CrossProduct(Target->GetActorForwardVector(), FVector::UpVector))
            && Target->bShieldInterceptionReady, TEXT("rear_and_flank_do_not_consume_counter"));
        Aim((Target->GetActorLocation() - Player->ViewCamera->GetComponentLocation()).Rotation());
        TargetBefore = Target->Health; Player->PressFire(); Go(EStep::EliteCharge); break;
    case EStep::EliteCharge:
        if (PhaseAge < .52f) break;
        Player->ReleaseFire(); Go(EStep::EliteContact); break;
    case EStep::EliteContact:
        if (PhaseAge < .8f) break;
        Check(Player->GetRegeneratingPieceCount() == 1 && Player->GetDeployedPieceCount() == 2
            && !Target->bShieldInterceptionReady && Target->Health < TargetBefore,
            TEXT("physical_salvo_loses_exactly_one_other_pieces_hit"), TEXT("Real outbound collision; active stance frozen after normal telegraph to isolate interception semantics."));
        Target->Destroy(); ResetPlayer(Origin + FVector(-900, 0, 93)); Player->ApplyUpgrade(TEXT("Frost"));
        Target = MakeTarget(Origin + FVector(-350, 0, 100)); TargetBefore = Target->Health;
        Scenario = TEXT("frost_launch_and_return_shatter"); Player->PressFire(); Go(EStep::FrostCharge); break;
    case EStep::FrostCharge:
        if (PhaseAge < .24f) break;
        Player->ReleaseFire(); Go(EStep::FrostOutbound); break;
    case EStep::FrostOutbound:
        if (PhaseAge < .55f) break;
        OutwardHealth = Target->Health;
        Check(OutwardHealth < TargetBefore && Target->ChillStacks == 1 && Player->GetRegeneratingPieceCount() == 0,
            TEXT("ordinary_enemy_hit_chills_without_destroying_piece"));
        Player->RecallShield(); Go(EStep::FrostReturn); break;
    case EStep::FrostReturn:
        if (Player->GetDeployedPieceCount() > 0 && PhaseAge < 2.f) break;
        Check(Player->GetAttachedPieceCount() == 6 && OutwardHealth - Target->Health > TargetBefore - OutwardHealth
            && Mode->EventText.Contains(TEXT("SHATTER")), TEXT("physical_return_consumes_chill_and_adds_shatter_damage"));
        Target->Destroy(); ResetPlayer(Origin + FVector(-900, 0, 93)); Go(EStep::Storm); break;
    case EStep::Storm:
    {
        Scenario = TEXT("finite_lethal_storm_chain");
        TArray<ADBEnemy*> Chain;
        const FVector Points[] = {FVector(0, 0, 100), FVector(0, 180, 100), FVector(0, -180, 100), FVector(350, 0, 100)};
        for (const FVector& P : Points)
        {
            ADBEnemy* E = MakeTarget(Origin + P); E->Health = E->MaxHealth = 10.f; Chain.Add(E);
            FDBHit Prime; Prime.Damage = 1.f; Prime.Element = EDBElement::Storm; Prime.InstigatorActor = Player;
            E->ApplyCombatHit(Prime);
        }
        const int32 BeforeKills = Mode->Kills;
        FDBHit Lethal; Lethal.Damage = 100.f; Lethal.Element = EDBElement::Storm; Lethal.InstigatorActor = Player;
        Chain[0]->ApplyCombatHit(Lethal); Chain[0]->ApplyCombatHit(Lethal);
        Check(Chain[0]->bDead && Chain[1]->bDead && Chain[2]->bDead && !Chain[3]->bDead
            && FMath::IsNearlyEqual(Chain[3]->Health, 9.f) && Mode->Kills == BeforeKills,
            TEXT("lethal_root_arcs_to_two_without_recursive_chain"), TEXT("Four physically visible, pre-marked enemies; lethal root and two lethal secondary hits; fourth untouched; isolated RoomId=-1."));
        DestroyFixtures(); Player->OnRunReset(); Go(EStep::Route); break;
    }
    case EStep::Route:
    {
        Scenario = TEXT("seeded_connected_route");
        OriginalLayout = Mode->LayoutSignature;
        TArray<FVector> Centers; for (const FDBRoom& R : Mode->Rooms) Centers.Add(R.Center);
        Mode->BuildWorld();
        bool Same = OriginalLayout == Mode->LayoutSignature && Centers.Num() == Mode->Rooms.Num();
        for (int32 I = 0; I < Centers.Num() && Mode->Rooms.IsValidIndex(I); ++I) Same &= Centers[I].Equals(Mode->Rooms[I].Center, .1f);
        Check(Same, TEXT("same_seed_reproduces_route_and_cover"), OriginalLayout);
        bool Changed = false;
        for (int32 I = 1; I <= 3 && !Changed; ++I) { Mode->Seed = OriginalSeed + I; Mode->BuildWorld(); Changed = OriginalLayout != Mode->LayoutSignature; }
        Check(Changed, TEXT("alternate_seed_changes_layout"), TEXT("Up to three nearby seeds sampled; not exhaustive procedural validation."));
        Mode->Seed = OriginalSeed; Mode->StartingPattern = NAME_None; Mode->LearnedPatterns.Reset(); Mode->StartNewRun(true);
        Player->GetCharacterMovement()->DisableMovement(); CheckRouteGeometry(); Go(EStep::FirstEncounter); break;
    }
    case EStep::FirstEncounter:
        Scenario = TEXT("earned_rewards_and_safe_practice");
        if (!StartEncounter(0, 1) || !ClearEncounter(0) || !TakeReward(TEXT("Mirror"), 0)) { Finish(true); return; }
        Go(EStep::FirstPractice); break;
    case EStep::FirstPractice:
        if (PhaseAge < .35f) break;
        CheckPractice(TEXT("Mirror"), 0); Go(EStep::SecondEncounter); break;
    case EStep::SecondEncounter:
        if (!StartEncounter(1, 3) || !ClearEncounter(1) || !TakeReward(TEXT("Frost"), 1)) { Finish(true); return; }
        Go(EStep::SecondPractice); break;
    case EStep::SecondPractice:
        if (PhaseAge < .35f) break;
        CheckPractice(TEXT("Frost"), 1); Mode->ClearRewardPractice(); Mode->ActivateRoom(2);
        PlacePlayer(Mode->GetRoomEntryPoint(2)); Aim(FRotator::ZeroRotator);
        Player->Health = 73.f; Player->PressFire(); Go(EStep::SaveCharge); break;
    case EStep::SaveCharge:
        if (PhaseAge < .24f) break;
        Scenario = TEXT("checkpoint_resume"); Player->ReleaseFire(); Flight = Player->GetPieceFlight(0);
        Check(Player->GetDeployedPieceCount() == 1, TEXT("save_with_piece_deployed"));
        Mode->SaveProgress(true); Mode->StoredSave = nullptr; Mode->LoadProgress(); Mode->ResumeRun();
        Check(!Mode->bSaveFailed && Mode->CurrentRoomId == 2 && Mode->bSliceAwaitingStart
            && Mode->ClaimedRooms.Contains(0) && Mode->ClaimedRooms.Contains(1)
            && Player->GetUpgradeRank(TEXT("Mirror")) == 1 && Player->GetUpgradeRank(TEXT("Frost")) == 1
            && FMath::IsNearlyEqual(Player->Health, 73.f) && Player->CurrentElement == EDBElement::Frost
            && Player->GetAttachedPieceCount() == 6 && Player->GetDeployedPieceCount() == 0 && Player->GetRegeneratingPieceCount() == 0
            && (!Flight.IsValid() || Flight->IsActorBeingDestroyed()) && Mode->LayoutSignature == OriginalLayout
            && FVector::Distance(Player->GetActorLocation(), Mode->GetRoomEntryPoint(2)) < 30.f,
            TEXT("resume_restores_earned_build_at_connected_room_entry"));
        Player->GetCharacterMovement()->DisableMovement(); Go(EStep::Journal); break;
    case EStep::Journal:
    {
        Scenario = TEXT("crc_checkpoint_fallback"); Player->Health = 73.f; Mode->SaveProgress(true);
        const int32 ExpectedRevision = Mode->SaveRevision;
        Player->Health = 61.f; Player->ApplyUpgrade(TEXT("Ram")); Mode->LearnedPatterns.AddUnique(TEXT("Ram")); Mode->SaveProgress(true);
        const FString Slot = Mode->SlotBase + FString::Printf(TEXT("_%d"), Mode->SaveRevision % 2);
        TArray<uint8> Bytes; bool Mutated = UGameplayStatics::LoadDataFromSlot(Bytes, Slot, 0) && Bytes.Num() >= 44;
        if (Mutated) { Bytes[8] ^= 1; Mutated = UGameplayStatics::SaveDataToSlot(Bytes, Slot, 0); }
        Mode->StoredSave = nullptr; Mode->LoadProgress();
        const bool Previous = Mode->StoredSave && Mode->StoredSave->Revision == ExpectedRevision;
        if (Previous) Mode->ResumeRun();
        Check(Mutated && Previous && Mode->CurrentRoomId == 2 && FMath::IsNearlyEqual(Player->Health, 73.f)
            && Player->HasUpgrade(TEXT("Mirror")) && Player->HasUpgrade(TEXT("Frost")) && !Player->HasUpgrade(TEXT("Ram"))
            && !Mode->LearnedPatterns.Contains(TEXT("Ram")), TEXT("corrupt_latest_loads_previous_earned_checkpoint"));
        Player->GetCharacterMovement()->DisableMovement(); Go(EStep::FinalEncounter); break;
    }
    case EStep::FinalEncounter:
        Scenario = TEXT("completion_death_and_reset");
        if (!StartEncounter(2, 2) || !ClearEncounter(2)) { Finish(true); return; }
        Check(!Mode->bWon && Mode->bBossWon && Mode->LearnedPatterns.Contains(TEXT("Capacitor")), TEXT("final_clear_allows_return_before_results"));
        Go(EStep::Victory); break;
    case EStep::Victory:
        if (!Mode->bWon && PhaseAge < 2.5f) break;
        Check(Mode->bWon && !Mode->bCanResume && !Player->CanAct(), TEXT("victory_is_inactive_saved_attempt"));
        Mode->StartNewRun(true);
        Check(Mode->Seed == OriginalSeed && Mode->LayoutSignature == OriginalLayout && Mode->ClaimedRooms.IsEmpty()
            && Mode->ClearedRooms.IsEmpty() && Player->GetAttachedPieceCount() == 6 && Player->GetUpgradeRank(TEXT("Mirror")) == 1
            && Mode->LearnedPatterns.Contains(TEXT("Frost")) && Mode->bBossWon, TEXT("same_seed_retry_retains_patterns_resets_pieces"));
        Player->ReceiveAttack(100000.f, Player->GetActorLocation(), true);
        Check(Player->bDead && Mode->bDefeated && !Mode->bCanResume, TEXT("death_saves_inactive_attempt"));
        Mode->StartNewRun(true);
        Check(!Player->bDead && !Mode->bDefeated && Player->CanAct() && Player->Health == Player->MaxHealth
            && Player->GetAttachedPieceCount() == 6 && Player->GetDeployedPieceCount() == 0 && Player->GetRegeneratingPieceCount() == 0
            && Player->AttackRecovery == 0.f, TEXT("death_retry_restores_usable_six_piece_shield"));
        CheckPieceOwnership(TEXT("reset_ownership")); Finish(); break;
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
        Scenario = TEXT("isolated_setup");
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
    Report->SetStringField(TEXT("suite"), TEXT("segmented-shield-connected-route-v1"));
    Report->SetStringField(TEXT("scope"), TEXT("Staged ordinary world ticks, real piece sweeps/input APIs, AI-created counter stance then frozen, artificial positions/lethal hits and explicit ActivateRoom staging. No OS input, normal journey or fun evidence."));
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
    const FString Path = Directory / TEXT("segmented-shield-checks.json");
    const bool bWritten = FFileHelper::SaveStringToFile(Json, *Path);
    UE_LOG(LogTemp, Display, TEXT("DB_SHIELD_QA_RESULT complete=%d aborted=%d passed=%d failed=%d written=%d path=%s"),
        bComplete, bAborted, Passed, Failed, bWritten, *Path);
}
