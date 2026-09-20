#pragma once

#include "CoreMinimal.h"
#include "overkill/core.hpp"

// Presentation selection and a read model around the one authoritative rules core.
// UI selection is reversible; only Rules::apply changes combat state.
class FFoundrySession
{
public:
    explicit FFoundrySession(uint64 InSeed);
    overkill::Rules Rules;
    overkill::State State;
    std::vector<overkill::Id> Selection;
    TMap<uint64, uint64> SpreadTargets;
    uint64 Target = 0;
    int32 Steering = 3;
    int32 Precision = -1;
    int32 InventoryPage = 0;
    bool bShowPanels = true;
    FString Message;
    TArray<FString> RecentEvents;
    FString LastAction;
    // Consumed once by the cosmetic adapter; previews never append here.
    std::vector<overkill::Event> CommittedEvents;

    void Restart();
    bool Submit(const overkill::Action& Action, const FString& Description);
    bool Control(const FString& Command);
    overkill::Action FireAction() const;
    overkill::Preview Preview(const overkill::Action& Action) const;
    FString PreviewText(const overkill::Action& Action) const;
    FString DescribePart(const overkill::Part& Part) const;
    FString NameFor(uint64 Id) const;
    void FixSelection();
    bool RunInputProbe(FString& Report);
private:
    uint64 Seed = 1;
};
