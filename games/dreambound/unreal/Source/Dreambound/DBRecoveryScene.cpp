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

// One authored place is shared by three encounter chapters. Chapter indices
// retain the existing reward/journal semantics without regenerating the scenery.
void ADBGameMode::BuildRecoveryCourtyard()
{
 ResetActors();Rooms.Reset();SpawnedRooms.Reset();RoomWaves.Reset();Random.Initialize(Seed);
 const TCHAR* Names[]={TEXT("Bellroot courtyard"),TEXT("The resonant ward"),TEXT("Beneath the ancient bell")};
 for(int32 I=0;I<3;++I){FDBRoom R;R.Center=FVector::ZeroVector;R.Name=Names[I];R.Parent=I-1;Rooms.Add(R);}
 FRandomStream Art(77231); // authored composition is stable; encounters and rewards use the run seed.

 // Ground carries small broken flagstones, with a continuous foundation below
 // the decorative joints. No scaled tile towers masquerade as stairs.
 Mesh("/Engine/BasicShapes/Cube.Cube",FVector(0,0,-70),FRotator::ZeroRotator,FVector(37,31,1),true,"M_Mortar");
 for(int32 X=-8;X<=8;++X)for(int32 Y=-7;Y<=7;++Y){
  const float Jx=Art.FRandRange(-5,5),Jy=Art.FRandRange(-5,5);
  Instance(Art.FRand()<.52f?"SM_StoneTile":"SM_StoneTile_B",FVector(X*200+Jx,Y*200+Jy,-24+Art.FRandRange(-1.5,1.5)),FRotator(0,Art.RandRange(0,3)*90.f,0),FVector(.995,.995,1));
 }

 // Walls form a sheltered court rather than a horizon full of duplicated pillars.
 for(int32 X=-4;X<=4;++X){
  Instance("SM_Wall",FVector(X*400,1460,0),FRotator::ZeroRotator,FVector(1,1,1.25));
  Instance("SM_Wall",FVector(X*400,-1460,0),FRotator::ZeroRotator,FVector(1,1,1.05));
 }
 for(int32 Y=-3;Y<=3;++Y){
  Instance("SM_Wall",FVector(1760,Y*400,0),FRotator(0,90,0),FVector(1,1,1.65));
  Instance("SM_Wall",FVector(-1760,Y*400,0),FRotator(0,90,0),FVector(1,1,1.35));
 }
 // A nearer arch frames the first-person entrance and the bell-tree beyond it.
 Instance("SM_Arch",FVector(-1420,-650,0),FRotator(0,90,0),FVector(1.35,1.15,1.35));
 for(float Y:{-1135.f,-170.f})Instance("SM_Pillar",FVector(-1420,Y,0),FRotator::ZeroRotator,FVector(.95,.95,1.2));

 // Northern cloister: evenly supported architecture with a walkable arcade,
 // broken edges and shaded depth behind the arches.
 for(int32 I=0;I<4;++I){
  float X=-1050+I*610;
  Instance("SM_Arch",FVector(X,1060,0),FRotator::ZeroRotator,FVector(1.15,1,1.1));
  Instance("SM_Wall",FVector(X,1390,340),FRotator::ZeroRotator,FVector(1.5,1,.62));
  if(I!=1)Instance("SM_Ivy",FVector(X-140,1320,455),FRotator(0,I*63.f,0),FVector(1.35));
 }
 // Authored masonry treads rise toward the far gallery. The continuous
 // support is covered by the sculpted stair and irregular landing stones.
 Instance("SM_Stair",FVector(1020,-1040,0),FRotator(0,-90,0),FVector(2.8,1.8,1.6667));
 Mesh("/Engine/BasicShapes/Cube.Cube",FVector(1570,-1000,65),FRotator::ZeroRotator,FVector(3.8,8.5,1.9),true,"M_Stone");
 for(int32 X=0;X<2;++X)for(int32 Y=0;Y<4;++Y)Instance("SM_StoneTile_B",FVector(1470+X*175,-1300+Y*190,136),FRotator(0,90,0),FVector(.87,.95,1));
 Instance("SM_Arch",FVector(1610,-700,160),FRotator(0,90,0),FVector(.9,1,1.18));

 // Landmark silhouette and its canopy are authored meshes. Roots visibly grow
 // from this trunk; they are not stretched strips placed across the skyline.
 Instance("SM_BellTree",FVector(360,260,0),FRotator(0,-12,0),FVector(1.35));
 Instance("SM_Canopy",FVector(360,260,837),FRotator(0,-12,0),FVector(1.5));
 Instance("SM_Bell",FVector(350,110,386),FRotator(0,-9,-6),FVector(1.7));
 // Upper ruins sit behind enclosing walls to add depth and silhouette.
 Instance("SM_Arch",FVector(1310,1680,350),FRotator(0,-7,0),FVector(1.5,1.2,1.45));
 Instance("SM_Wall",FVector(1530,1890,0),FRotator::ZeroRotator,FVector(2.5,1.6,2.6));
 Instance("SM_Pillar",FVector(900,1830,0),FRotator(0,0,-2),FVector(1.3,1.3,2.7));
 Instance("SM_Pillar",FVector(2070,1830,0),FRotator(0,0,2),FVector(1.3,1.3,2.4));
 Instance("SM_BellTree",FVector(-1850,1900,-20),FRotator(0,134,0),FVector(1.8));
 Instance("SM_Canopy",FVector(-1850,1900,1100),FRotator(0,134,0),FVector(2));

 // Low cover creates two readable return lanes around the central tree.
 Instance("SM_Wall",FVector(-120,-850,0),FRotator(0,-14,0),FVector(1.05,1,.43));
 Instance("SM_Rubble",FVector(160,-860,0),FRotator(0,32,0),FVector(1.3));
 Instance("SM_Wall",FVector(1280,20,0),FRotator(0,77,0),FVector(.65,1,.48));
 Instance("SM_PillarBroken",FVector(-780,1040,0),FRotator(0,17,0),FVector(1.1));
 Instance("SM_Ivy",FVector(-1630,-1210,275),FRotator(0,90,0),FVector(1.4));
 const FVector Growth[]={FVector(-1550,1050,0),FVector(-950,1210,0),FVector(-420,1210,0),FVector(660,970,0),FVector(1350,1150,0),FVector(1570,750,0),FVector(980,310,0),FVector(420,630,0),FVector(0,430,0),FVector(-1550,-1230,0),FVector(-1150,-1270,0),FVector(-300,-1230,0),FVector(-1450,-150,0),FVector(-1240,490,0),FVector(-520,-830,0),FVector(270,-170,0),FVector(720,-200,0),FVector(1460,-330,0)};
 for(int32 I=0;I<UE_ARRAY_COUNT(Growth);++I){
  Instance("SM_Fern",Growth[I],FRotator(0,Art.FRandRange(0,360),0),FVector(Art.FRandRange(.8,1.25)));
  if(I%2==0)Instance("SM_Rubble",Growth[I]+FVector(130,85,-5),FRotator(0,Art.FRandRange(0,360),0),FVector(Art.FRandRange(.65,1.15)));
  for(int32 J=0;J<6;++J)Instance("SM_Grass",Growth[I]+FVector(Art.FRandRange(-120,120),Art.FRandRange(-100,100),0),FRotator(0,Art.FRandRange(0,360),0),FVector(Art.FRandRange(.8,1.2)));
 }

 // One conspicuous ward stone serves all phases; the glow is quiet until a
 // decision is available. Current stage chooses which shared-position actor shows.
 Instance("SM_Rubble",FVector(-1120,-430,0),FRotator(0,24,0),FVector(1.05,1.05,.55));
 for(int32 I=0;I<Rooms.Num();++I)Rooms[I].Altar=Mesh("SM_Crystal",FVector(-1120,-430,76),FRotator::ZeroRotator,FVector(1.4),false,"M_Crystal");

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
