#pragma once
#include "CoreMinimal.h"

namespace MagnetSweep
{
enum class ETutorialStep : uint8
{
    Welcome, Attract, Release, Bundle, FirstSmelt, Risk, Precision,
    Quota, Upgrade, TryUpgrade, Complete, Skipped
};

// Pure action-driven lesson state. Runtime owns input, danger-clock suspension,
// board restocking, save routing and text. No lesson awards or removes career rewards.
struct FTutorialProgress
{
    ETutorialStep Step = ETutorialStep::Welcome;
    bool bEnabled = false;
    bool bRiskLearned = false;
    bool bRiskHeld = false;
    bool bPurchasedMod = false;
    int32 PurchasedMod = INDEX_NONE;

    bool Start();
    bool Begin();
    bool ObserveCapture(bool bLinked, bool bPrecision, bool bUseful, int32 CargoMass);
    // Call only on an actual player release / field-toggle-off, never an automatic teaching stop.
    bool ObserveFieldOff();
    bool ObserveSmelt(bool bQuotaMet);
    // Runtime must stop attraction and suspend the fuse while this hold is active;
    // dropping remains available. Only an observed drop teaches rescue; false clears
    // a stale hold after restocking without marking the rescue learned.
    bool HoldFirstRisk(bool bUnsafe);
    bool ObserveDrop(bool bChanged);
    bool ObservePurchase(int32 Kind);
    // Reconcile unavailable bundle practice / already-met quota only. It never invents
    // a successful precision capture or post-purchase action from board availability.
    bool Reconcile(bool bQuotaMet, bool bHasAvailableBundle, bool bHasUsefulAvailable);
    bool NeedsFreshTray(bool bHasUsefulAvailable) const;
    bool Skip();
    bool Finish();

    bool Validate(FString& Reason) const;
    static bool ValidateFields(int32 StepValue, bool bEnabled, bool bRiskLearned,
        bool bRiskHeld, bool bPurchasedMod, int32 PurchasedMod, FString& Reason);
};
}
