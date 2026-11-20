#if WITH_DEV_AUTOMATION_TESTS
#include "Misc/AutomationTest.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/ScopeExit.h"
#include "HAL/FileManager.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"
#include "Blueprint/WidgetTree.h"
#include "Components/TextBlock.h"
#include "GeoCatalog.h"
#include "Serialization/JsonSerializer.h"
#include "Simulation/SimulationSubsystem.h"
#include "UI/PlayerHUD.h"
#include "UI/CompanyHeaderLayout.h"

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeaderLayout,"CoI.UI02.ResponsiveHeaderBounds",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeaderLayout::RunTest(const FString&)
{
 for(double Width:{1280.,1699.,1700.,1920.,2560.}){const auto L=CoIHeader::Layout(Width);const double Inner=Width-56.;
  auto Fits=[&](FVector2D P,FVector2D S){TestTrue(TEXT("Positive cell size"),S.X>0&&S.Y>0);TestTrue(TEXT("Header contents fit padded bounds"),P.X>=0&&P.Y>=0&&P.X+S.X<=Inner+.01&&P.Y+S.Y<=L.Height-16.+.01);};
  Fits(L.IdentityPosition,L.IdentitySize);Fits(L.MetricsPosition,L.MetricsSize);Fits(L.TimePosition,L.TimeSize);
  auto Overlap=[](FVector2D A,FVector2D AS,FVector2D B,FVector2D BS){return A.X<B.X+BS.X&&B.X<A.X+AS.X&&A.Y<B.Y+BS.Y&&B.Y<A.Y+AS.Y;};
  TestFalse(TEXT("Metrics never overlap time controls"),Overlap(L.MetricsPosition,L.MetricsSize,L.TimePosition,L.TimeSize));TestFalse(TEXT("Identity never overlaps metrics"),Overlap(L.IdentityPosition,L.IdentitySize,L.MetricsPosition,L.MetricsSize));TestTrue(TEXT("Navigation follows entire header"),L.NavY>=12.+L.Height);TestTrue(TEXT("Work content follows navigation"),L.ContentY>=L.NavY+46.);
 }return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FHeaderNavigation,"CoI.UI02.LiveWidgetsPauseNavigationAndCompanyRestart",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FHeaderNavigation::RunTest(const FString&)
{
 FString Json;auto* Original=LoadObject<UCoIGeoCatalog>(nullptr,TEXT("/Game/Geography/Data/GeoCatalog.GeoCatalog"));
 if(!Original||!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectContentDir()/TEXT("Simulation/scenario.coisim"))))return false;
 const FString Filename=TEXT("ui02_fixture_")+FGuid::NewGuid().ToString(EGuidFormats::Digits)+TEXT(".coisim");const FString File=FPaths::ProjectContentDir()/TEXT("Simulation")/Filename;
 ON_SCOPE_EXIT{IFileManager::Get().Delete(*File);};
 TSharedPtr<FJsonObject> Config;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Json),Config);
 const FString LongName=TEXT("Internationales Handelshaus für Rohstoffe, Versorgung und regionale Logistik mit mehreren Niederlassungen (Testfirma)");
 for(const auto& C:Config->GetObjectField(TEXT("Definitions"))->GetArrayField(TEXT("Companies")))if(C->AsObject()->GetStringField(TEXT("Id"))==TEXT("company.player"))C->AsObject()->SetStringField(TEXT("Name"),LongName);
 for(const auto& A:Config->GetObjectField(TEXT("InitialState"))->GetArrayField(TEXT("Accounts")))if(A->AsObject()->GetStringField(TEXT("OwnerId"))==TEXT("company.player"))A->AsObject()->SetNumberField(TEXT("BalanceCents"),900000000000.);
 auto Write=[&](){FString Out;FJsonSerializer::Serialize(Config.ToSharedRef(),TJsonWriterFactory<>::Create(&Out));return FFileHelper::SaveStringToFile(Out,*File,FFileHelper::EEncodingOptions::ForceUTF8WithoutBOM);};if(!Write())return false;
 const auto IV=UWorld::InitializationValues().AllowAudioPlayback(false).CreatePhysicsScene(false).CreateNavigation(false).CreateAISystem(false).ShouldSimulatePhysics(false);
 auto* World=UWorld::CreateWorld(EWorldType::Game,false,NAME_None,nullptr,true,ERHIFeatureLevel::Num,&IV);
 if(!World)return false;ON_SCOPE_EXIT{World->DestroyWorld(false);};World->SetGameInstance(NewObject<UGameInstance>(World));
 auto* Ref=World->SpawnActor<ACoIGeoReference>();auto* Catalog=NewObject<UCoIGeoCatalog>(World);TSharedPtr<FJsonObject> Geo;FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(Original->DefinitionJson),Geo);Geo->GetObjectField(TEXT("scenario"))->SetStringField(TEXT("simulation_file"),Filename);FJsonSerializer::Serialize(Geo.ToSharedRef(),TJsonWriterFactory<>::Create(&Catalog->DefinitionJson));Ref->Catalog=Catalog;
 auto* Sim=World->GetSubsystem<UCoISimulationSubsystem>();if(!TestNotNull(TEXT("Real world simulation subsystem"),Sim)||!TestTrue(TEXT("Fixture scenario starts"),Sim->Restart()))return false;
 auto* HUD=NewObject<UCoIPlayerHUD>(World);HUD->Initialize();auto Slate=HUD->TakeWidget();ON_SCOPE_EXIT{HUD->ReleaseSlateResources(true);};
 auto HasText=[&](const FString& Text){TArray<UWidget*> All;HUD->WidgetTree->GetAllWidgets(All);for(const auto* W:All)if(const auto* T=Cast<UTextBlock>(W);T&&T->GetText().ToString()==Text)return true;return false;};
 TestTrue(TEXT("Long company identity retained in real widget"),HasText(LongName));TestTrue(TEXT("Large cash uses common exact formatter"),HasText(TEXT("9.000.000.000,00 USD")));TestTrue(TEXT("Missing month is not zero profit"),HasText(TEXT("Noch keine Monatsdaten")));
 const auto Before=Sim->Fingerprint();for(const TCHAR* Key:{TEXT("cash"),TEXT("result"),TEXT("employees"),TEXT("sites"),TEXT("networth")}){HUD->Action(FString(TEXT("header|"))+Key);TestEqual(TEXT("Metric opens available detail view"),HUD->CurrentView(),FString(TEXT("Unternehmensdetails")));HUD->Escape();}TestEqual(TEXT("Reading and navigation do not mutate economics"),Sim->Fingerprint(),Before);
 HUD->Action(TEXT("menu|open"));TestTrue(TEXT("Paused menu opens"),HUD->IsPauseMenuOpen());HUD->Action(TEXT("menu|settings"));TestTrue(TEXT("Settings stay in modal menu"),HUD->IsPauseMenuOpen());HUD->Escape();TestTrue(TEXT("Escape settings returns to parent menu"),HUD->IsPauseMenuOpen());HUD->Escape();TestTrue(TEXT("Initially paused remains paused"),Sim->IsPaused());
 HUD->Action(TEXT("speed4"));HUD->Action(TEXT("pause"));TestFalse(TEXT("Start resumes"),Sim->IsPaused());HUD->Action(TEXT("menu|open"));TestTrue(TEXT("Menu pauses a running simulation"),Sim->IsPaused());TestTrue(TEXT("Menu blocks map keyboard"),HUD->KeyboardBlocked());TestTrue(TEXT("Menu blocks map pointer everywhere"),HUD->PointerBlocked({-100,-100}));HUD->Action(TEXT("menu|saves"));TestEqual(TEXT("Saves available via menu"),HUD->CurrentView(),FString(TEXT("Spielstände")));HUD->Escape();TestTrue(TEXT("Saves return to pause menu"),HUD->IsPauseMenuOpen());HUD->Escape();TestFalse(TEXT("Continue restores previous running state"),Sim->IsPaused());TestEqual(TEXT("Selected speed preserved"),Sim->GetSpeed(),4);
 Sim->SetSpeed(16);for(int32 Frame=0;Sim->GetSnapshot().Step<32*24&&Frame<3000;++Frame)Sim->Tick(.5f);
 TestTrue(TEXT("Real complete month reached"),Sim->GetSnapshot().Step>=32*24);const auto& Result=Sim->GetSnapshot().Player.Company.MonthlyResult;
 TestTrue(TEXT("Actual monthly loss is available"),Result.State==CoISim::EDisplayState::Value&&Result.Value<0);TestTrue(TEXT("Negative result displayed from same snapshot"),HasText(Result.Display()));
 Sim->TogglePause();HUD->Action(TEXT("menu|open"));HUD->Action(TEXT("menu|saves"));Config->SetStringField(TEXT("PlayerCompanyId"),TEXT("company.food"));if(!Write()||!Sim->Restart())return false;HUD->Escape();TestTrue(TEXT("New generation preserves save-menu back link"),HUD->IsPauseMenuOpen());HUD->Escape();TestTrue(TEXT("A loaded or restarted party never resumes the old session"),Sim->IsPaused());TestTrue(TEXT("New company name replaces prior identity"),HasText(TEXT("Prairie Food")));TestFalse(TEXT("Old company identity removed"),HasText(LongName));HUD->Action(TEXT("header|sites"));TestTrue(TEXT("New company site action points to actual headquarters"),HasText(TEXT("Chicago · eigener Standort")));
 return true;
}
#endif
