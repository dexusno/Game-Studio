#pragma once

#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

namespace MagnetSweep
{
enum class EExpeditionModuleKind : uint8 { Active, Passive };

struct FExpeditionModule
{
    FName Id;
    FString Name;
    FString Description;
    EExpeditionModuleKind Kind = EExpeditionModuleKind::Passive;
    int32 Price = 0;
    int32 Slots = 1;
    TArray<FName> Provides;
    TArray<FName> RequiresAll;
    TArray<FName> RequiresAny;
    TArray<FName> OpportunityTags; // All must exist in the actual next worksite.
};

struct FExpeditionOwnedModule
{
    FName Id;
    int32 Paid = 0; // Actual basis: free starter/found equipment resells for zero.
};

struct FExpeditionRigRecords
{
    int32 RunsStarted = 0;
    int32 RunsWon = 0;
    TArray<int32> BestOutput = {0, 0, 0, 0};
    TArray<FName> DiscoveredModules;
    TArray<FName> UnlockedStarters;
    TArray<FName> LastWinningRig;
};

// Pure rig/economy state. Runtime owns the atomic world+rig save/checkpoint.
// Catalogue descriptions do not implement effects: shop context excludes modules
// which the current world implementation or upcoming worksite cannot support.
class FExpeditionRig
{
public:
    static constexpr int32 SiteCount = 4;
    static constexpr int32 ActiveSlots = 2;
    static constexpr int32 PassiveSlots = 4;
    static constexpr int32 PrechargePrice = 4;

    FExpeditionRig();
    static const TArray<FExpeditionModule>& Catalog();
    static const FExpeditionModule* FindModule(FName Id);
    static const TArray<FName>& InitialStarters();
    static int32 RefiningThreshold(int32 SiteIndex, int32 MilestoneIndex);

    bool NewRun(FName Starter, int32 Seed);
    bool IsStarterUnlocked(FName Id) const;
    // Set real opportunities/implemented IDs before NewRun or NextDepot. On the
    // first context after NewRun, GenerateOffers may fill the still-empty shop.
    // Existing generated stock is never rerolled by this call.
    void SetShopContext(const TArray<FName>& OpportunityTags, const TArray<FName>& ImplementedModules);
    bool GenerateOffers(FString& Reason);
    bool NextDepot(FString& Reason);
    bool Depart(FString& Reason);

    bool CanBuy(FName Id, FString& Reason) const;
    bool Buy(FName Id, FString& Reason);
    bool CanFit(FName Id, FString& Reason) const;
    bool Fit(FName Id, FString& Reason);
    bool Unfit(FName Id, FString& Reason);
    bool Sell(FName Id, FString& Reason);
    bool Precharge(FString& Reason);
    bool CanDepart(FString& Reason) const;

    // World reports committed, monotonically increasing output and actual site
    // dispatch. Index is zero-based; repeated awards pay nothing.
    int32 AwardOutput(int32 SiteIndex, int32 TotalOutput);
    int32 AwardSite(int32 SiteIndex);
    bool Has(FName Id) const;
    bool Owns(FName Id) const;
    bool HasCapability(FName Capability) const;
    bool IsCompatible(FName Id) const;
    bool HasOpportunity(FName Id) const;
    bool IsImplemented(FName Id) const;
    int32 GetResaleValue(FName Id) const;
    int32 GetUsedSlots(EExpeditionModuleKind Kind) const;

    int32 GetCash() const { return Cash; }
    int32 GetWallet() const { return Cash; }
    int32 GetSeed() const { return Seed; }
    int32 GetDepotIndex() const { return DepotIndex; }
    int32 GetSiteIndex() const { return DepotIndex; }
    bool IsAtDepot() const { return bAtDepot; }
    bool IsRunWon() const { return bRunWon; }
    bool IsSiteAwarded(int32 Index) const { return SiteAwarded.IsValidIndex(Index) && SiteAwarded[Index]; }
    int32 GetOutput(int32 Index) const { return Output.IsValidIndex(Index) ? Output[Index] : 0; }
    float GetStartingBattery() const { return bPrecharged ? 120.f : 100.f; }
    bool IsPrecharged() const { return bPrecharged; }
    bool HasVerifiedShopPair() const { return bShopPairVerified; }
    const FString& GetShopNotice() const { return ShopNotice; }
    const TArray<FName>& GetOffers() const { return Offers; }
    const TArray<FName>& GetFittedActives() const { return FittedActives; }
    const TArray<FName>& GetFittedPassives() const { return FittedPassives; }
    const TArray<FExpeditionOwnedModule>& GetInventory() const { return Inventory; }
    const FExpeditionRigRecords& GetRecords() const { return Records; }

    TSharedRef<FJsonObject> ToJson() const;
    bool FromJson(const FJsonObject& Json, FString& Reason); // Transactional: rejection leaves this unchanged.
    bool Validate(FString& Reason) const;

private:
    int32 Cash = 12;
    int32 Seed = 1;
    int32 DepotIndex = 0;
    int32 TotalPurchased = 0;
    int32 TotalRefunded = 0;
    FName Starter;
    bool bAtDepot = true;
    bool bRunWon = false;
    bool bPrecharged = false;
    bool bContextSet = false;
    bool bOffersGenerated = false;
    bool bShopPairVerified = false;
    FString ShopNotice;
    TArray<FExpeditionOwnedModule> Inventory;
    TArray<FName> FittedActives;
    TArray<FName> FittedPassives;
    TArray<FName> Offers;
    TArray<FName> GeneratedStock;
    TArray<FName> OpportunityContext;
    TArray<FName> ImplementedContext;
    TArray<int32> Output = {0, 0, 0, 0};
    TArray<bool> SiteAwarded = {false, false, false, false};
    TArray<int32> RefiningPaid = {0, 0, 0, 0};
    TArray<bool> PrechargeBought = {false, false, false, false};
    FExpeditionRigRecords Records;

    bool CompatibleWith(FName Id, const TArray<FName>& Actives) const;
    bool CanUseAfterRefit(FName Id) const;
    bool AtShop(FString& Reason) const;
    void Discover(FName Id);
};
}
