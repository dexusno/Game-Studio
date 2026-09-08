#include "DBCharacter.h"
#include "DBThrownShield.h"

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
    const FName AnchorId(TEXT("Anchor"));

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
    WeaponRoot->SetRelativeLocation(FVector(85.f, -52.f, -38.f));
    WeaponRoot->SetRelativeRotation(FRotator(-8.f, -34.f, -12.f));

    Forearm = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("CyborgForearm"));
    Forearm->SetupAttachment(WeaponRoot);
    Forearm->SetRelativeLocation(FVector(-12.f, 0.f, -12.f));
    PrepareViewMesh(Forearm);
    WeaponBody = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponBody"));
    WeaponBody->SetupAttachment(WeaponRoot);
    PrepareViewMesh(WeaponBody);
    WeaponBody->SetHiddenInGame(true);
    WeaponCore = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("WeaponCore"));
    WeaponCore->SetupAttachment(WeaponRoot);
    WeaponCore->SetRelativeLocation(FVector(-6.f, 0.f, 0.f));
    PrepareViewMesh(WeaponCore);

    for (int32 Index = 0; Index < 1; ++Index)
    {
        UStaticMeshComponent* Plate = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ShieldPetal%d"), Index));
        Plate->SetupAttachment(WeaponRoot);
        PrepareViewMesh(Plate);
        ShieldPlates.Add(Plate);
    }
    for (int32 Index = 0; Index < 10; ++Index)
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

    UStaticMesh* ArmMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Forearm.SM_Forearm"));
    UStaticMesh* CoreMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Core.SM_Core"));
    UStaticMesh* PlateMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_ShieldPlate.SM_ShieldPlate"));
    UStaticMesh* CrystalMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Crystal.SM_Crystal"));
    // Keep the legacy component for callers, but there is no gun body in this version.
    WeaponBody->SetStaticMesh(nullptr);
    Forearm->SetStaticMesh(ArmMesh ? ArmMesh : BeamMesh.Get());
    WeaponCore->SetStaticMesh(CoreMesh ? CoreMesh : SparkMesh.Get());
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
    CatchSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Catch.S_Catch"));
    RecallSound = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/S_Recall.S_Recall"));
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
    AttackRecovery = FMath::Max(0.f, AttackRecovery - DeltaSeconds);
    if (AttackRecovery <= 0.f) bStrikePoseActive = false;
    GuardRaiseCooldown = FMath::Max(0.f, GuardRaiseCooldown - DeltaSeconds);
    DashCooldown = FMath::Max(0.f, DashCooldown - DeltaSeconds);
    SpecialCooldown = FMath::Max(0.f, SpecialCooldown - DeltaSeconds);
    GuardBreakTime = FMath::Max(0.f, GuardBreakTime - DeltaSeconds);
    InvulnerabilityTime = FMath::Max(0.f, InvulnerabilityTime - DeltaSeconds);
    HitMarkerTime = FMath::Max(0.f, HitMarkerTime - DeltaSeconds);
    HurtFlashTime = FMath::Max(0.f, HurtFlashTime - DeltaSeconds);
    ParryFlashTime = FMath::Max(0.f, ParryFlashTime - DeltaSeconds);
    MessageTime = FMath::Max(0.f, MessageTime - DeltaSeconds);
    SinceGuarded += DeltaSeconds;
    SinceDamaged += DeltaSeconds;
    Heat = 0.f;
    bOverheated = false; // Legacy save/HUD compatibility: there is no gun heat cycle.
    if (IsShieldAway() && !IsValid(ThrownShield)) OnShieldCaught(true);

    if (bStrikePending)
    {
        StrikeDelay -= DeltaSeconds;
        if (StrikeDelay <= 0.f)
        {
            bStrikePending = false;
            ResolveRimStrike();
        }
    }
    if (!CanAct()) return;
    if (AttackRecovery <= 0.f && ShieldState == EDBShieldState::Held)
    {
        if (bStrikeBuffered)
        {
            bStrikeBuffered = false;
            StartRimStrike(false);
        }
        else if (bWantsFire)
        {
            ShieldState = EDBShieldState::Charging;
            ChargeHeld = 0.f;
        }
        else if (bWantsGuard && !bGuarding && GuardBreakTime <= 0.f && GuardEnergy >= 8.f)
        {
            // Holding guard through an attack restores ordinary protection, not a free parry.
            bGuarding = true;
            SinceGuarded = 1.f;
            GuardEnergy -= 3.f;
        }
    }
    if (ShieldState == EDBShieldState::Charging && bWantsFire)
    {
        ChargeHeld += DeltaSeconds;
        ThrowCharge = FMath::Clamp((ChargeHeld - .22f) / .65f, 0.f, 1.f);
    }
    if (bGuarding)
    {
        GuardEnergy = FMath::Max(0.f, GuardEnergy - 2.f * DeltaSeconds);
        if (GuardEnergy <= 0.f)
        {
            bGuarding = false;
            bWantsGuard = false;
            GuardBreakTime = 1.5f;
            SetCombatMessage(TEXT("GUARD BROKEN - dash to recover"));
        }
    }
    else if (SinceDamaged > .65f && GuardBreakTime <= 0.f)
        GuardEnergy = FMath::Min(MaxGuardEnergy, GuardEnergy + (HasUpgrade(CapacitorId) ? 27.f : 21.f) * DeltaSeconds);

    DashTime = FMath::Max(0.f, DashTime - DeltaSeconds);
    GetCharacterMovement()->GroundFriction = DashTime > 0.f ? 0.f : 8.f;
    GetCharacterMovement()->BrakingDecelerationFalling = DashTime > 0.f ? 0.f : 600.f;
    GetCharacterMovement()->MaxWalkSpeed = bGuarding ? 320.f : 590.f;
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
    bShieldReady = !IsShieldAway() && ShieldState == EDBShieldState::Held && AttackRecovery <= 0.f;
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
    if (IsShieldAway()) { RecallShield(); return; }
    if (AttackRecovery > .18f) return;
    bWantsFire = true;
    ChargeHeld = 0.f;
    ThrowCharge = 0.f;
    bGuarding = false;
    if (AttackRecovery <= 0.f) ShieldState = EDBShieldState::Charging;
}

void ADBCharacter::ReleaseFire()
{
    if (!bWantsFire) return;
    bWantsFire = false;
    if (!CanAct() || IsShieldAway()) return;
    if (AttackRecovery > 0.f)
    {
        bStrikeBuffered = true;
        return;
    }
    if (ChargeHeld >= .22f) LaunchShield();
    else StartRimStrike(false);
}

void ADBCharacter::PressGuard()
{
    if (!CanAct()) return;
    if (IsShieldAway())
    {
        SetCombatMessage(TEXT("SHIELD AWAY - evade / Q or LMB recalls"), 1.f);
        return;
    }
    bWantsGuard = true;
    if (AttackRecovery > 0.f || GuardBreakTime > 0.f || GuardRaiseCooldown > 0.f || GuardEnergy < 8.f || bGuarding) return;
    bWantsFire = false;
    ChargeHeld = ThrowCharge = 0.f;
    ShieldState = EDBShieldState::Held;
    bGuarding = true;
    SinceGuarded = 0.f;
    GuardRaiseCooldown = .38f;
    GuardEnergy -= 3.f;
    PlayCombatSound(GuardSound, .65f, .95f);
}

void ADBCharacter::ReleaseGuard()
{
    bWantsGuard = false;
    bGuarding = false;
}

void ADBCharacter::SuspendCombatInput()
{
    bWantsFire = false;
    bWantsGuard = false;
    bStrikeBuffered = false;
    bStrikePending = false;
    bGuarding = false;
    bRushActive = false;
    ChargeHeld = ThrowCharge = 0.f;
    if (ShieldState == EDBShieldState::Charging) ShieldState = EDBShieldState::Held;
    RushVictims.Reset();
    EchoShots.Reset();
    StopJumping();
    ConsumeMovementInputVector();
    // A launched shield stays where it is while paused; opening a menu cannot recover it.
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
    return GetShieldCatchLocation(); // Compatibility with older diagnostics; this is not a gun muzzle.
}

FVector ADBCharacter::GetShieldCatchLocation() const
{
    return WeaponRoot->GetComponentLocation();
}

bool ADBCharacter::IsShieldAway() const
{
    return ShieldState == EDBShieldState::Outbound || ShieldState == EDBShieldState::Lodged || ShieldState == EDBShieldState::Returning;
}

void ADBCharacter::RecallShield()
{
    if (!CanAct() || !IsShieldAway()) return;
    if (!IsValid(ThrownShield)) { OnShieldCaught(true); return; }
    if (ShieldState == EDBShieldState::Returning) return;
    ThrownShield->Recall();
    PlayCombatSound(RecallSound, .85f);
    SetCombatMessage(TEXT("RECALL - move to cut a new return line"), 1.2f);
}

void ADBCharacter::OnShieldCaught(bool bEmergency)
{
    ThrownShield = nullptr;
    ShieldState = EDBShieldState::Held;
    bShieldReady = false;
    AttackRecovery = .25f;
    bStrikePoseActive = false;
    bGuarding = false;
    CatchPose = 1.f;
    Recoil = 1.2f;
    ThrowCharge = 0.f;
    PlayCombatSound(CatchSound ? CatchSound.Get() : GuardSound.Get(), .9f, bEmergency ? .8f : 1.f);
    SetCombatMessage(bEmergency ? TEXT("RECONSTITUTED - return route obstructed") : TEXT("CAUGHT - shield ready"), 1.f);
    UpdateWeapon(0.f);
}

FString ADBCharacter::GetShieldStateLabel() const
{
    switch (ShieldState)
    {
    case EDBShieldState::Charging: return TEXT("CHARGING THROW - release LMB");
    case EDBShieldState::Outbound: return TEXT("SHIELD IN FLIGHT - guard unavailable");
    case EDBShieldState::Lodged: return HasUpgrade(AnchorId) ? TEXT("ANCHOR SET - Q recalls / cover has one facing") : TEXT("SHIELD AWAY - Q or LMB recalls");
    case EDBShieldState::Returning: return TEXT("RETURNING - reposition for the cut");
    default: return AttackRecovery > 0.f ? TEXT("COMMITTED - guard recovering") : (bGuarding ? TEXT("GUARDING") : TEXT("SHIELD IN HAND"));
    }
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


bool ADBCharacter::ApplyPhysicalShieldHit(ADBEnemy* Enemy, const FDBHit& Hit)
{
    if (!CanAct() || !IsValid(Enemy) || Enemy->bDead) return false;
    FDBHit Contact = Hit;
    const int32 PreviousChill = Enemy->ChillStacks;
    const bool bHadBurn = Enemy->BurnRemaining > 0.f;
    bool bReleasedForce = false;
    if (!Contact.bSecondary)
    {
        ++PulseSequence;
        if (StoredShots > 0 && (HasUpgrade(MirrorId) || GetUpgradeRank(AnchorId) >= 2))
        {
            const float Force = StoredDamage.Num() ? StoredDamage[0] : 20.f;
            if (StoredDamage.Num()) StoredDamage.RemoveAt(0);
            --StoredShots;
            Contact.Damage += Force * 1.8f;
            bReleasedForce = true;
        }
        if (EmpoweredShots > 0)
        {
            --EmpoweredShots;
            Contact.Damage *= 1.75f;
            bReleasedForce = true;
            if (GetUpgradeRank(CapacitorId) >= 3) DashCooldown = 0.f;
        }
    }
    if (!HitEnemy(Enemy, Contact)) return false;
    DrawSpark(Enemy->GetActorLocation() + FVector(0,0,20), Contact.Element, bReleasedForce ? 38.f : 20.f);
    PlayCombatSound(ImpactSound, bReleasedForce ? 1.f : .78f, bReleasedForce ? .75f : .95f);
    if (bReleasedForce) SetCombatMessage(TEXT("STORED FORCE RELEASED"), .8f);
    if (Contact.bSecondary || !CanAct()) return true;

    if (bReleasedForce && GetUpgradeRank(MirrorId) >= 3)
    {
        FDBHit ForceArc = Contact;
        ForceArc.Damage *= .5f;
        ForceArc.bSecondary = true;
        ApplyAreaHit(Enemy->GetActorLocation(), 500.f, ForceArc, Enemy, 2);
        if (!CanAct()) return true;
    }

    if (GetUpgradeRank(RamId) >= 2) GuardEnergy = FMath::Min(MaxGuardEnergy, GuardEnergy + 9.f);
    if (CurrentElement == EDBElement::Storm && GetUpgradeRank(StormId) >= 3)
        GuardEnergy = FMath::Min(MaxGuardEnergy, GuardEnergy + 7.f);

    const int32 EchoRank = GetUpgradeRank(EchoId);
    if (EchoRank > 0 && EchoShots.Num() < 20)
    {
        FEchoShot Echo;
        Echo.Delay = .23f;
        Echo.AimPoint = Enemy->GetActorLocation() + FVector(0,0,15);
        Echo.Target = Enemy;
        Echo.Element = Contact.Element;
        Echo.Damage = Contact.Damage * .55f;
        Echo.bTrack = EchoRank >= 2;
        Echo.bStormfracture = Contact.bStormfracture;
        EchoShots.Add(Echo);
        if (EchoRank >= 3 && EchoShots.Num() < 20)
        {
            Echo.Delay = .46f;
            Echo.Damage *= .8f;
            EchoShots.Add(Echo);
        }
    }
    const int32 CoreRank = GetUpgradeRank(ElementId(CurrentElement));
    const bool bFrostShatter = CurrentElement == EDBElement::Frost && CoreRank >= 3 && PreviousChill > 0;
    const bool bEmberDeath = CurrentElement == EDBElement::Ember && CoreRank >= 3 && bHadBurn && Enemy->bDead;
    if ((CoreRank >= 2 && PulseSequence % 3 == 0) || bFrostShatter || bEmberDeath)
    {
        FDBHit Bloom = Contact;
        Bloom.bSecondary = true;
        Bloom.bImpact = false;
        Bloom.Damage = bEmberDeath ? 36.f : (bFrostShatter ? 25.f : 14.f);
        ApplyAreaHit(Enemy->GetActorLocation(), bEmberDeath ? 340.f : 270.f, Bloom, Enemy, 6);
    }
    return true;
}

void ADBCharacter::StartRimStrike(bool bHeavy)
{
    if (!CanAct() || IsShieldAway() || AttackRecovery > 0.f) return;
    ShieldState = EDBShieldState::Held;
    bShieldReady = false;
    bWantsFire = false;
    bGuarding = false;
    ThrowCharge = ChargeHeld = 0.f;
    bHeavyStrike = bHeavy;
    bStrikePoseActive = true;
    StrikeTotal = bHeavy ? .8f : .5f;
    AttackRecovery = StrikeTotal;
    StrikeDelay = bHeavy ? .28f : .18f;
    bStrikePending = true;
    RushVictims.Reset();
    PlayCombatSound(AttackSound, bHeavy ? .9f : .72f, bHeavy ? .68f : .9f);
}

void ADBCharacter::ResolveRimStrike()
{
    if (!CanAct() || IsShieldAway()) return;
    const FVector Eye = ViewCamera->GetComponentLocation();
    const FVector Forward = GetAimDirection();
    const FVector Right = ViewCamera->GetRightVector();
    const FVector Up = ViewCamera->GetUpVector();
    const FVector Start = Eye + Forward * 55.f - Up * 24.f + (bHeavyStrike ? FVector::ZeroVector : -Right * 68.f);
    const FVector End = Eye + Forward * (bHeavyStrike ? 195.f : 140.f) - Up * 20.f
        + (bHeavyStrike ? FVector::ZeroVector : Right * 65.f);
    TArray<FHitResult> Contacts;
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBShieldRim), false, this);
    FCollisionObjectQueryParams Objects;
    Objects.AddObjectTypesToQuery(ECC_Pawn);
    GetWorld()->SweepMultiByObjectType(Contacts, Start, End, FQuat::Identity, Objects,
        FCollisionShape::MakeSphere(bHeavyStrike ? 48.f : 38.f), Params);
    FDBHit Contact;
    Contact.Damage = bHeavyStrike ? (HasUpgrade(RamId) ? 90.f : 66.f) : 48.f;
    Contact.Element = CurrentElement;
    Contact.bImpact = true;
    Contact.bStormfracture = HasUpgrade(FractureId);
    Contact.Source = GetActorLocation();
    Contact.Direction = Forward;
    Contact.InstigatorActor = this;
    for (const FHitResult& Result : Contacts)
    {
        ADBEnemy* Enemy = Cast<ADBEnemy>(Result.GetActor());
        if (!Enemy || Enemy->bDead || RushVictims.Contains(Enemy) || !HasLineOfSight(Eye, Enemy)) continue;
        RushVictims.Add(Enemy);
        ApplyPhysicalShieldHit(Enemy, Contact);
        if (!CanAct()) return;
    }
    ImpactPose = 1.f;
    Recoil = .9f;
    if (bHeavyStrike && HasUpgrade(RamId))
    {
        RushDirection = Forward.GetSafeNormal2D();
        RushDamage = 90.f;
        RushTime = .24f;
        DashTime = .24f;
        InvulnerabilityTime = .09f;
        bRushActive = true;
        GetCharacterMovement()->GroundFriction = 0.f;
        LaunchCharacter(RushDirection * 1320.f + FVector(0,0,25), true, false);
    }
}

void ADBCharacter::LaunchShield()
{
    if (!CanAct() || IsShieldAway() || AttackRecovery > 0.f) return;
    const FVector Eye = ViewCamera->GetComponentLocation();
    FVector Origin = GetShieldCatchLocation();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(DBShieldLaunchRoom), false, this);
    FHitResult Cover;
    if (GetWorld()->SweepSingleByChannel(Cover, Eye, Origin, FQuat::Identity, ECC_Visibility,
        FCollisionShape::MakeSphere(20.f), Params))
        Origin = Cover.Location + Cover.Normal * 3.f;
    const FVector Direction = (FindAimPoint() - Origin).GetSafeNormal();
    FActorSpawnParameters Spawn;
    Spawn.Owner = this;
    Spawn.Instigator = this;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    ThrownShield = GetWorld()->SpawnActor<ADBThrownShield>(ADBThrownShield::StaticClass(), Origin, Direction.Rotation(), Spawn);
    if (!ThrownShield)
    {
        ShieldState = EDBShieldState::Held;
        ThrowCharge = 0.f;
        return;
    }
    const float Charge = ThrowCharge;
    ShieldState = EDBShieldState::Outbound;
    bShieldReady = false;
    bGuarding = false;
    bWantsGuard = false;
    bWantsFire = false;
    AttackRecovery = .3f;
    bStrikePoseActive = false;
    bStrikePending = false;
    ThrownShield->Initialize(this, Direction, Charge);
    ImpactPose = 1.f;
    ThrowCharge = ChargeHeld = 0.f;
    PlayCombatSound(AttackSound, .95f, .72f);
    SetCombatMessage(TEXT("LAUNCHED - no guard / Q recalls through your new position"), 1.5f);
    UpdateWeapon(0.f);
}


void ADBCharacter::UpdateEchoes(float DeltaSeconds)
{
    for (int32 Index = EchoShots.Num() - 1; Index >= 0; --Index)
    {
        EchoShots[Index].Delay -= DeltaSeconds;
        if (EchoShots[Index].Delay > 0.f) continue;
        const FEchoShot Echo = EchoShots[Index];
        EchoShots.RemoveAtSwap(Index);
        FVector Point = Echo.AimPoint;
        if (Echo.bTrack && Echo.Target.IsValid() && !Echo.Target->bDead
            && FVector::DistSquared(Echo.Target->GetActorLocation(), Point) < FMath::Square(240.f))
            Point = Echo.Target->GetActorLocation() + FVector(0,0,15);
        FDBHit Hit;
        Hit.Damage = Echo.Damage;
        Hit.Element = Echo.Element;
        Hit.bImpact = true;
        Hit.bSecondary = true;
        Hit.bStormfracture = Echo.bStormfracture;
        Hit.InstigatorActor = this;
        // A local repetition of physical contact, not another shot from the player's view.
        ApplyAreaHit(Point, 125.f, Hit, nullptr, 4);
        if (ShieldPlates.Num() && ShieldPlates[0]->GetStaticMesh() && CombatEffects.Num() < 96)
        {
            UStaticMeshComponent* Afterimage = NewObject<UStaticMeshComponent>(this);
            AddInstanceComponent(Afterimage);
            Afterimage->SetStaticMesh(ShieldPlates[0]->GetStaticMesh());
            Afterimage->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            Afterimage->SetCastShadow(false);
            if (UMaterialInterface* Material = GetElementMaterial(Echo.Element)) Afterimage->SetMaterial(0, Material);
            Afterimage->RegisterComponent();
            Afterimage->SetWorldLocation(Point);
            Afterimage->SetWorldRotation(FRotator(35.f, GetActorRotation().Yaw, 45.f));
            Afterimage->SetWorldScale3D(FVector(.7f));
            CombatEffects.Add(Afterimage);
            EffectLife.Add(.1f);
        }
        PlayCombatSound(ImpactSound, .38f, 1.4f);
        if (!CanAct()) return;
    }
}

void ADBCharacter::UseSpecial()
{
    if (!CanAct()) return;
    if (IsShieldAway()) { RecallShield(); return; }
    if (SpecialCooldown > 0.f || AttackRecovery > 0.f) return;
    ShieldImpact();
}

void ADBCharacter::ShieldImpact()
{
    StartRimStrike(true);
    SpecialCooldown = GetUpgradeRank(RamId) >= 2 ? 1.3f : 1.65f;
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
        if (ApplyPhysicalShieldHit(Enemy, Hit))
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
    if (!HasUpgrade(MirrorId) && GetUpgradeRank(AnchorId) < 2) return;
    const int32 Capacity = FMath::Max(2, 1 + GetUpgradeRank(MirrorId)) + (GetUpgradeRank(CapacitorId) >= 2 ? 2 : 0);
    if (StoredShots >= Capacity) return;
    StoredDamage.Add(Damage);
    ++StoredShots;
}

void ADBCharacter::BankAnchoredForce(float Damage)
{
    if (GetUpgradeRank(AnchorId) >= 2) CaptureEnergy(Damage);
}

void ADBCharacter::ReceiveAttack(float Damage, FVector Source, bool bUnblockable, AActor* Attacker)
{
    if (!CanAct() || Damage <= 0.f || InvulnerabilityTime > 0.f) return;
    const FVector ToSource = (Source - ViewCamera->GetComponentLocation()).GetSafeNormal();
    const bool bFacing = FVector::DotProduct(GetAimDirection(), ToSource) >= .38f;
    const bool bCanBlock = bGuarding && !IsShieldAway() && ShieldState == EDBShieldState::Held
        && AttackRecovery <= 0.f && GuardBreakTime <= 0.f && bFacing && !bUnblockable;
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
            if (HasUpgrade(MirrorId)) CaptureEnergy(Damage);
            if (HasUpgrade(CapacitorId)) EmpoweredShots = GetUpgradeRank(CapacitorId) >= 2 ? 5 : 3;
            ParryFlashTime = .32f;
            Recoil = -.8f;
            SetCombatMessage(HasUpgrade(MirrorId) ? FString::Printf(TEXT("PERFECT BLOCK - %d charge%s stored"), StoredShots, StoredShots == 1 ? TEXT("") : TEXT("s")) : TEXT("PERFECT GUARD - punish the opening"), 1.2f);
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
        || Id == StormId || Id == FractureId || Id == SplitId || Id == CapacitorId || Id == AnchorId;
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
    default: return TEXT("KINETIC");
    }
}


FString ADBCharacter::GetSpecialLabel() const
{
    if (IsShieldAway()) return ShieldState == EDBShieldState::Returning ? TEXT("RETURNING SHIELD") : TEXT("Q / LMB  RECALL");
    return HasUpgrade(RamId) ? TEXT("Q  RAM BASH") : TEXT("Q  HEAVY BASH");
}

FString ADBCharacter::GetUpgradeDescription(FName Id, int32 Rank)
{
    Rank = FMath::Clamp(Rank, 1, 3);
    if (Id == MirrorId)
        return Rank == 1 ? TEXT("Raise your shield within 0.20s before a hit to store 1 charge (max 2). Your next shield hit spends 1 charge, adding 180% of the blocked attack's damage.")
            : Rank == 2 ? TEXT("Held ranged blocks also bank force. Store 3 payloads; wider timed-guard window.")
            : TEXT("Released force arcs from the physical impact into 2 nearby enemies. Store 4 payloads.");
    if (Id == RamId)
        return Rank == 1 ? TEXT("Q with shield in hand: rush into enemies. Q while it is away recalls it. Thrown hits gain damage and speed.")
            : Rank == 2 ? TEXT("Q with shield in hand: rush. Shield hits restore guard energy. Rush cooldown is 1.3s (was 1.65s). Q while it is away recalls it.")
            : TEXT("The opened rim sweeps a wider path in flight; the rush catches nearby flankers.");
    if (Id == EchoId)
        return Rank == 1 ? TEXT("Shield contact leaves a delayed local rim-impact echo.")
            : Rank == 2 ? TEXT("The echo follows its struck target a short distance from the original impact.")
            : TEXT("Each physical contact leaves two delayed impact echoes.");
    if (Id == FrostId)
        return Rank == 1 ? TEXT("AUTOMATIC: Shield hits slow an enemy for 4s. Hit it again while chilled for bonus shatter damage. R cycles cores if you own more than one.")
            : Rank == 2 ? TEXT("Every third contact spreads frost into nearby enemies.")
            : TEXT("Shattering chilled enemies creates another local frost burst.");
    if (Id == EmberId)
        return Rank == 1 ? TEXT("AUTOMATIC: Shield hits set enemies burning for 4s. You can move away while fire damages them. R cycles cores if you own more than one.")
            : Rank == 2 ? TEXT("Every third contact spreads flame to nearby enemies.")
            : TEXT("Directly killing a burning enemy causes a spreading fire explosion.");
    if (Id == StormId)
        return Rank == 1 ? TEXT("AUTOMATIC: Hit an enemy twice within 5s to arc to up to 2 other enemies within 6.2m and in sight. A lone enemy produces no arc.")
            : Rank == 2 ? TEXT("Every third contact seeds nearby enemies with storm.")
            : TEXT("Storm contacts replenish guard energy for your return to close combat.");
    if (Id == FractureId)
        return Rank == 1 ? TEXT("Shield impacts consume existing chill and storm marks in a finite chain burst.")
            : Rank == 2 ? TEXT("Physical impacts release another elemental aftershock into nearby foes.")
            : TEXT("Every direct elemental contact also spreads a local bloom.");
    if (Id == SplitId)
        return Rank == 1 ? TEXT("Your shield ricochets once toward another enemy ahead, or off a wall when not anchored.")
            : Rank == 2 ? TEXT("Your shield can ricochet twice before you call it back.")
            : TEXT("Three ricochets can seek enemies across a wider angle. Your return path remains a separate attack.");
    if (Id == CapacitorId)
        return Rank == 1 ? TEXT("Perfect guards empower 3 physical contacts. Guard energy recovers faster.")
            : Rank == 2 ? TEXT("A perfect guard empowers 5 contacts. Captured-force storage gains 2 slots.")
            : TEXT("An empowered contact also restores your dash, rewarding aggressive repositioning.");
    if (Id == AnchorId)
        return Rank == 1 ? TEXT("Throw and let the shield stop between you and a shooter. It blocks bolts crossing its front until broken. Q recalls it; melee and heavy attacks bypass it.")
            : Rank == 2 ? TEXT("Anchored blocks bank enemy force for later physical shield contacts.")
            : TEXT("A broken anchor erupts in a local impact burst before the shield returns.");
    return TEXT("");
}

void ADBCharacter::OnRunReset()
{
    SuspendCombatInput();
    if (IsValid(ThrownShield)) ThrownShield->Destroy();
    ThrownShield = nullptr;
    ShieldState = EDBShieldState::Held;
    bShieldReady = true;
    bStrikePoseActive = false;
    AttackRecovery = ChargeHeld = ThrowCharge = StrikeDelay = CatchPose = 0.f;
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
    if (!WeaponCore) return;
    if (CurrentElement != EDBElement::Neutral && !HasUpgrade(ElementId(CurrentElement))) CurrentElement = EDBElement::Neutral;
    if (GetElementMaterial(CurrentElement))
    {
        const int32 NamedSlot = WeaponCore->GetMaterialIndex(TEXT("M_Core"));
        const int32 GlowSlot = NamedSlot != INDEX_NONE ? NamedSlot : (WeaponCore->GetNumMaterials() > 1 ? 1 : 0);
        WeaponCore->SetMaterial(GlowSlot, GetElementMaterial(CurrentElement));
    }
    CoreLight->SetLightColor(ElementColor(CurrentElement));
    const FName Ids[] = { MirrorId, RamId, EchoId, FrostId, EmberId, StormId, FractureId, SplitId, CapacitorId, AnchorId };
    const FVector Locations[] = {
        FVector(-9,-24,12), FVector(-3,0,-34), FVector(-9,24,12),
        FVector(-8,-14,27), FVector(-8,14,27), FVector(-8,-30,-8),
        FVector(-8,30,-8), FVector(-6,0,36), FVector(-9,0,-24), FVector(-7,25,-25)
    };
    const FVector Sizes[] = {
        FVector(7,14,14), FVector(14,27,12), FVector(10,13,13),
        FVector(5,5,8.5), FVector(5,5,8.5), FVector(5,5,8.5),
        FVector(7,7,8), FVector(8,11,11), FVector(12,12,12), FVector(7,7,12)
    };
    for (int32 Index = 0; Index < AttachmentParts.Num(); ++Index)
    {
        UStaticMeshComponent* Part = AttachmentParts[Index];
        const int32 Rank = GetUpgradeRank(Ids[Index]);
        Part->SetVisibility(Rank > 0);
        Part->SetHiddenInGame(IsShieldAway() || Rank == 0);
        Part->SetRelativeLocation(Locations[Index]);
        Part->SetRelativeRotation(Index >= 3 && Index <= 6 ? FRotator(-20.f,0.f,0.f) : FRotator::ZeroRotator);
        if (const UStaticMesh* PartMesh = Part->GetStaticMesh())
        {
            const FVector Size = PartMesh->GetBounds().BoxExtent * 2.f;
            const FVector Desired = Sizes[Index] * (1.f + .1f * FMath::Max(0, Rank - 1));
            Part->SetRelativeScale3D(FVector(Desired.X / FMath::Max(1.f, Size.X),
                Desired.Y / FMath::Max(1.f, Size.Y), Desired.Z / FMath::Max(1.f, Size.Z)));
        }
        UMaterialInterface* Material = BronzeMaterial;
        if (Ids[Index] == FrostId) Material = FrostMaterial;
        else if (Ids[Index] == EmberId) Material = EmberMaterial;
        else if (Ids[Index] == StormId || Ids[Index] == FractureId) Material = StormMaterial;
        else if (Ids[Index] == MirrorId || Ids[Index] == CapacitorId || Ids[Index] == AnchorId) Material = CoreMaterial;
        if (Material) Part->SetMaterial(0, Material);
    }
}

void ADBCharacter::UpdateWeapon(float DeltaSeconds)
{
    GuardBlend = FMath::FInterpTo(GuardBlend, bGuarding ? 1.f : 0.f, DeltaSeconds, bGuarding ? 19.f : 12.f);
    Recoil = FMath::FInterpTo(Recoil, 0.f, DeltaSeconds, 12.f);
    ImpactPose = FMath::FInterpTo(ImpactPose, 0.f, DeltaSeconds, 8.f);
    CatchPose = FMath::FInterpTo(CatchPose, 0.f, DeltaSeconds, 8.f);
    EquipPose = FMath::FInterpTo(EquipPose, 0.f, DeltaSeconds, 3.5f);
    LookSwayX = FMath::FInterpTo(LookSwayX, 0.f, DeltaSeconds, 10.f);
    LookSwayY = FMath::FInterpTo(LookSwayY, 0.f, DeltaSeconds, 10.f);
    const float SpeedFraction = FMath::Clamp(GetVelocity().Size2D() / 590.f, 0.f, 1.f);
    BobPhase += DeltaSeconds * FMath::Lerp(1.5f, 9.f, SpeedFraction);
    const float Bob = FMath::Sin(BobPhase) * .8f * SpeedFraction;
    FVector Pose = FMath::Lerp(FVector(85,-52,-38), FVector(73,-51,-32), GuardBlend);
    FRotator Rotation = FMath::Lerp(FRotator(-8,-34,-12), FRotator(0,-7,-8), GuardBlend);

    if (ShieldState == EDBShieldState::Charging)
    {
        const float Anticipation = FMath::Clamp(ChargeHeld / .22f, 0.f, 1.f) * .65f + ThrowCharge * .35f;
        Pose += FVector(-23,-10,9) * Anticipation;
        Rotation += FRotator(-12,-18,-20) * Anticipation;
    }
    else if (IsShieldAway())
    {
        Pose = FVector(105,-43,-22);
        Rotation = FRotator(-6,3,14);
    }
    else if (bStrikePoseActive && AttackRecovery > 0.f)
    {
        const float Phase = 1.f - FMath::Clamp(AttackRecovery / StrikeTotal, 0.f, 1.f);
        const FVector Windup(58,-70,-29);
        const FVector Contact = bHeavyStrike ? FVector(125,-24,-23) : FVector(107,20,-20);
        const FRotator WindupRotation(-18,-62,-28);
        const FRotator ContactRotation = bHeavyStrike ? FRotator(0,-8,0) : FRotator(6,55,64);
        if (Phase < .18f)
        {
            Pose = FMath::Lerp(Pose, Windup, Phase / .18f);
            Rotation = FMath::Lerp(Rotation, WindupRotation, Phase / .18f);
        }
        else if (Phase < .42f)
        {
            const float Alpha = (Phase - .18f) / .24f;
            Pose = FMath::Lerp(Windup, Contact, Alpha);
            Rotation = FMath::Lerp(WindupRotation, ContactRotation, Alpha);
        }
        else
        {
            const float Alpha = FMath::SmoothStep(0.f, 1.f, (Phase - .42f) / .58f);
            Pose = FMath::Lerp(Contact, Pose, Alpha);
            Rotation = FMath::Lerp(ContactRotation, Rotation, Alpha);
        }
    }
    Pose += FVector(-Recoil * 4.f - CatchPose * 18.f, -LookSwayX + Bob * .6f + CatchPose * 10.f, Bob + LookSwayY - EquipPose * 8.f + CatchPose * 6.f);
    Rotation += FRotator(Recoil * 5.f - EquipPose * 10.f, CatchPose * 15.f, Bob);
    WeaponRoot->SetRelativeLocation(Pose);
    WeaponRoot->SetRelativeRotation(Rotation);

    WeaponBody->SetHiddenInGame(true);
    WeaponCore->SetHiddenInGame(IsShieldAway());
    WeaponCore->SetRelativeRotation(FRotator(0,0,GetWorld()->GetTimeSeconds() * (ShieldState == EDBShieldState::Charging ? 65.f : 10.f)));
    CoreLight->SetVisibility(!IsShieldAway());
    CoreLight->SetIntensity(40.f + ThrowCharge * 70.f + ParryFlashTime * 220.f);
    for (int32 Index = 0; Index < ShieldPlates.Num(); ++Index)
    {
        ShieldPlates[Index]->SetHiddenInGame(IsShieldAway() || Index != 0);
        ShieldPlates[Index]->SetRelativeLocation(FVector::ZeroVector);
        ShieldPlates[Index]->SetRelativeRotation(FRotator::ZeroRotator);
    }
    const FName Ids[] = { MirrorId, RamId, EchoId, FrostId, EmberId, StormId, FractureId, SplitId, CapacitorId, AnchorId };
    for (int32 Index = 0; Index < AttachmentParts.Num(); ++Index)
        AttachmentParts[Index]->SetHiddenInGame(IsShieldAway() || !HasUpgrade(Ids[Index]));
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
