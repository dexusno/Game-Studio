#include "DBRuntimeChecks.h"
#include "DBGameMode.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Controller.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"

namespace
{
struct FJournalCopy
{
    FString Slot;
    TArray<uint8> Bytes;
    bool bExisted = false;
};

TArray<ADBEnemy*> LivingEnemies(UWorld* World, int32 Room)
{
    TArray<ADBEnemy*> Result;
    for (TActorIterator<ADBEnemy> It(World); It; ++It)
        if (!It->bDead && It->RoomId == Room) Result.Add(*It);
    return Result;
}

void DefeatStagedWave(UWorld* World, ADBCharacter* Player, int32 Room)
{
    // Snapshot before damage so newly spawned reinforcements are not killed by this call.
    const TArray<ADBEnemy*> Targets = LivingEnemies(World, Room);
    for (ADBEnemy* Enemy : Targets)
    {
        FDBHit Hit;
        Hit.Damage = 100000.f;
        Hit.bSecondary = true;
        Hit.Source = Player->GetActorLocation();
        Hit.InstigatorActor = Player;
        Enemy->ApplyCombatHit(Hit);
    }
}
}

FIntPoint DBRunRuntimeChecks(ADBGameMode& Mode, FString& Report)
{
    FIntPoint Counts(0, 0);
    auto Check = [&](bool Good, const FString& Name, const FString& Detail = FString())
    {
        if (Good) ++Counts.X; else ++Counts.Y;
        Report += FString::Printf(TEXT("%s runtime / %s%s%s\n"), Good ? TEXT("PASS") : TEXT("FAIL"),
            *Name, Detail.IsEmpty() ? TEXT("") : TEXT(" / "), *Detail);
    };
    const bool bIsolated = FParse::Param(FCommandLine::Get(), TEXT("DBVerify"))
        && (Mode.SlotBase.StartsWith(TEXT("DBQA_")) || Mode.SlotBase.StartsWith(TEXT("DreamboundQA")))
        && !Mode.SlotBase.Contains(TEXT("/")) && !Mode.SlotBase.Contains(TEXT("\\"))
        && !Mode.SlotBase.Contains(TEXT(".."));
    Check(bIsolated, TEXT("explicit QA flag and isolated journal required"), Mode.SlotBase);
    if (!bIsolated) return Counts;
    UWorld* World = Mode.GetWorld();
    ADBCharacter* Player = Mode.Player;
    Check(World && Player && Player->GetController() && Player->ViewCamera && Mode.Rooms.Num() == 8,
        TEXT("actual world, character and camera available"));
    if (!World || !Player || !Player->GetController() || !Player->ViewCamera || Mode.Rooms.Num() != 8) return Counts;

    TArray<FJournalCopy> Backup;
    for (const FString& Suffix : {FString(TEXT("_0")), FString(TEXT("_1")), FString(TEXT("_settings"))})
    {
        FJournalCopy Copy;
        Copy.Slot = Mode.SlotBase + Suffix;
        Copy.bExisted = UGameplayStatics::DoesSaveGameExist(Copy.Slot, 0);
        if (Copy.bExisted && !UGameplayStatics::LoadDataFromSlot(Copy.Bytes, Copy.Slot, 0))
        {
            Check(false, TEXT("could not back up isolated journal; no scenarios started"), Copy.Slot);
            return Counts;
        }
        Backup.Add(MoveTemp(Copy));
    }
    Report += TEXT("SCENARIO NOTE: staged actors/positions and manual character ticks; no OS input, ordinary journey or fun claim.\n");

    auto OpenPlay = [&]()
    {
        Mode.bTitle = Mode.bPaused = Mode.bChoosingReward = Mode.bShowingBuild = Mode.bDefeated = Mode.bWon = false;
        Mode.SetMenuInput(false);
    };
    auto ResetPlayer = [&]()
    {
        OpenPlay();
        Player->Upgrades.Reset();
        Player->CurrentElement = EDBElement::Neutral;
        Player->OnRunReset();
        Player->GetController()->SetControlRotation(FRotator::ZeroRotator);
        Player->ViewCamera->SetWorldRotation(FRotator::ZeroRotator);
    };
    Mode.LearnedPatterns.Reset(); Mode.StartingPattern = NAME_None; Mode.bBossWon = false;
    Mode.StartNewRun(true);
    ResetPlayer();
    const FVector ForwardSource = Player->ViewCamera->GetComponentLocation() + Player->GetAimDirection() * 1000.f;
    const FVector RearSource = Player->ViewCamera->GetComponentLocation() - Player->GetAimDirection() * 1000.f;

    Player->ApplyUpgrade(TEXT("Mirror"));
    Player->PressGuard(); Player->Tick(.05f);
    const float PerfectHealth = Player->Health;
    Player->ReceiveAttack(20.f, ForwardSource);
    const float PostPerfectGuard = Player->GuardEnergy;
    Player->ReceiveAttack(20.f, ForwardSource);
    Check(FMath::IsNearlyEqual(Player->Health, PerfectHealth) && Player->StoredShots == 1
        && Player->GuardEnergy < PostPerfectGuard,
        TEXT("front timed guard captures once; the same opening then spends held guard"));
    Player->ReleaseGuard(); Player->PressGuard();
    Player->ReceiveAttack(20.f, ForwardSource);
    Check(!Player->bGuarding && Player->Health < PerfectHealth && Player->StoredShots == 1,
        TEXT("immediate re-raise cannot manufacture another perfect window"));

    ResetPlayer(); Player->ApplyUpgrade(TEXT("Mirror")); Player->PressGuard(); Player->Tick(.30f);
    const float HeldHealth = Player->Health, HeldEnergy = Player->GuardEnergy;
    Player->ReceiveAttack(20.f, ForwardSource);
    Check(FMath::IsNearlyEqual(Player->Health, HeldHealth) && Player->GuardEnergy < HeldEnergy && Player->StoredShots == 0,
        TEXT("late rank-one guard blocks without capture"));
    ResetPlayer(); Player->ApplyUpgrade(TEXT("Mirror")); Player->PressGuard();
    Player->ReceiveAttack(20.f, RearSource);
    Check(Player->Health < Player->MaxHealth && Player->StoredShots == 0, TEXT("rear attack bypasses a forward guard"));
    ResetPlayer(); Player->ApplyUpgrade(TEXT("Mirror")); Player->PressGuard();
    Player->ReceiveAttack(20.f, ForwardSource, true);
    Check(Player->Health < Player->MaxHealth && Player->StoredShots == 0, TEXT("unblockable attack is not captured by timed guard"));

    ResetPlayer(); Player->ApplyUpgrade(TEXT("Mirror")); Player->PressGuard();
    Player->ReceiveAttack(20.f, ForwardSource); Player->ReleaseGuard();
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ADBEnemy* Target = World->SpawnActor<ADBEnemy>(Player->GetActorLocation() + FVector(600.f, 0.f, 0.f), FRotator::ZeroRotator, Spawn);
    if (Target)
    {
        Target->Configure(EDBEnemyKind::Caster, 0, 1.f);
        Target->Health = Target->MaxHealth = 1000.f;
        Player->UseSpecial();
        const float FirstReturnedHealth = Target->Health;
        Player->UseSpecial();
        Check(FirstReturnedHealth < 1000.f && FMath::IsNearlyEqual(Target->Health, FirstReturnedHealth)
            && Player->StoredShots == 0 && Player->SpecialCooldown > 0.f,
            TEXT("captured force hits the aimed spawned target and cannot double-consume on Q spam"),
            FString::Printf(TEXT("target health %.1f"), Target->Health));
        Target->Destroy();
    }
    else Check(false, TEXT("spawned target for aimed captured-force check"));
    Mode.TogglePause();
    const float PausedHeat = Player->Heat;
    Player->PressFire(); Player->PressGuard(); Player->UseSpecial();
    Check(!Player->bGuarding && FMath::IsNearlyEqual(Player->Heat, PausedHeat), TEXT("paused input entry points do not act"));
    Mode.TogglePause();
    Player->Tick(.1f);
    Check(!Player->bGuarding && Player->Heat <= PausedHeat, TEXT("resume does not retain fire or guard intent"));

    // Real collision queries, with gates explicitly opened as a geometry fixture.
    bool bCorridorsClear = true, bFloorsPresent = true, bTechEdgesClosed = true;
    FString GeometryFailures;
    for (int32 LayoutSeed : {13001, 13002})
    {
        Mode.Seed = LayoutSeed; Mode.ClaimedRooms = {0, 1, 2, 3, 4, 5, 6, 7}; Mode.BuildWorld();
        FCollisionQueryParams Query(SCENE_QUERY_STAT(DBQACorridors), false, Player);
        for (int32 Room = 1; Room < Mode.Rooms.Num(); ++Room)
        {
            const FVector A = Mode.Rooms[Mode.Rooms[Room].Parent].Center, B = Mode.Rooms[Room].Center;
            const FVector Direction = (B - A).GetSafeNormal();
            const FVector Start = A + Direction * 1200.f + FVector(0, 0, 100.f);
            const FVector End = B - Direction * 1200.f + FVector(0, 0, 100.f);
            FHitResult Hit;
            if (World->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Pawn,
                FCollisionShape::MakeCapsule(32.f, 92.f), Query))
            {
                bCorridorsClear = false;
                const UStaticMeshComponent* Component = Cast<UStaticMeshComponent>(Hit.GetComponent());
                GeometryFailures += FString::Printf(TEXT("seed%d room%d blocked by %s; "), LayoutSeed, Room,
                    Component && Component->GetStaticMesh() ? *Component->GetStaticMesh()->GetName() : *GetNameSafe(Hit.GetActor()));
            }
            for (int32 Step = 0; Step <= 8; ++Step)
            {
                const FVector P = FMath::Lerp(Start, End, Step / 8.f);
                if (!World->LineTraceSingleByChannel(Hit, P + FVector(0, 0, 80.f), P - FVector(0, 0, 180.f), ECC_Visibility, Query))
                {
                    bFloorsPresent = false;
                    GeometryFailures += FString::Printf(TEXT("seed%d room%d floor gap at sample%d; "), LayoutSeed, Room, Step);
                }
            }
        }
        for (int32 TechRoom : {0, 6})
        {
            const FVector Start = Mode.Rooms[TechRoom].Center + FVector(600.f, 1250.f, 100.f);
            const FVector End = Start + FVector(0.f, 350.f, 0.f);
            if (!World->SweepTestByChannel(Start, End, FQuat::Identity, ECC_Pawn, FCollisionShape::MakeCapsule(32.f, 92.f), Query))
            {
                bTechEdgesClosed = false;
                GeometryFailures += FString::Printf(TEXT("seed%d tech room%d panel gap; "), LayoutSeed, TechRoom);
            }
        }
    }
    Check(bCorridorsClear, TEXT("opened corridor centerlines pass the actual player capsule on both layout parities"), GeometryFailures);
    Check(bFloorsPresent, TEXT("all corridor floor samples hit physical ground on both layout parities"));
    Check(bTechEdgesClosed, TEXT("tech-panel seams block capsule escape on both layout parities"));

    Mode.LearnedPatterns.Reset(); Mode.StartingPattern = NAME_None; Mode.StartNewRun(true);
    for (int32 Room : {3, 4})
    {
        Mode.ClearedRooms.AddUnique(Room - 1); Mode.ClaimedRooms.AddUnique(Room - 1);
        Player->SetActorLocation(Mode.Rooms[Room].Center + FVector(-1000.f, 0.f, 100.f));
        Mode.ActivateRoom(Room);
        const int32 FirstCount = LivingEnemies(World, Room).Num();
        DefeatStagedWave(World, Player, Room);
        const int32 SecondCount = LivingEnemies(World, Room).Num();
        Mode.ActivateRoom(Room - 1); Mode.ActivateRoom(Room);
        const bool bPreserved = Mode.CurrentWave == 1 && LivingEnemies(World, Room).Num() == SecondCount;
        DefeatStagedWave(World, Player, Room);
        const int32 FinalKills = Mode.Kills;
        Mode.ActivateRoom(Room - 1); Mode.ActivateRoom(Room);
        Check(FirstCount > 0 && SecondCount > 0 && bPreserved && LivingEnemies(World, Room).IsEmpty()
            && Mode.ClearedRooms.Contains(Room) && Mode.Kills == FinalKills,
            FString::Printf(TEXT("room%d second-wave re-entry finishes once without extra spawning"), Room));
    }

    ResetPlayer(); Mode.LearnedPatterns.Reset(); Mode.StartingPattern = NAME_None;
    Mode.ClearedRooms.AddUnique(7); Mode.ClaimedRooms.Remove(7); Mode.ShowOffers(7);
    const int32 MirrorOffer = Mode.Offers.IndexOfByPredicate([](const FDBOffer& Offer) { return Offer.Id == TEXT("Mirror"); });
    Mode.ChooseReward(MirrorOffer);
    Mode.ChooseReward(MirrorOffer);
    Check(Player->GetUpgradeRank(TEXT("Mirror")) == 1 && Mode.ClaimedRooms.Contains(7)
        && Mode.LearnedPatterns.Contains(TEXT("Mirror")) && Mode.StartingPattern == TEXT("Mirror"),
        TEXT("first earned pattern is ready for replay; repeated choice cannot claim it twice"));
    Player->ApplyUpgrade(TEXT("Mirror")); Player->ApplyUpgrade(TEXT("Mirror"));
    Mode.ShowOffers(7);
    Check(Mode.Offers.Num() == 2 && !Mode.Offers.ContainsByPredicate([](const FDBOffer& Offer) { return Offer.Id == TEXT("Mirror"); }),
        TEXT("capped Mirror is absent from trial choices"));
    for (FName Id : {FName(TEXT("Storm")), FName(TEXT("Echo"))})
        for (int32 Rank = 0; Rank < 3; ++Rank) Player->ApplyUpgrade(Id);
    Mode.ClaimedRooms.Remove(7); Mode.ShowOffers(7);
    const bool bRestoration = Mode.Offers.Num() == 1 && Mode.Offers[0].Id == TEXT("Restore");
    Player->Health = 41.f; Player->GuardEnergy = 17.f; Mode.ChooseReward(0);
    Check(bRestoration && FMath::IsNearlyEqual(Player->Health, Player->MaxHealth)
        && FMath::IsNearlyEqual(Player->GuardEnergy, Player->MaxGuardEnergy) && !Mode.LearnedPatterns.Contains(TEXT("Restore")),
        TEXT("fully capped trial restores resources without inventing a learned attachment"));
    const int32 PracticeSeed = Mode.Seed;
    Mode.bBossWon = true; Mode.StartNewRun(true);
    Check(Mode.Seed == PracticeSeed && Mode.LearnedPatterns.Contains(TEXT("Mirror")) && Mode.bBossWon
        && Player->Upgrades.Num() == 1 && Player->GetUpgradeRank(TEXT("Mirror")) == 1
        && Player->CurrentElement == EDBElement::Neutral && Player->StoredShots == 0
        && FMath::IsNearlyEqual(Player->Health, Player->MaxHealth) && Mode.ClaimedRooms.IsEmpty(),
        TEXT("same-seed replay resets the attempt and installs only the retained selected pattern"));

    // Journal reconstruction uses real serialization/loading and the ordinary ResumeRun path.
    Player->ApplyUpgrade(TEXT("Frost")); Player->ApplyUpgrade(TEXT("Frost")); Player->ApplyUpgrade(TEXT("Echo"));
    Player->Health = 73.f; Player->CurrentElement = EDBElement::Frost;
    Mode.LearnedPatterns.AddUnique(TEXT("Frost")); Mode.LearnedPatterns.AddUnique(TEXT("Echo"));
    Mode.CurrentRoomId = 2; Mode.ClearedRooms = {0, 1}; Mode.ClaimedRooms = {0, 1}; Mode.SaveProgress(true);
    Mode.StoredSave = nullptr; Mode.LoadProgress(); Mode.ResumeRun();
    Check(!Mode.bSaveFailed && Mode.CurrentRoomId == 2 && Mode.ClaimedRooms.Contains(1)
        && Player->GetUpgradeRank(TEXT("Frost")) == 2 && Player->GetUpgradeRank(TEXT("Echo")) == 1
        && Player->CurrentElement == EDBElement::Frost && FMath::IsNearlyEqual(Player->Health, 73.f),
        TEXT("journal reload reconstructs checkpoint claims, rank, active core and health"));
    Player->Health = 73.f; Mode.SaveProgress(true);
    const int32 RetainedRevision = Mode.SaveRevision;
    Player->Health = 61.f; Player->ApplyUpgrade(TEXT("Ram")); Mode.LearnedPatterns.AddUnique(TEXT("Ram")); Mode.SaveProgress(true);
    const FString NewestSlot = Mode.SlotBase + FString::Printf(TEXT("_%d"), Mode.SaveRevision % 2);
    // SaveDataToSlot rejects empty arrays. Corrupt one checksum byte in the current
    // 12-byte journal envelope instead. Keep the serialized payload valid so even a
    // missing integrity check cannot feed arbitrary legacy headers to Unreal's loader.
    TArray<uint8> InvalidSave;
    const bool bReadNewest = UGameplayStatics::LoadDataFromSlot(InvalidSave, NewestSlot, 0);
    bool bCorrupted = false;
    if (bReadNewest && InvalidSave.Num() >= 44)
    {
        InvalidSave[8] ^= 0x01;
        bCorrupted = UGameplayStatics::SaveDataToSlot(InvalidSave, NewestSlot, 0);
    }
    Mode.StoredSave = nullptr; Mode.LoadProgress();
    const bool bFallback = Mode.StoredSave && Mode.StoredSave->Revision == RetainedRevision;
    const int32 SelectedRevision = Mode.StoredSave ? Mode.StoredSave->Revision : -1;
    if (bFallback) Mode.ResumeRun();
    Check(bCorrupted && bFallback && FMath::IsNearlyEqual(Player->Health, 73.f)
        && !Player->HasUpgrade(TEXT("Ram")) && !Mode.LearnedPatterns.Contains(TEXT("Ram"))
        && Player->GetUpgradeRank(TEXT("Frost")) == 2 && Mode.bBossWon,
        TEXT("corrupt newest journal falls back to the previous usable checkpoint and equipment"),
        FString::Printf(TEXT("mutation=%d bytes=%d expected revision=%d selected=%d health=%.1f ram=%d learned ram=%d frost=%d boss=%d"),
            bCorrupted, InvalidSave.Num(), RetainedRevision, SelectedRevision, Player->Health,
            Player->HasUpgrade(TEXT("Ram")), Mode.LearnedPatterns.Contains(TEXT("Ram")),
            Player->GetUpgradeRank(TEXT("Frost")), Mode.bBossWon));

    // No QA-generated journal survives a successful check; retain any prior QA fixtures exactly.
    Mode.bTitle = true; Mode.bPaused = true; Mode.bChoosingReward = false; Mode.SetMenuInput(true);
    bool bRestored = true;
    for (const FJournalCopy& Copy : Backup)
    {
        if (Copy.bExisted) bRestored &= UGameplayStatics::SaveDataToSlot(Copy.Bytes, Copy.Slot, 0);
        else if (UGameplayStatics::DoesSaveGameExist(Copy.Slot, 0)) bRestored &= UGameplayStatics::DeleteGameInSlot(Copy.Slot, 0);
    }
    Check(bRestored, TEXT("isolated journal fixtures restored; end-play saving suppressed"));
    Report += FString::Printf(TEXT("RUNTIME RESULT %d passed %d failed\n"), Counts.X, Counts.Y);
    return Counts;
}
