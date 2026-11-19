#pragma once
#include "SimModel.h"

namespace CoISim
{
// Derived display data only: never serialized, never used to settle transactions.
enum class EDisplayState : uint8
{
    Value,
    NoMonthlyHistory,
    Unavailable
};
struct FDisplayMetric
{
    FString Label, Source, Period, Calculation;
    EDisplayState State = EDisplayState::Unavailable;
    int64 Value = 0;
    bool Currency = false;
    FString Display() const;
    FString Tooltip() const;
};
struct FCompanyPresentation
{
    FDisplayMetric AvailableMoney, Employees, Sites, MonthlyResult, NetWorth;
    TMap<FString, int64> EmployeesByCity;
    TArray<FString> SiteCityIds;
};
FCompanyPresentation CompanyPresentation(const FState &, const FDefinitions &, const FString &Actor,
                                         const FString &StartDate);

namespace Display
{
FString Number(int64 Value);
FString Money(int64 Cents);
FString Quantity(int64 Kg);
FString Percent(double Ratio);
FString Duration(int64 Hours);
FString Time(const FString &StartDate, int64 Hour);
FString Period(const FString &StartDate, int64 BeginHour, int64 EndHour);
// Editable USD have no thousands separator; comma or dot, at most two decimals.
FString MoneyInput(int64 Cents);
bool ParseMoneyInput(const FString &, int64 &Cents, bool AllowZero = false);
bool IsMoneyField(const FString &Key);
FString Empty(EDisplayState State);
} // namespace Display
} // namespace CoISim
