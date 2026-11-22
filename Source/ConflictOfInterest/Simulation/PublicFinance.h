#pragma once
#include "CoreMinimal.h"
namespace CoISim
{
struct FPublicTreasuryRule
{
    FString Id, AccountId, Name;
    int64 OpeningReserveCents = 0, TaxPermille = 100, TransferCapCentsDay = 0;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("AccountId"), AccountId);
        W(TEXT("Name"), Name);
        W(TEXT("OpeningReserveCents"), OpeningReserveCents);
        W(TEXT("TaxPermille"), TaxPermille);
        W(TEXT("TransferCapCentsDay"), TransferCapCentsDay);
    }
};
struct FPublicOffice
{
    FString Id, CountryId, RegionId, CityId;
    int64 Jobs = 2, DailyWageCents = 180;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("CountryId"), CountryId);
        W(TEXT("RegionId"), RegionId);
        W(TEXT("CityId"), CityId);
        W(TEXT("Jobs"), Jobs);
        W(TEXT("DailyWageCents"), DailyWageCents);
    }
};
struct FPublicRules
{
    bool Enabled = false;
    TArray<FPublicTreasuryRule> Treasuries;
    TArray<FPublicOffice> Offices;
    FString EligibleBand = TEXT("low");
    int64 TransferHours = 24, TransferCentsPersonDay = 25, TransferMaxPeopleGroup = 20, TransferDurationDays = 14,
          MaxSavingsCentsPerson = 1000;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Enabled"), Enabled);
        W(TEXT("Treasuries"), Treasuries);
        W(TEXT("Offices"), Offices);
        W(TEXT("EligibleBand"), EligibleBand);
        W(TEXT("TransferHours"), TransferHours);
        W(TEXT("TransferCentsPersonDay"), TransferCentsPersonDay);
        W(TEXT("TransferMaxPeopleGroup"), TransferMaxPeopleGroup);
        W(TEXT("TransferDurationDays"), TransferDurationDays);
        W(TEXT("MaxSavingsCentsPerson"), MaxSavingsCentsPerson);
    }
};
struct FPublicTreasury
{
    FString Id;
    int64 TaxAssessed = 0, TaxPaid = 0, WagesPaid = 0, TransfersPaid = 0, TransferCuts = 0, NextTransferHour = 0,
          LastUnfundedJobs = 0, LastVacantJobs = 0;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("TaxAssessed"), TaxAssessed);
        W(TEXT("TaxPaid"), TaxPaid);
        W(TEXT("WagesPaid"), WagesPaid);
        W(TEXT("TransfersPaid"), TransfersPaid);
        W(TEXT("TransferCuts"), TransferCuts);
        W(TEXT("NextTransferHour"), NextTransferHour);
        W(TEXT("LastUnfundedJobs"), LastUnfundedJobs);
        W(TEXT("LastVacantJobs"), LastVacantJobs);
    }
};
struct FPublicClaim
{
    FString Id, CountryId;
    int64 FirstDay = -1, LastDay = -1, LastRecipients = 0, LastDue = 0, LastPaid = 0, Assessed = 0, Paid = 0;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("CountryId"), CountryId);
        W(TEXT("FirstDay"), FirstDay);
        W(TEXT("LastDay"), LastDay);
        W(TEXT("LastRecipients"), LastRecipients);
        W(TEXT("LastDue"), LastDue);
        W(TEXT("LastPaid"), LastPaid);
        W(TEXT("Assessed"), Assessed);
        W(TEXT("Paid"), Paid);
    }
};
struct FPublicTax
{
    FString Id, CountryId;
    int64 LastPeriodEnd = 0, LastBase = 0, LastAssessment = 0, Assessed = 0, Paid = 0;
    template <class R> void Fields(R &W)
    {
        W(TEXT("Id"), Id);
        W(TEXT("CountryId"), CountryId);
        W(TEXT("LastPeriodEnd"), LastPeriodEnd);
        W(TEXT("LastBase"), LastBase);
        W(TEXT("LastAssessment"), LastAssessment);
        W(TEXT("Assessed"), Assessed);
        W(TEXT("Paid"), Paid);
    }
};
struct FScenario;
struct FState;
struct FDefinitions;
FString PublicCountry(const FDefinitions &, const FString &Company);
int64 PublicTaxAmount(int64 Profit, int64 Permille);
bool ValidatePublicScenario(const FScenario &, FString &);
bool ValidatePublicState(const FScenario &, const FState &, FString &);
FString PublicDiagnostics(const FScenario &, const FState &);
} // namespace CoISim
