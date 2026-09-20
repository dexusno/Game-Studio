#pragma once
#if FOUNDRY_WITH_CAMPAIGN
#include "CoreMinimal.h"
class AFoundryStage;

// Opt-in automation only. The Stage callback keeps screenshot scheduling in
// the existing host; every durable action uses its typed production seam.
bool TickFoundryCityProbe(AFoundryStage& Stage, TFunctionRef<void(const FString&)> Capture);
#endif
