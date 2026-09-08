#include "DBGameMode.h"
#include "DBCharacter.h"
#include "DBEnemy.h"
#include "DBProjectile.h"
#include "Engine/World.h"
#include "EngineUtils.h"

FVector ADBGameMode::GetRoomEntryPoint(int32 Index) const
{
 if(!Rooms.IsValidIndex(Index))return FVector(0,0,110);
 const auto& R=Rooms[Index];
 if(R.Parent<0)return R.Center+FVector(-1540,-800,110);
 const FVector Back=(Rooms[R.Parent].Center-R.Center).GetSafeNormal2D();
 return R.Center+Back*1360+FVector(0,0,110);
}

void ADBGameMode::ClearRewardPractice()
{
 for(TActorIterator<ADBProjectile> It(GetWorld());It;++It)
  if(PracticeActors.Contains(Cast<ADBEnemy>(It->GetOwner())))It->Destroy();
 for(const auto& Actor:PracticeActors)if(IsValid(Actor))Actor->Destroy();
 PracticeActors.Reset();PracticeReward=NAME_None;PracticeInstruction.Reset();
 for(TActorIterator<AActor> It(GetWorld());It;++It)if(It->ActorHasTag(TEXT("DBTransientCombatEffect")))It->Destroy();
}

void ADBGameMode::BeginRewardPractice(FName Reward)
{
 ClearRewardPractice();if(!Player||!Rooms.IsValidIndex(CurrentRoomId)||Reward=="Restore")return;
 PracticeReward=Reward;
 if(Reward=="Mirror")PracticeInstruction="Hold RMB just before a bolt hits: MIRROR gains a charge. Hit a target with LMB or a launched piece to spend it.";
 else if(Reward=="Anchor")PracticeInstruction="Hold/release LMB to lodge pieces across the caster's bolt path. They block bolts until broken. Q recalls survivors.";
 else if(Reward=="Ram")PracticeInstruction="Approach a target and press F: RAM rushes and strikes. The F indicator shows its cooldown. Q only recalls pieces.";
 else if(Reward=="Frost")PracticeInstruction="Launch pieces at a target to add blue ice marks. Recall through it with Q to shatter. Close strikes also chill then shatter.";
 else if(Reward=="Storm")PracticeInstruction="Hit a target twice within 5 seconds. The second hit sends visible lightning to its nearby neighbor. No extra button.";
 else if(Reward=="Ember")PracticeInstruction="Strike or launch at a target: orange fire burns after the hit. Hit again to see the burn refresh. No extra button.";
 else PracticeInstruction="Try your installed attachment here. Press Tab for its inputs and current-rank effects. Follow the passage when ready.";
 FActorSpawnParameters Params;Params.SpawnCollisionHandlingOverride=ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;
 const FVector C=Rooms[CurrentRoomId].Center;
 for(int32 I=0;I<2;++I){
  if(auto* Target=GetWorld()->SpawnActor<ADBEnemy>(C+FVector(-500,-500+I*420,110),FRotator(0,180,0),Params)){
   Target->Configure(EDBEnemyKind::Melee,INDEX_NONE,1.f);Target->bPracticeTarget=true;
   Target->Health=Target->MaxHealth=500.f;PracticeActors.Add(Target);
  }
 }
 if(Reward=="Mirror"||Reward=="Anchor"){
  if(auto* Caster=GetWorld()->SpawnActor<ADBEnemy>(C+FVector(700,-650,110),FRotator(0,180,0),Params)){
   Caster->Configure(EDBEnemyKind::Caster,INDEX_NONE,1.f);Caster->Health=Caster->MaxHealth=5000.f;
   Caster->SetArenaBounds(C,FVector2D(1500,1500));PracticeActors.Add(Caster);
  }
 }
 Player->Health=Player->MaxHealth;
}
