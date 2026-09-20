#pragma once
#include "CoreMinimal.h"
class AFoundryStage;
class SWidget;
#if FOUNDRY_WITH_CAMPAIGN
TSharedRef<SWidget> MakeFoundryCampaignUI(AFoundryStage* Stage);
#endif
