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
void ADBHUD::Frame(float X,float Y,float W,float H,bool Accent){
 const FLinearColor Edge=Accent?FLinearColor(.79f,.63f,.36f,.88f):FLinearColor(.53f,.61f,.48f,.62f);
 Panel(X,Y,W,H,Accent?FLinearColor(.047f,.084f,.068f,.96f):FLinearColor(.018f,.035f,.029f,.83f));
 Panel(X+1,Y+1,W-2,1,FLinearColor(.79f,.73f,.55f,.19f));
 const float K=16.f;
 auto Line=[&](float X1,float Y1,float X2,float Y2){DrawLine(X1*UIScale,Y1*UIScale,X2*UIScale,Y2*UIScale,Edge,1.25f*UIScale);};
 Line(X,Y+K,X,Y);Line(X,Y,X+K,Y);
 Line(X+W-K,Y,X+W,Y);Line(X+W,Y,X+W,Y+K);
 Line(X,Y+H-K,X,Y+H);Line(X,Y+H,X+K,Y+H);
 Line(X+W-K,Y+H,X+W,Y+H);Line(X+W,Y+H,X+W,Y+H-K);
}
void ADBHUD::Label(const FString& Text,float X,float Y,float Size,FLinearColor Color) {
 static UFont* CrispFont=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/RobotoDistanceField.RobotoDistanceField"));
 UFont* Font=CrispFont?CrispFont:GEngine->GetMediumFont();
 float FontScale=Size/FMath::Max(1.f,float(Font->GetMaxCharHeight()));
 DrawText(Text,Color,X*UIScale,Y*UIScale,Font,FontScale*UIScale,false);
}
void ADBHUD::Button(FName Id,const FString& Text,float X,float Y,float W,float H,bool Accent){
 Frame(X,Y,W,H,Accent);
 Panel(X+10,Y+H/2-3,5,6,FLinearColor(.74f,.64f,.40f));
 Label(Text,X+24,Y+H/2-14,27,FLinearColor(0.94,0.9,0.79));
 AddHitBox(FVector2D(X,Y)*UIScale,FVector2D(W,H)*UIScale,Id,true);
}
void ADBHUD::WrappedLabel(const FString& Text,float X,float Y,float Width,float Size,FLinearColor Color){
 UFont* Font=LoadObject<UFont>(nullptr,TEXT("/Engine/EngineFonts/RobotoDistanceField.RobotoDistanceField"));
 if(!Font)Font=GEngine->GetMediumFont();
 const float Scale=Size/FMath::Max(1.f,float(Font->GetMaxCharHeight()));
 TArray<FString> Words;Text.ParseIntoArrayWS(Words);FString Line;
 for(const auto& Word:Words){FString Candidate=Line.IsEmpty()?Word:Line+TEXT(" ")+Word;float W=0,H=0;GetTextSize(Candidate,W,H,Font,Scale);
  if(!Line.IsEmpty()&&W>Width){Label(Line,X,Y,Size,Color);Y+=Size+7;Line=Word;}else Line=Candidate;}
 if(!Line.IsEmpty())Label(Line,X,Y,Size,Color);
}
void ADBHUD::DrawHUD(){
 Super::DrawHUD();if(!Canvas)return;
 auto* G=Cast<ADBGameMode>(UGameplayStatics::GetGameMode(this));if(!G)return;
 auto* P=G->Player;UIScale=FMath::Min(Canvas->SizeX/1920.f,Canvas->SizeY/1080.f);float H=Canvas->SizeY/UIScale;
 FLinearColor Ivory(.93f,.91f,.80f),Gold(.79f,.67f,.43f),Mint(.56f,.77f,.62f);
 if(!G->bTitle){
  Frame(40,32,650,112);
  Label(G->Rooms.IsValidIndex(G->CurrentRoomId)?G->Rooms[G->CurrentRoomId].Name.ToUpper():TEXT("BETWEEN WORLDS"),62,49,31,Ivory);
  Label(G->ObjectiveText(),62,94,20,Gold);
  Label(FString::Printf(TEXT("EXPEDITION %d  /  SEED %d"),G->Expeditions,G->Seed),1520,44,18,Ivory);
  if(P){
   Frame(42,H-258,370,80);
   const FLinearColor StaminaColor=P->Stamina<20.f?Gold:Mint;
   Label(FString::Printf(TEXT("%s / %d%%"),P->IsSprinting()?TEXT("SPRINTING"):TEXT("STAMINA"),
    FMath::RoundToInt(100.f*P->Stamina/FMath::Max(1.f,P->MaxStamina))),62,H-246,17,StaminaColor);
   Panel(62,H-217,324,6,FLinearColor(.09f,.14f,.11f));
   Panel(62,H-217,324*FMath::Clamp(P->Stamina/FMath::Max(1.f,P->MaxStamina),0.f,1.f),6,StaminaColor);
   Label(P->IsDashing()?TEXT("ALT / EVADING"):P->DashCooldown>0.f
    ?FString::Printf(TEXT("ALT / DASH  %.1fs"),P->DashCooldown):TEXT("ALT / DASH  READY"),62,H-204,16,P->DashCooldown>0.f?Gold:Mint);
   Frame(42,H-170,370,129);
   Label(FString::Printf(TEXT("%d  /  %d"),FMath::CeilToInt(P->Health),FMath::CeilToInt(P->MaxHealth)),62,H-155,27,Ivory);
   Panel(62,H-115,324,7,FLinearColor(0.2,0.13,0.12));Panel(62,H-115,324*FMath::Clamp(P->Health/P->MaxHealth,0.f,1.f),7,FLinearColor(0.77,0.4,0.31));
   Label(FString::Printf(TEXT("%d PIECES AVAILABLE TO BLOCK"),P->GetAttachedPieceCount()),62,H-89,18,Mint);
   FString E=StaticEnum<EDBElement>()->GetNameStringByValue(int64(P->CurrentElement)).ToUpper();
   Frame(1400,H-201,465,165);
   Label(P->IsFullChargeReady()?TEXT("FULL CHARGE / RELEASE!"):P->GetSelectedPieceCount()>0?FString::Printf(TEXT("RELEASE / %d PIECES"),P->GetSelectedPieceCount()):P->bGuarding?TEXT("SHIELD / EXPANDED"):TEXT("WEAPON / FOLDED"),1420,H-185,20,P->GetSelectedPieceCount()>0?Gold:Mint);
   if(P->GetChargeHoldTime()>0){Panel(1420,H-155,410,5,FLinearColor(.1,.15,.17));Panel(1420,H-155,410*P->GetChargeProgress(),5,Gold);}
   for(int32 I=0;I<P->GetShieldPieceCount();++I){
    const auto State=P->GetPieceState(I);const float X=1420+I*62;
    FLinearColor C=State==EDBShieldPieceState::Selected?Gold:State==EDBShieldPieceState::Attached?Mint:State==EDBShieldPieceState::Regenerating?FLinearColor(.22,.3,.36):FLinearColor(.54,.63,.83);
    Panel(X,H-140,48,28,FLinearColor(.04f,.065f,.05f));
    const float Fill=State==EDBShieldPieceState::Regenerating?P->GetPieceRegenerationProgress(I):1.f;
    Panel(X,H-114,48*Fill,2,C);
    DrawLine((X+7)*UIScale,(H-135)*UIScale,(X+1)*UIScale,(H-122)*UIScale,C,1.4f*UIScale);
    DrawLine((X+41)*UIScale,(H-135)*UIScale,(X+47)*UIScale,(H-122)*UIScale,C,1.4f*UIScale);
    Label(FString::FromInt(I+1),X+17,H-139,20,C);
    Label(State==EDBShieldPieceState::Regenerating?TEXT("REGEN"):State==EDBShieldPieceState::Returning?TEXT("BACK"):State==EDBShieldPieceState::Attached?TEXT("HELD"):State==EDBShieldPieceState::Selected?TEXT("LIT"):TEXT("AWAY"),X,H-106,12,C);
   }
   Label(FString::Printf(TEXT("%d held / %d away / %d rebuilding"),P->GetAttachedPieceCount(),P->GetDeployedPieceCount(),P->GetRegeneratingPieceCount()),1420,H-84,16,Ivory);
   const float Wait=FMath::Max(P->SpecialCooldown,P->AttackRecovery);
   Label((P->HasUpgrade("Ram")?FString(TEXT("F / RAM")):FString(TEXT("F / HEAVY BASH")))+(Wait>0?FString::Printf(TEXT(" / %.1fs"),Wait):TEXT(" / READY")),1420,H-61,17,Wait>0?Gold:Mint);
   if(P->StoredShots>0||P->HasUpgrade("Mirror"))Label(FString::Printf(TEXT("MIRROR / %d charges"),P->StoredShots),1420,H-228,20,Gold);
   if(P->GetTotalAnchorIntegrity()>0)Label(FString::Printf(TEXT("ANCHOR COVER / %.0f integrity"),P->GetTotalAnchorIntegrity()),1420,H-257,19,Mint);
   Label(P->GetElementLabel()+(P->CurrentElement==EDBElement::Neutral?TEXT(" / R CYCLES CORES"):TEXT(" / ON HIT")),62,H-58,17,P->CurrentElement==EDBElement::Frost?FLinearColor(.4,.85,1):P->CurrentElement==EDBElement::Ember?FLinearColor(1,.5,.2):P->CurrentElement==EDBElement::Storm?FLinearColor(.8,.5,1):Mint);
   if(P->HitMarkerTime>0){float X=Canvas->SizeX/2,Y=Canvas->SizeY/2;DrawLine(X-6,Y-6,X+6,Y+6,Gold,2);DrawLine(X-6,Y+6,X+6,Y-6,Gold,2);}
   if(P->HurtFlashTime>0)Panel(0,0,1920,H,FLinearColor(0.8f,0.08f,0.035f,FMath::Min(0.16f,P->HurtFlashTime*.45f)));
   Frame(530,H-72,820,68);
   Label(TEXT("LMB tap / strike   Hold 1.8s / full charge   RMB / unfold & block"),550,H-62,18,Ivory);
   Label(TEXT("Q recall   F heavy   Shift sprint   Alt dash   Space jump   Tab abilities   Esc pause"),550,H-34,16,Gold);
   if(!G->bPaused&&!G->bChoosingReward&&!G->bShowingBuild&&!G->bWon&&!G->bDefeated) {
    float X=Canvas->SizeX/2,Y=Canvas->SizeY/2;
    FLinearColor C=P->IsShieldAway()?Gold:P->bGuarding?Mint:Ivory;
    DrawLine(X-11*UIScale,Y,X-5*UIScale,Y,C,1.6*UIScale);DrawLine(X+5*UIScale,Y,X+11*UIScale,Y,C,1.6*UIScale);
    DrawLine(X,Y-11*UIScale,X,Y-5*UIScale,C,1.6*UIScale);DrawLine(X,Y+5*UIScale,X,Y+11*UIScale,C,1.6*UIScale);
    FString Interact=G->InteractText();
    if(!Interact.IsEmpty()){Frame(630,H*0.67f,660,60,true);Label(Interact,660,H*0.67f+16,25,Ivory);}
   }
  }
  for(TActorIterator<ADBEnemy> It(GetWorld());It;++It)if(!It->bDead&&(It->RoomId==G->CurrentRoomId||It->RoomId==INDEX_NONE)){
   if(P&&FVector::Dist(P->GetActorLocation(),It->GetActorLocation())<2200){
    FVector Mark=Project(It->GetActorLocation()+FVector(0,0,150));float MX=Mark.X/UIScale,MY=Mark.Y/UIScale;
    if(Mark.Z>0&&MX>70&&MX<1850&&MY>155&&MY<H-200){
     if(It->Kind!=EDBEnemyKind::Boss){Panel(MX-46,MY,92,4,FLinearColor(0.14f,0.08f,0.05f));Panel(MX-46,MY,92*FMath::Clamp(It->Health/It->MaxHealth,0.f,1.f),4,Gold);}
     if(!It->Telegraph.IsEmpty())Label(It->Telegraph,MX-75,MY-30,18,Gold);
     FString Status;if(It->ChillStacks>0)Status+=FString::Printf(TEXT("CHILLED x%d  "),It->ChillStacks);if(It->BurnRemaining>0)Status+=TEXT("BURNING  ");if(It->StormMarks>0)Status+=TEXT("STORM MARKED");
     if(!Status.IsEmpty())Label(Status,MX-85,MY+13,16,It->ChillStacks>0?FLinearColor(.4,.85,1):It->BurnRemaining>0?FLinearColor(1,.5,.2):FLinearColor(.8,.5,1));
     if(It->bPracticeTarget)Label(TEXT("PRACTICE TARGET"),MX-85,MY-54,16,Mint);
    }
   }
   if(It->Kind==EDBEnemyKind::Boss){
    Frame(620,44,660,72,true);
    Label(G->bRecoverySlice?TEXT("HEAVY SENTINEL"):TEXT("THE BELL GUARDIAN"),640,53,22,Gold);
    Panel(640,88,620,7,FLinearColor(0.22,0.11,0.1));
    Panel(640,88,620*FMath::Clamp(It->Health/It->MaxHealth,0.f,1.f),7,FLinearColor(0.95,0.54,0.26));
   }
  }
  if(G->EventRemaining>0&&!G->bChoosingReward){Frame(570,165,870,86,true);WrappedLabel(G->EventText,594,181,822,21,G->EventColor);}
  if(!G->PracticeInstruction.IsEmpty()&&!G->bPaused&&!G->bChoosingReward&&!G->bShowingBuild){
   Frame(42,170,480,192);Label(TEXT("TRY YOUR REWARD / SAFE PRACTICE"),62,188,19,Mint);
   WrappedLabel(G->PracticeInstruction,62,223,436,20,Ivory);Label(TEXT("Leave through the passage when ready."),62,325,17,Gold);
  }
  if(G->bRecoverySlice&&G->ClaimedRooms.Contains(G->CurrentRoomId)&&G->Rooms.IsValidIndex(G->CurrentRoomId+1)){
   const FVector D=(G->Rooms[G->CurrentRoomId+1].Center-G->Rooms[G->CurrentRoomId].Center).GetSafeNormal();
   FVector Mark=Project(G->Rooms[G->CurrentRoomId].Center+D*1600+FVector(0,0,230));
   if(Mark.Z>0)Label(TEXT("NEXT WARD / PASSAGE OPEN"),Mark.X/UIScale-130,Mark.Y/UIScale,19,Mint);
  }
  if(G->bRecoverySlice&&!G->bPaused&&!G->bChoosingReward&&!G->bShowingBuild&&!G->ClaimedRooms.Contains(G->CurrentRoomId)&&(G->bSliceAwaitingStart||G->ClearedRooms.Contains(G->CurrentRoomId))){
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
  if(P&&P->MessageTime>0&&!G->bChoosingReward){Frame(590,H*.60f-12,752,P->LastCombatMessage.Len()>90?90:54);WrappedLabel(P->LastCombatMessage,610,H*0.60f,710,21,Gold);}
 }
 if(G->bTitle){
  Panel(0,0,920,H,FLinearColor(.011f,.024f,.018f,.94f));
  Panel(875,50,1,H-100,FLinearColor(.56f,.50f,.32f,.40f));
  // Original six-petal inlay echoes the new folded weapon and sanctuary carving.
  for(int32 I=0;I<6;++I){
   const float A=I*PI/3.f;
   const FVector2D Center(726.f,223.f),D(FMath::Cos(A),FMath::Sin(A)),Side(-D.Y,D.X);
   const FVector2D V[4]={Center+D*23.f,Center+D*47.f+Side*12.f,Center+D*74.f,Center+D*47.f-Side*12.f};
   for(int32 J=0;J<4;++J)DrawLine(V[J].X*UIScale,V[J].Y*UIScale,V[(J+1)%4].X*UIScale,V[(J+1)%4].Y*UIScale,Gold,1.6f*UIScale);
  }
  Label(G->bRecoverySlice?TEXT("SANCTUARY / REVERIE 0.4.0"):TEXT("CYBORG / PLAYABLE BETA"),115,100,20,Gold);
  Label(TEXT("BETWEEN"),108,153,79,Ivory);Label(TEXT("WORLDS"),108,230,79,Ivory);
  Label(G->bRecoverySlice?TEXT("Your defense becomes your attack."):TEXT("The places you dream about are real."),115,343,29,Ivory);
  Label(G->bRecoverySlice?TEXT("Fold to strike. Unfold to defend. Commit to power."):TEXT("Build a weapon worth carrying between them."),115,392,23,Mint);
  Button("Resume",G->bCanResume?TEXT("Resume expedition"):G->bRecoverySlice?TEXT("Enter the courtyard"):TEXT("Begin expedition"),115,475,625,70,true);
  if(G->bCanResume)Button("New",TEXT("New expedition"),115,560,625,65);
  float PatternY=G->bCanResume?645:560;
  if(!G->LearnedPatterns.IsEmpty())Button("Pattern",TEXT("Starting pattern: ")+G->DescribeUpgrade(G->StartingPattern).Name,115,PatternY,625,60);
  Button("Quit",TEXT("Exit"),115,H-135,200,52);
  Label(TEXT("Mouse + keyboard  /  Headphones recommended"),115,H-190,19,Gold);
  Label(G->bRecoverySlice?TEXT("Three connected wards. Seeded routes. Two earned abilities."):TEXT("First beta: combat, rewards and a crossing."),115,H-60,18,Ivory);
 }
 else if(G->bChoosingReward){
  Panel(0,0,1920,H,FLinearColor(0.01,0.02,0.025,0.88));
  Label(TEXT("RECOVERED / CHOOSE AN ATTACHMENT"),260,H*0.18f,39,Ivory);
  Label(TEXT("Install now. Learn its pattern for future expeditions."),260,H*0.18f+59,23,Mint);
  for(int32 I=0;I<G->Offers.Num();++I){
   const auto& O=G->Offers[I];float X=260+I*477,Y=H*0.34f;
   Frame(X,Y,440,410,true);Panel(X+25,Y+15,390,2,O.Color);
   Label(FString::Printf(TEXT("0%d"),I+1),X+28,Y+30,25,O.Color);Label(O.Name,X+28,Y+84,32,Ivory);
   WrappedLabel(O.Description,X+28,Y+146,384,21,Ivory);
   int32 Rank=P?P->GetUpgradeRank(O.Id):0;
   Label(O.Id=="Restore"?TEXT("RESTORATION"):Rank>0?FString::Printf(TEXT("EVOLVE / RANK %d"),Rank+1):TEXT("NEW BEHAVIOR"),X+28,Y+354,18,O.Color);
   AddHitBox(FVector2D(X,Y)*UIScale,FVector2D(440,410)*UIScale,FName(*FString::Printf(TEXT("Offer%d"),I)),true);
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
  Label(G->bShowingBuild?TEXT("YOUR ABILITIES"):TEXT("PAUSED"),190,90,52,Ivory);
  Label(TEXT("LMB taps chain strikes. Hold 1.8s for full power. RMB unfolds to block."),190,163,24,Mint);
  if(G->bShowingBuild){
   TArray<FName> Keys;if(P)P->Upgrades.GetKeys(Keys);Keys.Sort(FNameLexicalLess());
   const int32 Pages=FMath::Max(1,FMath::DivideAndRoundUp(Keys.Num(),4));EquipmentPage=FMath::Clamp(EquipmentPage,0,Pages-1);
   for(int32 I=0;I<4&&Keys.IsValidIndex(EquipmentPage*4+I);++I){FName Id=Keys[EquipmentPage*4+I];const int32 Rank=P->GetUpgradeRank(Id);auto O=G->DescribeUpgrade(Id);float X=190+(I%2)*805,Y=225+(I/2)*249;
    Frame(X,Y,765,223);Label(O.Name+FString::Printf(TEXT(" / RANK %d"),Rank),X+22,Y+20,27,O.Color);
    WrappedLabel(ADBCharacter::GetUpgradeDescription(Id,Rank),X+22,Y+69,716,22,Ivory);
   }
   if(Keys.IsEmpty())Label(TEXT("Earn an attachment at a cleared ward to begin your build."),190,260,25,Ivory);
   Label(FString::Printf(TEXT("PAGE %d / %d"),EquipmentPage+1,Pages),190,750,20,Gold);
   if(Pages>1){Button("BuildPrev",TEXT("Previous"),430,737,230,51);Button("BuildNext",TEXT("Next"),680,737,230,51);}
  }else{
   Label(TEXT("LMB / tap to strike; hold to light pieces; release to throw"),190,257,23,Ivory);
   Label(TEXT("RMB / guard with attached pieces; a block spends one"),190,305,23,Ivory);
   Label(TEXT("Q / recall surviving pieces     F / heavy bash or Ram"),190,353,23,Ivory);
   Label(TEXT("Destroyed pieces regenerate independently."),190,401,23,Mint);
   Label(TEXT("Shift / sprint    Left Alt / dash    Space / jump    R / cycle cores"),190,449,23,Ivory);
   Label(TEXT("Sprint drains stamina. After exhaustion, recover and press Shift again."),190,489,20,Mint);
   Button("Equipment",TEXT("Read my ability instructions"),190,530,650,62,true);
   Label(FString::Printf(TEXT("Mouse sensitivity: %.2f"),G->Sensitivity),1130,270,25,Gold);
   Button("SensDown",TEXT("-"),1130,326,100,50);Button("SensUp",TEXT("+"),1250,326,100,50);
   Label(FString::Printf(TEXT("Sound: %d%%"),FMath::RoundToInt(G->SoundVolume*100)),1130,422,25,Gold);
   Button("SoundDown",TEXT("-"),1130,477,100,50);Button("SoundUp",TEXT("+"),1250,477,100,50);
   Button("SoundMute",G->SoundVolume>0?TEXT("Mute"):TEXT("Enable sound"),1130,550,330,52);
  }
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
 else if(Box=="SoundDown")G->AdjustSoundVolume(-0.1f);
 else if(Box=="SoundUp")G->AdjustSoundVolume(0.1f);
 else if(Box=="SoundMute")G->AdjustSoundVolume(G->SoundVolume>0?-1.f:.85f);
 else if(Box=="Equipment"){G->bPaused=false;G->bShowingBuild=true;EquipmentPage=0;G->SetMenuInput(true);}
 else if(Box=="BuildPrev")EquipmentPage=FMath::Max(0,EquipmentPage-1);
 else if(Box=="BuildNext")++EquipmentPage;
 else if(Box=="Pattern"&&!G->LearnedPatterns.IsEmpty()){
  int32 I=G->LearnedPatterns.IndexOfByKey(G->StartingPattern);G->StartingPattern=G->LearnedPatterns[(I+1)%G->LearnedPatterns.Num()];
 }
 else if(Box.ToString().StartsWith("Offer"))G->ChooseReward(FCString::Atoi(*Box.ToString().Mid(5)));
}
