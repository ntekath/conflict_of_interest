#include "PlayerHUD.h"
#include "CompanyHeaderLayout.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "Blueprint/WidgetTree.h"
#include "Components/Border.h"
#include "Components/ButtonSlot.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/ScrollBox.h"
#include "Components/TextBlock.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Engine/Engine.h"
#include "GameFramework/GameUserSettings.h"
#include "Styling/CoreStyle.h"
using namespace CoISim;
namespace {
const FLinearColor Brass(.68f,.49f,.24f),Paper(.88f,.85f,.76f),Muted(.58f,.64f,.63f);
void Active(UCoIActionButton* B,bool On){if(!B)return;auto Style=B->GetStyle();Style.Normal.TintColor=FSlateColor(On?FLinearColor(.32f,.24f,.12f):FLinearColor(.085f,.105f,.11f));B->SetStyle(Style);}
}
void UCoIPlayerHUD::BuildCompanyHeader()
{
 HeaderCanvas=WidgetTree->ConstructWidget<UCanvasPanel>();Top->SetContent(HeaderCanvas);
 IdentityScroll=WidgetTree->ConstructWidget<UScrollBox>();HeaderCanvas->AddChildToCanvas(IdentityScroll);
 auto* Identity=Row(nullptr);IdentityScroll->AddChild(Identity);
 auto* Color=WidgetTree->ConstructWidget<UBorder>();Color->SetBrushColor(Brass);Color->SetPadding(FMargin(3,0));Identity->AddChildToHorizontalBox(Color)->SetPadding(FMargin(0,0,8,0));
 Color->SetToolTipText(FText::FromString(TEXT("Vorhandene Messingfarbe für die eigene Unternehmensperspektive.")));
 IdentityButton=Button(Identity,TEXT("Unternehmen wird geladen"),TEXT("header|overview"));Cast<UHorizontalBoxSlot>(IdentityButton->Slot)->SetSize(FSlateChildSize(ESlateSizeRule::Fill));
 CompanyName=CastChecked<UTextBlock>(IdentityButton->GetContent());CompanyName->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",14));
 Cast<UButtonSlot>(CompanyName->Slot)->SetHorizontalAlignment(HAlign_Fill);
 HeaderMetrics=Row(nullptr);HeaderCanvas->AddChildToCanvas(HeaderMetrics);
 const TCHAR* Keys[]={TEXT("cash"),TEXT("result"),TEXT("employees"),TEXT("sites"),TEXT("networth")};
 const TCHAR* Labels[]={TEXT("Verfügbar"),TEXT("Monatsergebnis"),TEXT("Mitarbeiter"),TEXT("Standorte"),TEXT("Nettovermögen")};
 for(int32 I=0;I<5;++I){auto* B=Button(HeaderMetrics,TEXT(""),FString(TEXT("header|"))+Keys[I]);auto* Cell=CastChecked<UHorizontalBoxSlot>(B->Slot);Cell->SetSize(FSlateChildSize(ESlateSizeRule::Fill));Cell->SetPadding(FMargin(0,0,6,0));auto Style=B->GetStyle();Style.NormalPadding=FMargin(6,3);B->SetStyle(Style);
  auto* Box=WidgetTree->ConstructWidget<UVerticalBox>();B->SetContent(Box);Cast<UButtonSlot>(Box->Slot)->SetHorizontalAlignment(HAlign_Fill);auto* Label=Text(Box,Labels[I],10,true);Cast<UVerticalBoxSlot>(Label->Slot)->SetPadding(FMargin(0));auto* Value=Text(Box,TEXT("Wird geladen"),11);Cast<UVerticalBoxSlot>(Value->Slot)->SetPadding(FMargin(0));MetricButtons.Add(B);MetricValues.Add(Value);}
 TimeControls=WidgetTree->ConstructWidget<UVerticalBox>();HeaderCanvas->AddChildToCanvas(TimeControls);
 ClockText=Text(TimeControls,TEXT("Wird geladen"),11);auto* Controls=Row(TimeControls);
 PauseButton=Button(Controls,TEXT("Start"),TEXT("pause"));for(const TCHAR* Key:{TEXT("1"),TEXT("4"),TEXT("16")})SpeedButtons.Add(Button(Controls,FString(Key)+TEXT("x"),FString(TEXT("speed"))+Key));
 MenuButton=Button(Controls,TEXT("Menü"),TEXT("menu|open"),false,TEXT("Pausenmenü · Esc: Spielstände und lokale Einstellungen"));
 for(auto* B:{PauseButton.Get(),SpeedButtons[0].Get(),SpeedButtons[1].Get(),SpeedButtons[2].Get(),MenuButton.Get()}){auto Style=B->GetStyle();Style.NormalPadding=FMargin(7,4);B->SetStyle(Style);Cast<UHorizontalBoxSlot>(B->Slot)->SetPadding(FMargin(0,0,5,0));}
}
void UCoIPlayerHUD::LayoutCompanyHeader(double Width)
{
 const auto L=CoIHeader::Layout(Width);
 auto Place=[](UWidget* W,FVector2D P,FVector2D S){auto* Slot=CastChecked<UCanvasPanelSlot>(W->Slot);Slot->SetPosition(P);Slot->SetSize(S);};
 Place(IdentityScroll,L.IdentityPosition,L.IdentitySize);Place(HeaderMetrics,L.MetricsPosition,L.MetricsSize);Place(TimeControls,L.TimePosition,L.TimeSize);
}
void UCoIPlayerHUD::RefreshCompanyHeader()
{
 const auto& S=Sim->GetSnapshot();const auto& C=S.Player.Company;
 CompanyName->SetText(FText::FromString(Display::Name(S.PlayerName)));IdentityButton->SetToolTipText(FText::FromString(S.PlayerName+TEXT("\nUnternehmensübersicht öffnen. Lange Namen sind im Namensfeld scrollbar.")));
 const FDisplayMetric* Metrics[]={&C.AvailableMoney,&C.MonthlyResult,&C.Employees,&C.Sites,&C.NetWorth};
 for(int32 I=0;I<5;++I){const auto& M=*Metrics[I];MetricValues[I]->SetText(FText::FromString(M.Display()));MetricButtons[I]->SetToolTipText(FText::FromString(M.Tooltip()+TEXT("\nKlicken: zugehörige Unternehmensdetails öffnen.")));MetricValues[I]->SetColorAndOpacity(M.State!=EDisplayState::Value?Muted:I==1&&M.Value<0?FLinearColor(.95f,.47f,.40f):Paper);Active(MetricButtons[I],View==TEXT("Unternehmensdetails")&&MetricButtons[I]->Action==TEXT("header|")+HeaderDetail);}
 ClockText->SetText(FText::FromString(S.Date+TEXT(" · ")+(Sim->IsPaused()?TEXT("Pausiert"):TEXT("Läuft"))));ClockText->SetToolTipText(FText::FromString(FString::Printf(TEXT("Veröffentlichte Spielzeit · %.1f Simulationsstunden pro realer Sekunde"),Sim->GetDiagnostics().ActualHoursPerSecond)));
 const int32 Speeds[]={1,4,16};for(int32 I=0;I<3;++I){const bool SelectedSpeed=Sim->GetSpeed()==Speeds[I];Active(SpeedButtons[I],SelectedSpeed);SpeedButtons[I]->SetToolTipText(FText::FromString(FString::Printf(TEXT("%dx Spielzeit%s"),Speeds[I],SelectedSpeed?TEXT(" · ausgewählt"):TEXT(""))));}
 if(auto* Label=Cast<UTextBlock>(PauseButton->GetContent()))Label->SetText(FText::FromString(Sim->IsPaused()?TEXT("Start"):TEXT("Pause")));Active(PauseButton,Sim->IsPaused());Active(MenuButton,!MenuPage.IsEmpty()||ReturnToPause);
 PauseButton->SetToolTipText(FText::FromString(Sim->IsPaused()?TEXT("Pausiert · Spielzeit mit der ausgewählten Geschwindigkeit starten."):TEXT("Laufende Spielzeit pausieren.")));
 PauseButton->SetIsEnabled(!ReturnToPause);
}
void UCoIPlayerHUD::CompanyDetail()
{
 const auto& S=Sim->GetSnapshot();const auto& P=S.Player;Text(Body,S.PlayerName,16,true);
 if(HeaderDetail==TEXT("cash")){Metric(Body,P.Company.AvailableMoney);Text(Body,TEXT("Kontostand ")+Display::Money(S.PlayerMoneyCents)+TEXT(" · reserviert ")+Display::Money(S.PlayerReservedCents));Text(Body,P.Company.AvailableMoney.Tooltip(),11);Button(Body,TEXT("Gebuchte Zahlungen & Unternehmensübersicht"),TEXT("view|Unternehmen"));}
 else if(HeaderDetail==TEXT("result")){Metric(Body,P.Company.MonthlyResult);Text(Body,P.Company.MonthlyResult.Tooltip(),11);AccountingReport();}
 else if(HeaderDetail==TEXT("networth")){Metric(Body,P.Company.NetWorth);Text(Body,P.Company.NetWorth.Tooltip(),11);Button(Body,TEXT("Vorhandene Assets & Kostenrechnung"),TEXT("view|Unternehmen"));}
 else {const bool Employees=HeaderDetail==TEXT("employees");Metric(Body,Employees?P.Company.Employees:P.Company.Sites);Text(Body,(Employees?P.Company.Employees:P.Company.Sites).Tooltip(),11);int32 Count=0;
  for(const auto& CityRow:P.Cities)if(Employees?CityRow.OwnEmployees>0:P.Company.SiteCityIds.Contains(CityRow.Id)){++Count;auto* R=Row(Body);Text(R,CityRow.Name+(Employees?TEXT(" · ")+Display::Number(CityRow.OwnEmployees)+TEXT(" eigene Beschäftigte"):TEXT(" · eigener Standort")),13);Button(R,TEXT("Stadt öffnen"),TEXT("city|")+CityRow.Id);}
  if(!Count)Empty(Body,Employees?TEXT("Derzeit keine aktiv bezahlten eigenen Mitarbeiter."):TEXT("Derzeit keine eigenen fertiggestellten Standorte."));}
}
void UCoIPlayerHUD::OpenPauseMenu()
{
 if(!Sim||!Sim->IsReady()||!MenuPage.IsEmpty())return;if(ReturnToPause){BuildView(TEXT(""));return;}MenuResume=!Sim->IsPaused();if(MenuResume)Sim->TogglePause();MenuPage=TEXT("pause");RefreshPauseMenu();
}
void UCoIPlayerHUD::ClosePauseMenu(bool Resume)
{
 MenuPage.Empty();MenuShade->SetVisibility(ESlateVisibility::Collapsed);MenuPanel->SetVisibility(ESlateVisibility::Collapsed);const bool Continue=Resume&&MenuResume;MenuResume=false;ReturnToPause=false;if(Continue&&Sim&&Sim->IsReady()&&Sim->IsPaused())Sim->TogglePause();Refresh();
}
void UCoIPlayerHUD::RefreshPauseMenu()
{
 MenuShade->SetVisibility(ESlateVisibility::Visible);MenuPanel->SetVisibility(ESlateVisibility::Visible);auto* Scroll=WidgetTree->ConstructWidget<UScrollBox>();MenuPanel->SetContent(Scroll);auto* Box=WidgetTree->ConstructWidget<UVerticalBox>();Scroll->AddChild(Box);
 Text(Box,MenuPage==TEXT("settings")?TEXT("EINSTELLUNGEN"):TEXT("PAUSENMENÜ"),18,true);
 if(MenuPage==TEXT("settings")){Text(Box,TEXT("Lokale Darstellung · die Spielzeit bleibt pausiert."),12);if(auto* Settings=GEngine?GEngine->GetGameUserSettings():nullptr){Button(Box,Settings->IsVSyncEnabled()?TEXT("VSync: An"):TEXT("VSync: Aus"),TEXT("setting|vsync"));Text(Box,TEXT("Bildratenlimit: ")+(Settings->GetFrameRateLimit()>0?FString::Printf(TEXT("%.0f FPS"),Settings->GetFrameRateLimit()):TEXT("Unbegrenzt")),13);auto* R=Row(Box);for(const TCHAR* FPS:{TEXT("30"),TEXT("60"),TEXT("120"),TEXT("0")}){auto* B=Button(R,FCString::Atoi(FPS)?FString(FPS)+TEXT(" FPS"):TEXT("Unbegrenzt"),FString(TEXT("setting|"))+FPS);Active(B,FMath::IsNearlyEqual(Settings->GetFrameRateLimit(),(float)FCString::Atoi(FPS)));}Text(Box,TEXT("Wird über die vorhandenen Unreal-Spieleinstellungen lokal gespeichert. Das FPS-Limit ist unabhängig von 1x/4x/16x Spielzeit."),11);}else Empty(Box,TEXT("Spieleinstellungen sind in dieser Umgebung nicht verfügbar."));Button(Box,TEXT("Zurück zum Pausenmenü · Esc"),TEXT("menu|back"));}
 else {Text(Box,MenuResume?TEXT("Spielzeit für das Menü pausiert. Weiter setzt die vorherige Geschwindigkeit fort."):TEXT("Die Partie war bereits pausiert und bleibt nach Weiter pausiert."),12);Button(Box,TEXT("Weiter · Esc"),TEXT("menu|continue"),true);Button(Box,TEXT("Spielstände"),TEXT("menu|saves"));Button(Box,TEXT("Einstellungen"),TEXT("menu|settings"));Button(Box,TEXT("Entwicklerdiagnose ein-/ausblenden"),TEXT("menu|diagnostics"));}
 RefreshCompanyHeader();
}
bool UCoIPlayerHUD::HeaderAction(const FString& Verb,const FString& Arg)
{
 if(Verb==TEXT("header")){if(!Sim||!Sim->IsReady())return true;FocusedObject.Empty();CityContext.Empty();if(Arg==TEXT("overview"))BuildView(TEXT("Unternehmen"));else {HeaderDetail=Arg;BuildView(TEXT("Unternehmensdetails"));}RefreshCompanyHeader();return true;}
 if(Verb==TEXT("menu")){if(Arg==TEXT("open"))OpenPauseMenu();else if(Arg==TEXT("continue"))ClosePauseMenu(true);else if(Arg==TEXT("back")){MenuPage=TEXT("pause");RefreshPauseMenu();}else if(Arg==TEXT("settings")){MenuPage=TEXT("settings");RefreshPauseMenu();}else if(Arg==TEXT("saves")){ReturnToPause=true;MenuPage.Empty();MenuPanel->SetVisibility(ESlateVisibility::Collapsed);MenuShade->SetVisibility(ESlateVisibility::Collapsed);CityContext.Empty();BuildView(TEXT("Spielstände"));}else if(Arg==TEXT("diagnostics")){ClosePauseMenu(false);Action(TEXT("debug"));}return true;}
 if(Verb==TEXT("setting")){if(auto* Settings=GEngine?GEngine->GetGameUserSettings():nullptr){if(Arg==TEXT("vsync"))Settings->SetVSyncEnabled(!Settings->IsVSyncEnabled());else if(Arg==TEXT("30")||Arg==TEXT("60")||Arg==TEXT("120")||Arg==TEXT("0"))Settings->SetFrameRateLimit(FCString::Atof(*Arg));Settings->ApplyNonResolutionSettings();Settings->SaveSettings();}RefreshPauseMenu();return true;}
 return false;
}
