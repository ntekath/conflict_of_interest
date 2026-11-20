#pragma once
#include "SimModel.h"
namespace CoISim
{
inline const FString BankLedgerId = TEXT("bank.ledger");
inline const FString BankStaffId = TEXT("bank.staff");
struct FLoanQuote
{
    int64 RateBps = 0, SpreadBps = 0, CollateralCents = 0, SupportedCents = 0, PeriodPrincipalCents = 0,
          FirstInterestCents = 0, DailyCashCents = 0;
    bool Startup = false;
    TArray<FString> CollateralIds;
    FString Summary;
};
int64 BankMoneyChange(const FBankState &);
int64 BankEquity(const FBankRules &, const FBankState &);
int64 BankSpendable(const FBankRules &, const FBankState &);
bool ValidateBankScenario(const FScenario &, FString &);
bool AssessLoan(const FScenario &, const FState &, const FLoanRequest &, FLoanQuote &, FString &);
bool Pledged(const FState &, const FString &Asset);
} // namespace CoISim
