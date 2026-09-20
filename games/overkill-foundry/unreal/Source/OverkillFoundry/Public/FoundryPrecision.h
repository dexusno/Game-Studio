#pragma once
#include "CoreMinimal.h"

class SWidget;
struct FFoundryPrecisionOptions
{
    int32 WidthPercent = 100;
    FString Title = TEXT("Precision grab");
    // The calling UI derives haul/bonus descriptions with real core previews.
    FString Detail;
    FString BackLabel = TEXT("Back  ·  keep Precision for later");
    TFunction<void(int32)> OnResult;
    TFunction<void()> OnCancel;
};

// A modal child. Caller must block underlying game commands while it is shown.
// Back is available before Start. After Start, one stop or timeout resolves it.
TSharedRef<SWidget> MakeFoundryPrecisionWidget(FFoundryPrecisionOptions Options);
