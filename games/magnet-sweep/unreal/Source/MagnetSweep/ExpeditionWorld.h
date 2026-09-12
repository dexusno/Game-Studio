#pragma once
#include "CoreMinimal.h"
#include "Dom/JsonObject.h"

namespace MagnetSweep
{
class FExpeditionRig;

enum class EExpeditionMaterial : uint8 { Iron, Copper, Alloy, HotCell, Core, Mechanism };
enum class EExpeditionBodyState : uint8 { Available, Pulling, Cargo, Banked, Consumed, Dispatched };
enum class EExpeditionAction : uint8 { Attract, Extract, Launch, Weld, Arc, Winch, Vector, Relay, Interact, Ground, ArmRelay, TransferHeat, SwitchAnchor, LayGuide };
enum class EExpeditionEventKind : uint8 { Action, Captured, Severed, Impact, Welded, Discharge, Mechanism, Dropped, Quench, Banked, Dispatched };

struct FExpeditionBody
{
    int32 Id = INDEX_NONE;
    FName Role;
    EExpeditionMaterial Material = EExpeditionMaterial::Iron;
    EExpeditionBodyState State = EExpeditionBodyState::Available;
    FVector2D Position = FVector2D::ZeroVector;
    FVector2D Velocity = FVector2D::ZeroVector;
    FVector2D CargoOffset = FVector2D::ZeroVector;
    FVector2D FlowDirection = FVector2D::ZeroVector; // Zero is bidirectional; a diode exposes its conducting direction.
    float Radius = 15.f;
    float Mass = 2.f;
    int32 Value = 4;
    int32 Appraisal = 4; // Conserved current payout; structural quality is separate.
    int32 Quality = 100;
    int32 Charge = 0;
    bool bAnchored = false;
    bool bConductive = true;
    bool bHot = false;
    bool bBrittle = false;
    bool bGoal = false;
    bool bLaunched = false;
    bool bRebounded = false;
    bool bPunched = false;
    bool bSinkUsed = false;
    bool bFunctional = false;
    float FuseDelay = -1.f;
    float RecoverDelay = 0.f;
    int32 RootAction = 0;
    TArray<int32> Links;
    TArray<int32> SourceIds;
};

struct FExpeditionCommand
{
    EExpeditionAction Action = EExpeditionAction::Attract;
    FVector2D Magnet = FVector2D::ZeroVector;
    FVector2D Aim = FVector2D::ZeroVector;
    FVector2D Destination = FVector2D::ZeroVector;
    FVector2D SecondaryPoint = FVector2D::ZeroVector;
    int32 TargetId = INDEX_NONE;
    int32 SecondaryId = INDEX_NONE;
    TArray<int32> BodyIds; // Optional explicit ammunition/field selection; otherwise deterministic aim.
    bool bPrecision = false;
    bool bHasSecondaryPoint = false;
};

struct FExpeditionPreview
{
    bool bAllowed = false;
    FString Reason;
    EExpeditionAction Action = EExpeditionAction::Attract;
    int32 BatteryCost = 0;
    float Mass = 0.f;
    int32 Value = 0;
    int32 TargetId = INDEX_NONE;
    TArray<int32> BodyIds;
    TArray<FVector2D> PathPoints;
    bool bUnsafe = false;
};

struct FExpeditionResult
{
    bool bSucceeded = false;
    FString Message;
    int32 BatterySpent = 0;
    int32 Output = 0;
    TArray<int32> BodyIds;
};

struct FExpeditionEvent
{
    EExpeditionEventKind Kind = EExpeditionEventKind::Action;
    int32 ActionId = 0;
    FVector2D Position = FVector2D::ZeroVector;
    int32 Amount = 0;
    TArray<int32> BodyIds;
    FString Message;
};

struct FExpeditionObstacle
{
    FVector2D Center = FVector2D::ZeroVector;
    FVector2D HalfSize = FVector2D(20, 20);
    FName Role;
};

enum class EExpeditionConstraintKind : uint8 { Counterweight, Reaction, Gantry };
struct FExpeditionConstraint
{
    EExpeditionConstraintKind Kind = EExpeditionConstraintKind::Counterweight;
    int32 BodyA = INDEX_NONE; // Supported payload.
    int32 BodyB = INDEX_NONE; // Real supporting mass.
    FVector2D Anchor = FVector2D::ZeroVector;
    FVector2D Offset = FVector2D::ZeroVector;
    bool bActive = true;
};

struct FExpeditionWorldState
{
    int32 Version = 2;
    int32 SiteIndex = 0;
    int32 Seed = 1;
    int32 Battery = 100;
    int32 Output = 0;
    int32 ActionSerial = 0;
    int32 NextBodyId = 0;
    float WorldTime = 0.f;
    float UnsafeElapsed = 0.f;
    float HazardCooldown = 0.f;
    float PullRemaining = 0.f;
    FVector2D Magnet = FVector2D::ZeroVector;
    bool bCollarReleased = false;
    bool bBallastCleared = false;
    bool bArmBraced = false;
    bool bCircuitClosed = false;
    bool bCounterweightUsed = false;
    bool bDispatched = false;
    bool bEvacuated = false;
    int32 TetherBody = INDEX_NONE;
    FVector2D TetherAnchor = FVector2D::ZeroVector;
    int32 SecondTetherBody = INDEX_NONE;
    FVector2D SecondTetherAnchor = FVector2D::ZeroVector;
    int32 LatchedBody = INDEX_NONE;
    FVector2D LatchPosition = FVector2D::ZeroVector;
    int32 RelayBody = INDEX_NONE;
    FVector2D RelayDestination = FVector2D::ZeroVector;
    float RelayRemaining = 0.f;
    int32 DeferredTerminal = INDEX_NONE;
    int32 DeferredSensor = INDEX_NONE;
    int32 GroundBody = INDEX_NONE;
    int32 DeferredCharge = 0;
    bool bDeferredWasSafe = false;
    FVector2D ShearOrigin = FVector2D::ZeroVector;
    FVector2D ShearNormal = FVector2D::ZeroVector;
    float ShearRemaining = 0.f;
    TArray<int32> ShearIds;
    int32 SecondRelayBody = INDEX_NONE;
    FVector2D SecondRelayDestination = FVector2D::ZeroVector;
    TArray<int32> RelayIds;
    TArray<int32> SecondRelayIds;
    TArray<FVector2D> GuidePoints;
    int32 GuideUses = 0;
    int32 GuideBody = INDEX_NONE;
    int32 GuideSegment = 0;
    float GuideSpeed = 0.f;
    TArray<int32> CycloneIds;
    FVector2D CycloneCenter = FVector2D::ZeroVector;
    TArray<FExpeditionConstraint> Constraints;
    TArray<int32> PullIds;
    TArray<FExpeditionBody> Bodies;
};

// Independent expedition simulation: no actors, old-career ledger or owner-save access.
// Runtime pauses by not calling Tick. Commands commit battery once; cancelled movement is not refunded.
class FExpeditionWorld
{
public:
    static constexpr float SafeCapacity = 24.f;
    static constexpr float HardCapacity = 36.f;
    static constexpr float MinX = -500.f, MaxX = 500.f, MinY = -300.f, MaxY = 300.f;
    static FVector2D FurnacePosition() { return FVector2D(650, 0); }
    static FVector2D ReceiverPosition() { return FVector2D(650, -240); }
    static FVector2D CollarStop() { return FVector2D(180, -120); }
    static FVector2D BallastCatch() { return FVector2D(80, 220); }
    static FVector2D ArmStopper() { return FVector2D(290, 210); }
    static FVector2D CircuitSocket() { return FVector2D(-60, -160); }
    static FVector2D CounterweightPad() { return FVector2D(-270, 210); }
    static FVector2D PressCenter() { return FVector2D(217.5, 0); }
    static FVector2D PressHalfSize() { return FVector2D(27.5, 130); }
    static FVector2D ArmPivot() { return FVector2D(390, 210); }
    FVector2D GetArmTip() const;

    FExpeditionWorld();
    void StartSite(int32 SiteIndex, int32 Seed, int32 InitialBattery = 100);
    void Tick(float Delta, const FVector2D& Magnet, const FExpeditionRig& Rig);
    FExpeditionPreview Preview(const FExpeditionCommand& Command, const FExpeditionRig& Rig) const;
    FExpeditionResult Execute(const FExpeditionCommand& Command, const FExpeditionRig& Rig);
    FExpeditionResult Drop(); // Always the whole haul, including pending attraction; no resource refund.
    void CancelPull(); // Field-off: retain secured cargo, release only objects still in flight.
    bool CanBank(FString& Reason) const;
    FExpeditionResult Bank(); // Scrap only; objective remains cargo.
    bool CanDispatch(FString& Reason) const;
    FExpeditionResult Dispatch();
    void Evacuate();
    const FExpeditionWorldState& GetState() const { return State; }
    const TArray<FExpeditionBody>& GetBodies() const { return State.Bodies; }
    const TArray<FExpeditionObstacle>& GetObstacles() const { return Obstacles; }
    const FExpeditionBody* FindBody(int32 Id) const;
    int32 FindBodyAt(const FVector2D& Position, float ExtraRadius = 20.f, bool bIncludeCargo = false) const;
    TArray<int32> GetGroup(int32 Id) const;
    float GetCargoMass() const;
    int32 GetCargoValue() const;
    bool IsUnsafe() const;
    bool IsEnded() const { return State.bDispatched || State.bEvacuated; }
    bool IsCoreReleased() const { return State.bCollarReleased && State.bBallastCleared && State.bArmBraced; }
    bool IsArmSafe() const;
    bool IsPressSafe() const;
    int32 GetBattery() const { return State.Battery; }
    int32 GetOutput() const { return State.Output; }
    FString GetGoalText() const;
    TArray<FName> GetOpportunityTags() const;
    static bool IsModuleImplemented(FName ModuleId);
    TArray<FExpeditionEvent> DrainEvents();
    TSharedPtr<FJsonObject> ToJson() const;
    bool FromJson(const TSharedPtr<FJsonObject>& Json, FString& Error); // Strict, transactional.
    bool CheckInvariants(FString& Error) const;

private:
    FExpeditionWorldState State;
    TArray<FExpeditionObstacle> Obstacles;
    TArray<FExpeditionEvent> Events;
    TArray<int32> PendingCyclones;
    FExpeditionBody* MutableBody(int32 Id);
    void AddEvent(EExpeditionEventKind Kind, const FString& Message, const FVector2D& Position, const TArray<int32>& Ids = {}, int32 Amount = 0);
    void BuildObstacles();
    void Sever(int32 Id, const FExpeditionRig& Rig);
    void UpdateMechanisms();
    void Integrate(float Delta, const FExpeditionRig& Rig);
    void Quench(const FString& Reason);
    void Impact(FExpeditionBody& Body, const FVector2D& Normal, float Speed, const FExpeditionRig& Rig);
    bool ArcPath(int32 Target, const FExpeditionRig& Rig, TArray<int32>& OutPath) const;
    TArray<int32> ConductiveNeighbors(int32 Id, const FExpeditionRig& Rig) const;
    bool CircuitLoop(const FExpeditionRig& Rig, TArray<int32>& Reachable) const;
    void FireTerminal(int32 Id);
    void ResolveSensor(int32 Id);
    void MakeCyclone(int32 BodyId);
    void UpdateConstraints(float Delta);
};
}
