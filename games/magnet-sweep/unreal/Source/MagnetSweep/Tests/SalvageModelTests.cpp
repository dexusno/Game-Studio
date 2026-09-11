#include "SalvageModel.h"

#if WITH_DEV_AUTOMATION_TESTS
#include "WorkbenchRuntime.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Guid.h"
#include "Misc/Paths.h"
#include <limits>

using namespace MagnetSweep;

namespace
{
void SweepAllLoose(FSalvageModel& Model)
{
    const TArray<FSalvagePiece> Pieces = Model.GetPieces();
    for (const FSalvagePiece& Piece : Pieces)
        if (Piece.Kind == EPieceKind::Loose) Model.SweepAt(Piece.Position);
}

void CollectEverything(FSalvageModel& Model)
{
    SweepAllLoose(Model);
    const TArray<FSalvagePiece> Pieces = Model.GetPieces();
    for (const FSalvagePiece& Piece : Pieces)
        if (Piece.Kind == EPieceKind::Tangle)
            Model.CommitPull(Model.PreviewPull(Piece.Id, Piece.Position));
}

int32 SelectedTangles(const FSalvageModel& Model, const FPullSelection& Selection)
{
    int32 Count = 0;
    for (int32 Id : Selection.PieceIds)
        if (Model.FindPiece(Id)->Kind == EPieceKind::Tangle) ++Count;
    return Count;
}

bool GiveFullRig(FSalvageModel& Model)
{
    FSalvageSnapshot Snapshot = Model.GetSnapshot();
    Snapshot.UpgradeLevel = 2;
    Snapshot.CompletedDeliveryCount = 2;
    FString Error;
    return Model.RestoreSnapshot(Snapshot, Error);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageOwnershipTest, "MagnetSweep.Model.OwnershipAndBanking",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageOwnershipTest::RunTest(const FString& Parameters)
{
    FSalvageModel Model;
    FString Error;
    TestTrue(TEXT("Initial authored state satisfies material invariants"), Model.CheckInvariants(Error));
    const int32 Total = Model.GetTotalAmount();
    TestEqual(TEXT("Both forms of material have an explicit 400-unit supply"), Total, 400);
    TestTrue(TEXT("Furnace target leaves optional leftovers"), Model.GetGoal() < Total);
    TestFalse(TEXT("Cannot change deliveries before completing one"), Model.AdvanceLayout());

    const FVector2D Opening(-390, -215);
    const FRecoveryResult First = Model.SweepAt(Opening);
    TestTrue(TEXT("Opening sweep captures useful loose material"), First.Succeeded());
    const int32 CargoAfterFirst = Model.GetCargo();
    TestEqual(TEXT("Repeating identical sweep never awards twice"), Model.SweepAt(Opening).Amount, 0);
    TestEqual(TEXT("Cargo remains secure after a repeat input"), Model.GetCargo(), CargoAfterFirst);
    TestTrue(TEXT("Sweeping leaves isolated ring available"),
        Model.FindPiece(100)->State == EPieceState::Available);

    CollectEverything(Model);
    TestEqual(TEXT("Unrestricted cargo contains the entire tray"), Model.GetCargo(), Total);
    TestEqual(TEXT("No available material remains"), Model.GetAvailableAmount(), 0);
    const FBankResult Bank = Model.BankCargo();
    TestEqual(TEXT("Threshold crossing credits the whole action"), Bank.Amount, Total);
    TestEqual(TEXT("Banked amount retains surplus"), Model.GetBanked(), Total);
    TestEqual(TEXT("Bank empties cargo exactly once"), Model.GetCargo(), 0);
    TestTrue(TEXT("One whole-tray deposit completes once"), Bank.bCompletedNow);
    TestTrue(TEXT("First forge awards Breakaway"), Bank.ForgedUpgrade() && Model.HasBreakaway());
    TestEqual(TEXT("Whole tray does not leap over both milestones"), Model.GetUpgradeLevel(), 1);
    TestEqual(TEXT("Duplicate bank is a no-op"), Model.BankCargo().Amount, 0);
    TestEqual(TEXT("Duplicate bank cannot grant a second milestone"), Model.GetCompletedDeliveryCount(), 1);
    TestTrue(TEXT("Ledger stays consistent after full bank"), Model.CheckInvariants(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvagePullTest, "MagnetSweep.Model.PreviewCommitAndDirectLinks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvagePullTest::RunTest(const FString& Parameters)
{
    FSalvageModel Model;
    const FPullSelection BeforeCoil = Model.PreviewPull(101, FVector2D(450, 90));
    TestEqual(TEXT("Before coil only the selected tangle releases"), SelectedTangles(Model, BeforeCoil), 1);
    TestTrue(TEXT("Before coil the pull also captures loose scrap"), BeforeCoil.PieceIds.Num() > 1);
    TestTrue(TEXT("Base endpoint is limited by reach"),
        FMath::IsNearlyEqual((BeforeCoil.Endpoint - Model.FindPiece(101)->Position).Size(), 440.0, 0.001));
    TestTrue(TEXT("Full rig test state is valid"), GiveFullRig(Model));

    const FPullSelection Fan = Model.PreviewPull(101, FVector2D(450, 90));
    TestEqual(TEXT("Hub preview includes all three directly linked fan leaves"), SelectedTangles(Model, Fan), 4);
    const FRecoveryResult Collected = Model.CommitPull(Fan);
    TestTrue(TEXT("Committed piece IDs exactly match highlighted IDs"), Collected.PieceIds == Fan.PieceIds);
    TestEqual(TEXT("Committed material exactly matches preview"), Collected.Amount, Fan.Amount);
    TestEqual(TEXT("An identical second commit cannot duplicate salvage"), Model.CommitPull(Fan).Amount, 0);
    TestFalse(TEXT("Already recovered tangle is no longer latchable"),
        Model.PreviewPull(101, FVector2D(450, 90)).IsValid());

    Model.ResetLayout();
    const FPullSelection Leaf = Model.PreviewPull(102, FVector2D(-460, 130));
    TestTrue(TEXT("Leaf can directly release the hub"), Leaf.PieceIds.Contains(101));
    TestTrue(TEXT("Another leaf lies inside the same broad corridor"),
        FSalvageModel::IsInsideCapsule(Model.FindPiece(103)->Position,
            Model.FindPiece(102)->Position, Leaf.Endpoint, FSalvageModel::PullRadius));
    TestFalse(TEXT("Neighbor hub cannot recursively release another leaf"), Leaf.PieceIds.Contains(103));
    const FPullSelection Stale = Model.PreviewPull(100, Model.FindPiece(100)->Position);
    Model.SweepAt(FVector2D(400, -180));
    TestEqual(TEXT("State change invalidates old preview atomically"), Model.CommitPull(Stale).Amount, 0);
    TestTrue(TEXT("Invalid preview did not accidentally release selected tangle"),
        Model.FindPiece(100)->State == EPieceState::Available);
    FPullSelection Tampered = Model.PreviewPull(100, Model.FindPiece(100)->Position);
    Tampered.PieceIds.Add(104);
    TestEqual(TEXT("Modified ID list cannot award unrelated scrap"), Model.CommitPull(Tampered).Amount, 0);
    const FPullSelection Fresh = Model.PreviewPull(100, Model.FindPiece(100)->Position);
    TestTrue(TEXT("Zero-length direct tug still succeeds"), Model.CommitPull(Fresh).Succeeded());
    FString Error;
    TestTrue(TEXT("Pull sequence conserves material ownership"), Model.CheckInvariants(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageGeometryTest, "MagnetSweep.Model.CapsuleBoundariesAndReach",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageGeometryTest::RunTest(const FString& Parameters)
{
    const FVector2D Start(0, 0), End(100, 0);
    TestTrue(TEXT("Capsule side boundary is inclusive"), FSalvageModel::IsInsideCapsule({50,90}, Start, End, 90));
    TestFalse(TEXT("Material just outside broad corridor is not selected"), FSalvageModel::IsInsideCapsule({50,90.1}, Start, End, 90));
    TestTrue(TEXT("Finite end cap boundary is inclusive"), FSalvageModel::IsInsideCapsule({190,0}, Start, End, 90));
    TestFalse(TEXT("Corridor cannot extend infinitely past endpoint"), FSalvageModel::IsInsideCapsule({191,0}, Start, End, 90));
    TestTrue(TEXT("Zero length capsule is a forgiving disc"), FSalvageModel::IsInsideCapsule({0,90}, Start, Start, 90));
    TestFalse(TEXT("Zero length disc remains finite"), FSalvageModel::IsInsideCapsule({0,91}, Start, Start, 90));

    FSalvageModel Model;
    const FPullSelection Reach = Model.PreviewPull(101, FVector2D(9000, 9000));
    TestTrue(TEXT("Far cursor is clamped to the actual tray"), FSalvageModel::IsInsideTray(Reach.Endpoint));
    TestTrue(TEXT("Far cursor is also limited by current reach"),
        (Reach.Endpoint - Model.FindPiece(101)->Position).Size() <= Model.GetReach() + 0.001);
    TestFalse(TEXT("Invalid float cannot become a pull"), Model.PreviewPull(101,
        FVector2D(std::numeric_limits<double>::quiet_NaN(), 0)).IsValid());
    TestEqual(TEXT("Outside pointer cannot collect edge scrap"), Model.SweepAt({-501,-210}).Amount, 0);
    TestEqual(TEXT("Loose material cannot be latched as a ring"),
        Model.PreviewPull(0, {100,100}).RingId, INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageProgressionTest, "MagnetSweep.Model.ProgressionRetryAndContrastingLayouts",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageProgressionTest::RunTest(const FString& Parameters)
{
    FSalvageModel Model;
    // Collect just the opening crescent and isolated ring; the signature fan remains to enjoy.
    for (int32 Id = 0; Id < 10; ++Id) Model.SweepAt(Model.FindPiece(Id)->Position);
    Model.CommitPull(Model.PreviewPull(100, Model.FindPiece(100)->Position));
    const FBankResult Opening = Model.BankCargo();
    TestTrue(TEXT("Opening earns the defining coil before touching fan"), Opening.ForgedUpgrade());
    TestTrue(TEXT("Fan hub remains available immediately after reward"),
        Model.FindPiece(101)->State == EPieceState::Available);
    TestEqual(TEXT("Coil exposes linked fan burst on the same tray"),
        SelectedTangles(Model, Model.PreviewPull(101, {450,90})), 4);

    Model.CommitPull(Model.PreviewPull(101, {450,90}));
    const int32 Surplus = Model.GetCargo();
    TestTrue(TEXT("Completed delivery can still gather a substantial surplus"), Surplus > 0);
    TestFalse(TEXT("Advance cannot silently discard surplus cargo"), Model.AdvanceLayout());
    TestEqual(TEXT("Blocked advance leaves cargo unchanged"), Model.GetCargo(), Surplus);
    const FBankResult SurplusBank = Model.BankCargo();
    TestFalse(TEXT("Banking surplus does not award another upgrade"), SurplusBank.ForgedUpgrade());
    TestFalse(TEXT("Banking surplus does not complete delivery again"), SurplusBank.bCompletedNow);
    TestTrue(TEXT("Explicitly banked completion can advance"), Model.AdvanceLayout());
    TestEqual(TEXT("Next delivery uses contrasting fork"), Model.GetLayoutIndex(), 1);
    TestEqual(TEXT("New delivery retains coil"), Model.GetUpgradeLevel(), 1);
    TestEqual(TEXT("Comparable arrangement has same salvage supply"), Model.GetTotalAmount(), 400);
    SweepAllLoose(Model);
    const FBankResult Second = Model.BankCargo();
    TestTrue(TEXT("Next completed delivery earns reach"), Second.ForgedUpgrade());
    TestEqual(TEXT("Fully earned rig has longer reach"), Model.GetReach(), 720.0f);
    Model.ResetLayout();
    TestEqual(TEXT("Retry preserves completed rig"), Model.GetUpgradeLevel(), 2);
    TestEqual(TEXT("Retry restores exact arrangement and empty furnace"), Model.GetBanked(), 0);
    TestEqual(TEXT("Retry restores material"), Model.GetAvailableAmount(), 400);

    const FPullSelection Hub = Model.PreviewPull(100, {450,250});
    const FPullSelection Leaf = Model.PreviewPull(103, {400,250});
    TestTrue(TEXT("Fork hub direction cannot gather all its spread leaves"), SelectedTangles(Model, Hub) < 4);
    TestTrue(TEXT("Seam-side leaf has a meaningful linked pull"), Leaf.PieceIds.Contains(100));
    TestTrue(TEXT("Leaf's rich seam competes with the obvious hub direction"), Leaf.Amount > Hub.Amount);
    TestFalse(TEXT("Leaf cannot transitively release upper fork through hub"), Leaf.PieceIds.Contains(102));
    CollectEverything(Model);
    const FBankResult FullRigDelivery = Model.BankCargo();
    TestTrue(TEXT("Full rig delivery still completes"), FullRigDelivery.bCompletedNow);
    TestFalse(TEXT("Full rig completion does not promise nonexistent upgrade"), FullRigDelivery.ForgedUpgrade());
    TestTrue(TEXT("Full rig can request another delivery"), Model.AdvanceLayout());
    TestEqual(TEXT("Two authored arrangements alternate"), Model.GetLayoutIndex(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvageSnapshotTest, "MagnetSweep.Model.TransactionalSaveRestore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvageSnapshotTest::RunTest(const FString& Parameters)
{
    FSalvageModel Original;
    Original.SweepAt({-390,-215});
    Original.BankCargo();
    Original.CommitPull(Original.PreviewPull(100, {-355,-85}));
    const FSalvageSnapshot Snapshot = Original.GetSnapshot();
    FSalvageModel Restored;
    const FPullSelection OldGesture = Restored.PreviewPull(101, {450,90});
    FString Error;
    TestTrue(TEXT("Canonical mixed bank/cargo state restores"), Restored.RestoreSnapshot(Snapshot, Error));
    TestEqual(TEXT("Restore retains attached cargo"), Restored.GetCargo(), Original.GetCargo());
    TestEqual(TEXT("Restore retains banked progress"), Restored.GetBanked(), Original.GetBanked());
    TestEqual(TEXT("Restore retains available supply"), Restored.GetAvailableAmount(), Original.GetAvailableAmount());
    TestEqual(TEXT("Restore invalidates old live gestures"), Restored.CommitPull(OldGesture).Amount, 0);
    TestEqual(TEXT("Restored recovered ring cannot award twice"),
        Restored.CommitPull(Restored.PreviewPull(100, {-355,-85})).Amount, 0);
    TestTrue(TEXT("Restored model satisfies invariants"), Restored.CheckInvariants(Error));

    const int32 ValidCargo = Restored.GetCargo();
    const uint32 ValidEpoch = Restored.GetEpoch();
    FSalvageSnapshot Invalid = Snapshot;
    Invalid.Cargo += 1;
    TestFalse(TEXT("Corrupt ledger is rejected"), Restored.RestoreSnapshot(Invalid, Error));
    TestFalse(TEXT("Corrupt save reports actionable error"), Error.IsEmpty());
    TestEqual(TEXT("Failed restore preserves live cargo"), Restored.GetCargo(), ValidCargo);
    TestEqual(TEXT("Failed restore does not mutate live epoch"), Restored.GetEpoch(), ValidEpoch);
    Invalid = Snapshot;
    Invalid.Pieces[1].Id = Invalid.Pieces[0].Id;
    TestFalse(TEXT("Duplicate IDs are rejected"), Restored.RestoreSnapshot(Invalid, Error));
    Invalid = Snapshot;
    Invalid.Version = 999;
    TestFalse(TEXT("Incompatible save version is rejected"), Restored.RestoreSnapshot(Invalid, Error));
    Invalid = Snapshot;
    Invalid.Pieces[0].Position.X = std::numeric_limits<double>::infinity();
    TestFalse(TEXT("Nonfinite saved geometry is rejected"), Restored.RestoreSnapshot(Invalid, Error));
    Invalid = Snapshot;
    Invalid.Pieces[0].State = static_cast<EPieceState>(99);
    TestFalse(TEXT("Invalid material state is rejected"), Restored.RestoreSnapshot(Invalid, Error));
    Invalid = Snapshot;
    for (FSalvagePiece& Piece : Invalid.Pieces)
        if (Piece.Id == 101) Piece.DirectLinks.Add(999);
    TestFalse(TEXT("Dangling tangle link is rejected"), Restored.RestoreSnapshot(Invalid, Error));
    Invalid = Snapshot;
    Invalid.UpgradeLevel = 2;
    TestFalse(TEXT("Inconsistent earned progression is rejected"), Restored.RestoreSnapshot(Invalid, Error));
    Invalid = Snapshot;
    Invalid.Goal = 400;
    TestFalse(TEXT("Goal demanding final scrap is rejected"), Restored.RestoreSnapshot(Invalid, Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FSalvagePersistenceTest, "MagnetSweep.Model.JsonPersistenceAndBackupRecovery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FSalvagePersistenceTest::RunTest(const FString& Parameters)
{
    // No actor or scene is required: these methods only persist the pure material model/settings.
    FWorkbenchImpl Writer(nullptr);
    Writer.Profile = TEXT("automation_") + FGuid::NewGuid().ToString(EGuidFormats::Digits);
    Writer.SavePath = FPaths::Combine(FPaths::ProjectSavedDir(), TEXT("ConceptDemoTests"), Writer.Profile + TEXT(".json"));
    Writer.bMuted = true;
    Writer.Model.SweepAt({-390,-215});
    const int32 FirstCargo = Writer.Model.GetCargo();
    TestTrue(TEXT("Initial save writes a real primary file"), Writer.Save());
    Writer.Model.BankCargo();
    Writer.Model.CommitPull(Writer.Model.PreviewPull(100, {-355,-85}));
    TestTrue(TEXT("Second save rotates a valid recoverable backup"), Writer.Save());

    FWorkbenchImpl Loaded(nullptr);
    Loaded.SavePath = Writer.SavePath;
    TestTrue(TEXT("Full JSON save loads into a fresh model"), Loaded.Load());
    TestEqual(TEXT("JSON round trip preserves cargo"), Loaded.Model.GetCargo(), Writer.Model.GetCargo());
    TestEqual(TEXT("JSON round trip preserves banked material"), Loaded.Model.GetBanked(), Writer.Model.GetBanked());
    TestEqual(TEXT("JSON round trip preserves muted setting"), Loaded.bMuted, Writer.bMuted);
    TestEqual(TEXT("JSON round trip preserves all pieces"), Loaded.Model.GetPieces().Num(), Writer.Model.GetPieces().Num());
    TestTrue(TEXT("JSON round trip preserves direct links"), Loaded.Model.FindPiece(101)->DirectLinks == Writer.Model.FindPiece(101)->DirectLinks);
    TestEqual(TEXT("Loaded material cannot be recovered twice"),
        Loaded.Model.CommitPull(Loaded.Model.PreviewPull(100, {-355,-85})).Amount, 0);
    const int32 LiveCargo = Loaded.Model.GetCargo();
    const uint32 LiveEpoch = Loaded.Model.GetEpoch();
    TestFalse(TEXT("Truncated JSON is rejected"), Loaded.DecodeSave(TEXT("{\"save_version\":1,")));
    TestFalse(TEXT("Missing fields are rejected"), Loaded.DecodeSave(TEXT("{\"save_version\":1}")));
    TestEqual(TEXT("Invalid JSON leaves live cargo unchanged"), Loaded.Model.GetCargo(), LiveCargo);
    TestEqual(TEXT("Invalid JSON leaves live gesture epoch unchanged"), Loaded.Model.GetEpoch(), LiveEpoch);
    TestTrue(TEXT("Invalid JSON leaves mute setting unchanged"), Loaded.bMuted);

    TestTrue(TEXT("Test can corrupt the disposable primary file"),
        FFileHelper::SaveStringToFile(TEXT("{corrupt"), *Writer.SavePath));
    FWorkbenchImpl Recovery(nullptr);
    Recovery.SavePath = Writer.SavePath;
    TestTrue(TEXT("Corrupt primary recovers prior valid backup"), Recovery.Load());
    TestEqual(TEXT("Recovery uses prior valid cargo, not new-game defaults"), Recovery.Model.GetCargo(), FirstCargo);
    TestEqual(TEXT("Recovery uses prior valid furnace state"), Recovery.Model.GetBanked(), 0);
    FString BackupBefore;
    TestTrue(TEXT("Valid recovery backup remains readable"),
        FFileHelper::LoadFileToString(BackupBefore, *(Writer.SavePath + TEXT(".bak"))));
    TestTrue(TEXT("Recovered state can be saved again"), Recovery.Save());
    FString BackupAfter;
    FFileHelper::LoadFileToString(BackupAfter, *(Writer.SavePath + TEXT(".bak")));
    TestEqual(TEXT("Saving after corruption never replaces backup with corrupt data"), BackupAfter, BackupBefore);
    FWorkbenchImpl Reopened(nullptr);
    Reopened.SavePath = Writer.SavePath;
    TestTrue(TEXT("Repaired primary is playable on a subsequent open"), Reopened.Load());
    TestEqual(TEXT("Repaired save preserves recovered cargo"), Reopened.Model.GetCargo(), FirstCargo);

    // These are uniquely named disposable test files under this project's ignored Saved folder.
    IFileManager& Files = IFileManager::Get();
    const FString Directory = FPaths::GetPath(Writer.SavePath);
    TArray<FString> GeneratedFiles;
    Files.FindFiles(GeneratedFiles, *(Writer.SavePath + TEXT("*")), true, false);
    for (const FString& Name : GeneratedFiles) Files.Delete(*FPaths::Combine(Directory, Name));
    return true;
}
#endif
