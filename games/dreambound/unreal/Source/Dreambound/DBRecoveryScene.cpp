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
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
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
 // Foliage remains noncolliding. Reachable stone masses use fitted import
 // hulls; their placement preserves the entrance axes and combat lanes.
 auto Garden=[this](const TCHAR* Name,FVector Location,FRotator Rotation,FVector Scale){
  Instance(Name,Location,Rotation,Scale);
  if(auto** Batch=MeshBatches.Find(FString(Name)))
   (*Batch)->SetCollisionEnabled(FString(Name)==TEXT("SM_GardenRockCluster")||FString(Name)==TEXT("SM_GardenButtress")
    ?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
 };
 for(int32 RoomIndex=0;RoomIndex<Rooms.Num();++RoomIndex){
  auto& R=Rooms[RoomIndex];const FVector C=R.Center;const int32 Variant=RoomLayoutVariants[RoomIndex];
  FRandomStream Art(Seed+RoomIndex*1709);
  FRandomStream GardenArt(Seed+199903+RoomIndex*8209);
  Mesh("/Engine/BasicShapes/Cube.Cube",C+FVector(0,0,-70),FRotator::ZeroRotator,FVector(37,37,1),true,"M_Mortar");
  for(int32 X=-8;X<=8;++X)for(int32 Y=-8;Y<=8;++Y){
   // A gently worn broad cross connects every possible seeded entrance. Only
   // the material changes: tile heights/footprints and foundation stay intact.
   const float Ribbon=110.f*FMath::Sin((X*200.f+1600.f)*.0018f+RoomIndex*.6f);
   const bool WornPath=FMath::Abs(Y*200.f-Ribbon)<430.f||FMath::Abs(X*200.f)<350.f;
   Instance(Art.FRand()<.52f?"SM_StoneTile":"SM_StoneTile_B",C+FVector(X*200+Art.FRandRange(-4,4),Y*200+Art.FRandRange(-4,4),-24),FRotator(0,Art.RandRange(0,3)*90.f,0),FVector(.995,.995,1),WornPath?TEXT(""):TEXT("M_Stone"));
  }
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
   if(Door){
    Instance("SM_Arch",C+Out*1760,FRotator(0,Side*90.f+90,0),FVector(.82,1.1,1.3));
    // Side supports keep a >=354 cm central visual clearance. The upper open
    // arcade starts at Z480, beyond a normal running/jumping capsule envelope.
    for(int32 Flank:{-1,1})
     Garden(TEXT("SM_GardenButtress"),C+Out*1785+T*(342.f*Flank),FRotator(0,Side*90.f+90,0),FVector(1,1,1.08f));
    Garden(TEXT("SM_GardenButtress"),C+Out*1845+T*730,FRotator(0,Side*90.f+90,0),FVector(.94f,.96f,1.30f));
    Instance("SM_Arch",C+Out*1800+FVector(0,0,480),FRotator(0,Side*90.f+90,0),FVector(.90f,1.1f,.72f));
   }else{
    Garden(TEXT("SM_GardenButtress"),C+Out*1725+T*(Side%2?1080.f:-1020.f),FRotator(0,Side*90.f+90,0),FVector(1.08f,1.04f,1.12f+RoomIndex*.08f));
   }
   // A real 7–10 m rock silhouette behind the thin wall, with open space at
   // every connector. Width/depth bounds are recorded in the art metadata.
   for(int32 Bank=0;Bank<2;++Bank){
    const float Along=(Bank==0?-1060.f:1030.f)+(Door?0.f:GardenArt.FRandRange(-100.f,100.f));
    const float Height=.96f+RoomIndex*.14f+((Side+Bank)%3)*.12f;
    Garden(TEXT("SM_GardenCliffBank"),C+Out*2290+T*Along,FRotator(0,Side*90.f+90,0),FVector(1.03f,1.03f,Height));
   }
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
  Garden(TEXT("SM_GardenBough"),Tree+FVector(-70,-30,715),FRotator(0,-12+RoomIndex*37,0),FVector(1.20f,1.25f,1.10f));
  Garden(TEXT("SM_GardenBough"),Tree+FVector(70,30,790),FRotator(0,145+RoomIndex*31,0),FVector(1.02f,1.12f,.96f));
  Instance("SM_Bell",Tree+FVector(-20,-130,344),FRotator(0,-9,-6),FVector(RoomIndex==2?1.9:1.5));
  // A corner gallery creates height and shadow without blocking the exits.
  for(int32 K=0;K<2;++K){
   Instance("SM_Arch",C+FVector(-1130+K*470,1340,0),FRotator::ZeroRotator,FVector(.88,1,1.2));
   Instance("SM_Wall",C+FVector(-1130+K*470,1590,375),FRotator::ZeroRotator,FVector(1.2,1,.55));
  }
  Garden(TEXT("SM_GardenButtress"),C+FVector(-1490,1430,0),FRotator::ZeroRotator,FVector(1.0f,1.0f,1.12f));
  Garden(TEXT("SM_GardenButtress"),C+FVector(-440,1530,0),FRotator::ZeroRotator,FVector(.88f,1.0f,.95f));
  Garden(TEXT("SM_GardenBough"),C+FVector(-1490,1430,810),FRotator(0,-27-RoomIndex*23,0),FVector(1.25f,1.18f,1.14f));
  Garden(TEXT("SM_GardenBough"),C+FVector(1080,-1725,850),FRotator(0,137+RoomIndex*17,0),FVector(1.30f,1.14f,1.08f));
  if(RoomIndex==2)
   Garden(TEXT("SM_GardenBough"),C+FVector(-1950,-1620,850),FRotator(0,34,0),FVector(1.16f));
  Instance("SM_PillarBroken",C+FVector(1260,-1230,0),FRotator(0,17,0),FVector(1.1));
  Instance("SM_Arch",C+FVector(1440,1480,390),FRotator(0,30,0),FVector(1.1,1,1.4));
  for(int32 Corner=0;Corner<4;++Corner){
   const FRotator Around(0,Corner*90.f,0);
   const FVector Growth=C+Around.RotateVector(FVector(1390,1370,0));
   // Planted corners have a low stone mass and a broad leafy edge. No new
   // ground prop enters the central cross, ward/practice area or cover lanes.
   Garden(TEXT("SM_GardenRockCluster"),Growth,FRotator(0,Corner*90.f+23+RoomIndex*11,0),FVector(.88f,1.0f,.88f));
   for(int32 J=0;J<5;++J){
    const FVector Offset=Around.RotateVector(FVector(GardenArt.FRandRange(-155,120),GardenArt.FRandRange(-180,150),0));
    Garden(TEXT("SM_GardenUnderstory"),Growth+Offset,FRotator(0,GardenArt.FRandRange(0,360),0),FVector(GardenArt.FRandRange(.85f,1.15f)));
   }
   for(int32 J=0;J<3;++J){
    const FVector Offset=Around.RotateVector(FVector(-280+J*170,-250+J*60,0));
    Garden(TEXT("SM_GardenGrassDrift"),Growth+Offset,FRotator(0,Corner*90.f+18,0),FVector(.90f));
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
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,3000),FRotator(-31,-38,0));
 Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Sun->GetLightComponent()->SetIntensity(4.6f);Sun->GetLightComponent()->SetLightColor(FLinearColor(1,.86,.66));
 Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->bAtmosphereSunLight=true;
 Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->LightSourceAngle=1.2f;Sun->GetLightComponent()->ShadowSharpen=.25f;Generated.Add(Sun);
 Generated.Add(GetWorld()->SpawnActor<ASkyAtmosphere>());
 auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true);
 Sky->GetLightComponent()->SetIntensity(.58f);Sky->GetLightComponent()->RecaptureSky();Generated.Add(Sky);
 auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();Fog->GetComponent()->SetFogDensity(.018f);Fog->GetComponent()->SetFogHeightFalloff(.28f);
 Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.32,.39,.36));Generated.Add(Fog);
 auto* Post=GetWorld()->SpawnActor<APostProcessVolume>();Post->bUnbound=true;
 Post->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Post->Settings.AutoExposureApplyPhysicalCameraExposure=false;
 Post->Settings.bOverride_AutoExposureBias=true;Post->Settings.AutoExposureBias=-.10f;
 Post->Settings.bOverride_BloomIntensity=true;Post->Settings.BloomIntensity=.28f;
 Post->Settings.bOverride_VignetteIntensity=true;Post->Settings.VignetteIntensity=.16f;
 Post->Settings.bOverride_ColorSaturation=true;Post->Settings.ColorSaturation=FVector4(.94,.97,.92,1);
 Post->Settings.bOverride_AmbientOcclusionIntensity=true;Post->Settings.AmbientOcclusionIntensity=.8f;Generated.Add(Post);
 auto* Lamp=GetWorld()->SpawnActor<APointLight>(FVector(-1450,-760,220),FRotator::ZeroRotator);
 Lamp->GetLightComponent()->SetMobility(EComponentMobility::Movable);Lamp->GetLightComponent()->SetIntensity(2600);Lamp->GetLightComponent()->SetLightColor(FLinearColor(1,.61,.29));Cast<UPointLightComponent>(Lamp->GetLightComponent())->SetAttenuationRadius(850);Generated.Add(Lamp);
 bSliceAwaitingStart=!IsCleared(CurrentRoomId);UpdateGates();
}
