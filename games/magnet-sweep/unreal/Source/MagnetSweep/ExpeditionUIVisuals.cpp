#include "ExpeditionUIVisuals.h"
#include "WorkbenchRuntime.h"
#include "ExpeditionRig.h"
#include "CanvasItem.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "Framework/Application/SlateApplication.h"
#include "Fonts/FontMeasure.h"

namespace ExpeditionUI
{
const FLinearColor Ink(.010f,.021f,.032f),Surface(.025f,.046f,.060f,.985f),Edge(.15f,.23f,.26f);
const FLinearColor Ivory(.94f,.91f,.80f),Muted(.60f,.68f,.69f),Teal(.29f,.85f,.76f),Brass(.91f,.65f,.31f),Danger(1.f,.29f,.20f);
namespace
{
void Stroke(FWorkbenchImpl* D,UCanvas* C,float X,float Y,float XX,float YY,FLinearColor Color,float Weight=1)
{D->Line(C,FVector2D(D->UX+X*D->UIScale,D->UY+Y*D->UIScale),FVector2D(D->UX+XX*D->UIScale,D->UY+YY*D->UIScale),Color,Weight);}
FSlateFontInfo Font(FWorkbenchImpl* D,float Scale,bool Display)
{return FSlateFontInfo(Display&&D->DisplayFont?D->DisplayFont:D->RuntimeFont,FMath::Max(9.f,Scale*D->UIScale*(Display?24.f:16.f)),TEXT("Regular"));}
int32 Family(FName Id)
{
    const auto* M=MagnetSweep::FExpeditionRig::FindModule(Id);if(!M)return 0;
    TArray<FName> Tags=M->Provides;Tags.Append(M->RequiresAll);Tags.Append(M->RequiresAny);
    if(Tags.Contains(TEXT("Arc")))return 2;
    if(Tags.Contains(TEXT("Launch")))return 1;
    if(Tags.Contains(TEXT("PhysicalTether")))return 3;
    if(Tags.Contains(TEXT("Vector")))return 4;
    if(Tags.Contains(TEXT("FieldTransfer")))return 5;
    return 0;
}
}
void Rule(FWorkbenchImpl* D,UCanvas* C,float X,float Y,float W,FLinearColor Color)
{D->Rect(C,X,Y,W,1,Color);}
void Panel(FWorkbenchImpl* D,UCanvas* C,float X,float Y,float W,float H,FLinearColor Accent,bool Raised)
{
    D->Rect(C,X+4,Y+5,W,H,FLinearColor(0,0,0,.30f));
    D->Rect(C,X,Y,W,H,Edge);D->Rect(C,X+1,Y+1,W-2,H-2,Surface);
    D->Rect(C,X+2,Y+2,W-4,Raised?35:5,Raised?FLinearColor(.065f,.095f,.11f):FLinearColor(.075f,.11f,.13f));
    Rule(D,C,X+12,Y+H-3,W-24,FLinearColor(.004f,.012f,.017f));
    D->Rect(C,X,Y,FMath::Min(64.f,W*.16f),3,Accent);
    Stroke(D,C,X+W-20,Y,X+W,Y+20,Accent,.85f);
    if(Raised)for(float XX:{X+8,X+W-10})
    {D->Rect(C,XX,Y+H-10,3,3,FLinearColor(.27f,.32f,.33f));D->Rect(C,XX,Y+H-9,3,1,Ink);}
}
void Meter(FWorkbenchImpl* D,UCanvas* C,float X,float Y,float W,float Value,float Maximum,FLinearColor Color,int32 Segments)
{
    const float Fraction=FMath::Clamp(Maximum>0?Value/Maximum:0,0.f,1.f),Gap=3,SW=(W-(Segments-1)*Gap)/Segments;
    for(int32 Index=0;Index<Segments;++Index)
    {const bool Lit=Fraction>(float(Index)/Segments);D->Rect(C,X+Index*(SW+Gap),Y,SW,8,Lit?Color:FLinearColor(.09f,.14f,.16f));}
}
void Socket(FWorkbenchImpl* D,UCanvas* C,float X,float Y,bool Filled,FLinearColor Color)
{
    D->Rect(C,X,Y,12,12,Edge);D->Rect(C,X+2,Y+2,8,8,Filled?Color:Ink);
    if(Filled)D->Rect(C,X+3,Y+3,6,2,Ivory.CopyWithNewOpacity(.35f));
}
FLinearColor ModuleColor(FName Id)
{
    switch(Family(Id)){case 1:return FLinearColor(.94f,.57f,.30f);case 2:return FLinearColor(.48f,.77f,1.f);
    case 3:return Brass;case 4:return FLinearColor(.72f,.62f,.96f);case 5:return FLinearColor(.45f,.87f,.65f);default:return Teal;}
}
void Icon(FWorkbenchImpl* D,UCanvas* C,FName Module,float X,float Y,float Size,FLinearColor Color)
{
    auto L=[&](float AX,float AY,float BX,float BY,float Weight=2.f){Stroke(D,C,X+AX*Size,Y+AY*Size,X+BX*Size,Y+BY*Size,Color,Weight);};
    // A distinct engineered silhouette for each tool family, including supports.
    const int32 Kind=Family(Module);
    if(Kind==0){L(.22f,.18f,.22f,.65f,3);L(.78f,.18f,.78f,.65f,3);L(.22f,.65f,.38f,.82f,3);L(.38f,.82f,.62f,.82f,3);L(.62f,.82f,.78f,.65f,3);L(.15f,.22f,.3f,.22f);L(.7f,.22f,.85f,.22f);L(.38f,.18f,.38f,.42f,1);L(.62f,.18f,.62f,.42f,1);}
    else if(Kind==1){L(.14f,.34f,.75f,.34f,3);L(.14f,.66f,.75f,.66f,3);L(.32f,.5f,.9f,.5f,3);L(.72f,.36f,.9f,.5f);L(.72f,.64f,.9f,.5f);L(.18f,.25f,.18f,.75f);}
    else if(Kind==2){L(.64f,.1f,.29f,.52f,3);L(.29f,.52f,.62f,.52f,3);L(.62f,.52f,.36f,.9f,3);L(.15f,.32f,.08f,.32f,1);L(.85f,.69f,.93f,.69f,1);}
    else if(Kind==3){L(.5f,.14f,.5f,.81f,3);L(.25f,.36f,.75f,.36f,3);L(.5f,.81f,.21f,.60f,3);L(.5f,.81f,.79f,.60f,3);L(.21f,.60f,.21f,.45f);L(.79f,.60f,.79f,.45f);L(.41f,.15f,.59f,.15f);}
    else if(Kind==4){L(.17f,.5f,.82f,.5f,3);L(.64f,.28f,.85f,.5f,3);L(.64f,.72f,.85f,.5f,3);L(.22f,.20f,.42f,.36f);L(.22f,.80f,.42f,.64f);L(.1f,.37f,.1f,.63f);}
    else{L(.18f,.24f,.45f,.24f,3);L(.18f,.24f,.18f,.70f,3);L(.18f,.70f,.45f,.70f,3);L(.55f,.30f,.82f,.30f,3);L(.82f,.30f,.82f,.76f,3);L(.55f,.76f,.82f,.76f,3);L(.34f,.50f,.66f,.50f);L(.54f,.39f,.66f,.5f);L(.54f,.61f,.66f,.5f);}
    if(const auto* M=MagnetSweep::FExpeditionRig::FindModule(Module);M&&M->Kind==MagnetSweep::EExpeditionModuleKind::Passive)
    {D->Rect(C,X+Size*.72f,Y+Size*.73f,Size*.28f,Size*.27f,Surface);L(.79f,.87f,.94f,.87f);L(.865f,.79f,.865f,.95f);}
}
float TextWidth(FWorkbenchImpl* D,const FString& Value,float Scale,bool Display)
{
    if(!D||!FSlateApplication::IsInitialized())return Value.Len()*8.f*Scale;
    return FSlateApplication::Get().GetRenderer()->GetFontMeasureService()->Measure(Value,Font(D,Scale,Display)).X/D->UIScale;
}
void Text(FWorkbenchImpl* D,UCanvas* C,const FString& Value,float X,float Y,float Scale,FLinearColor Color,bool Display)
{
    if(!D||!C)return;
    FCanvasTextItem Item(FVector2D(D->UX+X*D->UIScale,D->UY+Y*D->UIScale),FText::FromString(Value),Font(D,Scale,Display),Color);
    Item.EnableShadow(FLinearColor(0,0,0,.6f),FVector2D(0,1));C->DrawItem(Item);
}
void FitText(FWorkbenchImpl* D,UCanvas* C,const FString& Value,float X,float Y,float Width,float Scale,FLinearColor Color,bool Display)
{const float Measured=TextWidth(D,Value,Scale,Display);Text(D,C,Value,X,Y,Measured>Width?Scale*Width/Measured:Scale,Color,Display);}
float Paragraph(FWorkbenchImpl* D,UCanvas* C,const FString& Value,float X,float Y,float Width,float Scale,FLinearColor Color)
{
    TArray<FString> Words;Value.ParseIntoArrayWS(Words);FString Row;
    for(const auto& Word:Words)
    {
        const FString Next=Row.IsEmpty()?Word:Row+TEXT(" ")+Word;
        if(!Row.IsEmpty()&&TextWidth(D,Next,Scale)>Width){Text(D,C,Row,X,Y,Scale,Color);Y+=22.f*Scale;Row=Word;}
        else Row=Next;
    }
    if(!Row.IsEmpty()){Text(D,C,Row,X,Y,Scale,Color);Y+=22.f*Scale;}
    return Y;
}
}
