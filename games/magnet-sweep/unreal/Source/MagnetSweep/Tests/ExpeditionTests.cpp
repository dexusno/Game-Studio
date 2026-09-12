#if WITH_DEV_AUTOMATION_TESTS
#include "ExpeditionRuntime.h"
#include "ExpeditionRig.h"
#include "ExpeditionWorld.h"
#include "WorkbenchRuntime.h"
#include "Dom/JsonObject.h"
#include "InputCoreTypes.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

using namespace MagnetSweep;

namespace
{
TArray<FName> Implemented()
{
    TArray<FName> Ids;
    for (const auto& Module : FExpeditionRig::Catalog())
        if (FExpeditionWorld::IsModuleImplemented(Module.Id)) Ids.Add(Module.Id);
    return Ids;
}

bool Configure(FExpeditionRig& Rig, const FExpeditionWorld& World, FName Starter, int32 Seed)
{
    Rig.SetShopContext(World.GetOpportunityTags(), Implemented());
    return Rig.NewRun(Starter, Seed);
}

void Advance(FExpeditionWorld& World, const FExpeditionRig& Rig,
             const FVector2D& Magnet, float Seconds)
{
    // The same fixed integration input is also used after deserialization.
    while (Seconds > KINDA_SMALL_NUMBER)
    {
        const float Step = FMath::Min(Seconds, 1.f / 60.f);
        World.Tick(Step, Magnet, Rig);
        Seconds -= Step;
    }
}

int32 LooseBody(const FExpeditionWorld& World, bool bHot = false)
{
    for (const auto& Body : World.GetBodies())
        if (Body.State == EExpeditionBodyState::Available && !Body.bAnchored &&
            !Body.bGoal && Body.bHot == bHot &&
            (bHot || (Body.Role == TEXT("loose_scrap") && Body.Value > 0 &&
                       Body.Material == EExpeditionMaterial::Iron && Body.Links.IsEmpty())))
            return Body.Id;
    return INDEX_NONE;
}

int32 RoleBody(const FExpeditionWorld& World, FName Role)
{
    for (const auto& Body : World.GetBodies()) if (Body.Role == Role) return Body.Id;
    return INDEX_NONE;
}

bool Pull(FExpeditionWorld& World, const FExpeditionRig& Rig, int32 Id,
          const FVector2D& Magnet, EExpeditionAction Action = EExpeditionAction::Attract)
{
    const auto* Body = World.FindBody(Id);
    if (!Body) return false;
    FExpeditionCommand Command;
    Command.Action = Action;
    Command.Magnet = Magnet;
    Command.Aim = Body->Position;
    Command.TargetId = Id;
    Command.BodyIds = {Id};
    Command.bPrecision = true;
    if (!World.Execute(Command, Rig).bSucceeded) return false;
    for (int32 Frame = 0; Frame < 120; ++Frame)
    {
        World.Tick(1.f / 60.f, Magnet, Rig);
        const auto* Current = World.FindBody(Id);
        if (Current && Current->State == EExpeditionBodyState::Cargo) return true;
    }
    return false;
}

void MoveMagnet(FExpeditionWorld& World, const FExpeditionRig& Rig, const FVector2D& Destination)
{
    FVector2D Current = World.GetState().Magnet;
    for (int32 Step = 0; Step < 300 && !Current.Equals(Destination, .05f); ++Step)
    {
        const FVector2D Delta = Destination - Current;
        Current += Delta.GetSafeNormal() * FMath::Min(Delta.Size(), 6.0);
        World.Tick(1.f / 60.f, Current, Rig);
    }
    Advance(World, Rig, Destination, .3f);
}

// Executes the authored fallback with paid pulls and actual placement. No
// mechanism flag, body state, position or energy balance is assigned by the test.
bool ReleaseAndCarryCore(FExpeditionWorld& World, const FExpeditionRig& Rig, FString& Error)
{
    const int32 Collar = RoleBody(World, TEXT("collar"));
    const int32 Ballast = RoleBody(World, TEXT("ballast"));
    const int32 Brace = RoleBody(World, TEXT("brace"));
    const int32 Core = RoleBody(World, TEXT("core"));
    if (Collar == INDEX_NONE || Ballast == INDEX_NONE || Brace == INDEX_NONE || Core == INDEX_NONE)
    { Error = TEXT("Authored physical objective roles are missing"); return false; }
    FExpeditionCommand Command;
    Command.Magnet = FExpeditionWorld::CollarStop(); Command.Aim = World.FindBody(Collar)->Position;
    Command.TargetId = Collar; Command.bPrecision = true;
    const auto Result = World.Execute(Command, Rig);
    if (!Result.bSucceeded) { Error = TEXT("Collar pull: ") + Result.Message; return false; }
    Advance(World, Rig, Command.Magnet, 1.3f);
    if (!World.GetState().bCollarReleased) { Error = TEXT("Physical collar did not reach its visible relief stop"); return false; }
    for (int32 Id : {Ballast, Brace})
    {
        const FVector2D Pickup = World.FindBody(Id)->Position + FVector2D(0, -55);
        if (!Pull(World, Rig, Id, Pickup)) { Error = TEXT("Could not physically secure ") + World.FindBody(Id)->Role.ToString(); return false; }
        const FVector2D Destination = Id == Ballast ? FExpeditionWorld::BallastCatch() : FExpeditionWorld::ArmStopper();
        MoveMagnet(World, Rig, FVector2D(Pickup.X, 265));
        MoveMagnet(World, Rig, FVector2D(Destination.X, 265));
        MoveMagnet(World, Rig, Destination);
        if (Id == Brace)
            for (int32 Wait = 0; Wait < 400 && !World.IsArmSafe(); ++Wait) World.Tick(1.f / 60.f, Destination, Rig);
        if (!World.Drop().bSucceeded) { Error = TEXT("Placement did not release held material"); return false; }
        Advance(World, Rig, Destination, .1f);
        if (Id == Ballast && !World.GetState().bBallastCleared) { Error = TEXT("Ballast placed on visible catch did not release its latch"); return false; }
    }
    if (!World.IsCoreReleased()) { Error = TEXT("Visible collar/catch/brace route did not release the core"); return false; }
    if (!Pull(World, Rig, Core, World.FindBody(Core)->Position + FVector2D(55, -40)))
    { Error = TEXT("Released core could not be physically secured"); return false; }
    MoveMagnet(World, Rig, FVector2D(530, -240));
    MoveMagnet(World, Rig, FExpeditionWorld::ReceiverPosition());
    Error.Reset(); return true;
}

// Combination isolation fixture, deliberately separate from the earned-shop
// regression: validated free/found equipment, actual world actions afterwards.
// It is never serialized to disk and is not evidence of an earned playthrough.
bool FixtureRig(FExpeditionRig& Rig, const FExpeditionWorld& World,
                const TArray<FName>& Actives, const TArray<FName>& Passives, FString& Error)
{
    if (Actives.IsEmpty()) return false;
    const FName Initial = FExpeditionRig::InitialStarters().Contains(Actives[0]) ? Actives[0] : FName(TEXT("extraction_coil"));
    if (!Configure(Rig, World, Initial, 103)) return false;
    const auto Json = Rig.ToJson();
    TArray<FName> All = Actives; All.Append(Passives);
    TArray<TSharedPtr<FJsonValue>> Inventory, ActiveValues, PassiveValues;
    for (FName Id : All)
    {
        const auto Item = MakeShared<FJsonObject>(); Item->SetStringField(TEXT("id"), Id.ToString());
        Item->SetNumberField(TEXT("paid"), 0); Inventory.Add(MakeShared<FJsonValueObject>(Item));
    }
    for (FName Id : Actives) ActiveValues.Add(MakeShared<FJsonValueString>(Id.ToString()));
    for (FName Id : Passives) PassiveValues.Add(MakeShared<FJsonValueString>(Id.ToString()));
    Json->SetArrayField(TEXT("inventory"), Inventory);
    Json->SetArrayField(TEXT("actives"), ActiveValues); Json->SetArrayField(TEXT("passives"), PassiveValues);
    TArray<TSharedPtr<FJsonValue>> Remaining;
    for (const auto& Offer : Json->GetArrayField(TEXT("offers")))
        if (!All.Contains(FName(*Offer->AsString()))) Remaining.Add(Offer);
    Json->SetArrayField(TEXT("offers"), Remaining);
    return Rig.FromJson(*Json, Error) && Rig.Depart(Error);
}

bool StartRuntime(FExpeditionRuntime& Runtime, int32 Seed)
{
    Runtime.World->StartSite(0, Seed);
    if (!Configure(*Runtime.Rig, *Runtime.World, TEXT("extraction_coil"), Seed)) return false;
    Runtime.Screen = EExpeditionScreen::Depot; Runtime.Depart();
    return Runtime.Screen == EExpeditionScreen::Site && Runtime.SiteEntry.IsValid();
}

TSharedPtr<FJsonObject> ParseJson(const FString& Text)
{
    TSharedPtr<FJsonObject> Result;
    FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Text), Result);
    return Result;
}

FString JsonText(const TSharedPtr<FJsonObject>& Json)
{
    FString Result;
    FJsonSerializer::Serialize(Json.ToSharedRef(), TJsonWriterFactory<>::Create(&Result));
    return Result;
}

void RuntimeAdvance(FExpeditionRuntime& Runtime, float Seconds)
{
    while (Seconds > KINDA_SMALL_NUMBER)
    {
        const float Step = FMath::Min(Seconds, 1.f / 60.f);
        Runtime.Tick(Step); Seconds -= Step;
    }
}

void RuntimeMove(FExpeditionRuntime& Runtime, const FVector2D& Destination)
{
    Runtime.Aim = Destination; Runtime.bWorldHit = true;
    for (int32 Frame = 0; Frame < 240 && !Runtime.Magnet.Equals(Destination, .05f); ++Frame)
        Runtime.Tick(1.f / 60.f);
    RuntimeAdvance(Runtime, .3f);
}

bool RuntimeRecoverCore(FExpeditionRuntime& Runtime, FString& Error)
{
    const int32 Collar = RoleBody(*Runtime.World, TEXT("collar"));
    if (Collar == INDEX_NONE) return false;
    RuntimeMove(Runtime, Runtime.World->FindBody(Collar)->Position + FVector2D(-25, 0));
    Runtime.Key(EKeys::LeftShift, true);
    Runtime.Key(EKeys::LeftMouseButton, true);
    Runtime.Aim = FExpeditionWorld::CollarStop();
    RuntimeAdvance(Runtime, 1.4f);
    Runtime.Key(EKeys::LeftMouseButton, false); Runtime.Key(EKeys::LeftShift, false);
    if (!Runtime.World->GetState().bCollarReleased) { Error = TEXT("Native handler collar pull did not reach its stop"); return false; }
    for (FName Role : {FName(TEXT("ballast")), FName(TEXT("brace")), FName(TEXT("core"))})
    {
        const int32 Id = RoleBody(*Runtime.World, Role);
        if (Id == INDEX_NONE) return false;
        RuntimeMove(Runtime, Runtime.World->FindBody(Id)->Position + FVector2D(0, -25));
        Runtime.Key(EKeys::LeftShift, true); Runtime.Key(EKeys::LeftMouseButton, true);
        RuntimeAdvance(Runtime, 1.4f);
        Runtime.Key(EKeys::LeftMouseButton, false); Runtime.Key(EKeys::LeftShift, false);
        if (Runtime.World->FindBody(Id)->State != EExpeditionBodyState::Cargo)
        { Error = TEXT("Runtime precision did not secure ") + Role.ToString(); return false; }
        if (Role == TEXT("core"))
        {
            RuntimeMove(Runtime, FVector2D(530, -240));
            RuntimeMove(Runtime, FExpeditionWorld::ReceiverPosition());
            return true;
        }
        const FVector2D Destination = Role == TEXT("ballast") ? FExpeditionWorld::BallastCatch() : FExpeditionWorld::ArmStopper();
        RuntimeMove(Runtime, FVector2D(Runtime.Magnet.X, 265));
        RuntimeMove(Runtime, FVector2D(Destination.X, 265));
        RuntimeMove(Runtime, Destination);
        if (Role == TEXT("brace"))
            for (int32 Wait = 0; Wait < 400 && !Runtime.World->IsArmSafe(); ++Wait) Runtime.Tick(1.f / 60.f);
        Runtime.Key(EKeys::RightMouseButton, true); RuntimeAdvance(Runtime, .1f);
        if (Role == TEXT("ballast") && !Runtime.World->GetState().bBallastCleared)
        { Error = TEXT("Runtime drop missed the ballast catch"); return false; }
    }
    Error = TEXT("Core route did not reach dispatch"); return false;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionInputCancellationTest,
    "MagnetSweep.Expedition.InputCancellation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionInputCancellationTest::RunTest(const FString& Parameters)
{
    FExpeditionInputGate Gate;
    Gate.bField = true;
    TestTrue(TEXT("Q aim starts from ordinary attraction"), Gate.BeginAim(0));
    TestFalse(TEXT("Aiming stops ordinary attraction"), Gate.bField);
    TestFalse(TEXT("Held-key repeat cannot purchase another operation"), Gate.BeginAim(0));
    TestEqual(TEXT("An unrelated release cannot commit Q"), Gate.EndAim(1), INDEX_NONE);
    TestEqual(TEXT("Q release commits exactly its live slot"), Gate.EndAim(0), 0);
    TestEqual(TEXT("Repeated release cannot commit twice"), Gate.EndAim(0), INDEX_NONE);

    TestTrue(TEXT("A new Q press can prepare another operation"), Gate.BeginAim(0));
    TestTrue(TEXT("F deliberately replaces the previous aim"), Gate.BeginAim(1));
    TestEqual(TEXT("Releasing replaced Q cannot fire"), Gate.EndAim(0), INDEX_NONE);
    TestEqual(TEXT("Only the latest matching F release commits"), Gate.EndAim(1), 1);

    for (int32 Slot = 0; Slot != 2; ++Slot)
    {
        TestTrue(TEXT("Aim can begin before a rescue or menu cancellation"), Gate.BeginAim(Slot));
        Gate.Cancel();
        TestEqual(TEXT("Cancelled key-up has no operation to spend"), Gate.EndAim(Slot), INDEX_NONE);
        TestFalse(TEXT("Cancellation cannot restart the field"), Gate.bField);
        TestTrue(TEXT("Cancellation does not permanently disable this key"), Gate.BeginAim(Slot));
        Gate.LoseFocus();
        TestEqual(TEXT("Focus-loss key-up cannot fire"), Gate.EndAim(Slot), INDEX_NONE);
        TestTrue(TEXT("A fresh deliberate press after focus recovery works"), Gate.BeginAim(Slot));
        TestEqual(TEXT("Fresh deliberate release is still usable"), Gate.EndAim(Slot), Slot);
    }
    TestFalse(TEXT("Invalid tool slot cannot arm"), Gate.BeginAim(2));
    TestEqual(TEXT("Invalid release cannot commit"), Gate.EndAim(2), INDEX_NONE);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionPhysicalCaptureTest,
    "MagnetSweep.Expedition.PhysicalCaptureAndFieldOff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionPhysicalCaptureTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(0, 71);
    FExpeditionRig Rig;
    TestTrue(TEXT("An initially available starter starts a run"), Configure(Rig, World, TEXT("extraction_coil"), 71));
    const int32 Id = LooseBody(World);
    if (!TestTrue(TEXT("Authored site supplies ordinary useful loose salvage"), Id != INDEX_NONE)) return false;
    const FVector2D Origin = World.FindBody(Id)->Position;
    const FVector2D Magnet = Origin + FVector2D(0, -75);
    FExpeditionCommand Command;
    Command.Action = EExpeditionAction::Attract;
    Command.Magnet = Magnet; Command.Aim = Origin; Command.TargetId = Id; Command.BodyIds = {Id};
    Command.bPrecision = true;
    const auto Preview = World.Preview(Command, Rig);
    if (!TestTrue(TEXT("The visible small salvage is a legal pull"), Preview.bAllowed)) return false;
    const auto Result = World.Execute(Command, Rig);
    TestTrue(TEXT("Manual pull commits"), Result.bSucceeded);
    TestEqual(TEXT("Pull pays its previewed cost exactly once"), 100 - World.GetBattery(), Preview.BatteryCost);
    TestTrue(TEXT("Commit begins physical travel rather than immediate ownership"),
        World.FindBody(Id)->State == EExpeditionBodyState::Pulling);
    World.Tick(1.f / 60.f, Magnet, Rig);
    TestTrue(TEXT("A field produces actual movement toward the magnet"),
        FVector2D::Distance(World.FindBody(Id)->Position, Magnet) < FVector2D::Distance(Origin, Magnet));
    Advance(World, Rig, Magnet, 1.5f);
    TestTrue(TEXT("The physical pull eventually secures its body"), World.FindBody(Id)->State == EExpeditionBodyState::Cargo);
    World.CancelPull();
    TestTrue(TEXT("Field-off retains already secured salvage"), World.FindBody(Id)->State == EExpeditionBodyState::Cargo);
    float SelectedMass = 0;
    for (int32 Selected : Preview.BodyIds)
        if (World.FindBody(Selected)->Material != EExpeditionMaterial::Mechanism) SelectedMass += World.FindBody(Selected)->Mass;
    TestTrue(TEXT("Secured mass is the real previewed load"), FMath::IsNearlyEqual(World.GetCargoMass(), SelectedMass));
    const int32 PaidBattery = World.GetBattery();
    const auto Dropped = World.Drop();
    TestTrue(TEXT("Deliberate whole-haul drop actually releases the body"), Dropped.bSucceeded);
    TestTrue(TEXT("Dropped salvage remains available and inside the tray"),
        World.FindBody(Id)->State == EExpeditionBodyState::Available &&
        World.FindBody(Id)->Position.X >= FExpeditionWorld::MinX && World.FindBody(Id)->Position.X <= FExpeditionWorld::MaxX);
    TestEqual(TEXT("Drop neither spends extra energy nor refunds the pull"), World.GetBattery(), PaidBattery);
    TestTrue(TEXT("No cargo remains after whole-haul drop"), FMath::IsNearlyZero(World.GetCargoMass()));
    FString Error;
    TestTrue(*FString::Printf(TEXT("Final physical state validates: %s"), *Error), World.CheckInvariants(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionPendingPullPersistenceTest,
    "MagnetSweep.Expedition.PendingPhysicalPullSave",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionPendingPullPersistenceTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(0, 73);
    FExpeditionRig Rig; Configure(Rig, World, TEXT("extraction_coil"), 73);
    const int32 Id = LooseBody(World);
    if (!TestTrue(TEXT("A real authored loose body exists"), Id != INDEX_NONE)) return false;
    const FVector2D Magnet = World.FindBody(Id)->Position + FVector2D(0, -100);
    FExpeditionCommand Command;
    Command.Magnet = Magnet; Command.Aim = World.FindBody(Id)->Position;
    Command.TargetId = Id; Command.BodyIds = {Id};
    Command.bPrecision = true;
    if (!TestTrue(TEXT("A physical pull begins before saving"), World.Execute(Command, Rig).bSucceeded)) return false;
    World.Tick(1.f / 60.f, Magnet, Rig);
    FExpeditionWorld Restored; FString Error;
    if (!TestTrue(*FString::Printf(TEXT("In-flight state restores: %s"), *Error), Restored.FromJson(World.ToJson(), Error))) return false;
    TestEqual(TEXT("Restoring cannot refund committed energy"), Restored.GetBattery(), World.GetBattery());
    TestEqual(TEXT("Root action identity survives"), Restored.GetState().ActionSerial, World.GetState().ActionSerial);
    Advance(World, Rig, Magnet, 1.5f); Advance(Restored, Rig, Magnet, 1.5f);
    TestTrue(TEXT("Restored physical action reaches the same secured state"),
        World.FindBody(Id)->State == EExpeditionBodyState::Cargo && Restored.FindBody(Id)->State == World.FindBody(Id)->State);
    TestTrue(TEXT("Restored trajectory agrees with uninterrupted integration"),
        Restored.FindBody(Id)->Position.Equals(World.FindBody(Id)->Position, .01f));
    TestEqual(TEXT("No energy is spent twice while a restored action settles"), Restored.GetBattery(), World.GetBattery());
    TestEqual(TEXT("A capture alone produces no smelt output"), Restored.GetOutput(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionUnsafeRecoveryTest,
    "MagnetSweep.Expedition.UnsafeFieldOffDropAndQuench",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionUnsafeRecoveryTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(0, 79);
    FExpeditionRig Rig; Configure(Rig, World, TEXT("extraction_coil"), 79);
    const int32 Hot = LooseBody(World, true);
    if (!TestTrue(TEXT("Authored site has a capturable readable hazard"), Hot != INDEX_NONE)) return false;
    const FVector2D Magnet = World.FindBody(Hot)->Position + FVector2D(-55, 0);
    if (!TestTrue(TEXT("Extraction can physically separate and capture a lone live cell"), Pull(World, Rig, Hot, Magnet, EExpeditionAction::Extract))) return false;
    TestTrue(TEXT("Live cargo enters the unsafe condition"), World.IsUnsafe());
    World.CancelPull();
    TestTrue(TEXT("Field-off cannot erase an already held hazard"), World.IsUnsafe());
    Advance(World, Rig, Magnet, .25f);
    TestTrue(TEXT("The unsafe fuse continues with the field off"), World.GetState().UnsafeElapsed > 0.f);
    FExpeditionWorld Expiry; FString Error;
    if (!TestTrue(TEXT("The actual dangerous state is restorable"), Expiry.FromJson(World.ToJson(), Error))) return false;
    const int32 BeforeDrop = World.GetBattery();
    TestTrue(TEXT("One whole-haul drop corrects the danger"), World.Drop().bSucceeded);
    Advance(World, Rig, Magnet, 3.5f);
    TestEqual(TEXT("A corrected haul pays no late quench"), World.GetBattery(), BeforeDrop);
    TestFalse(TEXT("Correction leaves no unsafe cargo"), World.IsUnsafe());
    TestTrue(TEXT("Correction does not destroy the live cell"), World.FindBody(Hot)->State == EExpeditionBodyState::Available);

    Expiry.DrainEvents();
    const int32 BeforeExpiry = Expiry.GetBattery();
    Advance(Expiry, Rig, Magnet, 3.5f);
    TestEqual(TEXT("A lone-cell expired fuse has a real 12-energy consequence"), Expiry.GetBattery(), FMath::Max(0, BeforeExpiry - 12));
    TestTrue(TEXT("Expiry spills its hazard recoverably"), Expiry.FindBody(Hot)->State == EExpeditionBodyState::Available);
    TestTrue(TEXT("Expiry empties the secured haul"), FMath::IsNearlyZero(Expiry.GetCargoMass()));
    int32 Quenches = 0;
    for (const auto& Event : Expiry.DrainEvents()) if (Event.Kind == EExpeditionEventKind::Quench) ++Quenches;
    TestEqual(TEXT("Overlapping updates report one failure incident"), Quenches, 1);
    Advance(Expiry, Rig, Magnet, 3.5f);
    TestEqual(TEXT("A spent incident cannot keep draining idle energy"), Expiry.GetBattery(), FMath::Max(0, BeforeExpiry - 12));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionAnchoredObjectiveTest,
    "MagnetSweep.Expedition.AnchoredObjectiveCannotBePlucked",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionAnchoredObjectiveTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(3, 83);
    FExpeditionRig Rig; Configure(Rig, World, TEXT("extraction_coil"), 83);
    int32 Core = INDEX_NONE;
    for (const auto& Body : World.GetBodies()) if (Body.bGoal) { Core = Body.Id; break; }
    if (!TestTrue(TEXT("Final worksite exposes its actual physical objective"), Core != INDEX_NONE)) return false;
    TestTrue(TEXT("The final intact core fits an empty baseline rig"),
        FMath::IsNearlyEqual(World.FindBody(Core)->Mass, 20.f) && World.FindBody(Core)->Mass <= FExpeditionWorld::SafeCapacity);
    TestTrue(TEXT("The visible objective begins physically secured"), World.FindBody(Core)->bAnchored);
    const int32 Battery = World.GetBattery();
    const FVector2D Original = World.FindBody(Core)->Position;
    for (const auto Action : {EExpeditionAction::Attract, EExpeditionAction::Extract})
    {
        FExpeditionCommand Command;
        Command.Action = Action; Command.TargetId = Core; Command.BodyIds = {Core};
        Command.Magnet = Original + FVector2D(0, -50); Command.Aim = Original;
        TestFalse(TEXT("Preview rejects pickup through unresolved physical restraints"), World.Preview(Command, Rig).bAllowed);
        TestFalse(TEXT("Dispatch cannot bypass preview by directly executing extraction"), World.Execute(Command, Rig).bSucceeded);
    }
    TestEqual(TEXT("Rejected objective input consumes no battery"), World.GetBattery(), Battery);
    TestTrue(TEXT("Rejected operations neither move nor detach the mission core"),
        World.FindBody(Core)->bAnchored && World.FindBody(Core)->Position.Equals(Original));
    TestFalse(TEXT("Receiver cannot manufacture completion before recovery"), World.Dispatch().bSucceeded);
    TestFalse(TEXT("An empty furnace cannot silently consume the objective"), World.Bank().bSucceeded);
    TestFalse(TEXT("The final operation still requires its physical conditions"), World.IsCoreReleased());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionRuntimeReleaseCancellationTest,
    "MagnetSweep.Expedition.RuntimeReleaseCancellation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionRuntimeReleaseCancellationTest::RunTest(const FString& Parameters)
{
    // Exercise the handler used by native dispatch without starting a viewport,
    // loading a profile, writing a save path or requiring owner input.
    FExpeditionRuntime Runtime(nullptr);
    Runtime.World->StartSite(0, 89);
    Configure(*Runtime.Rig, *Runtime.World, TEXT("extraction_coil"), 89);
    Runtime.Screen = EExpeditionScreen::Site;
    Runtime.bWorldHit = true; // Controlled world-space hit; no viewport is created.
    const int32 Target = RoleBody(*Runtime.World, TEXT("conductor"));
    if (!TestTrue(TEXT("Authored conductor supplies a real active-tool target"), Target != INDEX_NONE)) return false;
    Runtime.Aim = Runtime.World->FindBody(Target)->Position;
    Runtime.Magnet = Runtime.Aim + FVector2D(0, -55);
    const int32 Before = Runtime.World->GetBattery();

    Runtime.Key(EKeys::Q, true);
    TestEqual(TEXT("Real key handler arms Q"), Runtime.Input.ArmedSlot, 0);
    Runtime.Key(EKeys::RightMouseButton, true);
    Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Q-up after whole-haul rescue spends nothing"), Runtime.World->GetBattery(), Before);
    TestEqual(TEXT("Rescue leaves no delayed armed action"), Runtime.Input.ArmedSlot, INDEX_NONE);

    Runtime.Key(EKeys::Q, true);
    Runtime.SetPaused(true);
    Runtime.Key(EKeys::Q, false);
    Runtime.SetPaused(false);
    TestEqual(TEXT("Release during pause cannot spend on resume"), Runtime.World->GetBattery(), Before);
    Runtime.Key(EKeys::Q, true);
    TestEqual(TEXT("Releasing while paused did not leave Q permanently held"), Runtime.Input.ArmedSlot, 0);
    Runtime.Key(EKeys::Q, false);
    TestTrue(TEXT("Only a fresh deliberate release executes the valid active"), Runtime.World->GetBattery() < Before);
    const int32 AfterCommit = Runtime.World->GetBattery();
    Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Repeated key-up cannot purchase extraction twice"), Runtime.World->GetBattery(), AfterCommit);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionPhysicalFinalRouteTest,
    "MagnetSweep.Expedition.PhysicalFinalCoreRoute",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionPhysicalFinalRouteTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(3, 97);
    FExpeditionRig Rig; Configure(Rig, World, TEXT("extraction_coil"), 97);
    FString Error;
    const bool Recovered = ReleaseAndCarryCore(World, Rig, Error);
    if (!TestTrue(*FString::Printf(TEXT("Actual collar/ballast/brace route: %s"), *Error), Recovered)) return false;
    TestTrue(TEXT("A basic physical route leaves recovery energy headroom"), World.GetBattery() >= 50);
    TestTrue(TEXT("The intact 20 kg core is actually carried"), FMath::IsNearlyEqual(World.GetCargoMass(), 20.f));
    MoveMagnet(World, Rig, FExpeditionWorld::FurnacePosition());
    TestFalse(TEXT("Scrap furnace refuses the unique recovered mission core"), World.Bank().bSucceeded);
    MoveMagnet(World, Rig, FExpeditionWorld::ReceiverPosition());
    TestTrue(TEXT("Separate receiver accepts the recovered objective"), World.Dispatch().bSucceeded);
    TestTrue(TEXT("Actual dispatch ends the physical site"), World.GetState().bDispatched);
    TestFalse(TEXT("Repeated dispatch cannot complete it twice"), World.Dispatch().bSucceeded);
    TestTrue(TEXT("Completed world remains internally valid"), World.CheckInvariants(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionEarnedShopPivotTest,
    "MagnetSweep.Expedition.EarnedDispatchShopAndPivot",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionEarnedShopPivotTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr);
    Runtime.World->StartSite(0, 101);
    if (!TestTrue(TEXT("Real implemented site supplies initial shop context"),
        Configure(*Runtime.Rig, *Runtime.World, TEXT("extraction_coil"), 101))) return false;
    FString Error; FName Support;
    for (FName Id : Runtime.Rig->GetOffers())
    {
        const auto* Module = FExpeditionRig::FindModule(Id);
        if (Module && Module->Kind == EExpeditionModuleKind::Passive && Runtime.Rig->CanBuy(Id, Error)) { Support = Id; break; }
    }
    if (!TestFalse(TEXT("Initial earned-rig direction has a real affordable support"), Support.IsNone())) return false;
    const int32 SupportPrice = FExpeditionRig::FindModule(Support)->Price;
    if (!TestTrue(TEXT("Buy and fit consume the real outfitting budget"),
        Runtime.Rig->Buy(Support, Error) && Runtime.Rig->Fit(Support, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Depot; Runtime.Depart();
    TestTrue(TEXT("Actual runtime departure creates the site checkpoint"), Runtime.SiteEntry.IsValid() && Runtime.Screen == EExpeditionScreen::Site);
    const int32 CashBeforeRecovery = Runtime.Rig->GetCash();
    const bool Recovered = ReleaseAndCarryCore(*Runtime.World, *Runtime.Rig, Error);
    if (!TestTrue(*FString::Printf(TEXT("First machinery is recovered by world actions: %s"), *Error), Recovered)) return false;
    Runtime.Magnet = FExpeditionWorld::ReceiverPosition(); Runtime.BankOrDeliver();
    TestEqual(TEXT("Actual dispatch pays the first clear reward"), Runtime.Rig->GetCash(), CashBeforeRecovery + 10);
    TestTrue(TEXT("Runtime enters the next real shop after physical dispatch"),
        Runtime.Screen == EExpeditionScreen::Depot && Runtime.SiteIndex == 1 && Runtime.Rig->IsSiteAwarded(0));
    const int32 PaidCash = Runtime.Rig->GetCash();
    Runtime.BankOrDeliver();
    TestEqual(TEXT("Repeated contextual input cannot duplicate the prior site's pay"), Runtime.Rig->GetCash(), PaidCash);
    TArray<FName> Alternatives;
    for (FName Id : Runtime.Rig->GetOffers())
    {
        const auto* Module = FExpeditionRig::FindModule(Id);
        if (Module && Module->Kind == EExpeditionModuleKind::Active && Runtime.Rig->CanBuy(Id, Error)) Alternatives.Add(Id);
    }
    if (!TestTrue(TEXT("Earned shop provides two independently purchasable active directions"), Alternatives.Num() >= 2)) return false;
    if (!TestTrue(TEXT("Earned funds buy and fit a second active"),
        Runtime.Rig->Buy(Alternatives[0], Error) && Runtime.Rig->Fit(Alternatives[0], Error))) return false;
    const int32 BeforeSale = Runtime.Rig->GetCash();
    TestTrue(TEXT("Paid active can be sold while a working starter remains"), Runtime.Rig->Sell(Alternatives[0], Error));
    TestEqual(TEXT("Resale uses the actual 10-credit purchase basis"), Runtime.Rig->GetCash(), BeforeSale + 5);
    TestTrue(TEXT("Early specialist can be sold for a deliberate pivot"), Runtime.Rig->Sell(Support, Error));
    TestEqual(TEXT("Odd-price support resale rounds down, never mints money"), Runtime.Rig->GetCash(), BeforeSale + 5 + SupportPrice / 2);
    TestTrue(TEXT("The remaining saved offer supports a real affordable pivot"),
        Runtime.Rig->Buy(Alternatives[1], Error) && Runtime.Rig->Fit(Alternatives[1], Error));
    TestFalse(TEXT("Sold item is not silently restocked for another cycle"), Runtime.Rig->GetOffers().Contains(Alternatives[0]));
    TestTrue(TEXT("The replacement rig can actually depart"), Runtime.Rig->CanDepart(Error));
    TestTrue(TEXT("Final earned transaction history validates"), Runtime.Rig->Validate(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionConductorChainTest,
    "MagnetSweep.Expedition.ExtractedConductorCircuit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionConductorChainTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(0, 103);
    FExpeditionRig Rig; FString Error;
    if (!TestTrue(TEXT("Controlled two-tool fixture validates"),
        FixtureRig(Rig, World, {TEXT("extraction_coil"), TEXT("arc_driver")}, {}, Error))) return false;
    const int32 Conductor = RoleBody(World, TEXT("conductor"));
    const int32 Terminal = RoleBody(World, TEXT("receiver_terminal"));
    if (!TestTrue(TEXT("Circuit uses authored conductor and terminal bodies"), Conductor != INDEX_NONE && Terminal != INDEX_NONE)) return false;
    FExpeditionCommand Arc; Arc.Action = EExpeditionAction::Arc; Arc.TargetId = Terminal;
    Arc.Magnet = FVector2D(30, -160); Arc.Aim = World.FindBody(Terminal)->Position;
    TestFalse(TEXT("Arc alone cannot skip the missing conducting path"), World.Execute(Arc, Rig).bSucceeded);
    const int32 BeforeCut = World.GetBattery();
    if (!TestTrue(TEXT("Extraction physically frees the linked conductor"),
        Pull(World, Rig, Conductor, FVector2D(-280, -160), EExpeditionAction::Extract))) return false;
    TestTrue(TEXT("Selective extraction leaves surrounding ballast behind"),
        World.FindBody(Conductor)->Links.IsEmpty() && FMath::IsNearlyEqual(World.GetCargoMass(), 6.f));
    TestFalse(TEXT("Cutting alone does not operate the circuit"), World.GetState().bCircuitClosed);
    MoveMagnet(World, Rig, FExpeditionWorld::CircuitSocket());
    TestTrue(TEXT("Deliberate placement drops the actual conductor"), World.Drop().bSucceeded);
    Advance(World, Rig, FExpeditionWorld::CircuitSocket(), .1f);
    TestTrue(TEXT("Visible socket and actual conductor agree without hidden counter-aim"),
        World.FindBody(Conductor)->Position.Equals(FExpeditionWorld::CircuitSocket(), 5.f));
    const int32 BeforeArc = World.GetBattery();
    const auto Powered = World.Execute(Arc, Rig);
    TestTrue(TEXT("Two-tool physical preparation makes the remote operation possible"), Powered.bSucceeded);
    TestTrue(TEXT("Real circuit releases the remote ballast latch"), World.GetState().bCircuitClosed && World.GetState().bBallastCleared);
    TestEqual(TEXT("Cut and discharge retain their separate costs"), BeforeCut - World.GetBattery(), 14);
    TestEqual(TEXT("Internal arc charge never refunds its own energy"), World.GetBattery(), BeforeArc - 8);
    TestFalse(TEXT("Already actuated circuit cannot repeatedly claim its effect"), World.Execute(Arc, Rig).bSucceeded);
    TestEqual(TEXT("No smelt or output is awarded for merely operating a circuit"), World.GetOutput(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionCounterweightChainTest,
    "MagnetSweep.Expedition.WeldedCounterweightMovesMachine",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionCounterweightChainTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(0, 107);
    FExpeditionRig Rig; FString Error;
    if (!TestTrue(TEXT("Controlled launch/winch combination validates"),
        FixtureRig(Rig, World, {TEXT("rail_impeller"), TEXT("anchor_winch")},
                   {TEXT("slug_press"), TEXT("counterweight_hook")}, Error))) return false;
    const int32 Machine = RoleBody(World, TEXT("supported_machine"));
    if (!TestTrue(TEXT("An actual supported machine exists"), Machine != INDEX_NONE)) return false;
    FExpeditionCommand Tow; Tow.Action = EExpeditionAction::Winch; Tow.TargetId = Machine;
    Tow.Magnet = FVector2D(-120, 105); Tow.Aim = World.FindBody(Machine)->Position; Tow.Destination = Tow.Magnet;
    TestFalse(TEXT("Winch cannot fake a counterweight before it exists"), World.Execute(Tow, Rig).bSucceeded);
    FExpeditionCommand Gather;
    Gather.Magnet = FVector2D(-375, 35); Gather.Aim = Gather.Magnet; Gather.bPrecision = true;
    const auto GatherPreview = World.Preview(Gather, Rig);
    if (!TestTrue(TEXT("A real iron pocket can form the counterweight"), GatherPreview.bAllowed && GatherPreview.Mass >= 8)) return false;
    TestTrue(TEXT("Iron gathering is an actual paid physical action"), World.Execute(Gather, Rig).bSucceeded);
    Advance(World, Rig, Gather.Magnet, 1.5f);
    if (!TestFalse(TEXT("Combination setup must not assume an unsafe load is usable"), World.IsUnsafe())) return false;
    const float OriginalMass = World.GetCargoMass(); const int32 OriginalValue = World.GetCargoValue();
    const int32 BeforeWeld = World.GetBattery();
    FExpeditionCommand Weld; Weld.Action = EExpeditionAction::Weld; Weld.Magnet = Gather.Magnet; Weld.Aim = Gather.Magnet;
    for (const auto& Body : World.GetBodies())
        if (Body.State == EExpeditionBodyState::Cargo && Body.Material == EExpeditionMaterial::Iron) Weld.BodyIds.Add(Body.Id);
    const auto Welded = World.Execute(Weld, Rig);
    if (!TestTrue(TEXT("Separate paid weld produces one real body"), Welded.bSucceeded && Welded.BodyIds.Num() == 1)) return false;
    const int32 Slug = Welded.BodyIds[0];
    TestEqual(TEXT("Preparation costs four, with no automatic launch charge"), World.GetBattery(), BeforeWeld - 4);
    TestTrue(TEXT("Welding retains conserved cargo mass/value"),
        FMath::IsNearlyEqual(World.GetCargoMass(), OriginalMass) && World.GetCargoValue() == OriginalValue);
    TestFalse(TEXT("Preparing a counterweight does not automatically fire it"), World.FindBody(Slug)->bLaunched);
    TestTrue(TEXT("Welded body preserves several original material identities"), World.FindBody(Slug)->SourceIds.Num() >= 2);
    MoveMagnet(World, Rig, FExpeditionWorld::CounterweightPad());
    TestTrue(TEXT("A deliberate drop places the real weight on the marked pad"), World.Drop().bSucceeded);
    Advance(World, Rig, FExpeditionWorld::CounterweightPad(), .1f);
    const FVector2D MachineBefore = World.FindBody(Machine)->Position;
    if (!TestTrue(TEXT("Prepared slug plus winch can now release the supported machine"), World.Execute(Tow, Rig).bSucceeded)) return false;
    Advance(World, Rig, Tow.Magnet, 1.5f);
    TestTrue(TEXT("The intact machine physically moves toward the chosen anchor"),
        FVector2D::Distance(World.FindBody(Machine)->Position, Tow.Destination) < FVector2D::Distance(MachineBefore, Tow.Destination) - 20);
    TestTrue(TEXT("The operation used the supported recovery path"), World.GetState().bCounterweightUsed && !World.FindBody(Machine)->bAnchored);
    TestEqual(TEXT("Manipulation alone has not banked the machine's value"), World.GetOutput(), 0);
    TestTrue(TEXT("Conserved-source world validates after the combination"), World.CheckInvariants(Error));
    FExpeditionWorld Restored;
    TestTrue(TEXT("The supporting material and live physical constraint serialize together"), Restored.FromJson(World.ToJson(), Error));
    const int32 QualityBeforeRemoval = World.FindBody(Machine)->Quality;
    if (!TestTrue(TEXT("The player can physically remove the actual counterweight"),
        Pull(World, Rig, Slug, World.FindBody(Slug)->Position + FVector2D(0, -55)))) return false;
    Advance(World, Rig, World.GetState().Magnet, .1f);
    TestTrue(TEXT("Removing support causally settles the unsecured machine"), World.FindBody(Machine)->bAnchored);
    TestTrue(TEXT("Support failure affects the real machine's condition"), World.FindBody(Machine)->Quality < QualityBeforeRemoval);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionRuntimePauseTest,
    "MagnetSweep.Expedition.RuntimePauseFreezesCommittedWork",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionRuntimePauseTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr);
    if (!TestTrue(TEXT("A real isolated session departs"), StartRuntime(Runtime, 109))) return false;
    const int32 Hot = LooseBody(*Runtime.World, true);
    if (!TestTrue(TEXT("The authored live component exists"), Hot != INDEX_NONE)) return false;
    Runtime.Magnet = Runtime.World->FindBody(Hot)->Position + FVector2D(-80, 0);
    FExpeditionCommand Extract; Extract.Action = EExpeditionAction::Extract;
    Extract.Magnet = Runtime.Magnet; Extract.Aim = Runtime.World->FindBody(Hot)->Position; Extract.TargetId = Hot;
    if (!TestTrue(TEXT("A real paid extraction is in flight before pause"), Runtime.World->Execute(Extract, *Runtime.Rig).bSucceeded)) return false;
    Runtime.Tick(.05f);
    const FExpeditionWorldState Before = Runtime.World->GetState();
    const FVector2D BeforePosition = Runtime.World->FindBody(Hot)->Position;
    Runtime.SetPaused(true);
    Runtime.Key(EKeys::LeftMouseButton, false);
    for (int32 Frame = 0; Frame < 70; ++Frame) Runtime.Tick(.1f);
    const auto& Paused = Runtime.World->GetState();
    TestTrue(TEXT("Pause freezes the shared world clock"), FMath::IsNearlyEqual(Paused.WorldTime, Before.WorldTime));
    TestTrue(TEXT("Pause freezes a committed transfer instead of cancelling it"),
        FMath::IsNearlyEqual(Paused.PullRemaining, Before.PullRemaining) && Paused.PullIds == Before.PullIds);
    TestTrue(TEXT("Paused bodies retain their actual positions"), Runtime.World->FindBody(Hot)->Position.Equals(BeforePosition));
    TestTrue(TEXT("Pause also freezes the unsafe fuse"), FMath::IsNearlyEqual(Paused.UnsafeElapsed, Before.UnsafeElapsed));
    TestEqual(TEXT("No battery is spent or refunded while paused"), Paused.Battery, Before.Battery);
    Runtime.SetPaused(false);
    for (int32 Frame = 0; Frame < 14; ++Frame) Runtime.Tick(.1f);
    TestTrue(TEXT("The previously paid motion resumes and secures the body"), Runtime.World->FindBody(Hot)->State == EExpeditionBodyState::Cargo);
    TestTrue(TEXT("Its danger also resumes rather than resetting"), Runtime.World->GetState().UnsafeElapsed > Before.UnsafeElapsed);
    Runtime.Key(EKeys::RightMouseButton, true);
    TestFalse(TEXT("Actual runtime rescue clears the held danger"), Runtime.World->IsUnsafe());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionAtomicRuntimeSaveRetryTest,
    "MagnetSweep.Expedition.AtomicRuntimeSaveRetryAndAwards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionAtomicRuntimeSaveRetryTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr);
    if (!TestTrue(TEXT("A real session starts with an entry checkpoint"), StartRuntime(Runtime, 113))) return false;
    const int32 EntryCash = Runtime.Rig->GetCash(); const int32 EntryBattery = Runtime.World->GetBattery();
    const TArray<FName> EntryOffers = Runtime.Rig->GetOffers();
    FExpeditionCommand Gather; Gather.Magnet = FVector2D(185, -246); Gather.Aim = Gather.Magnet;
    Runtime.Magnet = Gather.Magnet;
    const auto Preview = Runtime.World->Preview(Gather, *Runtime.Rig);
    if (!TestTrue(TEXT("Actual rich pocket previews a safe haul worth the first milestone"),
        Preview.bAllowed && Preview.Mass <= 24 && Preview.Value >= FExpeditionRig::RefiningThreshold(0, 0))) return false;
    if (!TestTrue(TEXT("Rich pocket starts a paid world capture"), Runtime.World->Execute(Gather, *Runtime.Rig).bSucceeded)) return false;
    Runtime.Tick(.05f);
    const int32 SpentBattery = Runtime.World->GetBattery();
    const auto Pending = Runtime.World->GetState().PullIds;
    FExpeditionRuntime Restored(nullptr); FString Error;
    if (!TestTrue(TEXT("Whole session restores its actual in-flight capture"), Restored.DecodeSave(Runtime.EncodeSave(), Error))) return false;
    TestTrue(TEXT("Loaded active work resumes paused with its pending ownership intact"), Restored.bPaused && Restored.World->GetState().PullIds == Pending);
    TestEqual(TEXT("Whole-session restoration cannot refund the paid field"), Restored.World->GetBattery(), SpentBattery);
    Restored.SetPaused(false);
    for (int32 Frame = 0; Frame < 15; ++Frame) Restored.Tick(.1f);
    MoveMagnet(*Restored.World, *Restored.Rig, FVector2D(550, -246));
    MoveMagnet(*Restored.World, *Restored.Rig, FExpeditionWorld::FurnacePosition());
    Restored.Magnet = FExpeditionWorld::FurnacePosition();
    const int32 HaulValue = Restored.World->GetCargoValue();
    Restored.BankOrDeliver();
    TestEqual(TEXT("Real smelt commits exactly the physically carried output"), Restored.World->GetOutput(), HaulValue);
    TestEqual(TEXT("World callback pays the first actual refining milestone"), Restored.Rig->GetCash(), EntryCash + 2);
    TestEqual(TEXT("Rig output agrees with the smelted world ledger"), Restored.Rig->GetOutput(0), HaulValue);
    Restored.BankOrDeliver();
    TestEqual(TEXT("Repeated smelt input cannot pay a consumed haul twice"), Restored.Rig->GetCash(), EntryCash + 2);
    const FString BeforeRejectedRestore = Restored.EncodeSave();
    auto Broken = ParseJson(BeforeRejectedRestore);
    Broken->GetObjectField(TEXT("world"))->SetNumberField(TEXT("Battery"), -1);
    TestFalse(TEXT("Invalid world rejects a combined restore despite a valid rig"), Restored.DecodeSave(JsonText(Broken), Error));
    TestEqual(TEXT("Rejected restore leaves the entire earned session unchanged"), Restored.EncodeSave(), BeforeRejectedRestore);
    auto ForgedVictory = ParseJson(BeforeRejectedRestore);
    ForgedVictory->SetNumberField(TEXT("screen"), int32(EExpeditionScreen::Victory));
    TestFalse(TEXT("A valid JSON payload cannot invent a victory screen"), Restored.DecodeSave(JsonText(ForgedVictory), Error));
    FExpeditionRuntime OtherRun(nullptr);
    if (!TestTrue(TEXT("A separate valid checkpoint fixture is available"), StartRuntime(OtherRun, 127))) return false;
    auto WrongCheckpoint = ParseJson(BeforeRejectedRestore);
    WrongCheckpoint->SetObjectField(TEXT("site_entry"), OtherRun.SiteEntry);
    TestFalse(TEXT("A same-site checkpoint from another run cannot replace this career"), Restored.DecodeSave(JsonText(WrongCheckpoint), Error));
    TestEqual(TEXT("Invalid screen/checkpoint replacements are also transactional"), Restored.EncodeSave(), BeforeRejectedRestore);
    Restored.RetrySite();
    TestTrue(TEXT("Retry returns to a paused real site"), Restored.bPaused && Restored.Screen == EExpeditionScreen::Site);
    TestEqual(TEXT("Retry restores entry energy"), Restored.World->GetBattery(), EntryBattery);
    TestEqual(TEXT("Retry also rolls back the milestone cash"), Restored.Rig->GetCash(), EntryCash);
    TestEqual(TEXT("Retry rolls back physical bank output"), Restored.World->GetOutput(), 0);
    TestEqual(TEXT("Retry rolls back the matching reward ledger"), Restored.Rig->GetOutput(0), 0);
    TestTrue(TEXT("Retry preserves saved shop stock instead of rerolling it"), Restored.Rig->GetOffers() == EntryOffers);
    for (int32 Id : Pending) TestTrue(TEXT("Original paid targets return available on exact retry"), Restored.World->FindBody(Id)->State == EExpeditionBodyState::Available);
    TestTrue(TEXT("Retry remains serializable as a complete session"), Runtime.DecodeSave(Restored.EncodeSave(), Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionLastActiveSaleTest,
    "MagnetSweep.Expedition.LastActiveCannotStrandInitialRun",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionLastActiveSaleTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; FExpeditionRig Rig; FString Error;
    bool Found = false;
    // Select a reproducible starting-stock fixture, never a player-facing reroll.
    for (int32 Seed = 1; Seed <= 128 && !Found; ++Seed)
    {
        FExpeditionRig Candidate; Configure(Candidate, World, TEXT("extraction_coil"), Seed);
        if (Candidate.GetOffers().Contains(TEXT("insulated_jaw")))
        { Found = Rig.FromJson(*Candidate.ToJson(), Error); }
    }
    if (!TestTrue(TEXT("A valid initial five-credit support offer exists"), Found)) return false;
    TestTrue(TEXT("Player can buy and fit the support from real starting funds"), Rig.Buy(TEXT("insulated_jaw"), Error) && Rig.Fit(TEXT("insulated_jaw"), Error));
    TestEqual(TEXT("The reported novice edge reaches seven credits"), Rig.GetCash(), 7);
    TestFalse(TEXT("Selling the last owned active is refused before stranding the run"), Rig.Sell(TEXT("extraction_coil"), Error));
    TestEqual(TEXT("Refused last-active sale changes no money"), Rig.GetCash(), 7);
    TestTrue(TEXT("Unfitting is still reversible"), Rig.Unfit(TEXT("extraction_coil"), Error));
    TestFalse(TEXT("Inactive support plus no active cannot depart"), Rig.CanDepart(Error));
    TestTrue(TEXT("Restoring the owned starter makes the same run playable"), Rig.Fit(TEXT("extraction_coil"), Error) && Rig.CanDepart(Error));
    const auto Invalid = MakeShared<FJsonObject>();
    const int32 SafeCash = Rig.GetCash();
    TestFalse(TEXT("Empty or foreign rig JSON cannot replace a valid rig"), Rig.FromJson(*Invalid, Error));
    TestTrue(TEXT("Rejected rig restore preserves the usable equipment and wallet"), Rig.GetCash() == SafeCash && Rig.Has(TEXT("extraction_coil")));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionMassBoundaryTest,
    "MagnetSweep.Expedition.MassBoundariesAndAtomicGroups",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionMassBoundaryTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(0, 131);
    FExpeditionRig Rig; Configure(Rig, World, TEXT("extraction_coil"), 131);
    FExpeditionCommand Gather; Gather.Magnet = FVector2D(-330, 35); Gather.Aim = Gather.Magnet;
    const auto Preview = World.Preview(Gather, Rig);
    if (!TestTrue(TEXT("Crowded iron pocket previews a real over-safe load"), Preview.bAllowed && Preview.Mass > 24 && Preview.Mass <= 36)) return false;
    TestTrue(TEXT("The preview labels its overload before spending"), Preview.bUnsafe);
    for (int32 Id : Preview.BodyIds)
        for (int32 Linked : World.GetGroup(Id))
            TestTrue(TEXT("Broad attraction selects every member of an accepted linked group"), Preview.BodyIds.Contains(Linked));
    TestTrue(TEXT("A deliberately unsafe but hard-cap-valid pull is allowed"), World.Execute(Gather, Rig).bSucceeded);
    TestTrue(TEXT("Actual attached plus reserved mass never exceeds the hard limit"), World.GetCargoMass() <= 36.f);
    Advance(World, Rig, Gather.Magnet, 1.3f);
    if (!TestTrue(TEXT("This authored crowded load fills the hard limit"), FMath::IsNearlyEqual(World.GetCargoMass(), 36.f))) return false;
    const int32 Battery = World.GetBattery();
    FExpeditionCommand More; More.Magnet = Gather.Magnet; More.Aim = FVector2D(185, -246);
    TestFalse(TEXT("No partial extra group is captured above 36 kg"), World.Execute(More, Rig).bSucceeded);
    TestEqual(TEXT("Hard-cap refusal consumes no energy"), World.GetBattery(), Battery);
    TestTrue(TEXT("One emergency drop clears every body, not an automatic value filter"), World.Drop().bSucceeded && FMath::IsNearlyZero(World.GetCargoMass()));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionLegacySchemaIsolationTest,
    "MagnetSweep.Expedition.LegacySchemaIsolation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionLegacySchemaIsolationTest::RunTest(const FString& Parameters)
{
    // No Start/Load calls or filesystem destinations. Runtime save callbacks
    // return at the empty path, so no real career/profile is discovered.
    FWorkbenchImpl Legacy(nullptr);
    const FString LegacyBefore = Legacy.EncodeSave();
    FExpeditionRuntime Expedition(nullptr);
    if (!TestTrue(TEXT("Separate in-memory expedition fixture is valid"), StartRuntime(Expedition, 137))) return false;
    const FString ExpeditionBefore = Expedition.EncodeSave(); FString Error;
    TestFalse(TEXT("Expedition cannot reinterpret a legacy career as a new run"), Expedition.DecodeSave(LegacyBefore, Error));
    TestEqual(TEXT("Rejected legacy schema leaves the new session unchanged"), Expedition.EncodeSave(), ExpeditionBefore);
    TestFalse(TEXT("Legacy decoder cannot ingest the successor's schema"), Legacy.DecodeSave(ExpeditionBefore));
    TestEqual(TEXT("Rejected successor schema preserves the old in-memory career"), Legacy.EncodeSave(), LegacyBefore);
    TestTrue(TEXT("Neither fixture has a filesystem save destination"), Legacy.SavePath.IsEmpty() && Expedition.SavePath.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionFourSiteRuntimeTest,
    "MagnetSweep.Expedition.FourSiteRuntimeRecoveryAndEarnedRig",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionFourSiteRuntimeTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr);
    if (!TestTrue(TEXT("The full run starts through a real departure"), StartRuntime(Runtime, 149))) return false;
    FString Error; int32 EarnedPurchases = 0;
    for (int32 Site = 0; Site < 4; ++Site)
    {
        TestEqual(TEXT("The runtime is on the expected sequential worksite"), Runtime.SiteIndex, Site);
        const int32 CashBefore = Runtime.Rig->GetCash();
        const bool Recovered = RuntimeRecoverCore(Runtime, Error);
        if (!TestTrue(*FString::Printf(TEXT("Site %d actual key/physical route: %s"), Site + 1, *Error), Recovered)) return false;
        TestEqual(TEXT("Recovering alone cannot mint the dispatch reward"), Runtime.Rig->GetCash(), CashBefore);
        Runtime.Key(EKeys::E, true);
        TestTrue(TEXT("Real contextual delivery records this completed physical objective"), Runtime.Rig->IsSiteAwarded(Site));
        TestEqual(TEXT("Clear pay is earned exactly once by actual runtime dispatch"), Runtime.Rig->GetCash(), CashBefore + (Site < 3 ? 10 + 2 * Site : 0));
        if (Site == 3) break;
        TestTrue(TEXT("Each actual delivery opens the next generated depot"), Runtime.Screen == EExpeditionScreen::Depot && Runtime.Rig->HasVerifiedShopPair());
        FName Purchase;
        for (FName Id : Runtime.Rig->GetOffers())
        {
            const auto* Module = FExpeditionRig::FindModule(Id);
            const int32 Capacity = Module && Module->Kind == EExpeditionModuleKind::Active ? FExpeditionRig::ActiveSlots : FExpeditionRig::PassiveSlots;
            if (Module && Runtime.Rig->CanBuy(Id, Error) && Runtime.Rig->GetUsedSlots(Module->Kind) + Module->Slots <= Capacity)
            { Purchase = Id; break; }
        }
        if (!Purchase.IsNone())
        {
            const int32 Price = FExpeditionRig::FindModule(Purchase)->Price;
            TestTrue(TEXT("Earned funds purchase a real offered module"), Runtime.Rig->Buy(Purchase, Error));
            TestTrue(TEXT("The purchased module is explicitly fitted"), Runtime.Rig->Fit(Purchase, Error));
            TestEqual(TEXT("Purchase consumes its real listed price"), Runtime.Rig->GetCash(), CashBefore + 10 + 2 * Site - Price);
            ++EarnedPurchases;
        }
        Runtime.Depart();
        TestTrue(TEXT("The bought rig enters the next actual site"), Runtime.Screen == EExpeditionScreen::Site);
    }
    TestTrue(TEXT("At least one improvement was bought with actual expedition earnings"), EarnedPurchases > 0);
    TestTrue(TEXT("The fourth actual recovery reaches victory"), Runtime.Screen == EExpeditionScreen::Victory && Runtime.Rig->IsRunWon());
    TestEqual(TEXT("The completed run is archived once"), Runtime.Rig->GetRecords().RunsWon, 1);
    TestFalse(TEXT("The winning equipment archive is populated by the actual run"), Runtime.Rig->GetRecords().LastWinningRig.IsEmpty());
    const int32 FinalCash = Runtime.Rig->GetCash(); Runtime.Key(EKeys::E, true);
    TestEqual(TEXT("Victory input cannot pay another clear reward"), Runtime.Rig->GetCash(), FinalCash);
    FExpeditionRuntime Resumed(nullptr);
    TestTrue(TEXT("Actual four-site victory restores without inventing history"), Resumed.DecodeSave(Runtime.EncodeSave(), Error));
    TestTrue(TEXT("Saved victory retains the real earned archive"), Resumed.Rig->GetRecords().LastWinningRig == Runtime.Rig->GetRecords().LastWinningRig);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionRailSubsetSelectionTest,
    "MagnetSweep.Expedition.RuntimeWeldSubsetAndChosenLaunch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionRailSubsetSelectionTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103);
    FString Error;
    if (!TestTrue(TEXT("Controlled Rail loadout validates for interaction isolation"),
        FixtureRig(*Runtime.Rig, *Runtime.World, {TEXT("rail_impeller"), TEXT("anchor_winch")}, {TEXT("slug_press")}, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Site;
    RuntimeMove(Runtime, FVector2D(-375, 35));
    Runtime.Key(EKeys::LeftShift, true); Runtime.Key(EKeys::LeftMouseButton, true);
    RuntimeAdvance(Runtime, 1.5f);
    Runtime.Key(EKeys::LeftMouseButton, false); Runtime.Key(EKeys::LeftShift, false);
    TArray<int32> Iron;
    for (const auto& Body : Runtime.World->GetBodies())
        if (Body.State == EExpeditionBodyState::Cargo && Body.Material == EExpeditionMaterial::Iron) Iron.Add(Body.Id);
    if (!TestTrue(TEXT("Actual precision captures enough separate iron for a subset decision"), Iron.Num() >= 4)) return false;
    Iron.Sort();
    Runtime.Click(41); Runtime.Click(2000 + Iron[0]); Runtime.Click(2000 + Iron[1]);
    const auto WeldPreview = Runtime.CommandFor(0);
    TestTrue(TEXT("Actual cargo buttons select exactly the requested weld subset"),
        WeldPreview.Action == EExpeditionAction::Weld && WeldPreview.BodyIds.Num() == 2 &&
        WeldPreview.BodyIds.Contains(Iron[0]) && WeldPreview.BodyIds.Contains(Iron[1]));
    const int32 BeforeWeld = Runtime.World->GetBattery();
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("The actual release commits one four-energy weld"), Runtime.World->GetBattery(), BeforeWeld - 4);
    const int32 Slug = Runtime.SelectedCargoId;
    if (!TestTrue(TEXT("Welding selected a real new slug without firing"),
        Runtime.World->FindBody(Slug) && Runtime.World->FindBody(Slug)->Role == TEXT("welded_slug") && !Runtime.World->FindBody(Slug)->bLaunched)) return false;
    TestTrue(TEXT("Unselected third and fourth iron remain separate secured cargo"),
        Runtime.World->FindBody(Iron[2])->State == EExpeditionBodyState::Cargo && Runtime.World->FindBody(Iron[3])->State == EExpeditionBodyState::Cargo);
    Runtime.Click(40); Runtime.Click(2000 + Iron[3]);
    Runtime.Aim = Runtime.Magnet + FVector2D(140, 0); Runtime.bWorldHit = true;
    const auto Launch = Runtime.CommandFor(0);
    TestTrue(TEXT("Launch chooses the clicked body rather than the first cargo ID"), Launch.Action == EExpeditionAction::Launch && Launch.BodyIds.Num() == 1 && Launch.BodyIds[0] == Iron[3]);
    const int32 BeforeLaunch = Runtime.World->GetBattery();
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("A separate deliberate launch pays eight energy"), Runtime.World->GetBattery(), BeforeLaunch - 8);
    TestTrue(TEXT("Only the selected original body was launched"), Runtime.World->FindBody(Iron[3])->bLaunched && Runtime.World->FindBody(Iron[2])->State == EExpeditionBodyState::Cargo);
    TestTrue(TEXT("The prepared slug remains available for another tool"), Runtime.World->FindBody(Slug)->State == EExpeditionBodyState::Cargo && !Runtime.World->FindBody(Slug)->bLaunched);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionZeroEnergyDeliveryTest,
    "MagnetSweep.Expedition.ZeroEnergySecuredCoreDelivery",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionZeroEnergyDeliveryTest::RunTest(const FString& Parameters)
{
    // Controlled low starting charge; every body/goal transition is still earned
    // through the actual baseline operation and no energy is rewritten mid-run.
    FExpeditionWorld World; World.StartSite(3, 151, 24);
    FExpeditionRig Rig; Configure(Rig, World, TEXT("extraction_coil"), 151);
    FString Error; const bool Recovered = ReleaseAndCarryCore(World, Rig, Error);
    if (!TestTrue(*FString::Printf(TEXT("Four paid physical pulls with the final charge: %s"), *Error), Recovered)) return false;
    TestEqual(TEXT("The final committed capture consumes the last six energy"), World.GetBattery(), 0);
    TestFalse(TEXT("Zero battery does not prematurely end a secured recovery"), World.IsEnded());
    TestTrue(TEXT("An actually secured core can still be delivered at zero"), World.Dispatch().bSucceeded);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionDamagedWeldPersistenceTest,
    "MagnetSweep.Expedition.DamagedWeldPreservesSourceAndAppraisal",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionDamagedWeldPersistenceTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(0, 103);
    FExpeditionRig Rig; FString Error;
    if (!TestTrue(TEXT("Controlled Rail/Slug fixture validates"),
        FixtureRig(Rig, World, {TEXT("rail_impeller")}, {TEXT("slug_press")}, Error))) return false;
    FExpeditionCommand Gather; Gather.Magnet = FVector2D(-375, 35); Gather.Aim = Gather.Magnet; Gather.bPrecision = true;
    if (!TestTrue(TEXT("Actual collection prepares separate iron inputs"), World.Execute(Gather, Rig).bSucceeded)) return false;
    Advance(World, Rig, Gather.Magnet, 1.5f);
    TArray<int32> Iron;
    for (const auto& Body : World.GetBodies())
        if (Body.State == EExpeditionBodyState::Cargo && Body.Material == EExpeditionMaterial::Iron) Iron.Add(Body.Id);
    if (!TestTrue(TEXT("Two actual iron inputs are available"), Iron.Num() >= 2)) return false;
    // Validated damaged-input fixture isolates this persistence/value edge. It
    // does not claim a native or simulated press hit produced the damage here.
    const auto Damaged = World.ToJson();
    for (const auto& Item : Damaged->GetArrayField(TEXT("Bodies")))
        if (Item->AsObject()->GetIntegerField(TEXT("Id")) == Iron[0])
        {
            Item->AsObject()->SetNumberField(TEXT("Quality"), 50);
            Item->AsObject()->SetNumberField(TEXT("Appraisal"), Item->AsObject()->GetIntegerField(TEXT("Value")) / 2);
        }
    if (!TestTrue(TEXT("Damaged but otherwise unmodified material state validates"), World.FromJson(Damaged, Error))) return false;
    const int32 BeforeValue = World.GetCargoValue();
    FExpeditionCommand Weld; Weld.Action = EExpeditionAction::Weld;
    Weld.Magnet = Gather.Magnet; Weld.Aim = Gather.Magnet; Weld.BodyIds = {Iron[0], Iron[1]};
    if (!TestTrue(TEXT("A mixed-condition selected pair can be welded"), World.Execute(Weld, Rig).bSucceeded)) return false;
    TestEqual(TEXT("Welding cannot restore damage or silently destroy healthy material appraisal"), World.GetCargoValue(), BeforeValue);
    FExpeditionWorld Loaded;
    TestTrue(*FString::Printf(TEXT("Welded damaged material remains saveable: %s"), *Error), Loaded.FromJson(World.ToJson(), Error));
    TestTrue(TEXT("Raw source mass/value still conserves exactly"), World.CheckInvariants(Error));
    MoveMagnet(World, Rig, FVector2D(550, -246)); MoveMagnet(World, Rig, FExpeditionWorld::FurnacePosition());
    const auto Banked = World.Bank();
    TestTrue(TEXT("The actual resulting haul is bankable"), Banked.bSucceeded);
    TestEqual(TEXT("Smelt settles the preserved pre-weld appraisal once"), Banked.Output, BeforeValue);
    TestFalse(TEXT("A repeated settlement cannot duplicate welded value"), World.Bank().bSucceeded);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionOptionalReactionTest,
    "MagnetSweep.Expedition.RuntimeReactionIsOptionalAndCoupled",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionOptionalReactionTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
    if (!TestTrue(TEXT("Controlled Vector/Reaction fixture validates"),
        FixtureRig(*Runtime.Rig, *Runtime.World, {TEXT("vector_emitter")}, {TEXT("reaction_frame")}, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
    const int32 Brace = RoleBody(*Runtime.World, TEXT("brace"));
    const int32 Machine = RoleBody(*Runtime.World, TEXT("supported_machine"));
    Runtime.Magnet = Runtime.World->FindBody(Brace)->Position + FVector2D(-100, 0);
    Runtime.Aim = Runtime.World->FindBody(Brace)->Position;
    Runtime.Click(60);
    TestEqual(TEXT("Fitted Reaction does not demand a partner in basic Push"), Runtime.CommandFor(0).SecondaryId, INDEX_NONE);
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Basic Push still commits directly for six energy"), Runtime.World->GetBattery(), 94);
    TestTrue(TEXT("Basic Push actually imparts force"), Runtime.World->FindBody(Brace)->Velocity.X > 300);
    TestTrue(TEXT("Basic Push did not silently move the supported machine"), Runtime.World->FindBody(Machine)->bAnchored && Runtime.World->GetState().Constraints.IsEmpty());

    Runtime.Click(63); Runtime.Aim = Runtime.World->FindBody(Brace)->Position;
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    Runtime.Aim = Runtime.World->FindBody(Machine)->Position;
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Both explicit body-marking stages are free"), Runtime.World->GetBattery(), 94);
    TestEqual(TEXT("The selected partner survives until deliberate release"), Runtime.CommandFor(0).SecondaryId, Machine);
    Runtime.Aim = Runtime.Magnet + FVector2D(100, 0);
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Explicit coupled impulse pays twelve once"), Runtime.World->GetBattery(), 82);
    TestTrue(TEXT("Two real bodies receive opposing force"), Runtime.World->FindBody(Brace)->Velocity.X > 0 && Runtime.World->FindBody(Machine)->Velocity.X < 0);
    if (!TestEqual(TEXT("One real support constraint was created"), Runtime.World->GetState().Constraints.Num(), 1)) return false;
    const FVector2D Before = Runtime.World->FindBody(Machine)->Position;
    RuntimeAdvance(Runtime, .25f);
    TestTrue(TEXT("The formerly anchored machine moves under the coupled impulse"), FVector2D::Distance(Before, Runtime.World->FindBody(Machine)->Position) > 5);
    FExpeditionWorld Restored;
    TestTrue(TEXT("The real support pair survives strict world serialization"), Restored.FromJson(Runtime.World->ToJson(), Error));
    TestEqual(TEXT("Saved coupled action retains its exact cost"), Restored.GetBattery(), 82);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionOptionalSplitTest,
    "MagnetSweep.Expedition.RuntimeSplitKeepsBasicTransferAndConservesGroups",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionOptionalSplitTest::RunTest(const FString& Parameters)
{
    for (const bool Split : {false, true})
    {
        FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
        if (!TestTrue(TEXT("Controlled Relay/Split fixture validates"),
            FixtureRig(*Runtime.Rig, *Runtime.World, {TEXT("relay_projector")}, {TEXT("flow_splitter")}, Error))) return false;
        Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
        const int32 Source = RoleBody(*Runtime.World, TEXT("rich_scrap"));
        const auto Initial = *Runtime.World->FindBody(Source);
        const FVector2D ReceiverA = Initial.Position + FVector2D(0, 95);
        const FVector2D ReceiverB = Initial.Position + FVector2D(120, 45);
        Runtime.Click(Split ? 63 : 60); Runtime.Aim = Initial.Position;
        Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
        TestEqual(TEXT("Selecting the actual source is a free stage"), Runtime.World->GetBattery(), 100);
        Runtime.Aim = ReceiverA;
        Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
        if (Split)
        {
            TestEqual(TEXT("Split receiver A is marked without paying"), Runtime.World->GetBattery(), 100);
            Runtime.Aim = ReceiverB;
            TestTrue(TEXT("Only explicit Split sends a second endpoint"), Runtime.CommandFor(0).bHasSecondaryPoint);
            Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
        }
        TestEqual(TEXT("Only the optional split pays the additional four"), Runtime.World->GetBattery(), Split ? 86 : 90);
        const auto& State = Runtime.World->GetState();
        TestTrue(TEXT("The chosen source remains a real committed group"), State.RelayIds.Contains(Source));
        TestEqual(TEXT("Basic transfer has no compulsory secondary field"), !State.SecondRelayIds.IsEmpty(), Split);
        TArray<int32> A = State.RelayIds, B = State.SecondRelayIds;
        for (int32 Id : A) TestFalse(TEXT("No source belongs to both receivers"), B.Contains(Id));
        for (int32 Id : B)
            for (int32 Linked : Runtime.World->GetGroup(Id)) TestTrue(TEXT("Split moves whole connected groups"), B.Contains(Linked));
        const FVector2D Before = Runtime.World->FindBody(Source)->Position;
        RuntimeAdvance(Runtime, .3f);
        TestTrue(TEXT("The committed source moves physically toward its receiver"), Runtime.World->FindBody(Source)->Position.Y > Before.Y + 5);
        FExpeditionWorld Resumed;
        TestTrue(TEXT("Both pending field groups serialize without duplicating material"), Resumed.FromJson(Runtime.World->ToJson(), Error));
        TestTrue(TEXT("Current source mass/value invariants remain valid"), Runtime.World->CheckInvariants(Error));
        const int32 PaidBattery = Runtime.World->GetBattery();
        Runtime.Key(EKeys::RightMouseButton, true);
        TestTrue(TEXT("RMB cancels both pending receivers"), Runtime.World->GetState().RelayIds.IsEmpty() && Runtime.World->GetState().SecondRelayIds.IsEmpty());
        TestEqual(TEXT("Cancelling paid work does not refund the action"), Runtime.World->GetBattery(), PaidBattery);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionHeatTransferTest,
    "MagnetSweep.Expedition.RuntimeHeatTransferMovesDangerToChosenIron",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionHeatTransferTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
    if (!TestTrue(TEXT("Controlled field/heat fixture validates"),
        FixtureRig(*Runtime.Rig, *Runtime.World, {TEXT("vector_emitter")}, {TEXT("heat_sink_mould")}, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
    const int32 Hot = RoleBody(*Runtime.World, TEXT("hot_cell"));
    const int32 Sink = RoleBody(*Runtime.World, TEXT("heat_sink_iron"));
    Runtime.Click(61); Runtime.Aim = Runtime.World->FindBody(Hot)->Position;
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Marking hot source spends no battery"), Runtime.World->GetBattery(), 100);
    Runtime.Key(EKeys::RightMouseButton, true);
    TestEqual(TEXT("RMB abandons free preparation"), Runtime.PreparationStage, 0);
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    Runtime.Aim = Runtime.World->FindBody(Sink)->Position;
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Deliberate source/sink pair costs four"), Runtime.World->GetBattery(), 96);
    TestFalse(TEXT("Original source is cooled"), Runtime.World->FindBody(Hot)->bHot);
    TestTrue(TEXT("The chosen iron visibly becomes the persistent hazard"), Runtime.World->FindBody(Sink)->bHot && Runtime.World->FindBody(Sink)->bSinkUsed);
    TestTrue(TEXT("The hot component was not destroyed or invisibly detached"), Runtime.World->FindBody(Hot)->State == EExpeditionBodyState::Available && Runtime.World->GetGroup(Hot).Contains(Sink));
    TestEqual(TEXT("Manipulation creates no smelt output"), Runtime.World->GetOutput(), 0);
    const auto Saved = Runtime.World->ToJson(); FExpeditionWorld Resumed;
    TestTrue(TEXT("Heat relocation and one-use sink survive save"), Resumed.FromJson(Saved, Error));
    FExpeditionCommand Repeat; Repeat.Action = EExpeditionAction::TransferHeat; Repeat.TargetId = Hot; Repeat.SecondaryId = Sink;
    TestFalse(TEXT("The spent/cold pair cannot consume another charge"), Resumed.Execute(Repeat, *Runtime.Rig).bSucceeded);
    TestEqual(TEXT("Invalid repeat preserves energy"), Resumed.GetBattery(), 96);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionGuidedProjectileTest,
    "MagnetSweep.Expedition.RuntimeFieldLoomGuidesActualPaidProjectile",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionGuidedProjectileTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
    if (!TestTrue(TEXT("Controlled Relay/Rail/Loom fixture validates"),
        FixtureRig(*Runtime.Rig, *Runtime.World, {TEXT("relay_projector"), TEXT("rail_impeller")}, {TEXT("field_loom")}, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
    const int32 Ammo = LooseBody(*Runtime.World);
    if (!TestTrue(TEXT("Actual normal pull provides ammunition"), Pull(*Runtime.World, *Runtime.Rig, Ammo, Runtime.World->FindBody(Ammo)->Position + FVector2D(0, -55)))) return false;
    const FVector2D Entrance(-440, -160), Bend(-250, -160), Exit(-250, -260);
    Runtime.Magnet = Runtime.World->GetState().Magnet; RuntimeMove(Runtime, Entrance);
    Runtime.Click(62);
    for (const FVector2D Point : {Entrance, Bend})
    {
        Runtime.Aim = Point; Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
        TestEqual(TEXT("Marking entrance and bend does not spend energy"), Runtime.World->GetBattery(), 94);
    }
    Runtime.Aim = Exit; Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("The whole deliberate guide is one sixteen-energy commitment"), Runtime.World->GetBattery(), 78);
    TestEqual(TEXT("Guide starts as one actual future use"), Runtime.World->GetState().GuideUses, 1);
    Runtime.Click(40); Runtime.Click(2000 + Ammo); Runtime.Aim = Bend;
    Runtime.Key(EKeys::F, true); Runtime.Key(EKeys::F, false);
    TestEqual(TEXT("Guide never manufactures a free shot"), Runtime.World->GetBattery(), 70);
    bool Turned = false;
    for (int32 Frame = 0; Frame < 150; ++Frame)
    {
        Runtime.World->Tick(1.f / 60.f, Entrance, *Runtime.Rig);
        const auto* Body = Runtime.World->FindBody(Ammo);
        Turned |= Body && Body->Position.X > -290 && Body->Position.Y < -205;
    }
    TestTrue(TEXT("The launched body follows the actual bend toward the selected exit"), Turned);
    TestEqual(TEXT("Passing a real projectile consumes the guide once"), Runtime.World->GetState().GuideUses, 0);
    FExpeditionWorld Resumed;
    TestTrue(TEXT("Consumed guide and real projectile preserve strict source identity"), Resumed.FromJson(Runtime.World->ToJson(), Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionIntactGeneratorTest,
    "MagnetSweep.Expedition.IntactRecoveryPreservesUsableFiniteGenerator",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionIntactGeneratorTest::RunTest(const FString& Parameters)
{
    for (const bool Intact : {false, true})
    for (const bool SelectCage : {false, true})
    {
        AddInfo(FString::Printf(TEXT("Last-support recovery: capstone=%d, selected cage=%d"), Intact, SelectCage));
        FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
        TArray<FName> Passives; if (Intact) Passives.Add(TEXT("intact_recovery"));
        if (!TestTrue(TEXT("Controlled recovery/circuit loadout validates"),
            FixtureRig(Rig, World, {TEXT("extraction_coil"), TEXT("arc_driver")}, Passives, Error))) return false;
        const int32 Generator = RoleBody(World, TEXT("portable_generator"));
        const int32 Selected = SelectCage ? RoleBody(World, TEXT("generator_cage")) : Generator;
        if (!TestTrue(TEXT("The chosen final support is removed through actual paid motion"),
            Pull(World, Rig, Selected, World.FindBody(Selected)->Position + FVector2D(0, -55), EExpeditionAction::Extract))) return false;
        TestEqual(TEXT("The keystone preserves the real device rather than just increasing its price"), World.FindBody(Generator)->bFunctional, Intact);
        TestEqual(TEXT("Preserved device keeps two finite charges"), World.FindBody(Generator)->Charge, Intact ? 2 : 0);
        if (SelectCage)
        {
            TestTrue(TEXT("Cutting the mounting member really released the other assembly"), World.FindBody(Generator)->Links.IsEmpty());
            World.Drop(); Advance(World, Rig, World.GetState().Magnet, .1f);
            if (!TestTrue(TEXT("The now-freed unselected generator is collected through a separate paid pull"),
                Pull(World, Rig, Generator, World.FindBody(Generator)->Position + FVector2D(0, -55)))) return false;
        }
        MoveMagnet(World, Rig, FVector2D(-430, -160)); MoveMagnet(World, Rig, FVector2D(5, -160));
        World.Drop(); Advance(World, Rig, FVector2D(5, -160), .1f);
        FExpeditionCommand Arc; Arc.Action = EExpeditionAction::Arc;
        Arc.TargetId = RoleBody(World, TEXT("receiver_terminal")); Arc.Aim = World.FindBody(Arc.TargetId)->Position;
        const int32 Before = World.GetBattery();
        const auto Result = World.Execute(Arc, Rig);
        TestEqual(TEXT("Only the recovered functional source powers this otherwise open path"), Result.bSucceeded, Intact);
        TestEqual(TEXT("Valid discharge pays eight; invalid broken device costs nothing"), World.GetBattery(), Before - (Intact ? 8 : 0));
        if (Intact)
        {
            TestTrue(TEXT("Actual generator placement released the remote machinery"), World.GetState().bBallastCleared);
            TestEqual(TEXT("Supplying a discharge consumes one stored source charge"), World.FindBody(Generator)->Charge, 1);
            TestFalse(TEXT("Already completed latch cannot farm the remaining charge"), World.Execute(Arc, Rig).bSucceeded);
        }
        TestEqual(TEXT("Device operation is not a furnace payout"), World.GetOutput(), 0);
        FExpeditionWorld Loaded; TestTrue(TEXT("Device function and finite charge persist"), Loaded.FromJson(World.ToJson(), Error));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionClosedCircuitTest,
    "MagnetSweep.Expedition.ClosedCircuitPowersDistinctBranchFromFiniteSource",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionClosedCircuitTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
    if (!TestTrue(TEXT("Controlled Coil/Arc/Closed Circuit fixture validates"),
        FixtureRig(Rig, World, {TEXT("extraction_coil"), TEXT("arc_driver")}, {TEXT("closed_circuit")}, Error))) return false;
    const int32 Conductor = RoleBody(World, TEXT("conductor"));
    if (!TestTrue(TEXT("Actual seam extraction creates a portable conductor"), Pull(World, Rig, Conductor, FVector2D(-280, -160), EExpeditionAction::Extract))) return false;
    MoveMagnet(World, Rig, FExpeditionWorld::CircuitSocket()); World.Drop();
    Advance(World, Rig, FExpeditionWorld::CircuitSocket(), .1f);
    FExpeditionCommand Arc; Arc.Action = EExpeditionAction::Arc; Arc.TargetId = RoleBody(World, TEXT("receiver_terminal"));
    Arc.Aim = World.FindBody(Arc.TargetId)->Position;
    const int32 Source = RoleBody(World, TEXT("source_terminal"));
    const int32 BeforeCharge = World.FindBody(Source)->Charge;
    if (!TestTrue(TEXT("Prepared conducting return supports a real discharge"), World.Execute(Arc, Rig).bSucceeded)) return false;
    TestTrue(TEXT("The distinct arm branch also actuates through the closed return"), World.GetState().bBallastCleared && World.GetState().bArmBraced);
    TestEqual(TEXT("The additional branch consumes one real stored source charge"), World.FindBody(Source)->Charge, BeforeCharge - 1);
    const int32 Paid = World.GetBattery(); Advance(World, Rig, FExpeditionWorld::CircuitSocket(), 4.f);
    TestEqual(TEXT("Circuit stability never regenerates tool energy"), World.GetBattery(), Paid);
    TestEqual(TEXT("Finite source does not repeatedly trigger itself"), World.FindBody(Source)->Charge, BeforeCharge - 1);
    FExpeditionWorld Loaded; TestTrue(TEXT("Completed branches and source depletion validate together"), Loaded.FromJson(World.ToJson(), Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionGantryTest,
    "MagnetSweep.Expedition.WalkingGantryMovesRealSupportWithPayload",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionGantryTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
    if (!TestTrue(TEXT("Controlled Winch/Gantry fixture validates"),
        FixtureRig(Rig, World, {TEXT("anchor_winch")}, {TEXT("walking_gantry")}, Error))) return false;
    const int32 Weight = RoleBody(World, TEXT("brace"));
    const int32 Machine = RoleBody(World, TEXT("supported_machine"));
    if (!TestTrue(TEXT("Ordinary eight-kilogram body is a physically usable support"),
        Pull(World, Rig, Weight, World.FindBody(Weight)->Position + FVector2D(0, -55)))) return false;
    MoveMagnet(World, Rig, FExpeditionWorld::CounterweightPad()); World.Drop();
    Advance(World, Rig, FExpeditionWorld::CounterweightPad(), .2f);
    const FVector2D WeightBefore = World.FindBody(Weight)->Position;
    const FVector2D MachineBefore = World.FindBody(Machine)->Position;
    FExpeditionCommand Tow; Tow.Action = EExpeditionAction::Winch; Tow.TargetId = Machine;
    Tow.Magnet = FVector2D(-150, 105); Tow.Destination = Tow.Magnet; Tow.Aim = MachineBefore;
    if (!TestTrue(TEXT("Gantry accepts real resting support without requiring a welded role key"), World.Execute(Tow, Rig).bSucceeded)) return false;
    TestEqual(TEXT("Gantry's fourteen-energy commitment is paid once"), World.GetBattery(), 80);
    Advance(World, Rig, Tow.Magnet, 2.f);
    TestTrue(TEXT("The real payload travels"), World.FindBody(Machine)->Position.X < MachineBefore.X - 30);
    TestTrue(TEXT("Gantry carries the actual support with the payload"), World.FindBody(Weight)->Position.X < WeightBefore.X - 15);
    TestFalse(TEXT("Moving coupled support does not act like an abandoned static pad"), World.FindBody(Machine)->bAnchored);
    FExpeditionWorld Loaded; TestTrue(TEXT("Moving support coupling survives save"), Loaded.FromJson(World.ToJson(), Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionExtractionSupportsTest,
    "MagnetSweep.Expedition.ExtractionSupportsChangeHazardAndSeamOutcome",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionExtractionSupportsTest::RunTest(const FString& Parameters)
{
    for (const bool Insulated : {false, true})
    {
        FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
        TArray<FName> Supports; if (Insulated) Supports.Add(TEXT("insulated_jaw"));
        if (!TestTrue(TEXT("Controlled extraction fixture validates"), FixtureRig(Rig, World, {TEXT("extraction_coil")}, Supports, Error))) return false;
        const int32 Alloy = RoleBody(World, TEXT("live_alloy"));
        FExpeditionCommand Cut; Cut.Action = EExpeditionAction::Extract; Cut.TargetId = Alloy;
        Cut.Magnet = World.FindBody(Alloy)->Position + FVector2D(0, 55); Cut.Aim = World.FindBody(Alloy)->Position;
        if (!TestTrue(TEXT("A real hot-adjacent valuable seam is selected"), World.Execute(Cut, Rig).bSucceeded)) return false;
        TestEqual(TEXT("Insulation changes transferred danger, not the selected value"), World.FindBody(Alloy)->bHot, !Insulated);
        TestEqual(TEXT("The original hazard remains on the tray"), World.FindBody(RoleBody(World, TEXT("hot_cell")))->bHot, true);
        TestEqual(TEXT("Insulation does not add an unadvertised operation charge"), World.GetBattery(), 94);
    }
    FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
    if (!TestTrue(TEXT("Two seam modifiers fit with a real extraction tool"), FixtureRig(Rig, World, {TEXT("extraction_coil")}, {TEXT("cold_seam"), TEXT("crack_follower")}, Error))) return false;
    const int32 Head = RoleBody(World, TEXT("rich_bundle"));
    const int32 Middle = World.FindBody(Head)->Links[0];
    int32 Tail = INDEX_NONE; for (int32 Id : World.FindBody(Middle)->Links) if (Id != Head) Tail = Id;
    if (!TestTrue(TEXT("Authored bundle has a second real edge"), Tail != INDEX_NONE)) return false;
    FExpeditionCommand Cut; Cut.Action = EExpeditionAction::Extract; Cut.TargetId = Head;
    Cut.Magnet = World.FindBody(Head)->Position + FVector2D(0, -55); Cut.Aim = World.FindBody(Head)->Position;
    if (!TestTrue(TEXT("Composite cut executes on the actual bundle"), World.Execute(Cut, Rig).bSucceeded)) return false;
    TestEqual(TEXT("Cut and the two finite seam effects pay eleven together"), World.GetBattery(), 89);
    TestTrue(TEXT("Cold Seam leaves the actual adjacent metal brittle"), World.FindBody(Middle)->bBrittle);
    TestFalse(TEXT("Crack Follower severs exactly the adjacent second edge"), World.FindBody(Middle)->Links.Contains(Tail) || World.FindBody(Tail)->Links.Contains(Middle));
    TestEqual(TEXT("The other authored bundle remains linked"), World.GetGroup(Head + 3).Num(), 3);
    FExpeditionWorld Loaded; TestTrue(TEXT("Changed seam topology remains saveable"), Loaded.FromJson(World.ToJson(), Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionVectorSupportsTest,
    "MagnetSweep.Expedition.VectorBrakeAndPhysicalShearHaveDistinctEffects",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionVectorSupportsTest::RunTest(const FString& Parameters)
{
    FExpeditionWorld BrakeWorld; BrakeWorld.StartSite(0, 103); FExpeditionRig BrakeRig; FString Error;
    if (!TestTrue(TEXT("Controlled brake fixture validates"), FixtureRig(BrakeRig, BrakeWorld, {TEXT("vector_emitter")}, {TEXT("eddy_brake")}, Error))) return false;
    int32 Moving = INDEX_NONE; for (const auto& B : BrakeWorld.GetBodies()) if (B.Velocity.Size() > 100) { Moving = B.Id; break; }
    if (!TestTrue(TEXT("A real moving body exists without a synthetic velocity fixture"), Moving != INDEX_NONE)) return false;
    const float BeforeSpeed = BrakeWorld.FindBody(Moving)->Velocity.Size();
    FExpeditionCommand Push; Push.Action = EExpeditionAction::Vector; Push.TargetId = Moving;
    Push.Aim = BrakeWorld.FindBody(Moving)->Position; Push.Magnet = Push.Aim - FVector2D(100, 0);
    TestTrue(TEXT("Eddy Brake acts on the actual moving drum"), BrakeWorld.Execute(Push, BrakeRig).bSucceeded);
    TestTrue(TEXT("Real motion becomes less speed and a heat consequence"), BrakeWorld.FindBody(Moving)->Velocity.Size() < BeforeSpeed / 2 && BrakeWorld.FindBody(Moving)->bHot);
    TestEqual(TEXT("The actual brake pays its extra two"), BrakeWorld.GetBattery(), 92);

    FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig;
    // The rig's explicit seam-support prerequisite is the Sever capability
    // supplied by an equipped Coil. Shear alone currently supplies no rig tag.
    const bool ValidShearRig = FixtureRig(Rig, World, {TEXT("vector_emitter"), TEXT("extraction_coil")},
        {TEXT("shear_gate"), TEXT("cold_seam"), TEXT("crack_follower")}, Error);
    if (!TestTrue(*FString::Printf(TEXT("Controlled physical shear fixture validates: %s"), *Error), ValidShearRig)) return false;
    const int32 Head = RoleBody(World, TEXT("rich_bundle"));
    const int32 Middle = World.FindBody(Head)->Links[0];
    // Isolate the cutting rule with a clear upward corridor. A sideways shove
    // into the two neighboring bundles is allowed to stop before the plane.
    Push.TargetId = Head; Push.Aim = World.FindBody(Head)->Position; Push.Magnet = Push.Aim + FVector2D(0, 100);
    if (!TestTrue(TEXT("Shove pays for a real downstream shear plane"), World.Execute(Push, Rig).bSucceeded)) return false;
    TestFalse(TEXT("Buying the shove does not instantly sever its target"), World.FindBody(Head)->Links.IsEmpty());
    TestEqual(TEXT("Shear and seam operations pay their full composed cost"), World.GetBattery(), 86);
    FExpeditionWorld Saved; TestTrue(TEXT("Pending plane and candidate source IDs serialize"), Saved.FromJson(World.ToJson(), Error));
    Advance(World, Rig, Push.Magnet, 1.5f);
    AddInfo(FString::Printf(TEXT("Shear trajectory: head=(%.1f,%.1f), plane=(%.1f,%.1f), normal=(%.2f,%.2f), brittle=%d, links=%d, remaining=%.3f"),
        World.FindBody(Head)->Position.X, World.FindBody(Head)->Position.Y,
        World.GetState().ShearOrigin.X, World.GetState().ShearOrigin.Y,
        World.GetState().ShearNormal.X, World.GetState().ShearNormal.Y,
        World.FindBody(Head)->bBrittle, World.FindBody(Head)->Links.Num(), World.GetState().ShearRemaining));
    TestTrue(TEXT("Actual movement through the shear plane cuts the head seam"), World.FindBody(Head)->Links.IsEmpty());
    TestTrue(TEXT("Seam support acts on the physically crossed adjacent material"), World.FindBody(Middle)->bBrittle);
    TestTrue(TEXT("Cross-family trigger cannot duplicate source material"), World.CheckInvariants(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionSensorRelayTest,
    "MagnetSweep.Expedition.RuntimeSensorRelayRequiresChosenPhysicalImpact",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionSensorRelayTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
    if (!TestTrue(TEXT("Controlled Arc/Rail sensor fixture validates"), FixtureRig(*Runtime.Rig, *Runtime.World,
        {TEXT("arc_driver"), TEXT("rail_impeller")}, {TEXT("escapement_relay")}, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
    const int32 Sensor = RoleBody(*Runtime.World, TEXT("brace"));
    const int32 Receiver = RoleBody(*Runtime.World, TEXT("ordinary_terminal"));
    Runtime.Click(62); Runtime.Aim = Runtime.World->FindBody(Sensor)->Position;
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Marking the sensor is free"), Runtime.World->GetBattery(), 100);
    Runtime.Aim = Runtime.World->FindBody(Receiver)->Position;
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Selecting the receiver reserves one real eleven-energy charge"), Runtime.World->GetBattery(), 89);
    TestEqual(TEXT("The selected body is retained as the actual sensor"), Runtime.World->GetState().DeferredSensor, Sensor);
    RuntimeAdvance(Runtime, .25f);
    TestFalse(TEXT("Passing time or committing the relay does not immediately park the arm"), Runtime.World->GetState().bArmBraced);
    FExpeditionWorld Restored; TestTrue(TEXT("Pending sensor and reserved charge survive save"), Restored.FromJson(Runtime.World->ToJson(), Error));
    if (!TestTrue(TEXT("The same marked body can be physically secured"), Pull(*Runtime.World, *Runtime.Rig, Sensor, Runtime.World->FindBody(Sensor)->Position + FVector2D(0, -55)))) return false;
    Runtime.Magnet = Runtime.World->GetState().Magnet;
    RuntimeMove(Runtime, FVector2D(Runtime.Magnet.X, -270)); RuntimeMove(Runtime, FVector2D(450, -270));
    Runtime.Click(40); Runtime.Click(2000 + Sensor); Runtime.Aim = FVector2D(650, -270);
    Runtime.Key(EKeys::F, true); Runtime.Key(EKeys::F, false);
    TestEqual(TEXT("The sensor is propelled by a separately paid shot"), Runtime.World->GetBattery(), 75);
    for (int32 Frame = 0; Frame < 60 && Runtime.World->GetState().DeferredCharge > 0; ++Frame)
        Runtime.World->Tick(1.f / 60.f, Runtime.Magnet, *Runtime.Rig);
    TestTrue(TEXT("The chosen body's wall impact actually actuates its selected terminal"), Runtime.World->GetState().bArmBraced);
    TestEqual(TEXT("The physical impact consumes the reserved charge exactly once"), Runtime.World->GetState().DeferredCharge, 0);
    const int32 After = Runtime.World->GetBattery();
    Advance(*Runtime.World, *Runtime.Rig, Runtime.Magnet, 2.f);
    TestEqual(TEXT("Later impacts do not bill or refund another discharge"), Runtime.World->GetBattery(), After);
    TestEqual(TEXT("Operating the relay is not smelt output"), Runtime.World->GetOutput(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionCycloneConservationTest,
    "MagnetSweep.Expedition.CycloneConservesActualImpactFragmentsAndPaidRelaunch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionCycloneConservationTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
    if (!TestTrue(TEXT("Controlled Rail/Slug/Cyclone/Rebound fixture validates"), FixtureRig(*Runtime.Rig, *Runtime.World,
        {TEXT("rail_impeller")}, {TEXT("slug_press"), TEXT("salvage_cyclone"), TEXT("rebound_plate")}, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
    FExpeditionCommand Gather; Gather.Magnet = FVector2D(-375, 35); Gather.Aim = Gather.Magnet; Gather.bPrecision = true;
    if (!TestTrue(TEXT("A real collection supplies conserved fragment sources"), Runtime.World->Execute(Gather, *Runtime.Rig).bSucceeded)) return false;
    Advance(*Runtime.World, *Runtime.Rig, Gather.Magnet, 1.5f);
    FExpeditionCommand Weld; Weld.Action = EExpeditionAction::Weld; Weld.Magnet = Gather.Magnet; Weld.Aim = Gather.Magnet;
    for (const auto& B : Runtime.World->GetBodies()) if (B.State == EExpeditionBodyState::Cargo && B.Material == EExpeditionMaterial::Iron) Weld.BodyIds.Add(B.Id);
    const auto Made = Runtime.World->Execute(Weld, *Runtime.Rig);
    if (!TestTrue(TEXT("Actual weld supplies one conserved multi-source projectile"), Made.bSucceeded && Made.BodyIds.Num() == 1)) return false;
    const int32 Slug = Made.BodyIds[0]; const auto Original = *Runtime.World->FindBody(Slug);
    Runtime.Magnet = Runtime.World->GetState().Magnet;
    RuntimeMove(Runtime, FVector2D(-375, -270)); RuntimeMove(Runtime, FVector2D(450, -270));
    Runtime.Click(40); Runtime.Click(2000 + Slug); Runtime.Aim = FVector2D(650, -270);
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Cyclone's paid launch follows the real gather and weld"), Runtime.World->GetBattery(), 76);
    for (int32 Frame = 0; Frame < 90 && Runtime.World->GetState().CycloneIds.IsEmpty(); ++Frame)
        Runtime.World->Tick(1.f / 60.f, Runtime.Magnet, *Runtime.Rig);
    const auto Fragments = Runtime.World->GetState().CycloneIds;
    if (!TestTrue(TEXT("Actual wall collision creates finite material fragments"), Fragments.Num() >= 2)) return false;
    float Mass = 0; int32 Value = 0, Appraisal = 0; TSet<int32> Sources;
    for (int32 Id : Fragments)
    {
        const auto* B = Runtime.World->FindBody(Id); Mass += B->Mass; Value += B->Value; Appraisal += B->Appraisal;
        for (int32 Source : B->SourceIds)
        {
            TestTrue(TEXT("Each original source appears in exactly one live fragment"), !Sources.Contains(Source));
            Sources.Add(Source);
        }
    }
    TestTrue(TEXT("Fragment mass and raw value are conserved"), FMath::IsNearlyEqual(Mass, Original.Mass) && Value == Original.Value);
    TestTrue(TEXT("Fragment appraisal cannot exceed the actual paid body's condition"), Appraisal <= Original.Appraisal);
    TestTrue(TEXT("The original composite is consumed rather than duplicated"), Runtime.World->FindBody(Slug)->State == EExpeditionBodyState::Consumed);
    FExpeditionWorld Saved; TestTrue(TEXT("Actual split source lineage survives save"), Saved.FromJson(Runtime.World->ToJson(), Error));
    const int32 Fragment = Fragments.Last(); Runtime.Click(2000 + Fragment);
    Runtime.Aim = Runtime.World->FindBody(Fragment)->Position + FVector2D(-140, 0);
    const int32 Before = Runtime.World->GetBattery(); Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Relaunching an orbit fragment is another paid fourteen-energy action"), Runtime.World->GetBattery(), Before - 14);
    TestFalse(TEXT("The selected fragment leaves the orbit pool"), Runtime.World->GetState().CycloneIds.Contains(Fragment));
    TestTrue(TEXT("It is the selected real fragment that is launched"), Runtime.World->FindBody(Fragment)->bLaunched);
    TestTrue(TEXT("Paid relaunch cannot duplicate source mass/value"), Runtime.World->CheckInvariants(Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionEarnedRefiningRoutesTest,
    "MagnetSweep.Expedition.EarnedSiteTwoBuildsReachBothRefiningRewards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionEarnedRefiningRoutesTest::RunTest(const FString& Parameters)
{
    // Find a real deterministic starting stock containing the desired five-
    // credit support. No inventory, reward, source-value or phase injection.
    for (const bool UseWinch : {true, false})
    {
        FExpeditionRuntime Runtime(nullptr); FString Error;
        const FName Starter = UseWinch ? FName(TEXT("anchor_winch")) : FName(TEXT("extraction_coil"));
        const FName Support = UseWinch ? FName(TEXT("counterweight_hook")) : FName(TEXT("insulated_jaw"));
        bool Found = false;
        for (int32 Seed = 1; Seed <= 128 && !Found; ++Seed)
        {
            Runtime.World->StartSite(0, Seed);
            if (!Configure(*Runtime.Rig, *Runtime.World, Starter, Seed)) continue;
            Found = Runtime.Rig->GetOffers().Contains(Support) && Runtime.Rig->CanBuy(Support, Error);
        }
        if (!TestTrue(TEXT("Real saved stock supplies the chosen opening direction"), Found)) return false;
        if (!TestTrue(TEXT("The opening support is actually paid and fitted"), Runtime.Rig->Buy(Support, Error) && Runtime.Rig->Fit(Support, Error))) return false;
        Runtime.Screen = EExpeditionScreen::Depot; Runtime.Depart();
        const bool FirstRecovered = RuntimeRecoverCore(Runtime, Error);
        if (!TestTrue(*FString::Printf(TEXT("Actual first-site inputs earn the second depot: %s"), *Error), FirstRecovered)) return false;
        Runtime.Key(EKeys::E, true);
        TestTrue(TEXT("Actual core delivery advances exactly one site"), Runtime.Screen == EExpeditionScreen::Depot && Runtime.SiteIndex == 1);
        TestEqual(TEXT("Paid opening support and real clear leave seventeen credits"), Runtime.Rig->GetCash(), 17);
        Runtime.Depart();
        if (!TestTrue(TEXT("The earned rig actually departs for the revised physical worksite"), Runtime.Screen == EExpeditionScreen::Site && Runtime.SiteIndex == 1)) return false;
        const int32 BeforeCash = Runtime.Rig->GetCash();
        const int32 Prize = RoleBody(*Runtime.World, UseWinch ? FName(TEXT("supported_machine")) : FName(TEXT("live_alloy")));
        if (UseWinch)
        {
            const int32 Weight = RoleBody(*Runtime.World, TEXT("brace"));
            if (!TestTrue(TEXT("Ordinary paid pickup provides the real counterweight"), Pull(*Runtime.World, *Runtime.Rig, Weight, Runtime.World->FindBody(Weight)->Position + FVector2D(0, -55)))) return false;
            MoveMagnet(*Runtime.World, *Runtime.Rig, FExpeditionWorld::CounterweightPad());
            Runtime.World->Drop(); Advance(*Runtime.World, *Runtime.Rig, FExpeditionWorld::CounterweightPad(), .1f);
            Runtime.Magnet = Runtime.World->GetState().Magnet; RuntimeMove(Runtime, FVector2D(-120, 105));
            Runtime.Aim = Runtime.World->FindBody(Prize)->Position;
            Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
            Advance(*Runtime.World, *Runtime.Rig, Runtime.Magnet, 1.5f);
            if (!TestTrue(TEXT("Earned Winch/Hook physically releases the valuable machine"), !Runtime.World->FindBody(Prize)->bAnchored)) return false;
            if (!TestTrue(TEXT("Released machine is secured by another real paid pull"), Pull(*Runtime.World, *Runtime.Rig, Prize, Runtime.World->FindBody(Prize)->Position + FVector2D(0, -55)))) return false;
        }
        else
        {
            RuntimeMove(Runtime, Runtime.World->FindBody(Prize)->Position + FVector2D(0, -55));
            Runtime.Aim = Runtime.World->FindBody(Prize)->Position;
            Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
            Advance(*Runtime.World, *Runtime.Rig, Runtime.Magnet, 1.5f);
            if (!TestTrue(TEXT("Earned Coil/Jaw safely extracts the high-value live seam"), Runtime.World->FindBody(Prize)->State == EExpeditionBodyState::Cargo && !Runtime.World->IsUnsafe())) return false;
        }
        Runtime.Magnet = Runtime.World->GetState().Magnet;
        RuntimeMove(Runtime, FVector2D(Runtime.Magnet.X, -270)); RuntimeMove(Runtime, FVector2D(650, -270));
        RuntimeMove(Runtime, FExpeditionWorld::FurnacePosition()); Runtime.Key(EKeys::E, true);
        TestEqual(TEXT("Only actual smelting settles the recovered special material"), Runtime.World->GetOutput(), UseWinch ? 320 : 324);
        TestEqual(TEXT("First real refining threshold pays once"), Runtime.Rig->GetCash(), BeforeCash + 2);
        RuntimeMove(Runtime, FVector2D(650, -270)); RuntimeMove(Runtime, FVector2D(185, -246));
        Runtime.Key(EKeys::LeftMouseButton, true); RuntimeAdvance(Runtime, 1.5f); Runtime.Key(EKeys::LeftMouseButton, false);
        if (!TestTrue(TEXT("A real safe six-alloy supplement is available"), FMath::IsNearlyEqual(Runtime.World->GetCargoMass(), 24.f) && !Runtime.World->IsUnsafe())) return false;
        RuntimeMove(Runtime, FVector2D(650, -246)); RuntimeMove(Runtime, FExpeditionWorld::FurnacePosition()); Runtime.Key(EKeys::E, true);
        TestEqual(TEXT("Actual build route plus six loose pieces reaches the second threshold"), Runtime.World->GetOutput(), UseWinch ? 362 : 366);
        TestEqual(TEXT("Both visible milestones become four real spending credits"), Runtime.Rig->GetCash(), BeforeCash + 4);
        Runtime.Key(EKeys::E, true);
        TestEqual(TEXT("A repeated empty pour cannot duplicate either milestone"), Runtime.Rig->GetCash(), BeforeCash + 4);
        TestTrue(TEXT("The earned world and reward ledger still validate"), Runtime.World->CheckInvariants(Error) && Runtime.Rig->Validate(Error));
        FExpeditionRuntime Resumed(nullptr);
        TestTrue(TEXT("Actual earned optional route saves and restores transactionally"), Resumed.DecodeSave(Runtime.EncodeSave(), Error));
    }
    return true;
}

#endif
