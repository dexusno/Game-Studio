#include "DBGameMode.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PointLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/BoxComponent.h"
#include "Components/AudioComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/ExponentialHeightFogComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"

// Route topology comes first. The three encounter centers and flat combat pads
// remain compatible with the game; masonry, soil and water follow that graph.
// Reverie architecture is dressed with original LivingWorld habitat props.
// Hidden boxes only supply soil floors
// and the earned progression barriers; missing imports never show old assets.
void ADBGameMode::BuildRecoveryCourtyard()
{
 ResetActors();Rooms.Reset();SpawnedRooms.Reset();RoomWaves.Reset();RoomLayoutVariants.Reset();Random.Initialize(Seed);
 FRandomStream Layout(Seed);
 const FVector First=Layout.RandRange(0,1)?FVector(1,0,0):FVector(0,1,0);
 const int32 Turn=Layout.RandRange(-1,1);
 const FVector Second=FRotator(0,Turn*90.f,0).RotateVector(First);
 const FVector Centers[]={FVector::ZeroVector,First*4200,First*4200+Second*4200};
 const TCHAR* Names[]={TEXT("The spring cloister"),TEXT("The moss terraces"),TEXT("The crown sanctuary")};
 LayoutSignature=FString::Printf(TEXT("reverie-route:%d,%d"),First.X>.5f?0:1,Turn);
 for(int32 I=0;I<3;++I){
  FDBRoom Room;Room.Center=Centers[I];Room.Name=Names[I];Room.Parent=I-1;Rooms.Add(Room);
  const int32 Variant=Layout.RandRange(0,2);RoomLayoutVariants.Add(I,Variant);
  LayoutSignature+=FString::Printf(TEXT("/garden%d:%d"),I,Variant);
 }
 TMap<FString,UStaticMesh*> Assets;
 TMap<FString,UMaterialInterface*> Materials;
 auto Asset=[&Assets](const TCHAR* Name)->UStaticMesh*{
  if(UStaticMesh** Existing=Assets.Find(Name))return *Existing;
  const TCHAR* Family=FString(Name).StartsWith(TEXT("SM_LW_"))?TEXT("LivingWorld"):TEXT("Reverie");
  const FString Path=FString::Printf(TEXT("/Game/Art/%s/Meshes/%s.%s"),Family,Name,Name);
  UStaticMesh* Result=LoadObject<UStaticMesh>(nullptr,*Path);Assets.Add(Name,Result);
  if(!Result)UE_LOG(LogTemp,Error,TEXT("DB_REVERIE missing required mesh %s"),*Path);
  return Result;
 };
 auto Material=[&Materials](const TCHAR* Name)->UMaterialInterface*{
  if(UMaterialInterface** Existing=Materials.Find(Name))return *Existing;
  const FString Path=FString::Printf(TEXT("/Game/Art/Reverie/Materials/%s.%s"),Name,Name);
  UMaterialInterface* Result=LoadObject<UMaterialInterface>(nullptr,*Path);Materials.Add(Name,Result);
  if(!Result)UE_LOG(LogTemp,Error,TEXT("DB_REVERIE missing required material %s"),*Path);
  return Result;
 };
 auto Place=[this,&Asset,&Material](const TCHAR* Name,FVector Location,FRotator Rotation=FRotator::ZeroRotator,
  FVector Scale=FVector::OneVector,bool Collision=true,const TCHAR* Override=nullptr)->bool{
  UStaticMesh* Model=Asset(Name);if(!Model)return false;
  if(FString(Name)==TEXT("SM_RV_Wall")||FString(Name)==TEXT("SM_RV_WallEnd")||FString(Name)==TEXT("SM_RV_Buttress")){
   // Soil sits22cm below paving and its outer shoulder descends another80cm.
   // Embed masonry through both levels, preserving its existing rendered top
   // (and every coping/crest attachment) using the imported mesh's own height.
   const float FootingDepth=112.f;
   const float ModelTop=Model->GetBounds().Origin.Z+Model->GetBounds().BoxExtent.Z;
   if(ModelTop>1.f){Location.Z-=FootingDepth;Scale.Z+=FootingDepth/ModelTop;}
  }
  const FString Key=FString::Printf(TEXT("Reverie/%s/%d/%s"),Name,Collision?1:0,Override?Override:TEXT("authored"));
  UHierarchicalInstancedStaticMeshComponent** Existing=MeshBatches.Find(Key);
  auto* Batch=Existing?*Existing:nullptr;
  if(!Batch){
   AActor* Owner=GetWorld()->SpawnActor<AActor>();if(!Owner)return false;
   Generated.Add(Owner);Owner->Tags.Add(TEXT("DBReverieArt"));
   if(FString(Name).StartsWith(TEXT("SM_LW_")))Owner->Tags.Add(TEXT("DBLivingWorldArt"));
   Batch=NewObject<UHierarchicalInstancedStaticMeshComponent>(Owner);
   Owner->SetRootComponent(Batch);Owner->AddInstanceComponent(Batch);
   Batch->SetStaticMesh(Model);Batch->SetMobility(EComponentMobility::Static);
   Batch->SetCollisionEnabled(Collision?ECollisionEnabled::QueryAndPhysics:ECollisionEnabled::NoCollision);
   Batch->SetCollisionObjectType(ECC_WorldStatic);Batch->SetCollisionResponseToAllChannels(ECR_Block);
   if(Override)for(int32 Slot=0;Slot<Model->GetStaticMaterials().Num();++Slot)Batch->SetMaterial(Slot,Material(Override));
   if(FString(Name).Contains(TEXT("Water"))||FString(Name).Contains(TEXT("Sky")))Batch->SetCastShadow(false);
   if(FString(Name).StartsWith(TEXT("SM_LW_")))Batch->SetCullDistances(4800,6500);
   if(FString(Name)==TEXT("SM_LW_Dragonfly")||FString(Name)==TEXT("SM_LW_GardenMoth"))Batch->SetCastShadow(false);
   Batch->RegisterComponent();MeshBatches.Add(Key,Batch);
  }
  Batch->AddInstance(FTransform(Rotation,Location,Scale),true);return true;
 };
 // A buried dressed-stone base joins an authored asset to the sloping soil.
 // Wall's solid top is exactly600cm; Place extends its bottom another112cm
 // without moving this requested bearing surface or any playable top above it.
 auto StoneBedding=[&Place](FVector Surface,FVector2D Footprint){
  Place(TEXT("SM_RV_Wall"),Surface-FVector(0,0,32),FRotator::ZeroRotator,
   FVector(Footprint.X/400.f,Footprint.Y/94.f,32.f/600.f),true,TEXT("M_RV_StoneDark"));
 };
 // Altars and gates remain individual actors: progression hides/opens them.
 auto MovingMesh=[this,&Asset,&Material](const TCHAR* Name,FVector Location,FRotator Rotation,
  FVector Scale,const TCHAR* Override=nullptr)->AStaticMeshActor*{
  UStaticMesh* Model=Asset(Name);if(!Model)return nullptr;
  auto* Actor=GetWorld()->SpawnActor<AStaticMeshActor>(Location,Rotation);if(!Actor)return nullptr;
  Generated.Add(Actor);Actor->Tags.Add(TEXT("DBReverieArt"));
  auto* Component=Actor->GetStaticMeshComponent();Component->SetMobility(EComponentMobility::Movable);
  Component->SetStaticMesh(Model);Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
  Actor->SetActorScale3D(Scale);
  if(Override)for(int32 Slot=0;Slot<Model->GetStaticMaterials().Num();++Slot)Component->SetMaterial(Slot,Material(Override));
  return Actor;
 };
 auto Ground=[this](FVector Location,FVector Extents,float Yaw){
  AActor* Actor=GetWorld()->SpawnActor<AActor>();Generated.Add(Actor);Actor->Tags.Add(TEXT("DBReverieGround"));
  auto* Box=NewObject<UBoxComponent>(Actor);Actor->SetRootComponent(Box);Actor->AddInstanceComponent(Box);
  Box->SetBoxExtent(Extents);Box->SetWorldLocationAndRotation(Location,FRotator(0,Yaw,0));
  Box->SetMobility(EComponentMobility::Static);
  Box->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);Box->SetCollisionObjectType(ECC_WorldStatic);
  Box->SetCollisionResponseToAllChannels(ECR_Block);Box->SetHiddenInGame(true);Box->SetVisibility(false);
  Box->RegisterComponent();
 };
 auto DistanceToSegment=[](FVector Point,FVector Start,FVector End){
  const FVector Delta=End-Start;
  const float Along=FMath::Clamp(FVector::DotProduct(Point-Start,Delta)/FMath::Max(Delta.SizeSquared2D(),1.0),0.0,1.0);
  return FVector::Dist2D(Point,Start+Along*Delta);
 };
 for(int32 RoomIndex=0;RoomIndex<Rooms.Num();++RoomIndex){
  auto& R=Rooms[RoomIndex];const FVector C=R.Center;const int32 Variant=RoomLayoutVariants[RoomIndex];
  FRandomStream Art(Seed+199903+RoomIndex*8209);
  TArray<FVector> Doors;
  if(R.Parent>=0)Doors.Add((Rooms[R.Parent].Center-C).GetSafeNormal2D());
  if(RoomIndex+1<Rooms.Num())Doors.Add((Rooms[RoomIndex+1].Center-C).GetSafeNormal2D());
  FVector WardOffset(-1120,-430,76);
  if(R.Parent>=0){const FVector Back=(Rooms[R.Parent].Center-C).GetSafeNormal2D();WardOffset=Back*1120+FVector(-Back.Y,Back.X,0)*300+FVector(0,0,76);}
  const FVector Entry=GetRoomEntryPoint(RoomIndex)-C;
  auto Reserved=[&Doors,&WardOffset,&Entry](FVector P){
   if(P.SizeSquared2D()<FMath::Square(800.f)||FVector::Dist2D(P,WardOffset)<290.f||FVector::Dist2D(P,Entry)<300.f)return true;
   for(const FVector& D:Doors){const FVector T(-D.Y,D.X,0);if(FVector::DotProduct(P,D)>0&&FMath::Abs(FVector::DotProduct(P,T))<360.f)return true;}
   for(const FVector& Pad:{FVector(420,-580,0),FVector(1260,660,0),FVector(-340,860,0),FVector(-500,-500,0),FVector(-500,-80,0),FVector(700,-650,0)})
    if(FVector::Dist2D(P,Pad)<270.f)return true;
   return false;
  };
  // An octagonal continuous soil bed 22cm below the paving, with a visible
  // sculpted earth shoulder descending into the creek between sanctuaries.
  Ground(C+FVector(0,0,-42),FVector(1850,1100,20),0);
  Ground(C+FVector(0,0,-42),FVector(1100,1850,20),0);
  for(int32 Corner=0;Corner<4;++Corner){
   const FRotator Around(0,Corner*90.f,0);
   Ground(C+Around.RotateVector(FVector(1312,1312,-42)),FVector(530,230,20),135.f+Corner*90.f);
  }
  Place(TEXT("SM_RV_TerrainPatch"),C+FVector(0,0,-22),FRotator(0,Variant*90.f,0),FVector(.86,.86,1),false,TEXT("M_RV_Soil"));
  // Broken-bond paving follows a central gathering space and chosen routes.
  // Soil remains visible around grouped planting instead of a giant grid.
  const float GatheringRadius=800.f+Variant*30.f;
  TArray<FVector> PavingCenters;
  for(int32 Row=-8;Row<=8;++Row)for(int32 Column=-9;Column<=9;++Column){
   const FVector P(Column*200.f+(Row%2?100.f:0.f),Row*200.f,0);
   if(FMath::Abs(P.X)>1740.f||FMath::Abs(P.Y)>1740.f||FMath::Abs(P.X)+FMath::Abs(P.Y)>2710.f)continue;
   const float Radius=P.Size2D();bool Paved=Radius<GatheringRadius;
   for(const FVector& D:Doors){const FVector T(-D.Y,D.X,0);Paved|=FVector::DotProduct(P,D)>0&&FMath::Abs(FVector::DotProduct(P,T))<330.f;}
   Paved|=DistanceToSegment(P,Entry,FVector::ZeroVector)<280.f;
   Paved|=DistanceToSegment(P,WardOffset,FVector::ZeroVector)<290.f;
   for(const FVector& Pad:{FVector(420,-580,0),FVector(1260,660,0),FVector(-340,860,0),
    FVector(-500,-500,0),FVector(-500,-80,0),FVector(700,-650,0)})Paved|=FVector::Dist2D(P,Pad)<290.f;
   if(!Paved)continue;
   const bool Accent=(Radius>GatheringRadius-170.f&&Radius<GatheringRadius)||((Column+Row+Variant)%5==0);
   Place(Accent?TEXT("SM_RV_TileB"):TEXT("SM_RV_Tile"),C+P+FVector(0,0,-20),FRotator(0,Art.RandRange(0,3)*90.f,0));
   PavingCenters.Add(P);
  }
  for(const FVector& D:Doors){const FVector T(-D.Y,D.X,0);
   for(int32 Across=-1;Across<=1;++Across){
    const FVector P=D*1760.f+T*Across*200.f;
    Place(TEXT("SM_RV_TileB"),C+P+FVector(0,0,-20),FRotator(0,D.Rotation().Yaw,0));PavingCenters.Add(P);
   }
  }
  // The20cm paving cap used to stop above the soil, most visibly where the
  // shoulder falls away at bridge approaches. Support exposed slab edges;
  // interior slabs retain their existing top and do not need extra scenery.
  for(const FVector& P:PavingCenters){
   bool Exposed=false;
   for(const FVector& Offset:{FVector(101,75,0),FVector(101,-75,0),FVector(-101,75,0),FVector(-101,-75,0),
     FVector(75,101,0),FVector(-75,101,0),FVector(75,-101,0),FVector(-75,-101,0)}){
    const FVector Probe=P+Offset;bool Covered=false;
    for(const FVector& Neighbor:PavingCenters)
     if(FMath::Abs(Probe.X-Neighbor.X)<=100.5f&&FMath::Abs(Probe.Y-Neighbor.Y)<=100.5f){Covered=true;break;}
    if(!Covered){Exposed=true;break;}
   }
   // Slightly inset below the cap, with2cm overlap into its solid bedding.
   if(Exposed)StoneBedding(C+P+FVector(0,0,-18),FVector2D(194,186));
  }
  // Small unequal drifts soften exposed paving edges. Their candidates come
  // from actual slab boundaries, not a uniform lawn scatter, and retain a
  // foliage-width margin around clear routes, entry/ward and practice pads.
  FRandomStream EdgeGrowth(Seed+88711+RoomIndex*653);
  auto SlabDistance=[&PavingCenters](FVector P){
   float Nearest=10000.f;
   for(const FVector& Slab:PavingCenters){
    const float DX=FMath::Max(FMath::Abs(P.X-Slab.X)-100.0,0.0);
    const float DY=FMath::Max(FMath::Abs(P.Y-Slab.Y)-100.0,0.0);
    Nearest=FMath::Min(Nearest,FMath::Sqrt(DX*DX+DY*DY));
   }
   return Nearest;
  };
  auto ClearGrowth=[&Reserved,RoomIndex](FVector P){
   // Terrain's central authored deck is level here; outer shoulders descend.
   if(FMath::Abs(P.X)>1350.f||FMath::Abs(P.Y)>1350.f)return false;
   if(P.X>300.f&&P.Y>975.f)return false; // spring, rill and basin garden
   if(RoomIndex>0&&P.X<-210.f&&P.Y<-890.f)return false; // side stair/terrace
   for(const FVector& Margin:{FVector::ZeroVector,FVector(85,0,0),FVector(-85,0,0),FVector(0,85,0),FVector(0,-85,0)})
    if(Reserved(P+Margin))return false;
   return true;
  };
  TArray<FVector> Drifts;
  for(int32 Attempt=0;Attempt<160&&Drifts.Num()<9;++Attempt){
   const FVector Slab=PavingCenters[EdgeGrowth.RandRange(0,PavingCenters.Num()-1)];
   const FVector Out=FRotator(0,EdgeGrowth.RandRange(0,3)*90.f,0).Vector(),Along(-Out.Y,Out.X,0);
   const FVector Drift=Slab+Out*EdgeGrowth.FRandRange(165,235)+Along*EdgeGrowth.FRandRange(-65,65);
   if(!ClearGrowth(Drift)||SlabDistance(Drift)<35.f||SlabDistance(Drift)>160.f)continue;
   bool NearDrift=false;for(const FVector& Existing:Drifts)NearDrift|=FVector::DistSquared2D(Drift,Existing)<FMath::Square(240.f);
   if(NearDrift)continue;
   Drifts.Add(Drift);
   const int32 Count=EdgeGrowth.RandRange(3,5);
   for(int32 Plant=0;Plant<Count;++Plant){
    const FVector Growth=Drift+FVector(EdgeGrowth.FRandRange(-70,70),EdgeGrowth.FRandRange(-70,70),0);
    if(!ClearGrowth(Growth)||SlabDistance(Growth)<25.f)continue;
    const float Size=EdgeGrowth.FRandRange(.52f,.85f);
    Place(Plant==0?TEXT("SM_RV_Fern"):TEXT("SM_RV_Grass"),C+Growth+FVector(0,0,-21),
     FRotator(0,EdgeGrowth.FRandRange(0,360),0),FVector(Size,Size,Size*.9f),false);
   }
  }
  // Join nearby clumps into short, uneven ribbons of planting. Rejecting
  // paved/Reserved samples leaves deliberate breaks at every playable route.
  auto PlantRibbon=[&Place,&ClearGrowth,&SlabDistance,&EdgeGrowth,&C](FVector Start,FVector End){
   const FVector Side=FRotator(0,90,0).RotateVector((End-Start).GetSafeNormal2D());
   const int32 Steps=FMath::Max(1,FMath::CeilToInt(FVector::Dist2D(Start,End)/85.f));
   for(int32 Step=0;Step<=Steps;++Step){
    const FVector Center=FMath::Lerp(Start,End,Step/float(Steps));
    for(int32 Row=0;Row<2;++Row){
     const FVector Growth=Center+Side*((Row?35.f:-35.f)+EdgeGrowth.FRandRange(-24,24));
     const float EdgeDistance=SlabDistance(Growth);
     if(!ClearGrowth(Growth)||EdgeDistance<30.f||EdgeDistance>230.f)continue;
     const float Size=EdgeGrowth.FRandRange(.53f,.79f);
     Place((Step+Row)%6==0?TEXT("SM_RV_Fern"):TEXT("SM_RV_Grass"),C+Growth+FVector(0,0,-21),
      FRotator(0,EdgeGrowth.FRandRange(0,360),0),FVector(Size,Size,Size*.82f),false);
    }
   }
  };
  int32 Ribbons=0;
  for(int32 I=0;I<Drifts.Num()&&Ribbons<4;++I){
   int32 Nearest=INDEX_NONE;float Distance=610.f;
   for(int32 J=I+1;J<Drifts.Num();++J){const float D=FVector::Dist2D(Drifts[I],Drifts[J]);if(D<Distance){Distance=D;Nearest=J;}}
   if(Nearest!=INDEX_NONE){PlantRibbon(Drifts[I],Drifts[Nearest]);++Ribbons;}
  }
  if(RoomIndex==0){
   const FVector EntryGround(Entry.X,Entry.Y,0);
   const FVector Inward=(-EntryGround).GetSafeNormal2D(),Left(Inward.Y,-Inward.X,0);
   PlantRibbon(EntryGround+Inward*250.f+Left*390.f,EntryGround+Inward*1050.f+Left*390.f);
  }
  // Only actual graph edges receive arches. Their matching openings face
  // one another across a bridge; no arch is backed by an impassable panel.
  for(int32 Side=0;Side<4;++Side){
   const FVector Out=FRotator(0,Side*90.f,0).Vector(),T(-Out.Y,Out.X,0);
   const float Facing=Side*90.f-90.f;
   bool Connected=false;for(const FVector& D:Doors)Connected|=FVector::DotProduct(D,Out)>.9f;
   const bool HighSide=((Side+RoomIndex)%4==0)||(!Connected&&(Side+Variant)%2==0);
   const float Height=HighSide?1.32f:(.82f+.08f*((Side+Variant)%3));
   if(Connected){
    Place(TEXT("SM_RV_Arch"),C+Out*1800.f,FRotator(0,Facing,0));
    // Only the piers receive a below-floor footing. The protected opening
    // and the bridge threshold retain their exact ground0 clearance.
    for(int32 Sign:{-1,1})Place(TEXT("SM_RV_Wall"),C+Out*1800.f+T*Sign*294.f+FVector(0,0,-30),
     FRotator(0,Facing,0),FVector(.37,1.6,.05));
    for(int32 Sign:{-1,1})for(int32 Bay=0;Bay<2;++Bay){
     const FVector Wall=C+Out*1800.f+T*Sign*(560.f+Bay*400.f);
     Place(TEXT("SM_RV_Wall"),Wall,FRotator(0,Facing,0),FVector(1,1,Height));
     Place(TEXT("SM_RV_Coping"),Wall+FVector(0,0,600.f*Height),FRotator(0,Facing,0),FVector::OneVector,false);
    }
   }else{
    for(int32 Bay=-2;Bay<=2;++Bay){
     const FVector Wall=C+Out*1800.f+T*Bay*400.f;
     const float BayHeight=Height+((Bay==2&&HighSide)?.15f:0.f);
     Place(TEXT("SM_RV_Wall"),Wall,FRotator(0,Facing,0),FVector(1,1,BayHeight));
     Place(TEXT("SM_RV_Coping"),Wall+FVector(0,0,600.f*BayHeight),FRotator(0,Facing,0),FVector::OneVector,false);
    }
   }
   for(int32 Sign:{-1,1}){
    Place(TEXT("SM_RV_Buttress"),C+Out*1780.f+T*Sign*1020.f,FRotator(0,Facing,0),FVector(.80,.90,Height));
    Place(TEXT("SM_RV_Ivy"),C+Out*1710.f+T*Sign*850.f+FVector(0,0,510.f*Height),FRotator(0,Facing,0),FVector(1.15,1.15,1.30),false);
   }
   if(!Connected){
    Place(TEXT("SM_RV_RuinCrown"),C+Out*1800.f+T*(Variant-1)*350.f+FVector(0,0,600.f*Height),FRotator(0,Facing,0),FVector(1.1,1.1,.8f+.1f*RoomIndex),false);
    // New sculpted masses form unequal peaks with overlapping low shoulders.
    // The source is 540x521.62cm at its rooted base; use the full rotated
    // envelope rather than the narrower footprint of the retired cliff kit.
    for(int32 Group=0;Group<2;++Group){
     const float AlongBase=(Group?940.f:-900.f)+Art.FRandRange(-150,150);
     const float PeakScale=Art.FRandRange(2.9f,4.0f)+(RoomIndex==2?.20f:0.f);
     for(int32 Layer=0;Layer<2;++Layer){
      const float Size=PeakScale*(Layer==0?1.f:.61f);
      const FRotator Rotation(0,Facing+(Group?12.f:-18.f)+Layer*43.f+Art.FRandRange(-11,11),0);
      const FVector AxisX=Rotation.RotateVector(FVector(1,0,0)),AxisY=Rotation.RotateVector(FVector(0,1,0));
      const float HalfOut=(FMath::Abs(FVector::DotProduct(AxisX,Out))*270.f
       +FMath::Abs(FVector::DotProduct(AxisY,Out))*260.81f)*Size;
      const float HalfX=(FMath::Abs(AxisX.X)*270.f+FMath::Abs(AxisY.X)*260.81f)*Size;
      const float HalfY=(FMath::Abs(AxisX.Y)*270.f+FMath::Abs(AxisY.Y)*260.81f)*Size;
      const float Along=AlongBase+(Layer?(Group?390.f:-310.f):0.f);
      const FVector Bank=C+Out*((Layer?1870.f:2080.f)+HalfOut)+T*Along+FVector(0,0,Layer?-115.f:-210.f);
      bool NearOther=false;
      for(int32 Other=0;Other<Rooms.Num();++Other)if(Other!=RoomIndex){
       const FVector Delta=Bank-Rooms[Other].Center;
       NearOther|=FMath::Abs(Delta.X)<1950.f+HalfX&&FMath::Abs(Delta.Y)<1950.f+HalfY;
      }
      if(!NearOther)Place(TEXT("SM_RV_SculptedBank"),Bank,Rotation,FVector(Size),false);
     }
    }
   }
  }
  // Chamfered corners join wall bays. Stepped crests provide a varied ruin
  // silhouette while retaining a closed physical boundary at player height.
  for(int32 Corner=0;Corner<4;++Corner){
   const FRotator Around(0,Corner*90.f,0);
   const FVector Start=Around.RotateVector(FVector(1800,1100,0)),End=Around.RotateVector(FVector(1100,1800,0));
   const float Yaw=(End-Start).Rotation().Yaw;
   for(int32 Bay=0;Bay<3;++Bay){
    const float Height=.83f+.14f*((Corner+RoomIndex+Variant)%3);
    const FVector Wall=C+FMath::Lerp(Start,End,(Bay+.5f)/3.f);
    Place(TEXT("SM_RV_Wall"),Wall,FRotator(0,Yaw,0),FVector(.87,1,Height));
    Place(TEXT("SM_RV_Coping"),Wall+FVector(0,0,600*Height),FRotator(0,Yaw,0),FVector(.87,1,1),false);
   }
  }
  // Grouped banks: rock cover, roots and understory share an origin. Retained
  // encounter/practice pads and door approaches stay free of dense planting.
  const FVector Banks[]={FVector(-1160,1050,0),FVector(1070,-1130,0)};
  // CrownTree's measured lower roots span570.39x558.71cm. Move its beds
  // inward and use cardinal facing so that this base never expands into
  // a door lane, retained practice pad or chamfered corner wall.
  const FVector Trees[]={FVector(-1120-Variant*15,1080,0),FVector(1080,-1100+Variant*15,0)};
  for(int32 Bank=0;Bank<2;++Bank){
   const FVector P=Banks[Bank]+FVector((Variant-1)*(Bank?55.f:-40.f),0,0);
   // Uniform scale preserves the generated root/stone proportions. The
   // deeper base remains within the old cover envelope after rotation.
   Place(TEXT("SM_RV_SculptedBank"),C+P+FVector(0,0,-48),FRotator(0,Bank?28.f:-20.f,0),FVector(Bank?.65f:.62f));
   Place(TEXT("SM_RV_CrownTree"),C+Trees[Bank]+FVector(0,0,-22),FRotator(0,Bank?90.f:180.f,0),FVector::OneVector);
   Place(TEXT("SM_RV_Rubble"),C+P+FVector(Bank?-250.f:260.f,90,-18),FRotator(0,Bank*90.f+35,0),FVector(.65),false);
   const float BedFront=(-P).Rotation().Yaw;
   for(int32 Plant=0;Plant<21;++Plant){
    const float Angle=BedFront-72.f+Plant*7.2f+Art.FRandRange(-3,3);
    const FVector Growth=P+FRotator(0,Angle,0).Vector()*Art.FRandRange(325,405);
    if(!ClearGrowth(Growth)||SlabDistance(Growth)<25.f)continue;
    const TCHAR* Type=Plant%7==0?TEXT("SM_RV_Flowers"):Plant%4==0?TEXT("SM_RV_Fern"):TEXT("SM_RV_Grass");
    const float Size=Art.FRandRange(.61f,.94f);
    Place(Type,C+Growth+FVector(0,0,-21),FRotator(0,Art.FRandRange(0,360),0),FVector(Size,Size,Size*.9f),false);
   }
  }
  // Raised spring -> rill -> receiving basin is one coherent water feature.
  // It stays beyond the combat pads and every possible processional axis.
  const FVector Basin=C+FVector(1130,1240,0);
  // Reuse the hollow radial masonry as an embedded foundation: the lower
  // rim's physical top meetsZ0, while its coping overlaps the upper rim.
  // The water volume remains open and its original35cm surface is unchanged.
  Place(TEXT("SM_RV_Basin"),Basin+FVector(0,0,-114),FRotator(0,Variant*30.f,0),FVector(1.06,1.06,1.5));
  Place(TEXT("SM_RV_Basin"),Basin,FRotator(0,Variant*30.f,0));
  Place(TEXT("SM_RV_WaterDisc"),Basin+FVector(0,0,35),FRotator::ZeroRotator,FVector(4.85,4.85,1),false,TEXT("M_RV_Water"));
  const FRotator FountainFacing(0,135.f+Variant*12.f,0);
  Place(TEXT("SM_RV_FountainHero"),Basin,FountainFacing);
  // Soft reflected warmth reveals the carved front and bronze bowls without
  // changing their PBR material or introducing another shadow-casting light.
  auto* FountainFill=GetWorld()->SpawnActor<APointLight>(Basin+FountainFacing.RotateVector(FVector(-70,330,350)),FRotator::ZeroRotator);
  auto* FountainLight=Cast<UPointLightComponent>(FountainFill->GetLightComponent());
  FountainLight->SetMobility(EComponentMobility::Movable);FountainLight->SetIntensity(2200.f);
  FountainLight->SetLightColor(FLinearColor(1.f,.83f,.64f));FountainLight->SetAttenuationRadius(900.f);
  FountainLight->SetSourceRadius(110.f);FountainLight->SetSoftSourceRadius(160.f);
  FountainLight->SetCastShadows(false);Generated.Add(FountainFill);
  Place(TEXT("SM_RV_Waterfall"),Basin+FountainFacing.RotateVector(FVector(-20,90,160)),FountainFacing,
   FVector(.50,1,1.90),false,TEXT("M_RV_Waterfall"));
  Place(TEXT("SM_RV_Waterfall"),Basin+FountainFacing.RotateVector(FVector(10,160,35)),FountainFacing,
   FVector(.70,1,1.25),false,TEXT("M_RV_Waterfall"));
  if(auto* WaterSound=LoadObject<USoundBase>(nullptr,TEXT("/Game/Audio/Reverie/S_WaterLoop.S_WaterLoop"))){
   AActor* WaterActor=GetWorld()->SpawnActor<AActor>();Generated.Add(WaterActor);
   auto* WaterAudio=NewObject<UAudioComponent>(WaterActor);WaterActor->SetRootComponent(WaterAudio);WaterActor->AddInstanceComponent(WaterAudio);
   WaterAudio->bAutoActivate=false;WaterAudio->bAutoDestroy=false;WaterAudio->bOverrideAttenuation=true;
   WaterAudio->AttenuationOverrides.bSpatialize=true;WaterAudio->AttenuationOverrides.AttenuationShape=EAttenuationShape::Sphere;
   WaterAudio->AttenuationOverrides.AttenuationShapeExtents=FVector(400,0,0);WaterAudio->AttenuationOverrides.FalloffDistance=900.f;
   WaterAudio->SetWorldLocation(Basin+FVector(0,0,120));WaterAudio->SetSound(WaterSound);WaterAudio->SetVolumeMultiplier(.55f);
   WaterAudio->RegisterComponent();WaterAudio->Play();
  }
  StoneBedding(C+FVector(810,1240,12),FVector2D(604,174));
  Place(TEXT("SM_RV_Rill"),C+FVector(810,1240,50),FRotator::ZeroRotator);
  Place(TEXT("SM_RV_WaterPlane"),C+FVector(810,1240,35),FRotator::ZeroRotator,FVector(6,1.14,1),false,TEXT("M_RV_Water"));
  Place(TEXT("SM_RV_SculptedBank"),C+FVector(460,1240,-25),FRotator(0,20,0),FVector(.43));
  Place(TEXT("SM_RV_CrystalCluster"),C+FVector(470,1240,112),FRotator(0,Variant*70.f,0),FVector(.75),false);
  for(int32 Plant=0;Plant<12;++Plant){
   const FVector Growth=C+FVector(550+Plant*65,Plant%2?1465.f:1050.f,-20);
   Place(Plant%3==0?TEXT("SM_RV_Flowers"):TEXT("SM_RV_Fern"),Growth,FRotator(0,Art.FRandRange(0,360),0),FVector(.75f),false);
  }
  // Later sanctuaries gain a usable side terrace. Eight real shallow treads
  // reach its paved overlook; this branch rejoins the same open combat floor.
  // It occupies the unused SW quarter, beyond every retained encounter pad.
  if(RoomIndex>0){
   const FVector Terrace=C+FVector(-1200,-1200,0);
   Ground(Terrace+FVector(0,0,80),FVector(300,300,80),0);
   for(int32 X=-1;X<=1;++X)for(int32 Y=-1;Y<=1;++Y)
    Place(TEXT("SM_RV_TileB"),Terrace+FVector(X*200,Y*200,140),FRotator(0,(X+Y)*90.f,0));
   // Close every exposed side below the landing cap. These retain160cm
   // tops and embedded footings rather than leaving the rear tiles cantilevered.
   for(int32 Sign:{-1,1}){
    Place(TEXT("SM_RV_Wall"),Terrace+FVector(Sign*300.f,0,0),FRotator(0,90,0),FVector(1.5,1,.2666667f));
    Place(TEXT("SM_RV_Wall"),Terrace+FVector(0,Sign*300.f,0),FRotator::ZeroRotator,FVector(1.5,1,.2666667f));
   }
   // The stair asset begins atZ0; soil here is22cm lower and descends along
   // the outer side. A solid buried base supports its complete run, with
   // a125cm paved apron before the first riser and20cm physical side margins.
   StoneBedding(C+FVector(-605,-1200,0),FVector2D(610,480));
   StoneBedding(C+FVector(-237.5,-1200,-18),FVector2D(125,450));
   Place(TEXT("SM_RV_TileB"),C+FVector(-237.5,-1200,-20),FRotator::ZeroRotator,FVector(.625,2.4,1));
   // The imported FBX rises along local negativeY, opposite the Blender
   // positiveY authoring note. This puts20cm at the court and160cm at the
   // terrace, verified by actual runtime collision tread traces.
   Place(TEXT("SM_RV_Steps"),C+FVector(-600,-1200,0),FRotator(0,-90,0));
   Place(TEXT("SM_RV_CrystalCluster"),Terrace+FVector(-150,-130,160),FRotator(0,35+Variant*50.f,0),FVector(.85),false);
   Place(TEXT("SM_RV_Fern"),Terrace+FVector(-175,80,160),FRotator(0,75,0),FVector(.80),false);
  }
  // Seat the stepped plinth into the imperfect flagstone surface while
  // retaining its70cm physical top and the ward's existing interaction height.
  Place(TEXT("SM_RV_ArrivalPlinth"),C+WardOffset-FVector(0,0,80),FRotator(0,20+Variant*30.f,0),FVector(1,1,74.f/70.f));
  R.Altar=MovingMesh(TEXT("SM_RV_WardCrystal"),C+WardOffset,FRotator::ZeroRotator,FVector(1.1));
  auto* WardLight=GetWorld()->SpawnActor<APointLight>(C+WardOffset+FVector(0,0,75),FRotator::ZeroRotator);
  WardLight->GetLightComponent()->SetMobility(EComponentMobility::Movable);
  WardLight->GetLightComponent()->SetIntensity(500.f);WardLight->GetLightComponent()->SetLightColor(FLinearColor(.25f,.82f,.74f));
  Cast<UPointLightComponent>(WardLight->GetLightComponent())->SetAttenuationRadius(420.f);
  WardLight->GetLightComponent()->SetCastShadows(false);Generated.Add(WardLight);
  // LivingWorld: small habitat compositions add ecological and historical
  // detail to the existing court graph. All new meshes are decorative HISMs;
  // floor/capsule geometry, stair heights, gateways and encounter pads remain
  // governed by the structures above. Each footprint is checked, not just its
  // center, and contact derives from the actual world floor before planting.
  FRandomStream Habitat(Seed+420041+RoomIndex*1013);
  int32 LivingInstances=0;
  auto HabitatClear=[&Reserved,&SlabDistance,RoomIndex](FVector P,float Radius,bool Wet){
   if(FMath::Abs(P.X)+Radius>1375.f||FMath::Abs(P.Y)+Radius>1375.f)return false;
   if(!Wet&&P.X>280.f&&P.Y>940.f)return false;
   if(RoomIndex>0&&P.X-Radius<-205.f&&P.Y-Radius<-875.f)return false;
   if(SlabDistance(P)<Radius+15.f)return false;
   for(const FVector& Margin:{FVector::ZeroVector,FVector(Radius,0,0),FVector(-Radius,0,0),FVector(0,Radius,0),FVector(0,-Radius,0)})
    if(Reserved(P+Margin))return false;
   return true;
  };
  auto SoilProp=[this,&Place,&C,&HabitatClear,&LivingInstances](const TCHAR* Name,FVector P,float Size,float Yaw,float Radius,bool Wet=false){
   if(!HabitatClear(P,Radius,Wet))return false;
   FHitResult Contact;
   if(!GetWorld()->LineTraceSingleByChannel(Contact,C+P+FVector(0,0,240),C+P-FVector(0,0,140),ECC_Visibility))return false;
   // Bank/cover and rim tops are not planting ground. The existing central
   // soil/paving contacts lie at -22..0; outer descending shoulders are excluded.
   if(Contact.ImpactNormal.Z<.88f||Contact.ImpactPoint.Z-C.Z>5.f||Contact.ImpactPoint.Z-C.Z<-25.f)return false;
   if(Place(Name,Contact.ImpactPoint-FVector(0,0,2),FRotator(0,Yaw,0),FVector(Size),false)){++LivingInstances;return true;}
   return false;
  };
  // The spring has reeds and ferns along its receiving-channel bank, not
  // plants sprayed over its stone rims. Dry worlds use fewer reeds; the moss
  // court is dampest, retaining a readable habitat shift inside this one biome.
  for(int32 Patch=0;Patch<(RoomIndex==1?8:5);++Patch){
   const FVector P(475.f+Patch*62.f+Habitat.FRandRange(-12,12),1025.f+Habitat.FRandRange(-22,16),0);
   SoilProp(Patch%3==1?TEXT("SM_LW_FernRosette"):TEXT("SM_LW_Reeds"),P,
    Habitat.FRandRange(.63f,.85f),Habitat.FRandRange(0,360),51.f,true);
  }
  SoilProp(TEXT("SM_LW_FungusLog"),FVector(535,985,0),.72f,12.f+Variant*17.f,76.f,true);
  SoilProp(TEXT("SM_LW_ShelfFungi"),FVector(415,1055,0),.90f,160.f,29.f,true);
  // Pads float on the actual 35cm receiving-basin water surface. Their local
  // footprint and offsets fit within its 242.5cm water disc, clear of the rim.
  for(int32 Pad=0;Pad<3;++Pad){
   const float Angle=215.f+Pad*68.f+Variant*7.f;
   const FVector Offset=FRotator(0,Angle,0).Vector()*(175.f+Pad*5.f);
   if(Place(TEXT("SM_LW_WaterLily"),Basin+Offset+FVector(0,0,35.7f),FRotator(0,Angle+40,0),FVector(.64f+Pad*.035f),false))++LivingInstances;
  }
  for(int32 Insect=0;Insect<3;++Insect){
   const FVector P(650.f+Insect*170.f,1130.f+Insect*26.f,80.f+Insect*19.f);
   if(Place(TEXT("SM_LW_Dragonfly"),C+P,FRotator(0,50.f+Insect*95.f,0),FVector(.95f+Insect*.15f),false))++LivingInstances;
  }
  // Dry garden islands originate from selected ruin/tree-edge pockets. A
  // changed seed changes which companion grows around an anchor and its size,
  // orientation and broken-pot/lintel composition, with protected empty space.
  const FVector GardenAnchors[]={FVector(-1190,-650,0),FVector(-550,1270,0),FVector(1200,-570,0),
   FVector(1260,235,0),FVector(-1230,360,0),FVector(-805,-850,0)};
  for(int32 Island=0;Island<UE_ARRAY_COUNT(GardenAnchors);++Island){
   const FVector Anchor=GardenAnchors[Island]+FVector(Habitat.FRandRange(-32,32),Habitat.FRandRange(-25,25),0);
   const bool Damp=(RoomIndex==1||(Island+Variant)%3==0);
   const TCHAR* Feature=Damp?TEXT("SM_LW_FungusLog"):(Island%2?TEXT("SM_LW_CrackedUrn"):TEXT("SM_LW_FallenLintel"));
   const float Facing=Habitat.FRandRange(0,360);
   if(!SoilProp(Feature,Anchor,.78f,Facing,83.f))continue;
   SoilProp(TEXT("SM_LW_RootFan"),Anchor+FVector(-34,24,0),.88f,Facing+57.f,68.f);
   for(int32 Companion=0;Companion<5;++Companion){
    const float A=Facing+Companion*67.f;
    const FVector P=Anchor+FRotator(0,A,0).Vector()*Habitat.FRandRange(84.f,115.f);
    const TCHAR* Type=Damp?(Companion%2?TEXT("SM_LW_FernRosette"):TEXT("SM_LW_ShelfFungi")):
     (Companion==0?TEXT("SM_LW_TwistedScrub"):TEXT("SM_LW_MeadowFlowers"));
    const float Size=Habitat.FRandRange(.67f,.94f);
    if(SoilProp(Type,P,Size,A,Companion==0?57.f:42.f)&&!Damp&&Companion==2){
     if(Place(TEXT("SM_LW_GardenMoth"),C+P+FVector(12,-8,68),FRotator(0,A,0),FVector(1.4f),false))++LivingInstances;
    }
   }
  }
  // Existing paving-edge ribbons gain occasional divided ferns and flowers;
  // their slab-distance/footprint checks retain all deliberate path breaks.
  for(int32 Drift=0;Drift<Drifts.Num();++Drift){
   const FVector P=Drifts[Drift]+FVector(Habitat.FRandRange(-28,28),Habitat.FRandRange(-25,25),0);
   SoilProp(Drift%3==0?TEXT("SM_LW_MeadowFlowers"):TEXT("SM_LW_FernRosette"),P,
    Habitat.FRandRange(.60f,.80f),Habitat.FRandRange(0,360),44.f);
  }
  UE_LOG(LogTemp,Display,TEXT("DB_LIVING_WORLD room=%d habitat=%s instances=%d"),RoomIndex,RoomIndex==1?TEXT("damp spring"):TEXT("dry ruin garden"),LivingInstances);
 }
 for(int32 I=1;I<Rooms.Num();++I){
  const FVector A=Rooms[I-1].Center,B=Rooms[I].Center,D=(B-A).GetSafeNormal2D(),T(-D.Y,D.X,0);
  const FVector Mid=(A+B)*.5f;const float Yaw=D.Rotation().Yaw;
  Place(TEXT("SM_RV_Bridge"),Mid,FRotator(0,Yaw,0));
  Place(TEXT("SM_RV_WaterPlane"),Mid+FVector(0,0,-115),FRotator(0,Yaw,0),FVector(5.45,27,1),false,TEXT("M_RV_Water"));
  // The mid-bridge barrier spans the full 700cm deck, not just the 440cm
  // arch opening. Its visible lattice and solid collision both reach curbs.
  if(AStaticMeshActor* Gate=MovingMesh(TEXT("SM_RV_Gate"),Mid,FRotator(0,Yaw,0),FVector(1,1.6f,1))){
   auto* Barrier=NewObject<UBoxComponent>(Gate);Gate->AddInstanceComponent(Barrier);
   // Child collision inherits the actor's Y scale: 218.75*1.6=350cm.
   Barrier->SetupAttachment(Gate->GetRootComponent());Barrier->SetBoxExtent(FVector(16,350.f/1.6f,250));
   Barrier->SetRelativeLocation(FVector(0,0,250));Barrier->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
   Barrier->SetCollisionObjectType(ECC_WorldStatic);Barrier->SetCollisionResponseToAllChannels(ECR_Block);
   Barrier->SetHiddenInGame(true);Barrier->SetVisibility(false);Barrier->RegisterComponent();Rooms[I].Gates.Add(Gate);
  }
  for(int32 Sign:{-1,1})Place(TEXT("SM_RV_CrystalCluster"),Mid+T*Sign*370.f+FVector(0,0,65),FRotator(0,Yaw+Sign*30.f,0),FVector(.55),false);
 }
 // Painted cloud dome, warm sun and cool indirect light replace the former
 // open blue void. Height fog ties the distant ruin crests to the garden.
 const FVector SkyCenter=(Centers[0]+Centers[1]+Centers[2])/3.f+FVector(0,0,-200);
 Place(TEXT("SM_RV_SkyDome"),SkyCenter,FRotator(0,15,0),FVector(1500),false,TEXT("M_RV_Sky"));
 auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,3000),FRotator(-33,-38,0));
 Sun->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Sun->GetLightComponent()->SetIntensity(5.0f);Sun->GetLightComponent()->SetLightColor(FLinearColor(1,.80,.53));
 Sun->GetLightComponent()->SetIndirectLightingIntensity(1.2f);
 Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->SetForwardShadingPriority(1);
 Cast<UDirectionalLightComponent>(Sun->GetLightComponent())->LightSourceAngle=2.4f;Generated.Add(Sun);
 auto* Sky=GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetMobility(EComponentMobility::Movable);
 Sky->GetLightComponent()->SetRealTimeCaptureEnabled(true);Sky->GetLightComponent()->SetIntensity(1.1f);
 Sky->GetLightComponent()->SetLightColor(FLinearColor(.62,.83,.86));Sky->GetLightComponent()->RecaptureSky();Generated.Add(Sky);
 auto* Fill=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,2500),FRotator(-52,142,0));
 Fill->GetLightComponent()->SetMobility(EComponentMobility::Movable);Fill->GetLightComponent()->SetIntensity(.65f);
 Fill->GetLightComponent()->SetLightColor(FLinearColor(.53,.75,.83));
 Cast<UDirectionalLightComponent>(Fill->GetLightComponent())->SetForwardShadingPriority(0);
 Fill->GetLightComponent()->SetCastShadows(false);Generated.Add(Fill);
 auto* Fog=GetWorld()->SpawnActor<AExponentialHeightFog>();Fog->GetComponent()->SetFogDensity(.016f);
 Fog->GetComponent()->SetFogHeightFalloff(.27f);Fog->GetComponent()->SetStartDistance(1100.f);
 Fog->GetComponent()->SetFogMaxOpacity(.58f);Fog->GetComponent()->SetFogInscatteringColor(FLinearColor(.29,.40,.40));Generated.Add(Fog);
 auto* Post=GetWorld()->SpawnActor<APostProcessVolume>();Post->bUnbound=true;
 Post->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;Post->Settings.AutoExposureApplyPhysicalCameraExposure=false;
 Post->Settings.bOverride_AutoExposureBias=true;Post->Settings.AutoExposureBias=.05f;
 Post->Settings.bOverride_BloomIntensity=true;Post->Settings.BloomIntensity=.22f;
 Post->Settings.bOverride_VignetteIntensity=true;Post->Settings.VignetteIntensity=.12f;
 Post->Settings.bOverride_AmbientOcclusionIntensity=true;Post->Settings.AmbientOcclusionIntensity=.85f;Generated.Add(Post);
 UE_LOG(LogTemp,Display,TEXT("DB_LAYOUT seed=%d %s / reverie mesh families=%d"),Seed,*LayoutSignature,Assets.Num());
 bSliceAwaitingStart=!IsCleared(CurrentRoomId);UpdateGates();
}
