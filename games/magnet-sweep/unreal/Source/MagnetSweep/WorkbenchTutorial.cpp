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
bool FTutorialProgress::ObserveCapture(bool bLinked, bool bPrecision, bool bUseful, int32 CargoMass)
{
    if (!bEnabled || CargoMass <= 0) return false;
    // Exactly one transition per actual capture, even if it satisfies later lessons too.
    switch (Step)
    {
    case ETutorialStep::Attract:
        if (CargoMass >= 4) { Step = ETutorialStep::Release; return true; }
        break;
    case ETutorialStep::Bundle:
        if (bLinked) { Step = ETutorialStep::FirstSmelt; return true; }
        break;
    case ETutorialStep::Precision:
        if (bPrecision && bUseful) { Step = ETutorialStep::Quota; return true; }
        break;
    case ETutorialStep::TryUpgrade:
        if (bUseful && bPurchasedMod) return Finish();
        break;
    default: break;
    }
    return false;
}
bool FTutorialProgress::ObserveFieldOff()
{
    if (!bEnabled || Step != ETutorialStep::Release) return false;
    Step = ETutorialStep::Bundle; return true;
}
bool FTutorialProgress::ObserveSmelt(bool bQuotaMet)
{
    if (!bEnabled) return false;
    if (Step == ETutorialStep::FirstSmelt)
    {
        Step = bRiskLearned ? ETutorialStep::Precision : ETutorialStep::Risk;
        return true;
    }
    if (Step == ETutorialStep::Quota && bQuotaMet)
    {
        Step = bPurchasedMod ? ETutorialStep::TryUpgrade : ETutorialStep::Upgrade;
        return true;
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
    if (!bEnabled || !bChanged || !bRiskHeld) return false;
    bRiskLearned = true; bRiskHeld = false;
    if (Step == ETutorialStep::Risk) Step = ETutorialStep::Precision;
    // Dropping cargo did not demonstrate that releasing the field retains it.
    else if (Step == ETutorialStep::Release) Step = ETutorialStep::Attract;
    return true;
}
bool FTutorialProgress::ObservePurchase(int32 Kind)
{
    if (!bEnabled || Kind < 0 || Kind > 2) return false;
    const bool Changed = !bPurchasedMod || PurchasedMod != Kind || Step == ETutorialStep::Upgrade;
    bPurchasedMod = true; PurchasedMod = Kind;
    if (Step == ETutorialStep::Upgrade) Step = ETutorialStep::TryUpgrade;
    return Changed;
}
bool FTutorialProgress::Reconcile(bool bQuotaMet, bool bHasAvailableBundle, bool bHasUsefulAvailable)
{
    if (!bEnabled || bRiskHeld) return false;
    // Availability alone cannot prove a useful precision capture or use of a new mod.
    // NeedsFreshTray exposes exhausted capture lessons instead of silently skipping them.
    (void)bHasUsefulAvailable;
    if (Step == ETutorialStep::Bundle && !bHasAvailableBundle)
    {
        Step = ETutorialStep::FirstSmelt; return true;
    }
    if (Step == ETutorialStep::FirstSmelt && bQuotaMet)
    {
        // A met quota is authoritative evidence of earlier banking during off-path play.
        Step = bRiskLearned ? ETutorialStep::Precision : ETutorialStep::Risk; return true;
    }
    if (Step == ETutorialStep::Quota && bQuotaMet)
    {
        Step = bPurchasedMod ? ETutorialStep::TryUpgrade : ETutorialStep::Upgrade; return true;
    }
    if (Step == ETutorialStep::Upgrade && bPurchasedMod)
    {
        Step = ETutorialStep::TryUpgrade; return true;
    }
    return false;
}
bool FTutorialProgress::NeedsFreshTray(bool bHasUsefulAvailable) const
{
    if (!bEnabled || bRiskHeld || bHasUsefulAvailable) return false;
    return Step == ETutorialStep::Attract || Step == ETutorialStep::Bundle
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
    if (Enabled && SavedStep >= ETutorialStep::Precision && SavedStep <= ETutorialStep::TryUpgrade && !Learned)
        return Fail(TEXT("Later lessons require the recorded rescue event."));
    if (SavedStep == ETutorialStep::TryUpgrade && !Purchased)
        return Fail(TEXT("Upgrade practice requires a recorded purchase."));
    Reason.Reset(); return true;
}
}
