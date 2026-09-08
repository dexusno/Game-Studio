#pragma once
#include "CoreMinimal.h"

class ADBGameMode;

/** Bounded staged-world QA, never a normal playthrough or fun assessment.
 * Requires -DBVerify and an explicitly isolated DBQA_/DreamboundQA save slot.
 * Returns X=passed, Y=failed and appends human-readable evidence.
 */
FIntPoint DBRunRuntimeChecks(ADBGameMode& Mode, FString& Report);
