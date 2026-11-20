#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/ScopeExit.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "Components/EditableTextBox.h"
#include "GeoCatalog.h"
#include "Simulation/SimulationSubsystem.h"
#include "UI/PlayerHUD.h"
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FBankWidgetFlow, "CoI.Banking.LiveWidgetApplicationRepaymentAndInvestment",
                                 EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FBankWidgetFlow::RunTest(const FString &)
{
    auto *Catalog =
        LoadObject<UCoIGeoCatalog>(nullptr, TEXT("/Game/Geography/Data/GeoCatalog_DE_NL_1913.GeoCatalog_DE_NL_1913"));
    if (!Catalog)
        return false;
    const auto IV = UWorld::InitializationValues()
                        .AllowAudioPlayback(false)
                        .CreatePhysicsScene(false)
                        .CreateNavigation(false)
                        .CreateAISystem(false)
                        .ShouldSimulatePhysics(false);
    auto *World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &IV);
    if (!World)
        return false;
    ON_SCOPE_EXIT
    {
        World->DestroyWorld(false);
    };
    World->SetGameInstance(NewObject<UGameInstance>(World));
    World->SpawnActor<ACoIGeoReference>()->Catalog = Catalog;
    auto *Sim = World->GetSubsystem<UCoISimulationSubsystem>();
    if (!Sim->Restart())
        return false;
    auto *HUD = NewObject<UCoIPlayerHUD>(World);
    HUD->Initialize();
    auto Slate = HUD->TakeWidget();
    ON_SCOPE_EXIT
    {
        HUD->ReleaseSlateResources(true);
    };
    auto HasText = [&](const FString &Value) {
        TArray<UWidget *> All;
        HUD->WidgetTree->GetAllWidgets(All);
        for (const auto *W : All)
            if (const auto *T = Cast<UTextBlock>(W); T && T->GetText().ToString() == Value)
                return true;
        return false;
    };
    const auto Before = Sim->Fingerprint();
    HUD->Action(TEXT("header|overview"));
    TestTrue(TEXT("Company view exposes financing"), HasText(TEXT("FINANZIERUNG")));
    HUD->Action(TEXT("loanApply"));
    TestTrue(TEXT("Real secured preview valid"), HUD->ValidDraft);
    HUD->Fields[TEXT("loan.amount")]->SetText(FText::FromString(TEXT("1000,001")));
    HUD->Action(TEXT("preview"));
    TestFalse(TEXT("No hidden rounding of fractional cents"), HUD->ValidDraft);
    HUD->Fields[TEXT("loan.amount")]->SetText(FText::FromString(TEXT("1000,00")));
    HUD->Action(TEXT("preview"));
    TestTrue(TEXT("Valid formatted money accepted"), HUD->ValidDraft);
    TestEqual(TEXT("Reading and editing application creates no money"), Sim->Fingerprint(), Before);
    HUD->Action(TEXT("confirm"));
    HUD->Action(TEXT("confirm"));
    Sim->TogglePause();
    Sim->Tick(1.1f);
    TestEqual(TEXT("Only one issued credit despite repeated confirmation"), Sim->GetSnapshot().Player.Loans.Num(), 1);
    TestEqual(TEXT("UI submits exactly 1000 USD"), Sim->GetSnapshot().Player.Bank.CreatedCents, 100000LL);
    if (Sim->GetSnapshot().Player.Loans.IsEmpty())
        return false;
    const FString LoanId = Sim->GetSnapshot().Player.Loans[0].Id;
    HUD->Action(TEXT("loanRepay|") + LoanId);
    HUD->Fields[TEXT("loan.amount")]->SetText(FText::FromString(TEXT("100,00")));
    HUD->Action(TEXT("preview"));
    TestTrue(TEXT("Owned repayment preview valid"), HUD->ValidDraft);
    HUD->Action(TEXT("confirm"));
    Sim->Tick(1.1f);
    TestEqual(TEXT("UI principal booking exact"), Sim->GetSnapshot().Player.Bank.RepaidCents, 10000LL);
    TestEqual(TEXT("UI debt follows authoritative snapshot"), Sim->GetSnapshot().Player.Loans[0].OutstandingCents,
              90000LL);
    HUD->Action(TEXT("city"));
    HUD->Action(TEXT("cityTab|Investitionen"));
    TestTrue(TEXT("Investment view shows same financing"), HasText(TEXT("FINANZIERUNG")));
    Sim->TogglePause();
    return true;
}
#endif
