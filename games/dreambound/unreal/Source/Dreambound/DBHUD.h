#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DBHUD.generated.h"
UCLASS()
class DREAMBOUND_API ADBHUD : public AHUD {
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
 virtual void NotifyHitBoxRelease(FName BoxName) override;
private:
 void Label(const FString& Text,float X,float Y,float Size,FLinearColor Color=FLinearColor::White);
 void WrappedLabel(const FString& Text,float X,float Y,float Width,float Size,FLinearColor Color=FLinearColor::White);
 void Panel(float X,float Y,float W,float H,FLinearColor Color);
 void Frame(float X,float Y,float W,float H,bool bAccent=false);
 void Button(FName Id,const FString& Text,float X,float Y,float W,float H,bool bAccent=false);
 float UIScale=1;
 int32 EquipmentPage=0;
};
