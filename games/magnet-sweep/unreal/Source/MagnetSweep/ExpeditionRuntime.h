#pragma once

#include "CoreMinimal.h"
#include "InputCoreTypes.h"

class AMagnetWorkbench;
class APlayerController;
class UCanvas;
class UStaticMeshComponent;
class UAudioComponent;
class FJsonObject;
struct FWorkbenchImpl;
namespace MagnetSweep { class FExpeditionRig; class FExpeditionWorld; }
namespace MagnetSweep { struct FExpeditionCommand; }

// The same gate is used by physical key dispatch and automation. A cancelled
// aim never survives until an unrelated key-up, and key repeat cannot buy twice.
struct FExpeditionInputGate
{
    int32 ArmedSlot = INDEX_NONE;
    uint8 Held = 0;
    bool bField = false;
    bool BeginAim(int32 Slot)
    {
        if (Slot < 0 || Slot > 1 || (Held & (1 << Slot))) return false;
        Held |= 1 << Slot;
        ArmedSlot = Slot;
        bField = false;
        return true;
    }
    int32 EndAim(int32 Slot)
    {
        if (Slot < 0 || Slot > 1) return INDEX_NONE;
        Held &= ~(1 << Slot);
        if (ArmedSlot != Slot) return INDEX_NONE;
        ArmedSlot = INDEX_NONE;
        return Slot;
    }
    void Cancel() { ArmedSlot = INDEX_NONE; bField = false; }
    void LoseFocus() { Cancel(); Held = 0; }
};

enum class EExpeditionScreen : uint8 { Starter, Depot, Site, Victory, Retreated };
struct FExpeditionButton { FBox2D Rect; FString Label; int32 Id = 0; bool bEnabled = true; };
struct FExpeditionVisual
{
    UStaticMeshComponent* Mesh = nullptr;
    UStaticMeshComponent* Detail = nullptr;
    FVector Previous = FVector::ZeroVector;
    float Spin = 0;
};

struct FExpeditionRuntime
{
    explicit FExpeditionRuntime(AMagnetWorkbench* InOwner);
    ~FExpeditionRuntime();
    void Start();
    void Stop();
    void Tick(float Delta);
    void Paint(UCanvas* Canvas);
    bool Save();
    bool Load();
    FString EncodeSave() const;
    bool DecodeSave(const FString& Text, FString& Error);
    void RetrySite();

    AMagnetWorkbench* Owner = nullptr;
    APlayerController* PC = nullptr;
    TUniquePtr<FWorkbenchImpl> Display;
    TUniquePtr<MagnetSweep::FExpeditionRig> Rig;
    TUniquePtr<MagnetSweep::FExpeditionWorld> World;
    FExpeditionInputGate Input;
    EExpeditionScreen Screen = EExpeditionScreen::Starter;
    TSharedPtr<FJsonObject> SiteEntry;
    int32 SiteIndex = 0;
    int32 SelectedModule = INDEX_NONE;
    int32 RelaySource = INDEX_NONE;
    int32 SelectedCargoId = INDEX_NONE;
    TArray<int32> WeldSelection;
    bool bPrepareWeld = false;
    bool bPrecision = false;
    int32 ToolOperation[2] = {0,0};
    int32 PreparedSlot = INDEX_NONE;
    int32 PreparationStage = 0;
    int32 PreparedPartner = INDEX_NONE;
    FVector2D PreparedPoint = FVector2D::ZeroVector;
    FVector2D PreparedBend = FVector2D::ZeroVector;
    bool bPaused = false;
    bool bWasFocused = true;
    bool bWorldHit = false;
    bool bMuted = false;
    bool bConfirmNew = false;
    bool bContinuousField = false;
    bool bSaveAllowed = true;
    bool bLoadedBackup = false;
    float Elapsed = 0;
    float SaveElapsed = 0;
    float NoticeLife = 0;
    float Impact = 0;
    FVector2D Magnet = FVector2D(0, -240);
    FVector2D Aim = Magnet;
    FVector2D Pointer = FVector2D::ZeroVector;
    FString Notice;
    FString Profile = TEXT("expedition_preview");
    FString SavePath;
    FDelegateHandle InputHandle;
    TArray<FExpeditionButton> Buttons;
    TMap<int32, FExpeditionVisual> Visuals;
    TArray<UStaticMeshComponent*> MechanismShapes;
    UStaticMeshComponent* PressVisual = nullptr;
    UStaticMeshComponent* ArmVisual = nullptr;
    TWeakObjectPtr<UAudioComponent> Music;
    TWeakObjectPtr<UAudioComponent> FieldAudio;

    void UpdatePointer();
    void Key(const FKey& Key, bool bPressed, bool bRepeat = false);
    void Click(int32 Id);
    void Act(int32 Slot);
    MagnetSweep::FExpeditionCommand CommandFor(int32 Slot) const;
    MagnetSweep::FExpeditionCommand FieldCommand() const;
    void StartField();
    void RefreshCargoSelection();
    void CycleCargo();
    FString RecoveryHint() const;
    void ClearPreparation();
    FString PreparationHint(int32 Slot) const;
    bool PrepareAction(int32 Slot);
    void BankOrDeliver();
    void Depart();
    void SetPaused(bool bValue);
    void Show(const FString& Text, FName Cue = NAME_None);
    void UpdateVisuals(float Delta);
    void BuildMechanismVisuals();
    void RenderWorld(UCanvas* Canvas);
    void RenderDepot(UCanvas* Canvas);
    void AddButton(UCanvas* Canvas, int32 Id, const FString& Label, float X, float Y,
                   float Width, float Height, bool bEnabled = true);
    void Text(UCanvas* Canvas, const FString& Value, float X, float Y, float Scale = 1,
              FLinearColor Color = FLinearColor(.88f,.95f,.91f), bool bLarge = false);
    void Wrap(UCanvas* Canvas, const FString& Value, float X, float Y, int32 Characters = 55,
              float Scale = 1, FLinearColor Color = FLinearColor(.63f,.76f,.74f));
    TSharedPtr<FJsonObject> MakeState(bool bIncludeCheckpoint) const;
    bool RestoreState(const TSharedPtr<FJsonObject>& State, FString& Error, bool bRestoreCheckpoint);
};
