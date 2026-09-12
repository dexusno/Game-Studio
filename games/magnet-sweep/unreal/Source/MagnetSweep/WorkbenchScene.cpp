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
 S->SetCollisionEnabled(ECollisionEnabled::NoCollision);S->SetGenerateOverlapEvents(false);
 Owner->AddInstanceComponent(S);S->RegisterComponent();Place(S,P,Size,R);
 if(Static) StaticShapes.Add(S);return S;
}
void FWorkbenchImpl::SetupScene() {
 RuntimeFont=NewObject<UFont>(Owner);RuntimeFont->FontCacheType=EFontCacheType::Runtime;RuntimeFont->CompositeFont=*FCoreStyle::GetDefaultFont();Owner->Resources.Add(RuntimeFont);
 Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/MagnetSweep/Materials/M_Surface.M_Surface"));
 if(!Surface) Surface=LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
 Owner->Resources.Add(Surface);
 Mat(TEXT("Background"),C(.0015,.003,.005),0,.85);
 Mat(TEXT("Base"),C(.004,.012,.017),.12,.7);
 Mat(TEXT("Tray"),C(.002,.006,.010),0,.87);
 Mat(TEXT("Grid"),C(.034,.067,.070),.15,.65);
 Mat(TEXT("Edge"),C(.13,.20,.21),.6,.3);
 Mat(TEXT("Dark"),C(.009,.016,.018),.2,.6);
 Mat(TEXT("Steel"),C(.37,.49,.53),.65,.20);
 Mat(TEXT("Alloy"),C(.18,.36,.58),.72,.18);
 Mat(TEXT("Hazard"),C(.48,.018,.009),.3,.28);
 Mat(TEXT("Warning"),C(1,.08,.005),.15,.3,2.2);
 Mat(TEXT("Core"),C(1,.48,.045),.68,.18,1.3);
 Mat(TEXT("Glow"),C(.04,.65,.50),.1,.28,1.8);
 Mat(TEXT("Copper"),C(.7,.255,.075),.65,.28);
 Mat(TEXT("Brass"),C(.85,.52,.15),.7,.24);
 Mat(TEXT("Ivory"),C(.78,.81,.69),.25,.4);
 Mat(TEXT("Magnet"),C(.055,.38,.37),.4,.28);
 Mat(TEXT("Hot"),C(.8,.09,.008),.15,.3,.8);
 Mat(TEXT("Selected"),C(.36,1,.79),.4,.2,.6);
 Mat(TEXT("Ring"),C(1,.58,.2),.6,.26,.2);
 Camera=NewObject<UCameraComponent>(Owner);Owner->AddInstanceComponent(Camera);Camera->RegisterComponent();
 Camera->SetWorldLocation(FVector(80,920,1500));Camera->SetWorldRotation((FVector(80,0,0)-Camera->GetComponentLocation()).Rotation());
 Camera->ProjectionMode=ECameraProjectionMode::Orthographic;Camera->OrthoWidth=1900;Camera->bOverrideAspectRatioAxisConstraint=true;Camera->AspectRatioAxisConstraint=EAspectRatioAxisConstraint::AspectRatio_MaintainXFOV;Camera->bConstrainAspectRatio=false;
 Camera->OrthoNearClipPlane=1;Camera->OrthoFarClipPlane=10000;
 PC->SetViewTarget(Owner);
 auto* Key=Owner->GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector,FRotator(-56,-40,0));
 Key->GetLightComponent()->SetIntensity(4.5);Key->GetLightComponent()->SetLightColor(C(1,.84,.67));Cast<UDirectionalLightComponent>(Key->GetLightComponent())->SetLightSourceAngle(5.f);
 auto* Fill=Owner->GetWorld()->SpawnActor<ADirectionalLight>(FVector::ZeroVector,FRotator(-38,145,0));
 Fill->GetLightComponent()->SetIntensity(2.2);Fill->GetLightComponent()->SetLightColor(C(.5,.77,1));Fill->GetLightComponent()->SetCastShadows(false);
 auto* Sky=Owner->GetWorld()->SpawnActor<ASkyLight>();Sky->GetLightComponent()->SetIntensity(.35);Sky->GetLightComponent()->bLowerHemisphereIsBlack=false;Sky->GetLightComponent()->LowerHemisphereColor=C(.22,.3,.34);Sky->GetLightComponent()->RecaptureSky();
 if(auto* Ambient=LoadObject<UTextureCube>(nullptr,TEXT("/Engine/MapTemplates/Sky/DaylightAmbientCubemap.DaylightAmbientCubemap"))){Owner->Resources.Add(Ambient);Sky->GetLightComponent()->SourceType=SLS_SpecifiedCubemap;Sky->GetLightComponent()->SetCubemap(Ambient);Sky->GetLightComponent()->SetIntensity(.65f);}
 auto* PP=Owner->GetWorld()->SpawnActor<APostProcessVolume>();PP->bUnbound=true;
 PP->Settings.bOverride_AutoExposureMethod=true;PP->Settings.AutoExposureMethod=AEM_Manual;
 PP->Settings.bOverride_AutoExposureBias=true;PP->Settings.AutoExposureBias=0;
 PP->Settings.bOverride_AutoExposureApplyPhysicalCameraExposure=true;PP->Settings.AutoExposureApplyPhysicalCameraExposure=false;
 PP->Settings.bOverride_BloomIntensity=true;PP->Settings.BloomIntensity=.48f;
 PP->Settings.bOverride_VignetteIntensity=true;PP->Settings.VignetteIntensity=.25f;
 PP->Settings.bOverride_AmbientOcclusionIntensity=true;PP->Settings.AmbientOcclusionIntensity=.4f;
 Shape(TEXT("Cube"),FVector(80,0,-95),FVector(2600,2000,65),TEXT("Background"));
 Shape(TEXT("Cube"),FVector(80,0,-40),FVector(1470,800,58),TEXT("Base"));
 Shape(TEXT("SM_Tray"),FVector(0,0,-6),FVector(1040,660,22),TEXT("Tray"));
 for(int I=-4;I<=4;I++) Shape(TEXT("Cube"),FVector(I*100,0,5.3),FVector(.7,615,.5),TEXT("Grid"));
 for(int I=-2;I<=2;I++) Shape(TEXT("Cube"),FVector(0,I*100,5.3),FVector(995,.7,.5),TEXT("Grid"));
 for(int Side:{-1,1}) {
  Shape(TEXT("Cube"),FVector(Side*530,0,12),FVector(32,695,34),TEXT("Edge"));
  Shape(TEXT("Cube"),FVector(0,Side*337,12),FVector(1030,25,34),TEXT("Edge"));
  for(int Y:{-1,1}) Shape(TEXT("Cylinder"),FVector(Side*531,Y*324,33),FVector(15,15,7),TEXT("Brass"));
 }
 for(int I=0;I<7;I++) Shape(TEXT("Cube"),FVector(-420+I*125,-372,-7),FVector(68,10,2),I%2?TEXT("Dark"):TEXT("Copper"));
 // Replace an empty backing plane with a compact, lit salvage station.
 for(int Side:{-1,1}){
  Shape(TEXT("Cube"),FVector(Side*565,0,-6),FVector(12,750,18),TEXT("Dark"));
  for(int Y=-280;Y<=280;Y+=80)Shape(TEXT("Cylinder"),FVector(Side*532,Y,32),FVector(10,10,5),TEXT("Steel"));
  Shape(TEXT("Cube"),FVector(0,Side*349,30),FVector(905,4,4),TEXT("Glow"));
 }
 for(int I=0;I<19;I++)Shape(TEXT("Cube"),FVector(630+(I%4)*18,-300+(I/4)*18,12),FVector(10,10,6),TEXT("Edge"));
 for(int I=0;I<3;I++){
  Shape(TEXT("Cube"),FVector(660,220+I*34,13),FVector(145,23,16),TEXT("Edge"));
  Shape(TEXT("Cube"),FVector(635,220+I*34,24),FVector(70,12,4),TEXT("Copper"));
 }
 Shape(TEXT("Cube"),FVector(660,0,8),FVector(215,280,66),TEXT("Dark"));
 Shape(TEXT("Cylinder"),FVector(650,0,30),FVector(208,208,38),TEXT("Copper"));
 Shape(TEXT("Cylinder"),FVector(650,0,55),FVector(174,174,8),TEXT("Dark"));
 for(int Z=62;Z<=95;Z+=8)Shape(TEXT("SM_Ring"),FVector(650,0,Z),FVector(203,203,12),Z==94?TEXT("Brass"):TEXT("Copper"));
 FurnaceFill=Shape(TEXT("Cylinder"),FVector(650,0,62),FVector(142,142,3),TEXT("Hot"));
 for(int Side:{-1,1}) for(int I=0;I<5;I++) Shape(TEXT("Cube"),FVector(650+(I-2)*34,Side*123,43),FVector(20,14,4),I%2?TEXT("Dark"):TEXT("Brass"));
 auto* Lamp=Owner->GetWorld()->SpawnActor<APointLight>(FVector(650,0,120),FRotator::ZeroRotator);
 FurnaceLight=Lamp->PointLightComponent;FurnaceLight->SetIntensity(1800);FurnaceLight->SetAttenuationRadius(480);FurnaceLight->SetLightColor(C(1,.3,.065));FurnaceLight->SetCastShadows(false);
 auto AddMagnet=[&](FName MeshName,FVector Offset,FVector Size,FName Material){
  auto* S=Shape(MeshName,World(Magnet,80)+Offset,Size,Material,FRotator::ZeroRotator,false);
  MagnetShapes.Add(S);MagnetOffsets.Add(Offset);MagnetSizes.Add(Size);return S;};
 AddMagnet(TEXT("SM_MagnetBody"),FVector(0,0,0),FVector(100,90,29),TEXT("Magnet"));
 AddMagnet(TEXT("SM_MagnetBody"),FVector(0,0,-8),FVector(108,98,13),TEXT("Dark"));
 AddMagnet(TEXT("Cube"),FVector(0,33,16),FVector(44,16,8),TEXT("Copper"));
 AddMagnet(TEXT("Cylinder"),FVector(0,33,22),FVector(12,12,5),TEXT("Glow"));
 for(int Side:{-1,1}) {
  AddMagnet(TEXT("Cube"),FVector(Side*35,-37,0),FVector(27,20,30),TEXT("Ivory"));
  auto* Insulator=AddMagnet(TEXT("Cube"),FVector(Side*41,-7,9),FVector(19,28,9),TEXT("Warning"));StabilizerParts.Add(Insulator);
  AddMagnet(TEXT("Cube"),FVector(Side*27,-18,0),FVector(24,5,22),TEXT("Copper"));
  for(int I=0;I<3;I++) {auto* Coil=AddMagnet(TEXT("SM_Ring"),FVector(Side*26,3+I*7,0),FVector(29,29,6),TEXT("Brass"));Coils.Add(Coil);}
  auto* Arm=AddMagnet(TEXT("Cube"),FVector(Side*48,20,0),FVector(28,14,14),TEXT("Copper"));ReachParts.Add(Arm);
 }
 RecoveredBlock=Shape(TEXT("SM_Plate"),FVector(650,-216,25),FVector(118,70,30),TEXT("Copper"));RecoveredBlock->SetVisibility(false);
 const TArray<FName> CueNames={TEXT("magnet_loop"),TEXT("magnet_on"),TEXT("magnet_off"),TEXT("pickup_metal1"),TEXT("pickup_metal2"),TEXT("pickup_metal3"),TEXT("pickup_heavy"),TEXT("rare_find"),TEXT("vent"),TEXT("warning_loop"),TEXT("overload"),TEXT("smelt"),TEXT("payout"),TEXT("upgrade"),TEXT("contract_success"),TEXT("contract_fail"),TEXT("ui_click"),TEXT("workshop_music"),TEXT("furnace_loop")};
 for(FName Name:CueNames){USoundBase* S=LoadObject<USoundBase>(nullptr,*(TEXT("/Game/MagnetSweep/Audio/rework/")+Name.ToString()+TEXT(".")+Name.ToString()));if(S){Sounds.Add(Name,S);Owner->Resources.Add(S);}else UE_LOG(LogTemp,Error,TEXT("MISSING REWORK SOUND %s"),*Name.ToString());}
 UE_LOG(LogTemp,Display,TEXT("MAGNET_DEMO_BEGIN profile=%s sounds=%d layout=%d"),*Profile,Sounds.Num(),Model.GetLayoutIndex());
}
