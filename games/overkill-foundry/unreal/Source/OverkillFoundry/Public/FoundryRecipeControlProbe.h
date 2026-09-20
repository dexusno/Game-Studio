#pragma once
#if FOUNDRY_WITH_CAMPAIGN
#include "CoreMinimal.h"
// Prepared command-path assertions, run only by the explicit campaign probe.
bool RunFoundryRecipeControlProbe(FString& Report);
#endif
