#if WITH_DEV_AUTOMATION_TESTS
#include "ExpeditionRig.h"
#include "ExpeditionWorld.h"
#include "Dom/JsonObject.h"
#include "HAL/FileManager.h"
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"

using namespace MagnetSweep;

namespace ShopChoiceAudit
{
// This is a generated-stock and ledger audit. Award calls below assume successful
// recovery at an explicit output budget; no physics, enjoyment or build-plan
// equivalence is inferred from tags, distinct IDs, or the number of offers.
TArray<FName> Implemented()
{
    TArray<FName> Result;
    for(const auto& M:FExpeditionRig::Catalog())
        if(FExpeditionWorld::IsModuleImplemented(M.Id))Result.Add(M.Id);
    return Result;
}

FString Signature(const TArray<FName>& Ids)
{
    TArray<FString> Names;
    for(FName Id:Ids)Names.Add(Id.ToString());
    Names.Sort();return FString::Join(Names,TEXT("|"));
}

TArray<TSharedPtr<FJsonValue>> NamesJson(const TArray<FName>& Ids)
{
    TArray<TSharedPtr<FJsonValue>> Result;
    for(FName Id:Ids)Result.Add(MakeShared<FJsonValueString>(Id.ToString()));
    return Result;
}

TSharedRef<FJsonObject> CountsJson(const TMap<FString,int32>& Counts)
{
    auto Result=MakeShared<FJsonObject>();TArray<FString> Keys;Counts.GetKeys(Keys);Keys.Sort();
    for(const FString& Key:Keys)Result->SetNumberField(Key,Counts.FindChecked(Key));
    return Result;
}

int32 Bits(int32 Mask)
{
    int32 Count=0;for(;Mask;Mask>>=1)Count+=Mask&1;return Count;
}

struct FPurchase
{
    bool bUsable=false;
    bool bRefit=false;
    FExpeditionRig Rig;
    TArray<FName> Removed;
    TArray<FName> RestoredActives;
    FString Reason;
};

bool VisitPurchaseFits(const FExpeditionRig& Before,FName Id,TFunctionRef<bool(const FPurchase&)> Accept,FString& Reason)
{
    FExpeditionRig Purchased=Before;
    if(!Purchased.Buy(Id,Reason))return false;
    const auto Actives=Before.GetFittedActives(),Passives=Before.GetFittedPassives();
    TArray<FName> Restore={NAME_None};
    for(const auto& Owned:Before.GetInventory())if(const auto* M=FExpeditionRig::FindModule(Owned.Id);M&&M->Kind==EExpeditionModuleKind::Active&&Owned.Id!=Id&&!Actives.Contains(Owned.Id))Restore.Add(Owned.Id);
    // Explore actual legal fitting operations, smallest displacement first. This
    // exposes an offer requiring a refit instead of counting CanBuy as usable.
    for(int32 Removed=0;Removed<=Actives.Num()+Passives.Num();++Removed)
        for(int32 A=0;A<(1<<Actives.Num());++A)
            for(int32 P=0;P<(1<<Passives.Num());++P)
            {
                if(Bits(A)+Bits(P)!=Removed)continue;
                for(FName Extra:Restore)
                {
                FExpeditionRig Trial=Purchased;FString Why;TArray<FName> Displaced;
                bool Valid=true;
                for(int32 I=0;I<Actives.Num();++I)if(A&(1<<I)){Valid&=Trial.Unfit(Actives[I],Why);Displaced.Add(Actives[I]);}
                for(int32 I=0;I<Passives.Num();++I)if(P&(1<<I)){Valid&=Trial.Unfit(Passives[I],Why);Displaced.Add(Passives[I]);}
                if(!Extra.IsNone())Valid&=Trial.Fit(Extra,Why);
                if(Valid&&Trial.Fit(Id,Why)&&Trial.Has(Id)&&Trial.CanDepart(Why))
                {
                    FPurchase Result;Result.bUsable=true;Result.bRefit=Removed>0||!Extra.IsNone();Result.Removed=Displaced;Result.Rig=MoveTemp(Trial);if(!Extra.IsNone())Result.RestoredActives.Add(Extra);
                    if(Accept(Result)){Reason.Reset();return true;}
                }
                Reason=Why;
                }
            }
    return false;
}

FPurchase Purchase(const FExpeditionRig& Before,FName Id)
{
    FPurchase Result;Result.Rig=Before;FString Why;
    VisitPurchaseFits(Before,Id,[&](const FPurchase& Candidate){Result=Candidate;return true;},Why);
    if(!Result.bUsable)Result.Reason=Why;
    return Result;
}

FPurchase PurchasePair(const FExpeditionRig& Before,FName Active,FName Support)
{
    FPurchase Result;Result.Rig=Before;FString Why;
    // Do not use the generator's pair/forecast helper: independently witness the
    // transactions. All legal first-tool refits are explored, including the one
    // which retains an old prerequisite needed together with the new tool.
    VisitPurchaseFits(Before,Active,[&](const FPurchase& First)
    {
        FString SecondWhy;
        return VisitPurchaseFits(First.Rig,Support,[&](const FPurchase& Second)
        {
            if(!Second.Rig.Has(Active)||!Second.Rig.Has(Support))return false;
            Result=Second;Result.bRefit|=First.bRefit;
            for(FName Id:First.Removed)Result.Removed.AddUnique(Id);
            for(FName Id:First.RestoredActives)Result.RestoredActives.AddUnique(Id);
            return true;
        },SecondWhy);
    },Why);
    if(!Result.bUsable)Result.Reason=Why;
    return Result;
}

struct FGroup
{
    int32 Samples=0,CashMin=MAX_int32,CashMax=0;
    int32 NoChoice=0,NoDirectChoice=0,NoSupportAndActive=0;
    int32 RefitOnlyShops=0,Dormant=0,Unimplemented=0,InvalidPurchase=0;
    int32 SoleAspirationShops=0,NoAffordableAspiration=0;
    int32 PairSamples=0,ShopsWithPair=0,ShopsWithConditionalPair=0;
    int32 ConditionalOffers=0,ValidConditionalOffers=0,InvalidConditionalOffers=0;
    TMap<FString,int32> Stocks,Offers,Direct,Refit,ActiveFamilies,SupportByPrerequisite;
    TMap<FString,int32> SoleAspirations,AffordableHigh,InstalledWithoutOpportunity;
    TMap<FString,int32> CashCounts,UsefulChoiceCounts;
    TMap<FString,int32> CompatiblePairs,ConditionalPairs,PairRefits,ConditionalModules,PairChoiceCounts;

    TSharedRef<FJsonObject> Json() const
    {
        auto J=MakeShared<FJsonObject>();
        J->SetNumberField(TEXT("samples"),Samples);J->SetNumberField(TEXT("cash_min"),CashMin);J->SetNumberField(TEXT("cash_max"),CashMax);
        J->SetNumberField(TEXT("distinct_unordered_stocks"),Stocks.Num());
        J->SetNumberField(TEXT("no_affordable_legal_fit"),NoChoice);J->SetNumberField(TEXT("no_direct_fit"),NoDirectChoice);
        J->SetNumberField(TEXT("no_affordable_support_and_active_alternative"),NoSupportAndActive);J->SetNumberField(TEXT("refit_only_shops"),RefitOnlyShops);
        J->SetNumberField(TEXT("dormant_offers"),Dormant);J->SetNumberField(TEXT("unimplemented_offers"),Unimplemented);J->SetNumberField(TEXT("affordable_but_no_legal_fit"),InvalidPurchase);
        J->SetNumberField(TEXT("sole_eligible_high_tier_shops"),SoleAspirationShops);J->SetNumberField(TEXT("no_affordable_offered_high_tier"),NoAffordableAspiration);
        J->SetObjectField(TEXT("stock_frequencies"),CountsJson(Stocks));J->SetObjectField(TEXT("offered_modules"),CountsJson(Offers));
        J->SetObjectField(TEXT("direct_fit_modules"),CountsJson(Direct));J->SetObjectField(TEXT("refit_modules"),CountsJson(Refit));
        J->SetObjectField(TEXT("affordable_active_capability_choices"),CountsJson(ActiveFamilies));
        J->SetObjectField(TEXT("affordable_supports_grouped_by_required_capability"),CountsJson(SupportByPrerequisite));
        J->SetObjectField(TEXT("sole_eligible_high_tier_modules"),CountsJson(SoleAspirations));J->SetObjectField(TEXT("affordable_high_tier_modules"),CountsJson(AffordableHigh));
        J->SetObjectField(TEXT("installed_support_without_next_site_tag"),CountsJson(InstalledWithoutOpportunity));
        J->SetObjectField(TEXT("cash_frequencies"),CountsJson(CashCounts));J->SetObjectField(TEXT("legal_fit_choice_counts"),CountsJson(UsefulChoiceCounts));
        J->SetNumberField(TEXT("pair_audit_samples"),PairSamples);J->SetNumberField(TEXT("shops_with_affordable_offered_pair"),ShopsWithPair);J->SetNumberField(TEXT("shops_with_affordable_conditional_pair"),ShopsWithConditionalPair);
        J->SetNumberField(TEXT("conditional_support_offers"),ConditionalOffers);J->SetNumberField(TEXT("valid_conditional_support_offers"),ValidConditionalOffers);J->SetNumberField(TEXT("invalid_conditional_support_offers"),InvalidConditionalOffers);
        J->SetObjectField(TEXT("currently_compatible_support_pairs"),CountsJson(CompatiblePairs));J->SetObjectField(TEXT("conditional_support_pairs"),CountsJson(ConditionalPairs));J->SetObjectField(TEXT("pairs_requiring_refit"),CountsJson(PairRefits));
        J->SetObjectField(TEXT("conditional_support_modules"),CountsJson(ConditionalModules));J->SetObjectField(TEXT("affordable_offered_pair_counts"),CountsJson(PairChoiceCounts));
        return J;
    }
};

struct FAudit
{
    TMap<FString,FGroup> Groups;
    TArray<TSharedPtr<FJsonValue>> Examples;
    TMap<FString,int32> ExampleCounts;
    int32 StructuralErrors=0,SaveChecks=0,NoRerollChecks=0;

    void Example(const TCHAR* Kind,int32 Seed,FName Starter,const FString& Path,const FExpeditionRig& Rig,const FString& Detail)
    {
        const FString Category(Kind);const int32 Count=++ExampleCounts.FindOrAdd(Category);
        if(Count>16)return; // Retain reproducible examples without a multi-megabyte event dump.
        auto J=MakeShared<FJsonObject>();J->SetStringField(TEXT("kind"),Category);J->SetNumberField(TEXT("seed"),Seed);
        J->SetStringField(TEXT("starter"),Starter.ToString());J->SetStringField(TEXT("path"),Path);J->SetNumberField(TEXT("cash"),Rig.GetCash());
        J->SetArrayField(TEXT("stock"),NamesJson(Rig.GetOffers()));J->SetArrayField(TEXT("fitted_actives"),NamesJson(Rig.GetFittedActives()));J->SetArrayField(TEXT("fitted_passives"),NamesJson(Rig.GetFittedPassives()));J->SetStringField(TEXT("detail"),Detail);
        Examples.Add(MakeShared<FJsonValueObject>(J));
    }

    void Error(int32 Seed,FName Starter,const FString& Path,const FExpeditionRig& Rig,const FString& Detail)
    {++StructuralErrors;Example(TEXT("structural_error"),Seed,Starter,Path,Rig,Detail);}

    TArray<TPair<FName,FPurchase>> Observe(const FString& Phase,int32 Seed,FName Starter,const FString& Path,const FExpeditionRig& Rig)
    {
        auto& G=Groups.FindOrAdd(Starter.ToString()+TEXT("/")+Phase);
        ++G.Samples;G.CashMin=FMath::Min(G.CashMin,Rig.GetCash());G.CashMax=FMath::Max(G.CashMax,Rig.GetCash());
        ++G.CashCounts.FindOrAdd(FString::FromInt(Rig.GetCash()));++G.Stocks.FindOrAdd(Signature(Rig.GetOffers()));
        int32 Direct=0,Refitted=0,Supports=0,Actives=0,High=0;
        TArray<TPair<FName,FPurchase>> Choices;
        TSet<FName> ValidConditional;
        int32 PairCount=0,ConditionalPairCount=0;++G.PairSamples;
        for(FName Active:Rig.GetOffers())
        {
            const auto* Tool=FExpeditionRig::FindModule(Active);
            if(!Tool||Tool->Kind!=EExpeditionModuleKind::Active)continue;
            for(FName Support:Rig.GetOffers())
            {
                const auto* Mod=FExpeditionRig::FindModule(Support);
                if(!Mod||Mod->Kind!=EExpeditionModuleKind::Passive)continue;
                const auto Pair=PurchasePair(Rig,Active,Support);
                if(!Pair.bUsable)continue;
                ++PairCount;const FString PairId=Active.ToString()+TEXT(" + ")+Support.ToString();
                if(Rig.IsCompatible(Support))++G.CompatiblePairs.FindOrAdd(PairId);
                else
                {
                    ++ConditionalPairCount;ValidConditional.Add(Support);++G.ConditionalPairs.FindOrAdd(PairId);
                    Example(TEXT("affordable_conditional_pair"),Seed,Starter,Path,Rig,PairId+FString::Printf(TEXT(" costs%d; leaves%d; removed "),Tool->Price+Mod->Price,Pair.Rig.GetCash())+Signature(Pair.Removed)+TEXT("; restored owned actives ")+Signature(Pair.RestoredActives));
                }
                if(Pair.bRefit)++G.PairRefits.FindOrAdd(PairId);
            }
        }
        ++G.PairChoiceCounts.FindOrAdd(FString::FromInt(PairCount));
        if(PairCount)++G.ShopsWithPair;
        if(ConditionalPairCount)++G.ShopsWithConditionalPair;
        for(FName Id:Rig.GetOffers())
        {
            const auto* M=FExpeditionRig::FindModule(Id);
            if(!M){Error(Seed,Starter,Path,Rig,TEXT("Unknown offered catalogue ID"));continue;}
            ++G.Offers.FindOrAdd(Id.ToString());
            if(!Rig.IsImplemented(Id)){++G.Unimplemented;Error(Seed,Starter,Path,Rig,TEXT("Generated unimplemented stock: ")+Id.ToString());}
            if(!Rig.HasOpportunity(Id))
            {++G.Dormant;Error(Seed,Starter,Path,Rig,TEXT("Fresh generated stock lacks an authored opportunity: ")+Id.ToString());}
            if(!Rig.IsCompatible(Id))
            {
                ++G.ConditionalOffers;++G.ConditionalModules.FindOrAdd(Id.ToString());
                if(M->Kind==EExpeditionModuleKind::Passive&&ValidConditional.Contains(Id))++G.ValidConditionalOffers;
                else
                {
                    ++G.InvalidConditionalOffers;++G.Dormant;
                    Error(Seed,Starter,Path,Rig,TEXT("Conditional offer has no affordable legal offered active/support pair: ")+Id.ToString());
                }
            }
            FString Why;if(!Rig.CanBuy(Id,Why))continue;
            auto Bought=Purchase(Rig,Id);
            if(!Bought.bUsable){++G.InvalidPurchase;Example(TEXT("buyable_without_legal_fit"),Seed,Starter,Path,Rig,Id.ToString()+TEXT(": ")+Bought.Reason);continue;}
            if(Bought.bRefit){++Refitted;++G.Refit.FindOrAdd(Id.ToString());Example(TEXT("requires_refit"),Seed,Starter,Path,Rig,Id.ToString()+TEXT(" displaces ")+Signature(Bought.Removed));}
            else{++Direct;++G.Direct.FindOrAdd(Id.ToString());}
            if(M->Kind==EExpeditionModuleKind::Active)
            {++Actives;++G.ActiveFamilies.FindOrAdd(Signature(M->Provides));}
            else
            {
                ++Supports;
                const FString Prerequisite=TEXT("all:")+Signature(M->RequiresAll)+TEXT(";any:")+Signature(M->RequiresAny);
                ++G.SupportByPrerequisite.FindOrAdd(Prerequisite);
            }
            if(M->Price>=14){++High;++G.AffordableHigh.FindOrAdd(Id.ToString());}
            Choices.Emplace(Id,MoveTemp(Bought));
        }
        ++G.UsefulChoiceCounts.FindOrAdd(FString::FromInt(Choices.Num()));
        if(Choices.IsEmpty()){++G.NoChoice;Example(TEXT("no_affordable_legal_fit"),Seed,Starter,Path,Rig,Rig.GetShopNotice());}
        if(!Direct)++G.NoDirectChoice;
        if(!Direct&&Refitted)++G.RefitOnlyShops;
        if(!Supports||!Actives){++G.NoSupportAndActive;Example(TEXT("no_support_active_pair"),Seed,Starter,Path,Rig,FString::Printf(TEXT("Legal support choices%d; active choices%d"),Supports,Actives));}
        if(!High)++G.NoAffordableAspiration;
        TArray<FName> EligibleHigh;
        for(const auto& M:FExpeditionRig::Catalog())
            if(M.Price>=14&&!Rig.Owns(M.Id)&&Rig.IsImplemented(M.Id)&&Rig.HasOpportunity(M.Id)&&Rig.IsCompatible(M.Id))EligibleHigh.Add(M.Id);
        if(EligibleHigh.Num()==1)
        {++G.SoleAspirationShops;++G.SoleAspirations.FindOrAdd(EligibleHigh[0].ToString());Example(TEXT("sole_eligible_high_tier"),Seed,Starter,Path,Rig,EligibleHigh[0].ToString());}
        for(FName Id:Rig.GetFittedPassives())if(!Rig.HasOpportunity(Id))
        {++G.InstalledWithoutOpportunity.FindOrAdd(Id.ToString());Example(TEXT("installed_support_without_site_tag"),Seed,Starter,Path,Rig,Id.ToString()+TEXT(" remains fitted; tag absence is not proof that a player-created use is impossible"));}
        return Choices;
    }
};

bool AdvanceBudget(FExpeditionRig& Rig,int32 Site,int32 Output,int32 Seed,const TArray<FName>& ImplementedIds,FString& Reason)
{
    if(!Rig.Depart(Reason))return false;
    Rig.AwardOutput(Site,Output);
    if(Rig.AwardSite(Site)<=0){Reason=TEXT("Budget scenario failed to award a fresh assumed site clear");return false;}
    FExpeditionWorld Next;
    if(!Next.StartSite(Site+1,Seed+Site+1)){Reason=TEXT("Next authored world could not be created");return false;}
    Rig.SetShopContext(Next.GetOpportunityTags(),ImplementedIds);
    return Rig.NextDepot(Reason);
}
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FExpeditionGeneratedChoiceAudit,
    "MagnetSweep.Expedition.GeneratedShopChoicesAcross128Seeds",
    EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FExpeditionGeneratedChoiceAudit::RunTest(const FString& Parameters)
{
    using namespace ShopChoiceAudit;
    FAudit Audit;const auto ImplementedIds=Implemented();
    for(FName Starter:FExpeditionRig::InitialStarters())for(int32 Seed=1;Seed<=128;++Seed)
    {
        FExpeditionWorld OpeningWorld;OpeningWorld.StartSite(0,Seed);
        FExpeditionRig Opening;Opening.SetShopContext(OpeningWorld.GetOpportunityTags(),ImplementedIds);
        if(!Opening.NewRun(Starter,Seed)){Audit.Error(Seed,Starter,TEXT("opening"),Opening,TEXT("Initial starter could not begin a run"));continue;}
        const auto SavedStock=Opening.GetOffers();FString Why;
        FExpeditionRig Restored;
        if(!Restored.FromJson(*Opening.ToJson(),Why)||Restored.GetOffers()!=SavedStock)
            Audit.Error(Seed,Starter,TEXT("opening"),Opening,TEXT("Save restoration changed exact stock: ")+Why);
        else ++Audit.SaveChecks;
        Restored.SetShopContext(OpeningWorld.GetOpportunityTags(),ImplementedIds);Restored.GenerateOffers(Why);
        if(Restored.GetOffers()!=SavedStock)Audit.Error(Seed,Starter,TEXT("opening"),Opening,TEXT("Context refresh or repeated generation rerolled saved stock"));
        else ++Audit.NoRerollChecks;
        auto FirstChoices=Audit.Observe(TEXT("opening"),Seed,Starter,TEXT("save all"),Opening);
        // Include doing nothing. Each other branch buys exactly one real opening
        // offer; no invented item, fake inventory receipt, reroll or cash grant.
        FPurchase Keep;Keep.bUsable=true;Keep.Rig=Opening;FirstChoices.Insert(TPair<FName,FPurchase>(NAME_None,MoveTemp(Keep)),0);
        for(const auto& First:FirstChoices)for(int32 Output:{0,120})
        {
            FExpeditionRig Depot=First.Value.Rig;
            const FString Choice=First.Key.IsNone()?TEXT("save_all"):TEXT("buy_")+First.Key.ToString();
            const FString Budget=Output==0?TEXT("clear_only"):TEXT("clear_plus_120_output");
            const FString Path=TEXT("opening:")+Choice+TEXT(" -> assumed ")+Budget;
            if(!AdvanceBudget(Depot,0,Output,Seed,ImplementedIds,Why)){Audit.Error(Seed,Starter,Path,Depot,Why);continue;}
            const FString Phase=TEXT("depot1/")+Budget+(First.Key.IsNone()?TEXT("/saved"):TEXT("/opening_purchase"));
            auto SecondChoices=Audit.Observe(Phase,Seed,Starter,Path,Depot);
            FPurchase SaveAgain;SaveAgain.bUsable=true;SaveAgain.Rig=Depot;SecondChoices.Insert(TPair<FName,FPurchase>(NAME_None,MoveTemp(SaveAgain)),0);
            // One further legal purchase-or-save branch reveals whether a newly
            // constructed rig loses advertised stock opportunities at the rack.
            for(const auto& Second:SecondChoices)
            {
                FExpeditionRig Late=Second.Value.Rig;
                const FString LatePath=Path+TEXT(" -> depot1:")+(Second.Key.IsNone()?TEXT("save_all"):Second.Key.ToString())+TEXT(" -> assumed site1 core-only clear");
                if(!AdvanceBudget(Late,1,0,Seed,ImplementedIds,Why)){Audit.Error(Seed,Starter,LatePath,Late,Why);continue;}
                Audit.Observe(TEXT("depot2/baseline_probe/")+Budget,Seed,Starter,LatePath,Late);
            }
        }
    }
    auto Report=MakeShared<FJsonObject>();Report->SetNumberField(TEXT("schema"),2);Report->SetNumberField(TEXT("seed_first"),1);Report->SetNumberField(TEXT("seed_last"),128);
    Report->SetStringField(TEXT("evidence"),TEXT("Actual rig/world APIs; budget assumptions use AwardOutput/AwardSite. No physical play, player appeal, or equivalence of mechanical build plans is established."));
    Report->SetStringField(TEXT("weighting"),TEXT("Opening: one sample per starter/seed. Later groups: one sample per earlier single-purchase-or-save path using one smallest-displacement legal refit, separately for output0/120. Pair purchases are measured within each shop and are not added to the forward path tree. Repeated inventories across paths intentionally remain separate."));
    Report->SetStringField(TEXT("choice_definition"),TEXT("Affordable direct/refit option means actual Buy, optional Unfit, Fit, Has and CanDepart succeed. Support prerequisite groups are same-family variations, not distinct mechanical build plans."));
    Report->SetStringField(TEXT("high_tier_definition"),TEXT("Catalogue price >=14. Sole eligible is based on current fitted capabilities and authored opportunity tags, without assuming it is a desirable aspiration."));
    Report->SetStringField(TEXT("pair_definition"),TEXT("An offered active and offered passive are both bought at actual prices and fitted using actual Buy/Unfit/Fit/Has/CanDepart operations; first-active refits include restoring an owned unfitted second active. Final rig retains both items within two active/four passive sockets. No sales, credits, invented stock or generator forecast helpers are used. Each ID pair counts once per shop, not once per refit."));
    Report->SetStringField(TEXT("conditional_definition"),TEXT("A support incompatible with the incoming fitted rig is conditional. It is valid only if at least one currently offered active yields an affordable legal pair. Currently compatible pairs are reported separately and may be unrelated same-family purchases, not new mechanical combinations. Dormant now means unsupported opportunity or invalid conditional offer; schema1 baseline used current incompatibility directly."));
    Report->SetNumberField(TEXT("exact_save_stock_checks"),Audit.SaveChecks);Report->SetNumberField(TEXT("no_reroll_checks"),Audit.NoRerollChecks);Report->SetNumberField(TEXT("structural_errors"),Audit.StructuralErrors);
    Report->SetObjectField(TEXT("counterexample_counts"),CountsJson(Audit.ExampleCounts));Report->SetArrayField(TEXT("counterexamples_first16_per_kind"),Audit.Examples);
    auto Groups=MakeShared<FJsonObject>();TArray<FString> Keys;Audit.Groups.GetKeys(Keys);Keys.Sort();
    for(const FString& Key:Keys)
    {
        const auto& G=Audit.Groups.FindChecked(Key);Groups->SetObjectField(Key,G.Json());
        AddInfo(FString::Printf(TEXT("SHOP AUDIT %s: %d samples, %d stocks, cash%d..%d, no-fit%d, no support/active alternative%d, offered-pair shops%d, conditional-pair shops%d, invalid conditional%d. Availability only."),*Key,G.Samples,G.Stocks.Num(),G.CashMin,G.CashMax,G.NoChoice,G.NoSupportAndActive,G.ShopsWithPair,G.ShopsWithConditionalPair,G.InvalidConditionalOffers));
    }
    Report->SetObjectField(TEXT("groups"),Groups);
    FString Encoded;const auto Writer=TJsonWriterFactory<>::Create(&Encoded);const bool Serialized=FJsonSerializer::Serialize(Report,Writer);
    const FString Directory=FPaths::Combine(FPaths::ProjectSavedDir(),TEXT("Automation/ShopChoices"));
    const FString Path=FPaths::Combine(Directory,TEXT("seed-audit.json"));
    const bool Written=Serialized&&IFileManager::Get().MakeDirectory(*Directory,true)&&FFileHelper::SaveStringToFile(Encoded,*Path);
    TestTrue(TEXT("The diagnostic report is emitted under ignored project Saved/Automation"),Written);
    AddInfo(TEXT("SHOP AUDIT REPORT: ")+FPaths::ConvertRelativePathToFull(Path));
    TestEqual(TEXT("Generated stock and budget transitions preserve actual prerequisites, implementation and exact saved stock"),Audit.StructuralErrors,0);
    return true;
}
#endif
