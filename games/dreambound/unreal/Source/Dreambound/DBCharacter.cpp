#include "DBCharacter.h"

#include "DBEnemy.h"
#include "DBGameMode.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/MaterialInterface.h"
#include "Sound/SoundBase.h"

namespace
{
    const FName MirrorId(TEXT("Mirror"));
    const FName RamId(TEXT("Ram"));
    const FName EchoId(TEXT("Echo"));
    const FName FrostId(TEXT("Frost"));
    const FName EmberId(TEXT("Ember"));
    const FName StormId(TEXT("Storm"));
    const FName FractureId(TEXT("Stormfracture"));
    const FName SplitId(TEXT("Split"));
    const FName CapacitorId(TEXT("Capacitor"));

    FName ElementId(EDBElement Element)
    {
        switch (Element)
        {
        case EDBElement::Frost: return FrostId;
        case EDBElement::Ember: return EmberId;
        case EDBElement::Storm: return StormId;
        default: return NAME_None;
        }
    }

    FLinearColor ElementColor(EDBElement Element)
    {
        switch (Element)
        {
        case EDBElement::Frost: return FLinearColor(.25f, .8f, 1.f);
        case EDBElement::Ember: return FLinearColor(1.f, .23f, .04f);
        case EDBElement::Storm: return FLinearColor(.55f, .38f, 1.f);
        default: return FLinearColor(.35f, 1.f, .72f);
        }
    }

    void PrepareViewMesh(UStaticMeshComponent* Mesh)
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetGenerateOverlapEvents(false);
        Mesh->SetCastShadow(false);
        Mesh->SetOnlyOwnerSee(true);
        Mesh->SetFirstPersonPrimitiveType(EFirstPersonPrimitiveType::FirstPerson);
        Mesh->bReceivesDecals = false;
    }
}

ADBCharacter::ADBCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    GetCapsuleComponent()->InitCapsuleSize(32.f, 92.f);
    GetCapsuleComponent()->SetCollisionResponseToChannel(ECC_Visibility, ECR_Block);
    bUseControllerRotationYaw = true;

    UCharacterMovementComponent* Movement = GetCharacterMovement();
    Movement->MaxWalkSpeed = 590.f;
    Movement->MaxAcceleration = 3200.f;
    Movement->BrakingDecelerationWalking = 2300.f;
    Movement->GroundFriction = 8.f;
    Movement->JumpZVelocity = 565.f;
    Movement->GravityScale = 1.55f;
    Movement->AirControl = .55f;
    Movement->MaxStepHeight = 42.f;

    ViewCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    ViewCamera->SetupAttachment(GetCapsuleComponent());
    ViewCamera->SetRelativeLocation(FVector(0.f, 0.f, 66.f));
    ViewCamera->bUsePawnControlRotation = true;
    ViewCamera->SetFieldOfView(94.f);
    ViewCamera->SetEnableFirstPersonFieldOfView(true);
    ViewCamera->SetFirstPersonFieldOfView(94.f);
    ViewCamera->SetEnableFirstPersonScale(true);
    ViewCamera->SetFirstPersonScale(.5f);

    WeaponRoot = CreateDefaultSubobject<USceneComponent>(TEXT("WeaponPivot"));
    WeaponRoot->SetupAttachment(ViewCamera);
    // Keep the cuff behind the receiver in screen space, rather than across the near plane.
    WeaponRoot->SetRelativeLocation(FVector(50.f, 30.f, -32.f));
    WeaponRoot->SetRelativeRotation(FRotator(0.f, -13.f, -8.f));

    Forearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CyborgForearm"));
    Forearm->SetupAttachment(WeaponRoot);
    Forearm->SetRelativeLocation(FVector(0.f, 1.5f, -3.f));
    Forearm->SetRelativeScale3D(FVector(1.f, .82f, .82f));
    PrepareViewMesh(Forearm);
    WeaponBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponBody"));
    WeaponBody->SetupAttachment(WeaponRoot);
    PrepareViewMesh(WeaponBody);
    WeaponCore = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponCore"));
    WeaponCore->SetupAttachment(WeaponRoot);
    WeaponCore->SetRelativeLocation(FVector(33.f, 0.f, 10.f));
    PrepareViewMesh(WeaponCore);

    for (int32 Index = 0; Index < 5; ++Index)
    {
        UStaticMeshComponent* Plate = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ShieldPetal%d"), Index));
        Plate->SetupAttachment(WeaponRoot);
        PrepareViewMesh(Plate);
        ShieldPlates.Add(Plate);
    }
    for (int32 Index = 0; Index < 9; ++Index)
    {
        UStaticMeshComponent* Part = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("Attachment%d"), Index));
        Part->SetupAttachment(WeaponRoot);
        PrepareViewMesh(Part);
        Part->SetVisibility(false);
        AttachmentParts.Add(Part);
    }
    CoreLight = CreateDefaultSubobject<UPointLightComponent>(TEXT("CoreGlow"));
    CoreLight->SetupAttachment(WeaponCore);
    CoreLight->SetIntensity(45.f);
    CoreLight->SetAttenuationRadius(105.f);
    CoreLight->SetCastShadows(false);
    CoreLight->SetLightColor(ElementColor(EDBElement::Neutral));
}

void ADBCharacter::BeginPlay()
{
    Super::BeginPlay();
    BeamMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    SparkMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Sphere.Sphere"));
    CeramicMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Ceramic.M_Ceramic"));
    BronzeMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Bronze.M_Bronze"));
    DarkMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_DarkMetal.M_DarkMetal"));
    CoreMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Core.M_Core"));
    FrostMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Frost.M_Frost"));
    StormMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Storm.M_Storm"));
    EmberMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Ember.M_Ember"));

    UStaticMesh* BodyMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_WeaponBody.SM_WeaponBody"));
    UStaticMesh* ArmMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Forearm.SM_Forearm"));
    UStaticMesh* CoreMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Core.SM_Core"));
    UStaticMesh* PlateMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_ShieldPlate.SM_ShieldPlate"));
    UStaticMesh* CrystalMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Crystal.SM_Crystal"));
    WeaponBody->SetStaticMesh(BodyMesh ? BodyMesh : BeamMesh.Get());
    Forearm->SetStaticMesh(ArmMesh ? ArmMesh : BeamMesh.Get());
    WeaponCore->SetStaticMesh(CoreMesh ? CoreMesh : SparkMesh.Get());
    if (!BodyMesh) WeaponBody->SetRelativeScale3D(FVector(.55f, .16f, .15f));
    if (!ArmMesh)
    {
        Forearm->SetRelativeLocation(FVector(-24.f, 0.f, -4.f));
        Forearm->SetRelativeScale3D(FVector(.43f, .12f, .12f));
    }
    if (!CoreMesh) WeaponCore->SetRelativeScale3D(FVector(.13f));
    for (int32 Index = 0; Index < ShieldPlates.Num(); ++Index)
    {
        UStaticMeshComponent* Plate = ShieldPlates[Index];
        Plate->SetStaticMesh(PlateMesh ? PlateMesh : BeamMesh.Get());
        // SM_ShieldPlate is already the complete five-petal crescent assembly.
        Plate->SetVisibility(Index == 0);
        if (!PlateMesh) Plate->SetRelativeScale3D(FVector(.28f, .1f, .035f));
    }
    for (int32 Index = 0; Index < AttachmentParts.Num(); ++Index)
    {
        UStaticMeshComponent* Part = AttachmentParts[Index];
        UStaticMesh* PartMesh = CrystalMesh;
        if (Index == 0 || Index == 1) PartMesh = PlateMesh;
        else if (Index == 2 || Index == 7 || Index == 8) PartMesh = CoreMesh;
        Part->SetStaticMesh(PartMesh ? PartMesh : SparkMesh.Get());
    }
    AttackSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Attack.S_Attack"));
    GuardSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Guard.S_Guard"));
    ParrySound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Parry.S_Parry"));
    DashSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Dash.S_Dash"));
    ImpactSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Impact.S_Impact"));
    EquipSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Equip.S_Equip"));
    HurtSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Hurt.S_Hurt"));
    RefreshEquipmentVisuals();
    // The title can pause gameplay before the first active Tick. Initialize the folded
    // assembly now so its imported default transforms never appear behind the menu.
    UpdateWeapon(0.f);
}

void ADBCharacter::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &ADBCharacter::MoveForward);
    Input->BindAxis(TEXT("MoveRight"), this, &ADBCharacter::MoveRight);
    Input->BindAxis(TEXT("Turn"), this, &ADBCharacter::Turn);
    Input->BindAxis(TEXT("LookUp"), this, &ADBCharacter::LookUp);
    Input->BindAction(TEXT("Fire"), IE_Pressed, this, &ADBCharacter::PressFire);
    Input->BindAction(TEXT("Fire"), IE_Released, this, &ADBCharacter::ReleaseFire).bExecuteWhenPaused = true;
    Input->BindAction(TEXT("Guard"), IE_Pressed, this, &ADBCharacter::PressGuard);
    Input->BindAction(TEXT("Guard"), IE_Released, this, &ADBCharacter::ReleaseGuard).bExecuteWhenPaused = true;
    Input->BindAction(TEXT("Special"), IE_Pressed, this, &ADBCharacter::UseSpecial);
    Input->BindAction(TEXT("Dash"), IE_Pressed, this, &ADBCharacter::Dash);
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &ADBCharacter::BeginJump);
    Input->BindAction(TEXT("Jump"), IE_Released, this, &ADBCharacter::EndJump).bExecuteWhenPaused = true;
    Input->BindAction(TEXT("Interact"), IE_Pressed, this, &ADBCharacter::Interact);
    Input->BindAction(TEXT("Reward1"), IE_Pressed, this, &ADBCharacter::ChooseRewardOne).bExecuteWhenPaused = true;
    Input->BindAction(TEXT("Reward2"), IE_Pressed, this, &ADBCharacter::ChooseRewardTwo).bExecuteWhenPaused = true;
    Input->BindAction(TEXT("Reward3"), IE_Pressed, this, &ADBCharacter::ChooseRewardThree).bExecuteWhenPaused = true;
    Input->BindAction(TEXT("Pause"), IE_Pressed, this, &ADBCharacter::TogglePause).bExecuteWhenPaused = true;
    Input->BindAction(TEXT("Build"), IE_Pressed, this, &ADBCharacter::ToggleBuild).bExecuteWhenPaused = true;
    Input->BindAction(TEXT("CycleElement"), IE_Pressed, this, &ADBCharacter::CycleElement);
}

bool ADBCharacter::CanAct() const
{
    if (bDead || !GetWorld()) return false;
    const ADBGameMode* Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode());
    return !Mode || (!Mode->bPaused && !Mode->bChoosingReward && !Mode->bShowingBuild
        && !Mode->bTitle && !Mode->bWon && !Mode->bDefeated);
}

void ADBCharacter::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (!CanAct())
    {
        if (!bWasMenuBlocked)
        {
            SuspendCombatInput();
            GetCharacterMovement()->StopMovementImmediately();
        }
        bWasMenuBlocked = true;
        return;
    }
    bWasMenuBlocked = false;
    FireCooldown = FMath::Max(0.f, FireCooldown - DeltaSeconds);
    GuardRaiseCooldown = FMath::Max(0.f, GuardRaiseCooldown - DeltaSeconds);
    DashCooldown = FMath::Max(0.f, DashCooldown - DeltaSeconds);
    SpecialCooldown = FMath::Max(0.f, SpecialCooldown - DeltaSeconds);
    GuardBreakTime = FMath::Max(0.f, GuardBreakTime - DeltaSeconds);
    InvulnerabilityTime = FMath::Max(0.f, InvulnerabilityTime - DeltaSeconds);
    HitMarkerTime = FMath::Max(0.f, HitMarkerTime - DeltaSeconds);
    HurtFlashTime = FMath::Max(0.f, HurtFlashTime - DeltaSeconds);
    ParryFlashTime = FMath::Max(0.f, ParryFlashTime - DeltaSeconds);
    MessageTime = FMath::Max(0.f, MessageTime - DeltaSeconds);
    SinceFired += DeltaSeconds;
    SinceGuarded += DeltaSeconds;
    SinceDamaged += DeltaSeconds;

    const float HeatCooling = bOverheated ? 41.f : (SinceFired > .2f ? 29.f : 7.f);
    Heat = FMath::Max(0.f, Heat - HeatCooling * DeltaSeconds);
    if (bOverheated && Heat <= 18.f)
    {
        bOverheated = false;
        SetCombatMessage(TEXT("Core vented"), 1.f);
    }
    if (bGuarding)
    {
        GuardEnergy = FMath::Max(0.f, GuardEnergy - 2.f * DeltaSeconds);
        if (GuardEnergy <= 0.f)
        {
            bGuarding = false;
            GuardBreakTime = 1.5f;
            SetCombatMessage(TEXT("GUARD BROKEN - dash to recover"));
        }
    }
    else if (SinceDamaged > .65f && GuardBreakTime <= 0.f)
    {
        GuardEnergy = FMath::Min(MaxGuardEnergy, GuardEnergy + (HasUpgrade(CapacitorId) ? 27.f : 21.f) * DeltaSeconds);
    }
    DashTime = FMath::Max(0.f, DashTime - DeltaSeconds);
    GetCharacterMovement()->GroundFriction = DashTime > 0.f ? 0.f : 8.f;
    GetCharacterMovement()->BrakingDecelerationFalling = DashTime > 0.f ? 0.f : 600.f;
    GetCharacterMovement()->MaxWalkSpeed = bGuarding ? 285.f : 590.f;
    if (bRushActive)
    {
        RushTime -= DeltaSeconds;
        ResolveRush();
        if (RushTime <= 0.f)
        {
            bRushActive = false;
            RushVictims.Reset();
        }
    }
    UpdateEchoes(DeltaSeconds);
    if (bWantsFire && FireCooldown <= 0.f) FirePulse();
    UpdateWeapon(DeltaSeconds);
    UpdateEffects(DeltaSeconds);
}

void ADBCharacter::MoveForward(float Value)
{
    if (CanAct() && !FMath::IsNearlyZero(Value))
        AddMovementInput(FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::X), Value);
}

void ADBCharacter::MoveRight(float Value)
{
    if (CanAct() && !FMath::IsNearlyZero(Value))
        AddMovementInput(FRotationMatrix(FRotator(0.f, GetControlRotation().Yaw, 0.f)).GetUnitAxis(EAxis::Y), Value);
}

void ADBCharacter::Turn(float Value)
{
    if (!CanAct()) return;
    AddControllerYawInput(Value * FMath::Clamp(MouseSensitivity, .25f, 2.5f));
    LookSwayX = FMath::Clamp(LookSwayX + Value * .12f, -3.f, 3.f);
}

void ADBCharacter::LookUp(float Value)
{
    if (!CanAct()) return;
    AddControllerPitchInput(Value * FMath::Clamp(MouseSensitivity, .25f, 2.5f));
    LookSwayY = FMath::Clamp(LookSwayY + Value * .1f, -2.f, 2.f);
}

void ADBCharacter::BeginJump() { if (CanAct()) Jump(); }
void ADBCharacter::EndJump() { StopJumping(); }

void ADBCharacter::PressFire()
{
    if (!CanAct()) return;
    bWantsFire = true;
    if (FireCooldown <= 0.f) FirePulse();
}

void ADBCharacter::ReleaseFire() { bWantsFire = false; }

void ADBCharacter::PressGuard()
{
    if (!CanAct() || GuardBreakTime > 0.f || GuardRaiseCooldown > 0.f || GuardEnergy < 8.f || bGuarding) return;
    bGuarding = true;
    SinceGuarded = 0.f;
    GuardRaiseCooldown = .38f;
    GuardEnergy -= 3.f;
    PlayCombatSound(GuardSound, .5f, 1.12f);
}

void ADBCharacter::ReleaseGuard() { bGuarding = false; }

void ADBCharacter::SuspendCombatInput()
{
    bWantsFire = false;
    bGuarding = false;
    bRushActive = false;
    RushVictims.Reset();
    EchoShots.Reset();
    StopJumping();
    ConsumeMovementInputVector();
}

void ADBCharacter::Interact()
{
    if (!CanAct()) return;
    if (ADBGameMode* Mode = GetWorld()->GetAuthGameMode<ADBGameMode>()) Mode->Interact();
}

void ADBCharacter::ChooseRewardOne()
{
    SuspendCombatInput();
    if (ADBGameMode* Mode = GetWorld()->GetAuthGameMode<ADBGameMode>()) Mode->ChooseReward(0);
}
void ADBCharacter::ChooseRewardTwo()
{
    SuspendCombatInput();
    if (ADBGameMode* Mode = GetWorld()->GetAuthGameMode<ADBGameMode>()) Mode->ChooseReward(1);
}
void ADBCharacter::ChooseRewardThree()
{
    SuspendCombatInput();
    if (ADBGameMode* Mode = GetWorld()->GetAuthGameMode<ADBGameMode>()) Mode->ChooseReward(2);
}
void ADBCharacter::TogglePause()
{
    SuspendCombatInput();
    if (ADBGameMode* Mode = GetWorld()->GetAuthGameMode<ADBGameMode>()) Mode->TogglePause();
}
void ADBCharacter::ToggleBuild()
{
    SuspendCombatInput();
    if (ADBGameMode* Mode = GetWorld()->GetAuthGameMode<ADBGameMode>()) Mode->ToggleBuild();
}

FVector ADBCharacter::GetAimDirection() const
{
    return ViewCamera ? ViewCamera->GetForwardVector() : GetControlRotation().Vector();
}

FVector ADBCharacter::GetMuzzleLocation() const
{
    return WeaponRoot ? WeaponRoot->GetComponentTransform().TransformPosition(FVector(64.f, 0.f, 3.f)) : GetActorLocation() + GetAimDirection() * 85.f;
}

FVector ADBCharacter::FindAimPoint() const
{
    const FVector Start = ViewCamera->GetComponentLocation();
    const FVector End = Start + GetAimDirection() * 16000.f;
    FHitResult Hit;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBAim), false, this);
    return GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params) ? Hit.ImpactPoint : End;
}

bool ADBCharacter::HitEnemy(ADBEnemy* Enemy, const FDBHit& Hit)
{
    if (!IsValid(Enemy) || Enemy->bDead || !CanAct()) return false;
    Enemy->ApplyCombatHit(Hit);
    HitMarkerTime = .13f;
    bLastHitKilled = Enemy->bDead;
    if (Enemy->bDead) HitMarkerTime = .24f;
    const int32 FractureRank = GetUpgradeRank(FractureId);
    if (!Hit.bSecondary && CanAct() && FractureRank >= 2
        && (Hit.bImpact || (FractureRank >= 3 && Hit.Element != EDBElement::Neutral)))
    {
        FDBHit Aftershock = Hit;
        Aftershock.Damage = Hit.bImpact ? Hit.Damage * .4f : 10.f;
        Aftershock.bSecondary = true;
        Aftershock.bImpact = false;
        Aftershock.bStormfracture = false;
        ApplyAreaHit(Enemy->GetActorLocation(), 330.f, Aftershock, Enemy, 3);
    }
    return true;
}

ADBEnemy* ADBCharacter::TraceAttack(const FVector& AimPoint, const FDBHit& Hit, float BeamWidth)
{
    if (!CanAct()) return nullptr;
    const FVector Muzzle = GetMuzzleLocation();
    const FVector CameraLocation = ViewCamera->GetComponentLocation();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBPulse), false, this);
    FHitResult Impact;
    FVector End = AimPoint;
    // A weapon poked through cover cannot fire from the far side of that cover.
    bool bBlocked = GetWorld()->LineTraceSingleByChannel(Impact, CameraLocation, Muzzle, ECC_Visibility, Params);
    if (!bBlocked)
        bBlocked = GetWorld()->LineTraceSingleByChannel(Impact, Muzzle, AimPoint + (AimPoint - Muzzle).GetSafeNormal() * 4.f, ECC_Visibility, Params);
    ADBEnemy* Enemy = nullptr;
    if (bBlocked)
    {
        End = Impact.ImpactPoint;
        Enemy = Cast<ADBEnemy>(Impact.GetActor());
        FDBHit ActualHit = Hit;
        ActualHit.Source = Muzzle;
        ActualHit.Direction = (End - Muzzle).GetSafeNormal();
        ActualHit.InstigatorActor = this;
        if (Enemy) HitEnemy(Enemy, ActualHit);
        DrawSpark(End, Hit.Element, Enemy ? 10.f : 5.f);
    }
    DrawBeam(Muzzle, End, Hit.Element, BeamWidth);
    return Enemy;
}

void ADBCharacter::FirePulse()
{
    if (!CanAct() || FireCooldown > 0.f || bOverheated) return;
    if (bGuarding && GuardEnergy < 5.f)
    {
        SetCombatMessage(TEXT("Release guard to fire and recover"), .8f);
        FireCooldown = .25f;
        return;
    }
    const FVector AimPoint = FindAimPoint();
    const int32 SplitRank = GetUpgradeRank(SplitId);
    const int32 CoreRank = GetUpgradeRank(ElementId(CurrentElement));
    const bool bEmpowered = EmpoweredShots > 0;
    if (bEmpowered) --EmpoweredShots;
    ++PulseSequence;

    FDBHit Hit;
    Hit.Damage = bEmpowered ? 48.f : 27.f;
    Hit.Element = CurrentElement;
    Hit.bImpact = bEmpowered || (CurrentElement == EDBElement::Frost && CoreRank >= 3 && PulseSequence % 3 == 0);
    Hit.bStormfracture = HasUpgrade(FractureId);
    Hit.Source = GetMuzzleLocation();
    Hit.InstigatorActor = this;
    ADBEnemy* CentreEnemy = TraceAttack(AimPoint, Hit, bEmpowered ? 4.f : 2.f);

    if (SplitRank > 0 && CanAct())
    {
        // Finite independent side rays retain accuracy/positioning choices; no automatic homing.
        const int32 SideCount = SplitRank >= 2 ? 4 : 2;
        const FVector Forward = (AimPoint - ViewCamera->GetComponentLocation()).GetSafeNormal();
        const FVector Right = ViewCamera->GetRightVector();
        const FVector Up = ViewCamera->GetUpVector();
        const float Spread = (SplitRank >= 3 && bGuarding) ? .019f : .061f;
        FDBHit Fragment = Hit;
        Fragment.Damage *= .54f;
        Fragment.bSecondary = true;
        for (int32 Index = 0; Index < SideCount && CanAct(); ++Index)
        {
            const FVector Offset = Index < 2 ? Right * (Index == 0 ? -Spread : Spread) : Up * (Index == 2 ? -Spread : Spread);
            TraceAttack(ViewCamera->GetComponentLocation() + (Forward + Offset).GetSafeNormal() * 16000.f, Fragment, 1.25f);
        }
    }
    const int32 EchoRank = GetUpgradeRank(EchoId);
    if (EchoRank > 0 && CanAct())
    {
        FEchoShot Echo;
        Echo.Delay = .19f;
        Echo.AimPoint = AimPoint;
        Echo.Target = CentreEnemy;
        Echo.Element = CurrentElement;
        Echo.Damage = Hit.Damage * .62f;
        Echo.bTrack = EchoRank >= 2;
        Echo.bStormfracture = Hit.bStormfracture;
        if (EchoShots.Num() < 16) EchoShots.Add(Echo);
        if (EchoRank >= 3 && EchoShots.Num() < 16)
        {
            Echo.Delay = .38f;
            Echo.Damage *= .8f;
            EchoShots.Add(Echo);
        }
    }
    if (CentreEnemy && CoreRank >= 2 && PulseSequence % 3 == 0 && CanAct())
    {
        FDBHit Bloom = Hit;
        Bloom.bSecondary = true;
        Bloom.bImpact = false;
        Bloom.Damage = CurrentElement == EDBElement::Ember ? 18.f : 11.f;
        ApplyAreaHit(CentreEnemy->GetActorLocation(), 260.f, Bloom, CentreEnemy, 4);
        DrawSpark(CentreEnemy->GetActorLocation(), CurrentElement, 36.f);
    }
    if (CurrentElement == EDBElement::Storm && CoreRank >= 3 && CentreEnemy)
        GuardEnergy = FMath::Min(MaxGuardEnergy, GuardEnergy + 4.f);
    if (CurrentElement == EDBElement::Ember && CoreRank >= 3 && CentreEnemy && CentreEnemy->bDead)
        Heat = FMath::Max(0.f, Heat - 22.f);

    Heat = FMath::Min(MaxHeat, Heat + (SplitRank > 0 ? 16.5f : 11.5f) + (bGuarding ? 3.f : 0.f));
    if (Heat >= MaxHeat)
    {
        bOverheated = true;
        SetCombatMessage(TEXT("CORE OVERHEATED - guard or reposition"), 2.f);
        PlayCombatSound(GuardSound, .8f, .58f);
    }
    if (bGuarding) GuardEnergy = FMath::Max(0.f, GuardEnergy - 4.f);
    FireCooldown = bGuarding ? .33f : .245f;
    SinceFired = 0.f;
    Recoil = FMath::Min(1.8f, Recoil + (bEmpowered ? 1.25f : .8f));
    AddControllerPitchInput(bGuarding ? -.08f : -.16f);
    PlayCombatSound(AttackSound, bEmpowered ? .85f : .6f, bEmpowered ? .82f : 1.f + FMath::Sin(PulseSequence * 2.1f) * .025f);
}

void ADBCharacter::UpdateEchoes(float DeltaSeconds)
{
    for (int32 Index = EchoShots.Num() - 1; Index >= 0; --Index)
    {
        EchoShots[Index].Delay -= DeltaSeconds;
        if (EchoShots[Index].Delay > 0.f) continue;
        const FEchoShot Echo = EchoShots[Index];
        EchoShots.RemoveAtSwap(Index);
        const FVector TargetPoint = Echo.bTrack && Echo.Target.IsValid() && !Echo.Target->bDead
            ? Echo.Target->GetActorLocation() + FVector(0.f, 0.f, 35.f) : Echo.AimPoint;
        FDBHit Hit;
        Hit.Damage = Echo.Damage;
        Hit.Element = Echo.Element;
        Hit.bSecondary = true;
        Hit.bStormfracture = Echo.bStormfracture;
        Hit.InstigatorActor = this;
        TraceAttack(TargetPoint, Hit, 1.1f);
        // Killing the last enemy can open a reward and clear the remaining echoes.
        if (!CanAct()) return;
    }
}

void ADBCharacter::UseSpecial()
{
    if (!CanAct() || SpecialCooldown > 0.f) return;
    if (HasUpgrade(MirrorId) && StoredShots > 0 && !bGuarding) Counterfire();
    else ShieldImpact();
}

void ADBCharacter::Counterfire()
{
    if (!CanAct() || StoredShots <= 0) return;
    const float Payload = StoredDamage.Num() ? StoredDamage[0] : 18.f;
    if (StoredDamage.Num()) StoredDamage.RemoveAt(0);
    StoredShots = FMath::Max(0, StoredShots - 1);
    FDBHit Hit;
    Hit.Damage = 43.f + FMath::Min(Payload, 65.f) * 1.8f;
    Hit.Element = CurrentElement;
    Hit.bImpact = true;
    Hit.bStormfracture = HasUpgrade(FractureId);
    Hit.InstigatorActor = this;
    ADBEnemy* Target = TraceAttack(FindAimPoint(), Hit, 7.f);
    if (Target && GetUpgradeRank(MirrorId) >= 3 && CanAct())
    {
        FDBHit Ricochet = Hit;
        Ricochet.bSecondary = true;
        Ricochet.Damage *= .75f;
        ApplyAreaHit(Target->GetActorLocation(), 650.f, Ricochet, Target, 2);
    }
    Heat = FMath::Max(0.f, Heat - 17.f);
    Recoil = 1.8f;
    SpecialCooldown = .42f;
    ImpactPose = .35f;
    PlayCombatSound(ImpactSound, .9f, 1.25f);
    SetCombatMessage(TEXT("RETURNED FORCE"), .7f);
}

void ADBCharacter::ShieldImpact()
{
    const int32 RamRank = GetUpgradeRank(RamId);
    const bool bChargedImpact = GetUpgradeRank(CapacitorId) >= 3 && EmpoweredShots > 0;
    if (bChargedImpact) --EmpoweredShots;
    ImpactPose = 1.f;
    SpecialCooldown = RamRank >= 2 ? 1.9f : 2.7f;
    PlayCombatSound(ImpactSound, .8f, RamRank > 0 ? .8f : 1.f);
    FDBHit Hit;
    Hit.Damage = bChargedImpact ? 92.f : (RamRank > 0 ? 60.f : 36.f);
    RushDamage = Hit.Damage;
    Hit.Element = CurrentElement;
    Hit.bImpact = true;
    Hit.bStormfracture = HasUpgrade(FractureId);
    Hit.Source = GetActorLocation();
    Hit.Direction = GetAimDirection();
    Hit.InstigatorActor = this;

    for (TActorIterator<ADBEnemy> It(GetWorld()); It && CanAct(); ++It)
    {
        ADBEnemy* Enemy = *It;
        const FVector Delta = Enemy->GetActorLocation() - GetActorLocation();
        if (!Enemy->bDead && Delta.SizeSquared() <= FMath::Square(250.f)
            && FVector::DotProduct(Delta.GetSafeNormal(), GetAimDirection()) > .3f
            && HasLineOfSight(ViewCamera->GetComponentLocation(), Enemy))
        {
            if (HitEnemy(Enemy, Hit))
            {
                RushVictims.Add(Enemy);
                DrawSpark(Enemy->GetActorLocation(), CurrentElement, 32.f);
                if (RamRank >= 2) GuardEnergy = FMath::Min(MaxGuardEnergy, GuardEnergy + 14.f);
            }
        }
    }
    if (RamRank > 0 && CanAct())
    {
        RushDirection = GetAimDirection();
        RushDirection.Z = 0.f;
        RushDirection.Normalize();
        bRushActive = true;
        RushTime = .3f;
        DashTime = .28f;
        InvulnerabilityTime = .13f;
        GetCharacterMovement()->GroundFriction = 0.f;
        LaunchCharacter(RushDirection * 1520.f + FVector(0.f, 0.f, 35.f), true, false);
        Heat = FMath::Max(0.f, Heat - 14.f);
    }
    else RushVictims.Reset();
}

void ADBCharacter::ResolveRush()
{
    if (!CanAct()) return;
    FDBHit Hit;
    Hit.Damage = RushDamage;
    Hit.Element = CurrentElement;
    Hit.bImpact = true;
    Hit.bStormfracture = HasUpgrade(FractureId);
    Hit.Source = GetActorLocation();
    Hit.Direction = RushDirection;
    Hit.InstigatorActor = this;
    const bool bWide = GetUpgradeRank(RamId) >= 3;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It && CanAct(); ++It)
    {
        ADBEnemy* Enemy = *It;
        if (Enemy->bDead || RushVictims.Contains(Enemy)) continue;
        const FVector Delta = Enemy->GetActorLocation() - GetActorLocation();
        if (Delta.SizeSquared() > FMath::Square(bWide ? 320.f : 205.f)) continue;
        if (FVector::DotProduct(Delta.GetSafeNormal(), RushDirection) < (bWide ? -.15f : .1f)) continue;
        if (!HasLineOfSight(ViewCamera->GetComponentLocation(), Enemy)) continue;
        RushVictims.Add(Enemy);
        if (HitEnemy(Enemy, Hit))
        {
            DrawSpark(Enemy->GetActorLocation(), CurrentElement, bWide ? 48.f : 30.f);
            if (GetUpgradeRank(RamId) >= 2)
                GuardEnergy = FMath::Min(MaxGuardEnergy, GuardEnergy + 14.f);
        }
    }
}

void ADBCharacter::Dash()
{
    if (!CanAct() || DashCooldown > 0.f) return;
    FVector Direction = GetLastMovementInputVector();
    if (Direction.IsNearlyZero()) Direction = GetAimDirection();
    Direction.Z = 0.f;
    Direction.Normalize();
    DashCooldown = 1.45f;
    DashTime = .21f;
    InvulnerabilityTime = .13f;
    GetCharacterMovement()->GroundFriction = 0.f;
    LaunchCharacter(Direction * 1330.f + FVector(0.f, 0.f, 20.f), true, false);
    PlayCombatSound(DashSound, .65f);
}

void ADBCharacter::CaptureEnergy(float Damage)
{
    if (!HasUpgrade(MirrorId)) return;
    const int32 Capacity = 1 + GetUpgradeRank(MirrorId) + (GetUpgradeRank(CapacitorId) >= 2 ? 2 : 0);
    if (StoredShots >= Capacity) return;
    StoredDamage.Add(Damage);
    ++StoredShots;
}

void ADBCharacter::ReceiveAttack(float Damage, FVector Source, bool bUnblockable, AActor* Attacker)
{
    if (!CanAct() || Damage <= 0.f || InvulnerabilityTime > 0.f) return;
    const FVector ToSource = (Source - ViewCamera->GetComponentLocation()).GetSafeNormal();
    const bool bFacing = FVector::DotProduct(GetAimDirection(), ToSource) >= .38f;
    const bool bCanBlock = bGuarding && GuardBreakTime <= 0.f && bFacing && !bUnblockable;
    SinceDamaged = 0.f;
    if (bCanBlock)
    {
        const float ParryWindow = .2f + (GetUpgradeRank(MirrorId) >= 2 ? .045f : 0.f);
        if (SinceGuarded <= ParryWindow)
        {
            // Consume this guard's perfect window: a shotgun volley is one timed success,
            // with its remaining pellets taking the ordinary held-guard path.
            SinceGuarded = ParryWindow + 1.f;
            GuardEnergy = FMath::Min(MaxGuardEnergy, GuardEnergy + 13.f);
            Heat = FMath::Max(0.f, Heat - (HasUpgrade(CapacitorId) ? 35.f : 12.f));
            CaptureEnergy(Damage);
            if (HasUpgrade(CapacitorId)) EmpoweredShots = GetUpgradeRank(CapacitorId) >= 2 ? 5 : 3;
            ParryFlashTime = .32f;
            Recoil = -.8f;
            SetCombatMessage(HasUpgrade(MirrorId) ? TEXT("PERFECT GUARD - energy captured [Q]") : TEXT("PERFECT GUARD"), 1.2f);
            PlayCombatSound(ParrySound, .9f);
            if (ADBEnemy* Enemy = Cast<ADBEnemy>(Attacker))
            {
                if (FVector::DistSquared(Enemy->GetActorLocation(), GetActorLocation()) < FMath::Square(340.f))
                {
                    FDBHit Deflect;
                    Deflect.Damage = 12.f;
                    Deflect.Element = CurrentElement;
                    Deflect.bImpact = true;
                    Deflect.bSecondary = true;
                    Deflect.Source = GetActorLocation();
                    Deflect.Direction = GetAimDirection();
                    Deflect.InstigatorActor = this;
                    HitEnemy(Enemy, Deflect);
                }
            }
            return;
        }
        const float GuardCost = Damage * 1.12f + 5.f;
        if (GuardEnergy >= GuardCost)
        {
            GuardEnergy -= GuardCost;
            const FVector AttackOrigin = IsValid(Attacker) ? Attacker->GetActorLocation() : Source;
            if (GetUpgradeRank(MirrorId) >= 2 && FVector::DistSquared(AttackOrigin, GetActorLocation()) > FMath::Square(320.f))
                CaptureEnergy(Damage);
            Recoil = FMath::Min(1.5f, Recoil + .65f);
            PlayCombatSound(GuardSound, .85f, .92f);
            return;
        }
        const float Absorbed = GuardEnergy / GuardCost;
        Damage *= 1.f - Absorbed;
        GuardEnergy = 0.f;
        GuardBreakTime = 1.7f;
        bGuarding = false;
        SetCombatMessage(TEXT("GUARD BROKEN - keep moving"), 2.f);
    }
    else if (bGuarding && bUnblockable)
        SetCombatMessage(TEXT("HEAVY ATTACK - evade the red tell"), 1.5f);

    Health = FMath::Max(0.f, Health - Damage);
    HurtFlashTime = .4f;
    Recoil = FMath::Min(2.f, Recoil + .9f);
    PlayCombatSound(HurtSound, .8f);
    if (Health <= 0.f)
    {
        bDead = true;
        SuspendCombatInput();
        GetCharacterMovement()->StopMovementImmediately();
        // No local death/respawn timer. The mode owns the persistent defeat/retry screen.
        if (ADBGameMode* Mode = GetWorld()->GetAuthGameMode<ADBGameMode>()) Mode->NotifyPlayerDied();
    }
}

bool ADBCharacter::HasLineOfSight(const FVector& From, ADBEnemy* Enemy) const
{
    if (!IsValid(Enemy)) return false;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBAreaSight), false, this);
    FHitResult Obstruction;
    if (!GetWorld()->LineTraceSingleByChannel(Obstruction, From, Enemy->GetActorLocation(), ECC_Visibility, Params)) return true;
    return Obstruction.GetActor() == Enemy;
}

void ADBCharacter::ApplyAreaHit(const FVector& Centre, float Radius, const FDBHit& Hit, ADBEnemy* Ignore, int32 MaxTargets)
{
    TArray<ADBEnemy*> Candidates;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It)
    {
        if (!It->bDead && *It != Ignore && FVector::DistSquared(It->GetActorLocation(), Centre) < FMath::Square(Radius)) Candidates.Add(*It);
    }
    Candidates.Sort([Centre](const ADBEnemy& A, const ADBEnemy& B) {
        return FVector::DistSquared(A.GetActorLocation(), Centre) < FVector::DistSquared(B.GetActorLocation(), Centre);
    });
    int32 Applied = 0;
    for (ADBEnemy* Enemy : Candidates)
    {
        if (!CanAct() || Applied >= MaxTargets) break;
        FCollisionQueryParams Params(SCENE_QUERY_STAT(DBBloomSight), false, this);
        if (Ignore) Params.AddIgnoredActor(Ignore);
        FHitResult Obstruction;
        if (GetWorld()->LineTraceSingleByChannel(Obstruction, Centre, Enemy->GetActorLocation(), ECC_Visibility, Params)
            && Obstruction.GetActor() != Enemy) continue;
        FDBHit Actual = Hit;
        Actual.Source = Centre;
        Actual.Direction = (Enemy->GetActorLocation() - Centre).GetSafeNormal();
        Actual.InstigatorActor = this;
        if (HitEnemy(Enemy, Actual))
        {
            ++Applied;
            DrawBeam(Centre, Enemy->GetActorLocation(), Hit.Element, 2.8f, .12f);
        }
    }
}

bool ADBCharacter::HasUpgrade(FName Id) const { return GetUpgradeRank(Id) > 0; }

int32 ADBCharacter::GetUpgradeRank(FName Id) const
{
    const int32* Rank = Upgrades.Find(Id);
    return Rank ? FMath::Clamp(*Rank, 0, 3) : 0;
}

void ADBCharacter::ApplyUpgrade(FName Id)
{
    const bool bKnown = Id == MirrorId || Id == RamId || Id == EchoId || Id == FrostId || Id == EmberId
        || Id == StormId || Id == FractureId || Id == SplitId || Id == CapacitorId;
    if (!bKnown) return;
    const int32 Rank = GetUpgradeRank(Id);
    if (Rank >= 3) return;
    Upgrades.Add(Id, Rank + 1);
    if (Id == FrostId) CurrentElement = EDBElement::Frost;
    else if (Id == EmberId) CurrentElement = EDBElement::Ember;
    else if (Id == StormId) CurrentElement = EDBElement::Storm;
    else if (Id == CapacitorId)
    {
        EmpoweredShots = Rank >= 1 ? 5 : 3;
        Heat = 0.f;
        bOverheated = false;
    }
    EquipPose = 1.f;
    SetCombatMessage(GetUpgradeDescription(Id, Rank + 1), 4.f);
    RefreshEquipmentVisuals();
    PlayCombatSound(EquipSound, .8f);
}

void ADBCharacter::CycleElement()
{
    if (!CanAct()) return;
    const EDBElement Elements[] = { EDBElement::Neutral, EDBElement::Frost, EDBElement::Storm, EDBElement::Ember };
    int32 Current = 0;
    for (int32 Index = 0; Index < 4; ++Index) if (Elements[Index] == CurrentElement) Current = Index;
    for (int32 Offset = 1; Offset <= 4; ++Offset)
    {
        const EDBElement Next = Elements[(Current + Offset) % 4];
        if (Next == EDBElement::Neutral || HasUpgrade(ElementId(Next)))
        {
            CurrentElement = Next;
            EquipPose = .45f;
            RefreshEquipmentVisuals();
            SetCombatMessage(GetElementLabel() + TEXT(" core active"), 1.2f);
            PlayCombatSound(EquipSound, .38f, 1.3f);
            break;
        }
    }
}

FString ADBCharacter::GetElementLabel() const
{
    switch (CurrentElement)
    {
    case EDBElement::Frost: return TEXT("FROST");
    case EDBElement::Storm: return TEXT("STORM");
    case EDBElement::Ember: return TEXT("EMBER");
    default: return TEXT("PULSE");
    }
}

FString ADBCharacter::GetSpecialLabel() const
{
    if (HasUpgrade(MirrorId) && StoredShots > 0 && !bGuarding) return TEXT("Q  RETURN FORCE");
    return HasUpgrade(RamId) ? TEXT("Q  RAM RUSH") : TEXT("Q  SHIELD IMPACT");
}

FString ADBCharacter::GetUpgradeDescription(FName Id, int32 Rank)
{
    Rank = FMath::Clamp(Rank, 1, 3);
    if (Id == MirrorId)
        return Rank == 1 ? TEXT("Perfect guards store 2 attacks. Q returns one; RMB+Q keeps your impact.")
            : Rank == 2 ? TEXT("Held ranged blocks also capture. 3 stored attacks; wider perfect-guard window.")
            : TEXT("Returned force ricochets to 2 nearby targets. Store 4 attacks.");
    if (Id == RamId)
        return Rank == 1 ? TEXT("Impact becomes a rushing breach. Strike foes in your path; vent weapon heat.")
            : Rank == 2 ? TEXT("Rushing hits restore guard energy. Ram recovers faster.")
            : TEXT("The rushing shield throws a wide shock front that catches flanking enemies.");
    if (Id == EchoId)
        return Rank == 1 ? TEXT("Pulses leave a delayed second shot at the aimed point.")
            : Rank == 2 ? TEXT("An echo tracks its original target, unless cover blocks the return shot.")
            : TEXT("Every pulse leaves two delayed echoes.");
    if (Id == FrostId)
        return Rank == 1 ? TEXT("Pulses chill and eventually freeze. Impact shatters chilled enemies. R changes cores.")
            : Rank == 2 ? TEXT("Every third pulse spreads frost to nearby enemies.")
            : TEXT("Every third frost pulse also shatters: build chill, then break it at range.");
    if (Id == EmberId)
        return Rank == 1 ? TEXT("Pulses ignite enemies. Leave them burning while you reposition. R changes cores.")
            : Rank == 2 ? TEXT("Every third pulse blooms into nearby targets, spreading flame.")
            : TEXT("Direct ember kills vent heat, sustaining an aggressive burning build.");
    if (Id == StormId)
        return Rank == 1 ? TEXT("Mark enemies with storm; strike again to arc to nearby targets. R changes cores.")
            : Rank == 2 ? TEXT("Every third pulse seeds nearby enemies with storm.")
            : TEXT("Storm hits return guard energy, supporting fire through raised defense.");
    if (Id == FractureId)
        return Rank == 1 ? TEXT("Elemental hits consume existing chill and storm marks in a finite chain burst.")
            : Rank == 2 ? TEXT("Impacts and returned force release an elemental aftershock to nearby foes.")
            : TEXT("Every direct elemental shot also spreads an additional bloom to nearby foes.");
    if (Id == SplitId)
        return Rank == 1 ? TEXT("Add two angled pulse fragments. Close range concentrates the spread; more heat.")
            : Rank == 2 ? TEXT("Add two vertical fragments as well as the side fragments.")
            : TEXT("Raised guard focuses all fragments into a tight precision cluster.");
    if (Id == CapacitorId)
        return Rank == 1 ? TEXT("Perfect guards supercharge 3 pulses into impacts and vent heat. Guard recovers faster.")
            : Rank == 2 ? TEXT("A perfect guard supercharges 5 pulses. Mirror storage gains 2 slots.")
            : TEXT("Supercharge also powers a devastating close impact; all earlier effects remain.");
    return TEXT("");
}

void ADBCharacter::OnRunReset()
{
    SuspendCombatInput();
    Health = MaxHealth;
    GuardEnergy = MaxGuardEnergy;
    Heat = 0.f;
    bDead = false;
    bOverheated = false;
    bWasMenuBlocked = false;
    StoredDamage.Reset();
    StoredShots = 0;
    EmpoweredShots = 0;
    FireCooldown = GuardRaiseCooldown = DashCooldown = SpecialCooldown = GuardBreakTime = 0.f;
    HitMarkerTime = HurtFlashTime = ParryFlashTime = MessageTime = 0.f;
    InvulnerabilityTime = DashTime = RushTime = 0.f;
    SinceFired = SinceGuarded = SinceDamaged = 10.f;
    GuardBlend = Recoil = ImpactPose = EquipPose = 0.f;
    LookSwayX = LookSwayY = BobPhase = 0.f;
    PulseSequence = 0;
    LastCombatMessage.Empty();
    for (UStaticMeshComponent* Effect : CombatEffects) if (IsValid(Effect)) Effect->DestroyComponent();
    CombatEffects.Reset();
    EffectLife.Reset();
    GetCharacterMovement()->StopMovementImmediately();
    GetCharacterMovement()->SetMovementMode(MOVE_Walking);
    GetCharacterMovement()->GroundFriction = 8.f;
    GetCharacterMovement()->MaxWalkSpeed = 590.f;
    RefreshEquipmentVisuals();
    ViewCamera->SetFieldOfView(94.f);
    UpdateWeapon(0.f);
}

UMaterialInterface* ADBCharacter::GetElementMaterial(EDBElement Element) const
{
    switch (Element)
    {
    case EDBElement::Frost: return FrostMaterial ? FrostMaterial.Get() : CoreMaterial.Get();
    case EDBElement::Storm: return StormMaterial ? StormMaterial.Get() : CoreMaterial.Get();
    case EDBElement::Ember: return EmberMaterial ? EmberMaterial.Get() : CoreMaterial.Get();
    default: return CoreMaterial.Get();
    }
}

void ADBCharacter::RefreshEquipmentVisuals()
{
    if (!WeaponBody) return;
    if (CurrentElement != EDBElement::Neutral && !HasUpgrade(ElementId(CurrentElement))) CurrentElement = EDBElement::Neutral;
    if (GetElementMaterial(CurrentElement))
    {
        const int32 NamedSlot = WeaponCore->GetMaterialIndex(TEXT("M_Core"));
        const int32 GlowSlot = NamedSlot != INDEX_NONE ? NamedSlot : (WeaponCore->GetNumMaterials() > 1 ? 1 : 0);
        WeaponCore->SetMaterial(GlowSlot, GetElementMaterial(CurrentElement));
    }
    CoreLight->SetLightColor(ElementColor(CurrentElement));
    const FName Ids[] = { MirrorId, RamId, EchoId, FrostId, EmberId, StormId, FractureId, SplitId, CapacitorId };
    const FVector Locations[] = {
        FVector(25,-11,3), FVector(48,1,-8), FVector(8,0,12),
        FVector(22,-8,5), FVector(32,8,4), FVector(40,-8,4),
        FVector(15,9,5), FVector(55,0,3), FVector(9,0,-11)
    };
    const FVector Sizes[] = {
        FVector(18,12,3), FVector(28,5,6), FVector(10,10,10),
        FVector(5,5,8.5), FVector(5,5,8.5), FVector(5,5,8.5),
        FVector(7,7,8), FVector(9,14,14), FVector(12,12,12)
    };
    for (int32 Index = 0; Index < AttachmentParts.Num(); ++Index)
    {
        UStaticMeshComponent* Part = AttachmentParts[Index];
        const int32 Rank = GetUpgradeRank(Ids[Index]);
        Part->SetVisibility(Rank > 0);
        Part->SetRelativeLocation(Locations[Index]);
        Part->SetRelativeRotation(Index == 0 ? FRotator(0,0,50)
            : (Index >= 3 && Index <= 6 ? FRotator(-22.f,0.f,0.f) : FRotator::ZeroRotator));
        if (const UStaticMesh* PartMesh = Part->GetStaticMesh())
        {
            const FVector Size = PartMesh->GetBounds().BoxExtent * 2.f;
            const FVector Desired = Sizes[Index] * (1.f + .12f * FMath::Max(0, Rank - 1));
            Part->SetRelativeScale3D(FVector(Desired.X / FMath::Max(1.f, Size.X),
                Desired.Y / FMath::Max(1.f, Size.Y), Desired.Z / FMath::Max(1.f, Size.Z)));
        }
        UMaterialInterface* Material = BronzeMaterial;
        if (Ids[Index] == FrostId) Material = FrostMaterial;
        else if (Ids[Index] == EmberId) Material = EmberMaterial;
        else if (Ids[Index] == StormId || Ids[Index] == FractureId) Material = StormMaterial;
        else if (Ids[Index] == MirrorId || Ids[Index] == CapacitorId) Material = CoreMaterial;
        if (Material) Part->SetMaterial(0, Material);
    }
}

void ADBCharacter::UpdateWeapon(float DeltaSeconds)
{
    GuardBlend = FMath::FInterpTo(GuardBlend, bGuarding ? 1.f : 0.f, DeltaSeconds, bGuarding ? 17.f : 10.f);
    Recoil = FMath::FInterpTo(Recoil, 0.f, DeltaSeconds, 13.f);
    ImpactPose = FMath::FInterpTo(ImpactPose, 0.f, DeltaSeconds, 9.f);
    EquipPose = FMath::FInterpTo(EquipPose, 0.f, DeltaSeconds, 3.5f);
    LookSwayX = FMath::FInterpTo(LookSwayX, 0.f, DeltaSeconds, 10.f);
    LookSwayY = FMath::FInterpTo(LookSwayY, 0.f, DeltaSeconds, 10.f);
    const float SpeedFraction = FMath::Clamp(GetVelocity().Size2D() / 590.f, 0.f, 1.f);
    BobPhase += DeltaSeconds * FMath::Lerp(1.5f, 10.f, SpeedFraction);
    const float Bob = FMath::Sin(BobPhase) * .8f * SpeedFraction;
    const FVector Rest(50.f, 30.f, -32.f);
    const FVector Guard(47.f, 31.f, -33.f);
    WeaponRoot->SetRelativeLocation(FMath::Lerp(Rest, Guard, GuardBlend)
        + FVector(-Recoil * 5.f + ImpactPose * 17.f, -LookSwayX + Bob * .7f, Bob + LookSwayY - EquipPose * 9.f));
    WeaponRoot->SetRelativeRotation(FRotator(Recoil * 5.f - ImpactPose * 8.f - EquipPose * 13.f,
        -13.f + GuardBlend * 5.f, -8.f - GuardBlend * 6.f + Bob));
    WeaponCore->SetRelativeRotation(FRotator(0.f, 0.f, GetWorld()->GetTimeSeconds() * (bOverheated ? 90.f : 18.f)));
    CoreLight->SetIntensity((bOverheated ? 95.f : 40.f) + Recoil * 60.f + ParryFlashTime * 220.f);
    for (int32 Index = 0; Index < ShieldPlates.Num(); ++Index)
    {
        if (Index != 0) continue;
        const FVector Folded(14.f, 0.f, -10.f);
        const FVector Open(31.f, 0.f, 10.f);
        ShieldPlates[Index]->SetRelativeLocation(FMath::Lerp(Folded, Open, GuardBlend));
        ShieldPlates[Index]->SetRelativeRotation(FRotator((1.f - GuardBlend) * 85.f, 0.f, 0.f));
    }
    ViewCamera->SetFieldOfView(FMath::FInterpTo(ViewCamera->FieldOfView, DashTime > 0.f ? 100.f : 94.f, DeltaSeconds, 9.f));
}

void ADBCharacter::DrawBeam(const FVector& Start, const FVector& End, EDBElement Element, float Width, float Duration)
{
    if (!BeamMesh || CombatEffects.Num() >= 96) return;
    const FVector Delta = End - Start;
    if (Delta.SizeSquared() < 1.f) return;
    UStaticMeshComponent* Beam = NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(Beam);
    Beam->SetStaticMesh(BeamMesh);
    Beam->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Beam->SetCastShadow(false);
    Beam->bReceivesDecals = false;
    if (UMaterialInterface* Material = GetElementMaterial(Element)) Beam->SetMaterial(0, Material);
    Beam->RegisterComponent();
    Beam->SetWorldLocation((Start + End) * .5f);
    Beam->SetWorldRotation(Delta.Rotation());
    Beam->SetWorldScale3D(FVector(Delta.Size() / 100.f, Width / 100.f, Width / 100.f));
    CombatEffects.Add(Beam);
    EffectLife.Add(Duration);
}

void ADBCharacter::DrawSpark(const FVector& Location, EDBElement Element, float Size)
{
    if (!SparkMesh || CombatEffects.Num() >= 96) return;
    UStaticMeshComponent* Spark = NewObject<UStaticMeshComponent>(this);
    AddInstanceComponent(Spark);
    Spark->SetStaticMesh(SparkMesh);
    Spark->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Spark->SetCastShadow(false);
    if (UMaterialInterface* Material = GetElementMaterial(Element)) Spark->SetMaterial(0, Material);
    Spark->RegisterComponent();
    Spark->SetWorldLocation(Location);
    Spark->SetWorldScale3D(FVector(Size / 100.f));
    CombatEffects.Add(Spark);
    EffectLife.Add(.12f);
}

void ADBCharacter::UpdateEffects(float DeltaSeconds)
{
    for (int32 Index = CombatEffects.Num() - 1; Index >= 0; --Index)
    {
        EffectLife[Index] -= DeltaSeconds;
        if (EffectLife[Index] <= 0.f)
        {
            if (IsValid(CombatEffects[Index])) CombatEffects[Index]->DestroyComponent();
            CombatEffects.RemoveAtSwap(Index);
            EffectLife.RemoveAtSwap(Index);
        }
    }
}

void ADBCharacter::SetCombatMessage(const FString& Text, float Duration)
{
    LastCombatMessage = Text;
    MessageTime = Duration;
}

void ADBCharacter::PlayCombatSound(USoundBase* Sound, float Volume, float Pitch)
{
    if (Sound) UGameplayStatics::PlaySound2D(this, Sound, Volume, Pitch);
}
