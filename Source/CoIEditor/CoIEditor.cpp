#include "Modules/ModuleManager.h"
#include "CoIBuildMapCommandlet.h"
#include "Editor.h"
#include "Misc/CoreDelegates.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "HAL/IConsoleManager.h"
#include "Containers/Ticker.h"
class FCoIEditorModule : public IModuleInterface
{
 FDelegateHandle InitHandle;FTSTicker::FDelegateHandle Ticker;IConsoleObject* Command=nullptr;
 static void Build()
 {
  if(GEditor&&GEditor->PlayWorld){UE_LOG(LogTemp,Warning,TEXT("Stop Play before rebuilding a map."));return;}
  auto* C=NewObject<UCoIBuildMapCommandlet>();C->AddToRoot();C->Main(TEXT("Scenario"));C->RemoveFromRoot();
 }
public:
 void StartupModule() override
 {
  Command=IConsoleManager::Get().RegisterConsoleCommand(TEXT("CoI.RebuildMap"),TEXT("Rebuild generated project map; stop Play first."),FConsoleCommandDelegate::CreateStatic(&Build),ECVF_Default);
  InitHandle=FCoreDelegates::GetOnPostEngineInit().AddLambda([this]{
   if(!IsRunningCommandlet()&&!IFileManager::Get().FileExists(*(FPaths::ProjectContentDir()/TEXT("Maps/GreatLakes1913.umap"))))
    Ticker=FTSTicker::GetCoreTicker().AddTicker(FTickerDelegate::CreateLambda([](float){Build();return false;}),3.f);
  });
 }
 void ShutdownModule() override
 {FCoreDelegates::GetOnPostEngineInit().Remove(InitHandle);FTSTicker::GetCoreTicker().RemoveTicker(Ticker);if(Command)IConsoleManager::Get().UnregisterConsoleObject(Command);}
};
IMPLEMENT_MODULE(FCoIEditorModule,CoIEditor)
