#if FOUNDRY_WITH_CAMPAIGN
#include "FoundryCampaignUI.h"
#include "FoundryCampaign.h"
#include "FoundryHost.h"
#include "FoundryPrecision.h"
#include "overkill/robots.hpp"
#include "Framework/Application/SlateApplication.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SScrollBox.h"
#include "Widgets/Layout/SSeparator.h"
#include "Widgets/Layout/SWrapBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SSpinBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/SOverlay.h"
#include <algorithm>

namespace
{
using CA = overkill::CampaignAction;
using CT = overkill::CampaignActionType;
FString T(const std::string& S) { return UTF8_TO_TCHAR(S.c_str()); }
const TCHAR* Material[] = {TEXT("Iron"), TEXT("Copper"), TEXT("Carbon"), TEXT("Glass"), TEXT("Circuit")};
FLinearColor Ink(.005f,.009f,.012f,.97f), Panel(.009f,.017f,.022f,.98f), Gold(.95f,.59f,.23f), Paper(.91f,.91f,.84f), Muted(.65f,.72f,.73f);
FString Materials(const overkill::Materials& M)
{
    FString S;
    for (int32 I=0; I<5; ++I) if (M[I]) S += FString::Printf(TEXT("%s %d   "), Material[I], M[I]);
    return S.IsEmpty() ? TEXT("No material cost") : S.TrimEnd();
}
FString Kind(overkill::Kind K)
{
    const TCHAR* Labels[] = {TEXT("Ammo"),TEXT("Shield"),TEXT("Spread"),TEXT("Utility"),TEXT("Magnet"),TEXT("Modifier")};
    return Labels[static_cast<int32>(K)];
}
FString Supplies(const overkill::Materials& M)
{
    FString S;for(int32 I=0;I<5;++I)S+=FString::Printf(TEXT("%s %d   "),Material[I],M[I]);return S.TrimEnd();
}
FString Cooldown(const overkill::Recipe& R)
{ return R.cooldown?FString::Printf(TEXT("Cooldown %d"),R.cooldown):TEXT("Cooldown: None · normally once per turn"); }
FString Payable(const overkill::Preview& P)
{
    if(!P.result.ok)return TEXT("Unavailable: ")+T(P.result.reason);
    overkill::Materials Paid{};int32 Hp=0,Heat=0,Shield=0;
    for(const auto& E:P.result.events)
    {
        if(E.type=="recipe_used")for(int32 I=0;I<5;++I)Paid[I]+=E.paid[I];
        else if(E.type=="hp_cost")Hp+=E.amount;else if(E.type=="heat_cost")Heat+=E.amount;else if(E.type=="shield_cost")Shield+=E.amount;
    }
    FString S=TEXT("Pay now: ")+Materials(Paid);
    if(Hp)S+=FString::Printf(TEXT(" · %d HP"),Hp);if(Heat)S+=FString::Printf(TEXT(" · %d Heat"),Heat);if(Shield)S+=FString::Printf(TEXT(" · %d Shield"),Shield);
    return S;
}
CA Action(CT Type, uint64 Subject=0, std::string Choice={}) { CA A; A.type=Type; A.subject=Subject; A.choice=std::move(Choice); return A; }

class SFoundryCampaignUI : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFoundryCampaignUI) {} SLATE_ARGUMENT(AFoundryStage*, Stage) SLATE_END_ARGS()
    void Construct(const FArguments& Args) { Stage=Args._Stage; Rebuild(); }
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual void Tick(const FGeometry& G, double Time, float Delta) override
    {
        SCompoundWidget::Tick(G,Time,Delta);
        if (Stage.IsValid() && Model().ViewRevision != SeenRevision) Rebuild();
    }
    virtual FReply OnKeyDown(const FGeometry&, const FKeyEvent& E) override
    {
        // A held Precision stop key must not become a new End Turn/Fire when
        // focus returns here after the modal closes. Commands need a new press.
        if(E.IsRepeat())return FReply::Handled();
        if(Model().bPrecisionModal)return FReply::Handled();
        const FKey K=E.GetKey();
        if (K==EKeys::Escape) { if(bExchange) {bExchange=false; ++Model().ViewRevision;} else Model().Close(); return FReply::Handled(); }
        if (K==EKeys::F3) { Stage->Control(TEXT("diagnostic")); return FReply::Handled(); }
        if (K==EKeys::F9) { Stage->CaptureView(); return FReply::Handled(); }
        if (Model().Page==TEXT("combat") && Model().Drawer.IsEmpty() && !Pending() && !Stage->IsPresentationBusy())
        {
            const TCHAR* Command=K==EKeys::C?TEXT("collect"):K==EKeys::P?TEXT("precision"):K==EKeys::L?TEXT("load"):K==EKeys::U?TEXT("unload"):K==EKeys::SpaceBar?TEXT("fire"):K==EKeys::Enter?TEXT("end"):nullptr;
            if(Command) {Stage->Control(Command);return FReply::Handled();}
        }
        return FReply::Unhandled();
    }
private:
    TWeakObjectPtr<AFoundryStage> Stage;
    uint64 SeenRevision=0, ChoiceId=0;
    TMap<FString,TSharedPtr<SScrollBox>> Scrolls;
    TMap<FString,float> ScrollOffsets;
    std::vector<overkill::Id> ChoiceObjects;
    overkill::Materials Removed{}, Added{};
    int32 SubscriptionMaterial=0;
    bool bExchange=false;
    uint64 SeenPrecisionAttempt=0;
    TSharedPtr<SWidget> PrecisionWidget;
    CA ExchangeAction;
    FFoundryCampaign& Model() const { return *Stage->GetCampaign(); }
    const overkill::Campaign* Current() const { return Model().Current(); }
    TSharedRef<SWidget> Text(const FString& S, int32 Size=18, FLinearColor Color=Paper) const
    { return SNew(STextBlock).Text(FText::FromString(S)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size)).ColorAndOpacity(Color).AutoWrapText(true); }
    TSharedRef<SWidget> Heading(const FString& S) const { return Text(S,30,Paper); }
    TSharedRef<SWidget> Button(const FString& Label, TFunction<void()> Fn, bool Enabled=true, bool Selected=false, const FString& Tip=TEXT(""))
    {
        return SNew(SButton).IsEnabled(Enabled).ContentPadding(FMargin(15,10)).ButtonColorAndOpacity(Selected?FLinearColor(.40f,.22f,.08f):FLinearColor(.10f,.17f,.19f))
            .ToolTipText(FText::FromString(Tip)).OnClicked_Lambda([this,Fn=MoveTemp(Fn)](){Fn();return FReply::Handled().SetUserFocus(SharedThis(this),EFocusCause::SetDirectly);})[Text(Label,18,Selected?Gold:Paper)];
    }
    TSharedRef<SWidget> Command(const FString& Label, CA A, bool Enabled=true, const FString& Tip=TEXT(""))
    { return Button(Label,[this,A](){Model().Apply(A);},Enabled,false,Tip); }
    TSharedRef<SWidget> Box(TSharedRef<SWidget> Child, FMargin Padding=FMargin(20)) const
    { return SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush("WhiteBrush")).BorderBackgroundColor(Panel).Padding(Padding)[Child]; }
    TSharedRef<SWidget> Scroll(const FString& Id, TSharedRef<SWidget> Child)
    {
        TSharedPtr<SScrollBox> Widget;
        TSharedRef<SScrollBox> Result=SAssignNew(Widget,SScrollBox).ScrollBarAlwaysVisible(true)+SScrollBox::Slot().Padding(0,0,14,0)[Child];
        Widget->SetScrollOffset(ScrollOffsets.FindRef(Id)); Scrolls.Add(Id,Widget); return Result;
    }
    void Add(TSharedRef<SVerticalBox> V, TSharedRef<SWidget> W, float Gap=10) { V->AddSlot().AutoHeight().Padding(0,0,0,Gap)[W]; }
    void Nav(const FString& Page) { bExchange=false; Model().Navigate(Page); }
    bool Pending() const
    {
        const auto* C=Current();
        return C && (!C->fight.upgradeChoices.empty() || std::any_of(C->upgradeOffers.begin(),C->upgradeOffers.end(),[](const auto& O){return !O.deferred;}));
    }
    TSharedRef<SWidget> RecipeFacts(const std::string& Id, const overkill::RecipeCopy* Copy=nullptr)
    {
        auto V=SNew(SVerticalBox);
        Add(V,Text(Model().ItemName(Id),23,Gold));
        Add(V,Text(Model().ItemDescription(Id),18));
        if(const auto* R=Model().Combat.Rules.recipe(Id))
        {Add(V,Text(Kind(R->kind)+TEXT(" · ")+Cooldown(*R),17,Muted));Add(V,Text(TEXT("Printed cost: ")+Materials(R->cost),18,Gold));}
        if(Copy)
        {
            Add(V,Text(FString::Printf(TEXT("Copy %llu · "),Copy->id)+(Copy->storage==overkill::MemoryKind::Utility?TEXT("Utility memory"):TEXT("General memory")),17,Muted));
            if(Copy->cooldown)Add(V,Text(FString::Printf(TEXT("Current cooldown: %d"),Copy->cooldown),17,Muted));
            for(const auto& Enhancement:Copy->tags)
            {
                Add(V,Text(TEXT("Copy enhancement · ")+Model().ItemName(Enhancement.source,true),17,Gold));
                Add(V,Text(Model().ItemDescription(Enhancement.source,true),17));
            }
        }
        return V;
    }
    TSharedRef<SWidget> ItemCard(const std::string& Id, bool Upgrade, const FString& Verb, TFunction<void()> Fn, bool Enabled=true)
    {
        auto V=SNew(SVerticalBox);
        if(Upgrade){Add(V,Text(Model().ItemName(Id,true),23,Gold));Add(V,Text(Model().ItemDescription(Id,true),18));}
        else Add(V,RecipeFacts(Id));
        Add(V,Button(Verb,MoveTemp(Fn),Enabled),0);
        return Box(V);
    }
    void Rebuild();
    TSharedRef<SWidget> Header();
    TSharedRef<SWidget> Title();
    TSharedRef<SWidget> Arrival();
    TSharedRef<SWidget> Route();
    TSharedRef<SWidget> Shop(bool EventShop);
    TSharedRef<SWidget> Memory(bool Crafting);
    TSharedRef<SWidget> Parts();
    TSharedRef<SWidget> Combat(bool ActionView);
    TSharedRef<SWidget> Rewards();
    TSharedRef<SWidget> Mystery();
    TSharedRef<SWidget> Upgrades();
    TSharedRef<SWidget> UpgradeDecision();
    TSharedRef<SWidget> Exchange(CA A, const FString& Name, bool Local);
};

void SFoundryCampaignUI::Rebuild()
{
    if (!Stage.IsValid() || !Stage->GetCampaign()) return;
    for(const auto& Pair:Scrolls) if(Pair.Value.IsValid()) ScrollOffsets.Add(Pair.Key,Pair.Value->GetScrollOffset());
    Scrolls.Empty(); SeenRevision=Model().ViewRevision;
    if(Model().bPrecisionModal)
    {
        TSharedRef<SWidget> Modal=SNullWidget::NullWidget;
        if(Model().PrecisionResult>=0)
        {
            auto Failure=SNew(SVerticalBox);Add(Failure,Heading(TEXT("Precision could not be saved")));
            Add(Failure,Text(Model().Message,21,Gold));
            Add(Failure,Text(TEXT("This measured result will not be rerolled here. Return to the title; Continue follows the saved encounter's normal restart rules."),20));
            Add(Failure,Button(TEXT("Return to title"),[this](){Model().LeaveFailedPrecision();}),0);Modal=Box(Failure);
        }
        else
        {
            if(!PrecisionWidget.IsValid() || SeenPrecisionAttempt!=Model().PrecisionAttempt)
            {
                SeenPrecisionAttempt=Model().PrecisionAttempt;
                FFoundryPrecisionOptions Options;
                Options.WidthPercent=overkill::upgradePrecisionWidthPercent(Current()->fight);
                Options.Title=Model().PrecisionChoice?TEXT("Precision retry"):TEXT("Precision grab");
                if(Model().PrecisionChoice)Options.BackLabel=TEXT("Back to retry choice");
                Options.Detail=FString::Printf(TEXT("Steering: %s. %s\n"),Material[Model().PrecisionSteering],Model().PrecisionChoice?TEXT("The original material choice is locked."):TEXT("Once per encounter."));
                const TCHAR* Outcomes[]={TEXT("Miss"),TEXT("Good"),TEXT("Perfect")};
                for(int32 I=0;I<3;++I)
                {
                    const auto P=Model().Combat.Preview(Model().PrecisionAction(I));
                    overkill::Materials Gain{};
                    for(int32 M=0;M<5;++M)Gain[M]=P.state.materials[M]-Model().Combat.State.materials[M];
                    Options.Detail+=FString(Outcomes[I])+TEXT(": ")+(P.result.ok?Supplies(Gain):T(P.result.reason))+TEXT("\n");
                }
                const auto WeakStage=Stage;const uint64 Attempt=SeenPrecisionAttempt;
                Options.OnResult=[WeakStage,Attempt](int32 Result){if(WeakStage.IsValid() && WeakStage->GetCampaign() && WeakStage->GetCampaign()->FinishPrecision(Attempt,Result))WeakStage->PlayPresentationCue(Result==0?TEXT("precision_miss"):TEXT("precision_good"));};
                Options.OnCancel=[WeakStage,Attempt](){if(WeakStage.IsValid() && WeakStage->GetCampaign())WeakStage->GetCampaign()->CancelPrecision(Attempt);};
                PrecisionWidget=MakeFoundryPrecisionWidget(MoveTemp(Options));
            }
            Modal=PrecisionWidget.ToSharedRef();
        }
        // Replace all other controls while the attempt is visible. Keep this
        // child instance across revisions so focus/UI changes cannot reset time.
        ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1600).HeightOverride(900).HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).WidthOverride(1040)[Modal]]]];
        SlatePrepass(GetCachedGeometry().Scale);
        FSlateApplication::Get().SetUserFocus(0,Modal,EFocusCause::SetDirectly);
        return;
    }
    const bool RestorePrecisionFocus=PrecisionWidget.IsValid();
    PrecisionWidget.Reset();
    TSharedRef<SWidget> Content=SNullWidget::NullWidget;
    const FString Page=Model().Page;
    if(Page==TEXT("title") || Page==TEXT("confirm-new")) Content=Title();
    else if(Stage->IsPresentationBusy()) Content=Combat(true);
    else if(Pending()) Content=Box(UpgradeDecision());
    else if(bExchange) Content=Box(Exchange(ExchangeAction,Model().ItemName(ExchangeAction.choice),true));
    else if(Page==TEXT("arrival")) Content=Box(Arrival());
    else if(Page==TEXT("route")) Content=Box(Route());
    else if(Page==TEXT("shop") || Page==TEXT("event-shop")) Content=Box(Shop(Page==TEXT("event-shop")));
    else if(Page==TEXT("memory")) Content=Box(Memory(false));
    else if(Page==TEXT("upgrades")) Content=Box(Upgrades());
    else if(Page==TEXT("combat")) Content=Combat(false);
    else if(Page==TEXT("rewards")) Content=Box(Rewards());
    else if(Page==TEXT("mystery")) Content=Box(Mystery());
    else
    {
        auto V=SNew(SVerticalBox);
        Add(V,Heading(Page==TEXT("defeat")?TEXT("Mara fell at Cinderwall"):TEXT("Cinderwall cleared")));
        Add(V,Text(Page==TEXT("defeat")?TEXT("This campaign has ended. Discovered recipes stay in your Collection."):TEXT("The city's twelve encounters are complete. Your discoveries remain in your profile.")));
        Add(V,Button(TEXT("Return to title"),[this](){Nav(TEXT("title"));}));
        Content=Box(V);
    }
    auto Frame=SNew(SVerticalBox);
    Frame->AddSlot().AutoHeight()[Header()];
    Frame->AddSlot().FillHeight(1).Padding(30,20,30,15)[Content];
    auto Footer=SNew(SHorizontalBox);
    Footer->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Text(Model().Message,17,Paper)];
    if(Page!=TEXT("title"))Footer->AddSlot().AutoWidth()[Button(TEXT("Menu  ·  Esc"),[this](){Model().Close();})];
    Frame->AddSlot().AutoHeight()[Box(Footer,FMargin(30,8))];
    if(Model().bDiagnostic)
    {
        auto V=SNew(SVerticalBox); Add(V,Text(TEXT("Diagnostics · F3 closes"),16,Gold));
        for(const auto& S:Model().Combat.RecentEvents) Add(V,Text(S,12),2);
        Frame->AddSlot().MaxHeight(180)[Box(Scroll(TEXT("diagnostic"),V))];
    }
    ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1600).HeightOverride(900)[Frame]]];
    // Tick happens after the ordinary prepass. Newly constructed pages need
    // their desired sizes before this frame's arrange/paint, not next frame.
    SlatePrepass(GetCachedGeometry().Scale);
    if(RestorePrecisionFocus)FSlateApplication::Get().SetUserFocus(0,SharedThis(this),EFocusCause::SetDirectly);
}

TSharedRef<SWidget> SFoundryCampaignUI::Header()
{
    auto H=SNew(SHorizontalBox);
    H->AddSlot().FillWidth(1).VAlign(VAlign_Center)[Text(TEXT("OVERKILL FOUNDRY"),23,Gold)];
    const auto* C=Current();
    if(C && Model().Page!=TEXT("title") && Model().Page!=TEXT("confirm-new"))
    {
        H->AddSlot().AutoWidth().Padding(12,0)[Text(FString::Printf(TEXT("MARA   %d / %d HP     SHIELD %d     HEAT %d     %d CR"),C->fight.hp,C->fight.maxHp,overkill::Rules::shield(C->fight),C->fight.heat,C->fight.credits),21)];
        H->AddSlot().AutoWidth().Padding(12,0)[Button(TEXT("Memory"),[this](){Nav(TEXT("memory"));},!Stage->IsPresentationBusy())];
        H->AddSlot().AutoWidth()[Button(TEXT("Upgrades"),[this](){Nav(TEXT("upgrades"));},!Stage->IsPresentationBusy())];
    }
    return Box(H,FMargin(30,15));
}
TSharedRef<SWidget> SFoundryCampaignUI::Title()
{
    auto V=SNew(SVerticalBox);
    Add(V,Text(TEXT("CINDERWALL"),48,Gold));
    Add(V,Text(TEXT("Build the next shot. Break the siege."),25));
    Add(V,Text(TEXT("Mara's forge turns recovered metal into ammunition and protection. Reach the end of the city with the build you earn."),20));
    if(Model().Page==TEXT("confirm-new"))
    {
        Add(V,Text(TEXT("Replace your unfinished campaign?"),25,Gold));
        Add(V,Text(TEXT("The current run will be replaced. Recipe discoveries and character unlocks stay in your profile.")));
        Add(V,Button(TEXT("Replace campaign and start"),[this](){Model().StartNew(true);}));
        Add(V,Button(TEXT("Keep current campaign"),[this](){Model().Page=TEXT("title");++Model().ViewRevision;}));
    }
    else
    {
        Add(V,Button(TEXT("New Game · Mara"),[this](){Model().StartNew();},!Model().bSaveUnavailable));
        Add(V,Button(TEXT("Continue"),[this](){Model().Continue();},Model().HasActiveSave()));
        if(Current()) Add(V,Text(FString::Printf(TEXT("Collection: %llu discovered recipes"),static_cast<uint64>(Current()->profile.recipes.size())),17,Muted));
        Add(V,Button(TEXT("Quit"),[](){FPlatformMisc::RequestExit(false);}));
    }
    return SNew(SHorizontalBox)+SHorizontalBox::Slot().AutoWidth().VAlign(VAlign_Center)[SNew(SBox).WidthOverride(540)[Box(V,FMargin(28))]]+SHorizontalBox::Slot().FillWidth(1)[SNullWidget::NullWidget];
}
TSharedRef<SWidget> SFoundryCampaignUI::Arrival()
{
    auto V=SNew(SVerticalBox); Add(V,Heading(TEXT("A gift from the Mayor")));
    Add(V,Text(TEXT("Choose one permanent system upgrade before entering Cinderwall."),20,Muted));
    auto H=SNew(SHorizontalBox);
    for(const auto& Id:Current()->mayorOffers) H->AddSlot().FillWidth(1).Padding(0,15,15,0)[ItemCard(Id,true,TEXT("Accept gift"),[this,Id](){Model().Apply(Action(CT::ChooseMayor,0,Id));})];
    V->AddSlot().FillHeight(1)[Scroll(TEXT("mayor"),H)]; return V;
}
TSharedRef<SWidget> SFoundryCampaignUI::Route()
{
    const auto& C=*Current(); auto V=SNew(SVerticalBox);
    Add(V,Heading(TEXT("Cinderwall · choose your next encounter")));
    auto Track=SNew(SHorizontalBox);
    for(int32 I=1;I<=12;++I) Track->AddSlot().FillWidth(1).Padding(0,0,7,0)[Box(Text(FString::Printf(TEXT("%02d%s"),I,I<C.route.position?TEXT("  ✓"):I==C.route.position?TEXT("  ◀"):TEXT("")),20,I==C.route.position?Gold:Muted),FMargin(10))];
    Add(V,Track); auto Row=SNew(SHorizontalBox);
    for(const auto& O:overkill::routeOffers(C.route))
    {
        auto Card=SNew(SVerticalBox);
        FString Label=O.kind==overkill::EncounterKind::Officer?TEXT("Officer"):O.kind==overkill::EncounterKind::Boss?TEXT("City boss"):O.kind==overkill::EncounterKind::Mystery?TEXT("Mystery"):TEXT("Regular");
        Add(Card,Text(Label,26,Gold));
        if(!O.district.empty()) Add(Card,Text(O.district=="C1-D-FOUNDRY"?TEXT("Foundry district"):O.district=="C1-D-PATROL"?TEXT("Patrol district"):TEXT("Workshop district"),18,Muted));
        if(O.kind!=overkill::EncounterKind::Mystery)
        {
            overkill::Id Next=1; const auto Enemies=overkill::makeCinderwallFormation(O.formation,O.encounterSeed,O.encounterKey,Next,O.binderDefaultSeen);
            for(const auto& E:Enemies) Add(Card,Text(T(E.name)+FString::Printf(TEXT(" · %d HP · %d Armor"),E.hp,E.armor)),7);
            Add(Card,Text(O.kind==overkill::EncounterKind::Officer?TEXT("Victory: cores, recipe analysis and an upgrade."):TEXT("Defeated robots leave energy cores and recipe analysis."),17,Muted));
        }
        else { const auto Reveal=Model().Rules.revealedMysteryCategory(C,O.id);Add(Card,Text(Reveal.empty()?TEXT("An uncharted opportunity. Its outcome is revealed when you enter."):TEXT("Survey: ")+T(Reveal),20)); }
        const auto Revealed=Model().Rules.revealedRecipe(C,O); if(!Revealed.empty()) Add(Card,Text(TEXT("Known reward: ")+Model().ItemName(Revealed),18,Gold));
        Add(Card,Command(TEXT("Enter encounter"),Action(CT::EnterOffer,O.id)));
        if(overkill::ownedUpgrade(C.fight,"MY1-12"))
        {
            Add(Card,Command(TEXT("Inspect alternative"),Action(CT::PreviewRouteReplacement,O.id)));
            for(const auto& P:C.routePreviews) if(P.id==O.id)
            {
                FString Description=TEXT("Mystery");
                if(P.kind!=overkill::EncounterKind::Mystery)
                {overkill::Id Next=1;Description.Empty();for(const auto& E:overkill::makeCinderwallFormation(P.formation,P.encounterSeed,P.encounterKey,Next,P.binderDefaultSeen))Description+=T(E.name)+TEXT("  ");}
                Add(Card,Text(TEXT("Alternative: ")+Description));Add(Card,Command(TEXT("Use route pass"),Action(CT::ReplaceRouteOffer,O.id)));
            }
        }
        Row->AddSlot().FillWidth(1).Padding(0,5,15,0)[Box(Card)];
    }
    V->AddSlot().FillHeight(1)[Scroll(TEXT("route"),Row)];
    Add(V,Button(TEXT("Visit the shop"),[this](){Nav(TEXT("shop"));}),0); return V;
}

TSharedRef<SWidget> SFoundryCampaignUI::Shop(bool EventShop)
{
    const auto& C=*Current(); auto V=SNew(SVerticalBox);
    Add(V,Heading(EventShop?TEXT("The exchange"):TEXT("Foundry supplies")));
    Add(V,Text(TEXT("Prices include your upgrades. Stock is finite; one purchase takes one item."),18,Muted));
    auto Stock=SNew(SVerticalBox);
    for(const auto& P:EventShop?C.eventShop:C.shop)
    {
        const int32 Price=Model().Hooks.productPrice(C,P);
        auto Card=SNew(SVerticalBox);
        const FString Name=P.kind==overkill::ProductKind::Material?FString(Material[P.material]):Model().ItemName(P.definition,P.kind==overkill::ProductKind::Upgrade);
        Add(Card,Text(Name+FString::Printf(TEXT("   %d CR   ·   %d left"),Price,P.quantity),22,Gold));
        if(P.kind!=overkill::ProductKind::Material) Add(Card,Text(Model().ItemDescription(P.definition,P.kind==overkill::ProductKind::Upgrade),17));
        CA A=Action(CT::Buy,P.id);A.eventShop=EventShop;
        Add(Card,Button(TEXT("Buy one"),[this,A,P](){
            if(P.kind==overkill::ProductKind::Recipe && !Model().Rules.hasFreeMemory(*Current(),P.definition))
            {ExchangeAction=A;ExchangeAction.choice=P.definition;bExchange=true;++Model().ViewRevision;}
            else Model().Apply(A);
        },P.quantity>0 && C.fight.credits>=Price),0);
        Add(Stock,Box(Card));
    }
    auto Sell=SNew(SVerticalBox); Add(Sell,Text(TEXT("Your energy cores"),24,Gold));
    if(C.cores.empty()) Add(Sell,Text(TEXT("Claim defeated robots' cores after combat, then sell them here."),18,Muted));
    for(const auto& Core:C.cores)
    {
        FString Name=T(Core.robot);for(const auto& D:overkill::cinderwallRobotDefinitions())if(Core.robot==D.id)Name=UTF8_TO_TCHAR(D.name);
        Add(Sell,Command(Name+FString::Printf(TEXT(" · sell for %d CR"),Model().Hooks.coreSaleValue(C,Core)),Action(CT::SellCore,Core.id)));
    }
    Add(Sell,Text(TEXT("Reserve parts"),24,Gold));
    for(const auto& P:C.fight.parts) if(P.place==overkill::Place::Reserve)
    {
        const auto Value=Model().Hooks.partSaleValue(C,P);
        Add(Sell,Command(T(P.output)+FString::Printf(TEXT(" · sell for %d CR"),Value),Action(CT::SellPart,P.id),Value>=0));
    }
    auto H=SNew(SHorizontalBox);
    H->AddSlot().FillWidth(2).Padding(0,0,24,0)[Scroll(TEXT("stock"),Stock)];
    H->AddSlot().FillWidth(1)[Scroll(TEXT("sell"),Sell)];
    V->AddSlot().FillHeight(1)[H]; Add(V,Button(TEXT("Close shop"),[this](){Model().Close();}),0); return V;
}

TSharedRef<SWidget> SFoundryCampaignUI::Memory(bool Crafting)
{
    const auto& C=*Current(); auto V=SNew(SVerticalBox);
    int32 General=0,Utility=0,Borrowed=0;for(const auto& Copy:C.fight.memory){if(Copy.storage==overkill::MemoryKind::General)++General;else if(Copy.storage==overkill::MemoryKind::Utility)++Utility;else ++Borrowed;}
    Add(V,Heading(Crafting?TEXT("Recipes"):TEXT("Recipe memory")));
    Add(V,Text(FString::Printf(TEXT("General %d / %d   ·   Utility %d / %d   ·   Borrowed %d"),General,C.memorySlots+overkill::upgradeGeneralMemoryBonus(C.fight),Utility,overkill::upgradeUtilityMemoryBonus(C.fight),Borrowed),18,Muted));
    if(Crafting)Add(V,Text(Supplies(C.fight.materials),19,Gold));
    auto List=SNew(SVerticalBox);auto Detail=SNew(SVerticalBox);
    if(!Model().FocusRecipe && !C.fight.memory.empty())Model().FocusRecipe=C.fight.memory.front().id;
    for(const auto& Copy:C.fight.memory)
    {
        const auto* R=Model().Combat.Rules.recipe(Copy.recipe); if(!R) continue;
        FString Label=Model().ItemName(Copy.recipe)+TEXT(" · ")+Kind(R->kind);
        if(Copy.cooldown)Label+=FString::Printf(TEXT(" · cooldown %d"),Copy.cooldown);
        const auto Id=Copy.id;
        Add(List,Button(Label,[this,Id](){Model().FocusRecipe=Id;++Model().ViewRevision;},true,Model().FocusRecipe==Id),7);
        if(Model().FocusRecipe!=Id)continue;
        Add(Detail,Text(Model().ItemName(Copy.recipe),28,Gold));
        Add(Detail,Text(Model().ItemDescription(Copy.recipe),21));
        Add(Detail,Text(TEXT("Printed cost: ")+Materials(R->cost),19,Gold));
        Add(Detail,Text(Kind(R->kind)+TEXT(" · ")+(R->kind==overkill::Kind::Utility?TEXT("Immediate effect"):FString::Printf(TEXT("Produces %d"),R->outputCount))+TEXT(" · ")+Cooldown(*R),18,Muted));
        if(Crafting)
        {
            const auto A=overkill::Action::craft(Id); const auto P=Model().Combat.Preview(A);
            if(P.result.ok)Add(Detail,Text(Payable(P),20,Gold));
            Add(Detail,Text(P.result.ok?TEXT("Ready to use"):T(P.result.reason),20,P.result.ok?Paper:Gold));
            Add(Detail,Button(TEXT("Use recipe"),[this,Id](){Stage->Control(FString::Printf(TEXT("craft:%llu"),Id));},P.result.ok));
            Add(Detail,Text(R->kind==overkill::Kind::Utility?TEXT("This Utility resolves immediately. It does not create a saved Utility part."):TEXT("Crafted parts go to your reserve. Install Shield parts, select ammunition, then Lock and load."),18,Muted));
        }
        else if(C.phase!=overkill::CityPhase::Fight && Copy.storage!=overkill::MemoryKind::Borrowed && overkill::upgradeUtilityMemoryBonus(C.fight)>0)
        {
            const bool ToUtility=Copy.storage==overkill::MemoryKind::General;
            Add(Detail,Command(ToUtility?TEXT("Move to Utility memory"):TEXT("Move to general memory"),Action(CT::MoveRecipeCopy,Copy.id,ToUtility?"utility":"general")));
        }
    }
    auto H=SNew(SHorizontalBox);
    H->AddSlot().FillWidth(1).Padding(0,0,24,0)[Scroll(TEXT("memory-list"),List)];
    H->AddSlot().FillWidth(1)[Scroll(TEXT("memory-detail"),Detail)];
    V->AddSlot().FillHeight(1)[H];
    Add(V,Button(TEXT("Close"),[this](){Model().Close();}),0); return V;
}

TSharedRef<SWidget> SFoundryCampaignUI::Parts()
{
    const auto& S=Model().Combat.State;auto V=SNew(SVerticalBox);
    Add(V,Heading(TEXT("Assemble the next shot")));
    Add(V,Text(TEXT("Select reserve Ammo and Spread parts for the bullet. Click Shield parts to install or remove them. Installed Shield protects automatically."),19,Muted));
    auto Reserve=SNew(SVerticalBox), Installed=SNew(SVerticalBox), Bullet=SNew(SVerticalBox);
    Add(Reserve,Text(TEXT("Reserve"),23,Gold));Add(Installed,Text(TEXT("Installed Shield"),23,Gold));Add(Bullet,Text(TEXT("Selected / loaded bullet"),23,Gold));
    for(const auto& P:S.parts)
    {
        const bool Selected=std::find(Model().Combat.Selection.begin(),Model().Combat.Selection.end(),P.id)!=Model().Combat.Selection.end();
        FString Name=T(P.output)+TEXT(" · ")+Kind(P.kind);
        if(P.place==overkill::Place::Installed)Name+=FString::Printf(TEXT(" · %d left"),P.shield);
        if(P.createdRound<S.round)Name+=TEXT(" · saved");
        const auto Id=P.id;auto Column=P.place==overkill::Place::Installed?Installed:P.place==overkill::Place::Loaded?Bullet:Reserve;
        FString Tip=Model().ItemDescription(P.recipe);
        if(P.kind==overkill::Kind::Shield)Tip+=TEXT("\n")+Payable(Model().Combat.Preview(P.place==overkill::Place::Installed?overkill::Action::remove(Id):overkill::Action::install(Id)));
        Add(Column,Button((Selected?TEXT("✓ "):TEXT(""))+Name,[this,Id](){Stage->Control(FString::Printf(TEXT("part:%llu"),Id));},true,Selected,Tip),7);
        if(Selected)Add(Bullet,Text(T(P.output),18),6);
        if(P.place==overkill::Place::Loaded && P.kind==overkill::Kind::Spread)
            Add(Bullet,Button(TEXT("Spread → ")+Model().Combat.NameFor(Model().Combat.SpreadTargets.FindRef(Id)),[this,Id](){Stage->Control(FString::Printf(TEXT("spread:%llu"),Id));}));
    }
    auto H=SNew(SHorizontalBox);
    H->AddSlot().FillWidth(1).Padding(0,0,15,0)[Scroll(TEXT("reserve"),Reserve)];
    H->AddSlot().FillWidth(1).Padding(0,0,15,0)[Scroll(TEXT("shield"),Installed)];
    H->AddSlot().FillWidth(1)[Scroll(TEXT("bullet"),Bullet)];
    V->AddSlot().FillHeight(1)[H];
    auto Buttons=SNew(SHorizontalBox);
    Buttons->AddSlot().FillWidth(1).Padding(0,0,10,0)[Button(TEXT("Lock and load"),[this](){Stage->Control(TEXT("load"));},Model().Combat.Preview(overkill::Action::load(Model().Combat.Selection)).result.ok)];
    Buttons->AddSlot().FillWidth(1).Padding(0,0,10,0)[Button(TEXT("Unload"),[this](){Stage->Control(TEXT("unload"));},!S.bullet.empty())];
    Buttons->AddSlot().FillWidth(1)[Button(Stage->IsActionView()?TEXT("Return to aiming"):TEXT("Return to preparation"),[this](){Model().Close();})];
    Add(V,Buttons,0);return V;
}

TSharedRef<SWidget> SFoundryCampaignUI::Combat(bool ActionView)
{
    if(!ActionView && Model().Drawer==TEXT("recipes"))return Box(Memory(true));
    if(!ActionView && Model().Drawer==TEXT("parts"))return Box(Parts());
    const auto& S=Model().Combat.State;auto V=SNew(SVerticalBox);
    auto Enemies=SNew(SHorizontalBox);
    int32 EnemyNumber=0;
    for(const auto& E:S.enemies)
    {
        ++EnemyNumber;
        if(E.dead||E.escaped)continue;
        auto Info=SNew(SVerticalBox);const auto Id=E.id;
        Add(Info,Text(FString::Printf(TEXT("%d · "),EnemyNumber)+T(E.name),19,Model().Combat.Target==Id?Gold:Paper),3);
        Add(Info,Text(FString::Printf(TEXT("%d / %d HP · %d Armor · %d Shield"),E.hp,E.maxHp,E.armor,E.shield),16),3);
        Add(Info,Text(T(overkill::Rules::intentText(E,S.round)),17,Gold),0);
        const FString Note=(E.definition!="C1-R01" && E.definition!="C1-R02")?TEXT("Temporary model. Name, status and committed intent describe this robot's actual rules."):TEXT("Select as the main target. Fire preview uses its actual defenses.");
        Enemies->AddSlot().FillWidth(1).Padding(5,0)[SNew(SButton).IsEnabled(!Stage->IsPresentationBusy()).ContentPadding(14).ButtonColorAndOpacity(Panel).ToolTipText(FText::FromString(Note)).OnClicked_Lambda([this,Id](){Stage->Control(FString::Printf(TEXT("target:%llu"),Id));return FReply::Handled().SetUserFocus(SharedThis(this),EFocusCause::SetDirectly);})[Info]];
    }
    V->AddSlot().AutoHeight().HAlign(HAlign_Right)[SNew(SBox).WidthOverride(1100)[Enemies]];
    V->AddSlot().FillHeight(1)[SNullWidget::NullWidget];
    if(ActionView) {Add(V,Box(Text(Stage->GetActionCaption(),23,Gold)),0);return V;}
    auto Controls=SNew(SVerticalBox);
    if(!Stage->IsActionView())Add(Controls,Text(FString::Printf(TEXT("ROUND %d    "),S.round)+Supplies(S.materials),20,Gold));
    if(S.phase==overkill::Phase::Collection)
    {
        Add(Controls,Text(TEXT("Collect behind the forge. Choose the material to steer toward."),20));
        auto Row=SNew(SHorizontalBox);
        for(int32 I=0;I<5;++I) Row->AddSlot().FillWidth(1).Padding(0,0,8,0)[Button(Material[I],[this,I](){Model().Combat.Steering=I;++Model().ViewRevision;},true,Model().Combat.Steering==I)];
        Row->AddSlot().FillWidth(1.5f).Padding(0,0,8,0)[Button(TEXT("Collect  ·  C"),[this](){Stage->Control(TEXT("collect"));})];
        const auto PrecisionPreview=Model().Combat.Preview(overkill::Action::collect(Model().Combat.Steering,0));
        Row->AddSlot().FillWidth(1.5f)[Button(TEXT("Precision  ·  P"),[this](){Stage->Control(TEXT("precision"));},PrecisionPreview.result.ok,false,T(PrecisionPreview.result.reason))];
        Add(Controls,Row,0);
    }
    else
    {
        const auto Fire=Model().Combat.FireAction();const auto P=Model().Combat.Preview(Fire);
        if(Stage->IsActionView())Add(Controls,Text(FString::Printf(TEXT("LOCKED AND LOADED · %llu %s · Main target: %s"),static_cast<uint64>(S.bullet.size()),S.bullet.size()==1?TEXT("part"):TEXT("parts"),*Model().Combat.NameFor(Model().Combat.Target)),19,Gold));
        Add(Controls,Text(Model().Combat.PreviewText(Fire),18,P.result.ok?Paper:Muted));
        auto Row=SNew(SHorizontalBox);
        Row->AddSlot().FillWidth(1).Padding(0,0,8,0)[Button(TEXT("Recipes"),[this](){Model().Drawer=TEXT("recipes");++Model().ViewRevision;})];
        Row->AddSlot().FillWidth(1).Padding(0,0,8,0)[Button(FString::Printf(TEXT("Parts · %llu loaded"),static_cast<uint64>(S.bullet.size())),[this](){Model().Drawer=TEXT("parts");++Model().ViewRevision;})];
        Row->AddSlot().FillWidth(1).Padding(0,0,8,0)[Button(TEXT("Shop"),[this](){Nav(TEXT("shop"));})];
        if(Stage->IsActionView())
        {
            Row->AddSlot().FillWidth(1).Padding(0,0,8,0)[Button(TEXT("Unload  ·  U"),[this](){Stage->Control(TEXT("unload"));},!S.bullet.empty())];
            Row->AddSlot().FillWidth(1.2f).Padding(0,0,8,0)[Button(TEXT("Fire  ·  Space"),[this](){Stage->Control(TEXT("fire"));},P.result.ok,true)];
        }
        else Row->AddSlot().FillWidth(1.4f).Padding(0,0,8,0)[Button(TEXT("Lock and load  ·  L"),[this](){Stage->Control(TEXT("load"));},Model().Combat.Preview(overkill::Action::load(Model().Combat.Selection)).result.ok,true)];
        Row->AddSlot().FillWidth(1.2f)[Button(TEXT("End Turn  ·  Enter"),[this](){Stage->Control(TEXT("end"));},Model().Combat.Preview(overkill::Action::endTurn()).result.ok,false,Model().Combat.PreviewText(overkill::Action::endTurn()))];
        Add(Controls,Row,0);
    }
    Add(V,Box(Controls,FMargin(20,14)),0);return V;
}

TSharedRef<SWidget> SFoundryCampaignUI::Exchange(CA A,const FString& Name,bool Local)
{
    auto V=SNew(SVerticalBox);Add(V,Heading(TEXT("Memory is full")));Add(V,Text(TEXT("Keep ")+Name+TEXT(" by replacing one permanent recipe copy. Existing parts are unchanged."),21));
    std::string Incoming=A.choice;const overkill::RecipeCopy* SourceCopy=nullptr;
    if(Incoming.empty() && A.target)for(const auto& Copy:Current()->fight.memory)if(Copy.id==A.target){Incoming=Copy.recipe;SourceCopy=&Copy;break;}
    auto Offered=SNew(SVerticalBox);Add(Offered,Text(TEXT("Incoming recipe"),21,Gold));Add(Offered,RecipeFacts(Incoming,SourceCopy));
    Add(Offered,Text(TEXT("The chosen copy and its enhancements leave memory. Compare both recipes before replacing it."),18,Muted));
    auto List=SNew(SVerticalBox);
    Add(List,Text(TEXT("Choose a copy to replace"),21,Gold));
    for(const auto& Copy:Current()->fight.memory)if(Copy.storage!=overkill::MemoryKind::Borrowed)
    {
        CA Select=A;Select.exchange=Copy.id;
        auto Card=SNew(SVerticalBox);Add(Card,RecipeFacts(Copy.recipe,&Copy));
        Add(Card,Button(TEXT("Replace ")+Model().CopyName(Copy.id),[this,Select](){if(Model().Apply(Select))bExchange=false;}),0);
        Add(List,Box(Card,FMargin(14)));
    }
    auto Columns=SNew(SHorizontalBox);
    Columns->AddSlot().FillWidth(1).Padding(0,0,24,0)[Scroll(TEXT("exchange-incoming"),Offered)];
    Columns->AddSlot().FillWidth(1.25f)[Scroll(TEXT("exchange"),List)];
    V->AddSlot().FillHeight(1)[Columns];
    Add(V,Button(TEXT("Cancel exchange"),[this,A,Local](){
        if(Local){bExchange=false;++Model().ViewRevision;}
        else if(A.type==CT::ResolveUpgradeOffer)Model().Apply(Action(CT::CancelUpgradeExchange,A.subject));
        else Model().Apply(Action(CT::BackRecipes));
    }),0);return V;
}

TSharedRef<SWidget> SFoundryCampaignUI::Rewards()
{
    const auto& C=*Current();auto V=SNew(SVerticalBox);Add(V,Heading(TEXT("Salvage secured")));
    if(C.skipConfirmation)
    {
        Add(V,Text(TEXT("Leave the unclaimed rewards behind?"),26,Gold));
        for(const auto& R:C.rewards)if(R.claim==overkill::ClaimState::Pending)Add(V,Text(R.kind==overkill::RewardKind::Cores?TEXT("Unclaimed energy cores"):R.kind==overkill::RewardKind::Recipe?TEXT("Unclaimed recipe analysis"):R.kind==overkill::RewardKind::Upgrade?TEXT("Unclaimed system upgrade"):TEXT("Unclaimed Credits")));
        Add(V,Command(TEXT("Return to rewards"),Action(CT::CancelAdvance)));Add(V,Command(TEXT("Leave them behind"),Action(CT::ConfirmAdvance)));return V;
    }
    if(C.recipeWindow)
    {
        if(!C.pendingRecipeChoice.empty())return Exchange(Action(CT::ClaimReward,C.recipeWindow,C.pendingRecipeChoice),Model().ItemName(C.pendingRecipeChoice),false);
        auto Row=SNew(SHorizontalBox);
        for(const auto& R:C.rewards)if(R.id==C.recipeWindow)for(const auto& Id:R.choices)
            Row->AddSlot().FillWidth(1).Padding(0,10,12,0)[ItemCard(Id,false,TEXT("Keep recipe"),[this,Id,Reward=R.id](){Model().Apply(Action(CT::ClaimReward,Reward,Id));})];
        V->AddSlot().FillHeight(1)[Scroll(TEXT("recipe-reward"),Row)];
        if(overkill::ownedUpgrade(C.fight,"MY1-14"))Add(V,Command(TEXT("Use Reward Spectrometer"),Action(CT::RerollRecipeReward,C.recipeWindow)));
        Add(V,Command(TEXT("Back to rewards"),Action(CT::BackRecipes)),0);return V;
    }
    auto List=SNew(SVerticalBox);
    for(const auto& R:C.rewards)
    {
        if(R.claim!=overkill::ClaimState::Pending)
        {
            const FString Name=R.kind==overkill::RewardKind::Cores?TEXT("Energy cores"):R.kind==overkill::RewardKind::Recipe?TEXT("Recipe analysis"):R.kind==overkill::RewardKind::Upgrade?TEXT("System upgrade"):TEXT("Credits");
            Add(List,Button(Name+(R.claim==overkill::ClaimState::Claimed?TEXT(" · Claimed"):TEXT(" · Left behind")),[](){},false));continue;
        }
        if(R.kind==overkill::RewardKind::Cores)Add(List,Command(FString::Printf(TEXT("Take %llu energy cores"),static_cast<uint64>(R.cores.size())),Action(CT::ClaimReward,R.id)));
        else if(R.kind==overkill::RewardKind::Credits)Add(List,Command(FString::Printf(TEXT("Take %d Credits"),R.credits),Action(CT::ClaimReward,R.id)));
        else if(R.kind==overkill::RewardKind::Recipe)Add(List,Command(TEXT("Analyze recipes · choose one"),Action(CT::OpenRecipes,R.id)));
        else for(const auto& Id:R.choices)Add(List,ItemCard(Id,true,TEXT("Take upgrade"),[this,Id,Reward=R.id](){Model().Apply(Action(CT::ClaimReward,Reward,Id));}));
    }
    const bool Unclaimed=std::any_of(C.rewards.begin(),C.rewards.end(),[](const auto& R){return R.claim==overkill::ClaimState::Pending;});
    V->AddSlot().FillHeight(1)[Scroll(TEXT("rewards"),List)];Add(V,Command(Unclaimed?TEXT("Skip remaining rewards"):TEXT("Continue through Cinderwall"),Action(CT::RequestAdvance)),0);return V;
}

TSharedRef<SWidget> SFoundryCampaignUI::Mystery()
{
    const auto& C=*Current();const auto* O=overkill::selectedRouteOffer(C.route);auto V=SNew(SVerticalBox);
    if(!O)return Text(TEXT("No Mystery is selected."));
    if(O->mystery=="C1-M-PATROL")
    {Add(V,Heading(TEXT("An unexpected patrol")));Add(V,Text(TEXT("Robots block the passage. Prepare to fight.")));Add(V,Command(TEXT("Face the patrol"),Action(CT::EnterPatrol)));}
    else if(O->mystery=="C1-M-EXCHANGE")
    {Add(V,Heading(TEXT("The backstreet exchange")));Add(V,Text(TEXT("A merchant has a small, fixed selection of uncommon supplies.")));Add(V,Button(TEXT("Inspect the merchant's stock"),[this](){Nav(TEXT("event-shop"));}));Add(V,Command(TEXT("Leave the exchange"),Action(CT::LeaveMystery)));}
    else
    {
        Add(V,Heading(TEXT("A dangerous calibration")));Add(V,Text(TEXT("Pay 8 HP for one offered upgrade. The calibration must leave you alive."),21,Gold));
        auto Choices=SNew(SVerticalBox);for(const auto& R:C.rewards)for(const auto& Id:R.choices)Add(Choices,ItemCard(Id,true,TEXT("Accept · pay 8 HP"),[this,Id](){Model().Apply(Action(CT::AcceptCalibration,0,Id));},C.fight.hp>8));
        V->AddSlot().FillHeight(1)[Scroll(TEXT("calibration"),Choices)];Add(V,Command(TEXT("Decline and leave"),Action(CT::LeaveMystery)),0);
    }
    return V;
}

TSharedRef<SWidget> SFoundryCampaignUI::Upgrades()
{
    auto V=SNew(SVerticalBox);Add(V,Heading(TEXT("Installed system upgrades")));auto List=SNew(SVerticalBox);
    for(const auto& U:Current()->fight.upgrades)
    {
        auto Card=SNew(SVerticalBox);Add(Card,Text(Model().ItemName(U.id,true),23,Gold));Add(Card,Text(Model().ItemDescription(U.id,true),19));
        if(U.charges)Add(Card,Text(FString::Printf(TEXT("%d charges remaining"),U.charges),18,Muted));
        if(U.id=="MY1-10" && Current()->phase==overkill::CityPhase::Fight)
            for(const auto& Copy:Current()->fight.memory)if(Copy.cooldown>0)
            {
                CA A=Action(CT::Combat);A.combat.type=overkill::ActionType::ActivateUpgrade;A.combat.upgrade=U.id;A.combat.subject=Copy.id;
                const auto Preview=Model().Combat.Preview(A.combat);
                Add(Card,Command(TEXT("Clear cooldown · ")+Model().CopyName(Copy.id),A,Preview.result.ok,T(Preview.result.reason)));
            }
        Add(List,Box(Card));
    }
    V->AddSlot().FillHeight(1)[Scroll(TEXT("upgrades"),List)];Add(V,Button(TEXT("Close"),[this](){Model().Close();}),0);return V;
}

TSharedRef<SWidget> SFoundryCampaignUI::UpgradeDecision()
{
    const auto& C=*Current();auto V=SNew(SVerticalBox);
    if(!C.fight.upgradeChoices.empty())
    {
        const auto Choice=C.fight.upgradeChoices.front();
        if(ChoiceId!=Choice.id){ChoiceId=Choice.id;ChoiceObjects.clear();Removed={};Added={};}
        Add(V,Heading(Model().ItemName(Choice.source,true)));Add(V,Text(Model().ItemDescription(Choice.source,true),20));
        CA Base=Action(CT::ResolveUpgradeChoice);Base.combat.type=overkill::ActionType::ResolveUpgradeChoice;Base.combat.upgradeChoice.choice=Choice.id;
        auto List=SNew(SVerticalBox);
        if(Choice.kind==overkill::UpgradeChoiceKind::Material)
            for(const auto Value:Choice.values){CA A=Base;A.combat.upgradeChoice.values={Value};Add(List,Command(Material[Value],A));}
        else if(Choice.kind==overkill::UpgradeChoiceKind::Pack)
            for(const auto& Option:Choice.options){CA A=Base;A.combat.upgradeChoice.option=Option;Add(List,Command(Option=="assault"?TEXT("Assault pack"):TEXT("Defence pack"),A));}
        else if(Choice.kind==overkill::UpgradeChoiceKind::Enemy)
            for(const auto Id:Choice.objects){CA A=Base;A.combat.upgradeChoice.objects={Id};Add(List,Command(Model().Combat.NameFor(Id),A));}
        else if(Choice.kind==overkill::UpgradeChoiceKind::RecipeCopies)
        {
            Add(List,Text(FString::Printf(TEXT("Choose %d–%d recipe copies."),Choice.minimum,Choice.maximum),21,Gold));
            for(const auto Id:Choice.objects){const bool Selected=std::find(ChoiceObjects.begin(),ChoiceObjects.end(),Id)!=ChoiceObjects.end();Add(List,Button(Model().CopyName(Id),[this,Id](){const auto It=std::find(ChoiceObjects.begin(),ChoiceObjects.end(),Id);if(It==ChoiceObjects.end())ChoiceObjects.push_back(Id);else ChoiceObjects.erase(It);++Model().ViewRevision;},true,Selected));}
            CA A=Base;A.combat.upgradeChoice.objects=ChoiceObjects;Add(List,Command(TEXT("Confirm copies"),A,ChoiceObjects.size()>=static_cast<size_t>(Choice.minimum)&&ChoiceObjects.size()<=static_cast<size_t>(Choice.maximum)));
        }
        else if(Choice.kind==overkill::UpgradeChoiceKind::HaulExchange)
        {
            Add(List,Text(TEXT("Exchange up to three base-haul units, one for one. Select units to give and receive."),20));
            for(int32 I=0;I<5;++I)
            {
                auto Row=SNew(SHorizontalBox);Row->AddSlot().FillWidth(1)[Text(FString::Printf(TEXT("%s · base haul %d"),Material[I],Choice.baseHaul[I]))];
                Row->AddSlot().FillWidth(1)[Button(FString::Printf(TEXT("Give %d  +"),Removed[I]),[this,I](){Removed[I]=(Removed[I]+1)%4;++Model().ViewRevision;})];
                Row->AddSlot().FillWidth(1)[Button(FString::Printf(TEXT("Receive %d  +"),Added[I]),[this,I](){Added[I]=(Added[I]+1)%4;++Model().ViewRevision;})];Add(List,Row);
            }
            CA A=Base;A.combat.upgradeChoice.removed=Removed;A.combat.upgradeChoice.added=Added;Add(List,Command(TEXT("Confirm exchange"),A));
        }
        else if(Choice.kind==overkill::UpgradeChoiceKind::PrecisionRetry)
        {
            Add(List,Text(TEXT("Keep the original haul, or make the saved extra Precision attempt. Its material choice stays locked."),21));
            Add(List,Button(TEXT("Try Precision again"),[this,Id=Choice.id](){Model().BeginPrecision(Id);}));
        }
        V->AddSlot().FillHeight(1)[Scroll(TEXT("upgrade-choice"),List)];
        if(Choice.optional){CA A=Base;A.combat.upgradeChoice.decline=true;Add(V,Command(TEXT("Keep current result / skip"),A),0);}return V;
    }
    for(const auto& O:C.upgradeOffers)if(!O.deferred)
    {
        Add(V,Heading(Model().ItemName(O.source,true)));Add(V,Text(Model().ItemDescription(O.source,true),20));
        CA Base=Action(CT::ResolveUpgradeOffer,O.id);
        if(!O.selected.empty())
        {
            if(O.kind==overkill::UpgradeRequestKind::CopyRecipe)Base.target=FCString::Strtoui64(*T(O.selected),nullptr,10);else Base.choice=O.selected;
            return Exchange(Base,O.kind==overkill::UpgradeRequestKind::CopyRecipe?Model().CopyName(Base.target):Model().ItemName(O.selected),false);
        }
        auto List=SNew(SVerticalBox);
        if(O.kind==overkill::UpgradeRequestKind::Subscription)
        {
            Add(List,Text(TEXT("Choose a material discount and a permanent recipe copy."),21,Gold));auto Row=SNew(SHorizontalBox);
            for(int32 I=0;I<5;++I)Row->AddSlot().FillWidth(1).Padding(0,0,8,0)[Button(Material[I],[this,I](){SubscriptionMaterial=I;++Model().ViewRevision;},true,SubscriptionMaterial==I)];Add(List,Row);
        }
        if(O.kind==overkill::UpgradeRequestKind::CopyRecipe || O.kind==overkill::UpgradeRequestKind::Subscription)
            for(const auto Id:O.copies){CA A=Base;A.target=Id;A.material=SubscriptionMaterial;Add(List,Command(Model().CopyName(Id),A));}
        else if(O.index>=0 && O.index<static_cast<int32>(O.candidates.size()))
        {
            Add(List,Text(FString::Printf(TEXT("Choice %d of %llu"),O.index+1,static_cast<uint64>(O.candidates.size())),18,Muted));
            for(const auto& Id:O.candidates[O.index])Add(List,ItemCard(Id,O.kind==overkill::UpgradeRequestKind::UpgradeOffer,TEXT("Accept"),[this,Base,Id](){CA A=Base;A.choice=Id;Model().Apply(A);}));
        }
        V->AddSlot().FillHeight(1)[Scroll(TEXT("upgrade-offer"),List)];
        if(O.optional){CA A=Base;A.decline=true;Add(V,Command(O.creditAlternative?FString::Printf(TEXT("Keep %d Credits instead"),O.creditAlternative):TEXT("Skip this offer"),A),0);}return V;
    }
    return Text(TEXT("Your saved upgrade selection is complete."));
}
}
TSharedRef<SWidget> MakeFoundryCampaignUI(AFoundryStage* Stage) { return SNew(SFoundryCampaignUI).Stage(Stage); }
#endif
