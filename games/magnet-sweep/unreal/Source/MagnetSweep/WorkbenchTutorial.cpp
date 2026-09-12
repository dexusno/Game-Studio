#include "WorkbenchTutorial.h"

namespace MagnetSweep
{
namespace
{
bool IsTerminal(ETutorialStep Step)
{
    return Step == ETutorialStep::Complete || Step == ETutorialStep::Skipped;
}
bool SetTerminal(FTutorialProgress& Progress, ETutorialStep Terminal)
{
    // Explicit Start is required to overwrite a previously completed/skipped record.
    const auto Next = IsTerminal(Progress.Step) ? Progress.Step : Terminal;
    const bool Changed = Progress.bEnabled || Progress.bRiskHeld || Progress.Step != Next;
    Progress.Step = Next; Progress.bEnabled = false; Progress.bRiskHeld = false;
    return Changed;
}
}

bool FTutorialProgress::Start()
{
    const bool Changed = !bEnabled || Step != ETutorialStep::Welcome || bRiskLearned
        || bRiskHeld || bPurchasedMod || PurchasedMod != INDEX_NONE;
    Step = ETutorialStep::Welcome; bEnabled = true; bRiskLearned = bRiskHeld = bPurchasedMod = false;
    PurchasedMod = INDEX_NONE;
    return Changed;
}
bool FTutorialProgress::Begin()
{
    if (!bEnabled || Step != ETutorialStep::Welcome) return false;
    Step = ETutorialStep::Attract; return true;
}
bool FTutorialProgress::IsTrainingGuardActive() const
{
    return bEnabled && !bPurchasedMod;
}
bool FTutorialProgress::ObserveCapture(bool bLinked, bool bPrecision, bool bUseful, int32 CargoMass, bool bUsedBreakaway)
{
    if (!bEnabled || bRiskHeld || CargoMass <= 0) return false;
    (void)bLinked;
    // Exactly one transition per actual capture, even if it satisfies later lessons too.
    switch (Step)
    {
    case ETutorialStep::Attract:
    case ETutorialStep::Release:
    case ETutorialStep::Bundle:
        if (bUseful) { Step = ETutorialStep::FirstSmelt; return true; }
        break;
    case ETutorialStep::Precision:
        if (bPrecision && bUseful && bPurchasedMod) return Finish();
        break;
    case ETutorialStep::TryUpgrade:
        if (bUseful && bPurchasedMod && (PurchasedMod != 1 || bUsedBreakaway))
        {
            Step = bRiskLearned ? ETutorialStep::Precision : ETutorialStep::Risk;
            return true;
        }
        break;
    default: break;
    }
    return false;
}
bool FTutorialProgress::ObserveFieldOff()
{
    if (!bEnabled || bRiskHeld || Step != ETutorialStep::Release) return false;
    Step = ETutorialStep::FirstSmelt; return true;
}
bool FTutorialProgress::ObserveSmelt(bool bQuotaMet)
{
    if (!bEnabled || bRiskHeld) return false;
    if (Step == ETutorialStep::Welcome || Step == ETutorialStep::Attract
        || Step == ETutorialStep::Release || Step == ETutorialStep::Bundle
        || Step == ETutorialStep::FirstSmelt || Step == ETutorialStep::Quota
        || ((!bPurchasedMod) && (Step == ETutorialStep::Risk || Step == ETutorialStep::Precision)))
    {
        const auto Next = bPurchasedMod ? ETutorialStep::TryUpgrade
            : (bQuotaMet ? ETutorialStep::Upgrade : ETutorialStep::Quota);
        const bool Changed = Step != Next; Step = Next; return Changed;
    }
    return false;
}
bool FTutorialProgress::HoldFirstRisk(bool bUnsafe)
{
    if (!bEnabled || bRiskLearned) return false;
    if (!bUnsafe)
    {
        // A restocked/retried board cannot retain a hold on cargo which no longer
        // exists. This clears stale state without claiming the rescue was learned.
        if (bRiskHeld) { bRiskHeld = false; return true; }
        return false;
    }
    if (bRiskHeld) return false;
    bRiskHeld = true; return true;
}
bool FTutorialProgress::ObserveDrop(bool bChanged)
{
    if (!bEnabled || !bChanged) return false;
    const bool WasHeld = bRiskHeld;
    const auto Before = Step;
    if (WasHeld) { bRiskLearned = true; bRiskHeld = false; }
    if (Step == ETutorialStep::Release || Step == ETutorialStep::Bundle || Step == ETutorialStep::FirstSmelt)
        Step = ETutorialStep::Attract;
    else if (Step == ETutorialStep::Risk && WasHeld)
        Step = bPurchasedMod ? ETutorialStep::Precision : ETutorialStep::Quota;
    return WasHeld || Before != Step;
}
bool FTutorialProgress::ObservePurchase(int32 Kind)
{
    if (!bEnabled || Kind < 0 || Kind > 2) return false;
    const bool Changed = !bPurchasedMod || PurchasedMod != Kind;
    const auto Before = Step;
    bPurchasedMod = true; PurchasedMod = Kind;
    // A real purchase is authoritative even when the player used the shop off-path.
    if (!bRiskHeld) Step = ETutorialStep::TryUpgrade;
    return Changed || Before != Step;
}
bool FTutorialProgress::Reconcile(bool bQuotaMet, bool bHasAvailableBundle, bool bHasUsefulAvailable, int32 CargoValue)
{
    if (!bEnabled || bRiskHeld) return false;
    (void)bHasAvailableBundle; (void)bHasUsefulAvailable;
    const auto Before = Step;
    if (Step == ETutorialStep::Attract && CargoValue > 0) Step = ETutorialStep::FirstSmelt;
    if (Step == ETutorialStep::Release || Step == ETutorialStep::Bundle)
        Step = ETutorialStep::FirstSmelt;
    // Old tutorials taught risk before rewards. Preserve their real rescue flags,
    // but route unfinished players back to earning their first upgrade.
    if (!bPurchasedMod && (Step == ETutorialStep::Risk || Step == ETutorialStep::Precision))
        Step = bQuotaMet ? ETutorialStep::Upgrade : ETutorialStep::Quota;
    if (Step == ETutorialStep::Attract || Step == ETutorialStep::FirstSmelt
        || Step == ETutorialStep::Quota || Step == ETutorialStep::Upgrade)
    {
        if (bPurchasedMod) Step = ETutorialStep::TryUpgrade;
        else if (bQuotaMet) Step = ETutorialStep::Upgrade;
    }
    return Before != Step;
}
bool FTutorialProgress::NeedsFreshTray(bool bHasUsefulAvailable) const
{
    if (!bEnabled || bRiskHeld || bHasUsefulAvailable) return false;
    return Step == ETutorialStep::Attract || Step == ETutorialStep::Release
        || Step == ETutorialStep::Bundle || Step == ETutorialStep::FirstSmelt
        || Step == ETutorialStep::Quota || Step == ETutorialStep::Risk
        || Step == ETutorialStep::Precision || Step == ETutorialStep::TryUpgrade;
}
bool FTutorialProgress::Skip()
{
    // Runtime must really drop a held haul first, then report ObserveDrop. Otherwise
    // skipping would silently remove the safety hold while leaving dangerous cargo.
    if (bRiskHeld) return false;
    return SetTerminal(*this, ETutorialStep::Skipped);
}
bool FTutorialProgress::Finish()
{
    if (bRiskHeld) return false;
    return SetTerminal(*this, ETutorialStep::Complete);
}
bool FTutorialProgress::Validate(FString& Reason) const
{
    return ValidateFields(static_cast<int32>(Step), bEnabled, bRiskLearned, bRiskHeld,
        bPurchasedMod, PurchasedMod, Reason);
}
bool FTutorialProgress::ValidateFields(int32 StepValue, bool Enabled, bool Learned,
    bool Held, bool Purchased, int32 Mod, FString& Reason)
{
    const auto Fail = [&Reason](const TCHAR* Text) { Reason = Text; return false; };
    if (StepValue < static_cast<int32>(ETutorialStep::Welcome) || StepValue > static_cast<int32>(ETutorialStep::Skipped))
        return Fail(TEXT("Invalid tutorial step."));
    const auto SavedStep = static_cast<ETutorialStep>(StepValue);
    if ((Purchased && (Mod < 0 || Mod > 2)) || (!Purchased && Mod != INDEX_NONE))
        return Fail(TEXT("Tutorial purchase identity does not match its recorded event."));
    if (Held && (!Enabled || Learned || IsTerminal(SavedStep)))
        return Fail(TEXT("Invalid first-risk teaching hold."));
    if (IsTerminal(SavedStep) && Enabled) return Fail(TEXT("Terminal tutorial cannot remain enabled."));
    if (!Enabled && !IsTerminal(SavedStep) && SavedStep != ETutorialStep::Welcome)
        return Fail(TEXT("An in-progress tutorial cannot be silently disabled."));
    if (Enabled && SavedStep == ETutorialStep::Precision && !Learned)
        return Fail(TEXT("Precision rehearsal requires the recorded rescue event."));
    if (SavedStep == ETutorialStep::TryUpgrade && !Purchased)
        return Fail(TEXT("Upgrade practice requires a recorded purchase."));
    Reason.Reset(); return true;
}
}
