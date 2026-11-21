#pragma once
#include "CoreMinimal.h"
// Slate logical pixels after project DPI scaling; acceptance values, not economic parameters.
namespace CoIPanels
{
constexpr double NarrowWidth = 424.;
constexpr double WideWidth = 608.;
constexpr int32 MaxHistory = 32;
inline double Width(double ViewWidth)
{
    return FMath::Clamp(NarrowWidth + (ViewWidth - 1280.) * (WideWidth - NarrowWidth) / 640., 360., WideWidth);
}
} // namespace CoIPanels
