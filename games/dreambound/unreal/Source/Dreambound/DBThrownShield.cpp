#include "DBThrownShield.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "Camera/CameraComponent.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "Materials/MaterialInterface.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"

ADBThrownShield::ADBThrownShield()
{
    PrimaryActorTick.bCanEverTick = true;
    FlightRoot = CreateDefaultSubobject<USceneComponent>(TEXT("FlightRoot"));
    SetRootComponent(FlightRoot);
    Disc = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicalShield"));
    Disc->SetupAttachment(FlightRoot);
    Disc->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Disc->SetCastShadow(true);
    Core = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicalCore"));
    Core->SetupAttachment(Disc);
    Core->SetRelativeLocation(FVector(-6.f, 0.f, 0.f));
    Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADBThrownShield::BeginPlay()
{
    Super::BeginPlay();
    Disc->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_ShieldPlate.SM_ShieldPlate")));
    Core->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Core.SM_Core")));
    ImpactSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Impact.S_Impact"));
    BlockSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Guard.S_Guard"));
}

void ADBThrownShield::Initialize(ADBCharacter* InBearer, FVector Direction, float Charge)
{
    Bearer = InBearer;
    SetOwner(InBearer);
    if (!Bearer) { Destroy(); return; }
    FlightDirection = Direction.GetSafeNormal();
    if (FlightDirection.IsNearlyZero()) FlightDirection = FVector::ForwardVector;
    Charge = FMath::Clamp(Charge, 0.f, 1.f);
    const int32 RamRank = Bearer->GetUpgradeRank(TEXT("Ram"));
    Speed = 1650.f + Charge * 700.f + RamRank * 90.f;
    Damage = 58.f + Charge * 38.f + RamRank * 8.f;
    Range = 1300.f + Charge * 900.f;
    Ricochets = Bearer->GetUpgradeRank(TEXT("Split"));
    bAnchor = Bearer->HasUpgrade(TEXT("Anchor"));
    AnchorIntegrity = 90.f + 35.f * Bearer->GetUpgradeRank(TEXT("Anchor"));
    OutwardPath.Add(GetActorLocation());
    LastBearerPoint = Bearer->GetShieldCatchLocation();
    BearerPath.Add(LastBearerPoint);
    // Carry the installed physical inserts with the same shield, rather than leaving
    // the recognizable build attached to an empty first-person wrist.
    const FName Ids[] = { TEXT("Mirror"), TEXT("Ram"), TEXT("Echo"), TEXT("Frost"), TEXT("Ember"),
        TEXT("Storm"), TEXT("Stormfracture"), TEXT("Split"), TEXT("Capacitor"), TEXT("Anchor") };
    TArray<UStaticMeshComponent*> ViewParts;
    Bearer->GetComponents<UStaticMeshComponent>(ViewParts);
    for (UStaticMeshComponent* SourcePart : ViewParts)
    {
        const FString PartName = SourcePart->GetName();
        if (!PartName.StartsWith(TEXT("Attachment")) || !SourcePart->GetStaticMesh()) continue;
        const int32 Index = FCString::Atoi(*PartName.Mid(10));
        if (Index < 0 || Index >= UE_ARRAY_COUNT(Ids) || !Bearer->HasUpgrade(Ids[Index])) continue;
        UStaticMeshComponent* FlightPart = NewObject<UStaticMeshComponent>(this);
        AddInstanceComponent(FlightPart);
        FlightPart->SetupAttachment(Disc);
        FlightPart->SetStaticMesh(SourcePart->GetStaticMesh());
        FlightPart->SetRelativeTransform(SourcePart->GetRelativeTransform());
        FlightPart->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        for (int32 Slot = 0; Slot < SourcePart->GetNumMaterials(); ++Slot)
            FlightPart->SetMaterial(Slot, SourcePart->GetMaterial(Slot));
        FlightPart->RegisterComponent();
        FlightAttachments.Add(FlightPart);
    }
    AnchorForward = FlightDirection.GetSafeNormal2D();
    if (AnchorForward.IsNearlyZero()) AnchorForward = Bearer->GetActorForwardVector();
    bInitialized = true;
    UpdateMaterial();
}

void ADBThrownShield::UpdateMaterial()
{
    if (!Bearer) return;
    LastElement = Bearer->CurrentElement;
    const TCHAR* Name = LastElement == EDBElement::Frost ? TEXT("M_Frost")
        : LastElement == EDBElement::Storm ? TEXT("M_Storm")
        : LastElement == EDBElement::Ember ? TEXT("M_Ember") : TEXT("M_Core");
    const FString Path = FString::Printf(TEXT("/Game/Art/Materials/%s.%s"), Name, Name);
    if (UMaterialInterface* Material = LoadObject<UMaterialInterface>(nullptr, *Path))
    {
        const int32 NamedSlot = Core->GetMaterialIndex(TEXT("M_Core"));
        Core->SetMaterial(NamedSlot == INDEX_NONE ? 0 : NamedSlot, Material);
    }
}

void ADBThrownShield::RecordBearerPath()
{
    const FVector Point = Bearer->GetShieldCatchLocation();
    if (FVector::DistSquared(Point, LastBearerPoint) > FMath::Square(65.f))
    {
        // The player's actual travelled route supplies a recovery path around new cover.
        if (BearerPath.Num() < 512) BearerPath.Add(Point);
        LastBearerPoint = Point;
    }
}

void ADBThrownShield::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!bInitialized || !IsValid(Bearer)) { Destroy(); return; }
    if (!Bearer->CanAct()) return;
    RecordBearerPath();
    if (LastElement != Bearer->CurrentElement) UpdateMaterial();
    BlockFlash = FMath::Max(0.f, BlockFlash - DeltaSeconds);
    if (bReturning) TickReturn(DeltaSeconds);
    else if (!bLodged) TickOutbound(DeltaSeconds);
    if (IsActorBeingDestroyed()) return;
    if (!bLodged)
    {
        Spin += DeltaSeconds * (bReturning ? 1200.f : 930.f);
        SetActorRotation(FlightDirection.Rotation());
        Disc->SetRelativeRotation(FRotator(77.f, 0.f, Spin));
    }
    else
    {
        SetActorRotation(AnchorForward.Rotation());
        Disc->SetRelativeRotation(FRotator(0.f, 0.f, FMath::Sin(BlockFlash * 25.f) * BlockFlash * 12.f));
    }
}

bool ADBThrownShield::ClearWorldPath(FVector From, FVector To, float Radius) const
{
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBShieldRoute), false, this);
    Params.AddIgnoredActor(Bearer);
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It) Params.AddIgnoredActor(*It);
    FHitResult Hit;
    return !GetWorld()->SweepSingleByChannel(Hit, From, To, FQuat::Identity,
        ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params);
}

bool ADBThrownShield::SweepTravel(FVector Destination, bool bOnReturn)
{
    FVector Start = GetActorLocation();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBPhysicalShield), false, this);
    Params.AddIgnoredActor(Bearer);
    TSet<TWeakObjectPtr<ADBEnemy>>& Victims = bOnReturn ? ReturnVictims : OutwardVictims;
    for (const TWeakObjectPtr<ADBEnemy>& Victim : Victims) if (Victim.IsValid()) Params.AddIgnoredActor(Victim.Get());
    const float Radius = bOnReturn ? 20.f : (Bearer->GetUpgradeRank(TEXT("Ram")) >= 3 ? 43.f : 34.f);
    for (int32 Pass = 0; Pass < 12; ++Pass)
    {
        FHitResult Hit;
        if (!GetWorld()->SweepSingleByChannel(Hit, Start, Destination, FQuat::Identity,
            ECC_Visibility, FCollisionShape::MakeSphere(Radius), Params))
        {
            SetActorLocation(Destination);
            return true;
        }
        if (ADBEnemy* Enemy = Cast<ADBEnemy>(Hit.GetActor()))
        {
            Params.AddIgnoredActor(Enemy);
            if (!Enemy->bDead && !Victims.Contains(Enemy))
            {
                Victims.Add(Enemy);
                FDBHit Contact;
                Contact.Damage = Damage * (bOnReturn ? 1.12f : 1.f);
                Contact.Element = Bearer->CurrentElement;
                Contact.bImpact = true;
                Contact.bStormfracture = Bearer->HasUpgrade(TEXT("Stormfracture"));
                Contact.Source = Start;
                Contact.Direction = (Destination - Start).GetSafeNormal();
                Contact.InstigatorActor = Bearer;
                Bearer->ApplyPhysicalShieldHit(Enemy, Contact);
                if (!Bearer->CanAct()) { SetActorLocation(Hit.Location); return false; }
                // A final kill can request recall from inside this damage callback.
                if (!bOnReturn && bReturning) { SetActorLocation(Hit.Location); return false; }
                if (!bOnReturn && Ricochets > 0)
                {
                    SetActorLocation(Hit.Location);
                    if (RedirectToEnemy(Enemy)) { --Ricochets; return true; }
                }
            }
            Start = Hit.Location + (Destination - Start).GetSafeNormal() * 2.f;
            continue;
        }
        SetActorLocation(Hit.Location + Hit.Normal * 2.f);
        if (!bOnReturn)
        {
            if (Ricochets > 0 && !bAnchor && !Hit.bStartPenetrating)
            {
                FlightDirection = FMath::GetReflectionVector(FlightDirection, Hit.Normal).GetSafeNormal();
                --Ricochets;
                OutwardPath.Add(GetActorLocation());
            }
            else Lodge(GetActorLocation(), Hit.Normal);
        }
        return false;
    }
    // A dense formation never creates an unbounded collision-processing loop.
    SetActorLocation(Start);
    return false;
}

bool ADBThrownShield::RedirectToEnemy(ADBEnemy* Previous)
{
    ADBEnemy* Best = nullptr;
    float BestDistance = FMath::Square(1000.f);
    const float Cone = Bearer->GetUpgradeRank(TEXT("Split")) >= 3 ? -.45f : .05f;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It)
    {
        if (It->bDead || *It == Previous || OutwardVictims.Contains(*It)) continue;
        const FVector Delta = It->GetActorLocation() + FVector(0,0,20) - GetActorLocation();
        if (Delta.SizeSquared() >= BestDistance || FVector::DotProduct(Delta.GetSafeNormal(), FlightDirection) < Cone) continue;
        if (!ClearWorldPath(GetActorLocation(), It->GetActorLocation() + FVector(0,0,20), 25.f)) continue;
        BestDistance = Delta.SizeSquared();
        Best = *It;
    }
    if (!Best) return false;
    OutwardPath.Add(GetActorLocation());
    FlightDirection = (Best->GetActorLocation() + FVector(0,0,20) - GetActorLocation()).GetSafeNormal();
    return true;
}

void ADBThrownShield::TickOutbound(float DeltaSeconds)
{
    OutwardTime += DeltaSeconds;
    const float Distance = FMath::Min(Speed * DeltaSeconds, 120.f);
    const FVector Before = GetActorLocation();
    SweepTravel(Before + FlightDirection * Distance, false);
    if (bReturning || !Bearer->CanAct()) return;
    Travelled += FVector::Distance(Before, GetActorLocation());
    if (OutwardPath.Num() == 0 || FVector::DistSquared(OutwardPath.Last(), GetActorLocation()) > FMath::Square(65.f))
        OutwardPath.Add(GetActorLocation());
    if (!bLodged && (Travelled >= Range || OutwardTime >= 2.5f)) Lodge(GetActorLocation(), FVector::ZeroVector);
}

void ADBThrownShield::Lodge(FVector Location, FVector SurfaceNormal)
{
    bLodged = true;
    if (bAnchor && SurfaceNormal.Z > .55f)
        Location.Z += 100.f; // The magical anchor suspends a ground placement at standing chest height.
    SetActorLocation(Location);
    OutwardPath.Add(Location);
    if (ImpactSound) UGameplayStatics::PlaySoundAtLocation(this, ImpactSound, Location, .8f, .72f);
    Bearer->ShieldState = EDBShieldState::Lodged;
    Bearer->LastCombatMessage = bAnchor ? TEXT("ANCHOR SET - cover has one facing / Q recalls") : TEXT("SHIELD AWAY - move for a return cut / Q recalls");
    Bearer->MessageTime = 2.4f;
}

void ADBThrownShield::Recall()
{
    if (!bInitialized || bReturning || !IsValid(Bearer)) return;
    bReturning = true;
    bLodged = false;
    ReturnTime = 0.f;
    RetraceIndex = OutwardPath.Num() - 1;
    BearerTrailIndex = 0;
    bFollowingBearerTrail = false;
    Bearer->ShieldState = EDBShieldState::Returning;
}

void ADBThrownShield::TickReturn(float DeltaSeconds)
{
    ReturnTime += DeltaSeconds;
    const FVector CatchPoint = Bearer->GetShieldCatchLocation();
    if (FVector::DistSquared(GetActorLocation(), CatchPoint) < FMath::Square(72.f)) { Catch(false); return; }
    // If navigation geometry changes or a moving platform invalidates a route, recover
    // visibly without dealing a through-wall hit. Ordinary recall still travels physically.
    if (ReturnTime > 4.5f || GetActorLocation().Z < -2500.f) { Catch(true); return; }

    FVector TargetPoint = CatchPoint;
    if (!ClearWorldPath(GetActorLocation(), CatchPoint))
    {
        if (!bFollowingBearerTrail)
        {
            while (RetraceIndex >= 0 && FVector::DistSquared(GetActorLocation(), OutwardPath[RetraceIndex]) < FMath::Square(55.f)) --RetraceIndex;
            if (RetraceIndex >= 0) TargetPoint = OutwardPath[RetraceIndex];
            else bFollowingBearerTrail = true;
        }
        if (bFollowingBearerTrail)
        {
            while (BearerTrailIndex < BearerPath.Num() && FVector::DistSquared(GetActorLocation(), BearerPath[BearerTrailIndex]) < FMath::Square(65.f)) ++BearerTrailIndex;
            if (BearerTrailIndex < BearerPath.Num()) TargetPoint = BearerPath[BearerTrailIndex];
        }
    }
    FlightDirection = (TargetPoint - GetActorLocation()).GetSafeNormal();
    const float Step = FMath::Min3(2650.f * DeltaSeconds, 140.f, static_cast<float>(FVector::Distance(TargetPoint, GetActorLocation())));
    SweepTravel(GetActorLocation() + FlightDirection * Step, true);
}

bool ADBThrownShield::InterceptProjectile(FVector Start, FVector End, float IncomingDamage, bool bUnblockable)
{
    if (!bInitialized || !bLodged || !bAnchor || bUnblockable || !Bearer || !Bearer->CanAct()) return false;
    const FVector Segment = End - Start;
    const float Approach = FVector::DotProduct(Segment.GetSafeNormal(), AnchorForward);
    if (Approach > -.15f) return false;
    const float Denominator = FVector::DotProduct(Segment, AnchorForward);
    if (FMath::Abs(Denominator) < .001f) return false;
    const float T = FVector::DotProduct(GetActorLocation() - Start, AnchorForward) / Denominator;
    if (T < 0.f || T > 1.f) return false;
    const FVector Contact = Start + Segment * T;
    if (FVector::DistSquared(Contact, GetActorLocation()) > FMath::Square(44.f)) return false;
    AnchorIntegrity -= FMath::Max(5.f, IncomingDamage);
    Bearer->BankAnchoredForce(IncomingDamage);
    BlockFlash = .35f;
    if (BlockSound) UGameplayStatics::PlaySoundAtLocation(this, BlockSound, Contact, .65f, .9f);
    if (AnchorIntegrity <= 0.f)
    {
        if (Bearer->GetUpgradeRank(TEXT("Anchor")) >= 3)
        {
            int32 Affected = 0;
            for (TActorIterator<ADBEnemy> It(GetWorld()); It && Affected < 8; ++It)
            {
                if (It->bDead || FVector::DistSquared(It->GetActorLocation(), GetActorLocation()) > FMath::Square(360.f)
                    || !ClearWorldPath(GetActorLocation(), It->GetActorLocation(), 4.f)) continue;
                FDBHit Burst;
                Burst.Damage = 48.f;
                Burst.Element = Bearer->CurrentElement;
                Burst.bImpact = true;
                Burst.bSecondary = true;
                Burst.Source = GetActorLocation();
                Burst.Direction = (It->GetActorLocation() - GetActorLocation()).GetSafeNormal();
                Burst.InstigatorActor = Bearer;
                Bearer->ApplyPhysicalShieldHit(*It, Burst);
                ++Affected;
            }
        }
        Bearer->LastCombatMessage = TEXT("ANCHOR BROKEN - shield returning");
        Bearer->MessageTime = 1.5f;
        Recall();
    }
    return true;
}

void ADBThrownShield::Catch(bool bEmergency)
{
    if (IsValid(Bearer)) Bearer->OnShieldCaught(bEmergency);
    Destroy();
}
