#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "Components/ComboBoxString.h"
#include "Simulation/SimCore.h"
#include "PlayerHUD.generated.h"
class UVerticalBox;class UHorizontalBox;class UTextBlock;class UBorder;class UCanvasPanel;class UScrollBox;class UEditableTextBox;class UComboBoxString;
UCLASS()
class UCoIChoice : public UComboBoxString {GENERATED_BODY() public:UCoIChoice();protected:virtual TSharedRef<SWidget> HandleGenerateWidget(TSharedPtr<FString> Item) const override;};
UCLASS()
class UCoIActionButton : public UButton {
 GENERATED_BODY()
public:
 UCoIActionButton();FString Action;
 UPROPERTY() TObjectPtr<class UCoIPlayerHUD> OwnerHUD;
 UFUNCTION() void Dispatch();
};
UCLASS()
class CONFLICTOFINTEREST_API UCoIPlayerHUD : public UUserWidget {
 GENERATED_BODY()
public:
 UPROPERTY() TObjectPtr<class UCoIMapInterface> MapOwner;
 void Action(const FString& Key);void MapSelection(const FString& Id,const FString& Summary=TEXT(""));bool PointerBlocked(FVector2D Absolute) const;bool OverlayCovered(FVector2D Position,FVector2D Size=FVector2D::ZeroVector) const;bool KeyboardBlocked() const;bool Escape();
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual void NativeConstruct() override;virtual void NativeDestruct() override;virtual void NativeTick(const FGeometry&,float) override;
 virtual FReply NativeOnPreviewKeyDown(const FGeometry&,const FKeyEvent&) override;
 virtual FReply NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
 virtual FReply NativeOnMouseWheel(const FGeometry&,const FPointerEvent&) override;
private:
 UPROPERTY() TObjectPtr<class UCoISimulationSubsystem> Sim;
 UPROPERTY() TObjectPtr<UCanvasPanel> Root;
 UPROPERTY() TObjectPtr<UBorder> MapControls;UPROPERTY() TObjectPtr<UBorder> MapLegend;
 UPROPERTY() TObjectPtr<UTextBlock> LegendText;UPROPERTY() TObjectPtr<UTextBlock> MetricDetails;
 UPROPERTY() TArray<TObjectPtr<UBorder>> LegendColors;
 void RefreshMap();
 UPROPERTY() TObjectPtr<UBorder> Top;UPROPERTY() TObjectPtr<UBorder> Nav;UPROPERTY() TObjectPtr<UBorder> Work;UPROPERTY() TObjectPtr<UBorder> SelectionPanel;UPROPERTY() TObjectPtr<UBorder> Notices;UPROPERTY() TObjectPtr<UBorder> Debug;UPROPERTY() TObjectPtr<UBorder> ModalShade;UPROPERTY() TObjectPtr<UBorder> Dialog;
 UPROPERTY() TObjectPtr<UTextBlock> AccountText;UPROPERTY() TObjectPtr<UTextBlock> ClockText;UPROPERTY() TObjectPtr<UTextBlock> SelectionText;UPROPERTY() TObjectPtr<UTextBlock> NoticeText;UPROPERTY() TObjectPtr<UTextBlock> DebugText;UPROPERTY() TObjectPtr<UTextBlock> WorkTitle;UPROPERTY() TObjectPtr<UTextBlock> FormPreview;UPROPERTY() TObjectPtr<UTextBlock> FormState;
 UPROPERTY() TObjectPtr<UCoIActionButton> PauseButton;UPROPERTY() TObjectPtr<UCoIActionButton> ConfirmButton;
 UPROPERTY() TObjectPtr<UVerticalBox> WorkBox;UPROPERTY() TObjectPtr<UVerticalBox> Body;UPROPERTY() TObjectPtr<UVerticalBox> Form;UPROPERTY() TObjectPtr<UScrollBox> WorkScroll;
 UPROPERTY() TMap<FString,TObjectPtr<UComboBoxString>> Choices;
 UPROPERTY() TMap<FString,TObjectPtr<UEditableTextBox>> Fields;
 UPROPERTY() TArray<TObjectPtr<UCoIActionButton>> NavButtons;
 TMap<FString,TMap<FString,FString>> ChoiceValues;TMap<FString,FString> Selected;
 FDelegateHandle Changed;FString FocusedObject,SelectionSummary;FString View,SelectionId,DialogKind,ContextId,LastStatus;bool Updating=false,SelectionCollapsed=false;
 uint64 SeenGeneration=0,SeenVersion=MAX_uint64,QuoteVersion=0;int64 Token=0,PendingTicket=0;uint64 QuoteGeneration=0;int64 QuoteHour=0;
 CoISim::FCommand Draft;bool ValidDraft=false;
 void Refresh();void BuildView(const FString& NewView);void RefreshTable();void RefreshSelection();void OpenTrade(bool Buy,const FString& Id=TEXT(""));void OpenTransport(const FString& Stock=TEXT(""),bool Empty=false);void OpenCancel(const FString& Id,bool Transport);void OpenDialog(const FString& Title);void Preview();void Confirm();void CloseDialog();
 UTextBlock* Text(class UPanelWidget* Parent,const FString& Value,int32 Size=13,bool Gold=false);
 UCoIActionButton* Button(class UPanelWidget* Parent,const FString& Caption,const FString& Key,bool Accent=false,const FString& Tip=TEXT(""));
 UHorizontalBox* Row(class UPanelWidget* Parent);
 UBorder* Surface(const FVector2D& Pos,const FVector2D& Size);
 UComboBoxString* Choice(class UPanelWidget* Parent,const FString& Key,const FString& Label,const TArray<TPair<FString,FString>>& Options,const FString& Default=TEXT(""));
 void Field(class UPanelWidget* Parent,const FString& Key,const FString& Label,const FString& Value);
 void Cells(UVerticalBox* Box,const TArray<FString>& Values,const TArray<float>& Weights,bool Header=false,const FString& Caption=TEXT(""),const FString& Key=TEXT(""),const FString& Caption2=TEXT(""),const FString& Key2=TEXT(""));
 bool ReadPositive(const FString& Key,int64& Value,bool AllowZero=false) const;
 FString Id(const FString& Key) const;FString City(const FString&) const;FString Good(const FString&) const;FString Store(const FString&) const;FString Status(const FString&) const;FString Time(int64 Hour) const;
 UFUNCTION() void ChoiceChanged(FString Value,ESelectInfo::Type Type);
 UFUNCTION() void InputChanged(const FText& Value);
};
