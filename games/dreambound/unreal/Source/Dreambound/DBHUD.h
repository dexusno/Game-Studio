#pragma once
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "DBHUD.generated.h"
UCLASS()
class DREAMBOUND_API ADBHUD : public AHUD {
 GENERATED_BODY()
public:
 virtual void DrawHUD() override;
 virtual void NotifyHitBoxClick(FName BoxName) override;
private:
 void Label(const FString& Text,float X,float Y,float Size,FLinearColor Color=FLinearColor::White);
 void Panel(float X,float Y,float W,float H,FLinearColor Color);
 void Button(FName Id,const FString& Text,float X,float Y,float W,float H,bool bAccent=false);
 float UIScale=1;
};
