#pragma once
#include "CoreMinimal.h"
class UCanvas;
struct FWorkbenchImpl;

// Original, resolution-independent console art. Coordinates use the same 1600x900
// canvas as the existing input rectangles; decoration never changes hit testing.
namespace ExpeditionUI
{
extern const FLinearColor Ink, Surface, Edge, Ivory, Muted, Teal, Brass, Danger;
void Rule(FWorkbenchImpl* D,UCanvas* C,float X,float Y,float W,FLinearColor Color);
void Panel(FWorkbenchImpl* D,UCanvas* C,float X,float Y,float W,float H,FLinearColor Accent,bool Raised=false);
void Meter(FWorkbenchImpl* D,UCanvas* C,float X,float Y,float W,float Value,float Maximum,FLinearColor Color,int32 Segments=20);
void Socket(FWorkbenchImpl* D,UCanvas* C,float X,float Y,bool Filled,FLinearColor Color);
void Icon(FWorkbenchImpl* D,UCanvas* C,FName Module,float X,float Y,float Size,FLinearColor Color);
FLinearColor ModuleColor(FName Module);
float TextWidth(FWorkbenchImpl* D,const FString& Text,float Scale,bool Display=false);
void Text(FWorkbenchImpl* D,UCanvas* C,const FString& Value,float X,float Y,float Scale,FLinearColor Color,bool Display=false);
void FitText(FWorkbenchImpl* D,UCanvas* C,const FString& Value,float X,float Y,float Width,float Scale,FLinearColor Color,bool Display=false);
float Paragraph(FWorkbenchImpl* D,UCanvas* C,const FString& Value,float X,float Y,float Width,float Scale,FLinearColor Color);
}
