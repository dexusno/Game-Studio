#include "DBShieldChecks.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "DBGameMode.h"
#include "DBProjectile.h"
#include "DBThrownShield.h"
#include "Camera/CameraComponent.h"
#include "Components/BoxComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
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

namespace
{
bool IsTrellisArtCheck()
{
    return FParse::Param(FCommandLine::Get(), TEXT("DBArtCheck"));
}
}

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
    bMovementCheck = FParse::Param(FCommandLine::Get(), TEXT("DBMovementCheck"));
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
    if (bMovementCheck)
    {
        // Ordinary CharacterMovement ticks on an isolated, level collision fixture.
        // There is enough runway for a complete stamina bar without teleporting mid-sprint.
        if (!MakeBox(Origin + FVector(0, 0, -25), FVector(10000, 3000, 25))
            || !MakeBox(Origin + FVector(350, 1800, 200), FVector(25, 600, 200)))
        { Check(false, TEXT("movement_fixture"), TEXT("Could not create runway/wall.")); Finish(true); return false; }
        StageWalkingPlayer(Origin + FVector(-4500, 0, 93));
        Scenario = TEXT("grounded_locomotion");
        GoMovement(EMovementStep::Settle);
        bInitialized = true;
        WriteResult(false);
        return true;
    }
    if (IsTrellisArtCheck())
    {
        bInitialized = true;
        Go(EStep::Route);
        WriteResult(false);
        return true;
    }
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
    bool Connected = Mode->Rooms.Num() == 3, Clear = true, Floored = true, Closed = true, OpenedClear = true, Bounded = true;
    int32 Samples = 0; FString Failures;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBSegmentRouteQA), false, Player);
    for (const FDBRoom& Room : Mode->Rooms) for (AActor* Gate : Room.Gates) Params.AddIgnoredActor(Gate);
    for (int32 I = 1; I < Mode->Rooms.Num(); ++I)
    {
        const FVector A = Mode->Rooms[I - 1].Center, B = Mode->Rooms[I].Center;
        const FVector D = (B - A).GetSafeNormal2D(), T(-D.Y, D.X, 0);
        Connected &= Mode->Rooms[I].Parent == I - 1 && FMath::IsNearlyEqual(FVector::Distance(A, B), 4200.f, 1.f);
        const FVector Start = A + D * 1300.f + FVector(0, 0, 110), End = B - D * 1300.f + FVector(0, 0, 110);
        for (float Lane : {-130.f, 0.f, 130.f}) {
            FHitResult Obstacle;
            if (GetWorld()->SweepSingleByChannel(Obstacle, Start + T * Lane, End + T * Lane, FQuat::Identity, ECC_Pawn,
                FCollisionShape::MakeCapsule(Player->GetCapsuleComponent()->GetScaledCapsuleRadius(), Player->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()), Params)) {
                Clear = false;
                const auto* Component = Cast<UStaticMeshComponent>(Obstacle.GetComponent());
                Failures += FString::Printf(TEXT(" link%d lane%.0f blocked by %s;"), I, Lane,
                    Component && Component->GetStaticMesh() ? *Component->GetStaticMesh()->GetName() : *GetNameSafe(Obstacle.GetActor()));
            }
            for (int32 S = 0; S <= 16; ++S) {
                const FVector P = FMath::Lerp(Start, End, S / 16.f) + T * Lane; FHitResult Floor;
                Floored &= GetWorld()->LineTraceSingleByChannel(Floor, P, P - FVector(0, 0, 350), ECC_Visibility, Params)
                    && Floor.ImpactNormal.Z > .65f && Floor.ImpactPoint.Z >= -30.f && Floor.ImpactPoint.Z <= 2.f;
                ++Samples;
            }
        }
        Closed &= !Mode->Rooms[I].Gates.IsEmpty();
        for (AActor* Gate : Mode->Rooms[I].Gates) {
            if (!IsValid(Gate)) { Closed = false; OpenedClear = false; continue; }
            const bool WasClosed = Gate->GetActorEnableCollision();
            FCollisionQueryParams GateQuery(SCENE_QUERY_STAT(DBReverieClosedGate), false, Player);
            const FCollisionShape Capsule = FCollisionShape::MakeCapsule(32.f,88.f);
            for (float Lane : {-270.f,0.f,270.f}) {
                const FVector GateStart=(A+B)*.5f-D*90.f+T*Lane+FVector(0,0,110);
                const FVector GateEnd=(A+B)*.5f+D*90.f+T*Lane+FVector(0,0,110);
                FHitResult Hit;
                const bool Blocked = WasClosed && GetWorld()->SweepSingleByChannel(Hit,GateStart,GateEnd,
                    FQuat::Identity,ECC_Pawn,Capsule,GateQuery) && Hit.GetActor()==Gate;
                Closed &= Blocked;
                if (!Blocked) Failures += FString::Printf(TEXT(" link%d closed gate bypass at lane%.0f;"),I,Lane);
            }
            Gate->SetActorEnableCollision(false);
            for (float Lane : {-270.f,0.f,270.f}) {
                const FVector GateStart=(A+B)*.5f-D*90.f+T*Lane+FVector(0,0,110);
                const FVector GateEnd=(A+B)*.5f+D*90.f+T*Lane+FVector(0,0,110);
                FHitResult Hit;
                const bool Free = !GetWorld()->SweepSingleByChannel(Hit,GateStart,GateEnd,FQuat::Identity,ECC_Pawn,Capsule,GateQuery);
                OpenedClear &= Free;
                if (!Free) Failures += FString::Printf(TEXT(" link%d opened gate blocks lane%.0f;"),I,Lane);
            }
            Gate->SetActorEnableCollision(WasClosed);
        }
    }
    for (int32 I = 0; I < Mode->Rooms.Num(); ++I) for (int32 Side = 0; Side < 4; ++Side) {
        const FVector C = Mode->Rooms[I].Center, Out = FRotator(0, Side*90.f, 0).Vector(), T(-Out.Y, Out.X, 0);
        bool Door = false;
        for (int32 Other : {I-1, I+1}) if (Mode->Rooms.IsValidIndex(Other))
            Door |= FVector::DotProduct((Mode->Rooms[Other].Center-C).GetSafeNormal2D(), Out) > .9f;
        if (Door) continue;
        FHitResult Wall;
        const bool HitWall = GetWorld()->SweepSingleByChannel(Wall, C+Out*1630.f+T*600.f+FVector(0,0,110),
            C+Out*1950.f+T*600.f+FVector(0,0,110), FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(32.f,88.f), Params);
        const auto* Component = Cast<UStaticMeshComponent>(Wall.GetComponent());
        Bounded &= HitWall && Component && Component->GetStaticMesh() && Component->GetStaticMesh()->GetFName() == FName(TEXT("SM_RV_Wall"));
    }
    return Check(Connected && Clear && Floored && Closed && OpenedClear && Bounded, TEXT("connected_collision_and_floor"),
        FString::Printf(TEXT("Three linked rooms; three capsule lanes per gateway/bridge, %d floor probes, center/+-270cm gate sweeps blocked while closed and clear after opening, enclosing walls on every unconnected cardinal side. Gate collision restored after the isolated open fixture.%s"), Samples, *Failures));
}

void ADBShieldCheckRunner::DestroyFixtures()
{
    for (AActor* Actor : FixtureActors) if (IsValid(Actor)) Actor->Destroy();
    FixtureActors.Reset(); Target = nullptr; OtherTarget = nullptr; BlockedBlastTarget = nullptr; Cover = nullptr;
}

void ADBShieldCheckRunner::StageWalkingPlayer(FVector Location)
{
    ResetPlayer(Location);
    Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    MovementProbe.Input = FVector::ZeroVector;
}

void ADBShieldCheckRunner::GoMovement(EMovementStep Next)
{
    FMovementProbe& M = MovementProbe;
    M.Step = Next;
    M.Age = M.Distance = M.PeakSpeed = M.MaxZ = M.SampleDistance = M.SampleSeconds = 0.f;
    M.Start = M.Last = Player->GetActorLocation();
    M.StartStamina = Player->Stamina;
}

void ADBShieldCheckRunner::TickMovement(float DeltaSeconds)
{
    FMovementProbe& M = MovementProbe;
    UCharacterMovementComponent* Movement = Player->GetCharacterMovement();
    M.Age += DeltaSeconds;
    M.Elapsed += DeltaSeconds;
    const FVector Position = Player->GetActorLocation();
    const float Travel = FVector::Dist2D(Position, M.Last);
    M.Distance += Travel;
    // NullRHI may run sub-millisecond world ticks. A 1ms denominator floor
    // would under-report real speed despite correct total distance/time.
    if (DeltaSeconds > UE_SMALL_NUMBER)
    {
        M.PeakSpeed = FMath::Max(M.PeakSpeed, Travel / DeltaSeconds);
        M.MinTickSeconds = FMath::Min(M.MinTickSeconds, DeltaSeconds);
        M.MaxTickSeconds = FMath::Max(M.MaxTickSeconds, DeltaSeconds);
        ++M.ObservedTicks;
    }
    M.MaxZ = FMath::Max(M.MaxZ, float(FMath::Abs(Position.Z - M.Start.Z)));
    if (M.Age > .3f) { M.SampleDistance += Travel; M.SampleSeconds += DeltaSeconds; }
    M.Last = Position;
    if (M.Elapsed > 24.f)
    { Check(false, TEXT("movement_world_timeout"), FString::Printf(TEXT("step=%d elapsed=%.3fs"), int32(M.Step), M.Elapsed)); Finish(true); return; }

    const bool bGrounded = Movement->IsMovingOnGround() && Movement->CurrentFloor.IsWalkableFloor();
    const auto ReadyOnFloor = [&]()
    {
        if (M.Age >= .18f && bGrounded && FMath::Abs(Player->GetVelocity().Z) < 1.f) return true;
        if (M.Age > 1.f)
        { Check(false, TEXT("walking_floor_ready"), FString::Printf(TEXT("step=%d z=%.2f mode=%d"), int32(M.Step), Position.Z, int32(Movement->MovementMode))); Finish(true); }
        return false;
    };
    switch (M.Step)
    {
    case EMovementStep::Settle:
        if (!ReadyOnFloor()) break;
        Player->PressSprint(); GoMovement(EMovementStep::Idle); break;
    case EMovementStep::Idle:
        if (M.Age < .65f) break;
        M.bIdleGood = M.Distance < 1.f && !Player->IsSprinting()
            && FMath::IsNearlyEqual(Player->Stamina, Player->MaxStamina, .1f);
        Player->ReleaseSprint(); M.Input = FVector::ForwardVector;
        GoMovement(EMovementStep::Walk); break;
    case EMovementStep::Walk:
        if (M.Age < .85f) break;
        {
            const float MeasuredSpeed = M.SampleDistance / FMath::Max(M.SampleSeconds, .001f);
            Check(M.bIdleGood && bGrounded && !Player->IsSprinting()
                && MeasuredSpeed > 565.f && MeasuredSpeed < 615.f,
                TEXT("stationary_sprint_and_real_walk"),
                FString::Printf(TEXT("Stationary Shift retained full stamina=%d; physical walking after acceleration %.1f cm/s over %.3fs."), M.bIdleGood, MeasuredSpeed, M.SampleSeconds));
        }
        Player->PressSprint(); GoMovement(EMovementStep::Sprint); break;
    case EMovementStep::Sprint:
        // Observe the actual exhaustion transition. A nearly-empty bar can
        // still be sprinting for another tick and has not latched exhaustion.
        if ((Player->Stamina > 0.f || Player->IsSprinting()) && M.Age < 5.6f) break;
        Check(Player->Stamina <= 0.f && !Player->IsSprinting() && bGrounded
            && M.Age >= 4.9f && M.Age <= 5.35f && M.Distance > 4200.f && M.Distance < 4700.f
            && M.PeakSpeed > 860.f && M.PeakSpeed < 920.f,
            TEXT("sprint_exhausts_after_physical_run"),
            FString::Printf(TEXT("Full-bar run %.3fs, displacement/path %.1f cm, peak %.1f cm/s, stamina %.3f; expected roughly five seconds at 885 cm/s after acceleration."), M.Age, M.Distance, M.PeakSpeed, Player->Stamina));
        M.FirstRecovery = -1.f; M.bDelayGood = false;
        GoMovement(EMovementStep::RecoverHeld); break;
    case EMovementStep::RecoverHeld:
        // Keep both movement and the original Shift hold through recovery.
        if (M.FirstRecovery < 0.f && Player->Stamina > .2f) M.FirstRecovery = M.Age;
        if (M.Age >= .55f && M.Age <= .7f) M.bDelayGood = Player->Stamina <= .2f && !Player->IsSprinting();
        if (M.Age < 1.8f) break;
        Check(M.bDelayGood && M.FirstRecovery >= .76f && M.FirstRecovery <= .92f
            && Player->Stamina >= 22.f && Player->Stamina <= 28.f && !Player->IsSprinting()
            && Player->GetVelocity().Size2D() < 615.f,
            TEXT("delayed_recovery_does_not_restart_held_sprint"),
            FString::Printf(TEXT("First observed recovery %.3fs; stamina %.2f at %.3fs, speed %.1f cm/s while original Shift remains held. Expected 0.8s delay then 25/s."), M.FirstRecovery, Player->Stamina, M.Age, Player->GetVelocity().Size2D()));
        Player->ReleaseSprint(); Player->PressSprint();
        GoMovement(EMovementStep::ResumeSprint); break;
    case EMovementStep::ResumeSprint:
        if (M.Age < .45f) break;
        M.bFreshSprintGood = Player->IsSprinting() && M.PeakSpeed > 820.f
            && M.StartStamina >= 20.f && Player->Stamina < 20.f;
        Player->ReleaseSprint(); Player->PressSprint();
        GoMovement(EMovementStep::RejectLowSprint); break;
    case EMovementStep::RejectLowSprint:
        if (M.Age < .3f) break;
        Check(M.bFreshSprintGood && !Player->IsSprinting() && Player->GetVelocity().Size2D() < 615.f,
            TEXT("fresh_sprint_requires_recovered_threshold"),
            FString::Printf(TEXT("Fresh press above 20 produced physical sprint=%d; second fresh press at %.2f stamina stayed walking at %.1f cm/s."), M.bFreshSprintGood, M.StartStamina, Player->GetVelocity().Size2D()));
        StageWalkingPlayer(Origin + FVector(0, 1800, 93));
        GoMovement(EMovementStep::WallSettle); break;
    case EMovementStep::WallSettle:
        if (!ReadyOnFloor()) break;
        Player->PressSprint(); M.Input = FVector::ForwardVector;
        GoMovement(EMovementStep::WallApproach); break;
    case EMovementStep::WallApproach:
        if (M.Age < .85f) break;
        GoMovement(EMovementStep::WallHold); break;
    case EMovementStep::WallHold:
        if (M.Age < 1.f) break;
        Check(M.Distance < 1.f && M.PeakSpeed < 3.f && !Player->IsSprinting()
            && Player->Stamina >= M.StartStamina - .1f,
            TEXT("sprint_into_solid_wall_does_not_drain"),
            FString::Printf(TEXT("After physical contact, held movement/Shift for %.3fs: displacement %.3f cm, stamina %.2f -> %.2f."), M.Age, M.Distance, M.StartStamina, Player->Stamina));
        Scenario = TEXT("grounded_evasion");
        StageWalkingPlayer(Origin + FVector(-2500, 0, 93));
        GoMovement(EMovementStep::DashSettle); break;
    case EMovementStep::DashSettle:
        if (!ReadyOnFloor()) break;
        Player->Dash(); GoMovement(EMovementStep::OpenDash); break;
    case EMovementStep::OpenDash:
        if (Player->IsDashing() && M.Age < .6f) break;
        Check(!Player->IsDashing() && bGrounded && M.Age >= .20f && M.Age <= .34f
            && M.Distance >= 420.f && M.Distance <= 480.f && M.MaxZ < 2.f && M.PeakSpeed > 1500.f
            && FMath::IsNearlyEqual(Player->Stamina, M.StartStamina, .1f),
            TEXT("dash_moves_four_metres_without_hop"),
            FString::Printf(TEXT("Actual dash %.3fs, path %.2f cm, peak %.1f cm/s, maximum vertical offset %.3f cm, stamina %.2f -> %.2f. Sampling includes the tick that reports source completion."), M.Age, M.Distance, M.PeakSpeed, M.MaxZ, M.StartStamina, Player->Stamina));
        Player->ReleaseDash(); Player->Dash();
        M.bCooldownBlocked = !Player->IsDashing() && Player->DashCooldown > .8f;
        M.bHeldPointSet = false; GoMovement(EMovementStep::DashCooldown); break;
    case EMovementStep::DashCooldown:
        if (M.Age > .3f && !M.bHeldPointSet) { M.HeldPoint = Position; M.bHeldPointSet = true; }
        Player->Dash(); // Repeated calls without ReleaseDash cannot become a new press.
        if (M.Age < 1.55f) break;
        {
            const bool HeldStill = M.bHeldPointSet && FVector::Dist2D(Position, M.HeldPoint) < 1.f
                && !Player->IsDashing() && Player->DashCooldown <= .001f;
            Player->ReleaseDash(); Player->Dash();
            Check(M.bCooldownBlocked && HeldStill && Player->IsDashing(),
                TEXT("dash_cooldown_and_release_edge"),
                FString::Printf(TEXT("Early fresh press rejected=%d; held repeat stayed still after cooldown=%d; release/new press starts=%d."), M.bCooldownBlocked, HeldStill, Player->IsDashing()));
        }
        StageWalkingPlayer(Origin + FVector(0, 1800, 93));
        GoMovement(EMovementStep::WallDashSettle); break;
    case EMovementStep::WallDashSettle:
        if (!ReadyOnFloor()) break;
        Player->Dash(); GoMovement(EMovementStep::WallDash); break;
    case EMovementStep::WallDash:
        if (M.Age < .45f) break;
        {
            const float ContactX = Origin.X + 325.f - Player->GetCapsuleComponent()->GetScaledCapsuleRadius();
            Check(!Player->IsDashing() && bGrounded && M.Distance > 250.f
                && Position.X <= ContactX + 1.f && Position.X >= ContactX - 3.f
                && M.MaxZ < 2.f && Player->GetVelocity().Size2D() < 3.f,
                TEXT("dash_stops_at_physical_wall"),
                FString::Printf(TEXT("Path %.2f cm; capsule X %.3f, wall contact limit %.3f; vertical offset %.3f cm, final speed %.3f cm/s."), M.Distance, Position.X, ContactX, M.MaxZ, Player->GetVelocity().Size2D()));
        }
        Scenario = TEXT("movement_menu_and_reset");
        StageWalkingPlayer(Origin + FVector(-2500, 0, 93));
        GoMovement(EMovementStep::PauseSettle); break;
    case EMovementStep::PauseSettle:
        if (!ReadyOnFloor()) break;
        M.Input = FVector::ForwardVector; Player->PressSprint();
        GoMovement(EMovementStep::PauseSprint); break;
    case EMovementStep::PauseSprint:
        if (M.Age < .3f) break;
        M.Input = FVector::ZeroVector; Player->ReleaseSprint(); Player->Dash();
        GoMovement(EMovementStep::PauseDash); break;
    case EMovementStep::PauseDash:
        if (M.Age < .07f) break;
        M.bPauseGood = Player->IsDashing() && M.Distance > 80.f && Player->Stamina < Player->MaxStamina;
        Mode->TogglePause(); M.SavedCooldown = Player->DashCooldown;
        GoMovement(EMovementStep::Paused); break;
    case EMovementStep::Paused:
        if (M.Age < .35f) break;
        M.bPauseGood &= Mode->bPaused && !Player->IsDashing() && !Player->IsSprinting()
            && M.Distance < .01f && FMath::IsNearlyEqual(Player->Stamina, M.StartStamina, .001f)
            && FMath::IsNearlyEqual(Player->DashCooldown, M.SavedCooldown, .001f);
        Mode->TogglePause(); GoMovement(EMovementStep::Resume); break;
    case EMovementStep::Resume:
        if (M.Age < .35f) break;
        Check(M.bPauseGood && !Mode->bPaused && !Player->IsDashing() && !Player->IsSprinting()
            && M.Distance < 1.f && Player->GetVelocity().Size2D() < 1.f,
            TEXT("pause_suspends_dash_and_resume_has_no_stale_input"),
            FString::Printf(TEXT("Active dash canceled/frozen with stamina and cooldown retained=%d; resume drift %.3f cm over %.3fs."), M.bPauseGood, M.Distance, M.Age));
        Player->OnRunReset(); Player->PressSprint(); Player->Dash();
        GoMovement(EMovementStep::ResetDash); break;
    case EMovementStep::ResetDash:
        if (M.Age < .07f) break;
        M.bResetStarted = Player->IsDashing() && M.Distance > 80.f;
        Player->OnRunReset(); GoMovement(EMovementStep::ResetWait); break;
    case EMovementStep::ResetWait:
        if (M.Age < .35f) break;
        {
            const bool Cleared = M.bResetStarted && !Player->IsDashing() && !Player->IsSprinting()
                && M.Distance < 1.f && FMath::IsNearlyEqual(Player->Stamina, Player->MaxStamina, .001f)
                && Player->DashCooldown <= .001f && FMath::IsNearlyEqual(Movement->MaxWalkSpeed, 590.f, .1f);
            Player->Dash();
            Check(Cleared && Player->IsDashing(), TEXT("run_reset_clears_burst_resources_and_held_latches"),
                FString::Printf(TEXT("Reset during measured dash; clean resources/stillness=%d, drift %.3f cm; fresh dash accepted without stale held latch=%d."), Cleared, M.Distance, Player->IsDashing()));
        }
        Finish(); return;
    }
    if (!bFinished && !MovementProbe.Input.IsNearlyZero() && Player->CanAct())
        Player->AddMovementInput(MovementProbe.Input);
}

void ADBShieldCheckRunner::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized || bFinished) return;
    if (FPlatformTime::Seconds() - StartedAt > (bMovementCheck ? 45.0 : 150.0))
    { Check(false, TEXT("bounded_timeout"), FString::Printf(TEXT("step=%d"), int32(Step))); Finish(true); return; }
    if (!IsValid(Mode) || !IsValid(Player)) { Check(false, TEXT("runtime_actor_lost")); Finish(true); return; }
    if (bMovementCheck) { TickMovement(DeltaSeconds); return; }
    PhaseAge += DeltaSeconds;
    switch (Step)
    {
    case EStep::Prepare:
        if (PhaseAge < .08f) break;
        Scenario = TEXT("allocation_and_retained_guard");
        Target->Destroy(); Target = nullptr; Player->PressFire(); Go(EStep::HalfCharge); break;
    case EStep::HalfCharge:
        if (PhaseAge < Player->GetSelectionHoldTime(3) + .02f) break;
        Check(Player->GetSelectedPieceCount() == 3 && Player->GetAttachedPieceCount() == 6 && !Player->bGuarding,
            TEXT("hold_selects_three"), FString::Printf(TEXT("Ordinary hold observed %.4fs; third-piece threshold from charge tuning."), PhaseAge));
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
        if (PhaseAge < Player->GetSelectionHoldTime(3) + .02f) break;
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
        if (PhaseAge < Player->GetSelectionHoldTime(3) + .02f) break;
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
        Target->Destroy(); ResetPlayer(Origin + FVector(-900, 0, 93)); Go(EStep::FoldIdle); break;
    case EStep::FoldIdle:
        if (PhaseAge < .6f) break;
        Scenario = TEXT("folding_shield_pose");
        Check(Player->GetShieldExpansion() < .03f && !Player->bGuarding, TEXT("idle_is_collapsed"));
        FoldedSpan = 0.f;
        for (int32 I=0;I<6;++I) for (int32 J=I+1;J<6;++J)
            FoldedSpan=FMath::Max(FoldedSpan,float(FVector::Dist(Player->GetPieceCatchLocation(I),Player->GetPieceCatchLocation(J))));
        Player->PressGuard(); Go(EStep::FoldGuard); break;
    case EStep::FoldGuard:
    {
        if (PhaseAge < .6f) break;
        float ExpandedSpan=0.f;
        for (int32 I=0;I<6;++I) for (int32 J=I+1;J<6;++J)
            ExpandedSpan=FMath::Max(ExpandedSpan,float(FVector::Dist(Player->GetPieceCatchLocation(I),Player->GetPieceCatchLocation(J))));
        Check(Player->bGuarding && Player->GetShieldExpansion()>.97f && ExpandedSpan>FoldedSpan*1.25f,
            TEXT("guard_separates_physical_piece_sockets"),FString::Printf(TEXT("folded span %.2fcm, expanded %.2fcm"),FoldedSpan,ExpandedSpan));
        Player->ReleaseGuard(); Go(EStep::FoldReturn); break;
    }
    case EStep::FoldReturn:
        if (PhaseAge < .6f) break;
        Check(Player->GetShieldExpansion()<.03f && !Player->bGuarding,TEXT("release_refolds"));
        Target=MakeTarget(Origin+FVector(-260,0,110));TargetBefore=Target->Health;
        Aim((Target->GetActorLocation()+FVector(0,0,35)-Player->GetPawnViewLocation()).Rotation());
        Scenario=TEXT("full_charge_payoff");Player->PressFire();Go(EStep::FiveCharge);break;
    case EStep::FiveCharge:
        if(PhaseAge<Player->GetSelectionHoldTime(5)+.02f)break;
        Check(Player->GetSelectedPieceCount()==5&&!Player->IsFullChargeReady(),TEXT("five_pieces_are_not_full_power"));
        Player->ReleaseFire();Go(EStep::FiveContact);break;
    case EStep::FiveContact:
        if(PhaseAge<.7f)break;
        PartialVolleyDamage=TargetBefore-Target->Health;
        Check(PartialVolleyDamage>0.f&&Player->GetAttachedPieceCount()==1,TEXT("partial_volley_hits_and_keeps_armor"));
        ResetPlayer(Origin+FVector(-900,0,93));Target->Health=TargetBefore;
        OtherTarget=MakeTarget(Origin+FVector(-260,220,110));
        BlockedBlastTarget=MakeTarget(Origin+FVector(-260,-220,110));
        Cover=MakeBox(Origin+FVector(-260,-110,140),FVector(150,15,140));
        Aim((Target->GetActorLocation()+FVector(0,0,35)-Player->GetPawnViewLocation()).Rotation());
        Player->PressFire();Go(EStep::FullCharge);break;
    case EStep::FullCharge:
    {
        if(PhaseAge<Player->GetFullChargeHoldTime()+.03f)break;
        Check(Player->IsFullChargeReady()&&PhaseAge>=1.7f,TEXT("full_power_requires_long_hold"));
        Player->ReleaseFire();bool AllEmpowered=true;TSet<uint32> Volleys;
        for(int32 I=0;I<6;++I){auto* F=Player->GetPieceFlight(I);AllEmpowered&=F&&F->IsFullyCharged();if(F)Volleys.Add(F->GetVolleyId());}
        Check(AllEmpowered&&Volleys.Num()==1&&Player->GetAttachedPieceCount()==0,TEXT("one_full_volley_commits_all_armor"));
        Go(EStep::FullContact);break;
    }
    case EStep::FullContact:
    {
        if(PhaseAge<.7f)break;
        Check(!Player->IsChargeLoopPlaying(),TEXT("release_stops_charge_component"),TEXT("Lifecycle state only; NullRHI/nosound does not assess sound."));
        const float FullDamage=TargetBefore-Target->Health;
        Check(FullDamage/6.f>PartialVolleyDamage/5.f*1.5f,TEXT("full_release_more_than_piece_count_bonus"),
            FString::Printf(TEXT("five-piece damage %.1f, full six %.1f; minimum 1.5x damage per piece"),PartialVolleyDamage,FullDamage));
        Check(FMath::IsNearlyEqual(OtherTarget->MaxHealth-OtherTarget->Health,80.f,.1f),TEXT("one_shared_blast_hits_neighbor_once"));
        Check(BlockedBlastTarget->Health==BlockedBlastTarget->MaxHealth,TEXT("blast_respects_cover"));
        OtherTarget->Destroy();BlockedBlastTarget->Destroy();Cover->Destroy();
        Target->Destroy();ResetPlayer(Origin+FVector(-900,0,93));Target=MakeTarget(Origin+FVector(-625,0,110));
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Aim((Target->GetActorLocation()+FVector(0,0,35)-Player->GetPawnViewLocation()).Rotation());
        TargetBefore=Target->Health;Scenario=TEXT("reachable_melee_chain");Player->PressFire();Player->ReleaseFire();Go(EStep::MeleeFirst);break;
    }
    case EStep::MeleeFirst:
        if(PhaseAge<.6f)break;
        FirstMeleeDamage=TargetBefore-Target->Health;
        Check(FirstMeleeDamage>0,TEXT("tap_reaches_enemy_275cm_away"));
        Check(Player->GetActorLocation().X>Origin.X-875.f&&Player->GetActorLocation().X<Origin.X-825.f,
            TEXT("unobstructed_melee_actually_steps_forward"),FString::Printf(TEXT("forward displacement %.1fcm"),Player->GetActorLocation().X-Origin.X+900.f));
        PlacePlayer(Origin+FVector(-900,0,93));TargetBefore=Target->Health;Player->PressFire();Player->ReleaseFire();Go(EStep::MeleeSecond);break;
    case EStep::MeleeSecond:
        if(PhaseAge<.6f)break;
        SecondMeleeDamage=TargetBefore-Target->Health;
        Check(SecondMeleeDamage>0,TEXT("second_tap_connects"));
        PlacePlayer(Origin+FVector(-900,0,93));TargetBefore=Target->Health;Player->PressFire();Player->ReleaseFire();Go(EStep::MeleeThird);break;
    case EStep::MeleeThird:
        if(PhaseAge<.8f)break;
        Check(TargetBefore-Target->Health>FMath::Max(FirstMeleeDamage,SecondMeleeDamage)*1.2f,TEXT("third_tap_has_stronger_payoff"));
        ResetPlayer(Origin+FVector(-900,0,93));TargetBefore=Target->Health;
        Player->GetCharacterMovement()->SetMovementMode(MOVE_Walking);
        Cover=MakeBox(Origin+FVector(-835,0,140),FVector(15,200,140));
        Player->PressFire();Player->ReleaseFire();Go(EStep::MeleeCover);break;
    case EStep::MeleeCover:
        if(PhaseAge<.6f)break;
        Check(Target->Health==TargetBefore&&Player->GetActorLocation().X<Origin.X-875.f&&Player->GetActorLocation().X>Origin.X-899.f,TEXT("melee_and_forward_step_respect_wall"));
        Target->Destroy();Cover->Destroy();ResetPlayer(Origin+FVector(-900,0,93));Go(EStep::Storm);break;
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
        Check(Same, TEXT("same_seed_reproduces_route_and_gardens"), OriginalLayout);
        bool Changed = false;
        for (int32 I = 1; I <= 3 && !Changed; ++I) { Mode->Seed = OriginalSeed + I; Mode->BuildWorld(); Changed = OriginalLayout != Mode->LayoutSignature; }
        Check(Changed, TEXT("alternate_seed_changes_layout"), TEXT("Up to three nearby seeds sampled; not exhaustive procedural validation."));
        Mode->Seed = OriginalSeed; Mode->StartingPattern = NAME_None; Mode->LearnedPatterns.Reset(); Mode->StartNewRun(true);
        Player->GetCharacterMovement()->DisableMovement(); CheckRouteGeometry();
        {
            TMap<FName,int32> Families;
            TArray<FVector> ArchSites;
            bool NewOnly = true, Aligned = true;
            for (const auto& Pair : Mode->MeshBatches) {
                const auto* Batch = Pair.Value;
                if (!Batch || !Batch->GetStaticMesh() || Batch->GetInstanceCount() == 0) continue;
                const UStaticMesh* Model = Batch->GetStaticMesh();
                NewOnly &= Model->GetPathName().StartsWith(TEXT("/Game/Art/Reverie/"));
                NewOnly &= Model->GetFName()!=FName(TEXT("SM_RV_RockBank"))&&Model->GetFName()!=FName(TEXT("SM_RV_Cliff"))
                    &&Model->GetFName()!=FName(TEXT("SM_RV_Tree"));
                Families.FindOrAdd(Model->GetFName()) += Batch->GetInstanceCount();
                if (Model->GetFName() == FName(TEXT("SM_RV_Arch"))) for (int32 Instance = 0; Instance < Batch->GetInstanceCount(); ++Instance) {
                    FTransform Transform; Batch->GetInstanceTransform(Instance, Transform, true); ArchSites.Add(Transform.GetLocation());
                    bool OnLink = false;
                    for (int32 I = 1; I < Mode->Rooms.Num(); ++I) {
                        const FVector A = Mode->Rooms[I-1].Center, B = Mode->Rooms[I].Center, D = (B-A).GetSafeNormal2D();
                        OnLink |= (Transform.GetLocation().Equals(A+D*1800.f, 1.f) || Transform.GetLocation().Equals(B-D*1800.f, 1.f))
                            && FMath::Abs(FVector::DotProduct(Transform.GetUnitAxis(EAxis::Y), D)) > .99f;
                    }
                    Aligned &= OnLink;
                }
            }
            FString Missing;
            for (const TCHAR* Name : {TEXT("SM_RV_Tile"),TEXT("SM_RV_TileB"),TEXT("SM_RV_Wall"),TEXT("SM_RV_Arch"),
                TEXT("SM_RV_Bridge"),TEXT("SM_RV_TerrainPatch"),TEXT("SM_RV_SculptedBank"),TEXT("SM_RV_CrownTree"),
                TEXT("SM_RV_Fern"),TEXT("SM_RV_Grass"),TEXT("SM_RV_Ivy"),TEXT("SM_RV_Flowers"),TEXT("SM_RV_CrystalCluster"),
                TEXT("SM_RV_Basin"),TEXT("SM_RV_Rill"),TEXT("SM_RV_FountainHero"),TEXT("SM_RV_Waterfall"),
                TEXT("SM_RV_WaterDisc"),TEXT("SM_RV_WaterPlane"),TEXT("SM_RV_SkyDome"),TEXT("SM_RV_Steps")})
                if (Families.FindRef(FName(Name)) <= 0) Missing += FString(Name) + TEXT(" ");
            for (const auto& R : Mode->Rooms) {
                const auto* Altar = IsValid(R.Altar) ? R.Altar->FindComponentByClass<UStaticMeshComponent>() : nullptr;
                NewOnly &= Altar && Altar->GetStaticMesh() && Altar->GetStaticMesh()->GetFName() == FName(TEXT("SM_RV_WardCrystal"));
                for (const auto* Gate : R.Gates) {
                    const auto* Mesh = IsValid(Gate) ? Gate->FindComponentByClass<UStaticMeshComponent>() : nullptr;
                    NewOnly &= Mesh && Mesh->GetStaticMesh() && Mesh->GetStaticMesh()->GetFName() == FName(TEXT("SM_RV_Gate"));
                }
            }
            Check(NewOnly && Missing.IsEmpty() && Families.FindRef(FName(TEXT("SM_RV_CrownTree")))==Mode->Rooms.Num()*2,
                TEXT("new_environment_assets_actually_instanced"),
                FString::Printf(TEXT("%d loaded/instanced mesh families; two CrownTrees per room; no legacy paths or retired Blender tree/rock/cliff instances; new individual ward/gate actors. Missing: %s"), Families.Num(), *Missing));
            for (int32 I=1; I<Mode->Rooms.Num(); ++I) {
                const FVector A=Mode->Rooms[I-1].Center, B=Mode->Rooms[I].Center, D=(B-A).GetSafeNormal2D();
                for (const FVector& Expected : {A+D*1800.f, B-D*1800.f}) {
                    int32 Matches=0; for (const FVector& Site : ArchSites) if (Site.Equals(Expected,1.f)) ++Matches;
                    Aligned &= Matches==1;
                }
            }
            Check(Aligned && ArchSites.Num() == 4 && Families.FindRef(FName(TEXT("SM_RV_Bridge"))) == 2,
                TEXT("every_arch_serves_connected_route"), TEXT("Exactly four gateway instances, each on a selected room edge with its open local-Y traversal axis aligned to one of two bridges."));

            bool ClearPads = true, PadFloors = true, TerraceFloors = true;
            int32 PadCount = 0; FString PadFailures;
            int32 TreadCount = 0, GoodTreads = 0; FString StairFailures;
            FCollisionQueryParams P(SCENE_QUERY_STAT(DBReveriePads),false,Player);
            for (int32 I = 0; I < Mode->Rooms.Num(); ++I) {
                const FVector C = Mode->Rooms[I].Center;
                TArray<FVector> Pads = {Mode->GetRoomEntryPoint(I), C+FVector(420,-580,110), C+FVector(1260,660,110),
                    C+FVector(1260,760,110), C+FVector(1260,560,110), C+FVector(-340,860,110),
                    C+FVector(-500,-500,110), C+FVector(-500,-80,110), C+FVector(700,-650,110)};
                if (IsValid(Mode->Rooms[I].Altar)) {
                    FVector Approach = Mode->Rooms[I].Altar->GetActorLocation();
                    Approach += (C-Approach).GetSafeNormal2D()*200.f; Approach.Z=110.f; Pads.Add(Approach);
                }
                for (const FVector& Point : Pads) {
                    const bool Free = !GetWorld()->OverlapBlockingTestByChannel(Point,FQuat::Identity,ECC_Pawn,FCollisionShape::MakeCapsule(32.f,88.f),P);
                    FHitResult Floor;
                    const bool HasFloor = GetWorld()->LineTraceSingleByChannel(Floor,Point,Point-FVector(0,0,170),ECC_Visibility,P)
                        && Floor.ImpactNormal.Z>.65f && Floor.ImpactPoint.Z>=-30.f && Floor.ImpactPoint.Z<=2.f;
                    ClearPads &= Free; PadFloors &= HasFloor; ++PadCount;
                    if (!Free || !HasFloor) PadFailures += FString::Printf(TEXT("room%d %s clear%d floor%d; "),I,*Point.ToCompactString(),Free,HasFloor);
                }
                if (I>0) for (int32 Tread=0; Tread<8; ++Tread) {
                    const FVector Point=C+FVector(-337.5f-Tread*75.f,-1200,330);
                    FHitResult Floor;
                    const float Expected=20.f*(Tread+1);
                    const bool Hit=GetWorld()->LineTraceSingleByChannel(Floor,Point,Point-FVector(0,0,400),ECC_Visibility,P);
                    const bool Good=Hit&&FMath::IsNearlyEqual(Floor.ImpactPoint.Z,Expected,2.f)&&Floor.ImpactNormal.Z>.65f;
                    TerraceFloors &= Good; ++TreadCount; GoodTreads+=Good?1:0;
                    if(!Good){
                        const auto* Component=Cast<UStaticMeshComponent>(Floor.GetComponent());
                        StairFailures+=FString::Printf(TEXT("room%d tread%d at%s expected%.1f hit%d mesh%s actual%s normal%s; "),I,Tread,
                            *Point.ToCompactString(),Expected,Hit,Component&&Component->GetStaticMesh()?*Component->GetStaticMesh()->GetName():*GetNameSafe(Floor.GetActor()),
                            *Floor.ImpactPoint.ToCompactString(),*Floor.ImpactNormal.ToCompactString());
                    }
                }
            }
            Check(ClearPads && PadFloors,TEXT("spawn_combat_and_ward_approaches_clear"),
                FString::Printf(TEXT("%d capsule-overlap and physical floor probes at retained entry, encounter, practice and ward approach positions. %s"),PadCount,*PadFailures));
            Check(TerraceFloors,TEXT("terrace_stairs_have_physical_treads"),FString::Printf(
                TEXT("%d/%d physical tread probes find successive20cm rises. No native-input traversal claim. %s"),GoodTreads,TreadCount,*StairFailures));
        }
        if (IsTrellisArtCheck()) { Finish(false); return; }
        Go(EStep::FirstEncounter); break;
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
    const bool bArtCheck = !bMovementCheck && IsTrellisArtCheck();
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetStringField(TEXT("suite"), bMovementCheck ? TEXT("grounded-movement-v1") : bArtCheck ? TEXT("reverie-environment-route-v2") : TEXT("folding-shield-combat-route-v2"));
    Report->SetStringField(TEXT("scope"), bMovementCheck
        ? TEXT("Ordinary world ticks and real CharacterMovement displacement/collision on a staged level runway and solid wall. Public sprint/dash APIs and AddMovementInput exercise stamina exhaustion/recovery, blocked movement, dash travel, cooldown/held edge, pause and run reset. Isolated QA journals backed up/restored. No legacy combat/route cases, native input, visual or feel evidence.")
        : bArtCheck
        ? TEXT("Seeded route/garden reproduction and variation, three capsule lanes through each gateway/bridge, continuous floors, physical closed gates and enclosing walls, actual new asset instances, functional arch placement, retained spawn/combat/ward pads and side stair floor profiles. Gates are excluded only from passage queries. Isolated QA profile backed up/restored; no combat fixture, visual judgment or ordinary play.")
        : TEXT("Staged ordinary world ticks, real piece sweeps/input APIs, AI-created counter stance then frozen, artificial positions/lethal hits and explicit ActivateRoom staging. No OS input, normal journey or fun evidence."));
    if (bArtCheck || bMovementCheck)
    {
        Report->SetBoolField(TEXT("combat_tested"), false);
        Report->SetBoolField(TEXT("ordinary_play_tested"), false);
    }
    Report->SetStringField(TEXT("engine"), FEngineVersion::Current().ToString());
    Report->SetStringField(TEXT("executable"), FPlatformProcess::ExecutableName());
    Report->SetStringField(TEXT("platform"), TEXT("Windows"));
    Report->SetStringField(TEXT("save_slot"), IsValid(Mode) ? Mode->SlotBase : TEXT("unavailable"));
    Report->SetNumberField(TEXT("seed"), OriginalSeed);
    Report->SetBoolField(TEXT("null_rhi"), FParse::Param(FCommandLine::Get(), TEXT("NullRHI")));
    Report->SetBoolField(TEXT("complete"), bComplete);
    Report->SetBoolField(TEXT("aborted"), bAborted);
    Report->SetNumberField(TEXT("last_step"), bMovementCheck ? int32(MovementProbe.Step) : int32(Step));
    Report->SetNumberField(TEXT("elapsed_wall_seconds"), FPlatformTime::Seconds() - StartedAt);
    if (bMovementCheck)
    {
        Report->SetNumberField(TEXT("elapsed_tick_seconds"), MovementProbe.Elapsed);
        Report->SetNumberField(TEXT("observed_ticks"), MovementProbe.ObservedTicks);
        Report->SetNumberField(TEXT("minimum_tick_seconds"), MovementProbe.ObservedTicks ? MovementProbe.MinTickSeconds : 0.f);
        Report->SetNumberField(TEXT("maximum_tick_seconds"), MovementProbe.MaxTickSeconds);
    }
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
    Report->SetStringField(TEXT("not_run"), bMovementCheck
        ? (bAborted ? TEXT("Remaining movement steps after last_step; combat/immunity, slopes/stairs/ledges, native keyboard/focus, ordinary play, graphics, audio, feel and performance.")
                    : TEXT("Combat/immunity, slopes/stairs/ledges, native keyboard/focus, ordinary play, graphics, audio, feel and performance."))
        : bArtCheck
        ? (bAborted ? TEXT("Remaining art checks after last_step; all combat checks, ordinary inputs/play, rendered graphics, audio/feel and performance.")
                   : TEXT("All combat checks, ordinary inputs/play, rendered graphics, audio/feel and performance."))
        : (bAborted ? TEXT("Remaining steps after last_step; ordinary inputs, graphics/audio/feel and an unstaged full journey.")
                    : TEXT("Ordinary inputs, graphics/audio/feel, performance and an unstaged full journey.")));
    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Report, Writer);
    const FString Directory = FPaths::ProjectSavedDir() / TEXT("QA");
    IFileManager::Get().MakeDirectory(*Directory, true);
    const FString Path = Directory / (bMovementCheck ? TEXT("movement-checks.json") : bArtCheck ? TEXT("reverie-art-checks.json") : TEXT("combat-feel-checks.json"));
    const bool bWritten = FFileHelper::SaveStringToFile(Json, *Path);
    UE_LOG(LogTemp, Display, TEXT("DB_SHIELD_QA_RESULT complete=%d aborted=%d passed=%d failed=%d written=%d path=%s"),
        bComplete, bAborted, Passed, Failed, bWritten, *Path);
}
