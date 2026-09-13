#include "WorkbenchRuntime.h"
#include "MagnetGame.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/DirectionalLight.h"
#include "Engine/PointLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/StaticMesh.h"
#include "Engine/TextureCube.h"
#include "Engine/Engine.h"
#include "Engine/Font.h"
#include "Styling/CoreStyle.h"
#include "Fonts/CompositeFont.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/GameUserSettings.h"
#include "Engine/World.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Sound/SoundBase.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"

using namespace MagnetSweep;
namespace {
const FVector2D Furnace(650,0);
FLinearColor C(float R,float G,float B,float A=1) {return FLinearColor(R,G,B,A);}
float Ease(float T) {return 1-FMath::Pow(1-FMath::Clamp(T,0.f,1.f),3);}
}
UStaticMesh* FWorkbenchImpl::Mesh(FName Name) {
 if(auto* Found=Meshes.Find(Name)) return *Found;
 FString Path;
 if(Name==TEXT("Cube")||Name==TEXT("Cylinder")||Name==TEXT("Sphere")) Path=TEXT("/Engine/BasicShapes/")+Name.ToString()+TEXT(".")+Name.ToString();
 else Path=TEXT("/Game/MagnetSweep/Meshes/")+Name.ToString()+TEXT(".")+Name.ToString();
 UStaticMesh* Result=LoadObject<UStaticMesh>(nullptr,*Path);
 if(!Result) {UE_LOG(LogTemp,Warning,TEXT("Magnet missing mesh %s"),*Path); Result=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));}
 Owner->Resources.Add(Result);Meshes.Add(Name,Result);return Result;
}
UMaterialInstanceDynamic* FWorkbenchImpl::Mat(FName Name,FLinearColor Color,float Metal,float Rough,float Glow,bool bOverlay) {
 auto* M=UMaterialInstanceDynamic::Create(bOverlay&&Overlay?Overlay:Surface,Owner);
 M->SetVectorParameterValue(TEXT("Color"),Color);M->SetScalarParameterValue(TEXT("Metallic"),Metal);M->SetScalarParameterValue(TEXT("Roughness"),Rough);M->SetScalarParameterValue(TEXT("Glow"),Glow);
 Materials.Add(Name,M);Owner->Resources.Add(M);return M;
}
void FWorkbenchImpl::Place(UStaticMeshComponent* S,FVector P,FVector Size,FRotator R) {
 if(!S||!S->GetStaticMesh()) return;
 const auto B=S->GetStaticMesh()->GetBounds();
 FVector Scale=Size/(B.BoxExtent*2).ComponentMax(FVector(.01));
 S->SetWorldTransform(FTransform(R,P-R.RotateVector(B.Origin*Scale),Scale));
}
UStaticMeshComponent* FWorkbenchImpl::Shape(FName MeshName,FVector P,FVector Size,FName Material,FRotator R,bool Static) {
 auto* S=NewObject<UStaticMeshComponent>(Owner);
 S->SetMobility(EComponentMobility::Movable);S->SetStaticMesh(Mesh(MeshName));
 if(Materials.Contains(Material)) S->SetMaterial(0,Materials[Material]);
 for(int32 Slot=0;Slot<S->GetStaticMesh()->GetStaticMaterials().Num();++Slot)
 {
  const FName SlotName=S->GetStaticMesh()->GetStaticMaterials()[Slot].MaterialSlotName;
  if(auto* Assigned=Materials.Find(SlotName))S->SetMaterial(Slot,*Assigned);
 }
 if(MeshName==TEXT("SM_HeroMagnet"))
  if(auto* Hero=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MagnetSweep/Materials/M_HeroMagnet.M_HeroMagnet")))
  {S->SetMaterial(0,Hero);Owner->Resources.AddUnique(Hero);}
 S->SetCollisionEnabled(ECollisionEnabled::NoCollision);S->SetGenerateOverlapEvents(false);
 Owner->AddInstanceComponent(S);S->RegisterComponent();Place(S,P,Size,R);
 if(Static) StaticShapes.Add(S);return S;
}
void FWorkbenchImpl::SetupScene() {
 auto MakeFont=[&](const TCHAR* File)
 {
  auto* Font=NewObject<UFont>(Owner);Font->FontCacheType=EFontCacheType::Runtime;
  const FString Path=FPaths::ProjectContentDir()/TEXT("Fonts")/File;
  if(FPaths::FileExists(Path))Font->CompositeFont.DefaultTypeface.Fonts.Add(FTypefaceEntry(TEXT("Regular"),Path,EFontHinting::Default,EFontLoadingPolicy::LazyLoad));
  else {Font->CompositeFont=*FCoreStyle::GetDefaultFont();UE_LOG(LogTemp,Warning,TEXT("Missing visual font %s"),File);}
  Owner->Resources.Add(Font);return Font;
 };
 RuntimeFont=MakeFont(TEXT("Barlow-Medium.ttf"));DisplayFont=MakeFont(TEXT("BarlowCondensed-SemiBold.ttf"));
 Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MagnetSweep/Materials/M_VisualSurface.M_VisualSurface"));
 if(!Surface)Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MagnetSweep/Materials/M_Surface.M_Surface"));
 if(!Surface) Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 Owner->Resources.Add(Surface);
 Mat(TEXT("Background"),C(.0015,.003,.005),0,.85);
 Mat(TEXT("Base"),C(.004,.012,.017),.12,.7);
 Mat(TEXT("Tray"),C(.030,.045,.049),.5,.51);
 Mat(TEXT("Grid"),C(.034,.067,.070),.15,.65);
 Mat(TEXT("Edge"),C(.13,.20,.21),.6,.3);
 Mat(TEXT("Dark"),C(.009,.016,.018),.2,.6);
 Mat(TEXT("Steel"),C(.33,.40,.43),.88,.23);
 Mat(TEXT("Alloy"),C(.18,.36,.58),.72,.18);
 Mat(TEXT("Hazard"),C(.48,.018,.009),.3,.28);
 Mat(TEXT("Warning"),C(1,.08,.005),.15,.3,2.2);
 Mat(TEXT("Core"),C(1,.48,.045),.68,.18,1.3);
 Mat(TEXT("Glow"),C(.04,.65,.50),.1,.28,1.8);
 Mat(TEXT("Copper"),C(.7,.255,.075),.65,.28);
 Mat(TEXT("Brass"),C(.56,.31,.085),.8,.28);
 Mat(TEXT("Ivory"),C(.78,.81,.69),.25,.4);
 Mat(TEXT("Magnet"),C(.018,.11,.13),.54,.32);
 Mat(TEXT("RimGlow"),C(.025,.25,.21),.1,.4,.45);
 Mat(TEXT("Hot"),C(.95,.15,.012),.1,.35,3.2);
 Mat(TEXT("Selected"),C(.36,1,.79),.4,.2,.6);
 Mat(TEXT("Ring"),C(1,.58,.2),.6,.26,.2);
 Camera=NewObject<UCameraComponent>(Owner);Owner->AddInstanceComponent(Camera);Camera->RegisterComponent();
 Camera->SetWorldLocation(FVector(80,920,1500));Camera->SetWorldRotation((FVector(80,0,0)-Camera->GetComponentLocation()).Rotation());
 Camera->ProjectionMode=ECameraProjectionMode::Orthographic;Camera->OrthoWidth=1900;Camera->bOverrideAspectRatioAxisConstraint=true;Camera->AspectRatioAxisConstraint=EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV;Camera->bConstrainAspectRatio=false;
 Camera->OrthoNearClipPlane=1;Camera->OrthoFarClipPlane=10000;
 PC->SetViewTarget(Owner);
 auto* Key=Owner->GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector,FRotator(-56,-40,0));
 Key->GetLightComponent()->SetIntensity(5.4);Key->GetLightComponent()->SetLightColor(C(1,.84,.67));Cast<UDirectionalLightComponent>(Key->GetLightComponent())->SetLightSourceAngle(9.f);
 Cast<UDirectionalLightComponent>(Key->GetLightComponent())->SetForwardShadingPriority(1);
 auto* Fill=Owner->GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector,FRotator(-38,145,0));
 Fill->GetLightComponent()->SetIntensity(1.65);Fill->GetLightComponent()->SetLightColor(C(.5,.77,1));Fill->GetLightComponent()->SetCastShadows(false);
 auto* Sky=Owner->GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetIntensity(.35);Sky->GetLightComponent()->bLowerHemisphereIsBlack=false;Sky->GetLightComponent()->LowerHemisphereColor=C(.22,.3,.34);Sky->GetLightComponent()->RecaptureSky();
 if(auto* Ambient=LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap"))){Owner->Resources.Add(Ambient);Sky->GetLightComponent()->SourceType=SLS_SpecifiedCubemap;Sky->GetLightComponent()->SetCubemap(Ambient);Sky->GetLightComponent()->SetIntensity(1.0f);}
 auto* PP=Owner->GetWorld()->SpawnActor<APostProcessVolume>();PP->bUnbound=true;
 PP->Settings.bOverride_AutoExposureMethod=true;PP->Settings.AutoExposureMethod=AEM_Manual;
 PP->Settings.bOverride_AutoExposureBias=true;PP->Settings.AutoExposureBias=0;
 PP->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;PP->Settings.AutoExposureApplyPhysicalCameraExposure=false;
 PP->Settings.bOverride_BloomIntensity=true;PP->Settings.BloomIntensity=.48f;
 PP->Settings.bOverride_VignetteIntensity=true;PP->Settings.VignetteIntensity=.25f;
 PP->Settings.bOverride_AmbientOcclusionIntensity=true;PP->Settings.AmbientOcclusionIntensity=.8f;
 // Detailed workshop kit is purely visual; every collision and target stays in the model.
 Shape(TEXT("Cube"),FVector(80,0,-170),FVector(2600,2000,45),TEXT("Background"));
 Shape(TEXT("Cube"),FVector(80,0,-56),FVector(1510,850,68),TEXT("Base"));
 Shape(TEXT("SM_VDeck"),FVector(0,0,-21),FVector(1080,704,66),TEXT("Dark"));
 for(int Side:{-1,1}) {
  for(int Front:{-1,1}) {
   Shape(TEXT("Cube"),FVector(Side*650,Front*310,-105),FVector(85,85,150),TEXT("Dark"));
   Shape(TEXT("SM_VBearing"),FVector(Side*650,Front*310,-163),FVector(110,110,22),TEXT("Steel"));
  }
  Shape(TEXT("Cube"),FVector(Side*555,0,-7),FVector(14,735,30),TEXT("Copper"));
  for(int I=0;I<9;I++)Shape(TEXT("Cube"),FVector(Side*594,-275+I*64,-6),FVector(44,23,13),TEXT("Dark"));
 }
 // A service column and small working gauges give the tray a believable setting.
 Shape(TEXT("Cube"),FVector(-671,0,9),FVector(128,480,42),TEXT("Magnet"));
 for(int I=0;I<3;I++) {
  Shape(TEXT("SM_VBearing"),FVector(-674,-155+I*142,45),FVector(84,84,15),TEXT("Brass"));
  Shape(TEXT("Cylinder"),FVector(-674,-155+I*142,55),FVector(45,45,3),TEXT("Dark"));
  Shape(TEXT("Cube"),FVector(-674,-155+I*142,58),FVector(28,3,2),TEXT("Ivory"),FRotator(0,25+I*32,0));
 }
 for(int I=0;I<16;I++) {
  const float X=-480+I*62;
  Shape(TEXT("Cube"),FVector(X,-369,0),FVector(33,12,5),I%2?TEXT("Dark"):TEXT("Brass"),FRotator(0,20,0));
 }
 Shape(TEXT("SM_VFurnace"),FVector(650,0,30),FVector(236,278,148),TEXT("Dark"));
 FurnaceFill=Shape(TEXT("Cylinder"),FVector(650,0,61),FVector(142,142,3),TEXT("Hot"));
 if(auto* Heat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MagnetSweep/Materials/M_FurnaceHeat.M_FurnaceHeat")))
 {FurnaceFill->SetMaterial(0,Heat);Owner->Resources.AddUnique(Heat);}
 for(int I=0;I<3;I++)Shape(TEXT("SM_VHeatSink"),FVector(657,210+I*42,6),FVector(135,32,24),TEXT("Copper"));
 auto* Lamp=Owner->GetWorld()->SpawnActor<APointLight>(FVector(650,0,115),FRotator::ZeroRotator);
 FurnaceLight=Lamp->PointLightComponent;FurnaceLight->SetIntensity(2300);FurnaceLight->SetAttenuationRadius(430);FurnaceLight->SetLightColor(C(1,.26,.045));FurnaceLight->SetCastShadows(false);
 auto* Rim=Owner->GetWorld()->SpawnActor<APointLight>(FVector(-450,-420,210),FRotator::ZeroRotator);
 Rim->PointLightComponent->SetIntensity(1400);Rim->PointLightComponent->SetAttenuationRadius(850);Rim->PointLightComponent->SetLightColor(C(.13,.58,.73));Rim->PointLightComponent->SetCastShadows(false);
 const FVector HeroSize(136,147.4f,47);
 auto* Hero=Shape(TEXT("SM_HeroMagnet"),World(Magnet,78),HeroSize,TEXT("Magnet"),FRotator::ZeroRotator,false);
 MagnetShapes.Add(Hero);MagnetOffsets.Add(FVector::ZeroVector);MagnetSizes.Add(HeroSize);
 RecoveredBlock=Shape(TEXT("SM_Plate"),FVector(650,-216,25),FVector(118,70,30),TEXT("Copper"));RecoveredBlock->SetVisibility(false);
 const TArray<FName> CueNames={TEXT("magnet_loop"),TEXT("magnet_on"),TEXT("magnet_off"),TEXT("pickup_metal1"),TEXT("pickup_metal2"),TEXT("pickup_metal3"),TEXT("pickup_heavy"),TEXT("rare_find"),TEXT("vent"),TEXT("warning_loop"),TEXT("overload"),TEXT("smelt"),TEXT("payout"),TEXT("upgrade"),TEXT("contract_success"),TEXT("contract_fail"),TEXT("ui_click"),TEXT("workshop_music"),TEXT("furnace_loop")};
 for(FName Name:CueNames){USoundBase* S=LoadObject<USoundBase>(nullptr,*(TEXT("/Game/MagnetSweep/Audio/rework/")+Name.ToString()+TEXT(".")+Name.ToString()));if(S){Sounds.Add(Name,S);Owner->Resources.Add(S);}else UE_LOG(LogTemp,Error,TEXT("MISSING REWORK SOUND %s"),*Name.ToString());}
 UE_LOG(LogTemp,Display,TEXT("MAGNET_DEMO_BEGIN profile=%s sounds=%d layout=%d"),*Profile,Sounds.Num(),Model.GetLayoutIndex());
}
