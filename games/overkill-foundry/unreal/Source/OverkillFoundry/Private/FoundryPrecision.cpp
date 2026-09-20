#include "FoundryPrecision.h"
#include "../../../../presentation/precision.hpp"

#include "Framework/Application/SlateApplication.h"
#include "Input/Reply.h"
#include "InputCoreTypes.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SLeafWidget.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Text/STextBlock.h"

DEFINE_LOG_CATEGORY_STATIC(LogFoundryPrecision, Log, All);

namespace
{
using foundry::presentation::PrecisionAttempt;
using foundry::presentation::PrecisionStage;

class SFoundryPrecisionMeter : public SLeafWidget
{
public:
    SLATE_BEGIN_ARGS(SFoundryPrecisionMeter) {}
        SLATE_ATTRIBUTE(double, Position)
        SLATE_ARGUMENT(double, GoodHalf)
        SLATE_ARGUMENT(double, PerfectHalf)
    SLATE_END_ARGS()
    void Construct(const FArguments& A) { Position=A._Position; Good=A._GoodHalf; Perfect=A._PerfectHalf; }
    virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D(680,90); }
    virtual int32 OnPaint(const FPaintArgs&, const FGeometry& G, const FSlateRect&, FSlateWindowElementList& Out,
                         int32 Layer, const FWidgetStyle&, bool) const override
    {
        const FVector2D Size=G.GetLocalSize();
        const double Left=14, Width=FMath::Max(1.0,Size.X-28);
        auto Rect=[&](double X,double Y,double W,double H,FLinearColor Color,int32 Z) {
            FSlateDrawElement::MakeBox(Out,Layer+Z,G.ToPaintGeometry(FVector2D(W,H),FSlateLayoutTransform(FVector2D(X,Y))),
                FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),ESlateDrawEffect::None,Color);
        };
        Rect(Left,24,Width,40,FLinearColor(.045f,.068f,.075f),0);
        Rect(Left+Width*(.5-Good),24,Width*Good*2,40,FLinearColor(.12f,.47f,.45f),1);
        Rect(Left+Width*(.5-Perfect),24,Width*Perfect*2,40,FLinearColor(.94f,.60f,.20f),2);
        for(int32 I=0;I<=10;++I) Rect(Left+Width*I/10,70,1,7,FLinearColor(.50f,.56f,.55f),3);
        Rect(Left+Width*.5-1,15,2,55,FLinearColor(.98f,.86f,.63f),4);
        const double X=Left+Width*Position.Get();
        Rect(X-3,8,6,66,FLinearColor(.99f,.99f,.91f),5);
        Rect(X-8,5,16,7,FLinearColor(.99f,.99f,.91f),5);
        return Layer+6;
    }
private:
    TAttribute<double> Position;
    double Good=.16,Perfect=.045;
};

class SFoundryPrecision : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SFoundryPrecision) {} SLATE_ARGUMENT(FFoundryPrecisionOptions, Options) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        Options=Args._Options;
        Attempt=PrecisionAttempt(Options.WidthPercent);
        auto Label=[](const FString& Text,int32 Size) {
            return SNew(STextBlock).Text(FText::FromString(Text)).Font(FCoreStyle::GetDefaultFontStyle("Regular",Size))
                .ColorAndOpacity(FLinearColor(.93f,.94f,.87f)).AutoWrapText(true);
        };
        ChildSlot[
            SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")))
            .BorderBackgroundColor(FLinearColor(.009f,.017f,.022f,.99f)).Padding(32)[
                SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,15)[Label(Options.Title,28)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)[Label(Options.Detail,18)]
                +SVerticalBox::Slot().AutoHeight()[
                    SNew(SFoundryPrecisionMeter).Position_Lambda([this](){return Attempt.position();})
                        .GoodHalf(Attempt.goodHalfWidth()).PerfectHalf(Attempt.perfectHalfWidth())]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)[Label(TEXT("TEAL: Good     GOLD CENTRE: Perfect     OUTSIDE: Miss"),17)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)[
                    SNew(STextBlock).Text_Lambda([this](){
                        if(Attempt.paused())return FText::FromString(TEXT("Paused while the game is unfocused."));
                        return FText::FromString(Attempt.stage()==PrecisionStage::Ready
                            ?TEXT("Choose Start when ready. Stop the marker once with Space or a click.")
                            :TEXT("Stop in the coloured band. Waiting until the end records a miss."));
                    }).Font(FCoreStyle::GetDefaultFontStyle("Regular",19)).AutoWrapText(true)
                ]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[
                    SNew(SButton).ContentPadding(FMargin(24,14)).ButtonColorAndOpacity(FLinearColor(.38f,.21f,.07f))
                    .OnClicked_Lambda([this](){auto Self=SharedThis(this);Press();return FReply::Handled().SetUserFocus(Self,EFocusCause::SetDirectly);})[
                        SNew(STextBlock).Text_Lambda([this](){return FText::FromString(Attempt.stage()==PrecisionStage::Ready?TEXT("Start Precision"):TEXT("STOP  ·  Space"));})
                        .Font(FCoreStyle::GetDefaultFontStyle("Bold",22))]
                ]
                +SVerticalBox::Slot().AutoHeight()[
                    SNew(SButton).ContentPadding(FMargin(12,9))
                    .Visibility_Lambda([this](){return Attempt.stage()==PrecisionStage::Ready?EVisibility::Visible:EVisibility::Collapsed;})
                    .OnClicked_Lambda([this](){Cancel();return FReply::Handled();})[Label(*Options.BackLabel,18)]
                ]
                +SVerticalBox::Slot().AutoHeight().Padding(0,15,0,0)[Label(TEXT("Starting commits this attempt. Escape cannot cancel it once the marker moves. Your ordinary haul is safe on a miss."),16)]
            ]
        ];
    }
    virtual bool SupportsKeyboardFocus() const override { return true; }
    virtual FReply OnKeyDown(const FGeometry&,const FKeyEvent& E) override
    {
        if(E.IsRepeat())return FReply::Handled();
        if(E.GetKey()==EKeys::SpaceBar||E.GetKey()==EKeys::Enter) {Press();return FReply::Handled();}
        if(E.GetKey()==EKeys::Escape) {if(Attempt.stage()==PrecisionStage::Ready)Cancel();return FReply::Handled();}
        return FReply::Handled(); // A modal never bubbles combat shortcuts.
    }
    virtual void Tick(const FGeometry& G,double Time,float Delta) override
    {
        SCompoundWidget::Tick(G,Time,Delta);
        if(const auto Result=Attempt.tick(Delta,FSlateApplication::Get().IsActive()))Finish(*Result);
    }
private:
    FFoundryPrecisionOptions Options;
    PrecisionAttempt Attempt;
    bool bSubmitted=false;
    void Press()
    {
        if(bSubmitted)return;
        if(Attempt.stage()==PrecisionStage::Ready){Attempt.start();return;}
        if(const auto Result=Attempt.stop())Finish(*Result);
    }
    void Cancel()
    {
        if(bSubmitted||Attempt.stage()!=PrecisionStage::Ready)return;
        bSubmitted=true;
        if(Options.OnCancel)Options.OnCancel();
    }
    void Finish(int32 Result)
    {
        if(bSubmitted)return;
        bSubmitted=true;
        UE_LOG(LogFoundryPrecision,Display,TEXT("FOUNDRY_PRECISION result=%d elapsed=%.6f width=%d version=%s"),
            Result,Attempt.elapsed(),Options.WidthPercent,UTF8_TO_TCHAR(foundry::presentation::PrecisionVersion));
        if(Options.OnResult)Options.OnResult(Result);
    }
};
}

TSharedRef<SWidget> MakeFoundryPrecisionWidget(FFoundryPrecisionOptions Options)
{
    return SNew(SFoundryPrecision).Options(MoveTemp(Options));
}
