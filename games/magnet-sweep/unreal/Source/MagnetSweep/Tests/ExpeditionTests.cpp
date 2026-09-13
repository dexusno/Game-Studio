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

bool RuntimePickBody(FExpeditionRuntime& Runtime, int32 Id, const FVector2D& Approach, FString& Error)
{
    const auto* Body = Runtime.World->FindBody(Id);
    if (!Body) { Error = TEXT("Missing actual pickup body"); return false; }
    RuntimeMove(Runtime, Body->Position + Approach);
    Runtime.Key(EKeys::LeftShift, true); Runtime.Key(EKeys::LeftMouseButton, true);
    RuntimeAdvance(Runtime, 1.4f);
    Runtime.Key(EKeys::LeftMouseButton, false); Runtime.Key(EKeys::LeftShift, false);
    if (Runtime.World->FindBody(Id)->State != EExpeditionBodyState::Cargo)
    { Error = TEXT("Actual precision did not secure ") + Runtime.World->FindBody(Id)->Role.ToString(); return false; }
    return true;
}

void RuntimePlaceHaul(FExpeditionRuntime& Runtime, const FVector2D& Destination)
{
    RuntimeMove(Runtime, Destination); Runtime.Key(EKeys::RightMouseButton, true); RuntimeAdvance(Runtime, .25f);
}

bool RuntimeBalanceRack(FExpeditionRuntime& Runtime, FString& Error)
{
    const auto* Left = Runtime.World->FindMarker(TEXT("balance_left"));
    const auto* Right = Runtime.World->FindMarker(TEXT("balance_right"));
    if (!Left || !Right) { Error = TEXT("Balanced worksite is missing its visible platforms"); return false; }
    const FVector2D L = Left->Position, R = Right->Position;
    if (!RuntimePickBody(Runtime, RoleBody(*Runtime.World, TEXT("ballast")), FVector2D(0, 25), Error)) return false;
    RuntimePlaceHaul(Runtime, L);
    if (!RuntimePickBody(Runtime, RoleBody(*Runtime.World, TEXT("brace")), FVector2D(0, 25), Error)) return false;
    RuntimePlaceHaul(Runtime, R);
    // These two stable source pieces are ordinary iron in the authored rack,
    // not keys. Their actual mass and positions enter the same platform rule.
    for (int32 I = 0; I < 2; ++I)
    {
        if (!RuntimePickBody(Runtime, 12 + I, FVector2D(0, -25), Error)) return false;
        RuntimePlaceHaul(Runtime, R + FVector2D(I == 0 ? -45 : 45, 0));
    }
    if (!Runtime.World->IsCoreReleased()) { Error = TEXT("Actually placed twelve-versus-twelve platforms did not release the core"); return false; }
    return true;
}

bool RuntimeRecoverCore(FExpeditionRuntime& Runtime, FString& Error)
{
    const FName Layout = Runtime.World->GetState().LayoutId;
    if (Layout == TEXT("balanced_rack"))
    {
        if (!RuntimeBalanceRack(Runtime, Error)) return false;
        if (!RuntimePickBody(Runtime, RoleBody(*Runtime.World, TEXT("core")), FVector2D(0, 25), Error)) return false;
        RuntimeMove(Runtime, FVector2D(Runtime.Magnet.X, -260));
        RuntimeMove(Runtime, FExpeditionWorld::ReceiverPosition()); return true;
    }
    if (Layout == TEXT("counterweight_exchange"))
    {
        const auto* CoverPark = Runtime.World->FindMarker(TEXT("cover"));
        const auto* Stage = Runtime.World->FindMarker(TEXT("staging"));
        const auto* Seat = Runtime.World->FindMarker(TEXT("counterbalance"));
        if (!CoverPark || !Stage || !Seat) { Error = TEXT("Final worksite is missing its actual recovery markers"); return false; }
        const FVector2D ParkPoint = CoverPark->Position, StagePoint = Stage->Position, SeatPoint = Seat->Position;
        const int32 Cover = RoleBody(*Runtime.World, TEXT("counterweight_cover")), Core = RoleBody(*Runtime.World, TEXT("core"));
        if (!RuntimePickBody(Runtime, Cover, FVector2D(0, 25), Error)) return false;
        RuntimePlaceHaul(Runtime, ParkPoint);
        if (!RuntimePickBody(Runtime, Core, FVector2D(0, 25), Error)) return false;
        RuntimeMove(Runtime, FVector2D(Runtime.Magnet.X, -260)); RuntimePlaceHaul(Runtime, StagePoint);
        if (!RuntimePickBody(Runtime, Cover, FVector2D(0, 25), Error)) return false;
        RuntimePlaceHaul(Runtime, SeatPoint);
        if (!RuntimePickBody(Runtime, Core, FVector2D(0, -25), Error)) return false;
        RuntimeMove(Runtime, FExpeditionWorld::ReceiverPosition()); return true;
    }
    if (Layout != TEXT("e1")) { Error = TEXT("No independently authored input route for layout ") + Layout.ToString(); return false; }
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

bool FireActualBrace(FExpeditionWorld& World, const FExpeditionRig& Rig,
                     const FVector2D& Position, const FVector2D& Aim, int32& Shot)
{
    Shot = RoleBody(World, TEXT("brace"));
    if (!Pull(World, Rig, Shot, World.FindBody(Shot)->Position + FVector2D(0, -55))) return false;
    MoveMagnet(World, Rig, FVector2D(World.GetState().Magnet.X, -270));
    MoveMagnet(World, Rig, FVector2D(Position.X, -270)); MoveMagnet(World, Rig, Position);
    FExpeditionCommand Launch; Launch.Action = EExpeditionAction::Launch; Launch.TargetId = Shot;
    Launch.BodyIds = {Shot}; Launch.Magnet = Position; Launch.Aim = Aim;
    return World.Execute(Launch, Rig).bSucceeded;
}

void EncodeLegacyWorldV2(const TSharedPtr<FJsonObject>& World)
{
    // Controlled historical-format fixture; no owner save is read. The actual
    // source bodies, paid motion and earned ledger are deliberately unchanged.
    World->SetNumberField(TEXT("Version"), 2);
    World->RemoveField(TEXT("LayoutId")); World->RemoveField(TEXT("LayoutRevision"));
    for (const auto& Body : World->GetArrayField(TEXT("Bodies"))) Body->AsObject()->RemoveField(TEXT("PenetratedBody"));
}

bool ReachRackWithFixtureEquipment(FExpeditionRuntime& Runtime, const TArray<FName>& Actives,
                                  const TArray<FName>& Passives, FString& Error)
{
    // Only initial equipment is a labelled found-gear fixture. All subsequent
    // site history, energy, source appraisal and refining rewards are earned.
    Runtime.World->StartSite(0,103);
    if (!FixtureRig(*Runtime.Rig,*Runtime.World,Actives,Passives,Error)) return false;
    Runtime.Screen=EExpeditionScreen::Site; Runtime.SiteIndex=0; Runtime.bWorldHit=true;
    for (int32 Site=0; Site<2; ++Site)
    {
        if (!RuntimeRecoverCore(Runtime,Error)) return false;
        Runtime.Key(EKeys::E,true);
        if (Runtime.Screen!=EExpeditionScreen::Depot || Runtime.SiteIndex!=Site+1)
        { Error=TEXT("Actual earlier dispatch did not advance the earned fixture"); return false; }
        if (Site==0) Runtime.Depart();
    }
    if (!Runtime.Rig->Precharge(Error)) return false; // Real four-credit purchase.
    Runtime.Depart();
    return Runtime.Screen==EExpeditionScreen::Site && Runtime.World->GetState().LayoutId==TEXT("balanced_rack");
}

void RuntimeSmelt(FExpeditionRuntime& Runtime)
{
    RuntimeMove(Runtime,{Runtime.Magnet.X,-280}); RuntimeMove(Runtime,{650,-280});
    RuntimeMove(Runtime,FExpeditionWorld::FurnacePosition()); Runtime.Key(EKeys::E,true);
}

bool EarnFirstSiteKeystoneBudget(FExpeditionRuntime& Runtime, FName Starter, int32 Seed, FString& Error)
{
    // No fixture equipment, synthetic rewards or inventory edits: this is the
    // actual free starter, five original sources, one pour and core delivery.
    Runtime.World->StartSite(0,Seed);
    if (!Configure(*Runtime.Rig,*Runtime.World,Starter,Seed)) return false;
    Runtime.Screen=EExpeditionScreen::Depot; Runtime.Depart();
    if (Runtime.Screen!=EExpeditionScreen::Site) return false;
    for (int32 Id=36; Id<=40; ++Id)
    {
        if (!RuntimePickBody(Runtime,Id,FVector2D(0,-25),Error)) return false;
    }
    RuntimeSmelt(Runtime);
    if (Runtime.World->GetOutput()!=120 || Runtime.Rig->GetCash()!=14)
    { Error=TEXT("Actual first-site source recovery did not earn its first milestone"); return false; }
    if (!RuntimeRecoverCore(Runtime,Error)) return false;
    Runtime.Key(EKeys::E,true);
    if (Runtime.Screen!=EExpeditionScreen::Depot || Runtime.SiteIndex!=1 || Runtime.Rig->GetCash()!=24)
    { Error=TEXT("Actual first-site delivery did not leave the earned twenty-four credits"); return false; }
    return true;
}

bool RuntimeMoveFrameWithGantry(FExpeditionRuntime& Runtime, FString& Error, bool bRetainFinalCore=false)
{
    const auto* Frame=Runtime.World->FindFrameBody();
    const auto* Support=Runtime.World->FindMarker(TEXT("frame_support"));
    const auto* Dock=Runtime.World->FindMarker(TEXT("frame_receiver"));
    if (!Frame || !Support || !Dock) { Error=TEXT("Actual frame definition is missing"); return false; }
    const int32 FrameId=Frame->Id;
    const int32 Weight=RoleBody(*Runtime.World,TEXT("brace"));
    if (!RuntimePickBody(Runtime,Weight,{0,25},Error)) return false;
    RuntimePlaceHaul(Runtime,Support->Position);
    if (bRetainFinalCore)
    {
        if (!RuntimePickBody(Runtime,RoleBody(*Runtime.World,TEXT("counterweight_cover")),{0,25},Error)) return false;
        RuntimePlaceHaul(Runtime,Runtime.World->FindMarker(TEXT("cover"))->Position);
        if (!RuntimePickBody(Runtime,RoleBody(*Runtime.World,TEXT("core")),{0,25},Error)) return false;
    }
    RuntimeMove(Runtime,Dock->Position);
    Runtime.Aim=Runtime.World->FindBody(FrameId)->Position;
    Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
    Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,4.f);
    return Runtime.World->CanReceiveFrame(Error);
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
    FExpeditionCommand Broad; Broad.Aim = Original; Broad.Magnet = Original;
    const auto BroadPreview = World.Preview(Broad, Rig);
    TestTrue(TEXT("A broad field can select the legitimate nearby cover"), BroadPreview.bAllowed);
    TestFalse(TEXT("That broad field cannot include the anchored objective"), BroadPreview.BodyIds.Contains(Core));
    for (const auto Action : {EExpeditionAction::Attract, EExpeditionAction::Extract})
    {
        FExpeditionCommand Command;
        Command.Action = Action; Command.TargetId = Core; Command.BodyIds = {Core};
        Command.Magnet = Original + FVector2D(0, -50); Command.Aim = Original;
        Command.bPrecision = true;
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
    // Frozen E1 acceptance; this does not certify a later default finale.
    FExpeditionWorld World; World.StartSite(3, 97, 100, TEXT("e1"), 1);
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
        const FName ExpectedLayout = Site < 2 ? FName(TEXT("e1")) : Site == 2 ? FName(TEXT("balanced_rack")) : FName(TEXT("counterweight_exchange"));
        TestTrue(TEXT("A current new run enters its real default authored layout"), Runtime.World->GetState().LayoutId == ExpectedLayout);
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
    // Four pulls is the explicit frozen E1 route, not a future-layout quota.
    FExpeditionWorld World; World.StartSite(3, 151, 24, TEXT("e1"), 1);
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

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionGroundRelevanceTest,
    "MagnetSweep.Expedition.RuntimeGroundChangesOnlyItsPhysicalCircuitBranch",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionGroundRelevanceTest::RunTest(const FString& Parameters)
{
    for (FName GroundRole : {FName(TEXT("brace")), FName(TEXT("return_diode")), FName(TEXT("source_terminal"))})
    {
        FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
        if (!TestTrue(TEXT("Labelled Coil/Arc/Ground fixture validates"), FixtureRig(*Runtime.Rig, *Runtime.World,
            {TEXT("extraction_coil"), TEXT("arc_driver")}, {TEXT("ground_clip")}, Error))) return false;
        Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
        const int32 Conductor = RoleBody(*Runtime.World, TEXT("conductor"));
        if (!TestTrue(TEXT("Actual seam extraction supplies the circuit"), Pull(*Runtime.World, *Runtime.Rig, Conductor, FVector2D(-280, -160), EExpeditionAction::Extract))) return false;
        MoveMagnet(*Runtime.World, *Runtime.Rig, FExpeditionWorld::CircuitSocket()); Runtime.World->Drop();
        Advance(*Runtime.World, *Runtime.Rig, FExpeditionWorld::CircuitSocket(), .1f);
        Runtime.Magnet = Runtime.World->GetState().Magnet;
        const int32 Ground = RoleBody(*Runtime.World, GroundRole);
        Runtime.Click(65); Runtime.Aim = Runtime.World->FindBody(Ground)->Position;
        Runtime.Key(EKeys::F, true); Runtime.Key(EKeys::F, false);
        TestEqual(TEXT("The real Ground button selects the aimed endpoint for no battery"), Runtime.World->GetBattery(), 94);
        TestEqual(TEXT("Ground keeps the actual selected body identity"), Runtime.World->GetState().GroundBody, Ground);
        Runtime.Click(64); Runtime.Aim = Runtime.World->FindBody(RoleBody(*Runtime.World, TEXT("receiver_terminal")))->Position;
        Runtime.Key(EKeys::F, true); Runtime.Key(EKeys::F, false);
        const bool SourceBlocked = GroundRole == TEXT("source_terminal");
        TestEqual(*FString::Printf(TEXT("Ground at %s has the correct actual circuit outcome"), *GroundRole.ToString()), Runtime.World->GetState().bBallastCleared, !SourceBlocked);
        TestEqual(TEXT("A blocked path refuses without buying an arc"), Runtime.World->GetBattery(), SourceBlocked ? 94 : 86);
        TestEqual(TEXT("Unrelated ground cannot magically prevent return heat"), Runtime.World->FindBody(Conductor)->bHot, GroundRole == TEXT("brace"));
        FExpeditionWorld Loaded; TestTrue(TEXT("Chosen termination and resulting heat remain saveable"), Loaded.FromJson(Runtime.World->ToJson(), Error));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionConductiveTetherTest,
    "MagnetSweep.Expedition.StoredPhysicalTetherEnablesOtherwiseOpenCircuit",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionConductiveTetherTest::RunTest(const FString& Parameters)
{
    for (const bool Conductive : {false, true})
    {
        FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
        TArray<FName> Supports = {TEXT("twin_anchor")}; if (Conductive) Supports.Add(TEXT("conductive_tether"));
        if (!TestTrue(TEXT("Labelled Winch/Arc/stored-anchor fixture validates"), FixtureRig(*Runtime.Rig, *Runtime.World,
            {TEXT("anchor_winch"), TEXT("arc_driver")}, Supports, Error))) return false;
        Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
        const int32 WireEnd = RoleBody(*Runtime.World, TEXT("brace"));
        if (!TestTrue(TEXT("A real body is collected for the wire endpoint"), Pull(*Runtime.World, *Runtime.Rig, WireEnd, FVector2D(-150, 75)))) return false;
        MoveMagnet(*Runtime.World, *Runtime.Rig, FVector2D(20, -160)); Runtime.World->Drop();
        Advance(*Runtime.World, *Runtime.Rig, FVector2D(20, -160), .1f);
        Runtime.Magnet = Runtime.World->GetState().Magnet; RuntimeMove(Runtime, FVector2D(-180, -160));
        Runtime.Aim = Runtime.World->FindBody(WireEnd)->Position;
        Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
        const int32 Other = RoleBody(*Runtime.World, TEXT("ballast"));
        Runtime.Aim = Runtime.World->FindBody(Other)->Position;
        Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
        TestEqual(TEXT("Two real placed anchors cost sixteen after the six-energy collection"), Runtime.World->GetBattery(), 78);
        TestEqual(TEXT("The source wire is a retained paid anchor, not a fabricated graph edge"), Runtime.World->GetState().SecondTetherBody, WireEnd);
        RuntimeAdvance(Runtime, .35f);
        const int32 Receiver = RoleBody(*Runtime.World, TEXT("receiver_terminal"));
        Runtime.Aim = Runtime.World->FindBody(Receiver)->Position;
        Runtime.Key(EKeys::F, true); Runtime.Key(EKeys::F, false);
        TestEqual(TEXT("Only conductive metal in that real tether completes the open path"), Runtime.World->GetState().bBallastCleared, Conductive);
        TestEqual(TEXT("Conductive effect still requires a separately paid arc"), Runtime.World->GetBattery(), Conductive ? 70 : 78);
        FExpeditionWorld Loaded; TestTrue(TEXT("The physical endpoint and retained paid anchor serialize together"), Loaded.FromJson(Runtime.World->ToJson(), Error));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionInductionGapTest,
    "MagnetSweep.Expedition.InductionBridgesOneActuallyPlacedAirGap",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionInductionGapTest::RunTest(const FString& Parameters)
{
    for (const bool Induction : {false, true})
    {
        FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
        TArray<FName> Supports; if (Induction) Supports.Add(TEXT("induction_bridge"));
        if (!TestTrue(TEXT("Labelled Coil/Arc gap fixture validates"), FixtureRig(Rig, World,
            {TEXT("extraction_coil"), TEXT("arc_driver")}, Supports, Error))) return false;
        const int32 Conductor = RoleBody(World, TEXT("conductor"));
        const int32 Bridge = RoleBody(World, TEXT("conductor_ballast_a"));
        if (!TestTrue(TEXT("Actual cut frees the long conductor and loose bridge stock"), Pull(World, Rig, Conductor, FVector2D(-280, -160), EExpeditionAction::Extract))) return false;
        MoveMagnet(World, Rig, FVector2D(-40, -160)); World.Drop(); Advance(World, Rig, FVector2D(-40, -160), .1f);
        if (!TestTrue(TEXT("A separate real iron body is recovered for the near side"), Pull(World, Rig, Bridge, World.FindBody(Bridge)->Position + FVector2D(0, -55)))) return false;
        MoveMagnet(World, Rig, FVector2D(-120, -160)); World.Drop(); Advance(World, Rig, FVector2D(-120, -160), .1f);
        FExpeditionCommand Arc; Arc.Action = EExpeditionAction::Arc; Arc.TargetId = RoleBody(World, TEXT("receiver_terminal")); Arc.Aim = World.FindBody(Arc.TargetId)->Position;
        const int32 Before = World.GetBattery(); const auto Result = World.Execute(Arc, Rig);
        TestEqual(TEXT("The clear physical air gap requires Induction"), Result.bSucceeded, Induction);
        TestEqual(TEXT("One valid gap costs four beyond the normal arc"), World.GetBattery(), Before - (Induction ? 12 : 0));
        if (Induction)
        {
            TestTrue(TEXT("The resulting path includes both deliberately placed bodies"), Result.BodyIds.Contains(Bridge) && Result.BodyIds.Contains(Conductor));
            TestTrue(TEXT("The completed physical circuit actuates the real receiver"), World.GetState().bBallastCleared);
        }
        FExpeditionWorld Loaded; TestTrue(TEXT("The committed path and finite cost remain saveable"), Loaded.FromJson(World.ToJson(), Error));
    }
    FExpeditionWorld Wired; Wired.StartSite(0, 103); FExpeditionRig Rig; FString Error;
    if (!TestTrue(TEXT("Labelled wired control retains the same Induction loadout"), FixtureRig(Rig, Wired,
        {TEXT("extraction_coil"), TEXT("arc_driver")}, {TEXT("induction_bridge")}, Error))) return false;
    const int32 Conductor = RoleBody(Wired, TEXT("conductor"));
    if (!TestTrue(TEXT("Actual placement can eliminate the gap entirely"), Pull(Wired, Rig, Conductor, FVector2D(-280, -160), EExpeditionAction::Extract))) return false;
    MoveMagnet(Wired, Rig, FExpeditionWorld::CircuitSocket()); Wired.Drop(); Advance(Wired, Rig, FExpeditionWorld::CircuitSocket(), .1f);
    FExpeditionCommand Arc; Arc.Action = EExpeditionAction::Arc; Arc.TargetId = RoleBody(Wired, TEXT("receiver_terminal")); Arc.Aim = Wired.FindBody(Arc.TargetId)->Position;
    const int32 Before = Wired.GetBattery();
    TestTrue(TEXT("Induction-equipped rig still uses an existing wired path"), Wired.Execute(Arc, Rig).bSucceeded);
    TestEqual(TEXT("Unused Induction does not tax an ordinary wired discharge"), Wired.GetBattery(), Before - 8);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionInvalidSplitRangeTest,
    "MagnetSweep.Expedition.RuntimeSplitRejectsOutOfReachSecondReceiver",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionInvalidSplitRangeTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
    if (!TestTrue(TEXT("Labelled Relay/Split fixture validates"), FixtureRig(*Runtime.Rig, *Runtime.World,
        {TEXT("relay_projector")}, {TEXT("flow_splitter")}, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
    const int32 Source = RoleBody(*Runtime.World, TEXT("rich_scrap"));
    Runtime.Click(63); Runtime.Aim = Runtime.World->FindBody(Source)->Position;
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    Runtime.Aim += FVector2D(0, 95); Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    Runtime.Aim = FVector2D(-470, 270); // Inside tray, but far beyond the source's 440 reach.
    TestFalse(TEXT("The real preview rejects receiver B's range"), Runtime.World->Preview(Runtime.CommandFor(0), *Runtime.Rig).bAllowed);
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Invalid final targeting does not spend the whole split action"), Runtime.World->GetBattery(), 100);
    TestTrue(TEXT("Neither half of the invalid split began moving"), Runtime.World->GetState().RelayIds.IsEmpty() && Runtime.World->GetState().SecondRelayIds.IsEmpty());
    Runtime.Aim = FVector2D(175, -225); Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("The player can correct the endpoint without restarting preparation"), Runtime.World->GetBattery(), 86);
    TestFalse(TEXT("Corrected split commits the actual secondary group"), Runtime.World->GetState().SecondRelayIds.IsEmpty());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionTwinAnchorSwitchTest,
    "MagnetSweep.Expedition.RuntimeTwinAnchorReturnsToActualPaidPlacement",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionTwinAnchorSwitchTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0, 103); FString Error;
    if (!TestTrue(TEXT("Labelled Winch/Twin fixture validates"), FixtureRig(*Runtime.Rig, *Runtime.World,
        {TEXT("anchor_winch")}, {TEXT("twin_anchor")}, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Site; Runtime.bWorldHit = true;
    const int32 First = RoleBody(*Runtime.World, TEXT("brace")), Second = RoleBody(*Runtime.World, TEXT("ballast"));
    const FVector2D A(-150, 230), B(390, 100);
    RuntimeMove(Runtime, A); Runtime.Aim = Runtime.World->FindBody(First)->Position;
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false); RuntimeAdvance(Runtime, 1.f);
    TestTrue(TEXT("First paid tether physically moves its load"), Runtime.World->FindBody(First)->Position.Y > 155);
    RuntimeMove(Runtime, B); Runtime.Aim = Runtime.World->FindBody(Second)->Position;
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Placing the second actual anchor pays another eight"), Runtime.World->GetBattery(), 84);
    TestTrue(TEXT("The first body and its original anchor are retained"), Runtime.World->GetState().SecondTetherBody == First && Runtime.World->GetState().SecondTetherAnchor.Equals(A, .1f));
    Runtime.Click(61); Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("Actual Switch mode restores a previously paid anchor for free"), Runtime.World->GetBattery(), 84);
    TestTrue(TEXT("Switch uses the saved physical placement, not current magnet position"), Runtime.World->GetState().TetherBody == First && Runtime.World->GetState().TetherAnchor.Equals(A, .1f));
    FExpeditionWorld Loaded; TestTrue(TEXT("Both paid placements survive strict save"), Loaded.FromJson(Runtime.World->ToJson(), Error));
    Runtime.Key(EKeys::RightMouseButton, true);
    TestTrue(TEXT("Whole-haul cancellation clears both live anchor actions"), Runtime.World->GetState().TetherBody == INDEX_NONE && Runtime.World->GetState().SecondTetherBody == INDEX_NONE);
    Runtime.Key(EKeys::Q, true); Runtime.Key(EKeys::Q, false);
    TestEqual(TEXT("An invalid switch after cancellation cannot buy a hidden replacement"), Runtime.World->GetBattery(), 84);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionRatchetHoldTest,
    "MagnetSweep.Expedition.RatchetResistsRealDisturbanceWhileWinchWorksElsewhere",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionRatchetHoldTest::RunTest(const FString& Parameters)
{
    float Displacement[2] = {0, 0};
    for (int32 Variant = 0; Variant < 2; ++Variant)
    {
        FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
        TArray<FName> Supports; if (Variant == 1) Supports.Add(TEXT("ratchet_pawl"));
        if (!TestTrue(TEXT("Labelled Winch/Vector ratchet fixture validates"), FixtureRig(Rig, World,
            {TEXT("anchor_winch"), TEXT("vector_emitter")}, Supports, Error))) return false;
        const int32 Held = RoleBody(World, TEXT("brace")); const FVector2D Anchor(-150, 230);
        FExpeditionCommand Tow; Tow.Action = EExpeditionAction::Winch; Tow.TargetId = Held;
        Tow.Aim = World.FindBody(Held)->Position; Tow.Destination = Anchor; Tow.Magnet = Anchor;
        if (!TestTrue(TEXT("A real paid tow reaches the latch position"), World.Execute(Tow, Rig).bSucceeded)) return false;
        Advance(World, Rig, Anchor, 2.5f);
        if (Variant == 1) TestEqual(TEXT("The reached load is the one actually latched"), World.GetState().LatchedBody, Held);
        Tow.TargetId = RoleBody(World, TEXT("ballast")); Tow.Aim = World.FindBody(Tow.TargetId)->Position;
        Tow.Destination = FVector2D(-430, -270); Tow.Magnet = Tow.Destination;
        TestTrue(TEXT("The same winch starts a different distant load"), World.Execute(Tow, Rig).bSucceeded);
        FExpeditionCommand Push; Push.Action = EExpeditionAction::Vector; Push.TargetId = Held;
        Push.Aim = World.FindBody(Held)->Position; Push.Magnet = Push.Aim + FVector2D(100, 0);
        TestTrue(TEXT("A separately paid force actually disturbs the released first load"), World.Execute(Push, Rig).bSucceeded);
        Advance(World, Rig, Push.Magnet, 1.f);
        Displacement[Variant] = FVector2D::Distance(World.FindBody(Held)->Position, Anchor);
        AddInfo(FString::Printf(TEXT("Ratchet=%d, disturbed first-load distance=%.1f"), Variant, Displacement[Variant]));
        TestEqual(TEXT("Ratchet pays two extra for each real tow, with no refund loop"), World.GetBattery(), Variant ? 74 : 78);
        FExpeditionWorld Loaded; TestTrue(TEXT("Actual reached latch and new tow survive save"), Loaded.FromJson(World.ToJson(), Error));
    }
    TestTrue(TEXT("The support holds materially closer than the otherwise identical unlatched body"), Displacement[1] + 35 < Displacement[0] && Displacement[1] < 30);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionReboundMomentumTest,
    "MagnetSweep.Expedition.ReboundRetainsActualReflectedMomentumOnce",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionReboundMomentumTest::RunTest(const FString& Parameters)
{
    float Reflected[2] = {0, 0};
    for (int32 Variant = 0; Variant < 2; ++Variant)
    {
        FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
        TArray<FName> Supports; if (Variant) Supports.Add(TEXT("rebound_plate"));
        if (!TestTrue(TEXT("Labelled Rail reflection fixture validates"), FixtureRig(Rig, World, {TEXT("rail_impeller")}, Supports, Error))) return false;
        int32 Shot = INDEX_NONE;
        if (!TestTrue(TEXT("Real collected iron is fired at the actual tray wall"), FireActualBrace(World, Rig, FVector2D(450, -270), FVector2D(650, -270), Shot))) return false;
        const float Incoming = World.FindBody(Shot)->Velocity.Size();
        for (int32 Frame = 0; Frame < 120 && World.FindBody(Shot)->Velocity.X > 0; ++Frame) World.Tick(1.f / 240.f, FVector2D(450, -270), Rig);
        const auto* Body = World.FindBody(Shot); Reflected[Variant] = -Body->Velocity.X;
        TestTrue(TEXT("The actual collision reverses the projectile"), Body->Velocity.X < 0 && Body->Position.X > 460);
        TestTrue(TEXT("Reflection never creates more speed than arrived"), Body->Velocity.Size() <= Incoming + .1f);
        TestEqual(TEXT("The actual first reflection consumes the fitted plate's one-shot state"), Body->bRebounded, Variant != 0);
        TestEqual(TEXT("Reflection cannot create or refund battery"), World.GetBattery(), 86);
        FExpeditionWorld Loaded; TestTrue(TEXT("Spent reflection and moving source survive save"), Loaded.FromJson(World.ToJson(), Error));
    }
    TestTrue(TEXT("Rebound retains substantially more real return momentum than an ordinary wall impact"), Reflected[1] > Reflected[0] + 200);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionImpactFuseTest,
    "MagnetSweep.Expedition.ImpactFuseProducesOneDelayedPhysicalSecondaryImpulse",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionImpactFuseTest::RunTest(const FString& Parameters)
{
    FVector2D WitnessPosition[2];
    for (int32 Variant = 0; Variant < 2; ++Variant)
    {
        FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
        TArray<FName> Supports = {TEXT("rebound_plate")}; if (Variant) Supports.Add(TEXT("impact_fuse"));
        if (!TestTrue(TEXT("Labelled Rail/Fuse comparison validates"), FixtureRig(Rig, World, {TEXT("rail_impeller")}, Supports, Error))) return false;
        int32 Witness = INDEX_NONE;
        for (const auto& B : World.GetBodies()) if (B.Role == TEXT("rich_scrap") && B.Position.Equals(FVector2D(315, -222), .1f)) Witness = B.Id;
        if (!TestTrue(TEXT("A real nearby unlinked salvage body witnesses the secondary force"), Witness != INDEX_NONE)) return false;
        int32 Shot = INDEX_NONE;
        if (!TestTrue(TEXT("Actual iron capture and wall shot prepare the impact"), FireActualBrace(World, Rig, FVector2D(450, -270), FVector2D(650, -270), Shot))) return false;
        World.DrainEvents(); int32 Bursts = 0; float FirstImpactAt = -1, BurstAt = -1;
        for (int32 Frame = 0; Frame < 360; ++Frame)
        {
            World.Tick(1.f / 240.f, FVector2D(450, -270), Rig);
            for (const auto& Event : World.DrainEvents())
            {
                if (Event.BodyIds.Contains(Shot) && Event.Kind == EExpeditionEventKind::Impact && FirstImpactAt < 0) FirstImpactAt = World.GetState().WorldTime;
                if (Event.Message.Contains(TEXT("Delayed impact fuse"))) { ++Bursts; BurstAt = World.GetState().WorldTime; }
            }
        }
        WitnessPosition[Variant] = World.FindBody(Witness)->Position;
        TestEqual(TEXT("Only a fitted fuse generates exactly one delayed burst"), Bursts, Variant);
        if (Variant) TestTrue(TEXT("The burst follows the actual impact after its meaningful delay"), BurstAt - FirstImpactAt > .28f);
        TestEqual(TEXT("Impact triggers do not buy extra tool operations"), World.GetBattery(), 86);
        TestTrue(TEXT("Secondary force conserves original material identity"), World.CheckInvariants(Error));
    }
    AddInfo(FString::Printf(TEXT("Fuse witness displacement between matched shots: %.1f"), FVector2D::Distance(WitnessPosition[0], WitnessPosition[1])));
    TestTrue(TEXT("The delayed burst changes a real neighboring body's motion"), FVector2D::Distance(WitnessPosition[0], WitnessPosition[1]) > 15);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionPunchThroughTest,
    "MagnetSweep.Expedition.PunchThroughPreservesProjectileTravelBeyondFracturedBrace",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionPunchThroughTest::RunTest(const FString& Parameters)
{
    float SpeedAfterHit[2] = {0, 0}, FurthestX[2] = {0, 0};
    for (int32 Variant = 0; Variant < 2; ++Variant)
    {
        FExpeditionWorld World; World.StartSite(0, 103); FExpeditionRig Rig; FString Error;
        TArray<FName> Supports; if (Variant) Supports.Add(TEXT("punch_through_collar"));
        if (!TestTrue(TEXT("Labelled Rail/Punch comparison validates"), FixtureRig(Rig, World, {TEXT("rail_impeller")}, Supports, Error))) return false;
        int32 Shot = INDEX_NONE; const int32 Target = RoleBody(World, TEXT("brittle_brace"));
        if (!TestTrue(TEXT("Actual iron is launched into the real brittle fitting"), FireActualBrace(World, Rig, FVector2D(70, 0), FVector2D(300, 0), Shot))) return false;
        for (int32 Frame = 0; Frame < 180 && World.FindBody(Target)->bAnchored; ++Frame) World.Tick(1.f / 240.f, FVector2D(70, 0), Rig);
        if (!TestTrue(TEXT("The actual collision fractures the fitting"), !World.FindBody(Target)->bAnchored)) return false;
        SpeedAfterHit[Variant] = World.FindBody(Shot)->Velocity.X; FurthestX[Variant] = World.FindBody(Shot)->Position.X;
        FExpeditionWorld Loaded; TestTrue(TEXT("The first-hit projectile state saves before continued travel"), Loaded.FromJson(World.ToJson(), Error));
        for (int32 Frame = 0; Frame < 60; ++Frame)
        {
            World.Tick(1.f / 240.f, FVector2D(70, 0), Rig);
            Loaded.Tick(1.f / 240.f, FVector2D(70, 0), Rig);
            FurthestX[Variant] = FMath::Max(FurthestX[Variant], float(World.FindBody(Shot)->Position.X));
        }
        TestTrue(TEXT("Reloaded penetration preserves the same actual continued trajectory"),
            Loaded.FindBody(Shot)->Position.Equals(World.FindBody(Shot)->Position, .1f) && Loaded.FindBody(Shot)->Velocity.Equals(World.FindBody(Shot)->Velocity, .1f));
        TestEqual(TEXT("Punch pays its three-energy cost only as part of the actual shot"), World.GetBattery(), Variant ? 83 : 86);
        TestTrue(TEXT("The fractured body and projectile remain conserved material"), World.CheckInvariants(Error));
    }
    AddInfo(FString::Printf(TEXT("Punch comparison: forward speed %.1f/%.1f, furthest x %.1f/%.1f"), SpeedAfterHit[0], SpeedAfterHit[1], FurthestX[0], FurthestX[1]));
    TestTrue(TEXT("Punch preserves materially greater actual forward momentum"), SpeedAfterHit[1] > SpeedAfterHit[0] + 60);
    TestTrue(TEXT("The paid projectile travels farther beyond the fractured fitting"), FurthestX[1] > FurthestX[0] + 20);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionFrozenLegacyLayoutTest,
    "MagnetSweep.Expedition.LegacyLayoutSurvivesDepotDeparturePaidSaveAndRetry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionFrozenLegacyLayoutTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Earned(nullptr); FString Error;
    if (!TestTrue(TEXT("A real starter departs for the compatibility history"), StartRuntime(Earned, 233))) return false;
    for (int32 Site = 0; Site < 2; ++Site)
    {
        const bool Recovered = RuntimeRecoverCore(Earned, Error);
        if (!TestTrue(*FString::Printf(TEXT("Actual earlier core route earns old depot history: %s"), *Error), Recovered)) return false;
        Earned.Key(EKeys::E, true);
        if (Site == 0) Earned.Depart();
    }
    if (!TestTrue(TEXT("Earned history really reached depot index two"), Earned.SiteIndex == 2 && Earned.Screen == EExpeditionScreen::Depot)) return false;
    // Recreate the documented old authored board explicitly. This is a layout
    // compatibility fixture, not a claim that a current new run selected it.
    if (!TestTrue(TEXT("Frozen e1 remains an explicit supported authored definition"),
        Earned.World->StartSite(2, Earned.Rig->GetSeed() + 2, 100, TEXT("e1"), 1))) return false;
    Earned.Rig->SetShopContext(Earned.World->GetOpportunityTags(), Implemented());
    auto OldDepot = ParseJson(Earned.EncodeSave()); EncodeLegacyWorldV2(OldDepot->GetObjectField(TEXT("world")));
    const int32 EarnedCash = Earned.Rig->GetCash();
    FExpeditionRuntime Loaded(nullptr);
    const bool DepotLoaded = Loaded.DecodeSave(JsonText(OldDepot), Error);
    if (!TestTrue(*FString::Printf(TEXT("An earned legacy-schema2 depot loads: %s"), *Error), DepotLoaded)) return false;
    TestTrue(TEXT("Missing old layout metadata maps to frozen e1 revision one"), Loaded.World->GetState().LayoutId == TEXT("e1") && Loaded.World->GetState().LayoutRevision == 1);
    TestEqual(TEXT("Legacy format migration cannot mint or delete earned cash"), Loaded.Rig->GetCash(), EarnedCash);
    Loaded.Depart();
    if (!TestTrue(TEXT("Actual departure preserves that saved worksite instead of choosing a fresh layout"), Loaded.Screen == EExpeditionScreen::Site && Loaded.World->GetState().LayoutId == TEXT("e1"))) return false;
    TestTrue(TEXT("The retry checkpoint contains the same frozen authored identity"),
        Loaded.SiteEntry->GetObjectField(TEXT("world"))->GetStringField(TEXT("LayoutId")) == TEXT("e1"));
    const int32 Body = LooseBody(*Loaded.World);
    Loaded.Magnet = Loaded.World->FindBody(Body)->Position + FVector2D(0, -75);
    FExpeditionCommand PullCommand; PullCommand.Magnet = Loaded.Magnet; PullCommand.Aim = Loaded.World->FindBody(Body)->Position; PullCommand.bPrecision = true;
    if (!TestTrue(TEXT("The restored old site can commit a real paid action"), Loaded.World->Execute(PullCommand, *Loaded.Rig).bSucceeded)) return false;
    Loaded.World->Tick(.05f, Loaded.Magnet, *Loaded.Rig);
    const FVector2D PaidPosition = Loaded.World->FindBody(Body)->Position;
    auto OldActive = ParseJson(Loaded.EncodeSave());
    EncodeLegacyWorldV2(OldActive->GetObjectField(TEXT("world")));
    EncodeLegacyWorldV2(OldActive->GetObjectField(TEXT("site_entry"))->GetObjectField(TEXT("world")));
    FExpeditionRuntime Resumed(nullptr);
    const bool ActiveLoaded = Resumed.DecodeSave(JsonText(OldActive), Error);
    if (!TestTrue(*FString::Printf(TEXT("Legacy active world and its legacy retry checkpoint load together: %s"), *Error), ActiveLoaded)) return false;
    TestTrue(TEXT("Migration preserves the real paid in-flight position"), Resumed.World->FindBody(Body)->Position.Equals(PaidPosition, .001f));
    TestEqual(TEXT("Migration preserves paid battery"), Resumed.World->GetBattery(), 94);
    TestTrue(TEXT("Saved content is explicitly tagged schema3 e1 after migration"),
        Resumed.World->ToJson()->GetIntegerField(TEXT("Version")) == 3 && Resumed.World->GetState().LayoutId == TEXT("e1"));
    Resumed.RetrySite();
    TestTrue(TEXT("Actual retry retains the same frozen site definition"), Resumed.World->GetState().LayoutId == TEXT("e1") && Resumed.World->GetState().LayoutRevision == 1);
    TestEqual(TEXT("Retry rolls back the actual paid operation within that definition"), Resumed.World->GetBattery(), 100);
    TestEqual(TEXT("Retry retains the genuinely earned earlier rewards"), Resumed.Rig->GetCash(), EarnedCash);
    TestTrue(TEXT("Frozen site retry restores available material rather than regenerating another board"), Resumed.World->FindBody(Body)->State == EExpeditionBodyState::Available);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionUnknownLayoutTransactionTest,
    "MagnetSweep.Expedition.UnknownLayoutIdentityOrRevisionRefusesAtomically",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionUnknownLayoutTransactionTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); FString Error;
    if (!TestTrue(TEXT("An actual current site provides a valid live session"), StartRuntime(Runtime, 239))) return false;
    const FString Before = Runtime.EncodeSave();
    for (int32 Variant = 0; Variant < 3; ++Variant)
    {
        auto Broken = ParseJson(Before); const auto World = Broken->GetObjectField(TEXT("world"));
        if (Variant == 0) World->SetStringField(TEXT("LayoutId"), TEXT("qa_unknown_layout"));
        else if (Variant == 1) World->SetNumberField(TEXT("LayoutRevision"), 999);
        else World->RemoveField(TEXT("LayoutId"));
        TestFalse(TEXT("Unknown or missing current layout identity never silently regenerates content"), Runtime.DecodeSave(JsonText(Broken), Error));
        TestTrue(TEXT("Rejected composite restore leaves earned rig/world/checkpoint unchanged"), Runtime.EncodeSave() == Before);
    }
    auto BrokenEntry = ParseJson(Before);
    BrokenEntry->GetObjectField(TEXT("site_entry"))->GetObjectField(TEXT("world"))->SetNumberField(TEXT("LayoutRevision"), 999);
    TestFalse(TEXT("An unavailable retry definition invalidates the whole restore"), Runtime.DecodeSave(JsonText(BrokenEntry), Error));
    TestTrue(TEXT("Invalid retry metadata never replaces the valid session"), Runtime.EncodeSave() == Before);
    const FString WorldBefore = JsonText(Runtime.World->ToJson());
    TestFalse(TEXT("Explicit unknown new-site layout refuses before mutation"), Runtime.World->StartSite(2, 999, 100, TEXT("qa_unknown_layout"), 1));
    TestTrue(TEXT("Failed site construction preserves actual world content"), JsonText(Runtime.World->ToJson()) == WorldBefore);
    TestFalse(TEXT("Known layout with unknown revision also refuses before mutation"), Runtime.World->StartSite(2, 999, 100, TEXT("e1"), 999));
    TestTrue(TEXT("Failed revision lookup preserves the previous world"), JsonText(Runtime.World->ToJson()) == WorldBefore);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionBalancedMassTest,
    "MagnetSweep.Expedition.BalancedRackUsesRestingMassAndLatchesOnlyAfterCapture",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionBalancedMassTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(2, 251); FString Error;
    if (!TestTrue(TEXT("Labelled isolated rack fixture has ordinary Coil equipment"), FixtureRig(*Runtime.Rig, *Runtime.World, {TEXT("extraction_coil")}, {}, Error))) return false;
    Runtime.Screen = EExpeditionScreen::Site; Runtime.SiteIndex = 2; Runtime.bWorldHit = true;
    const FVector2D Left = Runtime.World->FindMarker(TEXT("balance_left"))->Position;
    const FVector2D Right = Runtime.World->FindMarker(TEXT("balance_right"))->Position;
    const int32 Ballast = RoleBody(*Runtime.World, TEXT("ballast")), Brace = RoleBody(*Runtime.World, TEXT("brace"));
    const int32 Core = RoleBody(*Runtime.World, TEXT("core"));
    if (!TestTrue(TEXT("Actual twelve-kilogram ballast is captured"), RuntimePickBody(Runtime, Ballast, {0,25}, Error))) return false;
    RuntimePlaceHaul(Runtime, Left);
    if (!TestTrue(TEXT("Actual eight-kilogram brace is captured"), RuntimePickBody(Runtime, Brace, {0,25}, Error))) return false;
    RuntimePlaceHaul(Runtime, Right);
    TestFalse(TEXT("Twelve against eight is insufficient despite two occupied platforms"), Runtime.World->IsCoreReleased());
    if (!TestTrue(TEXT("One actual two-kilogram trim weight is captured"), RuntimePickBody(Runtime, 12, {0,-25}, Error))) return false;
    RuntimePlaceHaul(Runtime, Right + FVector2D(-45,0));
    TestTrue(TEXT("Twelve against ten is an actual valid tolerant balance"), Runtime.World->IsCoreReleased());
    TestFalse(TEXT("Merely releasing the rack has not secured the core"), Runtime.World->GetState().bCoreSecured);
    if (!TestTrue(TEXT("The player can reclaim the left weight before taking the core"), RuntimePickBody(Runtime, Ballast, {0,-25}, Error))) return false;
    TestFalse(TEXT("Held weight no longer counts toward the platform condition"), Runtime.World->IsCoreReleased());
    TestTrue(TEXT("The untouched core physically reanchors when support is removed"), Runtime.World->FindBody(Core)->bAnchored);
    RuntimePlaceHaul(Runtime, Left);
    if (!TestTrue(TEXT("Replacing the real mass restores the physical release"), Runtime.World->IsCoreReleased())) return false;
    if (!TestTrue(TEXT("Actual useful core capture completes the support operation"), RuntimePickBody(Runtime, Core, {0,25}, Error))) return false;
    TestTrue(TEXT("Only actual Cargo arrival latches the secured objective"), Runtime.World->GetState().bCoreSecured);
    RuntimePlaceHaul(Runtime, {430,-260});
    if (!TestTrue(TEXT("The player reclaims a platform weight after staging the secured core"), RuntimePickBody(Runtime, Ballast, {0,-25}, Error))) return false;
    TestTrue(TEXT("Already secured material stays recoverable after the weights are removed"), Runtime.World->IsCoreReleased() && !Runtime.World->FindBody(Core)->bAnchored);
    FExpeditionWorld Resumed;
    TestTrue(*FString::Printf(TEXT("Actual secured-stage state restores: %s"), *Error), Resumed.FromJson(Runtime.World->ToJson(), Error));
    TestTrue(TEXT("Restored core remains available and its latch is preserved"), Resumed.GetState().bCoreSecured && Resumed.FindBody(Core)->State == EExpeditionBodyState::Available);
    TestFalse(TEXT("A staged core is not a delivered core"), Resumed.Dispatch().bSucceeded);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionRemoteFinalCounterweightTest,
    "MagnetSweep.Expedition.FinalRemoteToolsMoveRealReplacementWhileCoreStaysHeld",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionRemoteFinalCounterweightTest::RunTest(const FString& Parameters)
{
    // Controlled equipment comparisons, not claims about shop acquisition.
    // Variant two uses ordinary twelve-plus-eight mass, not the cover's ID.
    for (int32 Variant = 0; Variant < 3; ++Variant)
    {
        FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(3, 257); FString Error;
        const FName Tool = Variant == 1 ? FName(TEXT("relay_projector")) : FName(TEXT("anchor_winch"));
        if (!TestTrue(TEXT("Labelled final remote-tool fixture validates"), FixtureRig(*Runtime.Rig, *Runtime.World, {Tool}, {}, Error))) return false;
        Runtime.Screen = EExpeditionScreen::Site; Runtime.SiteIndex = 3; Runtime.bWorldHit = true;
        const FVector2D Park = Runtime.World->FindMarker(TEXT("cover"))->Position;
        const FVector2D Seat = Runtime.World->FindMarker(TEXT("counterbalance"))->Position;
        const int32 Cover = RoleBody(*Runtime.World, TEXT("counterweight_cover")), Core = RoleBody(*Runtime.World, TEXT("core"));
        if (!TestTrue(TEXT("Actual baseline pull moves the twenty-kilogram cover"), RuntimePickBody(Runtime, Cover, {0,25}, Error))) return false;
        RuntimePlaceHaul(Runtime, Park);
        if (!TestTrue(TEXT("The physically exposed twenty-kilogram core is really secured"), RuntimePickBody(Runtime, Core, {0,25}, Error))) return false;
        const int32 Before = Runtime.World->GetBattery();
        RuntimeMove(Runtime, {Runtime.Magnet.X,-260}); RuntimeMove(Runtime, Park + FVector2D(0,-25));
        Runtime.Key(EKeys::LeftShift,true); Runtime.Key(EKeys::LeftMouseButton,true); RuntimeAdvance(Runtime,.1f);
        Runtime.Key(EKeys::LeftMouseButton,false); Runtime.Key(EKeys::LeftShift,false);
        TestEqual(TEXT("Ordinary field cannot pay for an impossible forty-kilogram double haul"), Runtime.World->GetBattery(), Before);
        TestTrue(TEXT("Refused double pickup leaves the cover available and the core safely held"),
            Runtime.World->FindBody(Cover)->State == EExpeditionBodyState::Available && Runtime.World->FindBody(Core)->State == EExpeditionBodyState::Cargo && !Runtime.World->IsUnsafe());
        RuntimeMove(Runtime, {Runtime.Magnet.X,-260}); RuntimeMove(Runtime, FExpeditionWorld::ReceiverPosition());
        TestFalse(TEXT("Physical core arrival alone cannot bypass the counterbalance"), Runtime.World->Dispatch().bSucceeded);

        // Place the far/right mass first, so the second incoming load does not
        // have to travel through the first resting replacement.
        const TArray<int32> Weights = Variant == 2 ? TArray<int32>{RoleBody(*Runtime.World,TEXT("brace")),RoleBody(*Runtime.World,TEXT("ballast"))} : TArray<int32>{Cover};
        for (int32 I=0; I<Weights.Num(); ++I)
        {
            const FVector2D Destination = Seat + (Variant == 2 ? FVector2D(I == 0 ? 23 : -23,0) : FVector2D::ZeroVector);
            RuntimeMove(Runtime,{Runtime.Magnet.X,-260}); RuntimeMove(Runtime,{Destination.X,-260}); RuntimeMove(Runtime,Destination);
            Runtime.Aim = Runtime.World->FindBody(Weights[I])->Position;
            Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
            if (Variant == 1)
            {
                TestEqual(TEXT("Selecting a Relay source is free"), Runtime.World->GetBattery(), Before);
                Runtime.Aim = Destination; Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
            }
            Runtime.Aim = Destination; RuntimeAdvance(Runtime,5.f);
            const auto* Weight = Runtime.World->FindBody(Weights[I]);
            AddInfo(FString::Printf(TEXT("Remote variant %d weight %d reached (%.1f,%.1f), speed %.1f"),Variant,Weights[I],Weight->Position.X,Weight->Position.Y,Weight->Velocity.Size()));
            TestTrue(TEXT("The paid tool physically moves and settles its available replacement"),
                Weight->State == EExpeditionBodyState::Available && FVector2D::Distance(Weight->Position,Seat)<=28 && Weight->Velocity.Size()<45);
            TestTrue(TEXT("The core never requires staging during remote transport"), Runtime.World->FindBody(Core)->State == EExpeditionBodyState::Cargo && FMath::IsNearlyEqual(Runtime.World->GetCargoMass(),20.f));
        }
        const int32 Cost = Variant == 1 ? 10 : Variant == 2 ? 16 : 8;
        TestEqual(TEXT("Only the actually committed remote operations spend energy"), Runtime.World->GetBattery(), Before-Cost);
        RuntimeMove(Runtime,{Runtime.Magnet.X,-260}); RuntimeMove(Runtime,FExpeditionWorld::ReceiverPosition());
        for (int32 Id:Weights)
        {
            const auto* B=Runtime.World->FindBody(Id);
            AddInfo(FString::Printf(TEXT("At receiver variant %d weight %d: (%.2f,%.2f), speed %.2f, state %d, mass %.0f"),Variant,Id,B->Position.X,B->Position.Y,B->Velocity.Size(),int32(B->State),B->Mass));
        }
        TestTrue(TEXT("The empty cradle now has actual twenty-kilogram available support"), Runtime.World->GetState().bBallastCleared);
        if (Variant == 2) TestTrue(TEXT("Generic mixed replacement does not require the cover on its old seat"), FVector2D::Distance(Runtime.World->FindBody(Cover)->Position,Seat)>130);
        TestTrue(TEXT("Real receiver delivery succeeds after actual support movement"), Runtime.World->Dispatch().bSucceeded);
        TestFalse(TEXT("Completed dispatch cannot pay or complete twice"), Runtime.World->Dispatch().bSucceeded);
        TestTrue(TEXT("Remote route preserves source material invariants"), Runtime.World->CheckInvariants(Error));
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionRackMechanicalRefiningTest,
    "MagnetSweep.Expedition.RackWinchAndRailEarnDistinctPhysicalRecoveries",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionRackMechanicalRefiningTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); FString Error;
    const bool Ready=ReachRackWithFixtureEquipment(Runtime,{TEXT("anchor_winch"),TEXT("rail_impeller")},{TEXT("counterweight_hook")},Error);
    if (!TestTrue(*FString::Printf(TEXT("Found-equipment fixture earns preceding sites and pays for battery: %s"),*Error),Ready)) return false;
    const int32 Cash=Runtime.Rig->GetCash();
    const int32 Weight=RoleBody(*Runtime.World,TEXT("brace")), Machine=RoleBody(*Runtime.World,TEXT("supported_machine"));
    const int32 Brittle=RoleBody(*Runtime.World,TEXT("brittle_brace")), Ammo=RoleBody(*Runtime.World,TEXT("ballast"));
    if (!TestTrue(TEXT("Actual eight-kilogram brace becomes the mechanical support"),RuntimePickBody(Runtime,Weight,{0,25},Error))) return false;
    RuntimePlaceHaul(Runtime,Runtime.World->FindMarker(TEXT("counterweight"))->Position);
    RuntimeMove(Runtime,{-260,-130});
    const int32 Before=Runtime.World->GetBattery();
    Runtime.Aim=Runtime.World->FindBody(Brittle)->Position; Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
    TestEqual(TEXT("A prepared counterweight cannot pay to bypass an unrelated bolted brace"),Runtime.World->GetBattery(),Before);
    TestTrue(TEXT("The unsupported brittle mounting remains physically anchored"),Runtime.World->FindBody(Brittle)->bAnchored);
    Runtime.Aim=Runtime.World->FindBody(Machine)->Position; Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
    Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,2.f);
    TestEqual(TEXT("The same support enables the genuine machine tow for eight energy"),Runtime.World->GetBattery(),Before-8);
    if (!TestTrue(TEXT("The supported machine actually moved away from its mount"),!Runtime.World->FindBody(Machine)->bAnchored && Runtime.World->FindBody(Machine)->Position.X < -215)) return false;
    if (!TestTrue(TEXT("The mechanically released machine is physically captured"),RuntimePickBody(Runtime,Machine,{0,-25},Error))) return false;
    RuntimeSmelt(Runtime);
    TestEqual(TEXT("Actual first machine refinement banks its intact appraisal"),Runtime.World->GetOutput(),220);
    TestEqual(TEXT("Sub-threshold recovery does not prematurely pay a milestone"),Runtime.Rig->GetCash(),Cash);

    if (!TestTrue(TEXT("Real twelve-kilogram ballast supplies the separate impact tool"),RuntimePickBody(Runtime,Ammo,{0,25},Error))) return false;
    RuntimeMove(Runtime,{Runtime.Magnet.X,-280}); RuntimeMove(Runtime,{220,-280}); RuntimeMove(Runtime,{220,-140});
    Runtime.Click(2000+Ammo); Runtime.Aim=Runtime.World->FindBody(Brittle)->Position;
    Runtime.Key(EKeys::F,true); Runtime.Key(EKeys::F,false); Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,3.f);
    if (!TestTrue(TEXT("A real selected Rail collision releases the different bolted target"),!Runtime.World->FindBody(Brittle)->bAnchored)) return false;
    const FVector2D Away=(Runtime.World->FindBody(Brittle)->Position-Runtime.World->FindBody(Ammo)->Position).GetSafeNormal()*30.f;
    if (!TestTrue(TEXT("The fractured iron is then actually recovered"),RuntimePickBody(Runtime,Brittle,Away,Error))) return false;
    RuntimeSmelt(Runtime);
    TestEqual(TEXT("Two different physical mechanisms bank two intact iron/machine appraisals"),Runtime.World->GetOutput(),440);
    TestEqual(TEXT("Only that real pour pays the first rack milestone"),Runtime.Rig->GetCash(),Cash+2);
    for (int32 Id=36; Id<=40; ++Id)
        if (!TestTrue(TEXT("Actual ordinary top-row alloy supplements the specialized recovery"),RuntimePickBody(Runtime,Id,{0,-25},Error))) return false;
    RuntimeSmelt(Runtime);
    TestEqual(TEXT("The actually banked mechanical route reaches 480"),Runtime.World->GetOutput(),480);
    TestEqual(TEXT("Physical output pays both milestones once"),Runtime.Rig->GetCash(),Cash+4);
    Runtime.Key(EKeys::E,true); TestEqual(TEXT("Empty repeated settlement never duplicates those earnings"),Runtime.Rig->GetCash(),Cash+4);
    FExpeditionRuntime Resumed(nullptr);
    TestTrue(*FString::Printf(TEXT("Real earned mechanical outcome restores: %s"),*Error),Resumed.DecodeSave(Runtime.EncodeSave(),Error));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionRackGeneratorRefiningTest,
    "MagnetSweep.Expedition.PreservedGeneratorPowersTwoIsolatedRecoveriesAndRealRewards",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionRackGeneratorRefiningTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); FString Error;
    const bool Ready=ReachRackWithFixtureEquipment(Runtime,{TEXT("extraction_coil"),TEXT("arc_driver")},{TEXT("intact_recovery")},Error);
    if (!TestTrue(*FString::Printf(TEXT("Labelled found Coil/Arc/Intact rig earns the actual rack: %s"),*Error),Ready)) return false;
    const int32 Cash=Runtime.Rig->GetCash(), Generator=RoleBody(*Runtime.World,TEXT("portable_generator"));
    if (!TestTrue(TEXT("The physical portable source exists"),Generator!=INDEX_NONE)) return false;
    const int32 Before=Runtime.World->GetBattery();
    Runtime.Aim=Runtime.World->FindBody(4)->Position; Runtime.Key(EKeys::F,true); Runtime.Key(EKeys::F,false);
    TestEqual(TEXT("An isolated receiver cannot buy an imaginary internal charge"),Runtime.World->GetBattery(),Before);
    TestEqual(TEXT("A refused disconnected operation leaves both actual stored charges intact"),Runtime.World->FindBody(Generator)->Charge,2);
    RuntimeMove(Runtime,Runtime.World->FindBody(Generator)->Position+FVector2D(0,-25));
    Runtime.Aim=Runtime.World->FindBody(Generator)->Position; Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
    Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,1.4f);
    if (!TestTrue(TEXT("Actual twelve-energy preservation frees a functioning two-charge source"),
        Runtime.World->FindBody(Generator)->State==EExpeditionBodyState::Cargo && Runtime.World->FindBody(Generator)->bFunctional && Runtime.World->FindBody(Generator)->Charge==2)) return false;
    TestEqual(TEXT("Preservation paid its actual extraction plus function cost"),Runtime.World->GetBattery(),Before-12);
    for (int32 I=0; I<2; ++I)
    {
        // The left dock has a loose iron below it. Approach its source from
        // above with the real narrow field so the eventual drop stays singular.
        if (I==1 && !TestTrue(TEXT("The same finite source is physically collected again"),RuntimePickBody(Runtime,Generator,{0,35},Error))) return false;
        if (!TestTrue(TEXT("The actual selected haul contains the eight-kilogram source alone"),FMath::IsNearlyEqual(Runtime.World->GetCargoMass(),8.f))) return false;
        const FVector2D Dock=Runtime.World->FindMarker(I==0?FName(TEXT("circuit")):FName(TEXT("brace_power")))->Position;
        RuntimePlaceHaul(Runtime,Dock);
        const int32 Terminal=I==0?4:11;
        Runtime.Aim=Runtime.World->FindBody(Terminal)->Position;
        const auto Command=Runtime.CommandFor(1); const auto Preview=Runtime.World->Preview(Command,*Runtime.Rig);
        const auto* Source=Runtime.World->FindBody(Generator);
        AddInfo(FString::Printf(TEXT("Arc%d target%d source(%.1f,%.1f) state%d charge%d: %s"),I,Command.TargetId,Source->Position.X,Source->Position.Y,int32(Source->State),Source->Charge,*Preview.Reason));
        Runtime.Key(EKeys::F,true); Runtime.Key(EKeys::F,false);
        TestEqual(TEXT("Each useful isolated operation consumes one real source charge"),Runtime.World->FindBody(Generator)->Charge,1-I);
        TestTrue(TEXT("The selected receiver's actual different payload is released"),!Runtime.World->FindBody(I==0?9:10)->bAnchored);
        const int32 PaidBattery=Runtime.World->GetBattery(); Runtime.Key(EKeys::F,true); Runtime.Key(EKeys::F,false);
        TestEqual(TEXT("An already satisfied receiver cannot consume a second payment"),Runtime.World->GetBattery(),PaidBattery);
    }
    TestEqual(TEXT("Both isolated releases leave the finite source empty"),Runtime.World->FindBody(Generator)->Charge,0);
    TestEqual(TEXT("Unlocking material alone never settles output or credits"),Runtime.World->GetOutput(),0);
    TestEqual(TEXT("No invisible reward arrives before the furnace"),Runtime.Rig->GetCash(),Cash);
    for (int32 Id : {9,10})
    {
        if (!TestTrue(TEXT("Each electrically released prize is physically captured"),RuntimePickBody(Runtime,Id,{0,-25},Error))) return false;
        RuntimeSmelt(Runtime);
    }
    for (int32 Id=36; Id<=40; ++Id)
        if (!TestTrue(TEXT("Five real ordinary alloys supply the visible supplement"),RuntimePickBody(Runtime,Id,{0,-25},Error))) return false;
    RuntimeSmelt(Runtime);
    TestEqual(TEXT("Actual finite-source route banks 480 appraisal"),Runtime.World->GetOutput(),480);
    TestEqual(TEXT("Banking those actual materials earns four spendable credits"),Runtime.Rig->GetCash(),Cash+4);
    const bool Recovered=RuntimeRecoverCore(Runtime,Error);
    if (!TestTrue(*FString::Printf(TEXT("The optional electrical recovery still leaves a real core route: %s"),*Error),Recovered)) return false;
    Runtime.Key(EKeys::E,true);
    TestTrue(TEXT("Actual core delivery reaches the final depot"),Runtime.SiteIndex==3 && Runtime.Screen==EExpeditionScreen::Depot);
    TestEqual(TEXT("Physical rack completion adds its real fourteen-credit award"),Runtime.Rig->GetCash(),Cash+18);
    FExpeditionRuntime Resumed(nullptr);
    if (!TestTrue(*FString::Printf(TEXT("Earned electrical recovery and final depot restore: %s"),*Error),Resumed.DecodeSave(Runtime.EncodeSave(),Error))) return false;
    Resumed.Depart();
    const int32 FinalGenerator=RoleBody(*Resumed.World,TEXT("portable_generator"));
    RuntimeMove(Resumed,Resumed.World->FindBody(FinalGenerator)->Position+FVector2D(0,-25));
    Resumed.Aim=Resumed.World->FindBody(FinalGenerator)->Position; Resumed.Key(EKeys::Q,true); Resumed.Key(EKeys::Q,false);
    Resumed.Aim=Resumed.Magnet; RuntimeAdvance(Resumed,1.4f);
    if (!TestTrue(TEXT("The saved two-tool rig really preserves the final worksite's separate source"),Resumed.World->FindBody(FinalGenerator)->State==EExpeditionBodyState::Cargo && Resumed.World->FindBody(FinalGenerator)->Charge==2)) return false;
    RuntimePlaceHaul(Resumed,Resumed.World->FindMarker(TEXT("circuit"))->Position);
    Resumed.Aim=Resumed.World->FindBody(4)->Position; Resumed.Key(EKeys::F,true); Resumed.Key(EKeys::F,false);
    TestEqual(TEXT("The final isolated latch consumes one actual portable charge"),Resumed.World->FindBody(FinalGenerator)->Charge,1);
    if (!TestTrue(TEXT("The final alternate dispatch condition is really powered"),Resumed.World->GetState().bCircuitClosed)) return false;
    const int32 Cover=RoleBody(*Resumed.World,TEXT("counterweight_cover")), Core=RoleBody(*Resumed.World,TEXT("core"));
    if (!TestTrue(TEXT("Powered route still requires physically moving the cover"),RuntimePickBody(Resumed,Cover,{0,25},Error))) return false;
    RuntimePlaceHaul(Resumed,Resumed.World->FindMarker(TEXT("cover"))->Position);
    if (!TestTrue(TEXT("Powered route still requires actual core capture"),RuntimePickBody(Resumed,Core,{0,25},Error))) return false;
    RuntimeMove(Resumed,{Resumed.Magnet.X,-260}); RuntimeMove(Resumed,FExpeditionWorld::ReceiverPosition());
    TestFalse(TEXT("This electrical route deliberately leaves the physical replacement empty"),Resumed.World->GetState().bBallastCleared);
    Resumed.Key(EKeys::E,true);
    TestTrue(TEXT("Actual final powered delivery completes and archives the same two-tool expedition"),Resumed.Screen==EExpeditionScreen::Victory && Resumed.Rig->IsRunWon());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionLateRelaySourceTest,
    "MagnetSweep.Expedition.LateSensorRelayRequiresAndReservesRealPortableCharge",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionLateRelaySourceTest::RunTest(const FString& Parameters)
{
    for (int32 Site : {2,3})
    {
        FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(Site,269); FString Error;
        if (!TestTrue(TEXT("Labelled late relay equipment fixture validates"),FixtureRig(*Runtime.Rig,*Runtime.World,
            {TEXT("extraction_coil"),TEXT("arc_driver")},{TEXT("intact_recovery"),TEXT("escapement_relay")},Error))) return false;
        Runtime.Screen=EExpeditionScreen::Site; Runtime.SiteIndex=Site; Runtime.bWorldHit=true;
        const int32 Generator=RoleBody(*Runtime.World,TEXT("portable_generator")), Sensor=RoleBody(*Runtime.World,TEXT("brace"));
        Runtime.Click(66); Runtime.Aim=Runtime.World->FindBody(Sensor)->Position; Runtime.Key(EKeys::F,true); Runtime.Key(EKeys::F,false);
        Runtime.Aim=Runtime.World->FindBody(4)->Position; Runtime.Key(EKeys::F,true); Runtime.Key(EKeys::F,false);
        TestEqual(TEXT("Disconnected late relay cannot reserve an internal free charge"),Runtime.World->GetBattery(),100);
        TestEqual(TEXT("Refusal leaves no deferred trigger"),Runtime.World->GetState().DeferredCharge,0);
        TestEqual(TEXT("Refusal leaves the source untouched"),Runtime.World->FindBody(Generator)->Charge,2);
        Runtime.Key(EKeys::RightMouseButton,true);
        RuntimeMove(Runtime,Runtime.World->FindBody(Generator)->Position+FVector2D(0,-25));
        Runtime.Aim=Runtime.World->FindBody(Generator)->Position; Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
        Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,1.4f);
        if (!TestTrue(TEXT("A real intact extraction prepares the finite portable source"),Runtime.World->FindBody(Generator)->State==EExpeditionBodyState::Cargo && Runtime.World->FindBody(Generator)->bFunctional)) return false;
        RuntimePlaceHaul(Runtime,Runtime.World->FindMarker(TEXT("circuit"))->Position);
        Runtime.Click(66); Runtime.Aim=Runtime.World->FindBody(Sensor)->Position; Runtime.Key(EKeys::F,true); Runtime.Key(EKeys::F,false);
        Runtime.Aim=Runtime.World->FindBody(4)->Position; Runtime.Key(EKeys::F,true); Runtime.Key(EKeys::F,false);
        TestEqual(TEXT("A useful deferred operation really reserves one finite source charge"),Runtime.World->FindBody(Generator)->Charge,1);
        TestEqual(TEXT("The real reservation pays eleven once after twelve for source preservation"),Runtime.World->GetBattery(),77);
        TestEqual(TEXT("The charged physical sensor is pending rather than already triggered"),Runtime.World->GetState().DeferredCharge,1);
        TestFalse(TEXT("Paid reservation alone does not actuate the mechanism"),Runtime.World->GetState().PoweredTerminals.Contains(4));
        Runtime.Key(EKeys::RightMouseButton,true);
        TestEqual(TEXT("Dropping cannot return the already reserved source charge"),Runtime.World->FindBody(Generator)->Charge,1);
        FExpeditionWorld Resumed;
        const bool Loaded=Resumed.FromJson(Runtime.World->ToJson(),Error);
        if (!TestTrue(*FString::Printf(TEXT("Pending finite relay restores with real endpoints: %s"),*Error),Loaded)) return false;
        TestEqual(TEXT("Reload preserves paid source consumption"),Resumed.FindBody(Generator)->Charge,1);
        FExpeditionCommand Duplicate; Duplicate.Action=EExpeditionAction::ArmRelay; Duplicate.TargetId=Sensor; Duplicate.SecondaryId=4;
        Duplicate.Magnet=Runtime.Magnet; Duplicate.Aim=Runtime.World->FindBody(4)->Position;
        TestFalse(TEXT("Another trigger cannot reserve over the outstanding paid sensor"),Runtime.World->Execute(Duplicate,*Runtime.Rig).bSucceeded);
        TestEqual(TEXT("Repeated refused reserve cannot double spend energy"),Runtime.World->GetBattery(),77);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionSavedOpportunityRefreshTest,
    "MagnetSweep.Expedition.SavedWorksiteRefreshesExactOpportunitiesWithoutReroll",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionSavedOpportunityRefreshTest::RunTest(const FString& Parameters)
{
    for (int32 Site : {2,3})
    {
        // Frozen revision one has isolated receivers. New frame revisions
        // intentionally add a useful return circuit and must not be excluded.
        FExpeditionWorld World; World.StartSite(Site,277,100,Site==2?FName(TEXT("balanced_rack")):FName(TEXT("counterweight_exchange")),1); FExpeditionRig Rig;
        Configure(Rig,World,TEXT("arc_driver"),277);
        TestFalse(TEXT("An isolated late receiver is not an authored closed return circuit"),Rig.HasOpportunity(TEXT("closed_circuit")));
        TestFalse(TEXT("Actual late generated stock excludes unsupported Closed Circuit"),Rig.GetOffers().Contains(TEXT("closed_circuit")));
    }
    FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(0,281);
    Configure(*Runtime.Rig,*Runtime.World,TEXT("arc_driver"),281); Runtime.Screen=EExpeditionScreen::Depot;
    const TArray<FName> Stock=Runtime.Rig->GetOffers(); const int32 Cash=Runtime.Rig->GetCash();
    auto Legacy=ParseJson(Runtime.EncodeSave()); EncodeLegacyWorldV2(Legacy->GetObjectField(TEXT("world")));
    const auto RigJson=Legacy->GetObjectField(TEXT("rig")); TArray<TSharedPtr<FJsonValue>> OldTags;
    for (const auto& Value:RigJson->GetArrayField(TEXT("opportunities")))
        if (Value->AsString()!=TEXT("ClosedReturn")) OldTags.Add(Value);
    RigJson->SetArrayField(TEXT("opportunities"),OldTags);
    FExpeditionRuntime Resumed(nullptr); FString Error;
    const bool Loaded=Resumed.DecodeSave(JsonText(Legacy),Error);
    if (!TestTrue(*FString::Printf(TEXT("Actual old E1 depot context remains compatible: %s"),*Error),Loaded)) return false;
    TestTrue(TEXT("Restoring the exact old board refreshes its now-explicit closed-return opportunity"),Resumed.Rig->HasOpportunity(TEXT("closed_circuit")));
    TestTrue(TEXT("Compatibility refresh never rerolls the player's saved stock"),Resumed.Rig->GetOffers()==Stock);
    TestEqual(TEXT("A metadata refresh cannot create money"),Resumed.Rig->GetCash(),Cash);
    TestTrue(TEXT("The old depot is still frozen E1"),Resumed.World->GetState().LayoutId==TEXT("e1"));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionEarnedGantryFrameTest,
    "MagnetSweep.Expedition.EarnedGantryPurchaseRecoversOversizedFramesAcrossLateSites",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionEarnedGantryFrameTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); FString Error;
    const bool Earned=EarnFirstSiteKeystoneBudget(Runtime,TEXT("anchor_winch"),1,Error);
    if (!TestTrue(*FString::Printf(TEXT("Real first-site recovery earns the keystone budget: %s"),*Error),Earned)) return false;
    TestEqual(TEXT("Partial first-site recovery and its real clear leave exactly twenty-four credits"),Runtime.Rig->GetCash(),24);
    if (!TestTrue(TEXT("The actual generated depot offers Walking Gantry"),Runtime.Rig->GetOffers().Contains(TEXT("walking_gantry")))) return false;
    if (!TestTrue(TEXT("The real fourteen-credit specialist purchase succeeds"),Runtime.Rig->Buy(TEXT("walking_gantry"),Error))) return false;
    TestEqual(TEXT("Buying the repriced specialist leaves ten genuinely earned credits"),Runtime.Rig->GetCash(),10);
    if (!TestTrue(TEXT("The player explicitly fits the purchased two-socket support"),Runtime.Rig->Fit(TEXT("walking_gantry"),Error))) return false;
    TestEqual(TEXT("The actual purchase retains its fourteen-credit receipt"),Runtime.Rig->GetResaleValue(TEXT("walking_gantry")),7);
    TestTrue(TEXT("The generated ten-credit Arc remains a real affordable second-tool choice"),Runtime.Rig->GetOffers().Contains(TEXT("arc_driver")) && Runtime.Rig->CanBuy(TEXT("arc_driver"),Error));
    FExpeditionRuntime Restored(nullptr);
    if (!TestTrue(TEXT("The actual paid purchase and generated depot restore"),Restored.DecodeSave(Runtime.EncodeSave(),Error))) return false;
    Restored.Depart();
    if (!TestTrue(TEXT("The paid rig completes the actual second core route"),RuntimeRecoverCore(Restored,Error))) return false;
    Restored.Key(EKeys::E,true);
    if (!TestTrue(TEXT("Real second recovery reaches the first large-frame depot"),Restored.SiteIndex==2 && Restored.Screen==EExpeditionScreen::Depot)) return false;

    for (int32 Site=2; Site<=3; ++Site)
    {
        Restored.Depart();
        const auto* Frame=Restored.World->FindFrameBody();
        if (!TestTrue(TEXT("New default late layout exposes an actual forty-kilogram source"),Frame && Frame->Mass>FExpeditionWorld::HardCapacity && Restored.World->GetState().LayoutRevision==2)) return false;
        const int32 FrameId=Frame->Id, BeforeCash=Restored.Rig->GetCash();
        const bool Arrived=RuntimeMoveFrameWithGantry(Restored,Error,Site==3);
        if (!TestTrue(*FString::Printf(TEXT("Paid Gantry moves both actual receiving loads on site %d: %s"),Site,*Error),Arrived)) return false;
        const auto* ArrivedFrame=Restored.World->FindBody(FrameId);
        AddInfo(FString::Printf(TEXT("Earned Gantry site%d frame(%.2f,%.2f), speed%.2f, appraisal%d, battery%d"),Site,ArrivedFrame->Position.X,ArrivedFrame->Position.Y,ArrivedFrame->Velocity.Size(),ArrivedFrame->Appraisal,Restored.World->GetBattery()));
        const float RetainedMass=Site==3?20.f:0.f;
        TestTrue(TEXT("Oversized recovery remains available while the real chosen haul stays held"),ArrivedFrame->State==EExpeditionBodyState::Available && FMath::IsNearlyEqual(Restored.World->GetCargoMass(),RetainedMass));
        if (Site==3)
        {
            // Branch an actual earned state; do not manufacture cargo or gear.
            // The ordinary regrip is a counterfactual control comparison, not
            // a claim that the cheaper rig was equipped in this paid run.
            FExpeditionRuntime Regrip(nullptr);
            if (!TestTrue(TEXT("The actual paid final-core state can be cloned in memory for comparison"),Regrip.DecodeSave(Restored.EncodeSave(),Error))) return false;
            TestTrue(TEXT("An active saved operation restores intentionally paused"),Regrip.bPaused);
            Regrip.Key(EKeys::Escape,true);
            const int32 SupportBody=RoleBody(*Regrip.World,TEXT("brace"));
            if (!TestTrue(TEXT("An ordinary regrip really adds the actual eight-kilogram support"),RuntimePickBody(Regrip,SupportBody,{0,25},Error))) return false;
            TestTrue(TEXT("That regrip with the actual final core creates twenty-eight kilograms and a real unsafe haul"),FMath::IsNearlyEqual(Regrip.World->GetCargoMass(),28.f) && Regrip.World->IsUnsafe());
            Regrip.Key(EKeys::RightMouseButton,true);
            TestTrue(TEXT("The player's real whole-haul rescue clears that danger without fabricating a loss"),!Regrip.World->IsUnsafe() && FMath::IsNearlyZero(Regrip.World->GetCargoMass()));
        }
        TestEqual(TEXT("Entering the dock alone cannot pay output"),Restored.World->GetOutput(),0);
        const int32 BeforeGrip=Restored.World->GetBattery();
        RuntimeMove(Restored,ArrivedFrame->Position);
        Restored.Key(EKeys::LeftShift,true); Restored.Key(EKeys::LeftMouseButton,true); RuntimeAdvance(Restored,.1f);
        Restored.Key(EKeys::LeftMouseButton,false); Restored.Key(EKeys::LeftShift,false);
        TestTrue(TEXT("An actually unanchored forty-kilogram frame still cannot enter the thirty-six-kilogram grip"),Restored.World->FindBody(FrameId)->State==EExpeditionBodyState::Available && FMath::IsNearlyEqual(Restored.World->GetCargoMass(),RetainedMass));
        TestEqual(TEXT("Refused oversized pickup cannot spend another cast"),Restored.World->GetBattery(),BeforeGrip);
        RuntimeMove(Restored,Restored.World->FindMarker(TEXT("frame_receiver"))->Position);
        const int32 Appraisal=Restored.World->FindBody(FrameId)->Appraisal;
        if (!TestTrue(TEXT("The delivered frame retains meaningful actual appraisal"),Appraisal>=240)) return false;
        Restored.Key(EKeys::E,true);
        TestEqual(TEXT("Only actual E settlement banks current frame appraisal"),Restored.World->GetOutput(),Appraisal);
        TestTrue(TEXT("The installed frame uses the existing once-only material ledger"),Restored.World->FindBody(FrameId)->State==EExpeditionBodyState::Banked);
        TestEqual(TEXT("Frame refinement pays the existing site milestone, never a new final currency"),Restored.Rig->GetCash(),BeforeCash+(Site==2?2:0));
        Restored.Key(EKeys::E,true);
        TestEqual(TEXT("Repeated frame handoff cannot duplicate appraisal"),Restored.World->GetOutput(),Appraisal);
        TestTrue(TEXT("Actual source identities, charge and appraisal remain conserved"),Restored.World->CheckInvariants(Error));
        if (Site==2)
        {
            Restored.RetrySite();
            TestEqual(TEXT("Retry rolls back the actual frame output transaction"),Restored.World->GetOutput(),0);
            TestEqual(TEXT("Retry rolls back its site reward without deleting the earned keystone"),Restored.Rig->GetCash(),BeforeCash);
            TestTrue(TEXT("Retry retains the paid prior-depot specialist"),Restored.Rig->Has(TEXT("walking_gantry")) && Restored.Rig->GetResaleValue(TEXT("walking_gantry"))==7);
            TestTrue(TEXT("Retry waits for the player's explicit Resume"),Restored.bPaused);
            Restored.Key(EKeys::Escape,true);
            const bool Repeated=RuntimeMoveFrameWithGantry(Restored,Error);
            if (!TestTrue(*FString::Printf(TEXT("The same earned rig can physically repeat the restored operation after Resume: %s"),*Error),Repeated)) return false;
            Restored.Key(EKeys::E,true);
            if (!TestTrue(TEXT("The optional frame still permits the baseline rack core recovery"),RuntimeRecoverCore(Restored,Error))) return false;
            Restored.Key(EKeys::E,true);
            if (!TestTrue(TEXT("Actual third core leads to the final depot"),Restored.Screen==EExpeditionScreen::Depot && Restored.SiteIndex==3)) return false;
        }
        else
        {
            const int32 Core=RoleBody(*Restored.World,TEXT("core"));
            TestTrue(TEXT("Actual final core remains held through frame movement and settlement"),Restored.World->FindBody(Core)->State==EExpeditionBodyState::Cargo && FMath::IsNearlyEqual(Restored.World->GetCargoMass(),20.f));
            RuntimeMove(Restored,{Restored.Magnet.X,-260}); RuntimeMove(Restored,FExpeditionWorld::ReceiverPosition());
            Restored.Key(EKeys::E,true);
            TestTrue(TEXT("The same genuinely purchased keystone supplies the final alternate recovery and winning rig"),Restored.Screen==EExpeditionScreen::Victory && Restored.Rig->IsRunWon());
        }
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionPackagedLateRevisionTest,
    "MagnetSweep.Expedition.PackagedLateRevisionOneSurvivesNewFrameDefaultsAndRetry",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionPackagedLateRevisionTest::RunTest(const FString& Parameters)
{
    for (int32 LateSite : {2,3})
    {
        FExpeditionRuntime Runtime(nullptr); FString Error;
        if (!TestTrue(TEXT("Compatibility history starts as a real expedition"),StartRuntime(Runtime,293))) return false;
        for (int32 Site=0; Site<LateSite; ++Site)
        {
            if (!TestTrue(TEXT("Actual earlier objective earns the late saved depot"),RuntimeRecoverCore(Runtime,Error))) return false;
            Runtime.Key(EKeys::E,true);
            if (Site+1<LateSite) Runtime.Depart();
        }
        const FName Layout=LateSite==2?FName(TEXT("balanced_rack")):FName(TEXT("counterweight_exchange"));
        if (!TestTrue(TEXT("Packaged revision one remains an explicit legal authored definition"),Runtime.World->StartSite(LateSite,Runtime.Rig->GetSeed()+LateSite,100,Layout,1))) return false;
        Runtime.Rig->SetShopContext(Runtime.World->GetOpportunityTags(),Implemented());
        const int32 OldBodies=Runtime.World->GetBodies().Num(), Cash=Runtime.Rig->GetCash();
        auto OldDepot=ParseJson(Runtime.EncodeSave()); const auto OldWorld=OldDepot->GetObjectField(TEXT("world"));
        for (const TCHAR* Field : {TEXT("bFrameHoistPowered"),TEXT("bFrameHoistActive"),TEXT("bFrameReceived")}) OldWorld->RemoveField(Field);
        for (const auto& Body:OldWorld->GetArrayField(TEXT("Bodies"))) Body->AsObject()->RemoveField(TEXT("bFloorContact"));
        FExpeditionRuntime Loaded(nullptr);
        const bool Restored=Loaded.DecodeSave(JsonText(OldDepot),Error);
        if (!TestTrue(*FString::Printf(TEXT("Old late depot without new frame fields restores: %s"),*Error),Restored)) return false;
        TestTrue(TEXT("The exact saved revision and absence of new source material are preserved"),Loaded.World->GetState().LayoutRevision==1 && Loaded.World->GetBodies().Num()==OldBodies && Loaded.World->FindFrameBody()==nullptr);
        Loaded.Depart();
        TestTrue(TEXT("Actual old-depot departure never silently adds the new frame"),Loaded.Screen==EExpeditionScreen::Site && Loaded.World->GetState().LayoutRevision==1 && Loaded.World->GetBodies().Num()==OldBodies);
        const int32 Id=LooseBody(*Loaded.World); const FVector2D Point=Loaded.World->FindBody(Id)->Position;
        RuntimeMove(Loaded,Point+FVector2D(0,-25)); Loaded.Key(EKeys::LeftShift,true); Loaded.Key(EKeys::LeftMouseButton,true);
        RuntimeAdvance(Loaded,.05f);
        const FVector2D PaidPosition=Loaded.World->FindBody(Id)->Position;
        const FString PaidSave=Loaded.EncodeSave(); FExpeditionRuntime Resumed(nullptr);
        if (!TestTrue(TEXT("Old-revision actual paid motion resumes"),Resumed.DecodeSave(PaidSave,Error))) return false;
        TestTrue(TEXT("Restore preserves actual source motion exactly"),Resumed.World->FindBody(Id)->Position.Equals(PaidPosition,.001f));
        FExpeditionWorld NewDefault; NewDefault.StartSite(LateSite,Loaded.Rig->GetSeed()+LateSite);
        TestTrue(TEXT("Fresh default is revision two with additional actual frame sources"),NewDefault.GetState().LayoutRevision==2 && NewDefault.FindFrameBody()!=nullptr && NewDefault.GetBodies().Num()>OldBodies);
        auto WrongEntry=ParseJson(PaidSave);
        WrongEntry->GetObjectField(TEXT("site_entry"))->SetObjectField(TEXT("world"),NewDefault.ToJson());
        TestFalse(TEXT("A separately valid new-revision checkpoint cannot replace an old active site"),Resumed.DecodeSave(JsonText(WrongEntry),Error));
        TestTrue(TEXT("Rejected checkpoint keeps the actual old paid session unchanged"),Resumed.EncodeSave()==PaidSave);
        Resumed.RetrySite();
        TestTrue(TEXT("Real retry returns to the saved revision-one entry"),Resumed.World->GetState().LayoutRevision==1 && Resumed.World->GetBodies().Num()==OldBodies && Resumed.World->FindFrameBody()==nullptr);
        TestEqual(TEXT("Old-layout retry restores its actual entry battery"),Resumed.World->GetBattery(),100);
        TestEqual(TEXT("Old-layout retry retains prior earned money"),Resumed.Rig->GetCash(),Cash);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionFrameCheapSupportTest,
    "MagnetSweep.Expedition.FrameAcceptsCheaperPhysicalSupportWithoutKeystoneGate",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionFrameCheapSupportTest::RunTest(const FString& Parameters)
{
    int32 Cost[2]={0,0}, PaidAppraisal[2]={0,0};
    for (int32 Variant=0; Variant<2; ++Variant)
    {
        FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(3,307); FString Error;
        const TArray<FName> Supports=Variant==0?TArray<FName>{TEXT("walking_gantry")}:TArray<FName>{TEXT("counterweight_hook"),TEXT("ratchet_pawl")};
        if (!TestTrue(TEXT("Labelled controlled support comparison validates"),FixtureRig(*Runtime.Rig,*Runtime.World,{TEXT("anchor_winch")},Supports,Error))) return false;
        Runtime.Screen=EExpeditionScreen::Site; Runtime.SiteIndex=3; Runtime.bWorldHit=true;
        const int32 Frame=Runtime.World->FindFrameBody()->Id, Weight=RoleBody(*Runtime.World,TEXT("brace"));
        const FVector2D Dock=Runtime.World->FindMarker(TEXT("frame_receiver"))->Position;
        const FVector2D WeightDock=Runtime.World->FindMarker(TEXT("frame_support_receiver"))->Position;
        if (!TestTrue(TEXT("Both routes use the same real eight-kilogram support"),RuntimePickBody(Runtime,Weight,{0,25},Error))) return false;
        RuntimePlaceHaul(Runtime,Runtime.World->FindMarker(TEXT("frame_support"))->Position);
        RuntimeMove(Runtime,Dock); Runtime.Aim=Runtime.World->FindBody(Frame)->Position;
        Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false); Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,4.f);
        if (Variant==1)
        {
            TestFalse(TEXT("A fixed counterweight left behind cannot count as receiving support"),Runtime.World->CanReceiveFrame(Error));
            TestTrue(TEXT("The cheaper pawl actually holds the moved frame at its reached destination"),Runtime.World->GetState().LatchedBody==Frame && FVector2D::Distance(Runtime.World->FindBody(Frame)->Position,Dock)<24);
            if (!TestTrue(TEXT("The staged route physically regrips its old support"),RuntimePickBody(Runtime,Weight,{0,-25},Error))) return false;
            RuntimePlaceHaul(Runtime,WeightDock); RuntimeMove(Runtime,Dock); RuntimeAdvance(Runtime,1.f);
        }
        const bool Ready=Runtime.World->CanReceiveFrame(Error);
        if (!TestTrue(*FString::Printf(TEXT("Actual support arrangement, not capstone ownership, enables receipt: %s"),*Error),Ready)) return false;
        Cost[Variant]=100-Runtime.World->GetBattery(); PaidAppraisal[Variant]=Runtime.World->FindBody(Frame)->Appraisal;
        AddInfo(FString::Printf(TEXT("Frame support route%d: energy%d, appraisal%d, carried%.0f, frame(%.1f,%.1f), support(%.1f,%.1f)"),Variant,Cost[Variant],PaidAppraisal[Variant],Runtime.World->GetCargoMass(),Runtime.World->FindBody(Frame)->Position.X,Runtime.World->FindBody(Frame)->Position.Y,Runtime.World->FindBody(Weight)->Position.X,Runtime.World->FindBody(Weight)->Position.Y));
        TestEqual(TEXT("Physical arrival still does not automatically settle value"),Runtime.World->GetOutput(),0);
        Runtime.Key(EKeys::E,true);
        TestEqual(TEXT("Both routes explicitly settle the actual source appraisal"),Runtime.World->GetOutput(),PaidAppraisal[Variant]);
        TestTrue(TEXT("Receiving conserves source identity and accounting"),Runtime.World->CheckInvariants(Error));
        if (Variant==1) TestFalse(TEXT("The accepted cheaper route has no hidden Gantry fixture"),Runtime.Rig->Has(TEXT("walking_gantry")));
    }
    TestEqual(TEXT("Gantry setup uses one actual support grip and the fourteen-energy coupled tow"),Cost[0],20);
    TestEqual(TEXT("Cheaper staged setup pays one additional six-energy regrip but a ten-energy tow"),Cost[1],22);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionFrameElectricalTest,
    "MagnetSweep.Expedition.FrameElectricalRoutesShareOcclusionFiniteChargeAndRealHandoff",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionFrameElectricalTest::RunTest(const FString& Parameters)
{
    for (int32 Variant=0; Variant<3; ++Variant)
    {
        // Found equipment is deliberate here. Purchase reachability is tested
        // by the separate earned Gantry run, not asserted for these fixtures.
        FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(3,313); FString Error;
        const TArray<FName> Actives=Variant==0?TArray<FName>{TEXT("arc_driver"),TEXT("anchor_winch")}:TArray<FName>{TEXT("arc_driver")};
        TArray<FName> Passives={TEXT("escapement_relay")};
        if (Variant!=0) Passives.Add(TEXT("closed_circuit"));
        if (!TestTrue(TEXT("Labelled electrical equipment validates"),FixtureRig(*Runtime.Rig,*Runtime.World,Actives,Passives,Error))) return false;
        Runtime.Screen=EExpeditionScreen::Site; Runtime.SiteIndex=3; Runtime.bWorldHit=true;
        const int32 Frame=Runtime.World->FindFrameBody()->Id;
        const FVector2D Initial=Runtime.World->FindBody(Frame)->Position;
        Runtime.Aim=Runtime.World->FindBody(63)->Position;
        TestEqual(TEXT("Actual Arc targeting selects the marked floor contact under the frame"),Runtime.CommandFor(0).TargetId,63);
        auto Denied=Runtime.World->Preview(Runtime.CommandFor(0),*Runtime.Rig);
        TestFalse(TEXT("The shared exposure rule refuses the covered manual contact"),Denied.bAllowed);
        AddInfo(FString::Printf(TEXT("Covered Arc: %s"),*Denied.Reason));
        Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
        Runtime.Click(62); Runtime.Aim=Runtime.World->FindBody(3)->Position;
        Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
        Runtime.Aim=Runtime.World->FindBody(63)->Position;
        TestEqual(TEXT("Actual staged sensor receiver selects the same floor contact"),Runtime.CommandFor(0).SecondaryId,63);
        TestFalse(TEXT("Sensor preparation cannot bypass that same covered contact"),Runtime.World->Preview(Runtime.CommandFor(0),*Runtime.Rig).bAllowed);
        Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
        TestEqual(TEXT("Both covered-target attempts are free refusals"),Runtime.World->GetBattery(),100);
        TestEqual(TEXT("Refusals reserve no finite frame charge"),Runtime.World->FindBody(Frame)->Charge,2);
        TestEqual(TEXT("Refusals leave no deferred trigger"),Runtime.World->GetState().DeferredCharge,0);
        Runtime.Key(EKeys::RightMouseButton,true); Runtime.Click(60);

        const auto PlaceReturn=[&]()
        {
            if (!RuntimePickBody(Runtime,64,{-25,0},Error)) return false;
            RuntimePlaceHaul(Runtime,Runtime.World->FindMarker(TEXT("frame_loop"))->Position);
            return FVector2D::Distance(Runtime.World->FindBody(64)->Position,Runtime.World->FindMarker(TEXT("frame_loop"))->Position)<10;
        };
        if (Variant==1 && !TestTrue(TEXT("The actual directional return tile is physically placed"),PlaceReturn())) return false;
        Runtime.Aim=Runtime.World->FindBody(62)->Position;
        TestEqual(TEXT("The normal Arc input is the exposed marked receiver"),Runtime.CommandFor(0).TargetId,62);
        const auto FirstPreview=Runtime.World->Preview(Runtime.CommandFor(0),*Runtime.Rig);
        if (!TestTrue(*FString::Printf(TEXT("Actual exposed input has a source-backed operation: %s"),*FirstPreview.Reason),FirstPreview.bAllowed)) return false;
        Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
        if (Variant!=1)
        {
            TestEqual(TEXT("An early ordinary pulse spends one finite source charge"),Runtime.World->FindBody(Frame)->Charge,1);
            TestFalse(TEXT("Mount release alone does not invent a powered hoist"),Runtime.World->GetState().bFrameHoistPowered);
            Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,.2f);
            TestTrue(TEXT("The flush contact does not physically push the released frame away"),Runtime.World->FindBody(Frame)->Position.Equals(Initial,2.f));
            if (Variant==0)
            {
                RuntimeMove(Runtime,Runtime.World->FindMarker(TEXT("frame_service"))->Position);
                Runtime.Aim=Runtime.World->FindBody(Frame)->Position;
                Runtime.Key(EKeys::F,true); Runtime.Key(EKeys::F,false);
                Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,4.f);
                if (!TestTrue(TEXT("The cheaper real tow exposes the contact rather than bypassing it"),Runtime.World->IsManualTargetExposed(63))) return false;
                Runtime.Aim=Runtime.World->FindBody(63)->Position;
            }
            else
            {
                if (!TestTrue(TEXT("After an early pulse the player can still physically complete the return"),PlaceReturn())) return false;
                Runtime.Aim=Runtime.World->FindBody(62)->Position;
            }
            const auto SecondPreview=Runtime.World->Preview(Runtime.CommandFor(0),*Runtime.Rig);
            if (!TestTrue(*FString::Printf(TEXT("A still-useful physical branch accepts its remaining charge: %s"),*SecondPreview.Reason),SecondPreview.bAllowed)) return false;
            Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
        }
        TestEqual(TEXT("Actual mount and hoist operations consume exactly the two source charges"),Runtime.World->FindBody(Frame)->Charge,0);
        if (!TestTrue(TEXT("The paid circuit starts the actual hoist"),Runtime.World->GetState().bFrameHoistPowered && Runtime.World->GetState().bFrameHoistActive)) return false;
        TestEqual(TEXT("Electrical preparation creates no automatic output"),Runtime.World->GetOutput(),0);
        Runtime.Key(EKeys::RightMouseButton,true);
        TestTrue(TEXT("Dropping cancels free hand preparation but preserves the paid hoist"),Runtime.World->GetState().bFrameHoistActive);
        const FVector2D PausedPosition=Runtime.World->FindBody(Frame)->Position;
        Runtime.Key(EKeys::Escape,true); RuntimeAdvance(Runtime,.5f);
        TestTrue(TEXT("Actual pause freezes the paid physical hoist"),Runtime.World->FindBody(Frame)->Position.Equals(PausedPosition,.001f));
        Runtime.Key(EKeys::Escape,true);
        FExpeditionWorld Reloaded;
        const bool Loaded=Reloaded.FromJson(Runtime.World->ToJson(),Error);
        if (!TestTrue(*FString::Printf(TEXT("The real paid hoist state restores without another charge: %s"),*Error),Loaded)) return false;
        TestTrue(TEXT("Saved paid motion and finite source charge remain present"),Reloaded.GetState().bFrameHoistActive && Reloaded.FindBody(Frame)->Charge==0);
        Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,5.f);
        RuntimeMove(Runtime,Runtime.World->FindMarker(TEXT("frame_receiver"))->Position); RuntimeAdvance(Runtime,1.f);
        const bool Ready=Runtime.World->CanReceiveFrame(Error);
        const auto* Body=Runtime.World->FindBody(Frame);
        AddInfo(FString::Printf(TEXT("Electrical frame route%d: battery%d, appraisal%d, position(%.2f,%.2f), speed%.2f, receipt: %s"),Variant,Runtime.World->GetBattery(),Body->Appraisal,Body->Position.X,Body->Position.Y,Body->Velocity.Size(),*Error));
        if (!TestTrue(*FString::Printf(TEXT("Actual hoist arrival permits explicit receipt: %s"),*Error),Ready)) return false;
        TestTrue(TEXT("The forty-kilogram body arrives physically without entering cargo"),Body->State==EExpeditionBodyState::Available && FMath::IsNearlyZero(Runtime.World->GetCargoMass()));
        const int32 Appraisal=Body->Appraisal, Battery=Runtime.World->GetBattery();
        Runtime.Key(EKeys::E,true);
        TestEqual(TEXT("E settles the current physical appraisal exactly once"),Runtime.World->GetOutput(),Appraisal);
        Runtime.Key(EKeys::E,true); Runtime.Aim=Runtime.World->FindBody(62)->Position;
        Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
        TestEqual(TEXT("The exhausted installed source cannot produce another payment"),Runtime.World->GetOutput(),Appraisal);
        TestEqual(TEXT("Repeated unavailable operations do not spend or refund battery"),Runtime.World->GetBattery(),Battery);
        TestTrue(TEXT("Electrical recovery conserves source, charge and appraisal"),Runtime.World->CheckInvariants(Error));
        TestEqual(TEXT("The distinct routes pay their actual preparation costs"),100-Battery,Variant==0?24:Variant==1?14:22);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionGantryHistoricalReceiptTest,
    "MagnetSweep.Expedition.GantryPriceChangePreservesHistoricalReceiptWithoutCashGrant",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionGantryHistoricalReceiptTest::RunTest(const FString& Parameters)
{
    FExpeditionRuntime Runtime(nullptr); FString Error;
    const bool Earned=EarnFirstSiteKeystoneBudget(Runtime,TEXT("anchor_winch"),1,Error);
    if (!TestTrue(*FString::Printf(TEXT("Receipt compatibility uses a genuinely earned twenty-four-credit depot: %s"),*Error),Earned)) return false;
    if (!TestTrue(TEXT("The current generated purchase pays fourteen"),Runtime.Rig->Buy(TEXT("walking_gantry"),Error))) return false;
    TestEqual(TEXT("The current transaction retains ten cash"),Runtime.Rig->GetCash(),10);
    const FString Current=Runtime.EncodeSave();
    // Serialization compatibility fixture: reconstruct the earlier legal
    // paid-24 receipt on the same genuinely earned 24-credit ledger. This is
    // not a claim that today's shop sold at the historical price.
    auto Historical=ParseJson(Current); auto RigJson=Historical->GetObjectField(TEXT("rig"));
    RigJson->SetNumberField(TEXT("cash"),0); RigJson->SetNumberField(TEXT("purchased"),24);
    for (const auto& Item:RigJson->GetArrayField(TEXT("inventory")))
        if (Item->AsObject()->GetStringField(TEXT("id"))==TEXT("walking_gantry")) Item->AsObject()->SetNumberField(TEXT("paid"),24);
    FExpeditionRuntime Old(nullptr);
    const bool Loaded=Old.DecodeSave(JsonText(Historical),Error);
    if (!TestTrue(*FString::Printf(TEXT("A coherent historical paid-24 receipt remains legal: %s"),*Error),Loaded)) return false;
    TestEqual(TEXT("Restoring the price change never grants a difference refund"),Old.Rig->GetCash(),0);
    TestEqual(TEXT("Historical resale remains based on actual twenty-four paid"),Old.Rig->GetResaleValue(TEXT("walking_gantry")),12);
    if (!TestTrue(TEXT("Actual historical-receipt sale succeeds"),Old.Rig->Sell(TEXT("walking_gantry"),Error))) return false;
    TestEqual(TEXT("Its one real sale pays twelve"),Old.Rig->GetCash(),12);
    FExpeditionRuntime OldSale(nullptr);
    if (!TestTrue(TEXT("Historical resale transaction restores"),OldSale.DecodeSave(Old.EncodeSave(),Error))) return false;
    TestEqual(TEXT("Resale restore cannot replay the refund"),OldSale.Rig->GetCash(),12);
    TestFalse(TEXT("An already sold historical item cannot pay again"),OldSale.Rig->Sell(TEXT("walking_gantry"),Error));
    TestEqual(TEXT("Today's receipt has seven-credit resale"),Runtime.Rig->GetResaleValue(TEXT("walking_gantry")),7);
    if (!TestTrue(TEXT("Actual current-price sale succeeds"),Runtime.Rig->Sell(TEXT("walking_gantry"),Error))) return false;
    TestEqual(TEXT("Ten retained cash plus seven refund is seventeen"),Runtime.Rig->GetCash(),17);
    for (int32 InvalidPrice : {15,23})
    {
        auto Invalid=ParseJson(Current); auto BadRig=Invalid->GetObjectField(TEXT("rig"));
        BadRig->SetNumberField(TEXT("cash"),24-InvalidPrice); BadRig->SetNumberField(TEXT("purchased"),InvalidPrice);
        for (const auto& Item:BadRig->GetArrayField(TEXT("inventory")))
            if (Item->AsObject()->GetStringField(TEXT("id"))==TEXT("walking_gantry")) Item->AsObject()->SetNumberField(TEXT("paid"),InvalidPrice);
        const FString Before=Runtime.EncodeSave();
        TestFalse(TEXT("Coherent arithmetic does not legalize another fabricated historical price"),Runtime.DecodeSave(JsonText(Invalid),Error));
        TestTrue(TEXT("Receipt rejection preserves the actual live resale transaction"),Runtime.EncodeSave()==Before);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionFrameReactionArrivalTest,
    "MagnetSweep.Expedition.FrameReactionMovesActualTwelveAndTwentyKilogramSupportsToReceipt",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionFrameReactionArrivalTest::RunTest(const FString& Parameters)
{
    for (int32 Site : {2,3})
    {
        FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(Site,317); FString Error;
        if (!TestTrue(TEXT("Labelled found Vector/Reaction equipment validates"),FixtureRig(*Runtime.Rig,*Runtime.World,
            {TEXT("vector_emitter")},{TEXT("reaction_frame")},Error))) return false;
        Runtime.Screen=EExpeditionScreen::Site; Runtime.SiteIndex=Site; Runtime.bWorldHit=true;
        const int32 Frame=Runtime.World->FindFrameBody()->Id;
        const int32 Weight=RoleBody(*Runtime.World,Site==2?FName(TEXT("ballast")):FName(TEXT("counterweight_cover")));
        const FVector2D FrameStart=Runtime.World->FindBody(Frame)->Position;
        const FVector2D Dock=Runtime.World->FindMarker(TEXT("frame_receiver"))->Position;
        const FVector2D WeightDock=Runtime.World->FindMarker(TEXT("frame_support_receiver"))->Position;
        const FVector2D Stage=Runtime.World->FindMarker(TEXT("frame_reaction"))->Position;
        const FVector2D Direction=(FrameStart-Dock).GetSafeNormal();
        if (!TestTrue(TEXT("The support is actually available, not an injected freed machine"),!Runtime.World->FindBody(Weight)->bAnchored)) return false;
        TestEqual(TEXT("Each layout uses its real available source mass"),Runtime.World->FindBody(Weight)->Mass,Site==2?12.f:20.f);
        if (!TestTrue(TEXT("Ordinary paid pickup supplies the actual reaction partner"),RuntimePickBody(Runtime,Weight,{0,25},Error))) return false;
        RuntimePlaceHaul(Runtime,Stage);
        if (!TestTrue(*FString::Printf(TEXT("Actual component-sized drop reaches the authored setup: (%.2f,%.2f)"),Runtime.World->FindBody(Weight)->Position.X,Runtime.World->FindBody(Weight)->Position.Y),Runtime.World->FindBody(Weight)->Position.Equals(Stage,3.f))) return false;
        const int32 Impulses=Site==2?2:1;
        for (int32 PullIndex=0; PullIndex<Impulses; ++PullIndex)
        {
            Runtime.Click(63); Runtime.Aim=Runtime.World->FindBody(Weight)->Position;
            Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
            Runtime.Aim=Runtime.World->FindBody(Frame)->Position;
            Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
            TestEqual(TEXT("Actual staged target is the selected ballast"),Runtime.CommandFor(0).TargetId,Weight);
            TestEqual(TEXT("Actual staged supported partner is the oversized frame"),Runtime.CommandFor(0).SecondaryId,Frame);
            Runtime.Aim=Runtime.Magnet+Direction*100.f;
            const auto Preview=Runtime.World->Preview(Runtime.CommandFor(0),*Runtime.Rig);
            if (!TestTrue(*FString::Printf(TEXT("The real staged reaction can commit: %s"),*Preview.Reason),Preview.bAllowed)) return false;
            const int32 Battery=Runtime.World->GetBattery();
            Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false);
            TestEqual(TEXT("Each deliberately repeated impulse is a new twelve-energy payment"),Runtime.World->GetBattery(),Battery-12);
            Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,3.f);
            AddInfo(FString::Printf(TEXT("Reaction site%d impulse%d: frame(%.2f,%.2f) d%.2f v%.2f; weight%.0fkg(%.2f,%.2f) d%.2f v%.2f"),Site,PullIndex+1,
                Runtime.World->FindBody(Frame)->Position.X,Runtime.World->FindBody(Frame)->Position.Y,FVector2D::Distance(Runtime.World->FindBody(Frame)->Position,Dock),Runtime.World->FindBody(Frame)->Velocity.Size(),Runtime.World->FindBody(Weight)->Mass,
                Runtime.World->FindBody(Weight)->Position.X,Runtime.World->FindBody(Weight)->Position.Y,FVector2D::Distance(Runtime.World->FindBody(Weight)->Position,WeightDock),Runtime.World->FindBody(Weight)->Velocity.Size()));
        }
        RuntimeMove(Runtime,Dock); RuntimeAdvance(Runtime,1.f);
        TestTrue(TEXT("Both actual bodies must settle inside their distinct receiving rings"),FVector2D::Distance(Runtime.World->FindBody(Frame)->Position,Dock)<=24 && FVector2D::Distance(Runtime.World->FindBody(Weight)->Position,WeightDock)<=20);
        const bool Ready=Runtime.World->CanReceiveFrame(Error);
        if (!TestTrue(*FString::Printf(TEXT("Real force and support placement permit the handoff: %s"),*Error),Ready)) return false;
        TestTrue(TEXT("Neither reaction body needs to enter oversized cargo"),Runtime.World->FindBody(Frame)->State==EExpeditionBodyState::Available && FMath::IsNearlyZero(Runtime.World->GetCargoMass()));
        TestEqual(TEXT("Reaction placement alone is not a reward"),Runtime.World->GetOutput(),0);
        const int32 Appraisal=Runtime.World->FindBody(Frame)->Appraisal;
        TestTrue(TEXT("The recovered optional frame retains meaningful real appraisal"),Appraisal>=240);
        Runtime.Key(EKeys::E,true); Runtime.Key(EKeys::E,true);
        TestEqual(TEXT("Actual E receipt banks current appraisal once"),Runtime.World->GetOutput(),Appraisal);
        TestEqual(TEXT("The real lighter support needs a second paid impulse, not a material key"),100-Runtime.World->GetBattery(),6+12*Impulses);
        TestTrue(TEXT("Reaction recovery conserves both real source bodies and appraisal"),Runtime.World->CheckInvariants(Error));
        FExpeditionWorld Saved;
        if (!TestTrue(*FString::Printf(TEXT("Settled reaction receipt restores strictly: %s"),*Error),Saved.FromJson(Runtime.World->ToJson(),Error))) return false;
        TestEqual(TEXT("Reload cannot repay the installed frame"),Saved.GetOutput(),Appraisal);
        TestFalse(TEXT("Installed frame remains unavailable to another settlement"),Saved.ReceiveFrame().bSucceeded);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionFrameRetainedCoreSupportTest,
    "MagnetSweep.Expedition.CheapRemoteFrameSupportPreservesActualHeldFinalCore",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FExpeditionFrameRetainedCoreSupportTest::RunTest(const FString& Parameters)
{
    int32 Cost[2]={0,0};
    for (int32 Variant=0; Variant<2; ++Variant)
    {
        FExpeditionRuntime Runtime(nullptr); Runtime.World->StartSite(3,331); FString Error;
        const TArray<FName> Supports=Variant==0?TArray<FName>{TEXT("walking_gantry")}:TArray<FName>{TEXT("counterweight_hook"),TEXT("ratchet_pawl")};
        if (!TestTrue(TEXT("Labelled matched support loadouts validate"),FixtureRig(*Runtime.Rig,*Runtime.World,{TEXT("anchor_winch")},Supports,Error))) return false;
        Runtime.Screen=EExpeditionScreen::Site; Runtime.SiteIndex=3; Runtime.bWorldHit=true;
        const int32 Frame=Runtime.World->FindFrameBody()->Id, Weight=RoleBody(*Runtime.World,TEXT("brace"));
        const int32 Cover=RoleBody(*Runtime.World,TEXT("counterweight_cover")), Core=RoleBody(*Runtime.World,TEXT("core"));
        const FVector2D Dock=Runtime.World->FindMarker(TEXT("frame_receiver"))->Position;
        const FVector2D WeightDock=Runtime.World->FindMarker(TEXT("frame_support_receiver"))->Position;
        if (!TestTrue(TEXT("The eight-kilogram receiving support is physically prepared"),RuntimePickBody(Runtime,Weight,{0,25},Error))) return false;
        RuntimePlaceHaul(Runtime,Runtime.World->FindMarker(TEXT("frame_support"))->Position);
        if (!TestTrue(TEXT("The actual final cover is picked up normally"),RuntimePickBody(Runtime,Cover,{0,25},Error))) return false;
        RuntimePlaceHaul(Runtime,Runtime.World->FindMarker(TEXT("cover"))->Position);
        if (!TestTrue(TEXT("The actual newly exposed twenty-kilogram core is secured"),RuntimePickBody(Runtime,Core,{0,25},Error))) return false;
        TestTrue(TEXT("Both comparisons begin with a real twenty-kilogram held core"),Runtime.World->FindBody(Core)->State==EExpeditionBodyState::Cargo && FMath::IsNearlyEqual(Runtime.World->GetCargoMass(),20.f));
        RuntimeMove(Runtime,Dock); Runtime.Aim=Runtime.World->FindBody(Frame)->Position;
        Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false); Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,4.f);
        if (Variant==1)
        {
            if (!TestTrue(TEXT("The real reached pawl holds the frame before the support is moved"),Runtime.World->GetState().LatchedBody==Frame)) return false;
            RuntimeMove(Runtime,WeightDock); Runtime.Aim=Runtime.World->FindBody(Weight)->Position;
            const auto Preview=Runtime.World->Preview(Runtime.CommandFor(0),*Runtime.Rig);
            if (!TestTrue(*FString::Printf(TEXT("A separate paid tow can move the existing support: %s"),*Preview.Reason),Preview.bAllowed)) return false;
            Runtime.Key(EKeys::Q,true); Runtime.Key(EKeys::Q,false); Runtime.Aim=Runtime.Magnet; RuntimeAdvance(Runtime,4.f);
            TestEqual(TEXT("Towing a second load does not silently replace the still-needed frame pawl"),Runtime.World->GetState().LatchedBody,Frame);
        }
        RuntimeMove(Runtime,Dock); RuntimeAdvance(Runtime,1.f);
        const bool Ready=Runtime.World->CanReceiveFrame(Error);
        AddInfo(FString::Printf(TEXT("Retained-core support%d: battery%d, frame(%.2f,%.2f), support(%.2f,%.2f), mass%.0f, appraisal%d, receipt: %s"),Variant,Runtime.World->GetBattery(),Runtime.World->FindBody(Frame)->Position.X,Runtime.World->FindBody(Frame)->Position.Y,Runtime.World->FindBody(Weight)->Position.X,Runtime.World->FindBody(Weight)->Position.Y,Runtime.World->GetCargoMass(),Runtime.World->FindBody(Frame)->Appraisal,*Error));
        if (!TestTrue(*FString::Printf(TEXT("Actual remotely moved receiving loads are ready: %s"),*Error),Ready)) return false;
        TestTrue(TEXT("The cheaper route can avoid regrip overload without discarding the core"),Runtime.World->FindBody(Core)->State==EExpeditionBodyState::Cargo && FMath::IsNearlyEqual(Runtime.World->GetCargoMass(),20.f) && !Runtime.World->IsUnsafe());
        const int32 Appraisal=Runtime.World->FindBody(Frame)->Appraisal;
        Cost[Variant]=100-Runtime.World->GetBattery();
        Runtime.Key(EKeys::E,true);
        TestEqual(TEXT("The same explicit handoff settles real appraisal"),Runtime.World->GetOutput(),Appraisal);
        TestTrue(TEXT("Frame installation does not burn or drop the held mission core"),Runtime.World->FindBody(Core)->State==EExpeditionBodyState::Cargo);
        RuntimeMove(Runtime,{Runtime.Magnet.X,-260}); RuntimeMove(Runtime,FExpeditionWorld::ReceiverPosition());
        TestTrue(TEXT("The physically retained core has a genuine final receiver route"),Runtime.World->CanDispatch(Error));
        TestTrue(TEXT("The matched support routes conserve source and output accounting"),Runtime.World->CheckInvariants(Error));
    }
    TestEqual(TEXT("Gantry uses three real preparatory grips plus its coupled tow"),Cost[0],32);
    TestEqual(TEXT("Cheaper remote correction uses the same grips plus two ten-energy tows"),Cost[1],38);
    return true;
}

#endif
