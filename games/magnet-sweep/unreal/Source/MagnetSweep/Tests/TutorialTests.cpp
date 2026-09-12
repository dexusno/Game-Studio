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
void ReachRisk(FTutorialProgress& T)
{
    T.Start(); T.Begin(); T.ObserveCapture(false,false,true,2);
    T.ObserveSmelt(true); T.ObservePurchase(1); T.ObserveCapture(false,false,true,2);
}
bool EarnCoil(FWorkbenchImpl& Runtime)
{
    Runtime.Tutorial.Start(); Runtime.Tutorial.Begin();
    for(int32 Start : {0,12})
    {
        TArray<int32> Iron; for(int32 Id=Start;Id<Start+12;++Id)Iron.Add(Id);
        Runtime.Model.CapturePieces(Iron); Runtime.Model.BankCargo();
        Runtime.Tutorial.ObserveSmelt(Runtime.Model.IsDeliveryCompleted());
    }
    Runtime.Model.CapturePieces({24}); Runtime.Model.BankCargo();
    Runtime.Tutorial.ObserveSmelt(Runtime.Model.IsDeliveryCompleted());
    if(!Runtime.Model.PurchaseUpgrade(EUpgrade::Coil))return false;
    Runtime.Tutorial.ObservePurchase(int32(EUpgrade::Coil));
    return true;
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialActionTest,"MagnetSweep.Tutorial.ActualActionSequence",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialActionTest::RunTest(const FString& Parameters)
{
    FTutorialProgress T; FString Reason;
    TestTrue(TEXT("Default tutorial is valid and does not change normal capture"),T.Validate(Reason) && !T.IsTrainingGuardActive());
    TestFalse(TEXT("Disabled tutorial ignores capture"),T.ObserveCapture(true,true,true,12));
    T.Start(); T.Begin();
    TestTrue(TEXT("Novice capture protection begins before any earned purchase"),T.IsTrainingGuardActive());
    TestFalse(TEXT("A valueless hot cell cannot stand in for useful salvage"),T.ObserveCapture(false,false,false,4));
    TestTrue(TEXT("One useful piece goes straight to freely available smelting"),
        T.ObserveCapture(false,false,true,2) && T.Step==ETutorialStep::FirstSmelt);
    TestFalse(TEXT("Field release is no longer an ordered prerequisite"),T.ObserveFieldOff());
    TestTrue(TEXT("Even a small real first payout advances to earning the upgrade"),
        T.ObserveSmelt(false) && T.Step==ETutorialStep::Quota);
    TestTrue(TEXT("First payout does not claim danger learning or remove novice protection"),
        !T.bRiskLearned && T.IsTrainingGuardActive() && T.Validate(Reason));
    TestTrue(TEXT("An actual quota payout opens the shop before risk practice"),
        T.ObserveSmelt(true) && T.Step==ETutorialStep::Upgrade && !T.bRiskLearned && T.Validate(Reason));
    TestFalse(TEXT("Invalid purchase identity cannot end novice protection"),T.ObservePurchase(3));
    TestTrue(TEXT("Real purchase alone ends the training guard and opens use"),
        T.ObservePurchase(1) && T.Step==ETutorialStep::TryUpgrade && !T.IsTrainingGuardActive() && T.Validate(Reason));
    TestFalse(TEXT("Empty or hazardous capture cannot demonstrate useful upgrade use"),T.ObserveCapture(false,false,false,4));
    TestTrue(TEXT("Useful upgraded capture introduces later risk practice"),
        T.ObserveCapture(false,false,true,4) && T.Step==ETutorialStep::Risk);
    TestFalse(TEXT("A safe load cannot create a first-warning hold"),T.HoldFirstRisk(false));
    TestTrue(TEXT("The actual first unsafe haul receives a rescue hold"),T.HoldFirstRisk(true));
    TestFalse(TEXT("A repeated warning does not restart the hold"),T.HoldFirstRisk(true));
    TestFalse(TEXT("An unsuccessful drop does not teach rescue"),T.ObserveDrop(false));
    TestTrue(TEXT("Real rescue leads to precision after the earned upgrade"),
        T.ObserveDrop(true) && T.bRiskLearned && !T.bRiskHeld && T.Step==ETutorialStep::Precision);
    TestFalse(TEXT("Dropping again cannot trap or erase the precision lesson"),T.ObserveDrop(true));
    TestFalse(TEXT("Hot-cell precision cannot finish useful practice"),T.ObserveCapture(false,true,false,4));
    TestFalse(TEXT("Broad capture cannot stand in for precision"),T.ObserveCapture(false,false,true,8));
    TestTrue(TEXT("Actual useful precision completes the later rehearsal"),
        T.ObserveCapture(false,true,true,12) && T.Step==ETutorialStep::Complete && !T.bEnabled);
    TestTrue(TEXT("Completion retains actual rescue and purchase evidence"),T.bRiskLearned && T.bPurchasedMod && T.Validate(Reason));
    TestFalse(TEXT("Completion does not reinstate the training guard"),T.IsTrainingGuardActive());
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialEarlyRiskTest,"MagnetSweep.Tutorial.EarlyRiskAndRetryHold",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialEarlyRiskTest::RunTest(const FString& Parameters)
{
    // Old saved tutorials can already hold unsafe cargo before the new training
    // guard. Honor their actual rescue without forcing danger again before rewards.
    FTutorialProgress Legacy; Legacy.Start(); Legacy.Step=ETutorialStep::Release;
    Legacy.HoldFirstRisk(true);
    TestFalse(TEXT("Reconciliation cannot dismiss a restored first warning"),Legacy.Reconcile(false,true,true));
    TestTrue(TEXT("Dropping a legacy held haul returns to unrestricted collection"),
        Legacy.ObserveDrop(true) && Legacy.Step==ETutorialStep::Attract && Legacy.bRiskLearned && !Legacy.bRiskHeld);
    TestTrue(TEXT("Real payment after the old warning still opens earning guidance"),
        Legacy.ObserveSmelt(false) && Legacy.Step==ETutorialStep::Quota);
    Legacy.ObservePurchase(2);
    TestTrue(TEXT("Already-learned rescue is preserved when the real upgraded rig is used"),
        Legacy.ObserveCapture(false,false,true,2) && Legacy.Step==ETutorialStep::Precision);
    TestFalse(TEXT("Previously learned danger does not receive another false first hold"),Legacy.HoldFirstRisk(true));
    TestTrue(TEXT("Actual precision can finish the preserved lesson"),Legacy.ObserveCapture(false,true,true,2));

    FTutorialProgress Retry; ReachRisk(Retry); Retry.HoldFirstRisk(true);
    TestTrue(TEXT("Restocking a safe board clears only its stale hold"),Retry.HoldFirstRisk(false));
    TestTrue(TEXT("Retry cannot invent rescue or undo the purchased rig"),
        !Retry.bRiskLearned && Retry.bPurchasedMod && Retry.Step==ETutorialStep::Risk && !Retry.IsTrainingGuardActive());
    TestFalse(TEXT("New material itself does not pass the risk action"),Retry.Reconcile(false,true,true));
    TestTrue(TEXT("The missing first rescue can still receive a real warning hold"),Retry.HoldFirstRisk(true));
    TestTrue(TEXT("Actual drop advances the retained risk lesson"),Retry.ObserveDrop(true) && Retry.Step==ETutorialStep::Precision);
    FString Reason; TestTrue(TEXT("Retry result remains saveable"),Retry.Validate(Reason));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialReconcileTest,"MagnetSweep.Tutorial.OffPathPlayAndExhaustedTray",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialReconcileTest::RunTest(const FString& Parameters)
{
    const ETutorialStep Early[]={ETutorialStep::Welcome,ETutorialStep::Attract,ETutorialStep::Release,
        ETutorialStep::Bundle,ETutorialStep::FirstSmelt};
    for(auto Step:Early)
    {
        FTutorialProgress T; T.Start(); T.Step=Step;
        TestTrue(TEXT("Every early lesson accepts a real below-quota payout without prerequisites"),
            T.ObserveSmelt(false) && T.Step==ETutorialStep::Quota);
        TestFalse(TEXT("Recognizing payout never fabricates danger learning"),T.bRiskLearned);
        TestTrue(TEXT("An actual purchase funded across retries bypasses unnecessary current quota"),
            T.ObservePurchase(0) && T.Step==ETutorialStep::TryUpgrade && !T.IsTrainingGuardActive());
    }
    for(auto Step:{ETutorialStep::Release,ETutorialStep::Bundle})
    {
        FTutorialProgress T; T.Start(); T.Step=Step;
        TestTrue(TEXT("Legacy release/bundle gates are removed even when links remain"),
            T.Reconcile(false,true,true) && T.Step==ETutorialStep::FirstSmelt);
        TestTrue(TEXT("Dropping the whole practice load gives a fresh collection instruction"),
            T.ObserveDrop(true) && T.Step==ETutorialStep::Attract);
        TestTrue(TEXT("Existing actual cargo makes a resumed collection save freely smeltable"),
            T.Reconcile(false,true,true,4) && T.Step==ETutorialStep::FirstSmelt);
    }
    for(auto Step:{ETutorialStep::Risk,ETutorialStep::Precision})
    {
        FTutorialProgress T; T.Start(); T.Step=Step; T.bRiskLearned=Step==ETutorialStep::Precision;
        TestTrue(TEXT("Old pre-upgrade danger lessons resume earning first"),
            T.Reconcile(false,true,true) && T.Step==ETutorialStep::Quota);
        TestTrue(TEXT("Earned quota is sufficient to open purchase guidance"),
            T.Reconcile(true,true,true) && T.Step==ETutorialStep::Upgrade);
    }
    FTutorialProgress T; ReachRisk(T); T.HoldFirstRisk(true); T.ObserveDrop(true);
    TestFalse(TEXT("Exhausted tray cannot invent a precision event"),T.Reconcile(true,false,false));
    TestTrue(TEXT("Unfinished precision requests fresh useful material"),T.NeedsFreshTray(false));
    TestFalse(TEXT("Existing useful material needs no restock"),T.NeedsFreshTray(true));
    T.ObserveCapture(false,true,true,4);
    TestFalse(TEXT("Completed guidance cannot demand forced replay"),T.NeedsFreshTray(false));
    FTutorialProgress Paid; Paid.Start(); Paid.ObserveSmelt(false);
    TestFalse(TEXT("Dropping after payment keeps the earning objective"),Paid.ObserveDrop(true));
    TestTrue(TEXT("No second release/bundle gate after dropping or retrying"),Paid.Step==ETutorialStep::Quota);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialValidationTest,"MagnetSweep.Tutorial.TerminalAndPersistenceValidation",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialValidationTest::RunTest(const FString& Parameters)
{
    FTutorialProgress T; ReachRisk(T); T.HoldFirstRisk(true);
    TestFalse(TEXT("Skip cannot silently abandon protected dangerous cargo"),T.Skip());
    T.ObserveDrop(true);
    TestTrue(TEXT("Skip disables safely after the actual drop"),T.Skip() && T.Step==ETutorialStep::Skipped && !T.bEnabled);
    TestFalse(TEXT("Finish preserves a skipped terminal record"),T.Finish());
    TestFalse(TEXT("Disabled guidance ignores later purchases"),T.ObservePurchase(0));
    T.Start();
    TestTrue(TEXT("Explicit Start alone resets teaching progress"),!T.bRiskLearned && !T.bPurchasedMod && T.IsTrainingGuardActive());
    TestTrue(TEXT("Explicit finish retains its original terminal API"),T.Finish() && T.Step==ETutorialStep::Complete);
    TestFalse(TEXT("Skipping never overwrites completed metadata"),T.Skip());
    FString Reason;
    const auto Valid=[&Reason](int32 Step,bool Enabled,bool Learned,bool Held,bool Purchased,int32 Mod){
        return FTutorialProgress::ValidateFields(Step,Enabled,Learned,Held,Purchased,Mod,Reason);
    };
    const ETutorialStep SavedIds[]={ETutorialStep::Welcome,ETutorialStep::Attract,ETutorialStep::Release,
        ETutorialStep::Bundle,ETutorialStep::FirstSmelt,ETutorialStep::Risk,ETutorialStep::Precision,
        ETutorialStep::Quota,ETutorialStep::Upgrade,ETutorialStep::TryUpgrade,ETutorialStep::Complete,ETutorialStep::Skipped};
    for(int32 Id=0;Id<12;++Id)TestEqual(TEXT("Existing serialized enum IDs stay unchanged"),int32(SavedIds[Id]),Id);
    TestFalse(TEXT("Negative saved step is rejected before enum conversion"),Valid(-1,true,false,false,false,-1));
    TestFalse(TEXT("Unknown saved step is rejected"),Valid(99,true,false,false,false,-1));
    TestFalse(TEXT("Enabled completed state is rejected"),Valid(10,true,true,false,true,0));
    TestFalse(TEXT("Disabled mid-lesson state is rejected"),Valid(5,false,false,false,false,-1));
    TestFalse(TEXT("Learned and held danger is contradictory"),Valid(5,true,true,true,false,-1));
    TestFalse(TEXT("Precision cannot forge an unobserved rescue"),Valid(6,true,false,false,true,0));
    TestFalse(TEXT("Upgrade use requires actual purchase evidence"),Valid(9,true,false,false,false,-1));
    TestFalse(TEXT("Unknown purchased mod is rejected"),Valid(8,true,false,false,true,3));
    TestFalse(TEXT("Absent purchase cannot carry a mod identity"),Valid(7,true,false,false,false,1));
    TestTrue(TEXT("Quota guidance now correctly precedes rescue"),Valid(7,true,false,false,false,-1));
    TestTrue(TEXT("Purchase guidance now correctly precedes rescue"),Valid(8,true,false,false,false,-1));
    TestTrue(TEXT("Using a paid mod now correctly precedes rescue"),Valid(9,true,false,false,true,2));
    TestTrue(TEXT("Old held Release save remains loadable"),Valid(2,true,false,true,false,-1));
    TestTrue(TEXT("Old pre-purchase Precision save remains loadable for reconciliation"),Valid(6,true,true,false,false,-1));
    return true;
}
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialEarningTest,"MagnetSweep.Tutorial.SafeFirstPayoutAndCaptureGuard",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialEarningTest::RunTest(const FString& Parameters)
{
    for(auto Step:{ETutorialStep::Attract,ETutorialStep::Release,ETutorialStep::Bundle,ETutorialStep::FirstSmelt})
    for(bool Mixed:{false,true})
    {
        FWorkbenchImpl R(nullptr); R.bMuted=true; R.Tutorial.Start(); R.Tutorial.Step=Step;
        R.Model.CapturePieces(Mixed?TArray<int32>{0,24}:TArray<int32>{0});
        const int32 Expected=Mixed?40:4;
        TestEqual(TEXT("Fixture contains actual iron or iron plus linked copper"),R.Model.GetCargo(),Expected);
        R.Deposit(false);
        TestEqual(TEXT("Real Deposit pays every safe material mix from every early lesson"),R.Model.GetBanked(),Expected);
        TestEqual(TEXT("The actual first payout reaches the wallet with no tutorial award"),R.Model.GetWallet(),Expected);
        TestEqual(TEXT("The actual first payout reaches XP with no fabricated lesson reward"),R.Model.GetXP(),Expected);
        TestEqual(TEXT("Even a tiny accepted pour spends exactly one advertised charge"),R.Model.GetHeatsRemaining(),3);
        // Simulate reopening a saved committed pour: the real ledger is authoritative,
        // even if the original animation's settled callback was never delivered.
        FWorkbenchImpl Reloaded(nullptr); Reloaded.bMuted=true;
        TestTrue(TEXT("Every early committed payout remains saveable"),Reloaded.DecodeSave(R.EncodeSave()));
        Reloaded.UpdateTutorial();
        TestTrue(TEXT("Saved real payout resumes earning guidance without another pickup"),Reloaded.Tutorial.Step==ETutorialStep::Quota);
        TestTrue(TEXT("Early banking retains training protection until an actual purchase"),Reloaded.Tutorial.IsTrainingGuardActive());
    }
    FWorkbenchImpl Guard(nullptr); Guard.bMuted=true; Guard.Tutorial.Start(); Guard.Tutorial.Begin();
    TestFalse(TEXT("Visible training guard blocks actual hot-cell attachment"),Guard.CanCaptureForPlayer(35));
    TestTrue(TEXT("The novice can take a legal connected copper bundle"),Guard.CanCaptureForPlayer(24));
    Guard.Action=EMagnetAction::Sweep; Guard.bFieldLatched=true;
    Guard.OnRecovery(Guard.Model.CapturePieces({0}),false,1);
    TestTrue(TEXT("The first useful piece exposes smelting without interrupting a larger sweep"),
        Guard.Tutorial.Step==ETutorialStep::FirstSmelt && Guard.Action==EMagnetAction::Sweep && Guard.bFieldLatched);
    TArray<int32> MoreIron; for(int32 Id=1;Id<8;++Id)MoreIron.Add(Id);
    Guard.OnRecovery(Guard.Model.CapturePieces(MoreIron),false,MoreIron.Num());
    TestEqual(TEXT("Guard boundary fixture holds sixteen kg"),Guard.Model.GetCargoMass(),16);
    TestTrue(TEXT("The normal model would allow the additional nine-kg bundle"),Guard.Model.CanCaptureGroup(24));
    TestFalse(TEXT("The novice guard rejects the whole linked bundle above safe capacity"),Guard.CanCaptureForPlayer(24));
    TestTrue(TEXT("Rejected guarded bundle stays fully available"),
        Guard.Model.FindPiece(24)->State==EPieceState::Available && Guard.Model.FindPiece(25)->State==EPieceState::Available
        && Guard.Model.FindPiece(26)->State==EPieceState::Available);
    Guard.OnRecovery(Guard.Model.CapturePieces({8,9,10,11}),false,4);
    TestEqual(TEXT("A novice haul can reach the actual safe capacity"),Guard.Model.GetCargoMass(),24);
    TestFalse(TEXT("Safe capacity blocks more novice iron instead of overloading"),Guard.CanCaptureForPlayer(12));
    TestTrue(TEXT("Full safe practice haul visibly stops the field with no danger"),
        Guard.Action==EMagnetAction::None && !Guard.bFieldLatched && !Guard.Model.IsCargoUnsafe());
    TestEqual(TEXT("Protected collection consumed no fuel"),Guard.Model.GetHeatsRemaining(),4);
    FWorkbenchImpl Purchased(nullptr); Purchased.bMuted=true;
    TestTrue(TEXT("Unlock fixture actually earns and buys its first rig mod"),EarnCoil(Purchased));
    TestFalse(TEXT("Real purchase ends training protection"),Purchased.Tutorial.IsTrainingGuardActive());
    TestTrue(TEXT("Ordinary hot-cell risk is available after the actual purchase"),Purchased.CanCaptureForPlayer(35));
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialDragReleaseTest,"MagnetSweep.Tutorial.NaturalDragReleasePayout",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialDragReleaseTest::RunTest(const FString& Parameters)
{
    // Tray, world furnace, enabled SMELT HAUL button, unrelated enabled UI.
    for(int32 Target=0;Target<4;++Target)
    {
        FWorkbenchImpl R(nullptr); R.bMuted=true;
        R.Tutorial.Start(); R.Tutorial.Step=ETutorialStep::Bundle;
        R.Model.CapturePieces({0,24}); // A real safe 40-credit mixed haul.
        R.bHaulDrag=true; R.bFurnaceHover=Target==1;
        if(Target>=2)
        {
            FDemoButton Button;
            Button.Id=Target==2?40:41; Button.Enabled=true;
            Button.Rect=FBox2D(FVector2D(100,100),FVector2D(200,140));
            R.Buttons.Add(Button); R.Pointer=FVector2D(150,120);
            TestEqual(TEXT("UI release uses an actual enabled button hit"),R.HitButton(),Button.Id);
        }
        const bool ShouldBank=Target==1 || Target==2;
        R.Action=EMagnetAction::None; // The training guard already stopped attraction.
        R.HandleRelease();
        TestFalse(TEXT("Actual release consumes the original tray-drag intent"),R.bHaulDrag);
        TestEqual(TEXT("World furnace or SMELT HAUL release pays; tray or unrelated UI retains cargo"),
            R.Model.GetBanked(),ShouldBank?40:0);
        TestEqual(TEXT("Release away from either smelt target retains every cargo piece"),R.Model.GetCargo(),ShouldBank?0:40);
        TestEqual(TEXT("Only an accepted smelt release awards real wallet income"),R.Model.GetWallet(),ShouldBank?40:0);
        TestEqual(TEXT("Only an accepted smelt release spends a charge"),R.Model.GetHeatsRemaining(),ShouldBank?3:4);
        const int32 Banked=R.Model.GetBanked();
        R.HandleRelease();
        TestEqual(TEXT("Repeating release without another tray drag cannot credit twice"),R.Model.GetBanked(),Banked);
    }
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FTutorialRuntimeTest,"MagnetSweep.Tutorial.RuntimeRescueAndSkipCallbacks",
    EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FTutorialRuntimeTest::RunTest(const FString& Parameters)
{
    // Real runtime callbacks, with no world/scene/audio required. Tick itself needs a
    // player controller and is intentionally not represented as a rendered/native test.
    FWorkbenchImpl Runtime(nullptr); Runtime.bMuted=true;
    TestTrue(TEXT("Runtime fixture earns and purchases its real first coil"),EarnCoil(Runtime));
    Runtime.OnRecovery(Runtime.Model.CapturePieces({30}),false,1);
    TestTrue(TEXT("Actual upgraded recovery reaches the later risk rehearsal"),Runtime.Tutorial.Step==ETutorialStep::Risk);
    const int32 Banked=Runtime.Model.GetBanked(), Wallet=Runtime.Model.GetWallet(), XP=Runtime.Model.GetXP();
    Runtime.Action=EMagnetAction::Sweep; Runtime.bFieldLatched=true;
    Runtime.OnRecovery(Runtime.Model.CapturePieces({35}),false,1);
    TestTrue(TEXT("Actual hot-cell recovery establishes the first safety hold before risk advancement"),
        Runtime.Model.IsCargoUnsafe() && Runtime.Tutorial.bRiskHeld && !Runtime.Tutorial.bRiskLearned);
    TestTrue(TEXT("Held danger stops the actual runtime field and its toggle"),
        Runtime.Action==EMagnetAction::None && !Runtime.bFieldLatched);
    TestEqual(TEXT("OnRecovery has not spent the protected first fuse"),Runtime.Model.GetFuseElapsed(),0.f);
    TestEqual(TEXT("First warning has not burned an extra fuel charge"),Runtime.Model.GetHeatsRemaining(),1);
    Runtime.UpdateTutorial();
    TestTrue(TEXT("Runtime reconciliation preserves the first warning hold"),Runtime.Tutorial.bRiskHeld);
    Runtime.Vent();
    TestEqual(TEXT("Actual runtime drop empties the model haul"),Runtime.Model.GetCargoMass(),0);
    TestTrue(TEXT("Actual runtime drop clears the hold and records the rescue"),
        !Runtime.Tutorial.bRiskHeld && Runtime.Tutorial.bRiskLearned);
    TestTrue(TEXT("Dropping after actual upgrade use leads to later precision practice"),Runtime.Tutorial.Step==ETutorialStep::Precision);
    TestTrue(TEXT("Actual spill callback generated feedback particles"),!Runtime.Particles.IsEmpty());
    TestEqual(TEXT("Guided rescue preserves previously banked material"),Runtime.Model.GetBanked(),Banked);
    TestEqual(TEXT("Guided rescue preserves previously earned money"),Runtime.Model.GetWallet(),Wallet);
    TestEqual(TEXT("Guided rescue preserves previously earned XP"),Runtime.Model.GetXP(),XP);
    TestEqual(TEXT("Guided rescue preserves fuel"),Runtime.Model.GetHeatsRemaining(),1);
    TestEqual(TEXT("Guided rescue preserves the actually purchased upgrade"),Runtime.Model.GetUpgradeTier(EUpgrade::Coil),1);

    FWorkbenchImpl SkipRuntime(nullptr); SkipRuntime.bMuted=true;
    // A legacy save may contain an already-held dangerous load before the new
    // training guard. Its Skip action must still perform an honest safe rescue.
    SkipRuntime.Tutorial.Start(); SkipRuntime.Tutorial.Step=ETutorialStep::Release;
    SkipRuntime.Model.CapturePieces({35}); SkipRuntime.Tutorial.HoldFirstRisk(true);
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
    Writer.Tutorial.Start(); Writer.Tutorial.Step=ETutorialStep::Release;
    Writer.Model.CapturePieces({35}); Writer.Tutorial.HoldFirstRisk(true);
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
