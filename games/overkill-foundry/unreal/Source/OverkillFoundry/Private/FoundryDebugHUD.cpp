#include "FoundryHost.h"
#include "FoundrySession.h"
#include "FoundryCampaignUI.h"
#include "FoundryRobot.h"

#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "Widgets/SWidget.h"
#include "EngineUtils.h"
#include <algorithm>

namespace
{
AFoundryStage* StageIn(UWorld* World)
{
    for (TActorIterator<AFoundryStage> It(World); It; ++It) return *It;
    return nullptr;
}
FString Text(const std::string& Value) { return UTF8_TO_TCHAR(Value.c_str()); }
const TCHAR* MaterialNames[] = { TEXT("Iron"), TEXT("Copper"), TEXT("Carbon"), TEXT("Glass"), TEXT("Circuit") };
const TCHAR* MaterialShort[] = { TEXT("Fe"), TEXT("Cu"), TEXT("C"), TEXT("Gl"), TEXT("Ci") };
FString MaterialsText(const overkill::Materials& Materials)
{
    FString Result;
    for (int32 Index = 0; Index < 5; ++Index)
        if (Materials[Index]) Result += FString::Printf(TEXT("%s %d  "), MaterialShort[Index], Materials[Index]);
    return Result.IsEmpty() ? TEXT("No materials") : Result.TrimEnd();
}
FString KindText(overkill::Kind Kind)
{
    switch (Kind)
    {
    case overkill::Kind::Ammo: return TEXT("Ammo");
    case overkill::Kind::Shield: return TEXT("Shield");
    case overkill::Kind::Spread: return TEXT("Spread");
    case overkill::Kind::Utility: return TEXT("Utility");
    case overkill::Kind::Magnet: return TEXT("Magnet");
    default: return TEXT("Modifier");
    }
}
FString EffectText(const overkill::Recipe& Recipe)
{
    FString Result = Text(Recipe.output) + TEXT(" / ") + KindText(Recipe.kind) + TEXT(" / ");
    for (const auto& Effect : Recipe.effects)
    {
        const TCHAR* Label = TEXT("Effect");
        using Op = overkill::Op;
        switch (Effect.op)
        {
        case Op::FlatDamage: Label = TEXT("Damage"); break;
        case Op::AttackIntentBonus: Label = TEXT("Bonus vs attacking intent"); break;
        case Op::PercentDamage: Label = TEXT("Damage percent"); break;
        case Op::ShieldValue: Label = TEXT("Shield on first install"); break;
        case Op::Heat: Label = TEXT("Heat"); break;
        case Op::HeatCost: Label = TEXT("Heat cost"); break;
        case Op::HpCost: Label = TEXT("HP cost"); break;
        case Op::Cool: Label = TEXT("Cooling"); break;
        case Op::ShieldGrant: Label = TEXT("Shield grant"); break;
        case Op::DelayedShield: Label = TEXT("Delayed Shield"); break;
        case Op::DelayedHeat: Label = TEXT("Delayed Heat"); break;
        case Op::DelayedHaul: Label = TEXT("Next haul bonus"); break;
        case Op::BurnIfHeat: Label = TEXT("Conditional Burn"); break;
        case Op::Burn: Label = TEXT("Burn"); break;
        case Op::Heal: Label = TEXT("Heal"); break;
        case Op::SpreadPercent: Label = TEXT("Spread percent"); break;
        case Op::BurnWard: Label = TEXT("Burn ward"); break;
        }
        Result += FString::Printf(TEXT("%s %d"), Label, Effect.amount);
        if (Effect.threshold) Result += FString::Printf(TEXT(" (threshold %d)"), Effect.threshold);
        Result += TEXT("; ");
    }
    return Result;
}
}

void AFoundryHUD::DrawHUD()
{
    Super::DrawHUD();
    AFoundryStage* Stage = StageIn(GetWorld());
    if (!Canvas || !GEngine || !Stage) return;
#if FOUNDRY_WITH_CAMPAIGN
    if (const auto* Campaign = Stage->GetCampaign(); Campaign && Campaign->Page == TEXT("combat") && Campaign->Drawer.IsEmpty() && !Campaign->bPrecisionModal)
    {
        const auto& S = Stage->GetSession().State;
        const float Scale = FMath::Min(Canvas->SizeX / 1600.f, Canvas->SizeY / 900.f);
        int32 Number = 0;
        for (const auto& Enemy : S.enemies)
        {
            ++Number;
            if (Enemy.dead || Enemy.escaped) continue;
            for (TActorIterator<AFoundryRobot> Robot(GetWorld()); Robot; ++Robot)
            {
                if (Robot->GetCoreId() != Enemy.id) continue;
                const FBox Bounds = Robot->GetBodyBounds();
                FVector2D Min(MAX_flt,MAX_flt), Max(-MAX_flt,-MAX_flt);
                for(int32 Corner=0;Corner<8;++Corner)
                {
                    const FVector P=Project(FVector(Corner&1?Bounds.Max.X:Bounds.Min.X,Corner&2?Bounds.Max.Y:Bounds.Min.Y,Corner&4?Bounds.Max.Z:Bounds.Min.Z));
                    Min.X=FMath::Min(Min.X,P.X);Min.Y=FMath::Min(Min.Y,P.Y);Max.X=FMath::Max(Max.X,P.X);Max.Y=FMath::Max(Max.Y,P.Y);
                }
                const FVector Position(Max.X+20*Scale,(Min.Y+Max.Y)*.5,0);
                const bool Selected = Stage->GetSession().Target == Enemy.id;
                const FLinearColor Color = Selected ? FLinearColor(1, .65f, .22f) : FLinearColor(.85f, .9f, .9f);
                DrawRect(FLinearColor(.015f,.025f,.03f,.94f), Position.X - 13 * Scale, Position.Y - 3 * Scale, 28 * Scale, 25 * Scale);
                DrawText(FString::FromInt(Number), Color.ToFColor(true), Position.X - 6 * Scale, Position.Y - 2 * Scale, GEngine->GetMediumFont(), Scale);
                if (Selected && Stage->IsActionView() && !Stage->IsPresentationBusy())
                {
                    const FVector Aim = Project(Robot->GetImpactLocation());
                    for (float Sign : {-1.f, 1.f})
                    {
                        DrawLine(Aim.X + 12 * Scale * Sign, Aim.Y, Aim.X + 22 * Scale * Sign, Aim.Y, Color, 2 * Scale);
                        DrawLine(Aim.X, Aim.Y + 12 * Scale * Sign, Aim.X, Aim.Y + 22 * Scale * Sign, Color, 2 * Scale);
                    }
                }
            }
        }
    }
#endif
#if FOUNDRY_WITH_CAMPAIGN
    if (!Stage->IsTechnicalMode())
    {
        if (!CampaignWidget.IsValid() && GEngine->GameViewport)
        {
            CampaignWidget = MakeFoundryCampaignUI(Stage);
            GEngine->GameViewport->AddViewportWidgetContent(CampaignWidget.ToSharedRef(), 10);
        }
        return;
    }
#endif
    FFoundrySession& Session = Stage->GetSession();
    const auto& State = Session.State;
    const float Scale = FMath::Min(Canvas->SizeX / 1600.0f, Canvas->SizeY / 900.0f);
    const float OffsetX = (Canvas->SizeX - 1600.0f * Scale) * 0.5f;
    const float OffsetY = (Canvas->SizeY - 900.0f * Scale) * 0.5f;
    const FLinearColor Gold(0.97f, 0.75f, 0.40f), White(0.88f, 0.94f, 0.96f), Dim(0.48f, 0.58f, 0.63f);
    const FLinearColor Panel(0.012f, 0.02f, 0.028f, 0.94f);
    auto Rect = [&](float X, float Y, float W, float H, FLinearColor Color)
    { DrawRect(Color, OffsetX + X * Scale, OffsetY + Y * Scale, W * Scale, H * Scale); };
    auto Label = [&](const FString& Value, float X, float Y, FLinearColor Color, float Size = 1.0f)
    { DrawText(Value, Color, OffsetX + X * Scale, OffsetY + Y * Scale, GEngine->GetMediumFont(), Scale * Size); };
    auto Button = [&](const FString& Command, const FString& Value, float X, float Y, float W, float H, bool bEnabled = true, bool bSelected = false)
    {
        const bool bHover = HoverCommand == Command;
        Rect(X, Y, W, H, bSelected ? FLinearColor(0.22f, 0.15f, 0.065f, 0.98f) : bEnabled ? FLinearColor(0.07f, 0.14f, 0.18f, 0.97f) : FLinearColor(0.035f, 0.055f, 0.065f, 0.97f));
        Rect(X, Y + H - 2, W, 2, bHover || bSelected ? Gold : FLinearColor(0.15f, 0.25f, 0.29f));
        Label(Value, X + 10, Y + 8, bEnabled ? White : Dim);
        // Disabled actions retain a hover region and report the core rejection on click.
        AddHitBox(FVector2D(OffsetX + X * Scale, OffsetY + Y * Scale), FVector2D(W * Scale, H * Scale), FName(*Command), true, Command.StartsWith(TEXT("spread:")) ? 1 : 0);
    };

    Rect(0, 0, 1600, 132, Panel);
    Label(TEXT("OVERKILL FOUNDRY  /  MARA AT CINDERWALL"), 24, 18, Gold, 1.2f);
    Label(TEXT("Cinderwall-v001 early art integration - debug HUD - city and campaign pending"), 720, 22, Dim);
    Label(FString::Printf(TEXT("ROUND %d     HP %d / %d     SHIELD %d     HEAT %d     CREDITS %d"), State.round, State.hp, State.maxHp, overkill::Rules::shield(State), State.heat, State.credits), 24, 53, White, 1.15f);
    Label(FString::Printf(TEXT("IRON %d     COPPER %d     CARBON %d     GLASS %d     CIRCUIT %d"), State.materials[0], State.materials[1], State.materials[2], State.materials[3], State.materials[4]), 24, 91, Gold);
    Label(TEXT("H panels  |  1 / 2 cameras  |  F9 capture  |  Esc exit"), 1030, 92, Dim);
    if (!Session.bShowPanels) return;

    Rect(18, 150, 360, 646, Panel);
    Label(TEXT("RECIPES  /  click to use"), 30, 166, Gold);
    Label(TEXT("Hover for effect and core rejection reason"), 30, 193, Dim, 0.9f);
    FString HoverDetail, HoverReason;
    for (size_t Index = 0; Index < State.memory.size(); ++Index)
    {
        const auto& Copy = State.memory[Index];
        const auto* Recipe = Session.Rules.recipe(Copy.recipe);
        if (!Recipe) continue;
        const FString Command = FString::Printf(TEXT("craft:%llu"), Copy.id);
        const auto Action = overkill::Action::craft(Copy.id);
        const auto Preview = Session.Preview(Action);
        const float Y = 222 + static_cast<float>(Index) * 46;
        FString Name = Text(Recipe->name);
        if (Copy.cooldown) Name += FString::Printf(TEXT("  [CD %d]"), Copy.cooldown);
        Button(Command, Name, 28, Y, 340, 42, Preview.result.ok);
        Label(MaterialsText(Recipe->cost), 39, Y + 24, Dim, 0.78f);
        if (HoverCommand == Command) { HoverDetail = EffectText(*Recipe); HoverReason = Preview.result.ok ? TEXT("Ready to use") : Text(Preview.result.reason); }
    }

    for (size_t Index = 0; Index < State.enemies.size(); ++Index)
    {
        const auto& Enemy = State.enemies[Index];
        const float X = 394 + static_cast<float>(Index) * 405;
        const FString Name = Text(Enemy.name) + (Enemy.dead ? TEXT("  DEFEATED") : Enemy.escaped ? TEXT("  ESCAPED") : TEXT(""));
        Button(FString::Printf(TEXT("target:%llu"), Enemy.id), Name, X, 150, 389, 98, !Enemy.dead && !Enemy.escaped, Session.Target == Enemy.id);
        Label(FString::Printf(TEXT("HP %d / %d   Armor %d   Shield %d"), Enemy.hp, Enemy.maxHp, Enemy.armor, Enemy.shield), X + 10, 185, White);
        Label(Text(overkill::Rules::intentText(Enemy, State.round)), X + 10, 218, Gold);
    }

    Rect(1216, 266, 366, 530, Panel);
    Label(TEXT("PARTS  /  click to select, install or use"), 1228, 282, Gold);
    Label(TEXT("Click installed Shield again to remove"), 1228, 309, Dim, 0.9f);
    constexpr int32 PageSize = 7;
    const int32 PageCount = FMath::Max(1, (static_cast<int32>(State.parts.size()) + PageSize - 1) / PageSize);
    Session.InventoryPage = FMath::Clamp(Session.InventoryPage, 0, PageCount - 1);
    for (int32 Slot = 0; Slot < PageSize; ++Slot)
    {
        const size_t Index = static_cast<size_t>(Session.InventoryPage * PageSize + Slot);
        if (Index >= State.parts.size()) break;
        const auto& Part = State.parts[Index];
        const bool bSelected = std::find(Session.Selection.begin(), Session.Selection.end(), Part.id) != Session.Selection.end();
        const float Y = 344 + Slot * 53.0f;
        const FString Command = FString::Printf(TEXT("part:%llu"), Part.id);
        Button(Command, FString::Printf(TEXT("%s#%llu %s"), bSelected ? TEXT("+ ") : TEXT(""), Part.id, *Text(Part.output)), 1227, Y, 344, 49, true, bSelected || Part.place == overkill::Place::Installed);
        FString Detail = Session.DescribePart(Part);
        const int32 Slash = Detail.Find(TEXT(" / "));
        if (Slash >= 0) Detail = Detail.Mid(Slash + 3);
        if (Part.kind == overkill::Kind::Spread && Part.place == overkill::Place::Loaded)
            Detail += TEXT(" -> ") + Session.NameFor(Session.SpreadTargets.FindRef(Part.id));
        Label(Detail.Left(48), 1238, Y + 28, Dim, 0.77f);
        if (HoverCommand == Command)
        {
            HoverDetail = Session.DescribePart(Part);
            if (Part.kind == overkill::Kind::Shield)
                HoverReason = Session.PreviewText(Part.place == overkill::Place::Installed ? overkill::Action::remove(Part.id) : overkill::Action::install(Part.id));
            else if (Part.kind == overkill::Kind::Modifier || Part.kind == overkill::Kind::Magnet)
            {
                overkill::Action Activate; Activate.type = overkill::ActionType::Activate; Activate.subject = Part.id;
                HoverReason = Session.PreviewText(Activate);
            }
        }
        if (Part.kind == overkill::Kind::Spread && Part.place == overkill::Place::Loaded)
            Button(FString::Printf(TEXT("spread:%llu"), Part.id), TEXT(">"), 1536, Y, 35, 24);
    }
    Button(TEXT("page:-1"), TEXT("Previous"), 1227, 746, 100, 34);
    Label(FString::Printf(TEXT("%d / %d"), Session.InventoryPage + 1, PageCount), 1350, 755, Dim);
    Button(TEXT("page:1"), TEXT("Next"), 1448, 746, 123, 34);

    const auto Fire = Session.FireAction();
    const auto FirePreview = Session.Preview(Fire);
    Rect(394, 265, 806, 66, Panel);
    Label(FString::Printf(TEXT("BULLET: %llu selected / %llu loaded     MAIN: %s"), static_cast<uint64>(Session.Selection.size()), static_cast<uint64>(State.bullet.size()), *Session.NameFor(Session.Target)), 406, 279, Gold);
    Label(Session.PreviewText(Fire).Left(102), 406, 309, FirePreview.result.ok ? White : Dim, 0.9f);

    if (State.phase == overkill::Phase::Victory || State.phase == overkill::Phase::Defeat || State.phase == overkill::Phase::Escaped)
    {
        Rect(505, 375, 578, 120, Panel);
        const FString Outcome = State.phase == overkill::Phase::Victory ? TEXT("VICTORY") : State.phase == overkill::Phase::Defeat ? TEXT("MARA DEFEATED") : TEXT("ENEMIES ESCAPED");
        Label(Outcome, 535, 395, Gold, 1.5f);
        Label(TEXT("Technical encounter complete. R restarts the same seed."), 535, 445, White);
    }
    Rect(394, 545, 806, 137, FLinearColor(0.012f, 0.02f, 0.028f, 0.80f));
    Label(TEXT("LATEST AUTHORITATIVE EVENTS"), 407, 559, Dim, 0.9f);
    const int32 StartEvent = FMath::Max(0, Session.RecentEvents.Num() - 4);
    for (int32 Index = StartEvent; Index < Session.RecentEvents.Num(); ++Index)
        Label(Session.RecentEvents[Index].Left(100), 407, 588 + (Index - StartEvent) * 21.0f, White, 0.92f);

    Button(TEXT("steer"), FString::Printf(TEXT("Tab  Steer: %s"), MaterialNames[Session.Steering]), 394, 698, 228, 42);
    const FString PrecisionLabel = Session.Precision < 0 ? TEXT("automatic") : FString::Printf(TEXT("test +%d"), Session.Precision);
    Button(TEXT("precision"), TEXT("P  Precision: ") + PrecisionLabel, 632, 698, 258, 42, !State.precisionSpent);
    const auto Collect = overkill::Action::collect(Session.Steering, Session.Precision);
    Button(TEXT("collect"), TEXT("C  Collect"), 900, 698, 300, 42, Session.Preview(Collect).result.ok);
    Button(TEXT("load"), TEXT("L  Load selected"), 394, 750, 218, 46, Session.Preview(overkill::Action::load(Session.Selection)).result.ok);
    overkill::Action Unload; Unload.type = overkill::ActionType::Unload;
    Button(TEXT("unload"), TEXT("U  Unload"), 622, 750, 142, 46, Session.Preview(Unload).result.ok);
    Button(TEXT("fire"), TEXT("Space  Fire"), 774, 750, 172, 46, FirePreview.result.ok);
    Button(TEXT("end"), TEXT("Enter  End Turn"), 956, 750, 244, 46, Session.Preview(overkill::Action::endTurn()).result.ok);
    Rect(0, 809, 1600, 91, Panel);
    Label(Session.Message.Left(180), 24, 821, White);
    Label(HoverDetail.Left(188), 24, 849, Gold, 0.91f);
    Label(HoverReason.Left(155), 24, 874, Dim, 0.88f);
    Label(TEXT("R  Restart same seed"), 1370, 873, Dim, 0.88f);
}

void AFoundryHUD::NotifyHitBoxClick(FName BoxName)
{
    if (AFoundryStage* Stage = StageIn(GetWorld())) Stage->Control(BoxName.ToString());
}
void AFoundryHUD::NotifyHitBoxBeginCursorOver(FName BoxName) { HoverCommand = BoxName.ToString(); }
void AFoundryHUD::NotifyHitBoxEndCursorOver(FName BoxName) { if (HoverCommand == BoxName.ToString()) HoverCommand.Empty(); }
void AFoundryHUD::EndPlay(const EEndPlayReason::Type Reason)
{
    if(CampaignWidget.IsValid() && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(CampaignWidget.ToSharedRef());
    CampaignWidget.Reset();
    Super::EndPlay(Reason);
}
