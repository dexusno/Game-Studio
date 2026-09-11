#include "DBEnemyMotionStudy.h"
#include "DBGameMode.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "DBProjectile.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Dom/JsonObject.h"
#include "DrawDebugHelpers.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "GameFramework/PlayerController.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformTime.h"
#include "Misc/App.h"
#include "Misc/CommandLine.h"
#include "Misc/FileHelper.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "UnrealClient.h"

ADBEnemyMotionStudy::ADBEnemyMotionStudy()
{
    PrimaryActorTick.bCanEverTick = true;
    PrimaryActorTick.bTickEvenWhenPaused = true;
    // Enemy/movement run in PrePhysics. Follow their completed pose before the
    // world's camera-manager update; PostUpdateWork would lag the view a frame.
    PrimaryActorTick.TickGroup = TG_PostPhysics;
}

void ADBEnemyMotionStudy::BeginPlay()
{
    Super::BeginPlay();
    Mode = Cast<ADBGameMode>(GetWorld()->GetAuthGameMode());
    KindName = TEXT("Melee"); ViewName = TEXT("Side");
    FParse::Value(FCommandLine::Get(), TEXT("DBCreature="), KindName);
    FParse::Value(FCommandLine::Get(), TEXT("DBCreatureView="), ViewName);
    FParse::Value(FCommandLine::Get(), TEXT("DBCreatureStudySeconds="), Duration);
    Duration = FMath::Clamp(Duration,4.f,32.f);
    bFootMarkers = FParse::Param(FCommandLine::Get(), TEXT("DBFootMarkers"));
    EDBEnemyKind Kind = KindName == TEXT("Caster") ? EDBEnemyKind::Caster
        : KindName == TEXT("Hunter") ? EDBEnemyKind::Hunter
        : KindName == TEXT("Boss") ? EDBEnemyKind::Boss : EDBEnemyKind::Melee;
    KindName = StaticEnum<EDBEnemyKind>()->GetNameStringByValue(static_cast<int64>(Kind));
    if (ViewName != TEXT("Front") && ViewName != TEXT("Player") && ViewName != TEXT("LowSide")) ViewName = TEXT("Side");
    Output = FPaths::ConvertRelativePathToFull(FPaths::ProjectSavedDir() / TEXT("EnemyMotionStudy") / (KindName + TEXT("-") + ViewName));
    IFileManager::Get().MakeDirectory(*Output, true);
    if (!Mode || !Mode->Player || Mode->Rooms.IsEmpty())
    {
        FailureReason = TEXT("Study setup is missing the game mode, player or room"); Finish(true); return;
    }
    Player = Mode->Player;
    Center = Mode->Rooms[0].Center;
    FActorSpawnParameters Spawn;
    Spawn.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
    for (TActorIterator<ADBEnemy> It(GetWorld()); It; ++It) It->Destroy();
    Mode->ClearRewardPractice();
    Mode->PracticeReward = TEXT("AnimationStudy");
    Mode->bSliceAwaitingStart = false;
    // The isolated study skips normal game-mode timers. Do not retain the
    // one-time arrival banner forever over the raised attack silhouette.
    Mode->EventRemaining = 0.f;
    const bool bBossStudy = Kind == EDBEnemyKind::Boss;
    Player->SetActorLocation(Center + (bBossStudy ? FVector(1050.f,900.f,110.f) : FVector(600.f,-600.f,110.f)));
    Player->GetCharacterMovement()->StopMovementImmediately();
    Creature = GetWorld()->SpawnActor<ADBEnemy>(Center + (bBossStudy ? FVector(-950.f,-750.f,110.f) : FVector(-500.f,-600.f,110.f)), FRotator::ZeroRotator, Spawn);
    if (!Creature) { FailureReason = TEXT("Enemy spawn failed"); Finish(true); return; }
    Creature->Configure(Kind, INDEX_NONE, 1.f);
    Creature->SetArenaBounds(Center, FVector2D(1420.f,1420.f));
    Creature->SetActorTickEnabled(false);
    Camera = GetWorld()->SpawnActor<ACameraActor>();
    Camera->GetCameraComponent()->FieldOfView = Kind == EDBEnemyKind::Boss ? 52.f : 44.f;
    if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
    {
        if (ViewName != TEXT("Player")) PC->SetViewTarget(Camera);
        Player->SetActorHiddenInGame(ViewName != TEXT("Player"));
        if (PC->GetHUD()) PC->GetHUD()->bShowHUD = ViewName == TEXT("Player");
    }
    // Every sampled frame is one actual rendered simulation step. Encoding does
    // not invent missing contact frames, and this mode is never a FPS benchmark.
    FApp::SetFixedDeltaTime(1.0 / 30.0);
    FApp::SetUseFixedTimeStep(true);
    if (GEngine) GEngine->SetMaxFPS(30.f);
    Samples = TEXT("frame,simulation_seconds,wall_seconds,dt,kind,phase,tell_remaining,tell_duration,aim_locked,vulnerable,health,speed_cm_s,visible_yaw,movement_blend,x,y,z,left_planted,right_planted,left_target_x,left_target_y,left_target_z,right_target_x,right_target_y,right_target_z,left_foot_x,left_foot_y,left_foot_z,right_foot_x,right_foot_y,right_foot_z,left_error_cm,right_error_cm,parent_unit_scale,projectiles,player_health,attack,attack_elapsed,camera_distance,camera_adjusted,right_hand_x,right_hand_y,right_hand_z,left_hand_x,left_hand_y,left_hand_z,right_shoulder_x,right_shoulder_y,right_shoulder_z,facing_x,facing_y,facing_z,pelvis_x,pelvis_y,pelvis_z,head_x,head_y,head_z,player_x,player_y,player_z,player_distance_cm,left_hand_target_x,left_hand_target_y,left_hand_target_z,right_hand_target_x,right_hand_target_y,right_hand_target_z,left_hand_error_cm,right_hand_error_cm,left_claw_contact_x,left_claw_contact_y,left_claw_contact_z,right_claw_contact_x,right_claw_contact_y,right_claw_contact_z\n");
    UpdateCamera(0.f);
}

void ADBEnemyMotionStudy::UpdateCamera(float DeltaSeconds)
{
    if (!IsValid(Creature) || !IsValid(Player)) return;
    if (ViewName == TEXT("Player"))
    {
        if (AController* Controller = Player->GetController())
            Controller->SetControlRotation((Creature->GetActorLocation() + FVector(0,0,35) - Player->GetPawnViewLocation()).Rotation());
        return;
    }
    const FVector Subject = Creature->GetActorLocation() + FVector(0,0,5);
    const FVector Offset = ViewName == TEXT("Front") ? FVector(730,-250,210)
        : ViewName == TEXT("LowSide") ? FVector(-120,-760,-15) : FVector(-120,-760,230);
    const FVector Desired = Subject + Offset * (KindName == TEXT("Boss") ? 1.12f : 1.f);
    FVector CameraPosition = Desired;
    FCollisionQueryParams Query(SCENE_QUERY_STAT(DBEnemyStudyCamera), false, Creature);
    Query.AddIgnoredActor(Player); Query.AddIgnoredActor(Camera);
    FHitResult Obstruction;
    bCameraObstructionAdjusted = GetWorld()->LineTraceSingleByObjectType(Obstruction,Subject,Desired,
        FCollisionObjectQueryParams(ECC_WorldStatic),Query);
    if (bCameraObstructionAdjusted)
        CameraPosition = Obstruction.Location + (Subject-Desired).GetSafeNormal() * 24.f;
    CameraDistance = FVector::Distance(Subject,CameraPosition);
    const float BaseFov = KindName == TEXT("Boss") ? 52.f : 44.f;
    const float FramingFov = FMath::RadiansToDegrees(2.f * FMath::Atan(FMath::Tan(FMath::DegreesToRadians(BaseFov * .5f))
        * FVector::Distance(Subject,Desired) / FMath::Max(80.f,CameraDistance)));
    Camera->GetCameraComponent()->FieldOfView = FMath::Clamp(FramingFov,BaseFov,88.f);
    Camera->SetActorLocation(CameraPosition);
    Camera->SetActorRotation((Subject - Camera->GetActorLocation()).Rotation());
}

void ADBEnemyMotionStudy::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bFinished) { FGenericPlatformMisc::RequestExit(false); return; }
    if (!IsValid(Creature) || !IsValid(Player) || !IsValid(Mode))
    {
        FailureReason = TEXT("A required study actor became invalid"); Finish(true); return;
    }
    if (Mode->bDefeated || Mode->bPaused || Mode->bTitle || Mode->bChoosingReward || Mode->bShowingBuild)
    {
        FailureReason = TEXT("Unexpected paused, menu or defeat state"); Finish(true); return;
    }
    UpdateCamera(DeltaSeconds);
    if (!bStarted)
    {
        Warmup += DeltaSeconds;
        if (Warmup < 3.f) return;
        bStarted = true; StartedAt = FPlatformTime::Seconds();
        Creature->SetActorTickEnabled(true);
        return; // Frame zero follows a complete enemy AI, movement and pose tick.
    }
    if (FPlatformTime::Seconds() - StartedAt > 180.f) { FailureReason = TEXT("Capture exceeded its wall-time limit"); Finish(true); return; }
    PlayerHealthBeforeRestore = Player->Health;
    Player->Health = Player->MaxHealth;
    Player->bDead = false;
    // Only the player's destination is choreographed. The non-practice enemy
    // uses its normal approach, steering, tell, damage, recovery and death code.
    FVector Goal(800,-600,0);
    if (Time < 4.f) Goal = FVector(600 + 50.f*Time,-600,0);
    else if (Time < 8.f) Goal = FVector(800,-600,0);
    else if (Time < 10.5f) Goal = FMath::Lerp(FVector(800,-600,0),FVector(200,650,0),(Time-8.f)/2.5f);
    else if (Time < 14.f) Goal = FVector(200,650,0);
    else if (Time < 16.5f) Goal = FMath::Lerp(FVector(200,650,0),FVector(-800,250,0),(Time-14.f)/2.5f);
    else if (Time < 22.f) Goal = FVector(-800,250,0);
    else
    {
        // Draw the final exchange back across paving so ground contact during
        // the collapse can be inspected without tall garden props hiding it.
        Goal = KindName == TEXT("Caster") ? FVector(300,800,0) : FVector(400,-650,0);
    }
    if (KindName == TEXT("Boss"))
    {
        // Begin outside ranged attack distance to expose the large rig's gait,
        // then enter normal close attack range to exercise its actual Slam.
        if (Time < 4.f) Goal = FVector(1050,900,0);
        else if (Time >= 14.f)
            Goal = Creature->GetActorLocation() - Center + Creature->GetActorForwardVector() * 220.f;
    }
    Goal += Center; Goal.Z = Player->GetActorLocation().Z;
    if (KindName == TEXT("Boss") && Time >= 14.f && BossCloseTargetFrame == INDEX_NONE)
    {
        // The fountain can block the diagnostic target's cross-court path.
        // Relocate only this scripted player once onto the clear paving ahead
        // of the boss, then let normal distance selection choose its close Slam.
        Player->SetActorLocation(Goal,false);
        Player->GetCharacterMovement()->StopMovementImmediately();
        BossCloseTargetFrame = Frame;
    }
    Player->SetActorLocation(FMath::VInterpConstantTo(Player->GetActorLocation(),Goal,DeltaSeconds,620.f),true);
    auto Hit = [&](bool bSide, float Damage)
    {
        FDBHit Impact;
        Impact.Damage = Damage; Impact.bImpact = true; Impact.InstigatorActor = Player;
        Impact.Source = Creature->GetActorLocation() + (bSide ? Creature->GetActorRightVector() : Creature->GetActorForwardVector()) * 250.f;
        Impact.Direction = (Creature->GetActorLocation()-Impact.Source).GetSafeNormal();
        Creature->ApplyCombatHit(Impact);
    };
    if (Time >= 20.f && !bFrontHit) { Hit(false,12.f); bFrontHit = true; }
    if (Time >= 22.f && !bSideHit) { Hit(true,12.f); bSideHit = true; }
    if (Time >= 28.f && !bDeathTriggered) { Hit(true,10000.f); bDeathTriggered = true; }
    RecordFrame(DeltaSeconds);
    Time += DeltaSeconds;
    if (Time >= Duration) Finish(false);
}

void ADBEnemyMotionStudy::RecordFrame(float DeltaSeconds)
{
    if (FScreenshotRequest::IsScreenshotRequested()) { ++MissedReadbacks; return; }
    const FDBEnemyAnimationDebug State = Creature->GetAnimationDebugState();
    if (bFootMarkers)
    {
        for (const FVector& Target : {State.left_target_world,State.right_target_world})
        {
            DrawDebugSphere(GetWorld(),Target,3.5f,8,FColor::Green,false,0.f,1,1.5f);
            DrawDebugLine(GetWorld(),Target-FVector(0,0,8),Target+FVector(0,0,8),FColor::White,false,0.f,1,1.5f);
        }
        // These points follow measured source skin vertices through the hand
        // bones. They expose skin contact separately from ankle/wrist targets.
        for (const FVector& Contact : {State.left_claw_contact_world,State.right_claw_contact_world})
        {
            if (Contact.IsNearlyZero()) continue;
            DrawDebugSphere(GetWorld(),Contact,3.f,8,FColor::Magenta,false,0.f,1,1.5f);
            DrawDebugLine(GetWorld(),Contact-FVector(0,0,8),Contact+FVector(0,0,8),FColor::Magenta,false,0.f,1,1.5f);
        }
    }
    const FVector At = Creature->GetActorLocation();
    const FString Phase = StaticEnum<EDBEnemyPhase>()->GetNameStringByValue(static_cast<int64>(Creature->Phase));
    PhaseSamples.FindOrAdd(Phase)++;
    int32 Projectiles = 0;
    for (TActorIterator<ADBProjectile> It(GetWorld()); It; ++It) ++Projectiles;
    Samples += FString::Printf(TEXT("%d,%.6f,%.6f,%.6f,%s,%s,%.5f,%.5f,%d,%d,%.3f,%.3f,%.3f,%.4f,%.4f,%.4f,%.4f,%d,%d,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.4f,%.3f,%d,%.3f,%s,%.5f,%.3f,%d"),
        Frame,Time,FPlatformTime::Seconds()-StartedAt,DeltaSeconds,*KindName,*Phase,Creature->TellTime,Creature->TellDuration,
        Creature->bAimLocked?1:0,Creature->bVulnerable?1:0,Creature->Health,State.speed_cm_s,State.visible_yaw,State.movement_blend,
        At.X,At.Y,At.Z,State.left_planted?1:0,State.right_planted?1:0,
        State.left_target_world.X,State.left_target_world.Y,State.left_target_world.Z,
        State.right_target_world.X,State.right_target_world.Y,State.right_target_world.Z,
        State.left_foot_world.X,State.left_foot_world.Y,State.left_foot_world.Z,
        State.right_foot_world.X,State.right_foot_world.Y,State.right_foot_world.Z,
        State.left_reach_error_cm,State.right_reach_error_cm,State.parent_unit_scale,Projectiles,PlayerHealthBeforeRestore,*State.attack_name,State.attack_elapsed,CameraDistance,bCameraObstructionAdjusted?1:0);
    for (const FVector& Point : {State.right_hand_world,State.left_hand_world,State.right_shoulder_world,
        State.facing_forward,State.pelvis_world,State.head_world,Player->GetActorLocation()})
        Samples += FString::Printf(TEXT(",%.4f,%.4f,%.4f"),Point.X,Point.Y,Point.Z);
    Samples += FString::Printf(TEXT(",%.4f"),FVector::Dist2D(At,Player->GetActorLocation()));
    for (const FVector& Target : {State.left_hand_target_world,State.right_hand_target_world})
        Samples += FString::Printf(TEXT(",%.4f,%.4f,%.4f"),Target.X,Target.Y,Target.Z);
    Samples += FString::Printf(TEXT(",%.4f,%.4f"),State.left_hand_reach_error_cm,State.right_hand_reach_error_cm);
    for (const FVector& Contact : {State.left_claw_contact_world,State.right_claw_contact_world})
        Samples += FString::Printf(TEXT(",%.4f,%.4f,%.4f"),Contact.X,Contact.Y,Contact.Z);
    Samples += TEXT("\n");
    FScreenshotRequest::RequestScreenshot(Output/FString::Printf(TEXT("Frame_%05d.png"),Frame++),false,false);
}

void ADBEnemyMotionStudy::Finish(bool bAborted)
{
    if (bFinished) return;
    bFinished = true;
    FFileHelper::SaveStringToFile(Samples,*(Output/TEXT("Frames.csv")));
    TSharedRef<FJsonObject> Report = MakeShared<FJsonObject>();
    Report->SetBoolField(TEXT("complete"),!bAborted);
    Report->SetBoolField(TEXT("aborted"),bAborted);
    Report->SetStringField(TEXT("failure_reason"),FailureReason);
    Report->SetStringField(TEXT("scope"),TEXT("Actual non-practice enemy AI and animation; choreographed invulnerable player target; isolated fixed30Hz simulation with one screenshot requested per rendered tick. Not ordinary input, subjective owner acceptance or a performance benchmark."));
    Report->SetStringField(TEXT("creature"),KindName); Report->SetStringField(TEXT("view"),ViewName);
    Report->SetStringField(TEXT("target_path"),KindName == TEXT("Boss") ? TEXT("boss-approach-ranged-close-v3") : TEXT("travel-turn-hit-death-v1"));
    Report->SetNumberField(TEXT("scripted_player_close_relocation_frame"),BossCloseTargetFrame);
    Report->SetBoolField(TEXT("foot_target_markers"),bFootMarkers);
    Report->SetBoolField(TEXT("claw_skin_contact_markers"),bFootMarkers);
    Report->SetStringField(TEXT("observer_tick_stage"),TEXT("PostPhysics, before camera-manager update"));
    Report->SetNumberField(TEXT("simulation_seconds"),Time); Report->SetNumberField(TEXT("frames"),Frame);
    Report->SetNumberField(TEXT("requested_seconds"),Duration);
    Report->SetNumberField(TEXT("missed_readbacks"),MissedReadbacks);
    Report->SetNumberField(TEXT("wall_seconds"),StartedAt>0?FPlatformTime::Seconds()-StartedAt:0);
    TSharedRef<FJsonObject> Phases = MakeShared<FJsonObject>();
    for (const auto& Pair:PhaseSamples) Phases->SetNumberField(Pair.Key,Pair.Value);
    Report->SetObjectField(TEXT("phase_samples"),Phases);
    FString Json;
    const TSharedRef<TJsonWriter<>> Writer = TJsonWriterFactory<>::Create(&Json);
    FJsonSerializer::Serialize(Report,Writer);
    FFileHelper::SaveStringToFile(Json,*(Output/TEXT("Capture.json")));
}
