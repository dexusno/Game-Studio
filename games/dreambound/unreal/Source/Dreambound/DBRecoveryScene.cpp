#include "DBGameMode.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/PointLight.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/PointLightComponent.h"
#include "Sound/SoundBase.h"
#include "Components/AudioComponent.h"
#include "Kismet/GameplayStatics.h"

// Seeded assembly uses connected room modules with protected door axes.
// Layouts change routes and cover placement; authored meshes retain their scale.
void ADBGameMode::BuildRecoveryCourtyard()
{
 ResetActors();Rooms.Reset();SpawnedRooms.Reset();RoomWaves.Reset();RoomLayoutVariants.Reset();Random.Initialize(Seed);
 FRandomStream Layout(Seed);
 const FVector First=Layout.RandRange(0,1)?FVector(1,0,0):FVector(0,1,0);
 const int32 Turn=Layout.RandRange(-1,1);
 const FVector Second=FRotator(0,Turn*90.f,0).RotateVector(First);
 const FVector Centers[]={FVector::ZeroVector,First*4200,First*4200+Second*4200};
 const TCHAR* Names[]={TEXT("Bellroot court"),TEXT("The split cloister"),TEXT("The sentinel garden")};
 LayoutSignature=FString::Printf(TEXT("route:%d,%d"),First.X>.5f?0:1,Turn);
 for(int32 I=0;I<3;++I){
  FDBRoom R;R.Center=Centers[I];R.Name=Names[I];R.Parent=I-1;Rooms.Add(R);
  const int32 Variant=Layout.RandRange(0,2);RoomLayoutVariants.Add(I,Variant);
  LayoutSignature+=FString::Printf(TEXT("/cover%d:%d"),I,Variant);
 }
 for(int32 RoomIndex=0;RoomIndex<Rooms.Num();++RoomIndex){
  auto& R=Rooms[RoomIndex];const FVector C=R.Center;const int32 Variant=RoomLayoutVariants[RoomIndex];
  FRandomStream Art(Seed+RoomIndex*1709);
  Mesh("/Engine/BasicShapes/Cube.Cube",C+FVector(0,0,-70),FRotator::ZeroRotator,FVector(37,37,1),true,"M_Mortar");
  for(int32 X=-8;X<=8;++X)for(int32 Y=-8;Y<=8;++Y)
   Instance(Art.FRand()<.52f?"SM_StoneTile":"SM_StoneTile_B",C+FVector(X*200+Art.FRandRange(-4,4),Y*200+Art.FRandRange(-4,4),-24),FRotator(0,Art.RandRange(0,3)*90.f,0),FVector(.995,.995,1));
  TArray<FVector> Doors;
  if(R.Parent>=0)Doors.Add((Rooms[R.Parent].Center-C).GetSafeNormal2D());
  if(RoomIndex+1<Rooms.Num())Doors.Add((Rooms[RoomIndex+1].Center-C).GetSafeNormal2D());
  for(int32 Side=0;Side<4;++Side){
   const FVector Out=FRotator(0,Side*90.f,0).Vector(),T(-Out.Y,Out.X,0);
   bool Door=false;for(const FVector& D:Doors)if(FVector::DotProduct(D,Out)>.9f)Door=true;
   for(int32 K=-4;K<=4;++K){
    if(Door&&K==0)continue;
    Instance("SM_Wall",C+Out*1760+T*K*400,FRotator(0,Side*90.f+90,0),FVector(1,1,Side==0?1.65:1.25));
    if(FMath::Abs(K)==3)Instance("SM_Ivy",C+Out*1670+T*K*400+FVector(0,0,310),FRotator(0,Side*90.f,0),FVector(1.2));
   }
   if(Door)Instance("SM_Arch",C+Out*1760,FRotator(0,Side*90.f+90,0),FVector(.82,1.1,1.3));
  }
  // All variants leave +/-X and +/-Y door approaches clear. The two cover
  // banks move between three configurations, changing close and recall lanes.
  const FVector CoverA[]={FVector(-690,620,0),FVector(-810,800,0),FVector(-650,540,0)};
  const FVector CoverB[]={FVector(810,-860,0),FVector(650,-780,0),FVector(880,-1000,0)};
  Instance("SM_Wall",C+CoverA[Variant],FRotator(0,Variant==1?90:0,0),FVector(1.3,1,.43));
  Instance("SM_Wall",C+CoverB[Variant],FRotator(0,Variant==2?90:0,0),FVector(1.15,1,.46));
  Instance("SM_Rubble",C+CoverA[Variant]+FVector(250,-60,0),FRotator(0,32,0),FVector(1.2));
  const FVector Tree=C+FVector(Variant==1?920:600,Variant==2?1150:940,0);
  Instance("SM_BellTree",Tree,FRotator(0,-12+RoomIndex*37,0),FVector(1.2));
  Instance("SM_Canopy",Tree+FVector(0,0,744),FRotator(0,-12+RoomIndex*37,0),FVector(1.38));
  Instance("SM_Bell",Tree+FVector(-20,-130,344),FRotator(0,-9,-6),FVector(RoomIndex==2?1.9:1.5));
  // A corner gallery creates height and shadow without blocking the exits.
  for(int32 K=0;K<2;++K){
   Instance("SM_Arch",C+FVector(-1130+K*470,1340,0),FRotator::ZeroRotator,FVector(.88,1,1.2));
   Instance("SM_Wall",C+FVector(-1130+K*470,1590,375),FRotator::ZeroRotator,FVector(1.2,1,.55));
  }
  Instance("SM_PillarBroken",C+FVector(1260,-1230,0),FRotator(0,17,0),FVector(1.1));
  Instance("SM_Arch",C+FVector(1440,1480,390),FRotator(0,30,0),FVector(1.1,1,1.4));
  for(int32 Corner=0;Corner<4;++Corner){
   FVector Growth=C+FRotator(0,Corner*90.f,0).RotateVector(FVector(1350,1350,0));
   for(int32 J=0;J<4;++J){
    FVector P=Growth+FVector(Art.FRandRange(-140,140),Art.FRandRange(-140,140),0);
    Instance("SM_Fern",P,FRotator(0,Art.FRandRange(0,360),0),FVector(Art.FRandRange(.7,1.25)));
    for(int32 K=0;K<3;++K)Instance("SM_Grass",P+FVector(Art.FRandRange(-80,80),Art.FRandRange(-80,80),0),FRotator(0,Art.FRandRange(0,360),0),FVector(Art.FRandRange(.7,1.2)));
   }
  }
  FVector Ward=C+FVector(-1120,-430,76);
  if(R.Parent>=0){FVector Back=(Rooms[R.Parent].Center-C).GetSafeNormal2D();Ward=C+Back*1120+FVector(-Back.Y,Back.X,0)*300+FVector(0,0,76);}
  Instance("SM_Rubble",Ward-FVector(0,0,76),FRotator(0,24,0),FVector(1.05,1.05,.55));
  R.Altar=Mesh("SM_Crystal",Ward,FRotator::ZeroRotator,FVector(1.4),false,"M_Crystal");
 }
 for(int32 I=1;I<Rooms.Num();++I){
  const FVector A=Rooms[I-1].Center,B=Rooms[I].Center,D=(B-A).GetSafeNormal2D(),T(-D.Y,D.X,0);
  const float Yaw=D.Rotation().Yaw;
  Mesh("/Engine/BasicShapes/Cube.Cube",(A+B)*.5+FVector(0,0,-70),FRotator(0,Yaw,0),FVector(9,6,1),true,"M_Mortar");
  for(int32 K=0;K<4;++K)for(int32 Across=-1;Across<=1;++Across)
   Instance("SM_StoneTile_B",A+D*(1800+K*200)+T*Across*190+FVector(0,0,-24),FRotator(0,Yaw,0),FVector(1,.95,1));
  for(int32 Side:{-1,1})for(int32 K=0;K<2;++K)
   Instance("SM_Wall",A+D*(1900+K*400)+T*350*Side,FRotator(0,Yaw,0),FVector(1,1,.8));
  Rooms[I].Gates.Add(Mesh("/Engine/BasicShapes/Cube.Cube",(A+B)*.5+FVector(0,0,170),FRotator(0,Yaw,0),FVector(.18,6,3.4),true,"M_Core"));
 }
 UE_LOG(LogTemp,Display,TEXT("DB_LAYOUT seed=%d %s"),Seed,*LayoutSignature);
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,3000),FRotator(-38,-42,0));
 Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Sun->GetLightComponent()->SetIntensity(5.7f);Sun->GetLightComponent()->SetLightColor(FLinearColor(1,.84,.62));
 Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->bAtmosphereSunLight=true;Sun->GetLightComponent()->ShadowSharpen=.25f;Generated.Add(Sun);
 Generated.Add(GetWorld()->SpawnActor<ASkyAtmosphere>());
 auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true);
 Sky->GetLightComponent()->SetIntensity(1.25f);Sky->GetLightComponent()->RecaptureSky();Generated.Add(Sky);
 auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();Fog->GetComponent()->SetFogDensity(.013f);Fog->GetComponent()->SetFogHeightFalloff(.28f);
 Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.32,.39,.36));Generated.Add(Fog);
 auto* Post=GetWorld()->SpawnActor<APostProcessVolume>();Post->bUnbound=true;
 Post->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Post->Settings.AutoExposureApplyPhysicalCameraExposure=false;
 Post->Settings.bOverride_AutoExposureBias=true;Post->Settings.AutoExposureBias=.45f;
 Post->Settings.bOverride_BloomIntensity=true;Post->Settings.BloomIntensity=.28f;
 Post->Settings.bOverride_VignetteIntensity=true;Post->Settings.VignetteIntensity=.22f;
 Post->Settings.bOverride_ColorSaturation=true;Post->Settings.ColorSaturation=FVector4(.94,.97,.92,1);
 Post->Settings.bOverride_AmbientOcclusionIntensity=true;Post->Settings.AmbientOcclusionIntensity=.8f;Generated.Add(Post);
 auto* Lamp=GetWorld()->SpawnActor<APointLight>(FVector(-1450,-760,220),FRotator::ZeroRotator);
 Lamp->GetLightComponent()->SetMobility(EComponentMobility::Movable);Lamp->GetLightComponent()->SetIntensity(2600);Lamp->GetLightComponent()->SetLightColor(FLinearColor(1,.61,.29));Cast<UPointLightComponent>(Lamp->GetLightComponent())->SetAttenuationRadius(850);Generated.Add(Lamp);
 bSliceAwaitingStart=!IsCleared(CurrentRoomId);UpdateGates();
}
