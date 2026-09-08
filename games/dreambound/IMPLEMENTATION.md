# Beta integration contract
Updated 2026-09-08. Current task owns integration. All workers share the workspace; do not revert other edits.

## Owned paths
- Integration: project metadata/Config, DBGameMode.*, DBHUD.*, DBTypes.h, world generation, progression/save, build scripts and shared records.
- Combat worker: Source/Dreambound/DBCharacter.h and .cpp only; own optional separate combat helper files.
- Enemy worker: Source/Dreambound/DBEnemy.h/.cpp and DBProjectile.h/.cpp only.
- Art worker: art/ and scripts/create_art.py plus scripts/import_art.py; return asset manifest rows in art/manifest-rows.csv. Importer may create Content/Art only; no project Config or map ownership.

## Shared names
Project games/dreambound/unreal/Dreambound.uproject; module DREAMBOUND_API.
Runtime C++ files at unreal/Source/Dreambound. DBTypes.h supplies EDBElement and FDBHit.
ADBCharacter public: ApplyUpgrade(FName), HasUpgrade(FName) const, GetUpgradeRank(FName) const, ReceiveAttack(float Damage, FVector Source, bool bUnblockable=false, AActor* Attacker=nullptr), GetAimDirection() const, GetMuzzleLocation() const. Expose Health, MaxHealth, GuardEnergy, MaxGuardEnergy, bGuarding, bDead, CurrentElement, TMap<FName,int32> Upgrades, FString LastCombatMessage and float MessageTime. Public OnRunReset and Capture/restore health/upgrades through these fields. Guard/parry determines received damage; projectiles call ReceiveAttack.
ADBEnemy public: EDBEnemyKind Kind (Melee,Caster,Hunter,Boss), int32 RoomId, bool bDead, float Health/MaxHealth; Configure(EDBEnemyKind,int32,float); ApplyCombatHit(const FDBHit&); Tick/AI combat. Enemy calls game mode NotifyEnemyKilled(this) exactly once. All actors use world coordinates in centimeters, +X forward,+Z up.
ADBProjectile public: Initialize(FVector Direction,float Speed,float Damage,bool bUnblockable,AActor* OwnerEnemy, FLinearColor Color). Enemy projectiles have travel, collision and finite life. Reflect/capture handling lives in Character ReceiveAttack; returning captured payload uses aimed player hit/effects.
ADBGameMode public: NotifyEnemyKilled(ADBEnemy*), NotifyPlayerDied(), NotifyEvent(const FString&,FLinearColor=FLinearColor::White), Interact(), ChooseReward(int32 ZeroBasedIndex), TogglePause(), ToggleBuild(), StartNewRun(bool bSameSeed=false), AdjustSensitivity(float Delta). Exposes bPaused,bChoosingReward,bShowingBuild. Player binds E/1/2/3/Esc/Tab via these functions. Mode pauses damage/progression during reward menus and handles cursor/focus. Use UE includes for Color, game mode classes as needed.

## Art naming and geometry
Content/Art/Meshes/SM_WeaponBody, SM_ShieldPlate, SM_Core, SM_Forearm, SM_GuardianBody, SM_GuardianHead, SM_GuardianArm, SM_GuardianLeg, SM_StoneTile, SM_Wall, SM_Pillar, SM_Arch, SM_Bell, SM_Root, SM_Crate, SM_Crystal, SM_TechPanel, SM_Grass.
Original Blender meshes FBX, materials unified on Unreal import. Weapon mesh origin at grip, +X points at target, dimensions approximately 60 cm long and 22 cm wide; plate separate for deployment. Forearm extends toward -X. Enemies approximately 180cm tall with separate parts pivoted for animation. Document part transforms and dimensions in art/README.md. Procedural generated source assets under art/generated; imported assets under Content/Art. Source script is authoritative and outputs reproducible.
Return real artifact paths and observed evidence. No claims of engine import, visual quality or gameplay unless actually exercised.
