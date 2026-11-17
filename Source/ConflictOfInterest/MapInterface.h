#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Components/Button.h"
#include "GeoMapData.h"
#include "MapSelection.h"
#include "Simulation/MapMetrics.h"
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
 bool IsPointerOverPanel() const;bool IsKeyboardBlocked() const;bool HandleEscape();void SetMapMode(const FString& Mode);
 void SetMetricLayer(const FString& Layer,const FString& Good,CoISim::EMapMetric Kind);
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
 UPROPERTY() TObjectPtr<class UCoIPlayerHUD> PlayerHUD;
 FString MetricLayer=TEXT("regions"),MetricGood=TEXT("good.food");CoISim::EMapMetric MetricKind=CoISim::EMapMetric::Households;int64 MetricStep=-1;
 UPROPERTY() TMap<FName,TObjectPtr<class UMaterialInterface>> BaseMaterials;
 UPROPERTY() TMap<FName,TObjectPtr<class UMaterialInstanceDynamic>> MetricMaterials;
 void ApplyMetricLayer();
 FCoIGeoMapData Data;
 FCoISelection Selection;
 ECoIMapView View=ECoIMapView::Regions;
 UPROPERTY() TObjectPtr<UTextBlock> Information;
 UPROPERTY() TObjectPtr<UTextBlock> ViewLabel;
 UPROPERTY() TObjectPtr<UTextBlock> SimulationLabel;
 UPROPERTY() TObjectPtr<UCoIMapButton> PauseControl;
 UPROPERTY() TObjectPtr<UTextBlock> SimulationDiagnostics;
 UPROPERTY() TObjectPtr<class UCoISimulationSubsystem> Simulation;
 FDelegateHandle SimulationChanged;
 void RefreshSimulation();
 UFUNCTION() void ToggleSimulation();
 UFUNCTION() void SpeedOne();
 UFUNCTION() void SpeedFour();
 UFUNCTION() void SpeedSixteen();
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
