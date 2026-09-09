#include "DBGameMode.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"

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
 // Keep this import path local to the recovery scene. The original kit's
 // Instance helper intentionally resolves /Game/Art/Meshes, not nested kits.
 // A missing new import falls back to existing art, never an invisible wall.
 TMap<FString,UStaticMesh*> TrellisAssets;
 auto Trellis=[this,&TrellisAssets](const TCHAR* Name,FVector Location,FRotator Rotation,FVector Scale)->bool{
  const FString Key=FString(TEXT("Trellis/"))+Name;
  UStaticMesh** Cached=TrellisAssets.Find(Key);
  UStaticMesh* Asset=Cached?*Cached:nullptr;
  if(!Cached){
   const FString Path=FString::Printf(TEXT("/Game/Art/Trellis/Meshes/%s.%s"),Name,Name);
   Asset=LoadObject<UStaticMesh>(nullptr,*Path);TrellisAssets.Add(Key,Asset);
   if(Asset){
    UE_LOG(LogTemp,Display,TEXT("DB_TRELLIS %s bounds_cm=%s"),Name,*(Asset->GetBounds().BoxExtent*2).ToString());
   }else{
    UE_LOG(LogTemp,Warning,TEXT("DB_TRELLIS missing %s; using original scene art"),*Path);
   }
  }
  if(!Asset)return false;
  auto** Existing=MeshBatches.Find(Key);
  auto* Batch=Existing?*Existing:nullptr;
  if(!Batch){
   auto* BatchOwner=GetWorld()->SpawnActor<AActor>();if(!BatchOwner)return false;
   Generated.Add(BatchOwner);
   Batch=NewObject<UHierarchicalInstancedStaticMeshComponent>(BatchOwner);
   BatchOwner->SetRootComponent(Batch);BatchOwner->AddInstanceComponent(Batch);
   Batch->SetStaticMesh(Asset);Batch->SetMobility(EComponentMobility::Static);
   // Imported UCX bodies supply physical cover. Render triangles and foliage
   // must not become complex collision; that is part of the import contract.
   Batch->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
   Batch->SetCollisionObjectType(ECC_WorldStatic);Batch->SetCollisionResponseToAllChannels(ECR_Block);
   Batch->RegisterComponent();MeshBatches.Add(Key,Batch);
  }
  Batch->AddInstance(FTransform(Rotation,Location,Scale),true);
  return true;
 };
 // TRELLIS +Y is the visible front, with width along X and a grounded pivot.
 // Generated cloisters are optional side galleries, not required route gates:
 // their opening is narrower than the original protected doorway envelope.
 auto Cloister=[this,&Trellis](FVector Location,float Yaw,float Scale){
  if(!Trellis(TEXT("SM_Trellis_Cloister"),Location,FRotator(0,Yaw,0),FVector(Scale)))
   Instance(TEXT("SM_Arch"),Location,FRotator(0,Yaw,0),FVector(Scale,Scale,Scale*1.12f));
 };
 // Foliage remains noncolliding. Reachable stone masses use authored hulls.
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
   // Retain all three authored slots: dark recessed mortar, worn stone and
   // lighter individual slabs. One blanket M_Stone override erased that depth.
   // The existing flat collision surface and tile footprints are unchanged.
   Instance(Art.FRand()<.52f?"SM_StoneTile":"SM_StoneTile_B",C+FVector(X*200+Art.FRandRange(-4,4),Y*200+Art.FRandRange(-4,4),-24),FRotator(0,Art.RandRange(0,3)*90.f,0),FVector(.995,.995,1));
  }
  TArray<FVector> Doors;
  if(R.Parent>=0)Doors.Add((Rooms[R.Parent].Center-C).GetSafeNormal2D());
  if(RoomIndex+1<Rooms.Num())Doors.Add((Rooms[RoomIndex+1].Center-C).GetSafeNormal2D());
  // Large colliding backdrops cannot occupy the space between linked courts:
  // their far edge would enter the next arena. Keep two unconnected sides.
  TArray<int32> BackingSides;
  for(int32 Offset=0;Offset<4&&BackingSides.Num()<2;++Offset){
   const int32 Side=(RoomIndex+1+Offset)%4;
   const FVector Out=FRotator(0,Side*90.f,0).Vector();
   bool Connected=false;for(const FVector& D:Doors)if(FVector::DotProduct(D,Out)>.9f)Connected=true;
   if(!Connected)BackingSides.Add(Side);
  }
  for(int32 Side=0;Side<4;++Side){
   const FVector Out=FRotator(0,Side*90.f,0).Vector(),T(-Out.Y,Out.X,0);
   bool Door=false;for(const FVector& D:Doors)if(FVector::DotProduct(D,Out)>.9f)Door=true;
   for(int32 K=-4;K<=4;++K){
    if(Door&&K==0)continue;
    // Group the silhouette into low shoulders and taller ends, rather than
    // stretching every wall into an identical high strip around the arena.
    const bool TallShoulder=(Side+RoomIndex)%4==1&&K>=1;
    const float WallHeight=TallShoulder?1.46f:(FMath::Abs(K)>=3?1.16f:1.04f);
    Instance("SM_Wall",C+Out*1760+T*K*400,FRotator(0,Side*90.f+90,0),FVector(1,1,WallHeight));
    if(K==((Side+RoomIndex)%2?3:-3))
     Instance("SM_Ivy",C+Out*1715+T*K*400+FVector(0,0,260),FRotator(0,Side*90.f,0),FVector(1.1));
   }
   if(Door){
    Instance("SM_Arch",C+Out*1760,FRotator(0,Side*90.f+90,0),FVector(.82,1.1,1.3));
    // Retain the proven doorway, its collision and the connector axis.
    for(int32 Flank:{-1,1})
     Garden(TEXT("SM_GardenButtress"),C+Out*1785+T*(342.f*Flank),FRotator(0,Side*90.f+90,0),FVector(1,1,1.08f));
   }else{
    Garden(TEXT("SM_GardenButtress"),C+Out*1785+T*(Side%2?1120.f:-1060.f),FRotator(0,Side*90.f+90,0),FVector(.94f,1.0f,1.08f+RoomIndex*.06f));
   }
   // Distant backing belongs to two edges, leaving sky and light around the
   // bell crown. No continuous ring of repeated rock slabs above every wall.
   if(BackingSides.Contains(Side)){
    for(int32 Bank=0;Bank<2;++Bank){
     const float Along=(Bank==0?-1080.f:1080.f)+GardenArt.FRandRange(-60.f,60.f);
     const FRotator BackingRotation(0,Side*90.f+90,0);
     const float BackingScale=3.55f+RoomIndex*.15f+Bank*.30f;
     // Measured RootRock depth is 344.06 cm. Its nearest edge stays at
     // 1920 cm, beyond the 1850 cm foundation, at every uniform scale.
     const FVector Backing=C+Out*(1920.f+172.03f*BackingScale)+T*Along;
     if(!Trellis(TEXT("SM_Trellis_RootRock"),Backing,BackingRotation,FVector(BackingScale))){
      const float Height=.88f+RoomIndex*.09f+Bank*.12f;
      Garden(TEXT("SM_GardenCliffBank"),C+Out*2370+T*Along,BackingRotation,FVector(1.03f,1.03f,Height));
     }
    }
   }
  }
  // All variants leave +/-X and +/-Y door approaches clear. The two cover
  // banks move between three configurations, changing close and recall lanes.
  const FVector CoverA[]={FVector(-690,620,0),FVector(-810,800,0),FVector(-650,540,0)};
  const FVector CoverB[]={FVector(810,-860,0),FVector(650,-780,0),FVector(880,-1000,0)};
  Instance("SM_Wall",C+CoverA[Variant],FRotator(0,Variant==1?90:0,0),FVector(1.3,1,.43));
  Instance("SM_Wall",C+CoverB[Variant],FRotator(0,Variant==2?90:0,0),FVector(1.15,1,.46));
  Instance("SM_Rubble",C+CoverA[Variant]+FVector(250,-60,0),FRotator(0,32,0),FVector(1.2));
  // The root-embraced bell is one coherent textured silhouette, not a trunk,
  // separate suspended bell and several floating canopy pieces. Different
  // diagonal quarters keep the three courts identifiable while leaving the
  // unchanged enemy pads, practice area and axial approaches exposed.
  const FVector TreeOffsets[]={FVector(750+Variant*60,1000+Variant*70,0),
   FVector(900+Variant*60,1130-Variant*45,0),FVector(-1020-Variant*40,1230-Variant*45,0)};
  const FVector Tree=C+TreeOffsets[RoomIndex];
  const float TreeYaw=(C-Tree).Rotation().Yaw-90.f;
  const float TreeScale=RoomIndex==2?1.08f:RoomIndex==1?.98f:1.f;
  if(!Trellis(TEXT("SM_Trellis_BellTree"),Tree,FRotator(0,TreeYaw,0),FVector(TreeScale))){
   Instance("SM_BellTree",Tree,FRotator(0,TreeYaw+180,0),FVector(1.05f));
   Instance("SM_Bell",Tree+FRotator(0,TreeYaw+180,0).RotateVector(FVector(-20,-130,344)),FRotator(0,TreeYaw+180,0),FVector(1.5f));
  }

  // Grounded optional cloisters replace the floating upper arches. The first
  // court has a close right-hand frame visible from ordinary startup; it sits
  // south of the approach, so traversing its generated opening is optional.
  if(RoomIndex==0){
   Cloister(C+FVector(-1060,-1220,0),54.f,1.f);
   Cloister(C+FVector(-1040+Variant*45,1550,0),180.f,1.f);
  }else if(RoomIndex==1){
   Cloister(C+FVector(-1540,930+Variant*30,0),-90.f,1.f);
   Cloister(C+FVector(-1540,1460,0),-90.f,.92f);
  }else{
   Cloister(C+FVector(1090+Variant*35,-1550,0),0.f,1.08f);
  }
  for(int32 Corner=0;Corner<4;++Corner){
   // The wider paired cloister occupies this planted corner; leave its
   // opening and approach visible instead of filling them with a rock mass.
   if(RoomIndex==1&&Corner==1)continue;
   const FRotator Around(0,Corner*90.f,0);
   FVector Growth=C+Around.RotateVector(FVector(1420,1390,0));
   if(RoomIndex==2&&Corner==3)Growth=C+FVector(1500,-1200,0);
   const float RockScale=GardenArt.FRandRange(.9f,1.08f);
   const FRotator RockRotation(0,Corner*90.f+GardenArt.FRandRange(8.f,32.f),0);
   if(!Trellis(TEXT("SM_Trellis_RootRock"),Growth,RockRotation,FVector(RockScale)))
    Garden(TEXT("SM_GardenRockCluster"),Growth,RockRotation,FVector(.88f,1.0f,.88f));
   // New clusters already include roots, moss and ferns. Use fewer companion
   // plants, in unequal groups, with the same noncolliding foliage policy.
   for(int32 J=0;J<2+(Corner+RoomIndex)%2;++J){
    const FVector Offset=Around.RotateVector(FVector(GardenArt.FRandRange(-170,100),GardenArt.FRandRange(-170,100),0));
    Garden(TEXT("SM_GardenUnderstory"),Growth+Offset,FRotator(0,GardenArt.FRandRange(0,360),0),FVector(GardenArt.FRandRange(.8f,1.05f)));
   }
   for(int32 J=0;J<2;++J){
    const FVector Offset=Around.RotateVector(FVector(-210+J*180,-200+J*70,0));
    Garden(TEXT("SM_GardenGrassDrift"),Growth+Offset,FRotator(0,Corner*90.f+18,0),FVector(.82f));
   }
  }
  // A readable near-scale carved stone on the opposite side from the ward.
  // Face the accepted front toward the arrival, and keep the actual altar and
  // its interaction radius unchanged; this is scenery, not another reward.
  FVector Marker=C+FVector(-800,-1020,0);
  if(R.Parent>=0){
   const FVector Back=(Rooms[R.Parent].Center-C).GetSafeNormal2D();
   Marker=C+Back*1390-FVector(-Back.Y,Back.X,0)*620;
  }
  Trellis(TEXT("SM_Trellis_Waymarker"),Marker,FRotator(0,(GetRoomEntryPoint(RoomIndex)-Marker).Rotation().Yaw-90.f,0),FVector::OneVector);
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
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,3000),FRotator(-36,-42,0));
 Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Sun->GetLightComponent()->SetIntensity(5.2f);Sun->GetLightComponent()->SetLightColor(FLinearColor(1,.90,.73));
 Sun->GetLightComponent()->SetIndirectLightingIntensity(1.15f);
 Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->bAtmosphereSunLight=true;
 Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->LightSourceAngle=1.8f;Generated.Add(Sun);
 Generated.Add(GetWorld()->SpawnActor<ASkyAtmosphere>());
 auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true);
 Sky->GetLightComponent()->SetIntensity(.92f);Sky->GetLightComponent()->SetLightColor(FLinearColor(.72,.84,1));Sky->GetLightComponent()->RecaptureSky();Generated.Add(Sky);
 // Modest broad fill retains carved relief on the shadow side of bronze and
 // stone. One warm sun supplies the scene's shadows; this adds no second set.
 auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,2500),FRotator(-48,138,0));
 Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Fill->GetLightComponent()->SetIntensity(.60f);Fill->GetLightComponent()->SetLightColor(FLinearColor(.62,.76,1));
 Fill->GetLightComponent()->SetCastShadows(false);Generated.Add(Fill);
 auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();Fog->GetComponent()->SetFogDensity(.009f);Fog->GetComponent()->SetFogHeightFalloff(.24f);
 Fog->GetComponent()->SetStartDistance(900.f);Fog->GetComponent()->SetFogMaxOpacity(.42f);
 Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.31,.39,.46));Generated.Add(Fog);
 auto* Post=GetWorld()->SpawnActor<APostProcessVolume>();Post->bUnbound=true;
 Post->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Post->Settings.AutoExposureApplyPhysicalCameraExposure=false;
 Post->Settings.bOverride_AutoExposureBias=true;Post->Settings.AutoExposureBias=-.05f;
 Post->Settings.bOverride_BloomIntensity=true;Post->Settings.BloomIntensity=.18f;
 Post->Settings.bOverride_VignetteIntensity=true;Post->Settings.VignetteIntensity=.10f;
 Post->Settings.bOverride_AmbientOcclusionIntensity=true;Post->Settings.AmbientOcclusionIntensity=.75f;Generated.Add(Post);
 bSliceAwaitingStart=!IsCleared(CurrentRoomId);UpdateGates();
}
