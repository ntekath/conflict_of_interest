#pragma once
#include "Simulation/DeveloperBuild.h"
#if COI_WITH_DEV_CONSOLE
#include "Widgets/SCompoundWidget.h"
class UCoISimulationSubsystem;class SEditableTextBox;class SMultiLineEditableTextBox;class STextBlock;
class SCoIDeveloperConsole : public SCompoundWidget {
public:
 SLATE_BEGIN_ARGS(SCoIDeveloperConsole){} SLATE_ARGUMENT(UCoISimulationSubsystem*,Sim) SLATE_ARGUMENT(FKey,ToggleKey) SLATE_EVENT(FSimpleDelegate,OnClose) SLATE_END_ARGS()
 virtual bool SupportsKeyboardFocus() const override{return true;}
 void Construct(const FArguments&);void FocusInput();TSharedRef<SWidget> FocusTarget() const;
 virtual void Tick(const FGeometry&,double,float) override;
 virtual FReply OnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
 virtual FReply OnMouseButtonDown(const FGeometry&,const FPointerEvent&) override{return FReply::Handled();}
 virtual FReply OnMouseButtonUp(const FGeometry&,const FPointerEvent&) override{return FReply::Handled();}
 virtual FReply OnMouseWheel(const FGeometry&,const FPointerEvent&) override{return FReply::Handled();}
private:
 TWeakObjectPtr<UCoISimulationSubsystem> Sim;FKey ToggleKey;FSimpleDelegate Close;
 TSharedPtr<SEditableTextBox> Entry;TSharedPtr<SMultiLineEditableTextBox> Output;TSharedPtr<STextBlock> Progress;
 int32 HistoryIndex=-1;double LastRefresh=0;
 void Submit(const FText&,ETextCommit::Type);FReply Run(const FString&);void Refresh();
};
#endif
