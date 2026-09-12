#include "SalvageModel.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "WorkbenchRuntime.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include <limits>

using namespace MagnetSweep;
namespace
{
TArray<int32> IdRange(int32 First, int32 Count)
{
    TArray<int32> Ids; for (int32 I = First; I < First + Count; ++I) Ids.Add(I); return Ids;
}
int32 FirstMaterial(const FSalvageModel& Model, EMaterial Material)
{
    for (const auto& P : Model.GetPieces()) if (P.Material == Material && P.State == EPieceState::Available) return P.Id;
    return INDEX_NONE;
}
bool CareerFixture(FSalvageModel& Model, int32 XP)
{
    FSalvageSnapshot S = Model.GetSnapshot(); S.XP = S.Wallet = XP;
    FString Error; return Model.RestoreSnapshot(S, Error);
}
// A deterministic legal planner checks content feasibility, not gameplay quality.
// It only captures complete non-hazard groups which fit a stable batch.
FBankResult BankBestStableLoad(FSalvageModel& Model, int32 ExclusiveId = MAX_int32)
{
    TArray<int32> Candidates;
    for (const auto& P : Model.GetPieces())
        if (P.Id < ExclusiveId && P.State == EPieceState::Available && P.Material != EMaterial::HotCell) Candidates.Add(P.Id);
    Candidates.Sort([&Model](int32 A, int32 B)
    {
        const auto* PA = Model.FindPiece(A); const auto* PB = Model.FindPiece(B);
        return PA->Amount * PB->Mass == PB->Amount * PA->Mass ? A < B : PA->Amount * PB->Mass > PB->Amount * PA->Mass;
    });
    for (int32 Id : Candidates)
    {
        const auto Group = Model.GetCaptureGroup(Id); int32 Mass = 0; bool bHot = false;
        for (int32 Linked : Group) { const auto* P = Model.FindPiece(Linked); Mass += P->Mass; bHot |= P->Material == EMaterial::HotCell; }
        if (!bHot && Model.GetCargoMass() + Mass <= Model.GetCapacity()) Model.CapturePieces({Id});
    }
    return Model.BankCargo();
}
void CompleteFirstConservatively(FSalvageModel& Model)
{
    Model.CapturePieces(IdRange(0,12)); Model.BankCargo();
    Model.CapturePieces(IdRange(12,12)); Model.BankCargo();
    Model.CapturePieces({24}); Model.BankCargo();
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageCapacityTest, "MagnetSweep.Rework.CapacityAndAtomicGroups",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageCapacityTest::RunTest(const FString& Parameters)
{
    FSalvageModel Model; FString Error;
    TestTrue(TEXT("Initial career validates"), Model.CheckInvariants(Error));
    TestEqual(TEXT("Capacity starts at24kg"), Model.GetCapacity(), 24);
    TestEqual(TEXT("Maximum attached mass is150percent"), Model.GetMaxCaptureMass(), 36);
    TestEqual(TEXT("Visible copper chain has three members"), Model.GetCaptureGroup(24).Num(), 3);
    const auto Load = Model.CapturePieces(IdRange(0,14));
    TestEqual(TEXT("Moderate overload can be deliberately captured"), Load.Mass, 28);
    TestTrue(TEXT("Carrying28kg starts visible instability"), Model.IsCargoUnsafe());
    const uint32 Epoch = Model.GetEpoch();
    const auto Refused = Model.CapturePieces({24});
    TestTrue(TEXT("28kg plus9kg chain refuses entire bundle"), Refused.bCapacityRefused);
    TestEqual(TEXT("Refusal preserves cargo mass"), Model.GetCargoMass(), 28);
    TestEqual(TEXT("Refusal preserves state epoch"), Model.GetEpoch(), Epoch);
    for (int32 Id : {24,25,26}) TestTrue(TEXT("Every refused chain member remains available"), Model.FindPiece(Id)->State == EPieceState::Available);
    TestFalse(TEXT("Already captured identity cannot award twice"), Model.CapturePieces({0}).Succeeded());
    TestEqual(TEXT("Duplicate capture leaves value intact"), Model.GetCargo(), 56);
    FString Reason; TestFalse(TEXT("An overloaded load cannot evade risk in furnace"), Model.CanSmelt(Reason));
    TestTrue(TEXT("Refusal tells player to drop the whole haul"), Reason.Contains(TEXT("drop the haul")));
    TestEqual(TEXT("Refused deposit consumes no heat"), Model.BankCargo().Amount, 0);
    TestEqual(TEXT("No heat is consumed by refusal"), Model.GetHeatsRemaining(), 4);
    Model.RetryJob();
    const auto Group = Model.CapturePieces({25});
    TestEqual(TEXT("Touching a middle link captures whole connected group"), Group.PieceIds.Num(), 3);
    TestEqual(TEXT("Linked group weight is combined"), Group.Mass, 9);
    TestEqual(TEXT("Linked group value is combined"), Group.Amount, 36);
    TestTrue(TEXT("Linked capture retains valid ledger"), Model.CheckInvariants(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageRewardTest, "MagnetSweep.Rework.SmeltGoalsAndOneTimeBonuses",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageRewardTest::RunTest(const FString& Parameters)
{
    FSalvageModel Model; FString Error;
    Model.CapturePieces(IdRange(0,12)); const auto First = Model.BankCargo();
    TestEqual(TEXT("First full ordinary load visibly earns48credits"), First.CashAwarded, 48);
    TestEqual(TEXT("First smelt awards matching permanent XP"), First.XPAwarded, 48);
    TestEqual(TEXT("Exactly one furnace heat spent"), Model.GetHeatsRemaining(), 3);
    TestEqual(TEXT("Empty repeat deposit is no-op"), Model.BankCargo().CashAwarded, 0);
    TestEqual(TEXT("Empty repeat cannot consume another heat"), Model.GetHeatsRemaining(), 3);
    Model.CapturePieces(IdRange(12,12)); Model.BankCargo();
    Model.CapturePieces({24}); const auto Clear = Model.BankCargo();
    TestTrue(TEXT("Three approachable loads clear first job"), Clear.bCompletedNow);
    TestEqual(TEXT("Third load banks36 and awards80completion bonus"), Clear.CashAwarded, 116);
    TestEqual(TEXT("Quota counter records material only"), Model.GetBanked(), 132);
    TestEqual(TEXT("Wallet includes meaningful clear bonus"), Model.GetWallet(), 212);
    TestEqual(TEXT("First clear unlocks level2"), Model.GetPlayerLevel(), 2);
    TestTrue(TEXT("First clear affords one real150mod"), Model.PurchaseUpgrade(EUpgrade::Capacity));
    TestEqual(TEXT("First earned capacity change is24to32"), Model.GetCapacity(), 32);
    TestFalse(TEXT("Same first reward cannot buy every mod"), Model.PurchaseUpgrade(EUpgrade::Coil));
    Model.CapturePieces({32,33,34}); const auto Gold = Model.BankCargo();
    TestTrue(TEXT("Optional rare/alloy load earns gold"), Gold.bGoldNow);
    TestFalse(TEXT("Surplus smelt does not pay completion twice"), Gold.bCompletedNow);
    TestEqual(TEXT("Gold batch credits88salvage plus40bonus"), Gold.CashAwarded, 128);
    TestEqual(TEXT("Four actual smelts exhaust fuel"), Model.GetHeatsRemaining(), 0);
    TestTrue(TEXT("Four-heat attempt is ended"), Model.IsJobEnded());
    TestFalse(TEXT("Already completed attempt is never reclassified failed"), Model.IsJobFailed());
    TestEqual(TEXT("Repeated gold deposit cannot pay twice"), Model.BankCargo().CashAwarded, 0);
    TestEqual(TEXT("Rare core permanently enters collection when smelted"), Model.GetCollectedCores().Num(), 1);
    const int32 Wallet = Model.GetWallet(), XP = Model.GetXP();
    Model.RetryJob();
    TestEqual(TEXT("Retry preserves banked money"), Model.GetWallet(), Wallet);
    TestEqual(TEXT("Retry preserves earned XP"), Model.GetXP(), XP);
    TestEqual(TEXT("Retry preserves purchased basket"), Model.GetCapacity(), 32);
    TestEqual(TEXT("Fresh attempt starts with four heats"), Model.GetHeatsRemaining(), 4);
    TestEqual(TEXT("Fresh attempt has no credited prior salvage"), Model.GetBanked(), 0);
    TestTrue(TEXT("Full reward/retry sequence is internally valid"), Model.CheckInvariants(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageRiskTest, "MagnetSweep.Rework.PersistentFuseVentAndForecastLoss",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageRiskTest::RunTest(const FString& Parameters)
{
    FSalvageModel Model; FString Error;
    Model.CapturePieces(IdRange(0,12)); Model.BankCargo();
    Model.CapturePieces(IdRange(12,12)); Model.CapturePieces({34,35});
    TestEqual(TEXT("Full ordinary cargo plus core and hotcell is32kg"), Model.GetCargoMass(), 32);
    TestTrue(TEXT("Highest value forecast is rare core"), Model.GetAtRiskPiece() && Model.GetAtRiskPiece()->Material == EMaterial::Core);
    TestFalse(TEXT("Fuse remains recoverable at2seconds"), Model.AdvanceRisk(2.f).bTripped);
    const auto S = Model.GetSnapshot(); FSalvageModel Loaded;
    TestTrue(TEXT("Unsafe cargo and running fuse restore transactionally"), Loaded.RestoreSnapshot(S, Error));
    TestEqual(TEXT("Loading does not reset approaching risk"), Loaded.GetFuseElapsed(), 2.f);
    TestFalse(TEXT("Pause represented by no elapsed time cannot trip"), Loaded.AdvanceRisk(0.f).bTripped);
    const int32 SavedWallet = Loaded.GetWallet();
    const auto Trip = Loaded.AdvanceRisk(1.1f, {250,-100});
    TestTrue(TEXT("Remaining fuse time expires after reload"), Trip.bTripped);
    TestEqual(TEXT("Precisely forecast highest-value piece lost"), Trip.LostValue, 40);
    TestTrue(TEXT("Rare core is the destroyed salvage identity"), Trip.DestroyedPieceIds.Contains(34));
    TestTrue(TEXT("Hot cell is also safely removed by trip"), Trip.DestroyedPieceIds.Contains(35));
    TestEqual(TEXT("All ordinary unbanked cargo spills recoverably"), Trip.ReleasedPieceIds.Num(), 12);
    TestEqual(TEXT("Trip cannot touch previously banked cash"), Loaded.GetWallet(), SavedWallet);
    TestEqual(TEXT("Trip cannot touch previously banked XP"), Loaded.GetXP(), SavedWallet);
    TestEqual(TEXT("Quenching also spends one of the remaining furnace charges"), Loaded.GetHeatsRemaining(), 2);
    TestEqual(TEXT("Trip leaves no attached cargo"), Loaded.GetCargoMass(), 0);
    TestEqual(TEXT("Unbanked lost core never enters collection"), Loaded.GetCollectedCores().Num(), 0);
    TestTrue(TEXT("Trip state validates"), Loaded.CheckInvariants(Error));

    FSalvageModel Vent;
    Vent.CapturePieces(IdRange(0,12)); Vent.CapturePieces({32,35}); Vent.AdvanceRisk(1.f);
    const auto Saved = Vent.VentCargo({0,0});
    TestTrue(TEXT("Vent gives a real recoverable response"), Saved.Changed());
    TestEqual(TEXT("Rescue drops every carried piece, not an automatically sorted subset"), Saved.ReleasedPieceIds.Num(), 14);
    TestTrue(TEXT("Valuable alloy is dropped together with the cell and iron"), Vent.FindPiece(32)->State == EPieceState::Available);
    TestFalse(TEXT("Dropping the whole haul clears instability"), Vent.IsCargoUnsafe());
    TestEqual(TEXT("Dropping never destroys salvage"), Saved.DestroyedPieceIds.Num(), 0);
    TestEqual(TEXT("Dropping the whole haul ends the fuse"), Vent.GetFuseElapsed(), 0.f);
    TestEqual(TEXT("No optimized cargo remains attached"), Vent.GetCargoMass(), 0);
    TestEqual(TEXT("A timely rescue spends no fuel charge"), Vent.GetHeatsRemaining(), 4);
    TestTrue(TEXT("Vented state validates"), Vent.CheckInvariants(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageDropTest, "MagnetSweep.Rework.FullHaulDropPlacementAndRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageDropTest::RunTest(const FString& Parameters)
{
    const FVector2D Origins[] = {{-500,-310},{500,-310},{-500,310},{500,310},{0,0},{650,0}};
    FString Error;
    for (const auto& Origin : Origins)
    {
        FSalvageModel Model;
        TestTrue(TEXT("Full-capacity placement fixture has a valid funded career"), CareerFixture(Model,1500));
        for (int32 Tier=0; Tier<3; ++Tier) Model.PurchaseUpgrade(EUpgrade::Capacity);
        TArray<int32> Haul = IdRange(0,24); for (int32 Id : {24,32,34,35}) Haul.Add(Id);
        const auto Captured = Model.CapturePieces(Haul);
        TestEqual(TEXT("Dense mixed haul weighs69kg within full rig capture ceiling"), Captured.Mass, 69);
        TestTrue(TEXT("Dense mixed fixture is unsafe before rescue"), Model.IsCargoUnsafe());
        const auto BundleOffset = Model.FindPiece(26)->Position-Model.FindPiece(24)->Position;
        const int32 Money = Model.GetWallet(), XP = Model.GetXP(), Available = Model.GetAvailableAmount();
        Model.AdvanceRisk(.75f);
        const auto Drop = Model.VentCargo(Origin);
        TestTrue(TEXT("The complete captured identity set is returned"), Drop.ReleasedPieceIds == Captured.PieceIds);
        TestEqual(TEXT("Drop preserves all salvage value as recoverable material"), Model.GetAvailableAmount(), Available+Captured.Amount);
        TestEqual(TEXT("Drop leaves no cargo value"), Model.GetCargo(), 0);
        TestEqual(TEXT("Drop leaves no cargo mass"), Model.GetCargoMass(), 0);
        TestEqual(TEXT("Drop preserves credits"), Model.GetWallet(), Money);
        TestEqual(TEXT("Drop preserves XP"), Model.GetXP(), XP);
        TestEqual(TEXT("Drop does not bank an unearned core"), Model.GetCollectedCores().Num(), 0);
        TestEqual(TEXT("Drop does not burn a furnace charge"), Model.GetHeatsRemaining(), 4);
        TestEqual(TEXT("Drop does not destroy any item"), Drop.DestroyedPieceIds.Num(), 0);
        TestTrue(TEXT("Normal connected bundle preserves its relative shape at a corner"),
            (Model.FindPiece(26)->Position-Model.FindPiece(24)->Position).Equals(BundleOffset,.001));
        for (int32 I=0; I<Drop.ReleasedPieceIds.Num(); ++I)
        {
            const auto* A = Model.FindPiece(Drop.ReleasedPieceIds[I]);
            TestTrue(TEXT("Every dropped piece stays inside the tray, including corner releases"), FSalvageModel::IsInsideTray(A->Position));
            TestTrue(TEXT("Every dropped identity is available again"), A->State == EPieceState::Available);
            const auto LinkedGroup = Model.GetCaptureGroup(A->Id);
            for (int32 J=I+1; J<Drop.ReleasedPieceIds.Num(); ++J)
            {
                const auto* B = Model.FindPiece(Drop.ReleasedPieceIds[J]);
                if (LinkedGroup.Contains(B->Id)) continue;
                const double Gap = A->Material == EMaterial::HotCell || B->Material == EMaterial::HotCell ? 118. : 60.;
                TestTrue(TEXT("Separate dropped groups remain readable; cells have precision-field clearance"),
                    (A->Position-B->Position).SizeSquared()+.001 >= Gap*Gap);
            }
        }
        TestTrue(TEXT("Corner/full-haul state satisfies save invariants"), Model.CheckInvariants(Error));
        TestEqual(TEXT("A released copper bundle is still recovered atomically"), Model.CapturePieces({24}).PieceIds.Num(), 3);
        TestTrue(TEXT("Its now-stable cargo can also be deliberately dropped"), Model.VentCargo(Origin).Changed());
        TestEqual(TEXT("Stable drop also empties the whole haul"), Model.GetCargoMass(), 0);
        TestTrue(TEXT("Dropped rare core can be recovered deliberately"), Model.CapturePieces({34}).Succeeded());
        TestEqual(TEXT("Recovered core pays its actual value once"), Model.BankCargo().Amount, 40);
        TestEqual(TEXT("Only the deliberate smelt adds it to the collection"), Model.GetCollectedCores().Num(), 1);
    }
    for (const FVector2D Corner : {FVector2D(-500,-310),FVector2D(500,310)})
    {
        FSalvageModel Stress; CareerFixture(Stress,5000);
        for (int32 Tier=0; Tier<3; ++Tier) Stress.PurchaseUpgrade(EUpgrade::Capacity);
        Stress.StartJob(5,241);
        auto Haul = IdRange(0,24);
        for (const auto& P : Stress.GetPieces()) if (P.Material == EMaterial::HotCell) Haul.Add(P.Id);
        TestEqual(TEXT("Stress haul reaches the actual72kg hard ceiling with all six cells"), Stress.CapturePieces(Haul).Mass, Stress.GetMaxCaptureMass());
        const auto Drop = Stress.VentCargo(Corner);
        TestEqual(TEXT("Hard-ceiling stress returns all thirty carried pieces"), Drop.ReleasedPieceIds.Num(), 30);
        for (int32 I=0; I<Drop.ReleasedPieceIds.Num(); ++I)
        {
            const auto* A = Stress.FindPiece(Drop.ReleasedPieceIds[I]);
            TestTrue(TEXT("Maximum-load corner drop stays within tray"), FSalvageModel::IsInsideTray(A->Position));
            for (int32 J=I+1; J<Drop.ReleasedPieceIds.Num(); ++J)
            {
                const auto* B = Stress.FindPiece(Drop.ReleasedPieceIds[J]);
                const double Gap = A->Material == EMaterial::HotCell || B->Material == EMaterial::HotCell ? 118. : 60.;
                TestTrue(TEXT("Maximum load and six cells retain required separate pickup spacing"),
                    (A->Position-B->Position).SizeSquared()+.001 >= Gap*Gap);
            }
        }
        TestTrue(TEXT("Maximum-load rescue keeps a valid ledger"), Stress.CheckInvariants(Error));
    }
    FSalvageModel Quench;
    Quench.CapturePieces({24,32,34,35});
    const auto BeforeLink = Quench.FindPiece(26)->Position-Quench.FindPiece(24)->Position;
    const auto Failed = Quench.AdvanceRisk(3.1f,{500,310});
    TestTrue(TEXT("Quench still destroys its forecast core"), Failed.DestroyedPieceIds.Contains(34));
    TestTrue(TEXT("Quench remainder uses the same bundle-preserving corner placement"),
        (Quench.FindPiece(26)->Position-Quench.FindPiece(24)->Position).Equals(BeforeLink,.001));
    for (int32 Id : Failed.ReleasedPieceIds)
        TestTrue(TEXT("Quenched remainder stays inside the corner bounds"), FSalvageModel::IsInsideTray(Quench.FindPiece(Id)->Position));
    TestEqual(TEXT("Shared placement does not remove the quench fuel penalty"), Quench.GetHeatsRemaining(), 3);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageQuenchFuelTest, "MagnetSweep.Rework.OverloadConsumesExistingFuel",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageQuenchFuelTest::RunTest(const FString& Parameters)
{
    FString Error;
    FSalvageModel CellOnly;
    TestTrue(TEXT("An isolated hot cell can be captured"), CellOnly.CapturePieces({35}).Succeeded());
    const auto CellQuench = CellOnly.AdvanceRisk(3.1f);
    TestTrue(TEXT("An isolated cell expires through the actual fuse"), CellQuench.bTripped);
    TestEqual(TEXT("Hot-cell-only quench has no salvage-value loss"), CellQuench.LostValue, 0);
    TestEqual(TEXT("Zero-value quench still consumes one existing charge"), CellOnly.GetHeatsRemaining(), 3);
    TestEqual(TEXT("Quench cannot manufacture banked value"), CellOnly.GetBanked(), 0);
    TestEqual(TEXT("Quench cannot manufacture cash"), CellOnly.GetWallet(), 0);
    TestEqual(TEXT("Quench cannot manufacture XP"), CellOnly.GetXP(), 0);
    TestTrue(TEXT("Zero-bank one-quench ledger is valid"), CellOnly.CheckInvariants(Error));
    const uint32 SettledEpoch = CellOnly.GetEpoch();
    TestFalse(TEXT("An already-resolved quench is not charged twice"), CellOnly.TriggerOverload().bTripped);
    TestEqual(TEXT("Duplicate quench preserves three charges"), CellOnly.GetHeatsRemaining(), 3);
    TestEqual(TEXT("Duplicate quench is a state no-op"), CellOnly.GetEpoch(), SettledEpoch);
    FSalvageModel Restored;
    TestTrue(TEXT("Quench fuel survives the existing snapshot format"), Restored.RestoreSnapshot(CellOnly.GetSnapshot(), Error));
    TestEqual(TEXT("Restored attempt still has three charges"), Restored.GetHeatsRemaining(), 3);

    CompleteFirstConservatively(CellOnly);
    TestTrue(TEXT("Three stable batches can still clear First Pour after one quench"), CellOnly.IsDeliveryCompleted());
    TestEqual(TEXT("Post-quench conservative route still banks132"), CellOnly.GetBanked(), 132);
    TestEqual(TEXT("Post-quench clear still pays its honest212 cash"), CellOnly.GetWallet(), 212);
    TestEqual(TEXT("One quench and three smelts exhaust exactly four charges"), CellOnly.GetHeatsRemaining(), 0);
    TestTrue(TEXT("Last-charge successful smelt ends the attempt"), CellOnly.IsJobEnded());
    TestFalse(TEXT("Successful recovery is not marked failed"), CellOnly.IsJobFailed());
    TestTrue(TEXT("Mixed smelt/quench completed ledger validates"), CellOnly.CheckInvariants(Error));

    FSalvageModel Repeated;
    for (int32 HotId : {35,36})
    {
        Repeated.CapturePieces({HotId});
        TestTrue(TEXT("A second distinct cell also costs a quench"), Repeated.AdvanceRisk(3.1f).bTripped);
    }
    // The two final quenches are ordinary over-capacity loads; lost iron remains lost.
    for (int32 First = 0; First < 2; ++First)
    {
        TestEqual(TEXT("Thirteen remaining iron pieces form a26kg unstable load"), Repeated.CapturePieces(IdRange(First,13)).Mass, 26);
        TestTrue(TEXT("Ordinary overload also consumes a charge"), Repeated.AdvanceRisk(3.1f).bTripped);
    }
    TestEqual(TEXT("Repeated quenches consume all four charges"), Repeated.GetHeatsRemaining(), 0);
    TestTrue(TEXT("Final quench ends an incomplete contract"), Repeated.IsJobEnded());
    TestTrue(TEXT("Uncompleted final-quench attempt is honestly failed"), Repeated.IsJobFailed());
    TestEqual(TEXT("Repeated failure grants no cash"), Repeated.GetWallet(), 0);
    TestEqual(TEXT("Repeated failure grants no XP"), Repeated.GetXP(), 0);
    const uint32 EndedEpoch = Repeated.GetEpoch();
    TestFalse(TEXT("Ended attempt cannot quench again"), Repeated.TriggerOverload().bTripped);
    TestEqual(TEXT("Fuel never goes below zero"), Repeated.GetHeatsRemaining(), 0);
    TestEqual(TEXT("Ended attempt quench is a state no-op"), Repeated.GetEpoch(), EndedEpoch);
    TestTrue(TEXT("Four-quench zero-bank terminal state validates"), Repeated.CheckInvariants(Error));

    FSalvageModel Closed; Closed.CapturePieces({35});
    auto ClosedState = Closed.GetSnapshot(); ClosedState.bJobEnded = true;
    TestTrue(TEXT("A closed snapshot may retain an unresolved attached cell"), Closed.RestoreSnapshot(ClosedState, Error));
    TestTrue(TEXT("Closed fixture is unsafe so the ended guard is exercised"), Closed.IsCargoUnsafe());
    TestFalse(TEXT("An already-ended unsafe attempt is not mutated by quench"), Closed.TriggerOverload().bTripped);
    TestEqual(TEXT("Ended guard preserves its attached cell"), Closed.GetCargoMass(), 4);
    TestEqual(TEXT("Ended guard does not spend a charge"), Closed.GetHeatsRemaining(), 4);

    FSalvageModel Protected;
    BankBestStableLoad(Protected); BankBestStableLoad(Protected);
    TestTrue(TEXT("Two deliberate valuable batches earn completion and gold"), Protected.IsDeliveryCompleted() && Protected.IsGoldAwarded());
    TestTrue(TEXT("A genuinely earned mod can be fitted before the last charge"), Protected.PurchaseUpgrade(EUpgrade::Capacity));
    Protected.CapturePieces({FirstMaterial(Protected,EMaterial::Iron)}); Protected.BankCargo();
    TestEqual(TEXT("Protected career has one charge remaining"), Protected.GetHeatsRemaining(), 1);
    const int32 BankedBefore = Protected.GetBanked(), WalletBefore = Protected.GetWallet(), XPBefore = Protected.GetXP();
    const auto CoresBefore = Protected.GetCollectedCores();
    const int32 CompletedBefore = Protected.GetCompletedDeliveryCount();
    Protected.CapturePieces({35});
    TestTrue(TEXT("Last remaining charge can be consumed by a hot cell"), Protected.AdvanceRisk(3.1f).bTripped);
    TestTrue(TEXT("Last-charge quench ends even an already-cleared attempt"), Protected.IsJobEnded());
    TestFalse(TEXT("Last-charge quench does not retract success"), Protected.IsJobFailed());
    TestTrue(TEXT("Completion and gold remain earned"), Protected.IsDeliveryCompleted() && Protected.IsGoldAwarded());
    TestEqual(TEXT("Quench preserves banked contract value"), Protected.GetBanked(), BankedBefore);
    TestEqual(TEXT("Quench preserves awarded cash and bonuses"), Protected.GetWallet(), WalletBefore);
    TestEqual(TEXT("Quench preserves awarded XP and level"), Protected.GetXP(), XPBefore);
    TestEqual(TEXT("Quench preserves purchased capacity tier"), Protected.GetUpgradeTier(EUpgrade::Capacity), 1);
    TestTrue(TEXT("Quench preserves permanently banked core collection"), Protected.GetCollectedCores() == CoresBefore);
    TestEqual(TEXT("Quench neither duplicates nor removes a completed contract"), Protected.GetCompletedDeliveryCount(), CompletedBefore);
    TestTrue(TEXT("Protected final-charge state validates"), Protected.CheckInvariants(Error));

    // Fuel validation still requires ledger evidence; zero-value quenches are not a
    // reason to accept arbitrary spent-charge counters in an otherwise untouched save.
    FSalvageModel Pristine; auto Invalid = Pristine.GetSnapshot(); Invalid.HeatsUsed = 1;
    TestFalse(TEXT("An unexplained spent charge is rejected"), Pristine.RestoreSnapshot(Invalid, Error));
    Invalid.Pieces[35].State = EPieceState::Lost;
    TestFalse(TEXT("An untouched lost item cannot fabricate quench evidence"), Pristine.RestoreSnapshot(Invalid, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageCareerTest, "MagnetSweep.Rework.CareerPurchasesFailureAndCollection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageCareerTest::RunTest(const FString& Parameters)
{
    FSalvageModel Model; FString Error;
    TestFalse(TEXT("Locked advanced contract cannot start"), Model.StartJob(5,42));
    TestFalse(TEXT("Unaffordable locked purchase does nothing"), Model.PurchaseUpgrade(EUpgrade::Capacity));
    for (int32 I = 0; I < 4; ++I) { Model.CapturePieces({I}); Model.BankCargo(); }
    TestTrue(TEXT("Four tiny loads really fail contract"), Model.IsJobFailed());
    TestEqual(TEXT("Failed honest salvage keeps16credits"), Model.GetWallet(), 16);
    TestEqual(TEXT("Failed honest salvage keeps16XP"), Model.GetXP(), 16);
    TestFalse(TEXT("Failed job pays no clear bonus"), Model.IsDeliveryCompleted());
    TestEqual(TEXT("Failed job cannot collect more through a completed board"), Model.CapturePieces({4}).Mass, 0);
    Model.RetryJob();
    TestEqual(TEXT("Retry never rolls banked money back"), Model.GetWallet(), 16);
    TestEqual(TEXT("Retry clears only attempt counters"), Model.GetBanked(), 0);
    CompleteFirstConservatively(Model);
    TestTrue(TEXT("Real clear unlocks second contract"), Model.IsJobUnlocked(1));
    TestTrue(TEXT("First mod purchase succeeds"), Model.PurchaseUpgrade(EUpgrade::Coil));
    TestEqual(TEXT("Coil tier visibly changes field size"), Model.GetFieldRadius(), 145.f);
    TestFalse(TEXT("Tier2 remains locked at level2"), Model.PurchaseUpgrade(EUpgrade::Coil));
    TestTrue(TEXT("Completed empty job can deliberately finish"), Model.FinishJob());
    TestTrue(TEXT("Unlocked new job starts"), Model.StartJob(1,87));
    TestTrue(TEXT("Full career transition validates"), Model.CheckInvariants(Error));

    FSalvageModel Funded;
    TestTrue(TEXT("Synthetic funded career fixture is valid"), CareerFixture(Funded,5000));
    for (int32 I=0; I<3; ++I) for (EUpgrade Mod : {EUpgrade::Capacity,EUpgrade::Coil,EUpgrade::Stabilizer})
        TestTrue(TEXT("Available next tier purchases atomically"), Funded.PurchaseUpgrade(Mod));
    TestEqual(TEXT("All nine real upgrades deduct2850credits"), Funded.GetWallet(), 2150);
    TestEqual(TEXT("Spending money never spends XP"), Funded.GetXP(), 5000);
    TestEqual(TEXT("Full rig capacity48kg"), Funded.GetCapacity(), 48);
    TestEqual(TEXT("Full coil has215range"), Funded.GetFieldRadius(), 215.f);
    TestEqual(TEXT("Full stabilizer has6second fuse"), Funded.GetFuseDuration(), 6.f);
    TestFalse(TEXT("Maxed tier cannot charge money again"), Funded.PurchaseUpgrade(EUpgrade::Capacity));
    TestTrue(TEXT("Purchased ledger verifies exact expenditure"), Funded.CheckInvariants(Error));
    Funded.RetryJob(); Funded.CapturePieces({FirstMaterial(Funded,EMaterial::Core)});
    TestEqual(TEXT("Picking up relic does not bank collection"), Funded.GetCollectedCores().Num(),0);
    const auto NewFind = Funded.BankCargo();
    TestEqual(TEXT("First secure relic is new find"), NewFind.NewCoreIds.Num(),1);
    Funded.RetryJob(); Funded.CapturePieces({FirstMaterial(Funded,EMaterial::Core)});
    const auto Duplicate = Funded.BankCargo();
    TestEqual(TEXT("Repeat relic pays its honest40value"), Duplicate.Amount,40);
    TestEqual(TEXT("Repeat relic does not duplicate album"), Duplicate.NewCoreIds.Num(),0);
    TestEqual(TEXT("Collection contains one unique find"), Funded.GetCollectedCores().Num(),1);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageBreakawayTest, "MagnetSweep.Rework.BreakawaySelectionAndAtomicRefusals",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageBreakawayTest::RunTest(const FString& Parameters)
{
    FSalvageModel Model; CareerFixture(Model,5000); FString Error;
    const int32 Assembly = 37;
    TestFalse(TEXT("The original coil cannot selectively extract linked salvage"),Model.BreakawayExtract(Assembly).Succeeded());
    TestTrue(TEXT("A real coil purchase unlocks extraction"),Model.PurchaseUpgrade(EUpgrade::Coil));
    Model.CapturePieces(IdRange(0,10));
    FSalvageModel Bulk; Bulk.RestoreSnapshot(Model.GetSnapshot(),Error);
    TestEqual(TEXT("The new optional assembly is a real eight-kg linked load"),Bulk.CapturePieces({Assembly}).Mass,8);
    TestTrue(TEXT("Ordinary collection of that assembly overloads the existing twenty-kg haul"),Bulk.IsCargoUnsafe());
    const int32 Wallet=Model.GetWallet(), XP=Model.GetXP();
    const auto Extracted=Model.BreakawayExtract(Assembly);
    TestTrue(TEXT("Breakaway captures only the specifically selected alloy"),
        Extracted.PieceIds.Num()==1 && Extracted.PieceIds[0]==Assembly && Extracted.Mass==4 && Extracted.Amount==24);
    TestEqual(TEXT("Selected extraction completes a legal twenty-four-kg haul"),Model.GetCargoMass(),24);
    TestFalse(TEXT("Mass-fitting useful extraction introduces no overload"),Model.IsCargoUnsafe());
    TestEqual(TEXT("Extraction grants no money before melting"),Model.GetWallet(),Wallet);
    TestEqual(TEXT("Extraction grants no XP before melting"),Model.GetXP(),XP);
    TestEqual(TEXT("The selected use spends exactly one pulse"),Model.GetBreakawayUsesRemaining(),0);
    TestTrue(TEXT("Both unwanted weights remain available and detached"),
        Model.FindPiece(Assembly+1)->State==EPieceState::Available && Model.FindPiece(Assembly+2)->State==EPieceState::Available
        && Model.FindPiece(Assembly+1)->DirectLinks.IsEmpty() && Model.FindPiece(Assembly+2)->DirectLinks.IsEmpty());
    TestTrue(TEXT("Selected item and both weights update their link presentation"),
        Model.FindPiece(Assembly)->Kind==EPieceKind::Loose && Model.FindPiece(Assembly+1)->Kind==EPieceKind::Loose
        && Model.FindPiece(Assembly+2)->Kind==EPieceKind::Loose);
    TestTrue(TEXT("Atomic extraction preserves model invariants"),Model.CheckInvariants(Error));

    const auto RejectUnchanged=[&](FSalvageModel& M,int32 Id,const TCHAR* Label)
    {
        const auto Before=M.GetSnapshot();
        TestFalse(Label,M.BreakawayExtract(Id).Succeeded());
        const auto After=M.GetSnapshot();
        TestEqual(TEXT("Rejected extraction preserves epoch"),After.Epoch,Before.Epoch);
        TestEqual(TEXT("Rejected extraction preserves cargo mass"),After.CargoMass,Before.CargoMass);
        TestEqual(TEXT("Rejected extraction preserves pulses"),After.BreakawayUses,Before.BreakawayUses);
        TestEqual(TEXT("Rejected extraction preserves capture serial"),After.CaptureSerial,Before.CaptureSerial);
        TestEqual(TEXT("Rejected extraction preserves wallet"),After.Wallet,Before.Wallet);
        for(int32 I=0;I<Before.Pieces.Num();++I)
            TestTrue(TEXT("Rejected extraction preserves every ownership and symmetric link"),
                After.Pieces[I].State==Before.Pieces[I].State && After.Pieces[I].DirectLinks==Before.Pieces[I].DirectLinks);
    };
    RejectUnchanged(Model,Assembly+3,TEXT("An exhausted coil cannot cut another assembly"));
    FSalvageModel Refused; CareerFixture(Refused,5000); Refused.PurchaseUpgrade(EUpgrade::Coil);
    RejectUnchanged(Refused,-1,TEXT("Invalid target is refused without mutation"));
    RejectUnchanged(Refused,0,TEXT("Loose salvage cannot waste a breakaway pulse"));
    Refused.CapturePieces(IdRange(0,11));
    RejectUnchanged(Refused,Assembly,TEXT("A twenty-two-kg haul cannot accept a four-kg selected piece"));
    Refused.CapturePieces({11,12});
    RejectUnchanged(Refused,Assembly,TEXT("An already unsafe haul cannot use extraction as free rescue"));
    Refused.VentCargo(); Refused.AbandonJob();
    RejectUnchanged(Refused,Assembly,TEXT("An ended job cannot extract salvage"));

    FSalvageModel Ballast; CareerFixture(Ballast,5000); Ballast.PurchaseUpgrade(EUpgrade::Coil);
    const auto Cheap=Ballast.BreakawayExtract(Assembly+1);
    TestTrue(TEXT("Explicit ballast selection takes the chosen iron rather than auto-sorting value"),Cheap.Mass==2 && Cheap.Amount==4);
    TestTrue(TEXT("Removing one ballast link preserves the remaining alloy-to-iron link symmetrically"),
        Ballast.FindPiece(Assembly)->DirectLinks==TArray<int32>{Assembly+2}
        && Ballast.FindPiece(Assembly+2)->DirectLinks==TArray<int32>{Assembly});
    TestTrue(TEXT("Ballast choice keeps a valid partial assembly"),Ballast.CheckInvariants(Error));

    FSalvageModel Hazard; CareerFixture(Hazard,5000); Hazard.PurchaseUpgrade(EUpgrade::Coil);
    auto HazardState=Hazard.GetSnapshot();
    HazardState.Pieces[35].DirectLinks.Add(Assembly+1); HazardState.Pieces[35].Kind=EPieceKind::Tangle;
    HazardState.Pieces[Assembly+1].DirectLinks.Add(35);
    TestTrue(TEXT("A deliberate linked-cell fixture is a valid symmetric assembly"),Hazard.RestoreSnapshot(HazardState,Error));
    const auto Cell=Hazard.BreakawayExtract(35);
    TestTrue(TEXT("A deliberately selected linked hot cell carries its actual hazard and spends its pulse"),
        Cell.Succeeded() && Cell.Amount==0 && Hazard.HasHotCell() && Hazard.IsCargoUnsafe() && Hazard.GetBreakawayUsesRemaining()==0);
    TestEqual(TEXT("Selecting a cell creates no money"),Hazard.GetWallet(),HazardState.Wallet);
    TestEqual(TEXT("Selecting a cell creates no XP"),Hazard.GetXP(),HazardState.XP);
    TestTrue(TEXT("The selected hot cell retains ordinary fuse consequences"),Hazard.AdvanceRisk(3.1f).bTripped);
    TestEqual(TEXT("Cell quench does not refill the spent pulse"),Hazard.GetBreakawayUsesRemaining(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageBreakawayBudgetTest, "MagnetSweep.Rework.BreakawayTiersRefillAndSavedState",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageBreakawayBudgetTest::RunTest(const FString& Parameters)
{
    FString Error;
    for(int32 Tier=1;Tier<=3;++Tier)
    {
        FSalvageModel Model; CareerFixture(Model,5000);
        for(int32 I=0;I<Tier;++I)TestTrue(TEXT("Each real paid coil tier purchases normally"),Model.PurchaseUpgrade(EUpgrade::Coil));
        for(int32 I=0;I<Tier;++I)
        {
            TestTrue(TEXT("Each paid tier permits another independent valuable selection in this batch"),Model.BreakawayExtract(37+I*3).Succeeded());
            TestEqual(TEXT("Exactly one additional four-kg alloy is carried per selection"),Model.GetCargoMass(),(I+1)*4);
        }
        TestEqual(TEXT("Each tier supports its advertised alloy value before melting"),Model.GetCargo(),Tier*24);
        TestFalse(TEXT("No tier can exceed its paid pulses by choosing an ordinary remaining link"),Model.BreakawayExtract(24).Succeeded());
        Model.VentCargo();
        TestEqual(TEXT("Dropping cargo never refills coil pulses"),Model.GetBreakawayUsesRemaining(),0);
        FSalvageModel Reloaded;
        TestTrue(TEXT("Spent pulses and cut links restore in the existing snapshot version"),Reloaded.RestoreSnapshot(Model.GetSnapshot(),Error));
        TestEqual(TEXT("Reloading cannot refill the coil"),Reloaded.GetBreakawayUsesRemaining(),0);
        TestTrue(TEXT("Dropped selected alloy stays permanently detached after reload"),Reloaded.FindPiece(37)->DirectLinks.IsEmpty());
        FWorkbenchImpl JsonWriter(nullptr),JsonReader(nullptr);
        JsonWriter.Model.RestoreSnapshot(Reloaded.GetSnapshot(),Error);
        TestTrue(TEXT("Actual career JSON preserves the spent extraction budget"),JsonReader.DecodeSave(JsonWriter.EncodeSave()));
        TestEqual(TEXT("Actual JSON reload cannot refill spent extractions"),JsonReader.Model.GetBreakawayUsesRemaining(),0);
        TestTrue(TEXT("Actual JSON reload preserves the severed graph"),JsonReader.Model.FindPiece(37)->DirectLinks.IsEmpty());
        TestEqual(TEXT("Empty smelt is not a recharge exploit"),Reloaded.BankCargo().Amount,0);
        TestEqual(TEXT("Rejected empty smelt leaves spent pulses unchanged"),Reloaded.GetBreakawayUsesRemaining(),0);
        Reloaded.CapturePieces({37}); const auto Payout=Reloaded.BankCargo();
        TestEqual(TEXT("Actual recovered alloy must pay normally before it recharges"),Payout.Amount,24);
        TestEqual(TEXT("A successful paid smelt restores exactly the installed tier budget"),Reloaded.GetBreakawayUsesRemaining(),Tier);
        Reloaded.BreakawayExtract(24); Reloaded.CapturePieces({35});
        TestEqual(TEXT("Unsafe smelt cannot refill a partly used coil"),Reloaded.BankCargo().Amount,0);
        Reloaded.TriggerOverload();
        TestEqual(TEXT("Overload quench never refills a partly used coil"),Reloaded.GetBreakawayUsesRemaining(),Tier-1);
        const int32 Wallet=Reloaded.GetWallet(); Reloaded.RetryJob();
        TestEqual(TEXT("A new attempt restores the advertised pulse budget"),Reloaded.GetBreakawayUsesRemaining(),Tier);
        TestEqual(TEXT("New-attempt recharge does not alter banked income"),Reloaded.GetWallet(),Wallet);
        TestTrue(TEXT("Every tier and reset preserves ledger invariants"),Reloaded.CheckInvariants(Error));
    }
    FSalvageModel Old; CareerFixture(Old,5000); Old.PurchaseUpgrade(EUpgrade::Coil);
    auto OldSnapshot=Old.GetSnapshot(); OldSnapshot.Pieces.SetNum(37); // Actual old layout, no added assemblies.
    TestTrue(TEXT("Old career populations stay valid without mandatory new content"),Old.RestoreSnapshot(OldSnapshot,Error));
    TestEqual(TEXT("Loading an old attempt does not append or rebuild its tray"),Old.GetPieces().Num(),37);
    TestEqual(TEXT("Old purchased coil retains its radius benefit"),Old.GetFieldRadius(),145.f);
    FWorkbenchImpl OldWriter(nullptr),OldReader(nullptr); OldWriter.Model.RestoreSnapshot(OldSnapshot,Error);
    TSharedPtr<FJsonObject> OldJson;
    TestTrue(TEXT("Old-format fixture starts from valid career JSON"),FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(OldWriter.EncodeSave()),OldJson));
    if(OldJson.IsValid())
    {
        OldJson->GetObjectField(TEXT("snapshot"))->RemoveField(TEXT("breakaway_uses"));
        FString LegacyText; FJsonSerializer::Serialize(OldJson.ToSharedRef(),TJsonWriterFactory<>::Create(&LegacyText));
        TestTrue(TEXT("A real old JSON save without extraction metadata remains loadable"),OldReader.DecodeSave(LegacyText));
        TestEqual(TEXT("Old JSON defaults to the retained coil's unused extraction budget"),OldReader.Model.GetBreakawayUsesRemaining(),1);
        TestEqual(TEXT("Old JSON restore does not add or replace saved pieces"),OldReader.Model.GetPieces().Num(),37);
    }
    TestTrue(TEXT("Old copper links can immediately use the newly meaningful capability"),Old.BreakawayExtract(24).Succeeded());
    auto Invalid=Old.GetSnapshot(); Invalid.BreakawayUses=-1;
    TestFalse(TEXT("Negative spent pulse metadata is rejected transactionally"),Old.RestoreSnapshot(Invalid,Error));
    Invalid=Old.GetSnapshot(); Invalid.BreakawayUses=2;
    TestFalse(TEXT("Spent count above the owned tier is rejected"),Old.RestoreSnapshot(Invalid,Error));
    TestEqual(TEXT("Rejected pulse metadata leaves the live spent budget intact"),Old.GetBreakawayUsesRemaining(),0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageContentTest, "MagnetSweep.Rework.DeterministicJobsAndFeasibleLoads",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageContentTest::RunTest(const FString& Parameters)
{
    for (int32 Job=0; Job<FSalvageModel::LayoutCount; ++Job)
    {
        FSalvageModel A,B; CareerFixture(A,5000); CareerFixture(B,5000);
        TestTrue(TEXT("All authored jobs unlock at master level"), A.StartJob(Job,41));
        B.StartJob(Job,41); const auto S=A.GetSnapshot();
        const auto& Definition=A.GetJob();
        const int32 OriginalCount=Definition.IronCount+Definition.CopperCount+Definition.AlloyCount+1+Definition.CellCount;
        TestEqual(TEXT("Three optional assemblies append nine items without replacing the ordinary routes"),A.GetPieces().Num(),OriginalCount+9);
        double MinimumNewClearance=TNumericLimits<double>::Max();
        TestEqual(TEXT("Same job seed reproduces population"), A.GetPieces().Num(),B.GetPieces().Num());
        for (int32 I=0; I<A.GetPieces().Num(); ++I)
        {
            TestTrue(TEXT("Same seed reproduces positions"), A.GetPieces()[I].Position.Equals(B.GetPieces()[I].Position,.00001));
            TestTrue(TEXT("Every piece is inside tray"), FSalvageModel::IsInsideTray(A.GetPieces()[I].Position));
            if(I>=OriginalCount)
            {
                const auto& Added=A.GetPieces()[I];
                TestTrue(TEXT("Optional extraction scrap stays at least twenty-five units inside walls"),
                    FMath::Abs(Added.Position.X)<=475 && FMath::Abs(Added.Position.Y)<=285);
                for(const auto& Other:A.GetPieces()) if(Other.Id!=Added.Id)
                    MinimumNewClearance=FMath::Min(MinimumNewClearance,(Added.Position-Other.Position).Size());
            }
        }
        TestTrue(TEXT("Optional assembly centers have at least forty-two units of clearance from all other pieces"),MinimumNewClearance>=41.999);
        AddInfo(FString::Printf(TEXT("Job %d optional assembly minimum center clearance: %.2f units"),Job+1,MinimumNewClearance));
        for(int32 Assembly=0;Assembly<3;++Assembly)
        {
            const int32 Alloy=OriginalCount+Assembly*3;
            TestTrue(TEXT("Each optional selection is its own alloy and two iron weights"),
                A.FindPiece(Alloy)->Material==EMaterial::Alloy && A.FindPiece(Alloy+1)->Material==EMaterial::Iron
                && A.FindPiece(Alloy+2)->Material==EMaterial::Iron && A.GetCaptureGroup(Alloy).Num()==3);
        }
        A.RetryJob(true); bool bChanged=false;
        for (int32 I=0; I<A.GetPieces().Num(); ++I) bChanged |= !A.GetPieces()[I].Position.Equals(S.Pieces[I].Position,.001);
        TestTrue(TEXT("A new seed changes physical opportunities"), bChanged);
        // Teaching contracts use24kg. Advanced contracts use40kg, affordable from the
        // first three minimum clears (900credits), rather than a950credit full basket.
        if (Job>=3) for (int32 Tier=0; Tier<2; ++Tier) A.PurchaseUpgrade(EUpgrade::Capacity);
        for (int32 Heat=0; Heat<4 && !A.IsDeliveryCompleted(); ++Heat) BankBestStableLoad(A,OriginalCount);
        TestTrue(FString::Printf(TEXT("Job%d retains a legal completion using only its original salvage, without extraction or added assemblies"), Job+1), A.IsDeliveryCompleted());
        FString Error; TestTrue(TEXT("Job completion preserves all ledger invariants"),A.CheckInvariants(Error));
    }
    // Explicit mathematical dead-end state is terminal, never an apparently live soft-lock.
    FSalvageModel Failure; auto S=Failure.GetSnapshot();
    for(auto& P:S.Pieces) P.State=EPieceState::Lost;
    S.bJobEnded=true; FString Error;
    TestTrue(TEXT("A fully lost attempt is valid terminal state"),Failure.RestoreSnapshot(S,Error));
    TestTrue(TEXT("Unreachable quota is honestly failed"),Failure.IsJobFailed());
    Failure.RetryJob(); TestFalse(TEXT("Fresh retry recovers from lost attempt"),Failure.IsJobFailed());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageSnapshotTest, "MagnetSweep.Rework.TransactionalSnapshotValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageSnapshotTest::RunTest(const FString& Parameters)
{
    FSalvageModel Model; CompleteFirstConservatively(Model); Model.PurchaseUpgrade(EUpgrade::Capacity);
    Model.RetryJob(); Model.CapturePieces(IdRange(0,18)); Model.AdvanceRisk(1.25f);
    const auto Valid=Model.GetSnapshot(); FSalvageModel Loaded; FString Error;
    TestTrue(TEXT("Mixed career/unsafe attempt restores"),Loaded.RestoreSnapshot(Valid,Error));
    const int32 LiveCargo=Loaded.GetCargo(); const uint32 Epoch=Loaded.GetEpoch();
    auto Invalid=Valid; Invalid.Cargo+=1;
    TestFalse(TEXT("Mismatched cargo ledger refused"),Loaded.RestoreSnapshot(Invalid,Error));
    TestEqual(TEXT("Refused restore preserves cargo"),Loaded.GetCargo(),LiveCargo);
    TestEqual(TEXT("Refused restore preserves epoch"),Loaded.GetEpoch(),Epoch);
    Invalid=Valid; Invalid.Version=1;
    TestFalse(TEXT("Old unlimited-demo save does not masquerade as career"),Loaded.RestoreSnapshot(Invalid,Error));
    Invalid=Valid; Invalid.Wallet+=1;
    TestFalse(TEXT("Purchased wallet/XP discrepancy refused"),Loaded.RestoreSnapshot(Invalid,Error));
    Invalid=Valid; Invalid.Pieces[1].Id=Invalid.Pieces[0].Id;
    TestFalse(TEXT("Duplicate IDs refused"),Loaded.RestoreSnapshot(Invalid,Error));
    Invalid=Valid; Invalid.FuseElapsed=std::numeric_limits<float>::quiet_NaN();
    TestFalse(TEXT("Nonfinite fuse refused"),Loaded.RestoreSnapshot(Invalid,Error));
    Invalid=Valid; Invalid.Pieces[0].Mass=99;
    TestFalse(TEXT("Forged item mass refused"),Loaded.RestoreSnapshot(Invalid,Error));
    Invalid=Valid; Invalid.Pieces[24].DirectLinks.Add(9999);
    TestFalse(TEXT("Dangling chain link refused"),Loaded.RestoreSnapshot(Invalid,Error));
    Invalid=Valid; Invalid.CollectedCores={0,0};
    TestFalse(TEXT("Duplicate collectible refused"),Loaded.RestoreSnapshot(Invalid,Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvagePersistenceTest, "MagnetSweep.Rework.JsonPersistenceAndBackupRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvagePersistenceTest::RunTest(const FString& Parameters)
{
    FWorkbenchImpl Writer(nullptr);
    Writer.Profile=TEXT("rework_automation_")+FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Writer.SavePath=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("SalvageCareerTests"),Writer.Profile+TEXT(".json"));
    Writer.bMuted=true; Writer.MusicVolume=.2f; Writer.SfxVolume=.65f;
    CompleteFirstConservatively(Writer.Model); Writer.Model.PurchaseUpgrade(EUpgrade::Capacity);
    Writer.Model.RetryJob(); Writer.Model.CapturePieces(IdRange(0,18)); Writer.Model.AdvanceRisk(1.5f);
    TestTrue(TEXT("Unsafe career state writes real atomic primary"),Writer.Save());
    const int32 Wallet=Writer.Model.GetWallet();
    Writer.Model.VentCargo(); Writer.Model.CapturePieces(IdRange(0,16)); Writer.Model.BankCargo(); Writer.bWorkshop=true;
    TestTrue(TEXT("Second state rotates validated backup"),Writer.Save());
    FWorkbenchImpl Loaded(nullptr); Loaded.SavePath=Writer.SavePath;
    TestTrue(TEXT("New career JSON loads"),Loaded.Load());
    TestEqual(TEXT("JSON preserves wallet"),Loaded.Model.GetWallet(),Writer.Model.GetWallet());
    TestEqual(TEXT("JSON preserves XP"),Loaded.Model.GetXP(),Writer.Model.GetXP());
    TestEqual(TEXT("JSON preserves purchased tier"),Loaded.Model.GetCapacity(),32);
    TestEqual(TEXT("JSON preserves heats"),Loaded.Model.GetHeatsUsed(),1);
    TestTrue(TEXT("JSON preserves mute"),Loaded.bMuted);
    TestTrue(TEXT("JSON preserves workshop presentation"),Loaded.bWorkshop);
    TestEqual(TEXT("JSON preserves sound effect volume"),Loaded.SfxVolume,.65f);
    TestEqual(TEXT("JSON preserves music volume"),Loaded.MusicVolume,.2f);
    const int32 LiveWallet=Loaded.Model.GetWallet();
    TestFalse(TEXT("Truncated JSON refused"),Loaded.DecodeSave(TEXT("{\"save_version\":2,")));
    TestFalse(TEXT("Legacy JSON refused"),Loaded.DecodeSave(TEXT("{\"save_version\":1}")));
    TestEqual(TEXT("Rejected JSON leaves wallet untouched"),Loaded.Model.GetWallet(),LiveWallet);
    TestTrue(TEXT("Disposable primary corruption can be injected"),FFileHelper::SaveStringToFile(TEXT("{corrupt"),*Writer.SavePath));
    AddExpectedError(TEXT("MAGNET_SAVE_RECOVERED_BACKUP"),EAutomationExpectedErrorFlags::Contains,1);
    FWorkbenchImpl Recovery(nullptr); Recovery.SavePath=Writer.SavePath;
    TestTrue(TEXT("Corrupt primary recovers real valid backup"),Recovery.Load());
    TestEqual(TEXT("Backup preserves prior protected money"),Recovery.Model.GetWallet(),Wallet);
    TestEqual(TEXT("Backup preserves running fuse"),Recovery.Model.GetFuseElapsed(),1.5f);
    TestEqual(TEXT("Backup preserves unsafe cargo"),Recovery.Model.GetCargoMass(),36);
    FString Before; FFileHelper::LoadFileToString(Before,*(Writer.SavePath+TEXT(".bak")));
    TestTrue(TEXT("Recovered career saves again"),Recovery.Save());
    FString After; FFileHelper::LoadFileToString(After,*(Writer.SavePath+TEXT(".bak")));
    TestEqual(TEXT("Corrupt primary never replaces valid backup"),After,Before);
    FWorkbenchImpl Reopened(nullptr); Reopened.SavePath=Writer.SavePath;
    TestTrue(TEXT("Repaired primary reopens"),Reopened.Load());
    TestTrue(TEXT("Saved fuse trips at its honest remaining deadline"),Reopened.Model.AdvanceRisk(2.f).bTripped);
    TestEqual(TEXT("Quench after JSON recovery consumes one saved charge"),Reopened.Model.GetHeatsRemaining(),3);
    TestTrue(TEXT("Quenched recovered state can be saved without inventing smelt value"),Reopened.Save());
    IFileManager& Files=IFileManager::Get(); const FString Dir=FPaths::GetPath(Writer.SavePath);
    TArray<FString> Generated; Files.FindFiles(Generated,*(Writer.SavePath+TEXT("*")),true,false);
    for(const auto& Name:Generated) Files.Delete(*FPaths::Combine(Dir,Name));
    return true;
}
#endif
