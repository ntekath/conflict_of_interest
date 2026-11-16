#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "GeoMapData.h"
#include "MapSelection.h"
#include "MapInterface.generated.h"
UCLASS()
class UCoIMapButton : public UButton
{
 GENERATED_BODY()
public: UCoIMapButton(){InitIsFocusable(false);}
};
class UTextBlock;
class UBorder;
UCLASS()
class CONFLICTOFINTEREST_API UCoIMapInterface : public UUserWidget
{
 GENERATED_BODY()
public:
 bool IsPointerOverPanel() const;
 const FString& GetSelectedId() const { return Selection.Id; }
protected:
 virtual TSharedRef<SWidget> RebuildWidget() override;
 virtual void NativeConstruct() override;
 virtual void NativeDestruct() override;
 virtual void NativeTick(const FGeometry&,float) override;
 virtual int32 NativePaint(const FPaintArgs&,const FGeometry&,const FSlateRect&,FSlateWindowElementList&,int32,const FWidgetStyle&,bool) const override;
 virtual FReply NativeOnMouseButtonDown(const FGeometry&,const FPointerEvent&) override;
 virtual FReply NativeOnMouseWheel(const FGeometry&,const FPointerEvent&) override;
private:
 FCoIGeoMapData Data;
 FCoISelection Selection;
 ECoIMapView View=ECoIMapView::Regions;
 UPROPERTY() TObjectPtr<UTextBlock> Information;
 UPROPERTY() TObjectPtr<UTextBlock> ViewLabel;
 UPROPERTY() TObjectPtr<UBorder> Panel;
 TMap<FName,bool> OriginalHidden;
 bool bReady=false;
 bool Project(FVector2D World,FVector2D& Local) const;
 bool InsidePanel(FVector2D Absolute) const;
 void RefreshInformation();
 void ApplyView();
 UFUNCTION() void Regions();
 UFUNCTION() void Countries();
 UFUNCTION() void Ownership();
 UFUNCTION() void ClearSelection();
};
