#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Materials/MaterialInterface.h"

/** Shared original volume rendering for the drawn charge and its actual projectile. */
namespace DBOrganicFire
{
    inline FLinearColor V(const FVector& P) { return FLinearColor(P.X,P.Y,P.Z,0.f); }

    inline UMaterialInstanceDynamic* Prepare(UStaticMeshComponent* Component, UObject* Owner)
    {
        Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube")));
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Component->SetCastShadow(false);
        Component->SetGenerateOverlapEvents(false);
        Component->SetCanEverAffectNavigation(false);
        UMaterialInterface* Source=LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Art/OrganicFire/Materials/M_OrganicFireVolume.M_OrganicFireVolume"));
        if (!Source) { UE_LOG(LogTemp,Error,TEXT("Organic fire material is missing; import the fire package.")); return nullptr; }
        UMaterialInstanceDynamic* Material=UMaterialInstanceDynamic::Create(Source,Owner);
        Component->SetMaterial(0,Material);
        return Material;
    }

    inline void Draw(UStaticMeshComponent* Component, UMaterialInstanceDynamic* Material,
        FVector Center, FRotator Rotation, FVector HalfSize, float Time, float Intensity,
        float Mode, float Seed, float Glow=1.f)
    {
        const bool Visible=Material && Intensity>.005f;
        Component->SetVisibility(Visible);
        if (!Visible) return;
        Component->SetWorldLocationAndRotation(Center,Rotation);
        Component->SetWorldScale3D(HalfSize/50.f);
        Material->SetVectorParameterValue(TEXT("Center"),V(Center));
        Material->SetVectorParameterValue(TEXT("Forward"),V(Rotation.Vector()));
        Material->SetVectorParameterValue(TEXT("Right"),V(FRotationMatrix(Rotation).GetUnitAxis(EAxis::Y)));
        Material->SetVectorParameterValue(TEXT("Up"),V(FRotationMatrix(Rotation).GetUnitAxis(EAxis::Z)));
        Material->SetVectorParameterValue(TEXT("HalfSize"),V(HalfSize));
        Material->SetScalarParameterValue(TEXT("Time"),Time);
        Material->SetScalarParameterValue(TEXT("Intensity"),Intensity);
        Material->SetScalarParameterValue(TEXT("Mode"),Mode);
        Material->SetScalarParameterValue(TEXT("Seed"),Seed);
        Material->SetScalarParameterValue(TEXT("Glow"),Glow);
    }
}
