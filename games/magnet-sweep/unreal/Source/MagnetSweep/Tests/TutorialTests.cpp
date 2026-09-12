#include "WorkbenchTutorial.h"
#if WITH_DEV_AUTOMATION_TESTS
#include "WorkbenchRuntime.h"
#include "Dom/JsonObject.h"
#include "Misc/AutomationTest.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

using namespace MagnetSweep;
namespace
{
void ReachBundle(FTutorialProgress& T)
{
    T.Start(); T.Begin(); T.ObserveCapture(false,false,true,4); T.ObserveFieldOff();
}
void ReachRisk(FTutorialProgress& T)
{
    ReachBundle(T); T.ObserveCapture(true,false,true,9); T.ObserveSmelt(false);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialActionTest,"MagnetSweep.Tutorial.ActualActionSequence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialActionTest::RunTest(const FString& Parameters)
{
    FTutorialProgress T; FString Reason;
    TestTrue(TEXT("Default not-yet-started tutorial is valid"),T.Validate(Reason));
    TestFalse(TEXT("Disabled tutorial ignores capture"),T.ObserveCapture(true,true,true,12));
    TestTrue(TEXT("Start enables Welcome"),T.Start() && T.bEnabled && T.Step==ETutorialStep::Welcome);
    TestTrue(TEXT("Begin opens actual attraction lesson"),T.Begin() && T.Step==ETutorialStep::Attract);
    TestFalse(TEXT("One2kg piece does not meet the first haul threshold"),T.ObserveCapture(false,false,true,2));
    TestTrue(TEXT("Capture advances only to release even when linked and precise"),
        T.ObserveCapture(true,true,true,9) && T.Step==ETutorialStep::Release);
    TestFalse(TEXT("Another capture cannot stand in for a player release"),T.ObserveCapture(true,true,true,12));
    TestTrue(TEXT("Actual field release opens bundle lesson"),T.ObserveFieldOff() && T.Step==ETutorialStep::Bundle);
    TestFalse(TEXT("Repeated release does not cascade"),T.ObserveFieldOff());
    TestFalse(TEXT("Ordinary loose pickup cannot pass bundle lesson"),T.ObserveCapture(false,false,true,14));
    TestTrue(TEXT("Actual connected capture opens first smelt lesson"),
        T.ObserveCapture(true,false,true,23) && T.Step==ETutorialStep::FirstSmelt);
    TestTrue(TEXT("Even a quota-reaching smelt first teaches risk"),T.ObserveSmelt(true) && T.Step==ETutorialStep::Risk);
    TestFalse(TEXT("A safe load cannot trigger a first-risk hold"),T.HoldFirstRisk(false));
    TestTrue(TEXT("Actual unsafe cargo requests one teaching hold"),T.HoldFirstRisk(true) && T.bRiskHeld);
    TestFalse(TEXT("Repeated unsafe frames do not repeat the hold event"),T.HoldFirstRisk(true));
    TestFalse(TEXT("An unsuccessful drop is not a learned rescue"),T.ObserveDrop(false));
    TestTrue(TEXT("Hold remains until real rescue"),T.bRiskHeld && !T.bRiskLearned);
    TestTrue(TEXT("Real drop teaches rescue and opens precision"),
        T.ObserveDrop(true) && T.bRiskLearned && !T.bRiskHeld && T.Step==ETutorialStep::Precision);
    TestFalse(TEXT("A hot-cell-only precise capture does not teach useful precision"),T.ObserveCapture(false,true,false,4));
    TestFalse(TEXT("Broad-field useful capture does not pass precision"),T.ObserveCapture(false,false,true,8));
    TestTrue(TEXT("Actual useful precision capture opens the quota lesson"),
        T.ObserveCapture(false,true,true,12) && T.Step==ETutorialStep::Quota);
    TestFalse(TEXT("Below-quota smelt cannot open upgrades"),T.ObserveSmelt(false));
    TestTrue(TEXT("Met quota opens real upgrade purchase"),T.ObserveSmelt(true) && T.Step==ETutorialStep::Upgrade);
    TestFalse(TEXT("Unknown upgrade identity is not a purchase"),T.ObservePurchase(3));
    TestTrue(TEXT("Actual coil purchase opens upgrade use"),
        T.ObservePurchase(1) && T.bPurchasedMod && T.PurchasedMod==1 && T.Step==ETutorialStep::TryUpgrade);
    TestFalse(TEXT("Unsuccessful empty capture cannot complete practice"),T.ObserveCapture(false,false,true,0));
    TestFalse(TEXT("Capturing only a hazard cannot complete upgrade practice"),T.ObserveCapture(false,false,false,4));
    TestTrue(TEXT("Using the upgraded rig on useful salvage completes guidance"),
        T.ObserveCapture(false,false,true,8) && T.Step==ETutorialStep::Complete && !T.bEnabled);
    TestTrue(TEXT("Completed tutorial persists its learned events"),T.bRiskLearned && T.bPurchasedMod && T.Validate(Reason));
    TestFalse(TEXT("Post-completion events cannot restart lessons"),T.ObserveSmelt(true));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialEarlyRiskTest,"MagnetSweep.Tutorial.EarlyRiskAndRetryHold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialEarlyRiskTest::RunTest(const FString& Parameters)
{
    FTutorialProgress T; T.Start(); T.Begin();
    T.ObserveCapture(false,false,false,4);
    TestTrue(TEXT("An early hazardous capture still reaches release lesson"),T.Step==ETutorialStep::Release);
    T.HoldFirstRisk(true);
    TestTrue(TEXT("First danger holds even before the dedicated risk lesson"),T.bRiskHeld);
    TestTrue(TEXT("Rescue while learning release returns to attraction with an empty haul"),
        T.ObserveDrop(true) && T.Step==ETutorialStep::Attract && T.bRiskLearned && !T.bRiskHeld);
    T.ObserveCapture(false,false,true,4); T.ObserveFieldOff(); T.ObserveCapture(true,false,true,13);
    TestTrue(TEXT("Later first smelt honors already-observed rescue without repeating it"),
        T.ObserveSmelt(false) && T.Step==ETutorialStep::Precision);
    TestFalse(TEXT("Later dangers are real gameplay, not another teaching hold"),T.HoldFirstRisk(true));

    FTutorialProgress Earlier; ReachBundle(Earlier); Earlier.HoldFirstRisk(true);
    TestTrue(TEXT("Rescue during bundle lesson records learning without skipping the bundle action"),
        Earlier.ObserveDrop(true) && Earlier.Step==ETutorialStep::Bundle && Earlier.bRiskLearned);

    FTutorialProgress Retry; ReachRisk(Retry); Retry.HoldFirstRisk(true);
    TestTrue(TEXT("A genuinely reset safe board clears its stale teaching hold"),Retry.HoldFirstRisk(false));
    TestTrue(TEXT("Retry does not claim a rescue or reset the pending lesson"),
        !Retry.bRiskLearned && !Retry.bRiskHeld && Retry.Step==ETutorialStep::Risk);
    TestFalse(TEXT("Restocking itself does not pass risk"),Retry.Reconcile(false,true,true));
    TestTrue(TEXT("A new unsafe haul can still receive the missing first hold"),Retry.HoldFirstRisk(true));
    TestTrue(TEXT("An actual subsequent rescue finishes the pending lesson"),
        Retry.ObserveDrop(true) && Retry.Step==ETutorialStep::Precision);
    FString Reason; TestTrue(TEXT("Retry/rescue state remains persistable"),Retry.Validate(Reason));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialReconcileTest,"MagnetSweep.Tutorial.OffPathPlayAndExhaustedTray",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialReconcileTest::RunTest(const FString& Parameters)
{
    FTutorialProgress T; ReachBundle(T);
    TestTrue(TEXT("Off-path early purchase is recorded without skipping the current lesson"),
        T.ObservePurchase(2) && T.Step==ETutorialStep::Bundle && T.PurchasedMod==2);
    TestFalse(TEXT("Available bundles require an actual linked capture"),T.Reconcile(true,true,true));
    TestTrue(TEXT("No remaining bundle avoids an impossible bundle gate"),
        T.Reconcile(true,false,false) && T.Step==ETutorialStep::FirstSmelt);
    TestTrue(TEXT("Met quota is evidence of an earlier smelt, but does not skip risk"),
        T.Reconcile(true,false,false) && T.Step==ETutorialStep::Risk);
    T.HoldFirstRisk(true);
    TestFalse(TEXT("Board reconciliation cannot dismiss a held danger"),T.Reconcile(true,false,false));
    T.ObserveDrop(true);
    TestFalse(TEXT("Exhausted board cannot invent a precision event"),T.Reconcile(true,false,false));
    TestTrue(TEXT("Pending precision explicitly requests useful fresh material"),
        T.Step==ETutorialStep::Precision && T.NeedsFreshTray(false));
    TestFalse(TEXT("Available useful material requires no restock"),T.NeedsFreshTray(true));
    TestFalse(TEXT("Retry/restock retains the pending precision lesson"),T.Reconcile(false,true,true));
    T.ObserveCapture(false,true,true,4);
    TestTrue(TEXT("Actual precision followed by existing quota uses the prior real purchase"),
        T.Reconcile(true,true,true) && T.Step==ETutorialStep::TryUpgrade);
    TestFalse(TEXT("Empty board does not count as using the upgrade"),T.Reconcile(true,false,false));
    TestTrue(TEXT("Upgrade practice can request a new tray without buying the mod twice"),
        T.NeedsFreshTray(false) && T.bPurchasedMod && T.PurchasedMod==2);
    T.Reconcile(false,true,true);
    TestTrue(TEXT("Actual useful capture completes the retained upgrade lesson"),T.ObserveCapture(false,false,true,4));
    TestFalse(TEXT("Completed guidance never requests a forced replay"),T.NeedsFreshTray(false));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialValidationTest,"MagnetSweep.Tutorial.TerminalAndPersistenceValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialValidationTest::RunTest(const FString& Parameters)
{
    FTutorialProgress T; ReachRisk(T); T.HoldFirstRisk(true);
    TestFalse(TEXT("Skip cannot silently remove protection while dangerous cargo is held"),T.Skip());
    TestTrue(TEXT("Rejected skip preserves the held lesson"),T.bEnabled && T.bRiskHeld);
    T.ObserveDrop(true);
    TestTrue(TEXT("Skip disables safely after actual drop"),T.Skip() && !T.bEnabled && T.Step==ETutorialStep::Skipped);
    TestFalse(TEXT("Finish does not falsely relabel a skipped tutorial completed"),T.Finish());
    TestTrue(TEXT("Skipped terminal identity is preserved"),T.Step==ETutorialStep::Skipped);
    TestFalse(TEXT("Skipped state ignores new purchase events"),T.ObservePurchase(0));
    TestTrue(TEXT("Only explicit Start resets terminal and learned state"),
        T.Start() && T.Step==ETutorialStep::Welcome && !T.bRiskLearned && !T.bPurchasedMod);
    TestTrue(TEXT("Explicit finish produces a disabled completed record"),T.Finish() && T.Step==ETutorialStep::Complete && !T.bEnabled);
    TestFalse(TEXT("Skip does not overwrite a completed record"),T.Skip());
    FString Reason;
    TestTrue(TEXT("Terminal record validates"),T.Validate(Reason));
    const auto Valid = [&Reason](int32 Step,bool Enabled,bool Learned,bool Held,bool Purchased,int32 Mod) {
        return FTutorialProgress::ValidateFields(Step,Enabled,Learned,Held,Purchased,Mod,Reason);
    };
    TestFalse(TEXT("Negative serialized step is rejected before enum conversion"),Valid(-1,true,false,false,false,-1));
    TestFalse(TEXT("Unknown serialized step is rejected"),Valid(99,true,false,false,false,-1));
    TestFalse(TEXT("Enabled completed state is rejected"),Valid(int32(ETutorialStep::Complete),true,true,false,true,0));
    TestFalse(TEXT("Disabled mid-lesson state is rejected"),Valid(int32(ETutorialStep::Risk),false,false,false,false,-1));
    TestFalse(TEXT("A learned risk cannot retain a teaching hold"),Valid(int32(ETutorialStep::Risk),true,true,true,false,-1));
    TestFalse(TEXT("Later lesson cannot forge an unobserved rescue"),Valid(int32(ETutorialStep::Precision),true,false,false,false,-1));
    TestFalse(TEXT("Practice requires a recorded purchase"),Valid(int32(ETutorialStep::TryUpgrade),true,true,false,false,-1));
    TestFalse(TEXT("Unknown purchased mod is rejected"),Valid(int32(ETutorialStep::Upgrade),true,true,false,true,3));
    TestFalse(TEXT("Absent purchase cannot carry a mod identity"),Valid(int32(ETutorialStep::Quota),true,true,false,false,1));
    TestTrue(TEXT("A valid saved hold restores enough state for safe resume"),Valid(int32(ETutorialStep::Release),true,false,true,false,-1));
    TestTrue(TEXT("A valid off-path purchased mod can persist before risk"),Valid(int32(ETutorialStep::Bundle),true,false,false,true,2));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialRuntimeTest,"MagnetSweep.Tutorial.RuntimeRescueAndSkipCallbacks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialRuntimeTest::RunTest(const FString& Parameters)
{
    // Real runtime callbacks, with no world/scene/audio required. Tick itself needs a
    // player controller and is intentionally not represented as a rendered/native test.
    FWorkbenchImpl Runtime(nullptr); Runtime.bMuted=true;
    TArray<int32> Iron; for(int32 Id=0;Id<12;++Id)Iron.Add(Id);
    Runtime.Model.CapturePieces(Iron); Runtime.Model.BankCargo();
    const int32 Banked=Runtime.Model.GetBanked(), Wallet=Runtime.Model.GetWallet(), XP=Runtime.Model.GetXP();
    Runtime.Tutorial.Start(); Runtime.Tutorial.Begin();
    Runtime.Action=EMagnetAction::Sweep; Runtime.bFieldLatched=true;
    Runtime.OnRecovery(Runtime.Model.CapturePieces({35}),false,1);
    TestTrue(TEXT("Actual hot-cell recovery establishes the first safety hold before risk advancement"),
        Runtime.Model.IsCargoUnsafe() && Runtime.Tutorial.bRiskHeld && !Runtime.Tutorial.bRiskLearned);
    TestTrue(TEXT("Held danger stops the actual runtime field and its toggle"),
        Runtime.Action==EMagnetAction::None && !Runtime.bFieldLatched);
    TestEqual(TEXT("OnRecovery has not spent the protected first fuse"),Runtime.Model.GetFuseElapsed(),0.f);
    TestEqual(TEXT("First warning has not burned an extra fuel charge"),Runtime.Model.GetHeatsRemaining(),3);
    Runtime.UpdateTutorial();
    TestTrue(TEXT("Runtime reconciliation preserves the first warning hold"),Runtime.Tutorial.bRiskHeld);
    Runtime.Vent();
    TestEqual(TEXT("Actual runtime drop empties the model haul"),Runtime.Model.GetCargoMass(),0);
    TestTrue(TEXT("Actual runtime drop clears the hold and records the rescue"),
        !Runtime.Tutorial.bRiskHeld && Runtime.Tutorial.bRiskLearned);
    TestTrue(TEXT("Dropping during release returns to attraction practice"),Runtime.Tutorial.Step==ETutorialStep::Attract);
    TestTrue(TEXT("Actual spill callback generated feedback particles"),!Runtime.Particles.IsEmpty());
    TestEqual(TEXT("Guided rescue preserves previously banked material"),Runtime.Model.GetBanked(),Banked);
    TestEqual(TEXT("Guided rescue preserves previously earned money"),Runtime.Model.GetWallet(),Wallet);
    TestEqual(TEXT("Guided rescue preserves previously earned XP"),Runtime.Model.GetXP(),XP);
    TestEqual(TEXT("Guided rescue preserves fuel"),Runtime.Model.GetHeatsRemaining(),3);

    FWorkbenchImpl SkipRuntime(nullptr); SkipRuntime.bMuted=true;
    SkipRuntime.Tutorial.Start(); SkipRuntime.Tutorial.Begin();
    SkipRuntime.OnRecovery(SkipRuntime.Model.CapturePieces({35}),false,1);
    TestTrue(TEXT("Skip fixture begins with an actual held dangerous haul"),SkipRuntime.Tutorial.bRiskHeld);
    SkipRuntime.bPaused=true;
    SkipRuntime.Button(451);
    TestEqual(TEXT("Actual Skip button drops dangerous cargo before disabling guidance"),SkipRuntime.Model.GetCargoMass(),0);
    TestTrue(TEXT("Skip button ends with a safe disabled skipped record"),
        !SkipRuntime.Tutorial.bEnabled && !SkipRuntime.Tutorial.bRiskHeld && SkipRuntime.Tutorial.Step==ETutorialStep::Skipped);
    TestTrue(TEXT("Skip button restores the user's pause state"),SkipRuntime.bPaused);
    TestTrue(TEXT("Skipped hot cell remains available for honest later play"),
        SkipRuntime.Model.FindPiece(35)->State==EPieceState::Available);
    TestEqual(TEXT("Skipping a held first danger does not spend fuel"),SkipRuntime.Model.GetHeatsRemaining(),4);
    TestEqual(TEXT("Skipping grants no money"),SkipRuntime.Model.GetWallet(),0);
    FString Reason;
    TestTrue(TEXT("Skipped model remains a valid career"),SkipRuntime.Model.CheckInvariants(Reason));
    TestTrue(TEXT("Skipped tutorial metadata validates"),SkipRuntime.Tutorial.Validate(Reason));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialJsonTest,"MagnetSweep.Tutorial.MetadataRoundTripAndTransactionalRejection",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialJsonTest::RunTest(const FString& Parameters)
{
    FWorkbenchImpl Writer(nullptr); Writer.bMuted=true; Writer.SfxVolume=.65f; Writer.MusicVolume=.2f;
    Writer.Tutorial.Start(); Writer.Tutorial.Begin();
    Writer.OnRecovery(Writer.Model.CapturePieces({35}),false,1);
    const FString Encoded=Writer.EncodeSave();
    FWorkbenchImpl Loaded(nullptr);
    TestTrue(TEXT("Actual career JSON accepts tutorial metadata"),Loaded.DecodeSave(Encoded));
    TestTrue(TEXT("JSON preserves exact pending release lesson and first-risk hold"),
        Loaded.Tutorial.Step==ETutorialStep::Release && Loaded.Tutorial.bEnabled && Loaded.Tutorial.bRiskHeld);
    TestFalse(TEXT("Saved hold is not falsely marked learned"),Loaded.Tutorial.bRiskLearned);
    TestEqual(TEXT("Tutorial JSON retains the corresponding actual dangerous cargo"),Loaded.Model.GetCargoMass(),4);
    TestEqual(TEXT("Tutorial JSON retains unspent first-warning fuse"),Loaded.Model.GetFuseElapsed(),0.f);
    TestTrue(TEXT("Tutorial JSON preserves real audio settings"),Loaded.bMuted && Loaded.SfxVolume==.65f && Loaded.MusicVolume==.2f);

    auto Parse=[&Encoded](){
        TSharedPtr<FJsonObject> Json;
        FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Encoded),Json);
        return Json;
    };
    auto Serialize=[](const TSharedPtr<FJsonObject>& Json){
        FString Text; FJsonSerializer::Serialize(Json.ToSharedRef(),TJsonWriterFactory<>::Create(&Text)); return Text;
    };
    auto Legacy=Parse();
    TestTrue(TEXT("Integration fixture is valid JSON"),Legacy.IsValid());
    if(!Legacy.IsValid())return false;
    Legacy->RemoveField(TEXT("tutorial"));
    FWorkbenchImpl ExistingCareer(nullptr); ExistingCareer.Tutorial.Start(); ExistingCareer.Tutorial.Begin();
    TestTrue(TEXT("An older v2 career without tutorial metadata remains loadable"),ExistingCareer.DecodeSave(Serialize(Legacy)));
    TestFalse(TEXT("Loading an old career does not force new guidance"),ExistingCareer.Tutorial.bEnabled);
    TestEqual(TEXT("Legacy loading preserves its real cargo instead of starting over"),ExistingCareer.Model.GetCargoMass(),4);

    const int32 LiveCargo=Loaded.Model.GetCargo(), LiveMass=Loaded.Model.GetCargoMass(), LiveWallet=Loaded.Model.GetWallet();
    const auto LiveStep=Loaded.Tutorial.Step;
    const auto Reject=[&](const TCHAR* Label,const TFunction<void(FJsonObject&)>& Corrupt){
        TestTrue(TEXT("Each malformed variant starts from an independently restored valid baseline"),Loaded.DecodeSave(Encoded));
        const uint32 LiveEpoch=Loaded.Model.GetEpoch();
        auto Json=Parse(); const auto Teaching=Json->GetObjectField(TEXT("tutorial"));
        Corrupt(*Teaching);
        // Other valid-looking fields change too: rejection must occur before applying
        // any model or settings state, not merely before assigning the tutorial object.
        const auto Snapshot=Json->GetObjectField(TEXT("snapshot"));
        Snapshot->SetNumberField(TEXT("wallet"),5000); Snapshot->SetNumberField(TEXT("xp"),5000);
        Json->SetBoolField(TEXT("muted"),false); Json->SetNumberField(TEXT("sfx_volume"),1);
        TestFalse(Label,Loaded.DecodeSave(Serialize(Json)));
        TestEqual(TEXT("Rejected metadata preserves live cargo value"),Loaded.Model.GetCargo(),LiveCargo);
        TestEqual(TEXT("Rejected metadata preserves live cargo mass"),Loaded.Model.GetCargoMass(),LiveMass);
        TestEqual(TEXT("Rejected metadata preserves live wallet"),Loaded.Model.GetWallet(),LiveWallet);
        TestEqual(TEXT("Rejected metadata preserves live epoch"),Loaded.Model.GetEpoch(),LiveEpoch);
        TestTrue(TEXT("Rejected metadata preserves live lesson and safety hold"),Loaded.Tutorial.Step==LiveStep && Loaded.Tutorial.bRiskHeld);
        TestTrue(TEXT("Rejected metadata preserves live audio settings"),Loaded.bMuted && Loaded.SfxVolume==.65f);
    };
    Reject(TEXT("Out-of-range raw tutorial integer is rejected"),[](FJsonObject& T){T.SetNumberField(TEXT("step"),4096);});
    Reject(TEXT("Fractional raw tutorial integer is rejected"),[](FJsonObject& T){T.SetNumberField(TEXT("step"),2.5);});
    Reject(TEXT("A numeric string is not a raw tutorial integer"),[](FJsonObject& T){T.SetStringField(TEXT("step"),TEXT("2"));});
    Reject(TEXT("A numeric tutorial boolean is not accepted as true"),[](FJsonObject& T){T.SetNumberField(TEXT("enabled"),1);});
    Reject(TEXT("A string tutorial boolean is rejected"),[](FJsonObject& T){T.SetStringField(TEXT("risk_held"),TEXT("true"));});
    Reject(TEXT("Missing tutorial boolean is rejected"),[](FJsonObject& T){T.RemoveField(TEXT("risk_held"));});
    Reject(TEXT("Contradictory learned-and-held state is rejected"),[](FJsonObject& T){T.SetBoolField(TEXT("risk_learned"),true);});
    Reject(TEXT("Unknown purchase identity is rejected"),[](FJsonObject& T){T.SetBoolField(TEXT("purchased_mod"),true);T.SetNumberField(TEXT("mod"),3);});
    return true;
}
#endif
