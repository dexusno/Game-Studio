#pragma once
#include "CoreMinimal.h"
#include "DBTypes.generated.h"

UENUM(BlueprintType)
enum class EDBElement : uint8 { Neutral, Frost, Storm, Ember };
UENUM(BlueprintType)
enum class EDBEnemyKind : uint8 { Melee, Caster, Hunter, Boss };

USTRUCT(BlueprintType)
struct FDBHit {
 GENERATED_BODY()
 float Damage = 20.f;
 EDBElement Element = EDBElement::Neutral;
 bool bImpact = false;
 bool bSecondary = false;
 bool bStormfracture = false;
 FVector Source = FVector::ZeroVector;
 FVector Direction = FVector::ForwardVector;
 AActor* InstigatorActor = nullptr;
};
