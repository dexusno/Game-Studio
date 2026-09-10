#include "DBGameMode.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "DBProjectile.h"
#include "DBHUD.h"
#include "DBRuntimeChecks.h"
#include "DBShieldChecks.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/PointLight.h"
#include "Engine/GameViewportClient.h"
#include "Components/StaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/PlayerCameraManager.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HighResScreenshot.h"
#include "UnrealClient.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/SWindow.h"
#include "Serialization/MemoryReader.h"
#include "Serialization/MemoryWriter.h"
#include "Misc/Crc.h"
#include "AudioDevice.h"

namespace {
constexpr uint32 DBJournalMagic=0x31534244;
bool WriteJournal(UDBSave* Save,const FString& Slot){
 TArray<uint8> Payload;if(!UGameplayStatics::SaveGameToMemory(Save,Payload))return false;
 if(Payload.Num()<32||Payload.Num()>2*1024*1024)return false;
 TArray<uint8> Bytes;FMemoryWriter Writer(Bytes,true);
 uint32 Magic=DBJournalMagic,Size=Payload.Num(),Checksum=FCrc::MemCrc32(Payload.GetData(),Payload.Num());
 Writer<<Magic;Writer<<Size;Writer<<Checksum;Writer.Serialize(Payload.GetData(),Payload.Num());
 return UGameplayStatics::SaveDataToSlot(Bytes,Slot,0);
}
UDBSave* ReadJournal(const FString& Slot){
 TArray<uint8> Bytes;if(!UGameplayStatics::LoadDataFromSlot(Bytes,Slot,0)||Bytes.Num()<44||Bytes.Num()>2*1024*1024+12)return nullptr;
 FMemoryReader Reader(Bytes,true);uint32 Magic=0,Size=0,Checksum=0;Reader<<Magic;Reader<<Size;Reader<<Checksum;
 if(Magic!=DBJournalMagic||Size!=uint32(Bytes.Num()-12)||Size>2*1024*1024)return nullptr;
 const uint8* Data=Bytes.GetData()+12;
 if(FCrc::MemCrc32(Data,Size)!=Checksum)return nullptr;
 if(Data[0]!='G'||Data[1]!='V'||Data[2]!='A'||Data[3]!='S')return nullptr;
 TArray<uint8> Payload;Payload.Append(Data,Size);
 return Cast<UDBSave>(UGameplayStatics::LoadGameFromMemory(Payload));
}
}

ADBGameMode::ADBGameMode() {
 DefaultPawnClass=ADBCharacter::StaticClass(); HUDClass=ADBHUD::StaticClass();
 PrimaryActorTick.bCanEverTick=true; PrimaryActorTick.bTickEvenWhenPaused=true;
}
void ADBGameMode::BeginPlay() {
 Super::BeginPlay();
 bRecoverySlice=!FParse::Param(FCommandLine::Get(),TEXT("DBLegacyBeta"));
 if(!bRecoverySlice)SlotBase=TEXT("DreamboundBeta");
 FParse::Value(FCommandLine::Get(),TEXT("DBSaveSlot="),SlotBase);
 bVerify=FParse::Param(FCommandLine::Get(),TEXT("DBVerify"));
 if(bVerify&&!SlotBase.StartsWith(TEXT("DreamboundQA"))&&!SlotBase.StartsWith(TEXT("DBQA_")))SlotBase=TEXT("DreamboundQA_Auto");
 bCapture=FParse::Param(FCommandLine::Get(),TEXT("DBCapture"));
 bMotionCapture=FParse::Param(FCommandLine::Get(),TEXT("DBMotionCapture"));
 bCapture=bCapture||bMotionCapture;
 Player=Cast<ADBCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
 LoadProgress();
 if(FAudioDeviceHandle Audio=GetWorld()->GetAudioDevice())Audio->SetTransientPrimaryVolume(SoundVolume);
 Seed=bCanResume?StoredSave->Seed:FMath::RandRange(10000,999999);
 FParse::Value(FCommandLine::Get(),TEXT("DBSeed="),Seed);
 BuildWorld();
 if(Player) {
  Player->SetActorLocation(Rooms[0].Center+(bRecoverySlice?FVector(-1540,-800,110):FVector(-700,0,100)));
  Player->GetController()->SetControlRotation(FRotator(0,bRecoverySlice?18:0,0));
  Player->MouseSensitivity=Sensitivity;
 }
 bTitle=true; bPaused=true; SetMenuInput(true);
 if(bCapture||bVerify) {
  bTitle=false;bPaused=false;SetMenuInput(false); StartNewRun(true);
  if(bCapture&&Player&&!bMotionCapture) {
   FString DetailView;FParse::Value(FCommandLine::Get(),TEXT("DBDetailView="),DetailView);
   if(DetailView==TEXT("Fountain")){
    CurrentRoomId=0;const FVector Position=Rooms[0].Center+FVector(380,400,110);
    Player->SetActorLocation(Position);
    Player->GetController()->SetControlRotation((Rooms[0].Center+FVector(1130,1240,330)-Position-FVector(0,0,50)).Rotation());
   }else if(DetailView==TEXT("Stairs")){
    CurrentRoomId=1;const FVector Position=Rooms[1].Center+FVector(150,-1600,120);
    Player->SetActorLocation(Position);
    Player->GetController()->SetControlRotation((Rooms[1].Center+FVector(-850,-1200,75)-Position-FVector(0,0,50)).Rotation());
   }else if(!FParse::Param(FCommandLine::Get(),TEXT("DBFirstView"))){
   CurrentRoomId=1;Player->SetActorLocation(Rooms[1].Center+FVector(-980,-650,110));
   Player->GetController()->SetControlRotation(FRotator(-3,38,0));
   Player->ApplyUpgrade("Frost");Player->ApplyUpgrade("Mirror");
   FActorSpawnParameters PoseParams;PoseParams.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
   if(auto* Figure=GetWorld()->SpawnActor<ADBEnemy>(Rooms[1].Center+FVector(120,40,115),FRotator(0,215,0),PoseParams))Figure->Configure(EDBEnemyKind::Melee,99,1.f);
   }
   NotifyEvent(TEXT("REVERIE / sanctuary graphics and audio study"),FLinearColor(0.65,0.86,1));
  }
 }
 UE_LOG(LogTemp,Display,TEXT("DREAMBOUND_READY seed=%d rooms=%d saved=%d"),Seed,Rooms.Num(),bCanResume);
}
void ADBGameMode::EndPlay(const EEndPlayReason::Type Reason) {
 if(!bVerify&&!bChecksRunning&&!bCapture&&!bTitle&&!bDefeated&&(ClearedRooms.Contains(CurrentRoomId)||(bRecoverySlice?bSliceAwaitingStart:CurrentRoomId==0))) SaveProgress(!bWon);
 bEnding=true; Super::EndPlay(Reason);
}
AActor* ADBGameMode::Mesh(const FString& Name,FVector Loc,FRotator Rot,FVector Scale,bool Collision,const FString& Mat) {
 FString Path=Name.StartsWith("/")?Name:FString::Printf(TEXT("%s/Meshes/%s.%s"),Name.StartsWith("SM_RV_")?TEXT("/Game/Art/Reverie"):TEXT("/Game/Art"),*Name,*Name);
 UStaticMesh* Asset=LoadObject<UStaticMesh>(nullptr,*Path);
 if(!Asset) Asset=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
 AStaticMeshActor* A=GetWorld()->SpawnActor<AStaticMeshActor>(Loc,Rot);
 if(!A)return nullptr;
 auto* C=A->GetStaticMeshComponent(); C->SetMobility(EComponentMobility::Movable);
 C->SetStaticMesh(Asset); C->SetWorldScale3D(Scale);
 C->SetCollisionEnabled(Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
 C->SetCollisionObjectType(ECC_WorldStatic); C->SetCollisionResponseToAllChannels(ECR_Block);
 if(!Mat.IsEmpty()) {
  FString P=FString::Printf(TEXT("%s/Materials/%s.%s"),Mat.StartsWith("M_RV_")?TEXT("/Game/Art/Reverie"):TEXT("/Game/Art"),*Mat,*Mat);
  if(auto* M=LoadObject<UMaterialInterface>(nullptr,*P)) for(int32 I=0;I<C->GetNumMaterials();++I)C->SetMaterial(I,M);
 }
 Generated.Add(A);return A;
}
void ADBGameMode::Instance(const FString& Name,FVector Loc,FRotator Rot,FVector Scale,const FString& Mat) {
 FString Key=Name+Mat;
 auto** Existing=MeshBatches.Find(Key);
 UHierarchicalInstancedStaticMeshComponent* C=Existing?*Existing:nullptr;
 if(!C) {
  FString P=FString::Printf(TEXT("%s/Meshes/%s.%s"),Name.StartsWith("SM_RV_")?TEXT("/Game/Art/Reverie"):TEXT("/Game/Art"),*Name,*Name);
  auto* Asset=LoadObject<UStaticMesh>(nullptr,*P); if(!Asset)return;
  AActor* A=GetWorld()->SpawnActor<AActor>(); Generated.Add(A);
  C=NewObject<UHierarchicalInstancedStaticMeshComponent>(A);
  A->SetRootComponent(C);A->AddInstanceComponent(C);
  C->SetStaticMesh(Asset);C->SetMobility(EComponentMobility::Static);
  C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
  C->SetCollisionObjectType(ECC_WorldStatic);C->SetCollisionResponseToAllChannels(ECR_Block);
  if(Name=="SM_Grass"||Name=="SM_Root"||Name=="SM_Canopy"||Name=="SM_Fern"||Name=="SM_Rubble")C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  if(!Mat.IsEmpty()) {
   FString MP=FString::Printf(TEXT("%s/Materials/%s.%s"),Mat.StartsWith("M_RV_")?TEXT("/Game/Art/Reverie"):TEXT("/Game/Art"),*Mat,*Mat);
   if(auto* M=LoadObject<UMaterialInterface>(nullptr,*MP))for(int32 I=0;I<C->GetNumMaterials();++I)C->SetMaterial(I,M);
  }
  C->RegisterComponent();MeshBatches.Add(Key,C);
 }
 C->AddInstance(FTransform(Rot,Loc,Scale),true);
}
void ADBGameMode::ResetActors() {
 ClearRewardPractice();
 for(auto* A:Generated)if(IsValid(A))A->Destroy();
 Generated.Reset();MeshBatches.Reset();
 for(TActorIterator<ADBEnemy> It(GetWorld());It;++It)It->Destroy();
 for(TActorIterator<ADBProjectile> It(GetWorld());It;++It)It->Destroy();
}
void ADBGameMode::BuildWorld() {
 if(bRecoverySlice){BuildRecoveryCourtyard();return;}
 ResetActors();Rooms.Reset();SpawnedRooms.Reset();RoomWaves.Reset(); Random.Initialize(Seed);
 const int32 S=(Seed%2)?1:-1;
 const TArray<FVector> Centers={FVector(-3600,0,0),FVector(0,0,0),FVector(3600,0,0),FVector(3600,S*3600,0),FVector(7200,S*3600,0),FVector(10800,S*3600,0),FVector(14400,S*3600,0),FVector(3600,-S*3600,0)};
 const TArray<FString> Names={TEXT("The Waking Cell"),TEXT("Bellroot Court"),TEXT("The Split Cloister"),TEXT("The Rootwalk"),TEXT("The Bell Keep"),TEXT("Guardian's Hollow"),TEXT("Rainstack Threshold"),TEXT("The Mirror Trial")};
 for(int32 I=0;I<Centers.Num();++I){FDBRoom R;R.Center=Centers[I];R.Name=Names[I];R.Parent=I==7?2:I-1;R.bOptional=I==7;R.bTech=I==0||I==6;Rooms.Add(R);}
 for(int32 I=0;I<Rooms.Num();++I)MakeRoom(I);
 for(int32 I=1;I<Rooms.Num();++I)MakeCorridor(Rooms[I].Parent,I);
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,3000),FRotator(-36,-28,0));
 Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Sun->GetLightComponent()->SetIntensity(4.6f);Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f,0.92f,0.79f));
 Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->bAtmosphereSunLight=true;Generated.Add(Sun);
 auto* Sky=GetWorld()->SpawnActor<ASkyAtmosphere>();Generated.Add(Sky);
 auto* Fill=GetWorld()->SpawnActor<ASkyLight>();
 Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Fill->GetLightComponent()->SetIntensity(1.65f);
 Fill->GetLightComponent()->SetRealTimeCaptureEnabled(true);
 Fill->GetLightComponent()->RecaptureSky();Generated.Add(Fill);
 auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>(FVector(0,0,-200),FRotator::ZeroRotator);
 Fog->GetComponent()->SetFogDensity(0.006f);
 Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(0.24,0.31,0.32));Generated.Add(Fog);
 auto* PP=GetWorld()->SpawnActor<APostProcessVolume>();PP->bUnbound=true;
 PP->Settings.bOverride_AutoExposureBias=true;PP->Settings.AutoExposureBias=0.15f;
 PP->Settings.bOverride_BloomIntensity=true;PP->Settings.BloomIntensity=0.22f;
 PP->Settings.bOverride_VignetteIntensity=true;PP->Settings.VignetteIntensity=0.18f;
 PP->Settings.bOverride_MotionBlurAmount=true;PP->Settings.MotionBlurAmount=0;
 PP->Settings.bOverride_ColorSaturation=true;PP->Settings.ColorSaturation=FVector4(0.98f,1.f,1.02f,1.f);
 Generated.Add(PP);
 UpdateGates();
}
void ADBGameMode::MakeRoom(int32 Index) {
 auto& R=Rooms[Index];const FVector C=R.Center;
 const bool Tech=R.bTech;
 // Authored room footprint and clear door axes are preserved for every seed.
 for(int32 X=-7;X<7;++X)for(int32 Y=-7;Y<7;++Y)
  Instance("SM_StoneTile",C+FVector(X*200+100,Y*200+100,-24),FRotator(0,90*((X+Y+20)%4),0),FVector::OneVector,Tech?"M_DarkMetal":"");
 TArray<FVector> Doors;
 if(R.Parent>=0)Doors.Add((Rooms[R.Parent].Center-C).GetSafeNormal());
 for(int32 I=0;I<Rooms.Num();++I)if(Rooms[I].Parent==Index)Doors.Add((Rooms[I].Center-C).GetSafeNormal());
 for(int32 Side=0;Side<4;++Side) {
  FVector Out=FRotator(0,Side*90,0).Vector(),Tangent=FVector(-Out.Y,Out.X,0);
  bool HasDoor=false;for(const auto& D:Doors)if(FVector::DotProduct(D,Out)>0.9)HasDoor=true;
  for(int32 Segment=-3;Segment<=3;++Segment) {
   if(HasDoor&&FMath::Abs(Segment)<=1)continue;
   FVector Pos=C+Out*1400+Tangent*Segment*400;
   Instance(Tech?"SM_TechPanel":"SM_Wall",Pos,FRotator(0,Side*90+90,0),FVector(Tech?1.75:1,1,Tech?1:1.35));
   if(!Tech&&Segment%2==0)Instance("SM_Pillar",Pos+Out*35,FRotator(0,Side*90,0),FVector(1.15,1.15,1.4));
  }
  if(HasDoor) {
   Instance("SM_Arch",C+Out*1400,FRotator(0,Side*90+90,0),FVector(2.3,1.2,1.35));
  }
 }
 // Distinct cover arrangements create usable lanes; no collision prop blocks a door.
 FRandomStream Layout(Seed+Index*933);
 int32 LayoutKind=(Seed+Index*3)%3;
 for(int32 I=0;I<(Index==0?2:6);++I) {
  float A=(I*60+Layout.FRandRange(-12,12))*PI/180;
  float Radius=LayoutKind==0?720:LayoutKind==1?(I%2?930:500):800;
  FVector P=C+FVector(FMath::Cos(A)*Radius,FMath::Sin(A)*Radius,0);
  if(FMath::Abs(P.X-C.X)<270||FMath::Abs(P.Y-C.Y)<270)P.Y+=350;
  if(I%2==0)Instance(Tech?"SM_TechPanel":"SM_Pillar",P,FRotator(0,I*60,0),FVector(1.1,1.1,Index==5?0.7:0.95));
  else Instance("SM_Crate",P,FRotator(0,Layout.FRandRange(0,180),0),FVector(1.35));
 }
 // An elevated side lane with generous steps provides a deliberate vantage.
 if(Index>0&&Index<5) {
  for(int32 Step=0;Step<7;++Step)for(int32 Across=-1;Across<=1;++Across)
   Instance("SM_StoneTile",C+FVector(-850+Step*80,-1010+Across*200,0),FRotator::ZeroRotator,FVector(0.4,1,(Step+1)*1.5));
  for(int32 X=0;X<4;++X)for(int32 Y=0;Y<3;++Y)
   Instance("SM_StoneTile",C+FVector(-290+X*200,-1220+Y*200,240),FRotator::ZeroRotator,FVector::OneVector);
 }
 if(!Tech) {
  for(int32 I=0;I<14;++I) {
   float Angle=I*2*PI/14;
   FVector P=C+FVector(FMath::Cos(Angle)*1250,FMath::Sin(Angle)*1250,0);
   if(FMath::Abs(P.X-C.X)<390||FMath::Abs(P.Y-C.Y)<390)continue;
   Instance("SM_Root",P,FRotator(0,I*26,0),FVector(1.6,1.4,1.3));
   Instance("SM_Grass",P+FVector(-80,80,15),FRotator(0,I*42,0),FVector(2.2));
  }
  Instance("SM_Bell",C+FVector(850,850,320),FRotator::ZeroRotator,FVector(Index==5?2.2:1.2));
  Instance("SM_Arch",C+FVector(850,850,0),FRotator(0,25,0),FVector(1.4,1.2,1.8));
  for(int32 K=0;K<5;++K){
   FVector P=C+FVector(-1800+K*800,1800,0);
   Instance("SM_Pillar",P,FRotator(0,20,0),FVector(2.5,2.5,4.5+K%2));
   Instance("SM_Root",P+FVector(150,0,600),FRotator(0,K*53,0),FVector(4,3,6));
  }
 }
 FVector Altar=C+FVector(Index==0?350:0,Index==0?0:450,90);
 R.Altar=Mesh("SM_Crystal",Altar,FRotator::ZeroRotator,FVector(1.2),false,Index==7?"M_Storm":"M_Core");
 if(R.Altar)R.Altar->SetActorHiddenInGame(Index!=0&&!IsCleared(Index));
 auto* Light=GetWorld()->SpawnActor<APointLight>(Altar+FVector(0,0,110),FRotator::ZeroRotator);
 Light->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Light->GetLightComponent()->SetIntensity(Tech?8000:1800);
 Light->GetLightComponent()->SetLightColor(Tech?FLinearColor(0.12,0.6,1):FLinearColor(0.25,0.8,0.65));
 Cast<UPointLightComponent>(Light->GetLightComponent())->SetAttenuationRadius(650);Generated.Add(Light);
}
void ADBGameMode::MakeCorridor(int32 From,int32 To) {
 FVector A=Rooms[From].Center,B=Rooms[To].Center,D=(B-A).GetSafeNormal(),T(-D.Y,D.X,0);
 for(float Along=1400;Along<2200;Along+=200)for(int32 Across=-2;Across<2;++Across)
  Instance("SM_StoneTile",A+D*(Along+100)+T*(Across*200+100)+FVector(0,0,-24),FRotator::ZeroRotator,FVector::OneVector);
 for(int32 Side:{-1,1})for(int32 K=0;K<2;++K)
  Instance("SM_Wall",A+D*(1550+K*400)+T*500*Side,FRotator(0,D.Rotation().Yaw,0),FVector(1,1,0.65));
 if(To!=7) {
  AActor* Gate=Mesh("/Engine/BasicShapes/Cube.Cube",(A+B)*0.5+FVector(0,0,160),FRotator(0,D.Rotation().Yaw,0),FVector(0.16,9,3.2),true,"M_Core");
  Rooms[To].Gates.Add(Gate);
 }
}
void ADBGameMode::UpdateGates() {
 if(bRecoverySlice){
  for(int32 I=1;I<Rooms.Num();++I){
   bool Open=ClaimedRooms.Contains(Rooms[I].Parent)&&!(I==CurrentRoomId&&SpawnedRooms.Contains(I)&&!IsCleared(I));
   for(auto* Gate:Rooms[I].Gates)if(IsValid(Gate)){Gate->SetActorHiddenInGame(Open);Gate->SetActorEnableCollision(!Open);}
  }
  for(int32 I=0;I<Rooms.Num();++I)if(IsValid(Rooms[I].Altar))
   Rooms[I].Altar->SetActorHiddenInGame(I!=CurrentRoomId||(!bSliceAwaitingStart&&!IsCleared(I))||ClaimedRooms.Contains(I));
  return;
 }
 for(int32 I=1;I<Rooms.Num();++I){
  bool Open=ClaimedRooms.Contains(Rooms[I].Parent)||I==7;
  for(auto* G:Rooms[I].Gates)if(IsValid(G)){G->SetActorHiddenInGame(Open);G->SetActorEnableCollision(!Open);}
 }
 for(int32 I=0;I<Rooms.Num();++I)if(IsValid(Rooms[I].Altar))
  Rooms[I].Altar->SetActorHiddenInGame(ClaimedRooms.Contains(I)||(!IsCleared(I)&&I!=0));
}
void ADBGameMode::ActivateRoom(int32 Index) {
 CurrentRoomId=Index;CurrentWave=RoomWaves.FindRef(Index);
 if(bRecoverySlice){bSliceAwaitingStart=!IsCleared(Index)&&!SpawnedRooms.Contains(Index);UpdateGates();return;}
 if(Index==0){ClearedRooms.AddUnique(0);UpdateGates();return;}
 if(IsCleared(Index)||SpawnedRooms.Contains(Index))return;
 SpawnedRooms.Add(Index);
 SaveProgress(true); SpawnWave(Index,0);
 NotifyEvent(Rooms[Index].bOptional?TEXT("MIRROR TRIAL / clear the defenders to learn Mirror Facet"):FString::Printf(TEXT("%s / break the ward"),*Rooms[Index].Name),FLinearColor(0.96,0.79,0.49));
 if(auto* S=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/Reverie/S_Encounter.S_Encounter")))UGameplayStatics::PlaySound2D(this,S,0.55f);
}
void ADBGameMode::SpawnWave(int32 Index,int32 Wave) {
 RoomWaves.Add(Index,Wave);if(CurrentRoomId==Index)CurrentWave=Wave;
 FRandomStream Rng(Seed+Index*7919+Wave*67);
 int32 Count=bRecoverySlice?(Index==0?1:Index==1?3:2):Index==5?1:Index==6?3:Index==7?5:3+Index;
 if(Wave>0)Count=Index==5?2:2+Index/2;
 for(int32 I=0;I<Count;++I) {
  float Angle=(I*360.f/Count+Rng.FRandRange(-20,20))*PI/180;
  FVector Pos=Rooms[Index].Center+FVector(FMath::Cos(Angle)*850,FMath::Sin(Angle)*850,110);
  if(bRecoverySlice){const FVector Points[]={FVector(420,-580,110),FVector(1260,660,110),FVector(-340,860,110)};Pos=Rooms[Index].Center+Points[I%3]+FVector(0,(Seed%2?1:-1)*Rng.FRandRange(0,100),0);}
  if(Player&&FVector::Dist2D(Pos,Player->GetActorLocation())<650)Pos=Rooms[Index].Center+(Rooms[Index].Center-Player->GetActorLocation()).GetSafeNormal2D()*650+FVector(0,0,110);
  EDBEnemyKind Kind=(Index==5&&Wave==0)?EDBEnemyKind::Boss:static_cast<EDBEnemyKind>(Rng.RandRange(0,2));
  if(bRecoverySlice)Kind=Index==2?(I==0?EDBEnemyKind::Boss:EDBEnemyKind::Caster):I==0?EDBEnemyKind::Melee:I==1?EDBEnemyKind::Caster:EDBEnemyKind::Hunter;
  FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
  auto* E=GetWorld()->SpawnActor<ADBEnemy>(Pos,FRotator::ZeroRotator,Params);
  if(E){E->Configure(Kind,Index,bRecoverySlice?(Index==0?0.85f:Index==2?0.9f:1.f):Index==5?1.f:0.9f+Index*0.08f);E->SetArenaBounds(Rooms[Index].Center,bRecoverySlice?FVector2D(1540,1540):FVector2D(1250,1250));}
 }
}
void ADBGameMode::NotifyEnemyKilled(ADBEnemy* Enemy) {
 if(!Enemy||Enemy->RoomId<0||bEnding||bDefeated)return;
 ++Kills;int32 Room=Enemy->RoomId;
 int32 Living=0;for(TActorIterator<ADBEnemy> It(GetWorld());It;++It)if(!It->bDead&&It->RoomId==Room)++Living;
 if(Living==0) {
  if(!bRecoverySlice&&RoomWaves.FindRef(Room)==0&&(Room==3||Room==4)) {SpawnWave(Room,1);NotifyEvent(TEXT("The ward answers / reinforcements"),FLinearColor(1,0.56,0.31));}
  else FinishRoom(Room);
 }
}
void ADBGameMode::FinishRoom(int32 Index) {
 ClearedRooms.AddUnique(Index);
 // Defeating the last opponent resolves its remaining bolts too. A late
 // projectile must not change an earned clear into defeat during the result delay.
 for(TActorIterator<ADBProjectile> It(GetWorld());It;++It)It->Destroy();
 if(Player)Player->Health=FMath::Min(Player->MaxHealth,Player->Health+30);
 if(Index==5){bBossWon=true;LearnedPatterns.AddUnique("Capacitor");}
 if((bRecoverySlice&&Index==2)||(!bRecoverySlice&&Index==6)){
  ClaimedRooms.AddUnique(Index);
  if(bRecoverySlice){bBossWon=true;LearnedPatterns.AddUnique("Capacitor");}
  NotifyEvent(bRecoverySlice?TEXT("COURTYARD CLEARED / your patterns endure"):TEXT("CROSSING COMPLETE / your discoveries endured"),FLinearColor(0.55,1,0.85));
  if(bRecoverySlice){SaveProgress(false);VictoryDelay=1.4f;if(Player)Player->RecallShield();}
  else {bWon=true;SaveProgress(false);SetMenuInput(true);}
 } else {
  UpdateGates();SaveProgress(true);
  NotifyEvent(Index==5?TEXT("GUARDIAN BROKEN / claim its living capacitor"):TEXT("WARD BROKEN / recover an attachment"),FLinearColor(0.5,1,0.77));
 }
 if(auto* S=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/Reverie/S_Clear.S_Clear")))UGameplayStatics::PlaySound2D(this,S,0.8f);
}
void ADBGameMode::NotifyPlayerDied() {
 if(!PracticeReward.IsNone()&&Player){Player->Health=Player->MaxHealth;Player->bDead=false;return;}
 if(bDefeated)return;
 bDefeated=true;bCanResume=false;SaveProgress(false);SetMenuInput(true);
 NotifyEvent(TEXT("Connection lost / learned patterns remain"),FLinearColor(1,0.48,0.38));
}
void ADBGameMode::NotifyEvent(const FString& Text,FLinearColor Color){EventText=Text;EventColor=Color;EventRemaining=4.5f;UE_LOG(LogTemp,Display,TEXT("DB_EVENT %s"),*Text);}
void ADBGameMode::Tick(float Dt) {
 Super::Tick(Dt);PulseTime+=Dt;
 if(!Player)Player=Cast<ADBCharacter>(UGameplayStatics::GetPlayerCharacter(this,0));
 if(bDefeated)VictoryDelay=0;
 if(VictoryDelay>0){VictoryDelay=FMath::Max(0.f,VictoryDelay-Dt);if(VictoryDelay==0){bWon=true;SaveProgress(false);SetMenuInput(true);}}
 if(bVerify&&PulseTime>3){bVerify=false;RunVerification();return;}
 if(bMotionCapture){TickMotionDemo(Dt);return;}
 if(bCapture){
  if(PulseTime>8&&!bCapturedFrame){
   bCapturedFrame=true;FString DetailView;FParse::Value(FCommandLine::Get(),TEXT("DBDetailView="),DetailView);
   FScreenshotRequest::RequestScreenshot(DetailView==TEXT("Fountain")?TEXT("Reverie_Fountain.png"):DetailView==TEXT("Stairs")?TEXT("Reverie_Stairs.png"):TEXT("Dreambound_FirstView.png"),true,false);
  }
  if(PulseTime>12)FGenericPlatformMisc::RequestExit(false);
 }
 if(!bCapture&&!bVerify&&!bChecksRunning&&!bTitle&&!bDefeated&&!bWon&&GEngine&&GEngine->GameViewport) {
  UGameViewportClient* V=GEngine->GameViewport;auto W=V->GetWindow();
  bool Focus=FSlateApplication::IsInitialized()&&FSlateApplication::Get().IsActive()&&W.IsValid()&&W->IsActive();
  if(bHadFocus&&!Focus&&!bPaused&&!bChoosingReward&&!bShowingBuild){bPaused=true;SetMenuInput(true);}
  bHadFocus=Focus;
 }
 if(bTitle||bPaused||bChoosingReward||bShowingBuild||bDefeated||bWon||!Player)return;
 RunSeconds+=Dt;EventRemaining=FMath::Max(0.f,EventRemaining-Dt);
 for(int32 I=0;I<Rooms.Num();++I){
  if(IsValid(Rooms[I].Altar)){Rooms[I].Altar->AddActorLocalRotation(FRotator(0,Dt*20,0));}
 }
 if(Player->GetActorLocation().Z<-400){Player->ReceiveAttack(99999,Player->GetActorLocation(),true);return;}
 if(bRecoverySlice){
  if(!PracticeReward.IsNone())Player->Health=Player->MaxHealth;
  for(int32 I=0;I<Rooms.Num();++I){
   FVector D=Player->GetActorLocation()-Rooms[I].Center;
   if(I!=CurrentRoomId&&FMath::Abs(D.X)<1510&&FMath::Abs(D.Y)<1510&&(I==0||ClaimedRooms.Contains(Rooms[I].Parent)||IsCleared(I))){
    ClearRewardPractice();ActivateRoom(I);SaveProgress(true);
    NotifyEvent(FString::Printf(TEXT("%s / approach the ward stone"),*Rooms[I].Name),FLinearColor(.72,.9,.83));break;
   }
  }
  return;
 }
 for(int32 I=0;I<Rooms.Num();++I) {
  FVector D=Player->GetActorLocation()-Rooms[I].Center;
  if(FMath::Abs(D.X)<1250&&FMath::Abs(D.Y)<1250&&I!=CurrentRoomId) {
   if(I==0||I==7||ClaimedRooms.Contains(Rooms[I].Parent)||IsCleared(I))ActivateRoom(I);
   break;
  }
 }
}
void ADBGameMode::Interact() {
 if(bTitle){if(bCanResume)ResumeRun();else StartNewRun();return;}
 if(bPaused||bChoosingReward||bShowingBuild||bDefeated||bWon||!Player)return;
 if(bRecoverySlice){
  auto* Altar=Rooms[CurrentRoomId].Altar;
  if(!IsValid(Altar)||FVector::Dist(Player->GetActorLocation(),Altar->GetActorLocation())>430)return;
  if(bSliceAwaitingStart){ClearRewardPractice();bSliceAwaitingStart=false;SpawnedRooms.Add(CurrentRoomId);SaveProgress(true);SpawnWave(CurrentRoomId,0);UpdateGates();NotifyEvent(CurrentRoomId==0?TEXT("Hold LMB to light pieces. Release to throw them. RMB guards with the rest."):TEXT("Choose how many pieces to risk. Q recalls; F is your heavy strike."),FLinearColor(.93,.78,.49));return;}
  if(IsCleared(CurrentRoomId)&&!ClaimedRooms.Contains(CurrentRoomId))ShowOffers(CurrentRoomId);
  return;
 }
 for(int32 I=0;I<Rooms.Num();++I)
  if(!ClaimedRooms.Contains(I)&&(I==0||IsCleared(I))&&IsValid(Rooms[I].Altar)&&FVector::Dist(Player->GetActorLocation(),Rooms[I].Altar->GetActorLocation())<420){ShowOffers(I);return;}
}
FString ADBGameMode::InteractText() const {
 if(!Player||bTitle)return "";
 if(bRecoverySlice){
  auto* A=Rooms[CurrentRoomId].Altar;
  if(IsValid(A)&&FVector::Dist(Player->GetActorLocation(),A->GetActorLocation())<430){
   if(bSliceAwaitingStart)return CurrentRoomId==0?TEXT("E  Begin the encounter"):CurrentRoomId==1?TEXT("E  Test your new attachment"):TEXT("E  Face the heavy sentinel");
   if(IsCleared(CurrentRoomId)&&!ClaimedRooms.Contains(CurrentRoomId))return TEXT("E  Claim your earned attachment");
  }
  return "";
 }
 for(int32 I=0;I<Rooms.Num();++I)if(!ClaimedRooms.Contains(I)&&(I==0||IsCleared(I))&&IsValid(Rooms[I].Altar)&&FVector::Dist(Player->GetActorLocation(),Rooms[I].Altar->GetActorLocation())<420)
 return I==5?TEXT("E  Claim the guardian's capacitor"):TEXT("E  Recover an attachment");
 return "";
}
FString ADBGameMode::ObjectiveText() const {
 if(!Rooms.IsValidIndex(CurrentRoomId))return "";
 if(bRecoverySlice){
  if(bWon)return TEXT("The court is clear. Your learned patterns remain.");
  if(bSliceAwaitingStart)return TEXT("Approach the ward stone / E when ready");
  if(IsCleared(CurrentRoomId))return ClaimedRooms.Contains(CurrentRoomId)?TEXT("Practice your new ability or follow the open passage"):TEXT("Attachment earned / return to the ward stone");
  int32 N=0;for(TActorIterator<ADBEnemy> It(GetWorld());It;++It)if(It->RoomId==CurrentRoomId&&!It->bDead)++N;
  return FString::Printf(TEXT("%d %s / kept pieces are your protection"),N,N==1?TEXT("threat remains"):TEXT("threats remain"));
 }
 if(bWon)return TEXT("The dreams were real. So is what you brought with you.");
 if(CurrentRoomId==0)return ClaimedRooms.Contains(0)?TEXT("Leave the cell / follow the light through the arch"):TEXT("Recover the dormant core / approach the crystal");
 if(IsCleared(CurrentRoomId))return ClaimedRooms.Contains(CurrentRoomId)?Rooms[CurrentRoomId].bOptional?TEXT("Trial complete / return to the Split Cloister"):TEXT("Ward open / follow the passage to the next realm anchor"):TEXT("Claim the ward's attachment / approach the crystal");
 int32 N=0;for(TActorIterator<ADBEnemy> It(GetWorld());It;++It)if(It->RoomId==CurrentRoomId&&!It->bDead)++N;
 return FString::Printf(TEXT("%s / %d defenders remain"),CurrentRoomId==5?TEXT("Break the Bell Guardian"):CurrentRoomId==6?TEXT("Test your carried power"):TEXT("Break the ward"),N);
}
FDBOffer ADBGameMode::DescribeUpgrade(FName Id) const {
 FDBOffer O;O.Id=Id;O.Name=Id.ToString();O.Color=FLinearColor(.7f,.85f,1.f);
 if(Id=="Mirror"){O.Name="Mirror Facet";O.Color=FLinearColor(.7f,.93f,1.f);}
 else if(Id=="Anchor"){O.Name="Anchor Spindle";O.Color=FLinearColor(.48f,.9f,.77f);}
 else if(Id=="Ram"){O.Name="Ram Edge";O.Color=FLinearColor(1.f,.74f,.43f);}
 else if(Id=="Echo"){O.Name="Echo Chamber";O.Color=FLinearColor(.82f,.66f,1.f);}
 else if(Id=="Frost"){O.Name="Frost Core";O.Color=FLinearColor(.48f,.86f,1.f);}
 else if(Id=="Storm"){O.Name="Storm Core";O.Color=FLinearColor(.88f,.75f,1.f);}
 else if(Id=="Ember"){O.Name="Ember Core";O.Color=FLinearColor(1.f,.48f,.26f);}
 else if(Id=="Stormfracture"){O.Name="Stormfracture";O.Color=FLinearColor(.61f,1.f,.78f);}
 else if(Id=="Split"){O.Name="Split Prism";O.Color=FLinearColor(1.f,.85f,.51f);}
 else if(Id=="Capacitor"){O.Name="Living Capacitor";O.Color=FLinearColor(1.f,.91f,.55f);}
 if(Id=="Restore"){O.Name="Restore the instrument";O.Description="All offered attachments are fully evolved. Restore health and guard.";}
 else O.Description=ADBCharacter::GetUpgradeDescription(Id,FMath::Min(3,(Player?Player->GetUpgradeRank(Id):0)+1));
 return O;
}
void ADBGameMode::ShowOffers(int32 Index) {
 RewardRoom=Index;Offers.Reset();
 TArray<FName> Pool;
 if(bRecoverySlice)Pool=Index==0?TArray<FName>{"Anchor","Mirror","Ram"}:TArray<FName>{"Frost","Storm","Ember"};
 else if(Index==0)Pool={"Frost","Ram","Echo","Mirror","Split"};
 else if(Index==5)Pool={"Capacitor"};
 else if(Index==7)Pool={"Mirror","Storm","Echo"};
 else Pool={"Mirror","Ram","Echo","Frost","Storm","Ember","Split","Stormfracture"};
 FRandomStream OfferRandom(Seed+Index*1543);
 Pool.RemoveAll([this](FName Id){return Player&&Player->GetUpgradeRank(Id)>=3;});
 {
  for(int32 I=0;I<3&&Pool.Num()>0;++I){
   int32 Pick=Index==7?0:OfferRandom.RandRange(0,Pool.Num()-1);FName Id=Pool[Pick];Pool.RemoveAt(Pick);
   if(Id=="Stormfracture"&&Player&&!Player->HasUpgrade("Frost")&&!Player->HasUpgrade("Storm"))Id="Frost";
   bool Duplicate=false;for(const auto& O:Offers)if(O.Id==Id)Duplicate=true;
   if(Duplicate){--I;continue;}
   Offers.Add(DescribeUpgrade(Id));
  }
 }
 if(Offers.IsEmpty())Offers.Add(DescribeUpgrade("Restore"));
 bChoosingReward=true;SetMenuInput(true);
}
void ADBGameMode::ChooseReward(int32 Index) {
 if(!bChoosingReward||!Offers.IsValidIndex(Index)||!Player)return;
 FName Id=Offers[Index].Id; ClaimReward(Id);
}
void ADBGameMode::ClaimReward(FName Id) {
 if(!Player||ClaimedRooms.Contains(RewardRoom))return;
 if(Id=="Restore"){Player->Health=Player->MaxHealth;Player->GuardEnergy=Player->MaxGuardEnergy;}
 else {Player->ApplyUpgrade(Id);LearnedPatterns.AddUnique(Id);if(StartingPattern.IsNone())StartingPattern=Id;}
 ClaimedRooms.AddUnique(RewardRoom);ClearedRooms.AddUnique(RewardRoom);
 EarnedRoomRewards.Add(RewardRoom,Id);
 if(RewardRoom==5)bBossWon=true;
 bChoosingReward=false;Offers.Reset();UpdateGates();SaveProgress(true);SetMenuInput(false);
 NotifyEvent(Id=="Restore"?TEXT("Health and guard restored / passage opened"):FString::Printf(TEXT("%s / pattern learned and installed"),*DescribeUpgrade(Id).Name),DescribeUpgrade(Id).Color);
 if(RewardRoom==5)NotifyEvent(TEXT("The passage is open / take your build into Rainstack"),FLinearColor(0.8,1,0.9));
 if(bRecoverySlice&&RewardRoom<2){BeginRewardPractice(Id);NotifyEvent(TEXT("Installed / practice here, then follow the open passage"),FLinearColor(.64,.94,.82));}
}
void ADBGameMode::SetMenuInput(bool On) {
 On=On||bSaveFailed;
 if(Player){
  Player->SuspendCombatInput();Player->GetCharacterMovement()->StopMovementImmediately();
  // Menus use the world as their backdrop; the first-person assembly belongs
  // to active play and can otherwise render before its first camera update.
  Player->SetActorHiddenInGame(On);
 }
 auto* PC=UGameplayStatics::GetPlayerController(this,0);if(!PC)return;
 PC->bShowMouseCursor=On;PC->bEnableClickEvents=true;
 PC->ResetIgnoreMoveInput();PC->ResetIgnoreLookInput();
 if(On){PC->SetIgnoreMoveInput(true);PC->SetIgnoreLookInput(true);FInputModeGameAndUI Input;Input.SetHideCursorDuringCapture(false);Input.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);PC->SetInputMode(Input);}
 else {FInputModeGameOnly Input;Input.SetConsumeCaptureMouseDown(false);PC->SetInputMode(Input);}
 // The opening menu pauses before the first controller tick. Prime its camera
 // so first-person meshes use the actual eye position rather than the spawn origin.
 if(Player&&PC->GetViewTarget()!=Player)PC->SetViewTarget(Player);
 if(PC->PlayerCameraManager)PC->PlayerCameraManager->UpdateCamera(0.f);
 PC->FlushPressedKeys();UGameplayStatics::SetGamePaused(this,On);
}
void ADBGameMode::TogglePause() {
 if(bTitle||bWon||bDefeated)return;
 if(bChoosingReward)return;
 if(bShowingBuild){bShowingBuild=false;SetMenuInput(false);return;}
 bPaused=!bPaused;SetMenuInput(bPaused);
}
void ADBGameMode::ToggleBuild() {
 if(bTitle||bPaused||bChoosingReward||bWon||bDefeated)return;
 bShowingBuild=!bShowingBuild;SetMenuInput(bShowingBuild);
}
void ADBGameMode::AdjustSensitivity(float Delta) {
 Sensitivity=FMath::Clamp(Sensitivity+Delta,0.25f,2.5f);
 if(Player)Player->MouseSensitivity=Sensitivity;
 if(StoredSave){StoredSave->Sensitivity=Sensitivity;StoredSave->SoundVolume=SoundVolume;WriteJournal(StoredSave,SlotBase+TEXT("_settings"));}
}
void ADBGameMode::AdjustSoundVolume(float Delta) {
 SoundVolume=FMath::Clamp(SoundVolume+Delta,0.f,1.f);
 if(FAudioDeviceHandle Audio=GetWorld()->GetAudioDevice())Audio->SetTransientPrimaryVolume(SoundVolume);
 if(!StoredSave)StoredSave=Cast<UDBSave>(UGameplayStatics::CreateSaveGameObject(UDBSave::StaticClass()));
 if(StoredSave){StoredSave->SoundVolume=SoundVolume;StoredSave->Sensitivity=Sensitivity;WriteJournal(StoredSave,SlotBase+TEXT("_settings"));}
}
void ADBGameMode::StartNewRun(bool SameSeed) {
 if(bSaveFailed){SaveProgress(!bDefeated&&!bWon);if(bSaveFailed)return;}
 UGameplayStatics::SetGamePaused(this,false);
 if(!SameSeed)Seed=FMath::RandRange(10000,999999);
 ++Expeditions;ClearedRooms.Reset();ClaimedRooms.Reset();EarnedRoomRewards.Reset();Offers.Reset();CurrentRoomId=0;CurrentWave=0;Kills=0;RunSeconds=0;
 bTitle=bPaused=bChoosingReward=bShowingBuild=bWon=bDefeated=false;VictoryDelay=0;
 BuildWorld();
 if(Player){Player->Upgrades.Reset();Player->CurrentElement=EDBElement::Neutral;Player->OnRunReset();Player->MouseSensitivity=Sensitivity;
  Player->SetActorLocation(Rooms[0].Center+(bRecoverySlice?FVector(-1540,-800,110):FVector(-750,0,100)));Player->GetController()->SetControlRotation(FRotator(0,bRecoverySlice?18:0,0));
  if(!StartingPattern.IsNone()&&LearnedPatterns.Contains(StartingPattern))Player->ApplyUpgrade(StartingPattern);
 }
 ActivateRoom(0);SaveProgress(true);SetMenuInput(false);
 NotifyEvent(bRecoverySlice?TEXT("Hold LMB to select pieces; release to throw. Q recalls. F heavy strike."):TEXT("Find the core / E to recover / Tab to inspect controls"),FLinearColor(0.78,0.88,1));
}
void ADBGameMode::ResumeRun() {
 if(!StoredSave||!StoredSave->bActiveRun){StartNewRun();return;}
 UDBSave* S=StoredSave;
 Seed=S->Seed;ClearedRooms=S->Cleared;ClaimedRooms=S->Claimed;EarnedRoomRewards=S->RoomRewards;
 CurrentRoomId=FMath::Clamp(S->CurrentRoom,0,bRecoverySlice?2:7);
 bTitle=bPaused=bChoosingReward=bShowingBuild=bWon=bDefeated=false;VictoryDelay=0;
 BuildWorld();
 if(Player){Player->OnRunReset();Player->MouseSensitivity=Sensitivity;Player->Upgrades.Reset();
  for(auto& P:S->Upgrades)for(int32 I=0;I<P.Value;++I)Player->ApplyUpgrade(P.Key);
  Player->CurrentElement=static_cast<EDBElement>(S->Element);Player->RefreshEquipmentVisuals();Player->Health=S->Health;
  FVector Entry=Rooms[CurrentRoomId].Parent>=0?(Rooms[Rooms[CurrentRoomId].Parent].Center-Rooms[CurrentRoomId].Center).GetSafeNormal()*1000:FVector(-750,0,0);
  if(bRecoverySlice)Entry=GetRoomEntryPoint(CurrentRoomId)-Rooms[CurrentRoomId].Center-FVector(0,0,110);
  Player->SetActorLocation(Rooms[CurrentRoomId].Center+Entry+FVector(0,0,100));
  Player->GetController()->SetControlRotation((-Entry).Rotation());
 }
 int32 R=CurrentRoomId;ActivateRoom(R);SetMenuInput(false);
 if(bRecoverySlice&&ClaimedRooms.Contains(R)&&EarnedRoomRewards.Contains(R))BeginRewardPractice(EarnedRoomRewards[R]);
 if(bRecoverySlice&&ClaimedRooms.Contains(2)){bWon=true;SaveProgress(false);SetMenuInput(true);return;}
 NotifyEvent(TEXT("Connection restored / current encounter restarts at its checkpoint"),FLinearColor(0.65,0.9,1));
}
void ADBGameMode::SaveProgress(bool Active) {
 if(bVerify||bCapture)return;
 if(bRecoverySlice&&ClaimedRooms.Contains(2))Active=false;
 auto* S=Cast<UDBSave>(UGameplayStatics::CreateSaveGameObject(UDBSave::StaticClass()));
 S->Revision=++SaveRevision;S->Seed=Seed;S->CurrentRoom=CurrentRoomId;S->Cleared=ClearedRooms;S->Claimed=ClaimedRooms;
 S->RoomRewards=EarnedRoomRewards;
 S->Patterns=LearnedPatterns;S->bActiveRun=Active;S->bBossWon=bBossWon;S->Expeditions=Expeditions;S->Sensitivity=Sensitivity;S->SoundVolume=SoundVolume;
 if(Player){S->Upgrades=Player->Upgrades;S->Health=Player->Health;S->Element=static_cast<int32>(Player->CurrentElement);}
 FString Slot=SlotBase+FString::Printf(TEXT("_%d"),S->Revision%2);
 bool Ok=WriteJournal(S,Slot);
 auto* Verify=Ok?ReadJournal(Slot):nullptr;
 if(Verify&&Verify->Revision==S->Revision){StoredSave=S;bCanResume=Active;bSaveFailed=false;SaveNotice=TEXT("Checkpoint saved");}
 else {bSaveFailed=true;bPaused=true;SetMenuInput(true);SaveNotice=TEXT("Save failed / previous checkpoint retained");UE_LOG(LogTemp,Error,TEXT("DB_SAVE_FAILED slot=%s"),*Slot);}
}
void ADBGameMode::LoadProgress() {
 for(int32 I=0;I<2;++I){
  auto* S=ReadJournal(SlotBase+FString::Printf(TEXT("_%d"),I));
  if(S&&S->Version==1&&S->Seed>=0&&S->CurrentRoom>=0&&S->CurrentRoom<8&&(!StoredSave||S->Revision>StoredSave->Revision))StoredSave=S;
 }
 if(StoredSave){LearnedPatterns=StoredSave->Patterns;bBossWon=StoredSave->bBossWon;Expeditions=StoredSave->Expeditions;SaveRevision=StoredSave->Revision;Sensitivity=StoredSave->Sensitivity;SoundVolume=FMath::Clamp(StoredSave->SoundVolume,0.f,1.f);bCanResume=StoredSave->bActiveRun;if(!LearnedPatterns.IsEmpty())StartingPattern=LearnedPatterns[0];}
 if(auto* Settings=ReadJournal(SlotBase+TEXT("_settings"))){Sensitivity=Settings->Sensitivity;SoundVolume=FMath::Clamp(Settings->SoundVolume,0.f,1.f);}
}
void ADBGameMode::QuitGame(bool bWithoutSaving) {
 if(bWithoutSaving){FGenericPlatformMisc::RequestExit(false);return;}
 if(!bDefeated&&!bWon&&!bTitle&&(IsCleared(CurrentRoomId)||(bRecoverySlice?bSliceAwaitingStart:CurrentRoomId==0)))SaveProgress(true);
 if(bSaveFailed)return;
 FGenericPlatformMisc::RequestExit(false);
}
void ADBGameMode::RunVerification() {
 if(bRecoverySlice){bChecksRunning=true;DBStartPhysicalShieldChecks(*this);return;}
 // Executed in the real game world with an isolated save slot; no fun claim.
 FString Result;int32 Passed=0,Failed=0;
 auto Check=[&](bool Good,const TCHAR* Name){Result+=FString::Printf(TEXT("%s %s\n"),Good?TEXT("PASS"):TEXT("FAIL"),Name);Good?++Passed:++Failed;};
 Check(Player!=nullptr,TEXT("spawned first-person character"));
 Check(Rooms.Num()==8,TEXT("eight connected authored spaces"));
 Check(Rooms[7].Parent==2&&Rooms[7].bOptional,TEXT("optional pursuit branch"));
 Check(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Art/Meshes/SM_WeaponBody.SM_WeaponBody"))!=nullptr,TEXT("authored weapon available"));
 if(Player){
  for(FName Id:{FName("Frost"),FName("Ram"),FName("Stormfracture"),FName("Echo"),FName("Mirror"),FName("Capacitor")})Player->ApplyUpgrade(Id);
  Check(Player->HasUpgrade("Frost")&&Player->HasUpgrade("Ram")&&Player->HasUpgrade("Stormfracture"),TEXT("contrasting combination installed"));
  float H=Player->Health;Player->ReceiveAttack(12,Player->GetActorLocation()+FVector(300,0,0),true);
  Check(Player->Health<H,TEXT("real damage path"));
  Player->Health=Player->MaxHealth;
 }
 CurrentRoomId=1;SpawnWave(1,0);int32 Before=Kills;
 TArray<ADBEnemy*> Targets;for(TActorIterator<ADBEnemy> It(GetWorld());It;++It)if(It->RoomId==1)Targets.Add(*It);
 for(auto* E:Targets){FDBHit Hit;Hit.Damage=100000;Hit.Source=Player?Player->GetActorLocation():FVector::ZeroVector;Hit.InstigatorActor=Player;E->ApplyCombatHit(Hit);}
 Check(Kills>Before&&IsCleared(1),TEXT("enemy defeat resolves actual encounter"));
 RewardRoom=1;ClaimReward("Echo");
 Check(ClaimedRooms.Contains(1)&&LearnedPatterns.Contains("Echo"),TEXT("earned reward recorded"));
 SaveProgress(true);int32 Revision=SaveRevision;StoredSave=nullptr;LoadProgress();
 Check(StoredSave&&StoredSave->Revision==Revision&&StoredSave->Claimed.Contains(1),TEXT("saved claims reload"));
 FString Path=FPaths::ProjectSavedDir()/TEXT("Verification.txt");
 FIntPoint Independent=DBRunRuntimeChecks(*this,Result);Passed+=Independent.X;Failed+=Independent.Y;
 Result+=FString::Printf(TEXT("RESULT %d passed %d failed\n"),Passed,Failed);
 FFileHelper::SaveStringToFile(Result,*Path);UE_LOG(LogTemp,Display,TEXT("DB_VERIFY %s"),*Result);
 FGenericPlatformMisc::RequestExit(false);
}
