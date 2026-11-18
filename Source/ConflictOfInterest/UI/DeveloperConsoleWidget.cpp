#include "DeveloperConsoleWidget.h"
#if COI_WITH_DEV_CONSOLE
#include "Simulation/SimulationSubsystem.h"
#include "Framework/Application/SlateApplication.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SMultiLineEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"
#include "Styling/CoreStyle.h"
void SCoIDeveloperConsole::Construct(const FArguments& Args){Sim=Args._Sim;ToggleKey=Args._ToggleKey;Close=Args._OnClose;
 ChildSlot[SNew(SBorder).Padding(18).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).BorderBackgroundColor(FLinearColor(.018f,.024f,.03f,1))[
 SNew(SVerticalBox)
 +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)[SNew(STextBlock).Text(FText::FromString(FString(TEXT("COI_DEVELOPER_CONSOLE_AP07A · Lokal · "))+ToggleKey.GetDisplayName().ToString()+TEXT(" / Esc schließen"))).Font(FCoreStyle::GetDefaultFontStyle("Bold",15)).ColorAndOpacity(FLinearColor(.9f,.7f,.35f))]
 +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,8)[SAssignNew(Progress,STextBlock).AutoWrapText(true)]
 +SVerticalBox::Slot().FillHeight(1).Padding(0,0,0,10)[SAssignNew(Output,SMultiLineEditableTextBox).IsReadOnly(true).AutoWrapText(true).Font(FCoreStyle::GetDefaultFontStyle("Regular",13))]
 +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,10)[SAssignNew(Entry,SEditableTextBox).ClearKeyboardFocusOnCommit(false).HintText(FText::FromString(TEXT("help · list companies · inspect company company.player · Enter ausführen · Pfeil hoch/runter: Historie"))).OnTextCommitted(this,&SCoIDeveloperConsole::Submit)]
 +SVerticalBox::Slot().AutoHeight()[SNew(SHorizontalBox)
 +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).Text(FText::FromString(TEXT("Hilfe"))).OnClicked_Lambda([this]{return Run(TEXT("help"));})]
 +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).Text(FText::FromString(TEXT("Welt validieren"))).OnClicked_Lambda([this]{return Run(TEXT("validate world"));})]
 +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).Text(FText::FromString(TEXT("Vorschub abbrechen"))).OnClicked_Lambda([this]{return Run(TEXT("cancel"));})]
 +SHorizontalBox::Slot().AutoWidth().Padding(0,0,8,0)[SNew(SButton).Text(FText::FromString(TEXT("Log exportieren"))).OnClicked_Lambda([this]{return Run(TEXT("export log"));})]
 +SHorizontalBox::Slot().AutoWidth()[SNew(SButton).Text(FText::FromString(TEXT("Schließen"))).OnClicked_Lambda([this]{Close.ExecuteIfBound();return FReply::Handled();})]
 ]]];Refresh();}
TSharedRef<SWidget> SCoIDeveloperConsole::FocusTarget() const{return Entry.ToSharedRef();}
void SCoIDeveloperConsole::FocusInput(){if(Entry)FSlateApplication::Get().SetKeyboardFocus(Entry,EFocusCause::SetDirectly);}
void SCoIDeveloperConsole::Refresh(){if(!Sim.IsValid())return;FString Text;for(const auto& L:Sim->Developer().Output())Text+=L+TEXT("\n");if(Text.IsEmpty())Text=TEXT("help listet Befehle. Eingriffe werden protokolliert.");if(Output->GetText().ToString()!=Text)Output->SetText(FText::FromString(Text));Progress->SetText(FText::FromString(Sim->DeveloperStatus()));}
void SCoIDeveloperConsole::Tick(const FGeometry& G,double Now,float Dt){SCompoundWidget::Tick(G,Now,Dt);if(Now-LastRefresh>.2){LastRefresh=Now;Refresh();}}
FReply SCoIDeveloperConsole::Run(const FString& Text){if(Sim.IsValid())Sim->ExecuteDeveloper(Text);HistoryIndex=-1;Refresh();FocusInput();return FReply::Handled();}
void SCoIDeveloperConsole::Submit(const FText& Text,ETextCommit::Type Commit){if(Commit==ETextCommit::OnEnter&&!Text.IsEmpty()){Run(Text.ToString());Entry->SetText(FText::GetEmpty());FocusInput();}}
FReply SCoIDeveloperConsole::OnPreviewKeyDown(const FGeometry&,const FKeyEvent& E){if(E.GetKey()==EKeys::Escape||E.GetKey()==ToggleKey){Close.ExecuteIfBound();return FReply::Handled();}if(Sim.IsValid()&&(E.GetKey()==EKeys::Up||E.GetKey()==EKeys::Down)){const auto& H=Sim->Developer().History();if(!H.IsEmpty()){if(HistoryIndex<0)HistoryIndex=H.Num();HistoryIndex=FMath::Clamp(HistoryIndex+(E.GetKey()==EKeys::Up?-1:1),0,H.Num());Entry->SetText(HistoryIndex==H.Num()?FText::GetEmpty():FText::FromString(H[HistoryIndex]));FocusInput();}return FReply::Handled();}return FReply::Unhandled();}
#endif
