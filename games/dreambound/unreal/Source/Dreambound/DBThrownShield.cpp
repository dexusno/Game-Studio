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
    Core->SetRelativeLocation(FVector::ZeroVector);
    Core->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

void ADBThrownShield::BeginPlay()
{
    Super::BeginPlay();
    Disc->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_ShieldSegment.SM_ShieldSegment")));
    Core->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_ShieldSegmentGlow.SM_ShieldSegmentGlow")));
    ImpactSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Impact.S_Impact"));
    BlockSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Guard.S_Guard"));
}

void ADBThrownShield::Initialize(ADBCharacter* InBearer, FVector Direction, float Charge, int32 InPieceId)
{
    Bearer = InBearer;
    PieceId = InPieceId;
    SetOwner(InBearer);
    if (!Bearer || PieceId < 0 || PieceId >= Bearer->GetShieldPieceCount()) { Destroy(); return; }
    FlightDirection = Direction.GetSafeNormal();
    if (FlightDirection.IsNearlyZero()) FlightDirection = FVector::ForwardVector;
    Charge = FMath::Clamp(Charge, 0.f, 1.f);
    const int32 RamRank = Bearer->GetUpgradeRank(TEXT("Ram"));
    Speed = 1800.f + Charge * 350.f + RamRank * 90.f;
    Damage = 23.f + Charge * 4.f + RamRank * 3.f;
    Range = 1400.f + Charge * 400.f;
    Ricochets = Bearer->GetUpgradeRank(TEXT("Split"));
    bAnchor = Bearer->HasUpgrade(TEXT("Anchor"));
    AnchorIntegrity = 30.f + 15.f * Bearer->GetUpgradeRank(TEXT("Anchor"));
    OutwardPath.Add(GetActorLocation());
    LastBearerPoint = Bearer->GetPieceCatchLocation(PieceId);
    BearerPath.Add(LastBearerPoint);
    AnchorForward = FlightDirection.GetSafeNormal2D();
    if (AnchorForward.IsNearlyZero()) AnchorForward = Bearer->GetActorForwardVector();
    bInitialized = true;
    if (!Disc->GetStaticMesh() || !Core->GetStaticMesh())
        UE_LOG(LogTemp, Error, TEXT("Shield piece %d has no segmented art; import SM_ShieldSegment and SM_ShieldSegmentGlow."), PieceId);
    UpdateMaterial();
    UpdatePiecePose(0.f);
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
    const FVector Point = Bearer->GetPieceCatchLocation(PieceId);
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
    if (IsActorBeingDestroyed() || bResolved) return;
    UpdatePiecePose(DeltaSeconds);
}

void ADBThrownShield::UpdatePiecePose(float DeltaSeconds)
{
    if (!Bearer) return;
    if (!bLodged) Spin += DeltaSeconds * (bReturning ? 740.f : 620.f);
    const FQuat DockRotation = FRotator(0.f, 0.f, PieceId * 60.f).Quaternion();
    FQuat PieceRotation = FRotator(0.f, 0.f, PieceId * 60.f + (bLodged ? 0.f : Spin)).Quaternion();
    FQuat FrameRotation = (bLodged ? AnchorForward : FlightDirection).Rotation().Quaternion();
    if (bReturning)
    {
        const float Alpha = FMath::Clamp(1.f - FVector::Distance(GetActorLocation(), Bearer->GetPieceCatchLocation(PieceId)) / 240.f, 0.f, 1.f);
        PieceRotation = FQuat::Slerp(PieceRotation, DockRotation, Alpha);
        FrameRotation = FQuat::Slerp(FrameRotation, Bearer->WeaponRoot->GetComponentQuat(), Alpha);
    }
    SetActorRotation(FrameRotation);
    // The FBX pivot is at shield centre, while collision follows the actual arc's centre.
    // Translate by the rotated centroid so spin never sweeps an invisible full disc.
    Disc->SetRelativeRotation(PieceRotation);
    Disc->SetRelativeLocation(-PieceRotation.RotateVector(FVector(0.f, 0.f, 26.f)));
    Disc->SetHiddenInGame(bEmergencyReturn);
    Core->SetHiddenInGame(false);
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
    const float Radius = Bearer->GetUpgradeRank(TEXT("Ram")) >= 3 ? 25.f : 18.f;
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
                const FVector IncomingDirection = (Destination - Start).GetSafeNormal();
                if (!bOnReturn && Enemy->TryInterceptShieldPiece(IncomingDirection))
                {
                    SetActorLocation(Hit.Location);
                    DestroyPiece();
                    return false;
                }
                FDBHit Contact;
                Contact.Damage = Damage * (bOnReturn ? 1.12f : 1.f);
                Contact.Element = Bearer->CurrentElement;
                // Outbound frost lays chill; the return or a held strike consumes it.
                Contact.bImpact = bOnReturn || Contact.Element != EDBElement::Frost;
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
    if (bResolved || IsActorBeingDestroyed() || bReturning || !Bearer->CanAct()) return;
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
    Bearer->OnShieldPieceFlightState(PieceId, this, EDBShieldPieceState::Lodged);
    Bearer->LastCombatMessage = bAnchor ? TEXT("ANCHOR PIECE SET - its visible arc blocks frontal bolts / Q recalls") : TEXT("PIECE DEPLOYED - Q recalls / remaining pieces stay usable");
    Bearer->MessageTime = 2.4f;
}

void ADBThrownShield::Recall()
{
    if (!bInitialized || bResolved || bReturning || !IsValid(Bearer) || !Bearer->CanAct()) return;
    bReturning = true;
    bLodged = false;
    ReturnTime = 0.f;
    RetraceIndex = OutwardPath.Num() - 1;
    BearerTrailIndex = 0;
    bFollowingBearerTrail = false;
    Bearer->OnShieldPieceFlightState(PieceId, this, EDBShieldPieceState::Returning);
}

void ADBThrownShield::TickReturn(float DeltaSeconds)
{
    ReturnTime += DeltaSeconds;
    const FVector CatchPoint = Bearer->GetPieceCatchLocation(PieceId);
    const float CatchDistance = FVector::Distance(GetActorLocation(), CatchPoint);
    if (CatchDistance < 11.f && (bEmergencyReturn || ClearWorldPath(GetActorLocation(), CatchPoint, 8.f)))
    {
        Catch(bEmergencyReturn);
        return;
    }
    // Invalidated routes recover as a visible non-damaging energy insert. No sweep/hit
    // happens during this exceptional recovery, and it never makes a spare piece.
    if (ReturnTime > 4.5f || GetActorLocation().Z < -2500.f) bEmergencyReturn = true;
    if (bEmergencyReturn)
    {
        FlightDirection = (CatchPoint - GetActorLocation()).GetSafeNormal();
        SetActorLocation(GetActorLocation() + FlightDirection * FMath::Min(3100.f * DeltaSeconds, CatchDistance));
        return;
    }

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
    if (!bInitialized || bResolved || !bLodged || !bAnchor || bUnblockable || !Bearer || !Bearer->CanAct()) return false;
    const FVector Segment = End - Start;
    const float Approach = FVector::DotProduct(Segment.GetSafeNormal(), AnchorForward);
    if (Approach > -.15f) return false;
    const float Denominator = FVector::DotProduct(Segment, AnchorForward);
    if (FMath::Abs(Denominator) < .001f) return false;
    const float T = FVector::DotProduct(GetActorLocation() - Start, AnchorForward) / Denominator;
    if (T < 0.f || T > 1.f) return false;
    const FVector Contact = Start + Segment * T;
    // Intercept only where this physical 58-degree arc actually exists, not a whole disc.
    const FVector LocalContact = Disc->GetComponentTransform().InverseTransformPosition(Contact);
    const float RadialDistance = FMath::Sqrt(LocalContact.Y * LocalContact.Y + LocalContact.Z * LocalContact.Z);
    if (RadialDistance < 13.f || RadialDistance > 40.f
        || LocalContact.Z / FMath::Max(1.f, RadialDistance) < .86f) return false;
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
        Bearer->LastCombatMessage = TEXT("ANCHOR PIECE BROKEN - rebuilding");
        Bearer->MessageTime = 1.5f;
        DestroyPiece();
    }
    return true;
}

void ADBThrownShield::DestroyPiece()
{
    if (bResolved || !bInitialized || !IsValid(Bearer) || !Bearer->CanAct()) return;
    bResolved = true;
    Bearer->OnShieldPieceDestroyed(PieceId, this);
    Destroy();
}

void ADBThrownShield::Catch(bool bEmergency)
{
    if (bResolved) return;
    bResolved = true;
    if (IsValid(Bearer)) Bearer->OnShieldPieceCaught(PieceId, this, bEmergency);
    Destroy();
}
