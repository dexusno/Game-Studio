#include "ExpeditionRig.h"
#include "Dom/JsonValue.h"
#include "Math/RandomStream.h"
#include <initializer_list>

namespace MagnetSweep
{
namespace
{
constexpr int32 SaveVersion = 1;
constexpr int32 MaxLedger = 10000000;
constexpr int32 MaxOutput = 1000000;

TArray<FName> Names(std::initializer_list<const TCHAR*> Values)
{
    TArray<FName> Result;
    for (const TCHAR* Value : Values) Result.Add(FName(Value));
    return Result;
}

bool Refuse(FString& Reason, const TCHAR* Text)
{
    Reason = Text;
    return false;
}

bool ReadInt(const FJsonObject& Json, const TCHAR* Key, int32 Low, int32 High, int32& Out)
{
    double Number = 0;
    if (!Json.TryGetNumberField(Key, Number) || !FMath::IsFinite(Number)
        || Number < Low || Number > High || Number != FMath::FloorToDouble(Number)) return false;
    Out = static_cast<int32>(Number);
    return true;
}

void PutNames(FJsonObject& Json, const TCHAR* Key, const TArray<FName>& Values)
{
    TArray<TSharedPtr<FJsonValue>> Array;
    for (FName Value : Values) Array.Add(MakeShared<FJsonValueString>(Value.ToString()));
    Json.SetArrayField(Key, Array);
}

bool ReadNames(const FJsonObject& Json, const TCHAR* Key, TArray<FName>& Out, int32 Maximum = 64)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Json.TryGetArrayField(Key, Values) || Values->Num() > Maximum) return false;
    Out.Reset();
    for (const auto& Value : *Values)
    {
        if (!Value.IsValid() || Value->Type != EJson::String) return false;
        const FString Name = Value->AsString();
        if (Name.IsEmpty() || Name.Len() > 80) return false;
        const FName Id(*Name);
        if (Id.IsNone() || Out.Contains(Id)) return false;
        Out.Add(Id);
    }
    return true;
}

void PutInts(FJsonObject& Json, const TCHAR* Key, const TArray<int32>& Values)
{
    TArray<TSharedPtr<FJsonValue>> Array;
    for (int32 Value : Values) Array.Add(MakeShared<FJsonValueNumber>(Value));
    Json.SetArrayField(Key, Array);
}

bool ReadInts(const FJsonObject& Json, const TCHAR* Key, TArray<int32>& Out, int32 High)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Json.TryGetArrayField(Key, Values) || Values->Num() != FExpeditionRig::SiteCount) return false;
    Out.Reset();
    for (const auto& Value : *Values)
    {
        if (!Value.IsValid() || Value->Type != EJson::Number) return false;
        const double Number = Value->AsNumber();
        if (!FMath::IsFinite(Number) || Number < 0 || Number > High
            || Number != FMath::FloorToDouble(Number)) return false;
        Out.Add(static_cast<int32>(Number));
    }
    return true;
}

void PutBools(FJsonObject& Json, const TCHAR* Key, const TArray<bool>& Values)
{
    TArray<TSharedPtr<FJsonValue>> Array;
    for (bool Value : Values) Array.Add(MakeShared<FJsonValueBoolean>(Value));
    Json.SetArrayField(Key, Array);
}

bool ReadBools(const FJsonObject& Json, const TCHAR* Key, TArray<bool>& Out)
{
    const TArray<TSharedPtr<FJsonValue>>* Values = nullptr;
    if (!Json.TryGetArrayField(Key, Values) || Values->Num() != FExpeditionRig::SiteCount) return false;
    Out.Reset();
    for (const auto& Value : *Values)
    {
        if (!Value.IsValid() || Value->Type != EJson::Boolean) return false;
        Out.Add(Value->AsBool());
    }
    return true;
}

bool Unique(const TArray<FName>& Values)
{
    TSet<FName> Seen;
    for (FName Id : Values) { if (Id.IsNone() || Seen.Contains(Id)) return false; Seen.Add(Id); }
    return true;
}
}

const TArray<FExpeditionModule>& FExpeditionRig::Catalog()
{
    static const TArray<FExpeditionModule> Modules = []
    {
        TArray<FExpeditionModule> Result;
        auto Add = [&](const TCHAR* Id, const TCHAR* Name, const TCHAR* Description,
                       EExpeditionModuleKind Kind, int32 Price, int32 Slots,
                       std::initializer_list<const TCHAR*> Provides,
                       std::initializer_list<const TCHAR*> All,
                       std::initializer_list<const TCHAR*> Any,
                       std::initializer_list<const TCHAR*> Opportunities)
        {
            FExpeditionModule M;
            M.Id = FName(Id); M.Name = Name; M.Description = Description;
            M.Kind = Kind; M.Price = Price; M.Slots = Slots;
            M.Provides = Names(Provides); M.RequiresAll = Names(All);
            M.RequiresAny = Names(Any); M.OpportunityTags = Names(Opportunities);
            Result.Add(MoveTemp(M));
        };
        using K = EExpeditionModuleKind;
        Add(TEXT("extraction_coil"), TEXT("Extraction Coil"),
            TEXT("Aim to sever and pull one unanchored linked piece. 6 battery; selected mass must fit. Other weight stays behind."),
            K::Active,10,1,{TEXT("Sever")},{},{},{TEXT("Sever")});
        Add(TEXT("rail_impeller"), TEXT("Rail Impeller"),
            TEXT("Launch one selected held body along your aim. 8 battery; real mass becomes ammunition and may damage fragile salvage."),
            K::Active,10,1,{TEXT("Launch")},{},{},{TEXT("Launch")});
        Add(TEXT("arc_driver"), TEXT("Arc Driver"),
            TEXT("Pulse one internal charge through an actual conducting path to a selected terminal. 8 battery. External sources can supply finite extra charge."),
            K::Active,10,1,{TEXT("Arc")},{},{},{TEXT("Arc")});
        Add(TEXT("anchor_winch"), TEXT("Anchor Winch"),
            TEXT("Place an anchor, bind a body and tow it along a valid line. 8 battery per tow; real tension, supports and collisions still matter."),
            K::Active,10,1,{TEXT("PhysicalTether")},{},{},{TEXT("PhysicalTether")});
        Add(TEXT("vector_emitter"), TEXT("Vector Emitter"),
            TEXT("Push or rotate a selected group instead of collecting it. 6 battery. Direction can clear a route or shove salvage into danger."),
            K::Active,10,1,{TEXT("Vector")},{},{},{TEXT("Vector")});
        Add(TEXT("relay_projector"), TEXT("Relay Projector"),
            TEXT("Place source and receiver fields for one bounded transfer. 10 battery; clear routes and safe capacity apply. New transfers cost again."),
            K::Active,10,1,{TEXT("FieldTransfer")},{},{},{TEXT("FieldTransfer")});
        Add(TEXT("insulated_jaw"),TEXT("Insulated Jaw"),
            TEXT("A real sever isolates heat across that cut connection. It does not neutralize a hot cell deliberately carried."),
            K::Passive,5,1,{}, {TEXT("Sever")},{},{TEXT("LiveSource")});
        Add(TEXT("crack_follower"),TEXT("Crack Follower"),
            TEXT("A sever also cuts one marked eligible neighboring seam. +3 battery reserved before action; it cannot trigger itself."),
            K::Passive,9,1,{}, {TEXT("Sever")},{},{TEXT("AdjacentSeams")});
        Add(TEXT("impact_fuse"),TEXT("Impact Fuse"),
            TEXT("A launched body schedules one delayed burst after impact. Surviving bodies may rebound first; the burst consumes remaining ammunition integrity."),
            K::Passive,5,1,{}, {TEXT("Launch")},{},{TEXT("Launch")});
        Add(TEXT("slug_press"),TEXT("Slug Press"),
            TEXT("Weld at least two selected iron pieces into a conserved-mass slug for 4 battery. Launch separately for 8, or use it as a counterweight."),
            K::Passive,9,1,{}, {TEXT("Launch")},{},{TEXT("IronPair")});
        Add(TEXT("ground_clip"),TEXT("Ground Clip"),
            TEXT("Choose a ground endpoint that stops your arc before it reaches an unwanted branch. Requires an actual grounded termination."),
            K::Passive,5,1,{}, {TEXT("Arc")},{},{TEXT("Arc")});
        Add(TEXT("conductive_tether"),TEXT("Conductive Tether"),
            TEXT("A real winch tether also carries current. Requires both Winch and Arc; the connection carries live danger as well as useful charge."),
            K::Passive,9,1,{}, {TEXT("PhysicalTether"),TEXT("Arc")},{},{TEXT("ConductiveGap")});
        Add(TEXT("counterweight_hook"),TEXT("Counterweight Hook"),
            TEXT("Suspend real ballast to reduce another linked load's effective tow weight. Position and available mass determine the result."),
            K::Passive,5,1,{}, {TEXT("PhysicalTether")},{},{TEXT("SupportedLoad")});
        Add(TEXT("twin_anchor"),TEXT("Twin Anchor"),
            TEXT("Keep two placed anchors and switch tow direction without destroying the first setup. Each committed tow still costs 8 battery."),
            K::Passive,9,1,{}, {TEXT("PhysicalTether")},{},{TEXT("FixedAnchor")});
        Add(TEXT("rebound_plate"),TEXT("Rebound Plate"),
            TEXT("A surviving launched body reflects its remaining impulse once. A destroyed body cannot bounce; no momentum is created."),
            K::Passive,5,1,{}, {TEXT("Launch")},{},{TEXT("ReboundSurface")});
        Add(TEXT("flow_splitter"),TEXT("Flow Splitter"),
            TEXT("Divide the same transfer load between two marked destinations. +4 battery; no material is duplicated."),
            K::Passive,9,1,{}, {TEXT("FieldTransfer")},{},{TEXT("ClearGuide")});
        Add(TEXT("intact_recovery"),TEXT("Intact Recovery"),
            TEXT("After the last seam release, preserve a functional assembly in a support cradle. +6 battery, same real mass; uses two passive sockets."),
            K::Passive,24,2,{}, {TEXT("Sever")},{},{TEXT("FunctionalAssembly")});
        Add(TEXT("salvage_cyclone"),TEXT("Salvage Cyclone"),
            TEXT("After launch impact, catch up to six surviving fragments into a local orbit for the next manual launch. +6 battery; uses two sockets."),
            K::Passive,24,2,{}, {TEXT("Launch")},{},{TEXT("BrittleBrace")});
        Add(TEXT("closed_circuit"),TEXT("Closed Circuit"),
            TEXT("A completed conducting loop discharges each unique live source once through valid branches. Normal arc cost; uses two sockets."),
            K::Passive,24,2,{}, {TEXT("Arc")},{},{TEXT("LiveSource"),TEXT("ClosedReturn")});
        Add(TEXT("walking_gantry"),TEXT("Walking Gantry"),
            TEXT("Tow machinery and its support together without another support pickup. +6 battery per tow; uses two sockets."),
            K::Passive,14,2,{}, {TEXT("PhysicalTether")},{},{TEXT("SupportedLoad"),TEXT("FunctionalAssembly")});
        Add(TEXT("cold_seam"),TEXT("Cold Seam"),
            TEXT("A sever makes eligible neighboring material brittle for one hit within 70 units. +2 battery; weakening a load-bearing support can be dangerous."),
            K::Passive,6,1,{}, {TEXT("Sever")},{},{TEXT("BrittleEligible")});
        Add(TEXT("ratchet_pawl"),TEXT("Ratchet Pawl"),
            TEXT("Latch one tensioned load at its reached position while the winch works elsewhere. +2 battery; exceeding the real support rating breaks it."),
            K::Passive,6,1,{}, {TEXT("PhysicalTether")},{},{TEXT("FixedAnchor"),TEXT("SupportedLoad")});
        Add(TEXT("eddy_brake"),TEXT("Eddy Brake"),
            TEXT("Brake one moving conductive body at a committed region without collecting it. +2 battery; the arrested body becomes hot."),
            K::Passive,6,1,{}, {},{TEXT("Vector"),TEXT("FieldTransfer")},{TEXT("MovingBody")});
        Add(TEXT("shear_gate"),TEXT("Shear Gate"),
            TEXT("A pushed body crossing a marked cutting plane severs one eligible seam. +3 battery; recoil and weakened supports remain physical."),
            K::Passive,14,1,{}, {TEXT("Vector")},{},{TEXT("ShearSeam")});
        Add(TEXT("induction_bridge"),TEXT("Induction Bridge"),
            TEXT("Send existing arc charge across one marked clear gap of at most 80 units. +4 battery; adds no charge and can expose nearby conductors."),
            K::Passive,16,1,{}, {TEXT("Arc")},{},{TEXT("ConductiveGap")});
        Add(TEXT("heat_sink_mould"),TEXT("Heat-Sink Mould"),
            TEXT("Transfer one hazard token into a separate sacrificial iron body at rest. +4 battery; that finite heat sink becomes hot instead."),
            K::Passive,16,1,{}, {},{TEXT("Vector"),TEXT("FieldTransfer")},{TEXT("HotIronPair")});
        Add(TEXT("escapement_relay"),TEXT("Escapement Relay"),
            TEXT("Reserve one real charge so a later mechanism impact actuates a second terminal. +3 battery to arm; one pending relay, no charge refund loop."),
            K::Passive,16,1,{}, {TEXT("Arc")},{},{TEXT("TimedTerminal")});
        Add(TEXT("punch_through_collar"),TEXT("Punch-Through Collar"),
            TEXT("Break one designated brittle brace, then continue the launched body with its remaining momentum. +3 battery; sacrifices intact quality."),
            K::Passive,14,1,{}, {TEXT("Launch")},{},{TEXT("BrittleBrace")});
        Add(TEXT("reaction_frame"),TEXT("Reaction Frame"),
            TEXT("Transmit a push's opposite reaction through a chosen support to a second constrained body. +6 battery; two sockets and a sound support required."),
            K::Passive,24,2,{}, {TEXT("Vector")},{},{TEXT("FixedAnchor"),TEXT("SupportedLoad")});
        Add(TEXT("field_loom"),TEXT("Field Loom"),
            TEXT("Guide one moving body through a clear corridor with a chosen exit direction. +6 battery; two sockets. Projectile use also requires Launch."),
            K::Passive,24,2,{}, {TEXT("FieldTransfer")},{},{TEXT("ClearGuide")});
        return Result;
    }();
    return Modules;
}

const FExpeditionModule* FExpeditionRig::FindModule(FName Id)
{
    return Catalog().FindByPredicate([Id](const FExpeditionModule& M) { return M.Id == Id; });
}

const TArray<FName>& FExpeditionRig::InitialStarters()
{
    static const TArray<FName> Values = Names({TEXT("extraction_coil"),TEXT("rail_impeller"),TEXT("arc_driver"),TEXT("anchor_winch")});
    return Values;
}

FExpeditionRig::FExpeditionRig()
{
    Records.UnlockedStarters = InitialStarters();
}

int32 FExpeditionRig::RefiningThreshold(int32 SiteIndex, int32 MilestoneIndex)
{
    static const int32 Thresholds[3][2] = {{120,240},{180,360},{240,480}};
    return SiteIndex >= 0 && SiteIndex < 3 && MilestoneIndex >= 0 && MilestoneIndex < 2
        ? Thresholds[SiteIndex][MilestoneIndex] : 0;
}

bool FExpeditionRig::IsStarterUnlocked(FName Id) const
{
    const auto* M = FindModule(Id);
    return M && M->Kind == EExpeditionModuleKind::Active && Records.UnlockedStarters.Contains(Id);
}

bool FExpeditionRig::NewRun(FName NewStarter, int32 NewSeed)
{
    if (NewSeed <= 0 || !IsStarterUnlocked(NewStarter) || Records.RunsStarted >= MaxLedger) return false;
    FExpeditionRig Fresh;
    Fresh.Records = Records; ++Fresh.Records.RunsStarted;
    Fresh.Seed = NewSeed; Fresh.Starter = NewStarter;
    Fresh.Inventory.Add({NewStarter,0}); Fresh.FittedActives.Add(NewStarter);
    Fresh.OpportunityContext = OpportunityContext; Fresh.ImplementedContext = ImplementedContext;
    Fresh.bContextSet = bContextSet;
    *this = MoveTemp(Fresh);
    if (bContextSet) { FString Reason; GenerateOffers(Reason); }
    return true;
}

void FExpeditionRig::SetShopContext(const TArray<FName>& Tags, const TArray<FName>& Implemented)
{
    OpportunityContext.Reset(); ImplementedContext.Reset();
    for (FName Tag : Tags) if (!Tag.IsNone()) OpportunityContext.AddUnique(Tag);
    for (FName Id : Implemented) if (FindModule(Id)) ImplementedContext.AddUnique(Id);
    bContextSet = true;
    // Changing context can invalidate a visible offer, but never restocks or rerolls it.
    if (!bOffersGenerated && bAtDepot && !Starter.IsNone()) { FString Reason; GenerateOffers(Reason); }
}

bool FExpeditionRig::IsImplemented(FName Id) const
{
    return bContextSet && ImplementedContext.Contains(Id);
}

bool FExpeditionRig::HasOpportunity(FName Id) const
{
    const auto* M = FindModule(Id);
    if (!M || !bContextSet) return false;
    for (FName Tag : M->OpportunityTags) if (!OpportunityContext.Contains(Tag)) return false;
    return true;
}

bool FExpeditionRig::Owns(FName Id) const
{
    return Inventory.ContainsByPredicate([Id](const FExpeditionOwnedModule& M) { return M.Id == Id; });
}

bool FExpeditionRig::CompatibleWith(FName Id, const TArray<FName>& Actives) const
{
    const auto* M = FindModule(Id);
    if (!M) return false;
    TSet<FName> Tags;
    for (FName Active : Actives)
        if (const auto* Tool = FindModule(Active)) for (FName Tag : Tool->Provides) Tags.Add(Tag);
    for (FName Required : M->RequiresAll) if (!Tags.Contains(Required)) return false;
    if (!M->RequiresAny.IsEmpty())
    {
        bool Found = false;
        for (FName Any : M->RequiresAny) Found |= Tags.Contains(Any);
        if (!Found) return false;
    }
    return true;
}

bool FExpeditionRig::IsCompatible(FName Id) const { return CompatibleWith(Id,FittedActives); }

bool FExpeditionRig::Has(FName Id) const
{
    return IsImplemented(Id) && (FittedActives.Contains(Id)
        || (FittedPassives.Contains(Id) && IsCompatible(Id)));
}

bool FExpeditionRig::HasCapability(FName Capability) const
{
    for (FName Id : FittedActives)
        if (Has(Id)) if (const auto* M = FindModule(Id)) if (M->Provides.Contains(Capability)) return true;
    return false;
}

int32 FExpeditionRig::GetUsedSlots(EExpeditionModuleKind Kind) const
{
    int32 Used = 0;
    const auto& Fitted = Kind == EExpeditionModuleKind::Active ? FittedActives : FittedPassives;
    for (FName Id : Fitted) if (const auto* M = FindModule(Id)) Used += M->Slots;
    return Used;
}

bool FExpeditionRig::CanUseAfterRefit(FName Id) const
{
    const auto* M = FindModule(Id);
    if (!M || !IsImplemented(Id) || !HasOpportunity(Id)) return false;
    // A new independent active can replace an active; incompatible supports must
    // be unfitted before departure. A passive can replace existing passive slots.
    return M->Kind == EExpeditionModuleKind::Active || IsCompatible(Id);
}

bool FExpeditionRig::GenerateOffers(FString& Reason)
{
    if (!bAtDepot || bRunWon || Starter.IsNone()) return Refuse(Reason,TEXT("Open an active expedition depot first."));
    if (bOffersGenerated) { Reason = bShopPairVerified ? FString() : ShopNotice; return bShopPairVerified; }
    if (!bContextSet) { ShopNotice = TEXT("The next site's implemented tools and opportunities have not been supplied."); Reason = ShopNotice; return false; }
    TArray<FName> Pool;
    for (const auto& M : Catalog()) if (!Owns(M.Id) && CanUseAfterRefit(M.Id)) Pool.Add(M.Id);
    FRandomStream Random(static_cast<int32>(static_cast<uint32>(Seed)
        ^ ((static_cast<uint32>(DepotIndex)+1u)*0x9e3779b9u)));
    for (int32 I = Pool.Num()-1; I > 0; --I) Pool.Swap(I,Random.RandRange(0,I));
    Offers.Reset();
    auto Pick = [&](TFunctionRef<bool(const FExpeditionModule&)> Accept)
    {
        for (FName Id : Pool)
            if (!Offers.Contains(Id)) if (const auto* M = FindModule(Id))
                if (Accept(*M)) { Offers.Add(Id); return true; }
        return false;
    };
    const bool Support = Pick([&](const FExpeditionModule& M) {
        return M.Kind == EExpeditionModuleKind::Passive && M.Price <= Cash && IsCompatible(M.Id);
    });
    bool Alternative = Pick([&](const FExpeditionModule& M) {
        return M.Kind == EExpeditionModuleKind::Active && M.Price <= Cash;
    });
    if (!Alternative) Alternative = Pick([&](const FExpeditionModule& M) { return M.Price <= Cash; });
    // Remaining stock still needs real capability/opportunity compatibility;
    // expensive aspirational items are not counted as the affordable pair.
    Pick([](const FExpeditionModule& M) { return M.Kind == EExpeditionModuleKind::Active; });
    if (Offers.Num() < 4) Pick([](const FExpeditionModule& M) { return M.Price >= 14; });
    while (Offers.Num() < 4 && Pick([](const FExpeditionModule&) { return true; })) {}
    GeneratedStock = Offers; bOffersGenerated = true;
    bShopPairVerified = Support && Alternative;
    ShopNotice = bShopPairVerified
        ? TEXT("Choose a support or another active tool. Fit purchases before departure.")
        : TEXT("Limited stock: no affordable support-and-alternative pair suits this rig and worksite. Keep your rig, refit, or save your credits.");
    if (Offers.Num() < 4) ShopNotice += TEXT(" Fewer than four suitable modules are available.");
    Reason = bShopPairVerified ? FString() : ShopNotice;
    return bShopPairVerified;
}

bool FExpeditionRig::AtShop(FString& Reason) const
{
    if (!bAtDepot || bRunWon || Starter.IsNone()) return Refuse(Reason,TEXT("Equipment changes are available at a depot."));
    Reason.Reset(); return true;
}

bool FExpeditionRig::CanBuy(FName Id, FString& Reason) const
{
    if (!AtShop(Reason)) return false;
    const auto* M = FindModule(Id);
    if (!M || !Offers.Contains(Id)) return Refuse(Reason,TEXT("This module is not in the saved depot stock."));
    if (Owns(Id)) return Refuse(Reason,TEXT("You already own this module."));
    if (!IsImplemented(Id) || !HasOpportunity(Id)) return Refuse(Reason,TEXT("The current worksite does not support this module's operation."));
    if (!IsCompatible(Id)) return Refuse(Reason,TEXT("Fit the required active tool before buying this support."));
    if (Cash < M->Price) return Refuse(Reason,TEXT("Not enough credits."));
    if (TotalPurchased > MaxLedger-M->Price) return Refuse(Reason,TEXT("Expedition transaction limit reached."));
    Reason.Reset(); return true;
}

bool FExpeditionRig::Buy(FName Id, FString& Reason)
{
    if (!CanBuy(Id,Reason)) return false;
    const int32 Price = FindModule(Id)->Price;
    Cash -= Price; TotalPurchased += Price; Inventory.Add({Id,Price}); Offers.Remove(Id);
    // Purchase and fitting are separate explicit actions; a full rig never
    // silently ejects a support to make room for an expensive purchase.
    return true;
}

bool FExpeditionRig::CanFit(FName Id, FString& Reason) const
{
    if (!AtShop(Reason)) return false;
    const auto* M = FindModule(Id);
    if (!M || !Owns(Id)) return Refuse(Reason,TEXT("You do not own this module."));
    if (!IsImplemented(Id)) return Refuse(Reason,TEXT("This world does not yet implement the module's operation."));
    const auto& Fitted = M->Kind == EExpeditionModuleKind::Active ? FittedActives : FittedPassives;
    if (Fitted.Contains(Id)) return Refuse(Reason,TEXT("This module is already fitted."));
    if (!IsCompatible(Id)) return Refuse(Reason,TEXT("Fit the required active capability first."));
    const int32 Limit = M->Kind == EExpeditionModuleKind::Active ? ActiveSlots : PassiveSlots;
    if (GetUsedSlots(M->Kind)+M->Slots > Limit) return Refuse(Reason,TEXT("Unfit another module to make enough sockets available."));
    Reason.Reset(); return true;
}

bool FExpeditionRig::Fit(FName Id, FString& Reason)
{
    if (!CanFit(Id,Reason)) return false;
    (FindModule(Id)->Kind == EExpeditionModuleKind::Active ? FittedActives : FittedPassives).Add(Id);
    return true;
}

bool FExpeditionRig::Unfit(FName Id, FString& Reason)
{
    if (!AtShop(Reason)) return false;
    if (!FittedActives.Contains(Id) && !FittedPassives.Contains(Id)) return Refuse(Reason,TEXT("This module is not fitted."));
    FittedActives.Remove(Id); FittedPassives.Remove(Id);
    return true;
}

int32 FExpeditionRig::GetResaleValue(FName Id) const
{
    const auto* Owned = Inventory.FindByPredicate([Id](const FExpeditionOwnedModule& M) { return M.Id == Id; });
    return Owned ? Owned->Paid/2 : 0;
}

bool FExpeditionRig::Sell(FName Id, FString& Reason)
{
    if (!AtShop(Reason)) return false;
    if (!Owns(Id)) return Refuse(Reason,TEXT("You do not own this module."));
    const auto* Selling = FindModule(Id);
    if (Selling && Selling->Kind == EExpeditionModuleKind::Active)
    {
        const bool AnotherActive = Inventory.ContainsByPredicate([Id](const FExpeditionOwnedModule& Owned) {
            const auto* M = FindModule(Owned.Id);
            return Owned.Id != Id && M && M->Kind == EExpeditionModuleKind::Active;
        });
        if (!AnotherActive) return Refuse(Reason,TEXT("Keep one owned active tool so the expedition can continue. You can still unfit and refit it."));
    }
    const int32 Refund = GetResaleValue(Id);
    Cash += Refund; TotalRefunded += Refund;
    Inventory.RemoveAll([Id](const FExpeditionOwnedModule& M) { return M.Id == Id; });
    FittedActives.Remove(Id); FittedPassives.Remove(Id);
    // Sold items never refill the shop offer, including a free starter.
    return true;
}

bool FExpeditionRig::Precharge(FString& Reason)
{
    if (!AtShop(Reason)) return false;
    if (PrechargeBought[DepotIndex]) return Refuse(Reason,TEXT("Precharge is already fitted for the next site."));
    if (Cash < PrechargePrice) return Refuse(Reason,TEXT("Precharge costs 4 credits."));
    if (TotalPurchased > MaxLedger-PrechargePrice) return Refuse(Reason,TEXT("Expedition transaction limit reached."));
    Cash -= PrechargePrice; TotalPurchased += PrechargePrice;
    bPrecharged = true; PrechargeBought[DepotIndex] = true;
    return true;
}

bool FExpeditionRig::CanDepart(FString& Reason) const
{
    if (!AtShop(Reason)) return false;
    if (FittedActives.IsEmpty()) return Refuse(Reason,TEXT("Fit at least one active tool before leaving."));
    for (FName Id : FittedActives)
        if (!IsImplemented(Id)) return Refuse(Reason,TEXT("A fitted tool is not implemented by this worksite."));
    for (FName Id : FittedPassives)
        if (!IsImplemented(Id) || !IsCompatible(Id)) return Refuse(Reason,TEXT("Unfit inactive supports or restore their required active tool before leaving."));
    Reason.Reset(); return true;
}

bool FExpeditionRig::Depart(FString& Reason)
{
    if (!CanDepart(Reason)) return false;
    bAtDepot = false; return true;
}

bool FExpeditionRig::NextDepot(FString& Reason)
{
    if (bRunWon || DepotIndex >= SiteCount-1) return Refuse(Reason,TEXT("The final extraction has no further depot."));
    if (!SiteAwarded[DepotIndex]) return Refuse(Reason,TEXT("Dispatch this site's objective before continuing."));
    ++DepotIndex; bAtDepot = true; bPrecharged = false;
    bOffersGenerated = false; bShopPairVerified = false; Offers.Reset(); GeneratedStock.Reset();
    if (bContextSet) GenerateOffers(Reason);
    else { Reason = TEXT("Supply the next site's implemented modules and opportunities before opening stock."); ShopNotice = Reason; }
    return true;
}

void FExpeditionRig::Discover(FName Id)
{
    if (const auto* M = FindModule(Id))
    {
        Records.DiscoveredModules.AddUnique(Id);
        if (M->Kind == EExpeditionModuleKind::Active) Records.UnlockedStarters.AddUnique(Id);
    }
}

int32 FExpeditionRig::AwardOutput(int32 SiteIndex, int32 TotalOutput)
{
    if (bAtDepot || bRunWon || SiteIndex != DepotIndex || SiteIndex < 0 || SiteIndex >= SiteCount
        || SiteAwarded[SiteIndex] || TotalOutput < Output[SiteIndex] || TotalOutput > MaxOutput) return 0;
    Output[SiteIndex] = TotalOutput;
    int32 Earned = 0;
    if (SiteIndex < 3)
    {
        int32 Due = 0;
        for (int32 I = 0; I < 2; ++I) if (TotalOutput >= RefiningThreshold(SiteIndex,I)) ++Due;
        Earned = (Due-RefiningPaid[SiteIndex])*2;
        RefiningPaid[SiteIndex] = Due; Cash += Earned;
    }
    return Earned;
}

int32 FExpeditionRig::AwardSite(int32 SiteIndex)
{
    if (bAtDepot || bRunWon || SiteIndex != DepotIndex || SiteIndex < 0 || SiteIndex >= SiteCount || SiteAwarded[SiteIndex]) return 0;
    SiteAwarded[SiteIndex] = true;
    const int32 Earned = SiteIndex < 3 ? 10+SiteIndex*2 : 0;
    Cash += Earned;
    Records.BestOutput[SiteIndex] = FMath::Max(Records.BestOutput[SiteIndex],Output[SiteIndex]);
    for (const auto& Owned : Inventory) Discover(Owned.Id);
    if (SiteIndex == SiteCount-1)
    {
        bRunWon = true; ++Records.RunsWon;
        Records.LastWinningRig = FittedActives;
        Records.LastWinningRig.Append(FittedPassives);
    }
    return Earned;
}

TSharedRef<FJsonObject> FExpeditionRig::ToJson() const
{
    auto Json = MakeShared<FJsonObject>();
    Json->SetNumberField(TEXT("version"),SaveVersion);
    Json->SetNumberField(TEXT("cash"),Cash); Json->SetNumberField(TEXT("seed"),Seed);
    Json->SetNumberField(TEXT("depot"),DepotIndex); Json->SetNumberField(TEXT("purchased"),TotalPurchased);
    Json->SetNumberField(TEXT("refunded"),TotalRefunded); Json->SetStringField(TEXT("starter"),Starter.ToString());
    Json->SetBoolField(TEXT("at_depot"),bAtDepot); Json->SetBoolField(TEXT("won"),bRunWon);
    Json->SetBoolField(TEXT("precharged"),bPrecharged); Json->SetBoolField(TEXT("context_set"),bContextSet);
    Json->SetBoolField(TEXT("offers_generated"),bOffersGenerated); Json->SetBoolField(TEXT("shop_pair"),bShopPairVerified);
    TArray<TSharedPtr<FJsonValue>> Owned;
    for (const auto& M : Inventory)
    {
        auto Entry = MakeShared<FJsonObject>(); Entry->SetStringField(TEXT("id"),M.Id.ToString()); Entry->SetNumberField(TEXT("paid"),M.Paid);
        Owned.Add(MakeShared<FJsonValueObject>(Entry));
    }
    Json->SetArrayField(TEXT("inventory"),Owned);
    PutNames(*Json,TEXT("actives"),FittedActives); PutNames(*Json,TEXT("passives"),FittedPassives);
    PutNames(*Json,TEXT("offers"),Offers); PutNames(*Json,TEXT("stock"),GeneratedStock);
    PutNames(*Json,TEXT("opportunities"),OpportunityContext); PutNames(*Json,TEXT("implemented"),ImplementedContext);
    PutInts(*Json,TEXT("output"),Output); PutInts(*Json,TEXT("refining_paid"),RefiningPaid);
    PutBools(*Json,TEXT("site_awarded"),SiteAwarded); PutBools(*Json,TEXT("precharge_bought"),PrechargeBought);
    auto Record = MakeShared<FJsonObject>();
    Record->SetNumberField(TEXT("started"),Records.RunsStarted); Record->SetNumberField(TEXT("won"),Records.RunsWon);
    PutInts(*Record,TEXT("best_output"),Records.BestOutput);
    PutNames(*Record,TEXT("discovered"),Records.DiscoveredModules);
    PutNames(*Record,TEXT("starters"),Records.UnlockedStarters);
    PutNames(*Record,TEXT("last_rig"),Records.LastWinningRig);
    Json->SetObjectField(TEXT("records"),Record);
    return Json;
}

bool FExpeditionRig::FromJson(const FJsonObject& Json, FString& Reason)
{
    FExpeditionRig Candidate;
    int32 Version = 0; FString StarterName;
    if (!ReadInt(Json,TEXT("version"),SaveVersion,SaveVersion,Version)
        || !ReadInt(Json,TEXT("cash"),0,MaxLedger,Candidate.Cash)
        || !ReadInt(Json,TEXT("seed"),1,MAX_int32,Candidate.Seed)
        || !ReadInt(Json,TEXT("depot"),0,SiteCount-1,Candidate.DepotIndex)
        || !ReadInt(Json,TEXT("purchased"),0,MaxLedger,Candidate.TotalPurchased)
        || !ReadInt(Json,TEXT("refunded"),0,MaxLedger,Candidate.TotalRefunded)
        || !Json.TryGetStringField(TEXT("starter"),StarterName) || StarterName.Len() > 80
        || !Json.TryGetBoolField(TEXT("at_depot"),Candidate.bAtDepot)
        || !Json.TryGetBoolField(TEXT("won"),Candidate.bRunWon)
        || !Json.TryGetBoolField(TEXT("precharged"),Candidate.bPrecharged)
        || !Json.TryGetBoolField(TEXT("context_set"),Candidate.bContextSet)
        || !Json.TryGetBoolField(TEXT("offers_generated"),Candidate.bOffersGenerated)
        || !Json.TryGetBoolField(TEXT("shop_pair"),Candidate.bShopPairVerified))
        return Refuse(Reason,TEXT("Missing or invalid expedition rig fields."));
    Candidate.Starter = FName(*StarterName);
    if (!ReadNames(Json,TEXT("actives"),Candidate.FittedActives,ActiveSlots)
        || !ReadNames(Json,TEXT("passives"),Candidate.FittedPassives,PassiveSlots)
        || !ReadNames(Json,TEXT("offers"),Candidate.Offers,4)
        || !ReadNames(Json,TEXT("stock"),Candidate.GeneratedStock,4)
        || !ReadNames(Json,TEXT("opportunities"),Candidate.OpportunityContext)
        || !ReadNames(Json,TEXT("implemented"),Candidate.ImplementedContext,30)
        || !ReadInts(Json,TEXT("output"),Candidate.Output,MaxOutput)
        || !ReadInts(Json,TEXT("refining_paid"),Candidate.RefiningPaid,2)
        || !ReadBools(Json,TEXT("site_awarded"),Candidate.SiteAwarded)
        || !ReadBools(Json,TEXT("precharge_bought"),Candidate.PrechargeBought))
        return Refuse(Reason,TEXT("Invalid expedition equipment or reward arrays."));
    const TArray<TSharedPtr<FJsonValue>>* Owned = nullptr;
    if (!Json.TryGetArrayField(TEXT("inventory"),Owned) || Owned->Num() > Catalog().Num())
        return Refuse(Reason,TEXT("Invalid expedition inventory."));
    for (const auto& Value : *Owned)
    {
        if (!Value.IsValid() || Value->Type != EJson::Object) return Refuse(Reason,TEXT("Invalid owned module entry."));
        const auto Entry = Value->AsObject(); FString Id; int32 Paid = 0;
        if (!Entry.IsValid() || !Entry->TryGetStringField(TEXT("id"),Id) || Id.Len() > 80
            || !ReadInt(*Entry,TEXT("paid"),0,26,Paid)) return Refuse(Reason,TEXT("Invalid owned module price."));
        Candidate.Inventory.Add({FName(*Id),Paid});
    }
    const TSharedPtr<FJsonObject>* Record = nullptr;
    if (!Json.TryGetObjectField(TEXT("records"),Record) || !Record || !Record->IsValid()
        || !ReadInt(**Record,TEXT("started"),1,MaxLedger,Candidate.Records.RunsStarted)
        || !ReadInt(**Record,TEXT("won"),0,MaxLedger,Candidate.Records.RunsWon)
        || !ReadInts(**Record,TEXT("best_output"),Candidate.Records.BestOutput,MaxOutput)
        || !ReadNames(**Record,TEXT("discovered"),Candidate.Records.DiscoveredModules,30)
        || !ReadNames(**Record,TEXT("starters"),Candidate.Records.UnlockedStarters,6)
        || !ReadNames(**Record,TEXT("last_rig"),Candidate.Records.LastWinningRig,6))
        return Refuse(Reason,TEXT("Invalid expedition progression records."));
    if (!Candidate.Validate(Reason)) return false;
    Candidate.ShopNotice = Candidate.bShopPairVerified
        ? TEXT("Restored the same saved offers. Fit purchases before departure.")
        : TEXT("Limited saved stock. Keep your rig, refit, or save your credits.");
    *this = MoveTemp(Candidate); Reason.Reset(); return true;
}

bool FExpeditionRig::Validate(FString& Reason) const
{
    const auto* Start = FindModule(Starter);
    if (!Start || Start->Kind != EExpeditionModuleKind::Active || Seed <= 0
        || DepotIndex < 0 || DepotIndex >= SiteCount || Cash < 0 || Cash > MaxLedger
        || TotalPurchased < 0 || TotalPurchased > MaxLedger || TotalRefunded < 0 || TotalRefunded > MaxLedger)
        return Refuse(Reason,TEXT("Invalid expedition identity or ledger bounds."));
    if (Output.Num() != SiteCount || SiteAwarded.Num() != SiteCount || RefiningPaid.Num() != SiteCount
        || PrechargeBought.Num() != SiteCount || Records.BestOutput.Num() != SiteCount)
        return Refuse(Reason,TEXT("Invalid expedition site history size."));
    int64 Earned = 12; int32 Services = 0;
    for (int32 I = 0; I < SiteCount; ++I)
    {
        if (Output[I] < 0 || Output[I] > MaxOutput || Records.BestOutput[I] < 0 || Records.BestOutput[I] > MaxOutput)
            return Refuse(Reason,TEXT("Invalid refining output."));
        if (I < DepotIndex && !SiteAwarded[I]) return Refuse(Reason,TEXT("A previous physical objective was not dispatched."));
        if (I > DepotIndex && (SiteAwarded[I] || Output[I] != 0 || RefiningPaid[I] != 0 || PrechargeBought[I]))
            return Refuse(Reason,TEXT("Future expedition rewards have already been paid."));
        int32 Expected = 0;
        if (I < 3) for (int32 M = 0; M < 2; ++M) if (Output[I] >= RefiningThreshold(I,M)) ++Expected;
        if (RefiningPaid[I] != Expected) return Refuse(Reason,TEXT("Refining payment does not match committed output."));
        Earned += 2*RefiningPaid[I];
        if (SiteAwarded[I])
        {
            if (I < 3) Earned += 10+2*I;
            if (Records.BestOutput[I] < Output[I]) return Refuse(Reason,TEXT("Dispatched output is missing from records."));
        }
        if (PrechargeBought[I]) Services += PrechargePrice;
    }
    if (bPrecharged != PrechargeBought[DepotIndex] || bRunWon != SiteAwarded[SiteCount-1]
        || (bRunWon && (DepotIndex != SiteCount-1 || bAtDepot))
        || (bAtDepot && (SiteAwarded[DepotIndex] || Output[DepotIndex] > 0)))
        return Refuse(Reason,TEXT("Expedition phase and reward history disagree."));
    if (Earned-TotalPurchased+TotalRefunded != Cash)
        return Refuse(Reason,TEXT("Expedition credits do not match earned and spent amounts."));
    TSet<FName> OwnedIds; int32 OwnedCost = 0;
    for (const auto& Owned : Inventory)
    {
        const auto* M = FindModule(Owned.Id);
        // The coupled-tow specialist was priced at 24 in packaged 0.6.1.
        // Keep that actual receipt and its half-paid resale basis intact.
        const bool HistoricalGantry=Owned.Id==TEXT("walking_gantry") && Owned.Paid==24;
        if (!M || OwnedIds.Contains(Owned.Id) || (Owned.Paid != 0 && Owned.Paid != M->Price && !HistoricalGantry))
            return Refuse(Reason,TEXT("Unknown, duplicate or mispriced owned module."));
        OwnedIds.Add(Owned.Id); OwnedCost += Owned.Paid;
    }
    if (TotalPurchased < OwnedCost+Services || int64(TotalRefunded)*2 > TotalPurchased-OwnedCost-Services)
        return Refuse(Reason,TEXT("Refunds or inventory exceed the paid transaction basis."));
    if (!Unique(FittedActives) || !Unique(FittedPassives)
        || GetUsedSlots(EExpeditionModuleKind::Active) > ActiveSlots
        || GetUsedSlots(EExpeditionModuleKind::Passive) > PassiveSlots)
        return Refuse(Reason,TEXT("Duplicate equipment or too many fitted sockets."));
    for (FName Id : FittedActives)
    {
        const auto* M = FindModule(Id);
        if (!M || !OwnedIds.Contains(Id) || M->Kind != EExpeditionModuleKind::Active)
            return Refuse(Reason,TEXT("Invalid fitted active tool."));
    }
    for (FName Id : FittedPassives)
    {
        const auto* M = FindModule(Id);
        if (!M || !OwnedIds.Contains(Id) || M->Kind != EExpeditionModuleKind::Passive
            || (!bAtDepot && !IsCompatible(Id))) return Refuse(Reason,TEXT("Invalid or inactive fitted support outside the depot."));
    }
    if (!bAtDepot && FittedActives.IsEmpty()) return Refuse(Reason,TEXT("An active site has no fitted tool."));
    if (!Unique(Offers) || !Unique(GeneratedStock) || Offers.Num() > 4 || GeneratedStock.Num() > 4
        || (!bOffersGenerated && (!Offers.IsEmpty() || !GeneratedStock.IsEmpty() || bShopPairVerified)))
        return Refuse(Reason,TEXT("Invalid saved depot stock."));
    for (FName Id : GeneratedStock) if (!FindModule(Id)) return Refuse(Reason,TEXT("Unknown saved offer."));
    for (FName Id : Offers)
        if (!FindModule(Id) || !GeneratedStock.Contains(Id) || OwnedIds.Contains(Id))
            return Refuse(Reason,TEXT("Remaining offer is owned or was not in the saved stock."));
    if (!Unique(OpportunityContext) || !Unique(ImplementedContext) || (!bContextSet && (!OpportunityContext.IsEmpty() || !ImplementedContext.IsEmpty())))
        return Refuse(Reason,TEXT("Invalid worksite capability context."));
    for (FName Id : ImplementedContext) if (!FindModule(Id)) return Refuse(Reason,TEXT("Unknown implemented module ID."));
    if (Records.RunsStarted < 1 || Records.RunsStarted > MaxLedger || Records.RunsWon < 0
        || Records.RunsWon > Records.RunsStarted || (bRunWon && Records.RunsWon < 1)
        || !Unique(Records.DiscoveredModules) || !Unique(Records.UnlockedStarters) || !Unique(Records.LastWinningRig))
        return Refuse(Reason,TEXT("Invalid expedition progression counters."));
    for (FName Id : InitialStarters()) if (!Records.UnlockedStarters.Contains(Id)) return Refuse(Reason,TEXT("A baseline starter is missing."));
    for (FName Id : Records.DiscoveredModules) if (!FindModule(Id)) return Refuse(Reason,TEXT("Unknown discovered module."));
    for (FName Id : Records.UnlockedStarters)
    {
        const auto* M = FindModule(Id);
        if (!M || M->Kind != EExpeditionModuleKind::Active || (!InitialStarters().Contains(Id) && !Records.DiscoveredModules.Contains(Id)))
            return Refuse(Reason,TEXT("An unlocked starter has no dispatched discovery."));
    }
    if (!Records.UnlockedStarters.Contains(Starter)) return Refuse(Reason,TEXT("The run selected a locked starter."));
    TArray<FName> ArchiveActives; int32 ArchivePassiveSlots = 0;
    for (FName Id : Records.LastWinningRig)
    {
        const auto* M = FindModule(Id);
        if (!M || !Records.DiscoveredModules.Contains(Id)) return Refuse(Reason,TEXT("Winning archive contains an undiscovered module."));
        if (M->Kind == EExpeditionModuleKind::Active) ArchiveActives.Add(Id); else ArchivePassiveSlots += M->Slots;
    }
    if (ArchiveActives.Num() > ActiveSlots || ArchivePassiveSlots > PassiveSlots
        || (Records.RunsWon == 0 && !Records.LastWinningRig.IsEmpty())
        || (Records.RunsWon > 0 && ArchiveActives.IsEmpty())) return Refuse(Reason,TEXT("Invalid winning rig socket record."));
    for (FName Id : Records.LastWinningRig) if (!CompatibleWith(Id,ArchiveActives)) return Refuse(Reason,TEXT("Winning archive has an inactive support."));
    Reason.Reset(); return true;
}
}
