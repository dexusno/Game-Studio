#pragma once
#include "CoreMinimal.h"

namespace MagnetSweep
{
enum class ETutorialStep : uint8
{
    // Save IDs are permanent. Release/Bundle survive only for old-save recovery.
    Welcome = 0, Attract = 1, Release = 2, Bundle = 3, FirstSmelt = 4,
    Risk = 5, Precision = 6, Quota = 7, Upgrade = 8, TryUpgrade = 9,
    Complete = 10, Skipped = 11
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
    // Runtime visibly limits capture to safe capacity and excludes hot cells until
    // an actual purchase. This helper never changes the model's normal rules.
    bool IsTrainingGuardActive() const;
    bool ObserveCapture(bool bLinked, bool bPrecision, bool bUseful, int32 CargoMass);
    // Legacy release recovery only. Field release and bundles are contextual tips.
    bool ObserveFieldOff();
    // Call only for a settled real payout. Any early step accepts every material mix.
    bool ObserveSmelt(bool bQuotaMet);
    // Runtime must stop attraction and suspend the fuse while this hold is active;
    // dropping remains available. Only an observed drop teaches rescue; false clears
    // a stale hold after restocking without marking the rescue learned.
    bool HoldFirstRisk(bool bUnsafe);
    bool ObserveDrop(bool bChanged);
    bool ObservePurchase(int32 Kind);
    // Reconcile legacy prerequisites and authoritative quota/purchase evidence. It
    // never invents precision, rescue or post-purchase use from board availability.
    bool Reconcile(bool bQuotaMet, bool bHasAvailableBundle, bool bHasUsefulAvailable, int32 CargoValue = 0);
    bool NeedsFreshTray(bool bHasUsefulAvailable) const;
    bool Skip();
    bool Finish();

    bool Validate(FString& Reason) const;
    static bool ValidateFields(int32 StepValue, bool bEnabled, bool bRiskLearned,
        bool bRiskHeld, bool bPurchasedMod, int32 PurchasedMod, FString& Reason);
};
}
