#pragma once
#include "CoreMinimal.h"
// Linear colors, shared by header, panels, rows, dropdowns and action states.
namespace CoITheme
{
inline const FLinearColor Ink(.035f, .045f, .05f), Paper(.88f, .85f, .76f), Brass(.68f, .49f, .24f),
    Muted(.58f, .64f, .63f);
inline const FLinearColor Button(.085f, .105f, .11f), Hover(.15f, .18f, .18f), AccentFill(.25f, .20f, .12f),
    Disabled(.045f, .055f, .06f), Loss(.95f, .47f, .40f);
constexpr int32 BodyFont = 12;
constexpr float ScrollbarWidth = 10.f;
} // namespace CoITheme
