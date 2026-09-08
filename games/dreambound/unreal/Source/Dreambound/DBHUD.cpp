#include "DBHUD.h"
#include "DBGameMode.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "CanvasItem.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

void ADBHUD::Panel(float X,float Y,float W,float H,FLinearColor C){DrawRect(C,X*UIScale,Y*UIScale,W*UIScale,H*UIScale);}
void ADBHUD::Label(const FString& Text,float X,float Y,float Size,FLinearColor Color) {
 static UFont* CrispFont=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/RobotoDistanceField.RobotoDistanceField"));
 UFont* Font=CrispFont?CrispFont:GEngine->GetMediumFont();
 float FontScale=Size/FMath::Max(1.f,float(Font->GetMaxCharHeight()));
 DrawText(Text,Color,X*UIScale,Y*UIScale,Font,FontScale*UIScale,false);
}
void ADBHUD::Button(FName Id,const FString& Text,float X,float Y,float W,float H,bool Accent){
 Panel(X,Y,W,H,Accent?FLinearColor(0.19,0.29,0.29,0.97):FLinearColor(0.06,0.085,0.09,0.97));
 Panel(X,Y,4,H,FLinearColor(0.82,0.64,0.36));
 Label(Text,X+24,Y+H/2-14,27,FLinearColor(0.94,0.9,0.79));
 AddHitBox(FVector2D(X,Y)*UIScale,FVector2D(W,H)*UIScale,Id,true);
}
void ADBHUD::DrawHUD(){
 Super::DrawHUD();if(!Canvas)return;
 auto* G=Cast<ADBGameMode>(UGameplayStatics::GetGameMode(this));if(!G)return;
 auto* P=G->Player;UIScale=Canvas->SizeX/1920.f;float H=Canvas->SizeY/UIScale;
 FLinearColor Ivory(0.94,0.91,0.82),Gold(0.84,0.67,0.39),Mint(0.5,0.9,0.83);
 if(!G->bTitle){
  Panel(40,32,650,112,FLinearColor(0.02,0.035,0.04,0.72));
  Label(G->Rooms.IsValidIndex(G->CurrentRoomId)?G->Rooms[G->CurrentRoomId].Name.ToUpper():TEXT("BETWEEN WORLDS"),62,49,31,Ivory);
  Label(G->ObjectiveText(),62,94,20,Gold);
  Label(FString::Printf(TEXT("EXPEDITION %d  /  SEED %d"),G->Expeditions,G->Seed),1520,44,18,Ivory);
  if(P){
   Panel(42,H-170,370,129,FLinearColor(0.02,0.035,0.04,0.8));
   Label(FString::Printf(TEXT("%d  /  %d"),FMath::CeilToInt(P->Health),FMath::CeilToInt(P->MaxHealth)),62,H-155,27,Ivory);
   Panel(62,H-115,324,7,FLinearColor(0.2,0.13,0.12));Panel(62,H-115,324*FMath::Clamp(P->Health/P->MaxHealth,0.f,1.f),7,FLinearColor(0.77,0.4,0.31));
   Label(TEXT("GUARD"),62,H-94,16,Mint);Panel(137,H-87,249,5,FLinearColor(0.1,0.22,0.23));
   Panel(137,H-87,249*FMath::Clamp(P->GuardEnergy/P->MaxGuardEnergy,0.f,1.f),5,Mint);
   FString E=StaticEnum<EDBElement>()->GetNameStringByValue(int64(P->CurrentElement)).ToUpper();
   Panel(1400,H-163,468,123,FLinearColor(0.02f,0.035f,0.04f,0.8f));
   Label(P->GetShieldStateLabel(),1430,H-140,25,P->IsShieldAway()?Gold:Mint);
   Label(P->GetSpecialLabel(),1430,H-103,20,Ivory);
   Panel(1430,H-61,300,5,FLinearColor(0.12f,0.16f,0.17f));
   Panel(1430,H-61,300*FMath::Clamp(P->ThrowCharge>0?P->ThrowCharge:P->bShieldReady?1.f:0.f,0.f,1.f),5,P->IsShieldAway()?FLinearColor(.9f,.4f,.2f):Mint);
   if(P->CurrentElement!=EDBElement::Neutral)Label(E,1750,H-76,17,Mint);
   if(P->HitMarkerTime>0){float X=Canvas->SizeX/2,Y=Canvas->SizeY/2;DrawLine(X-6,Y-6,X+6,Y+6,Gold,2);DrawLine(X-6,Y+6,X+6,Y-6,Gold,2);}
   if(P->HurtFlashTime>0)Panel(0,0,1920,H,FLinearColor(0.8f,0.08f,0.035f,FMath::Min(0.16f,P->HurtFlashTime*.45f)));
   Label(P->IsShieldAway()?TEXT("Q / recall    Move to shape the return path"):TEXT("Tap LMB / strike    Hold + release / throw"),670,H-61,19,Ivory);
   Label(TEXT("RMB / guard    Shift / dash    Tab / build    Esc / pause"),650,H-32,17,Gold);
   if(!G->bPaused&&!G->bChoosingReward&&!G->bShowingBuild&&!G->bWon&&!G->bDefeated) {
    float X=Canvas->SizeX/2,Y=Canvas->SizeY/2;
    FLinearColor C=P->IsShieldAway()?Gold:P->bGuarding?Mint:Ivory;
    DrawLine(X-11*UIScale,Y,X-5*UIScale,Y,C,1.6*UIScale);DrawLine(X+5*UIScale,Y,X+11*UIScale,Y,C,1.6*UIScale);
    DrawLine(X,Y-11*UIScale,X,Y-5*UIScale,C,1.6*UIScale);DrawLine(X,Y+5*UIScale,X,Y+11*UIScale,C,1.6*UIScale);
    FString Interact=G->InteractText();
    if(!Interact.IsEmpty()){Panel(630,H*0.67f,660,60,FLinearColor(0.02,0.05,0.05,0.92));Label(Interact,660,H*0.67f+16,25,Ivory);}
   }
  }
  for(TActorIterator<ADBEnemy> It(GetWorld());It;++It)if(!It->bDead&&It->RoomId==G->CurrentRoomId){
   if(P&&FVector::Dist(P->GetActorLocation(),It->GetActorLocation())<2200){
    FVector Mark=Project(It->GetActorLocation()+FVector(0,0,150));float MX=Mark.X/UIScale,MY=Mark.Y/UIScale;
    if(Mark.Z>0&&MX>70&&MX<1850&&MY>155&&MY<H-200){
     if(It->Kind!=EDBEnemyKind::Boss){Panel(MX-46,MY,92,4,FLinearColor(0.14f,0.08f,0.05f));Panel(MX-46,MY,92*FMath::Clamp(It->Health/It->MaxHealth,0.f,1.f),4,Gold);}
     if(!It->Telegraph.IsEmpty())Label(It->Telegraph,MX-75,MY-30,18,Gold);
    }
   }
   if(It->Kind==EDBEnemyKind::Boss){
    Panel(620,44,660,72,FLinearColor(0.025,0.035,0.04,0.85));
    Label(G->bRecoverySlice?TEXT("HEAVY SENTINEL"):TEXT("THE BELL GUARDIAN"),640,53,22,Gold);
    Panel(640,88,620,7,FLinearColor(0.22,0.11,0.1));
    Panel(640,88,620*FMath::Clamp(It->Health/It->MaxHealth,0.f,1.f),7,FLinearColor(0.95,0.54,0.26));
   }
  }
  if(G->EventRemaining>0&&!G->bChoosingReward){Panel(570,165,780,56,FLinearColor(0.025,0.045,0.045,0.82));Label(G->EventText,594,184,21,G->EventColor);}
  if(G->bRecoverySlice&&!G->bPaused&&!G->bChoosingReward&&!G->bShowingBuild&&(G->bSliceAwaitingStart||G->ClearedRooms.Contains(G->CurrentRoomId))){
   if(auto* A=G->Rooms[G->CurrentRoomId].Altar){FVector Mark=Project(A->GetActorLocation()+FVector(0,0,140));float MX=Mark.X/UIScale,MY=Mark.Y/UIScale;
    if(Mark.Z>0&&MX>120&&MX<1730&&MY>160&&MY<H-190)Label(G->bSliceAwaitingStart?TEXT("WARD STONE"):TEXT("EARNED ATTACHMENT"),MX-70,MY,18,Mint);
   }
  }
  if(!G->bRecoverySlice&&!G->bPaused&&!G->bChoosingReward&&!G->bShowingBuild&&G->ClearedRooms.Contains(G->CurrentRoomId)){
   for(int32 I=0;I<G->Rooms.Num();++I){
    auto& Room=G->Rooms[I];bool Reward=I==G->CurrentRoomId&&!G->ClaimedRooms.Contains(I);
    bool Next=Room.Parent==G->CurrentRoomId&&G->ClaimedRooms.Contains(G->CurrentRoomId)&&!G->ClaimedRooms.Contains(I);
    if(!Reward&&!Next)continue;
    FVector Target=Reward&&Room.Altar?Room.Altar->GetActorLocation():Room.Center+(G->Rooms[G->CurrentRoomId].Center-Room.Center).GetSafeNormal()*1250;
    FVector Mark=Project(Target+FVector(0,0,190));float MX=Mark.X/UIScale,MY=Mark.Y/UIScale;
    if(Mark.Z>0&&MX>120&&MX<1730&&MY>225&&MY<H-190){Label(Reward?TEXT("RECOVER"):Room.bOptional?TEXT("OPTIONAL / MIRROR TRIAL"):TEXT("PASSAGE"),MX-50,MY,18,Mint);}
   }
  }
  if(P&&P->MessageTime>0&&!G->bChoosingReward)Label(P->LastCombatMessage,770,H*0.58f,26,Gold);
 }
 if(G->bTitle){
  Panel(0,0,920,H,FLinearColor(0.012,0.024,0.028,0.92));
  Label(G->bRecoverySlice?TEXT("THE SHIELD / COURTYARD STUDY"):TEXT("CYBORG / PLAYABLE BETA"),115,100,20,Gold);
  Label(TEXT("BETWEEN"),108,153,79,Ivory);Label(TEXT("WORLDS"),108,230,79,Ivory);
  Label(G->bRecoverySlice?TEXT("Your defense becomes your attack."):TEXT("The places you dream about are real."),115,343,29,Ivory);
  Label(G->bRecoverySlice?TEXT("Strike. Commit the shield. Shape its return."):TEXT("Build a weapon worth carrying between them."),115,392,23,Mint);
  Button("Resume",G->bCanResume?TEXT("Resume expedition"):G->bRecoverySlice?TEXT("Enter the courtyard"):TEXT("Begin expedition"),115,475,625,70,true);
  if(G->bCanResume)Button("New",TEXT("New expedition"),115,560,625,65);
  float PatternY=G->bCanResume?645:560;
  if(!G->LearnedPatterns.IsEmpty())Button("Pattern",TEXT("Starting pattern: ")+G->DescribeUpgrade(G->StartingPattern).Name,115,PatternY,625,60);
  Button("Quit",TEXT("Exit"),115,H-135,200,52);
  Label(TEXT("Mouse + keyboard  /  Headphones recommended"),115,H-190,19,Gold);
  Label(G->bRecoverySlice?TEXT("Three encounters. Earn two attachments. Keep your learned patterns."):TEXT("First beta: combat, rewards and a crossing."),115,H-60,18,Ivory);
 }
 else if(G->bChoosingReward){
  Panel(0,0,1920,H,FLinearColor(0.01,0.02,0.025,0.88));
  Label(TEXT("RECOVERED / CHOOSE AN ATTACHMENT"),260,H*0.18f,39,Ivory);
  Label(TEXT("Install now. Learn its pattern for future expeditions."),260,H*0.18f+59,23,Mint);
  for(int32 I=0;I<G->Offers.Num();++I){
   const auto& O=G->Offers[I];float X=260+I*477,Y=H*0.34f;
   Panel(X,Y,440,370,FLinearColor(0.06,0.09,0.095,0.98));Panel(X,Y,440,5,O.Color);
   Label(FString::Printf(TEXT("0%d"),I+1),X+28,Y+30,25,O.Color);Label(O.Name,X+28,Y+84,32,Ivory);
   TArray<FString> Words;O.Description.ParseIntoArrayWS(Words);FString Line;float LY=Y+146;
   for(const auto& Word:Words){if(Line.Len()+Word.Len()>38){Label(Line,X+28,LY,21,Ivory);LY+=29;Line.Empty();}if(!Line.IsEmpty())Line+=" ";Line+=Word;}
   if(!Line.IsEmpty())Label(Line,X+28,LY,21,Ivory);
   int32 Rank=P?P->GetUpgradeRank(O.Id):0;
   Label(O.Id=="Restore"?TEXT("RESTORATION"):Rank>0?FString::Printf(TEXT("EVOLVE / RANK %d"),Rank+1):TEXT("NEW BEHAVIOR"),X+28,Y+296,18,O.Color);
   AddHitBox(FVector2D(X,Y)*UIScale,FVector2D(440,370)*UIScale,FName(*FString::Printf(TEXT("Offer%d"),I)),true);
  }
 }
 else if(G->bWon||G->bDefeated){
  Panel(0,0,1920,H,FLinearColor(0.012,0.027,0.032,0.92));
  Label(G->bWon?(G->bRecoverySlice?TEXT("COURTYARD CLEARED."):TEXT("YOU CARRIED IT THROUGH.")):TEXT("CONNECTION LOST."),360,170,60,Ivory);
  Label(G->bWon?(G->bRecoverySlice?TEXT("The heavy sentinel falls. A new starting pattern is yours."):TEXT("A guardian defeated. A new world reached. Your discoveries endured.")):TEXT("The expedition ended. Your learned patterns and milestones remain."),360,270,27,Mint);
  Label(FString::Printf(TEXT("%d patterns learned  /  %d enemies defeated  /  %.0f minutes"),G->LearnedPatterns.Num(),G->Kills,G->RunSeconds/60),360,330,25,Gold);
  Button("New",TEXT("New expedition / different seed"),360,430,670,75,true);
  Button("Same",TEXT("Practice this seed again"),360,525,670,65);
  if(!G->LearnedPatterns.IsEmpty())Button("Pattern",TEXT("Starting pattern: ")+G->DescribeUpgrade(G->StartingPattern).Name,360,610,670,60);
  Button("Quit",TEXT("Exit"),360,G->LearnedPatterns.IsEmpty()?610:690,300,60);
  Label(TEXT("For your playtest: did the reward change your tactics? Did you want another build?"),360,H-95,22,Ivory);
 }
 else if(G->bPaused||G->bShowingBuild){
  Panel(0,0,1920,H,FLinearColor(0.012,0.025,0.03,0.93));
  Label(G->bShowingBuild?TEXT("YOUR LIVING INSTRUMENT"):TEXT("PAUSED"),190,100,52,Ivory);
  Label(TEXT("Attack and defense remain yours to choose."),190,177,25,Mint);
  int32 I=0;if(P)for(const auto& Pair:P->Upgrades){
   auto O=G->DescribeUpgrade(Pair.Key);
   Label(FString::Printf(TEXT("%s  /  %d"),*O.Name,Pair.Value),190,250+I*49,27,O.Color);++I;
  }
  if(I==0)Label(TEXT("Recover an attachment to begin shaping your build."),190,250,24,Ivory);
  Label(TEXT("LMB / tap to strike; hold and release to throw"),1030,255,22,Ivory);
  Label(TEXT("RMB / guard while the shield is held"),1030,304,22,Ivory);
  Label(TEXT("Q / recall when away; heavy bash while held"),1030,353,22,Ivory);
  Label(TEXT("Shift / dash    Space / jump    R / change core"),1030,402,22,Ivory);
  Label(TEXT("E / interact    1 / 2 / 3 reward    Tab / build"),1030,451,22,Ivory);
  Label(FString::Printf(TEXT("Mouse sensitivity: %.2f"),G->Sensitivity),1030,536,25,Gold);
  Button("SensDown",TEXT("-"),1030,590,100,50);Button("SensUp",TEXT("+"),1150,590,100,50);
  Label(TEXT("After exiting, the current encounter resumes from its entry checkpoint."),190,H-220,20,Gold);
  Button("Back",TEXT("Return to the world"),190,H-160,650,65,true);
  Button("Quit",TEXT("Save checkpoint / exit"),1030,H-160,550,65);
 }
 if(G->bSaveFailed){
  Panel(0,H-125,1920,125,FLinearColor(0.16f,0.045f,0.03f,0.99f));
  Label(G->SaveNotice,55,H-100,24,Ivory);
  Label(TEXT("New progress is still in memory. Retry before leaving."),55,H-58,19,Gold);
  Button("RetrySave",TEXT("Retry save"),1110,H-104,300,64,true);
  Button("ForceQuit",TEXT("Exit without saving"),1450,H-104,405,64);
 }
}
void ADBHUD::NotifyHitBoxRelease(FName Box){
 Super::NotifyHitBoxRelease(Box);auto* G=Cast<ADBGameMode>(UGameplayStatics::GetGameMode(this));if(!G)return;
 if(Box=="Resume")G->ResumeRun();
 else if(Box=="New")G->StartNewRun();
 else if(Box=="Same")G->StartNewRun(true);
 else if(Box=="Quit")G->QuitGame();
 else if(Box=="ForceQuit")G->QuitGame(true);
 else if(Box=="RetrySave")G->SaveProgress(!G->bDefeated&&!G->bWon);
 else if(Box=="Back"){if(G->bShowingBuild)G->ToggleBuild();else G->TogglePause();}
 else if(Box=="SensDown")G->AdjustSensitivity(-0.1f);
 else if(Box=="SensUp")G->AdjustSensitivity(0.1f);
 else if(Box=="Pattern"&&!G->LearnedPatterns.IsEmpty()){
  int32 I=G->LearnedPatterns.IndexOfByKey(G->StartingPattern);G->StartingPattern=G->LearnedPatterns[(I+1)%G->LearnedPatterns.Num()];
 }
 else if(Box.ToString().StartsWith("Offer"))G->ChooseReward(FCString::Atoi(*Box.ToString().Mid(5)));
}
