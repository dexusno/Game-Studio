#include "DBCharacter.h"
#include "Materials/MaterialInstanceDynamic.h"
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
    ShieldHub = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("ShieldHub"));
    ShieldHub->SetupAttachment(WeaponRoot);
    PrepareViewMesh(ShieldHub);

    PieceStates.Init(EDBShieldPieceState::Attached, ShieldPieceCount);
    PieceFlights.SetNum(ShieldPieceCount);
    PieceRegenRemaining.Init(0.f, ShieldPieceCount);
    PieceRegenDuration.Init(3.f, ShieldPieceCount);
    PieceDockPulse.Init(0.f, ShieldPieceCount);
    for (int32 Index = 0; Index < ShieldPieceCount; ++Index)
    {
        UStaticMeshComponent* Plate = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("ShieldPetal%d"), Index));
        Plate->SetupAttachment(WeaponRoot);
        PrepareViewMesh(Plate);
        ShieldPlates.Add(Plate);
        UStaticMeshComponent* Glow = CreateDefaultSubobject<UStaticMeshComponent>(*FString::Printf(TEXT("PieceGlow%d"), Index));
        Glow->SetupAttachment(WeaponRoot);
        PrepareViewMesh(Glow);
        PieceGlows.Add(Glow);
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
    if (auto* GlowBase = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_CombatGlow.M_CombatGlow")))
    {
        auto* Selection = UMaterialInstanceDynamic::Create(GlowBase, this);
        Selection->SetVectorParameterValue(TEXT("Color"), FLinearColor(1.f,.48f,.045f));
        SelectedPieceMaterial = Selection;
    }
    FrostMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Frost.M_Frost"));
    StormMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Storm.M_Storm"));
    EmberMaterial = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Art/Materials/M_Ember.M_Ember"));

    UStaticMesh* ArmMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Forearm.SM_Forearm"));
    UStaticMesh* CoreMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Core.SM_Core"));
    UStaticMesh* HubMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_ShieldHub.SM_ShieldHub"));
    UStaticMesh* PlateMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_ShieldSegment.SM_ShieldSegment"));
    UStaticMesh* GlowMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_ShieldSegmentGlow.SM_ShieldSegmentGlow"));
    UStaticMesh* CrystalMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Game/Art/Meshes/SM_Crystal.SM_Crystal"));
    // Keep the legacy component for callers, but there is no gun body in this version.
    WeaponBody->SetStaticMesh(nullptr);
    Forearm->SetStaticMesh(ArmMesh ? ArmMesh : BeamMesh.Get());
    // The clearer shield framing exposes the old forearm's rear cap. Continue
    // that sleeve behind the camera so the hand stays connected to the bearer.
    if (auto* SleeveMesh = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cylinder.Cylinder")))
    {
        auto* Sleeve = NewObject<UStaticMeshComponent>(this, TEXT("ForearmContinuation"));
        AddInstanceComponent(Sleeve);Sleeve->SetupAttachment(WeaponRoot);
        Sleeve->SetStaticMesh(SleeveMesh);Sleeve->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Sleeve->SetCastShadow(false);Sleeve->SetMaterial(0, DarkMaterial);
        Sleeve->SetRelativeLocation(FVector(-75.f,0.f,-62.5f));
        Sleeve->SetRelativeRotation(FRotationMatrix::MakeFromZ(FVector(-30.f,0.f,-91.f)).Rotator());
        Sleeve->SetRelativeScale3D(FVector(.18f,.20f,.96f));Sleeve->RegisterComponent();
    }
    WeaponCore->SetStaticMesh(CoreMesh);
    ShieldHub->SetStaticMesh(HubMesh);
    if (!HubMesh || !PlateMesh || !GlowMesh)
        UE_LOG(LogTemp, Error, TEXT("Segmented shield requires SM_ShieldHub, SM_ShieldSegment and SM_ShieldSegmentGlow in /Game/Art/Meshes; no whole-disc fallback is used."));
    if (!ArmMesh)
    {
        Forearm->SetRelativeLocation(FVector(-24.f, 0.f, -4.f));
        Forearm->SetRelativeScale3D(FVector(.43f, .12f, .12f));
    }
    for (int32 Index = 0; Index < ShieldPlates.Num(); ++Index)
    {
        UStaticMeshComponent* Plate = ShieldPlates[Index];
        Plate->SetStaticMesh(PlateMesh);
        Plate->SetVisibility(true);
        Plate->SetRelativeRotation(FRotator(0.f, 0.f, Index * 60.f));
        PieceGlows[Index]->SetStaticMesh(GlowMesh);
        PieceGlows[Index]->SetRelativeRotation(FRotator(0.f, 0.f, Index * 60.f));
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
    Input->BindAction(TEXT("Recall"), IE_Pressed, this, &ADBCharacter::RecallShield);
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
    UpdatePieces(DeltaSeconds);

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
    if (AttackRecovery <= 0.f)
    {
        if (bStrikeBuffered)
        {
            bStrikeBuffered = false;
            StartRimStrike(false);
        }
        else if (!bWantsFire && bWantsGuard && !bGuarding && GetAttachedPieceCount() > 0)
        {
            // Holding guard through an attack restores ordinary protection, not a free parry.
            bGuarding = true;
            SinceGuarded = 1.f;
        }
    }
    if (bWantsFire && AttackRecovery <= 0.f)
    {
        ChargeHeld += DeltaSeconds;
        // Only the pieces present when this hold began are eligible. New arrivals dock unlit.
        while (SelectionCursor < SelectionQueue.Num() && ChargeHeld >= .22f + SelectionCursor * .14f)
        {
            const int32 Index = SelectionQueue[SelectionCursor++];
            if (PieceStates[Index] != EDBShieldPieceState::Attached) continue;
            PieceStates[Index] = EDBShieldPieceState::Selected;
            PlayCombatSound(EquipSound, .17f, 1.f + GetSelectedPieceCount() * .10f);
        }
        ThrowCharge = static_cast<float>(GetSelectedPieceCount()) / ShieldPieceCount;
    }
    if (GetAttachedPieceCount() == 0) bGuarding = false;

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
    RefreshAggregateShieldState();
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
    if (bWantsFire || AttackRecovery > .12f) return;
    ClearPieceSelection();
    for (int32 Index = 0; Index < ShieldPieceCount; ++Index)
        if (PieceStates[Index] == EDBShieldPieceState::Attached) SelectionQueue.Add(Index);
    bWantsFire = true;
    ChargeHeld = 0.f;
    ThrowCharge = 0.f;
    bGuarding = false;
    RefreshAggregateShieldState();
}

void ADBCharacter::ReleaseFire()
{
    if (!bWantsFire) return;
    bWantsFire = false;
    if (!CanAct()) { ClearPieceSelection(); return; }
    if (AttackRecovery > 0.f)
    {
        bStrikeBuffered = true;
        return;
    }
    if (GetSelectedPieceCount() > 0) LaunchShield();
    else StartRimStrike(false);
}

void ADBCharacter::PressGuard()
{
    if (!CanAct()) return;
    bWantsGuard = true;
    bWantsFire = false;
    ClearPieceSelection();
    if (GetAttachedPieceCount() == 0)
    {
        SetCombatMessage(TEXT("NO GUARD PIECES - Q recalls survivors / tap LMB strikes with the core"), 1.5f);
        return;
    }
    if (AttackRecovery > 0.f || GuardRaiseCooldown > 0.f || bGuarding) return;
    bGuarding = true;
    SinceGuarded = 0.f;
    GuardRaiseCooldown = .38f;
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
    ClearPieceSelection();
    RushVictims.Reset();
    EchoShots.Reset();
    StopJumping();
    ConsumeMovementInputVector();
    RefreshAggregateShieldState();
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

int32 ADBCharacter::GetAttachedPieceCount() const
{
    int32 Count = 0;
    for (EDBShieldPieceState State : PieceStates)
        if (State == EDBShieldPieceState::Attached || State == EDBShieldPieceState::Selected) ++Count;
    return Count;
}

int32 ADBCharacter::GetSelectedPieceCount() const
{
    int32 Count = 0;
    for (EDBShieldPieceState State : PieceStates) if (State == EDBShieldPieceState::Selected) ++Count;
    return Count;
}

int32 ADBCharacter::GetDeployedPieceCount() const
{
    int32 Count = 0;
    for (EDBShieldPieceState State : PieceStates)
        if (State == EDBShieldPieceState::Outbound || State == EDBShieldPieceState::Lodged || State == EDBShieldPieceState::Returning) ++Count;
    return Count;
}

int32 ADBCharacter::GetRegeneratingPieceCount() const
{
    int32 Count = 0;
    for (EDBShieldPieceState State : PieceStates) if (State == EDBShieldPieceState::Regenerating) ++Count;
    return Count;
}

EDBShieldPieceState ADBCharacter::GetPieceState(int32 Index) const
{
    return PieceStates.IsValidIndex(Index) ? PieceStates[Index] : EDBShieldPieceState::Regenerating;
}

float ADBCharacter::GetPieceRegenerationProgress(int32 Index) const
{
    if (!PieceStates.IsValidIndex(Index) || PieceStates[Index] != EDBShieldPieceState::Regenerating) return 0.f;
    return FMath::Clamp(1.f - PieceRegenRemaining[Index] / FMath::Max(.01f, PieceRegenDuration[Index]), 0.f, 1.f);
}

ADBThrownShield* ADBCharacter::GetPieceFlight(int32 Index) const
{
    return PieceFlights.IsValidIndex(Index) && IsValid(PieceFlights[Index]) ? PieceFlights[Index].Get() : nullptr;
}

float ADBCharacter::GetTotalAnchorIntegrity() const
{
    float Total = 0.f;
    for (ADBThrownShield* Flight : PieceFlights)
        if (IsValid(Flight) && Flight->IsAnchored()) Total += FMath::Max(0.f, Flight->GetAnchorIntegrity());
    return Total;
}

FVector ADBCharacter::GetPieceCatchLocation(int32 Index) const
{
    const FVector RadialCentre = FRotator(0.f, 0.f, FMath::Clamp(Index, 0, ShieldPieceCount - 1) * 60.f).RotateVector(FVector(0.f, 0.f, 26.f));
    return WeaponRoot->GetComponentTransform().TransformPosition(RadialCentre);
}

bool ADBCharacter::IsShieldAway() const
{
    // Compatibility/query only. Held offense and guarding use their actual attached pieces.
    return GetDeployedPieceCount() > 0;
}

void ADBCharacter::RefreshAggregateShieldState()
{
    ThrownShield = nullptr;
    bool bHasOutbound = false;
    bool bHasReturning = false;
    for (int32 Index = 0; Index < ShieldPieceCount; ++Index)
    {
        if (!ThrownShield && IsValid(PieceFlights[Index])) ThrownShield = PieceFlights[Index];
        bHasOutbound |= PieceStates[Index] == EDBShieldPieceState::Outbound;
        bHasReturning |= PieceStates[Index] == EDBShieldPieceState::Returning;
    }
    ShieldState = bWantsFire ? EDBShieldState::Charging : bHasReturning ? EDBShieldState::Returning
        : bHasOutbound ? EDBShieldState::Outbound : ThrownShield ? EDBShieldState::Lodged : EDBShieldState::Held;
    bShieldReady = GetAttachedPieceCount() > 0 && AttackRecovery <= 0.f && !bWantsFire;
    GuardEnergy = MaxGuardEnergy * static_cast<float>(GetAttachedPieceCount()) / ShieldPieceCount;
}

void ADBCharacter::ClearPieceSelection()
{
    for (EDBShieldPieceState& State : PieceStates)
        if (State == EDBShieldPieceState::Selected) State = EDBShieldPieceState::Attached;
    SelectionQueue.Reset();
    SelectionCursor = 0;
    ChargeHeld = ThrowCharge = 0.f;
}

void ADBCharacter::StartPieceRegeneration(int32 Index)
{
    if (!PieceStates.IsValidIndex(Index) || PieceStates[Index] == EDBShieldPieceState::Regenerating) return;
    // Clear the ownership before destroying the actor. A duplicate callback cannot reset this timer.
    ADBThrownShield* Flight = PieceFlights[Index];
    PieceFlights[Index] = nullptr;
    PieceStates[Index] = EDBShieldPieceState::Regenerating;
    PieceRegenDuration[Index] = HasUpgrade(CapacitorId) ? 2.6f : 3.f;
    PieceRegenRemaining[Index] = PieceRegenDuration[Index];
    PieceDockPulse[Index] = 0.f;
    if (IsValid(Flight)) Flight->Destroy();
    if (GetAttachedPieceCount() == 0) bGuarding = false;
    RefreshAggregateShieldState();
}

bool ADBCharacter::SpendGuardPiece()
{
    for (int32 Offset = 0; Offset < ShieldPieceCount; ++Offset)
    {
        const int32 Index = (NextBlockPiece + Offset) % ShieldPieceCount;
        if (PieceStates[Index] != EDBShieldPieceState::Attached && PieceStates[Index] != EDBShieldPieceState::Selected) continue;
        NextBlockPiece = (Index + 1) % ShieldPieceCount;
        DrawSpark(GetPieceCatchLocation(Index), CurrentElement, 16.f);
        StartPieceRegeneration(Index);
        return true;
    }
    return false;
}

bool ADBCharacter::AdvancePieceRegeneration(float Seconds)
{
    int32 Best = INDEX_NONE;
    for (int32 Index = 0; Index < ShieldPieceCount; ++Index)
        if (PieceStates[Index] == EDBShieldPieceState::Regenerating
            && (Best == INDEX_NONE || PieceRegenRemaining[Index] < PieceRegenRemaining[Best])) Best = Index;
    if (Best == INDEX_NONE) return false;
    PieceRegenRemaining[Best] = FMath::Max(0.f, PieceRegenRemaining[Best] - Seconds);
    return true;
}

void ADBCharacter::UpdatePieces(float DeltaSeconds)
{
    CatchSoundCooldown = FMath::Max(0.f, CatchSoundCooldown - DeltaSeconds);
    for (int32 Index = 0; Index < ShieldPieceCount; ++Index)
    {
        PieceDockPulse[Index] = FMath::Max(0.f, PieceDockPulse[Index] - DeltaSeconds * 3.f);
        if (PieceStates[Index] == EDBShieldPieceState::Regenerating)
        {
            PieceRegenRemaining[Index] = FMath::Max(0.f, PieceRegenRemaining[Index] - DeltaSeconds);
            if (PieceRegenRemaining[Index] <= 0.f)
            {
                PieceStates[Index] = EDBShieldPieceState::Attached;
                PieceDockPulse[Index] = 1.f;
                if (CatchSoundCooldown <= 0.f)
                {
                    PlayCombatSound(EquipSound, .35f, 1.25f);
                    CatchSoundCooldown = .07f;
                }
            }
        }
        else if ((PieceStates[Index] == EDBShieldPieceState::Outbound || PieceStates[Index] == EDBShieldPieceState::Lodged
            || PieceStates[Index] == EDBShieldPieceState::Returning) && !IsValid(PieceFlights[Index]))
        {
            // Unexpected actor removal is a lost piece, never an instant free replacement.
            StartPieceRegeneration(Index);
        }
    }
    RefreshAggregateShieldState();
}

void ADBCharacter::OnShieldPieceFlightState(int32 Index, ADBThrownShield* Flight, EDBShieldPieceState State)
{
    if (!PieceFlights.IsValidIndex(Index) || PieceFlights[Index] != Flight || !IsValid(Flight)) return;
    if (State != EDBShieldPieceState::Outbound && State != EDBShieldPieceState::Lodged && State != EDBShieldPieceState::Returning) return;
    if (PieceStates[Index] == EDBShieldPieceState::Regenerating) return;
    PieceStates[Index] = State;
    RefreshAggregateShieldState();
}

void ADBCharacter::OnShieldPieceDestroyed(int32 Index, ADBThrownShield* Flight)
{
    if (!IsValid(Flight) || !PieceFlights.IsValidIndex(Index) || PieceFlights[Index] != Flight || PieceStates[Index] == EDBShieldPieceState::Regenerating) return;
    StartPieceRegeneration(Index);
    SetCombatMessage(FString::Printf(TEXT("PIECE %d DESTROYED - rebuilding independently"), Index + 1), 1.4f);
    PlayCombatSound(ImpactSound, .55f, .6f);
    UpdateWeapon(0.f);
}

void ADBCharacter::OnShieldPieceCaught(int32 Index, ADBThrownShield* Flight, bool bEmergency)
{
    if (!IsValid(Flight) || !PieceFlights.IsValidIndex(Index) || PieceFlights[Index] != Flight || PieceStates[Index] == EDBShieldPieceState::Regenerating) return;
    PieceFlights[Index] = nullptr;
    PieceStates[Index] = EDBShieldPieceState::Attached;
    PieceRegenRemaining[Index] = 0.f;
    PieceDockPulse[Index] = 1.f;
    CatchPose = FMath::Max(CatchPose, .35f);
    Recoil = FMath::Max(Recoil, .3f);
    if (CatchSoundCooldown <= 0.f)
    {
        PlayCombatSound(CatchSound ? CatchSound.Get() : GuardSound.Get(), .5f, bEmergency ? .8f : 1.f);
        CatchSoundCooldown = .06f;
    }
    RefreshAggregateShieldState();
    SetCombatMessage(bEmergency ? TEXT("PIECE RECOVERED - obstructed route, no through-wall damage") : TEXT("PIECE DOCKED - available, unselected"), .8f);
    UpdateWeapon(0.f);
}

void ADBCharacter::RecallShield()
{
    if (!CanAct()) return;
    int32 Recalled = 0;
    for (ADBThrownShield* Flight : PieceFlights)
    {
        if (!IsValid(Flight) || Flight->IsReturning()) continue;
        Flight->Recall();
        ++Recalled;
    }
    if (Recalled == 0)
    {
        SetCombatMessage(GetRegeneratingPieceCount() > 0 ? TEXT("Q RECALL - destroyed pieces must rebuild") : TEXT("Q RECALL - no deployed pieces"), 1.f);
        return;
    }
    PlayCombatSound(RecallSound, .85f);
    SetCombatMessage(FString::Printf(TEXT("RECALLING %d - move to shape their return cuts"), Recalled), 1.2f);
}

void ADBCharacter::OnShieldCaught(bool bEmergency)
{
    // Retained for older callers; each catch resolves only its own identity.
    for (int32 Index = 0; Index < ShieldPieceCount; ++Index)
    {
        if (ADBThrownShield* Flight = GetPieceFlight(Index))
        {
            OnShieldPieceCaught(Index, Flight, bEmergency);
            Flight->Destroy();
            return;
        }
    }
}

FString ADBCharacter::GetShieldStateLabel() const
{
    if (bWantsFire)
        return FString::Printf(TEXT("%d SELECTED - release LMB launches only lit pieces"), GetSelectedPieceCount());
    if (AttackRecovery > 0.f) return TEXT("COMMITTED - guard returns after recovery");
    if (bGuarding) return FString::Printf(TEXT("GUARDING - %d pieces / one spent per blocked hit"), GetAttachedPieceCount());
    return FString::Printf(TEXT("%d HELD / %d DEPLOYED / %d REBUILDING"), GetAttachedPieceCount(), GetDeployedPieceCount(), GetRegeneratingPieceCount());
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

    if (CurrentElement == EDBElement::Storm && GetUpgradeRank(StormId) >= 3)
        AdvancePieceRegeneration(.18f);

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
    if (!CanAct() || AttackRecovery > 0.f) return;
    ClearPieceSelection();
    bShieldReady = false;
    bWantsFire = false;
    bGuarding = false;
    ThrowCharge = ChargeHeld = 0.f;
    bHeavyStrike = bHeavy;
    bReforgeUsed = false;
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
    if (!CanAct()) return;
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
    const float HeldFraction = static_cast<float>(GetAttachedPieceCount()) / ShieldPieceCount;
    Contact.Damage = bHeavyStrike ? (HasUpgrade(RamId) ? 62.f : 44.f) + HeldFraction * 24.f : 30.f + HeldFraction * 18.f;
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
        if (ApplyPhysicalShieldHit(Enemy, Contact) && bHeavyStrike && GetUpgradeRank(RamId) >= 2 && !bReforgeUsed)
        {
            bReforgeUsed = AdvancePieceRegeneration(.85f);
            if (bReforgeUsed) SetCombatMessage(TEXT("RAM REFORGE - one piece rebuild advanced"), 1.f);
        }
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
    if (!CanAct() || AttackRecovery > 0.f || GetSelectedPieceCount() == 0) return;
    const FVector Eye = ViewCamera->GetComponentLocation();
    const FVector AimPoint = FindAimPoint();
    const float Charge = ThrowCharge;
    int32 Launched = 0;
    for (int32 Index = 0; Index < ShieldPieceCount; ++Index)
    {
        if (PieceStates[Index] != EDBShieldPieceState::Selected) continue;
        FVector Origin = GetPieceCatchLocation(Index);
        FCollisionQueryParams Params(SCENE_QUERY_STAT(DBPieceLaunchRoom), false, this);
        FHitResult Cover;
        if (GetWorld()->SweepSingleByChannel(Cover, Eye, Origin, FQuat::Identity, ECC_Visibility,
            FCollisionShape::MakeSphere(16.f), Params)) Origin = Cover.Location + Cover.Normal * 3.f;
        const FVector Direction = (AimPoint - Origin).GetSafeNormal();
        FActorSpawnParameters Spawn;
        Spawn.Owner = this;
        Spawn.Instigator = this;
        Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
        ADBThrownShield* Flight = GetWorld()->SpawnActor<ADBThrownShield>(ADBThrownShield::StaticClass(), Origin, Direction.Rotation(), Spawn);
        if (!Flight) continue;
        PieceFlights[Index] = Flight;
        PieceStates[Index] = EDBShieldPieceState::Outbound;
        Flight->Initialize(this, Direction, Charge, Index);
        ++Launched;
    }
    ClearPieceSelection();
    bWantsFire = false;
    bGuarding = false;
    bStrikePoseActive = false;
    bStrikePending = false;
    if (Launched > 0)
    {
        AttackRecovery = .22f;
        ImpactPose = .7f;
        PlayCombatSound(AttackSound, .65f + Launched * .045f, .9f - Launched * .025f);
        SetCombatMessage(FString::Printf(TEXT("%d LAUNCHED / %d HELD - RMB protects with the remainder / Q recalls"),
            Launched, GetAttachedPieceCount()), 1.6f);
    }
    RefreshAggregateShieldState();
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
    if (SpecialCooldown > 0.f || AttackRecovery > 0.f)
    {
        SetCombatMessage(FString::Printf(TEXT("F HEAVY - ready in %.1fs"), FMath::Max(SpecialCooldown, AttackRecovery)), .8f);
        return;
    }
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
            if (GetUpgradeRank(RamId) >= 2 && !bReforgeUsed)
            {
                bReforgeUsed = AdvancePieceRegeneration(.85f);
                if (bReforgeUsed) SetCombatMessage(TEXT("RAM REFORGE - one piece rebuild advanced"), 1.f);
            }
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

void ADBCharacter::ReceiveAttack(float Damage, FVector Source, bool bUnblockable, AActor* Attacker, uint32 AttackId)
{
    if (!CanAct() || Damage <= 0.f) return;
    if (AttackId != 0)
    {
        const uint64 Key = (static_cast<uint64>(IsValid(Attacker) ? Attacker->GetUniqueID() : 0) << 32) | AttackId;
        if (ReceivedAttackKeys.Contains(Key)) return;
        // Keep a bounded history of actual attacks, not a blanket invulnerability window:
        // simultaneous distinct projectiles can each spend one piece.
        ReceivedAttackKeys.Add(Key);
        if (ReceivedAttackKeys.Num() > 128) ReceivedAttackKeys.RemoveAt(0);
    }
    if (InvulnerabilityTime > 0.f) return;
    const FVector ToSource = (Source - ViewCamera->GetComponentLocation()).GetSafeNormal();
    const bool bFacing = FVector::DotProduct(GetAimDirection(), ToSource) >= .38f;
    const bool bCanBlock = bGuarding && GetAttachedPieceCount() > 0 && !bWantsFire
        && AttackRecovery <= 0.f && bFacing && !bUnblockable;
    SinceDamaged = 0.f;
    if (bCanBlock && SpendGuardPiece())
    {
        const float ParryWindow = .2f + (GetUpgradeRank(MirrorId) >= 2 ? .045f : 0.f);
        if (SinceGuarded <= ParryWindow)
        {
            SinceGuarded = ParryWindow + 1.f;
            if (HasUpgrade(MirrorId)) CaptureEnergy(Damage);
            if (HasUpgrade(CapacitorId)) EmpoweredShots = GetUpgradeRank(CapacitorId) >= 2 ? 5 : 3;
            ParryFlashTime = .32f;
            Recoil = -.8f;
            SetCombatMessage(HasUpgrade(MirrorId)
                ? FString::Printf(TEXT("PERFECT BLOCK - piece spent / %d force charge%s stored"), StoredShots, StoredShots == 1 ? TEXT("") : TEXT("s"))
                : HasUpgrade(CapacitorId) ? TEXT("PERFECT BLOCK - piece spent / next contacts empowered")
                : TEXT("PERFECT BLOCK - piece spent / punish the opening"), 1.4f);
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
        }
        else
        {
            const FVector AttackOrigin = IsValid(Attacker) ? Attacker->GetActorLocation() : Source;
            if (GetUpgradeRank(MirrorId) >= 2 && FVector::DistSquared(AttackOrigin, GetActorLocation()) > FMath::Square(320.f)) CaptureEnergy(Damage);
            Recoil = FMath::Min(1.5f, Recoil + .65f);
            PlayCombatSound(GuardSound, .85f, .92f);
            SetCombatMessage(FString::Printf(TEXT("BLOCKED - %d guard pieces / spent piece rebuilding"), GetAttachedPieceCount()), .9f);
        }
        UpdateWeapon(0.f);
        return; // A present facing piece fully protects against one ordinary hit.
    }
    if (bGuarding && bUnblockable) SetCombatMessage(TEXT("HEAVY ATTACK - evade the red tell"), 1.5f);

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
    const FString Action = HasUpgrade(RamId) ? TEXT("F RAM / Q RECALL") : TEXT("F HEAVY / Q RECALL");
    return SpecialCooldown > 0.f ? FString::Printf(TEXT("%s (%.1fs)"), *Action, SpecialCooldown) : Action;
}

FString ADBCharacter::GetUpgradeDescription(FName Id, int32 Rank)
{
    Rank = FMath::Clamp(Rank, 1, 3);
    if (Id == MirrorId)
        return Rank == 1 ? TEXT("RMB within 0.20s before a hit: spend 1 piece and store force (max 2). Your next piece/core hit adds 180% of the blocked damage.")
            : Rank == 2 ? TEXT("RMB perfect block (0.245s), or an ordinary distant block: spend 1 piece and store force (max 3). Next physical hit releases it.")
            : TEXT("RMB precise or distant blocks store force (max 4). Your next physical hit releases it and arcs half the hit into 2 nearby enemies.");
    if (Id == RamId)
        return Rank == 1 ? TEXT("F: committed bash and forward rush, even with no armor pieces held. Launched pieces gain damage and speed. Q always recalls.")
            : Rank == 2 ? TEXT("F: rush every 1.3s. One landed bash/rush advances one rebuilding piece by 0.85s, once per rush. Launched pieces gain damage/speed.")
            : TEXT("F: wider rush catches flankers; one hit advances a rebuild by 0.85s. Cooldown 1.3s. Launched pieces have wider contact sweeps.");
    if (Id == EchoId)
        return Rank == 1 ? TEXT("AUTOMATIC: each direct piece or core hit repeats a local impact after 0.23s at 55% damage. Enemies can move out of the echo.")
            : Rank == 2 ? TEXT("AUTOMATIC: direct piece/core hits leave a 55% impact echo after 0.23s. It follows the struck target up to 2.4m.")
            : TEXT("AUTOMATIC: direct piece/core hits leave two local echoes at 0.23s and 0.46s. They follow the target a short distance.");
    if (Id == FrostId)
        return Rank == 1 ? TEXT("R selects Frost. Outbound pieces chill for 4s; Q returns them to shatter chilled foes. A close core/rim hit also shatters.")
            : Rank == 2 ? TEXT("R selects Frost: throw to chill, Q return or strike to shatter. Every third direct contact spreads frost nearby.")
            : TEXT("R selects Frost: throw to chill, Q return or strike to shatter. Shatters create a local burst; every third contact spreads frost.");
    if (Id == EmberId)
        return Rank == 1 ? TEXT("R selects Ember. AUTOMATIC: piece and core hits burn enemies for 4s. Recall through targets to set more fires while repositioning.")
            : Rank == 2 ? TEXT("R selects Ember. Hits burn for 4s; every third direct piece/core contact spreads flame into nearby enemies.")
            : TEXT("R selects Ember. Hits burn for 4s; directly killing a burning foe causes a fire explosion. Every third contact spreads flame.");
    if (Id == StormId)
        return Rank == 1 ? TEXT("R selects Storm. Hit a foe twice within 5s: lightning arcs to up to 2 other visible foes within 6.2m. A lone target has no arc.")
            : Rank == 2 ? TEXT("R selects Storm. Repeat hits within 5s arc to 2 nearby visible foes. Every third direct contact also spreads storm marks.")
            : TEXT("R selects Storm. Repeat hits chain lightning; every third contact spreads marks. Direct storm hits advance one rebuild by 0.18s.");
    if (Id == FractureId)
        return Rank == 1 ? TEXT("AUTOMATIC: return/held impacts consume existing chill and storm marks in a finite nearby burst. Combine with Frost or Storm.")
            : Rank == 2 ? TEXT("AUTOMATIC: impacts consume chill/storm into a burst and add an elemental aftershock. Throw Frost, then Q recall through the pack.")
            : TEXT("AUTOMATIC: impacts consume chill/storm into a burst. Every direct elemental contact also creates a local aftershock.");
    if (Id == SplitId)
        return Rank == 1 ? TEXT("AUTOMATIC: each launched piece ricochets once toward another enemy ahead, or off a wall without Anchor. Q recalls survivors.")
            : Rank == 2 ? TEXT("AUTOMATIC: each launched piece can ricochet twice toward nearby enemies. Q recalls survivors along a separate damaging path.")
            : TEXT("AUTOMATIC: each launched piece gets 3 ricochets and can seek wider angles. Q recalls survivors; pieces retained still guard.");
    if (Id == CapacitorId)
        return Rank == 1 ? TEXT("RMB perfect block: spend 1 piece and empower 3 physical contacts by 75%. New rebuild timers take 2.6s instead of 3s.")
            : Rank == 2 ? TEXT("RMB perfect block empowers 5 contacts by 75%. Force storage gains 2 slots; new piece rebuild timers take 2.6s.")
            : TEXT("RMB perfect block empowers 5 hits; each empowered hit restores your dash. Force storage +2; new rebuilds take 2.6s.");
    if (Id == AnchorId)
        return Rank == 1 ? TEXT("Let a thrown piece stop: its visible frontal arc blocks bolts (45 integrity each). Q recalls cover. Broken pieces rebuild; heavy attacks bypass.")
            : Rank == 2 ? TEXT("Lodged arcs block bolts (60 integrity each), storing force for your next physical hit. Q recalls cover; broken pieces rebuild.")
            : TEXT("Lodged arcs block bolts (75 integrity each) and bank force. A destroyed anchor bursts nearby, then rebuilds. Q recalls survivors.");
    return TEXT("");
}

void ADBCharacter::OnRunReset()
{
    SuspendCombatInput();
    for (int32 Index = 0; Index < ShieldPieceCount; ++Index)
    {
        ADBThrownShield* Flight = PieceFlights[Index];
        PieceFlights[Index] = nullptr;
        PieceStates[Index] = EDBShieldPieceState::Attached;
        PieceRegenRemaining[Index] = 0.f;
        PieceDockPulse[Index] = 0.f;
        if (IsValid(Flight)) Flight->Destroy();
    }
    ReceivedAttackKeys.Reset();
    NextBlockPiece = 0;
    CatchSoundCooldown = 0.f;
    bReforgeUsed = false;
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
        Part->SetHiddenInGame(Rank == 0);
        // Installed sockets live on the permanent hub/forearm. They never masquerade
        // as extra throwables or float at the positions of missing armor pieces.
        Part->SetRelativeLocation(FVector(Locations[Index].X - 4.f, Locations[Index].Y * .38f, Locations[Index].Z * .38f));
        Part->SetRelativeRotation(Index >= 3 && Index <= 6 ? FRotator(-20.f,0.f,0.f) : FRotator::ZeroRotator);
        if (const UStaticMesh* PartMesh = Part->GetStaticMesh())
        {
            const FVector Size = PartMesh->GetBounds().BoxExtent * 2.f;
            const FVector Desired = Sizes[Index] * .6f * (1.f + .1f * FMath::Max(0, Rank - 1));
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
    FVector Pose = FMath::Lerp(FVector(108,-50,-16), FVector(96,-46,-12), GuardBlend);
    FRotator Rotation = FMath::Lerp(FRotator(-8,-34,-12), FRotator(0,-7,-8), GuardBlend);

    if (bWantsFire)
    {
        const float Anticipation = FMath::Clamp(ChargeHeld / .22f, 0.f, 1.f) * .65f + ThrowCharge * .35f;
        Pose += FVector(-10,-2,4) * Anticipation;
        Rotation += FRotator(-4,-7,-8) * Anticipation;
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
    WeaponCore->SetHiddenInGame(false);
    ShieldHub->SetHiddenInGame(false);
    WeaponCore->SetRelativeRotation(FRotator(0,0,BobPhase * (bWantsFire ? 12.f : 2.f)));
    CoreLight->SetVisibility(true);
    CoreLight->SetIntensity(40.f + ThrowCharge * 70.f + ParryFlashTime * 220.f + (StoredShots + EmpoweredShots > 0 ? 55.f : 0.f));
    CoreLight->SetLightColor(StoredShots + EmpoweredShots > 0 ? FLinearColor(1.f,.65f,.1f) : ElementColor(CurrentElement));
    for (int32 Index = 0; Index < ShieldPlates.Num(); ++Index)
    {
        const EDBShieldPieceState State = PieceStates[Index];
        const bool bAttached = State == EDBShieldPieceState::Attached || State == EDBShieldPieceState::Selected;
        const bool bSelected = State == EDBShieldPieceState::Selected;
        const float Regen = GetPieceRegenerationProgress(Index);
        ShieldPlates[Index]->SetHiddenInGame(!bAttached);
        ShieldPlates[Index]->SetRelativeLocation(FVector::ZeroVector);
        ShieldPlates[Index]->SetRelativeRotation(FRotator(0.f, 0.f, Index * 60.f));
        ShieldPlates[Index]->SetRelativeScale3D(FVector(1.f + PieceDockPulse[Index] * .025f));
        UStaticMeshComponent* Glow = PieceGlows[Index];
        Glow->SetHiddenInGame(!(bSelected || (bGuarding && bAttached) || PieceDockPulse[Index] > 0.f || Regen > .05f));
        Glow->SetRelativeRotation(FRotator(0.f, 0.f, Index * 60.f));
        Glow->SetRelativeScale3D(FVector(State == EDBShieldPieceState::Regenerating ? .3f + .7f * Regen : 1.f));
        Glow->SetMaterial(0, bSelected ? (SelectedPieceMaterial ? SelectedPieceMaterial.Get() : CoreMaterial.Get())
            : State == EDBShieldPieceState::Regenerating || PieceDockPulse[Index] > 0.f ? FrostMaterial.Get() : GetElementMaterial(CurrentElement));
    }
    const FName Ids[] = { MirrorId, RamId, EchoId, FrostId, EmberId, StormId, FractureId, SplitId, CapacitorId, AnchorId };
    for (int32 Index = 0; Index < AttachmentParts.Num(); ++Index)
        AttachmentParts[Index]->SetHiddenInGame(!HasUpgrade(Ids[Index]));
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
