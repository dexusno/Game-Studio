#pragma once
#if FOUNDRY_WITH_CAMPAIGN
#include "CoreMinimal.h"
#include "FoundrySession.h"
#include "overkill/campaign_session.hpp"
#include "overkill/upgrades.hpp"

// View state is deliberately separate from the committed campaign envelope.
// All durable commands use the current run ID and next monotonic sequence.
class FFoundryCampaign
{
public:
    FFoundryCampaign(FFoundrySession& InCombat, const FString& InSavePath);
    ~FFoundryCampaign();
    FFoundrySession& Combat;
    overkill::CampaignHooks Hooks;
    overkill::CampaignRules Rules;
    overkill::CampaignSession Store;
    FString Page = TEXT("title"), ReturnPage, Drawer, Message;
    bool bDiagnostic = false, bSaveUnavailable = false, bCanResumeInMemory = false;
    bool bPrecisionModal = false;
    uint64 PrecisionAttempt = 0;
    overkill::Id PrecisionChoice = 0;
    int32 PrecisionSteering = 0, PrecisionResult = -1;
    uint64 ViewRevision = 1, SceneRevision = 0;
    overkill::Id FocusRecipe = 0;
    FString SavePath;

    const overkill::Campaign* Current() const;
    bool HasActiveSave() const;
    bool StartNew(bool bConfirmed = false, uint64 Seed = 0);
    bool Continue();
    bool Apply(overkill::CampaignAction Action);
    bool Control(const FString& Command);
    bool BeginPrecision(overkill::Id RetryChoice = 0);
    void CancelPrecision(uint64 Attempt);
    bool FinishPrecision(uint64 Attempt, int32 Result);
    overkill::Action PrecisionAction(int32 Result) const;
    void LeaveFailedPrecision();
    void Navigate(const FString& Destination);
    void Close();
    FString ItemName(const std::string& Id, bool bUpgrade = false) const;
    FString ItemDescription(const std::string& Id, bool bUpgrade = false) const;
    FString CopyName(overkill::Id Id) const;
    FString PhasePage() const;
private:
    bool Handle(const overkill::SessionResult& Result);
    void ReadCommitted();
};
#endif
