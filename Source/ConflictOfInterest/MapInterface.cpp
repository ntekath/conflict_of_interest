#include "MapInterface.h"
#include "UI/PlayerHUD.h"
#include "GeoCatalog.h"
#include "MapCameraPawn.h"
#include "Simulation/SimulationSubsystem.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/VerticalBox.h"
#include "Components/VerticalBoxSlot.h"
#include "Components/TextBlock.h"
#include "Engine/StaticMeshActor.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Framework/Application/SlateApplication.h"
#include "Rendering/DrawElements.h"
#include "Styling/CoreStyle.h"

TSharedRef<SWidget> UCoIMapInterface::RebuildWidget()
{
 if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree"),RF_Transient);
 auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
 PlayerHUD=CreateWidget<UCoIPlayerHUD>(GetOwningPlayer(),UCoIPlayerHUD::StaticClass());PlayerHUD->MapOwner=this;
 auto* HUDSlot=Root->AddChildToCanvas(PlayerHUD);HUDSlot->SetAnchors(FAnchors(0,0,1,1));HUDSlot->SetOffsets(FMargin(0));
 return Super::RebuildWidget();
}
void UCoIMapInterface::NativeConstruct()
{
 Super::NativeConstruct();ForceVolatile(true);SetVisibility(ESlateVisibility::Visible);
 for(TActorIterator<ACoIGeoReference> It(GetWorld());It;++It)if(It->Catalog)
 {FString Error;bReady=Data.Load(It->Catalog->DefinitionJson,Error);if(!bReady)UE_LOG(LogTemp,Error,TEXT("COI_UI %s"),*Error);break;}
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
 {OriginalHidden.Add(It->GetFName(),It->IsHidden());if(It->ActorHasTag(TEXT("display.city_markers")))It->SetActorHiddenInGame(true);}
 Simulation=GetWorld()->GetSubsystem<UCoISimulationSubsystem>();
 if(Simulation)SimulationChanged=Simulation->OnChanged.AddUObject(this,&UCoIMapInterface::RefreshSimulation);
 ApplyView();RefreshSimulation();
}
void UCoIMapInterface::NativeTick(const FGeometry& G,float Dt)
{
 Super::NativeTick(G,Dt);
 if(Panel)if(auto* PanelSlot=Cast<UCanvasPanelSlot>(Panel->Slot))PanelSlot->SetSize({300,FMath::Max(140.,FMath::Min(590.,G.GetLocalSize().Y-36))});
}
void UCoIMapInterface::NativeDestruct()
{
 if(Simulation)Simulation->OnChanged.Remove(SimulationChanged);SimulationChanged.Reset();Simulation=nullptr;
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)if(auto* Hidden=OriginalHidden.Find(It->GetFName()))It->SetActorHiddenInGame(*Hidden);
 OriginalHidden.Empty();Super::NativeDestruct();
}
void UCoIMapInterface::Regions(){View=ECoIMapView::Regions;ApplyView();}
void UCoIMapInterface::Countries(){View=ECoIMapView::Countries;ApplyView();}
void UCoIMapInterface::Ownership(){View=ECoIMapView::Ownership;ApplyView();}
void UCoIMapInterface::ClearSelection(){Selection.Id.Empty();RefreshInformation();}
void UCoIMapInterface::ApplyView()
{
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
  for(const auto& R:Data.Regions)if(It->ActorHasTag(FName(*R.Id)))It->SetActorHiddenInGame(View==ECoIMapView::Countries);
 if(ViewLabel)ViewLabel->SetText(FText::FromString(View==ECoIMapView::Countries?TEXT("Ansicht: Länder"):View==ECoIMapView::Regions?TEXT("Ansicht: Testregionen"):TEXT("Besitz: nicht zugewiesen")));
 RefreshInformation();
}
void UCoIMapInterface::RefreshInformation()
{
 if(PlayerHUD){FString Summary;for(const auto& C:Data.Countries)if(C.Id==Selection.Id)Summary=C.Name+TEXT(" · Land\nWerte werden aus Regionen aggregiert.");PlayerHUD->MapSelection(Selection.Id,Summary);return;}
 if(!Information)return;
 FString Info=TEXT("Ort auswählen\n\nStadt vor Fläche. Wasser und freier Hintergrund heben die Auswahl auf.");
 auto Name=[&](const TArray<FCoIGeoSurface>& List,const FString& Id){for(const auto& S:List)if(S.Id==Id)return S.Name;return Id;};
 for(const auto& C:Data.Cities)if(C.Id==Selection.Id)
  Info=FString::Printf(TEXT("STADT\n%s\n\nLand: %s\nTestregion: %s\n%.4f N, %.4f W\n\nBesitz: nicht zugewiesen\nAP05: aktive Testwirtschaft"),*C.Name,*Name(Data.Countries,C.CountryId),*Name(Data.Regions,C.RegionId),C.Latitude,-C.Longitude);
 for(const auto& R:Data.Regions)if(R.Id==Selection.Id)
  Info=FString::Printf(TEXT("WIRTSCHAFTSREGION\n%s\n\nLand: %s\nFiktives Testgebiet\nKeine amtliche Verwaltungsgrenze\n\nBesitz: nicht zugewiesen\nAP05: aktive Testwirtschaft"),*R.Name,*Name(Data.Countries,R.CountryId));
 for(const auto& C:Data.Countries)if(C.Id==Selection.Id)
  Info=FString::Printf(TEXT("LAND\n%s\n\n%s\nModerne Geografie als Näherung für 1913.\n\nKeine zusätzliche Landesbevölkerung."),*C.Name,C.Id==TEXT("country.usa")?TEXT("Vier fiktive US-Testregionen"):TEXT("Geografischer Kontext; keine aktive Wirtschaftsregion"));
 if(Simulation&&Simulation->IsReady())
 {
  const auto& Snapshot=Simulation->GetSnapshot();
  if(const auto* Detail=Snapshot.EconomyDetails.Find(Selection.Id))Info+=*Detail;
  for(const auto& R:Snapshot.Regions)if(R.Id==Selection.Id)Info+=FString::Printf(TEXT("\n\nTestbevölkerung: %lld\nBeschäftigte: %lld\nZuletzt berechnet: Schritt %lld"),R.Population,R.Employed,R.LastProcessedStep);
  for(const auto& C:Snapshot.Countries)if(C.Id==Selection.Id)Info+=FString::Printf(TEXT("\n\nAus Regionen aggregiert:\nBevölkerung: %lld\nBeschäftigte: %lld"),C.Population,C.Employed);
 }
 if(Simulation&&Simulation->IsReady()&&Selection.Id.IsEmpty())Info+=TEXT("\n\n")+Simulation->GetSnapshot().EconomySummary;
 Information->SetText(FText::FromString(Info));
}
bool UCoIMapInterface::Project(FVector2D World,FVector2D& Local) const
{
 FVector2D Screen;if(!GetOwningPlayer()||!GetOwningPlayer()->ProjectWorldLocationToScreen({World.X,World.Y,4},Screen,true))return false;
 const auto Viewport=UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(GetOwningPlayer());
 const double Scale=UWidgetLayoutLibrary::GetViewportScale(this);
 Local=GetCachedGeometry().AbsoluteToLocal(Viewport.LocalToAbsolute(Screen/Scale));return true;
}
bool UCoIMapInterface::InsidePanel(FVector2D Absolute) const
 {
  if(PlayerHUD)return PlayerHUD->PointerBlocked(Absolute);
  if(!Panel)return false;
  auto Local=GetCachedGeometry().AbsoluteToLocal(Absolute);
  auto* PanelSlot=Cast<UCanvasPanelSlot>(Panel->Slot);
  if(!PanelSlot)return false;
  auto Position=PanelSlot->GetPosition(),Size=PanelSlot->GetSize();
  return Local.X>=Position.X&&Local.Y>=Position.Y&&Local.X<=Position.X+Size.X&&Local.Y<=Position.Y+Size.Y;
 }
bool UCoIMapInterface::IsPointerOverPanel() const
 {return FSlateApplication::IsInitialized()&&InsidePanel(FSlateApplication::Get().GetCursorPos());}
FReply UCoIMapInterface::NativeOnMouseWheel(const FGeometry& G,const FPointerEvent& E)
 {return InsidePanel(E.GetScreenSpacePosition())?FReply::Handled():FReply::Unhandled();}
FReply UCoIMapInterface::NativeOnMouseButtonDown(const FGeometry& G,const FPointerEvent& E)
{
 if(E.GetEffectingButton()!=EKeys::LeftMouseButton)return FReply::Unhandled();
 if(InsidePanel(E.GetScreenSpacePosition()))return FReply::Handled();
 if(!bReady)return FReply::Handled();
 auto Pointer=G.AbsoluteToLocal(E.GetScreenSpacePosition());TArray<FVector2D> Centers;TArray<bool> Visible;
 for(const auto& C:Data.Cities){FVector2D P;bool OK=Project(C.Position,P);Centers.Add(P);Visible.Add(OK&&P.X>=8&&P.Y>=8&&P.X<=G.GetLocalSize().X-8&&P.Y<=G.GetLocalSize().Y-8&&!InsidePanel(G.LocalToAbsolute(P)));}
 int32 City=CoISelection::PickCity(Centers,Visible,Pointer);
 if(City!=INDEX_NONE)Selection.Id=Data.Cities[City].Id;
 else
 {
  FVector O,D;auto* PC=GetOwningPlayer();
  // Deproject uses physical viewport pixels; the pointer event may have DPI scale.
  const auto VG=UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(PC);
  FVector2D Pixel=VG.AbsoluteToLocal(E.GetScreenSpacePosition())*UWidgetLayoutLibrary::GetViewportScale(this);
  if(PC->DeprojectScreenPositionToWorld(Pixel.X,Pixel.Y,O,D)&&D.Z<-.000001f)
  {auto P=O+D*(-O.Z/D.Z);Selection.Id=CoISelection::Pick(Data,{P.X,P.Y},View,Centers,Visible,Pointer);}
  else Selection.Id.Empty();
 }
 RefreshInformation();UE_LOG(LogTemp,Display,TEXT("COI_SELECT %s"),*Selection.Id);return FReply::Handled();
}
int32 UCoIMapInterface::NativePaint(const FPaintArgs& Args,const FGeometry& G,const FSlateRect& Cull,FSlateWindowElementList& Out,int32 Layer,const FWidgetStyle& Style,bool Enabled) const
{
 int32 Top=Layer;if(PlayerHUD&&PlayerHUD->KeyboardBlocked())return Super::NativePaint(Args,G,Cull,Out,Layer,Style,Enabled);if(!bReady)return Super::NativePaint(Args,G,Cull,Out,Layer,Style,Enabled);
 const FLinearColor Gold(1,.78f,.24f),Orange(1,.48f,.08f);const auto Font=FCoreStyle::GetDefaultFontStyle("Bold",12);
 auto Line=[&](TArray<FVector2D> P,FLinearColor Color,float Width){FSlateDrawElement::MakeLines(Out,Top+1,G.ToPaintGeometry(FVector2f(G.GetLocalSize()),FSlateLayoutTransform()),P,ESlateDrawEffect::None,Color,true,Width);};
 // All rings, including islands/holes, follow the original selection ID even after a mode change.
 auto Highlight=[&](const TArray<FCoIGeoSurface>& Surfaces)
 {
  for(const auto& S:Surfaces)if(S.Id==Selection.Id)for(const auto& Ring:S.Rings)
  {
   for(int I=1;I<Ring.Num();++I)
   {
    FVector2D A,B;if(!Project(Ring[I-1],A)||!Project(Ring[I],B))continue;
    // The panel occupies the left edge. Clip segments at its right edge rather than crossing UI.
    const double Right=0;
    if(A.X<Right&&B.X<Right)continue;
    if(A.X<Right)A=FMath::Lerp(A,B,(Right-A.X)/(B.X-A.X));
    if(B.X<Right)B=FMath::Lerp(B,A,(Right-B.X)/(A.X-B.X));
    Line({A,B},Gold,2.5f);
   }
  }
 };
 Highlight(Data.Countries);Highlight(Data.Regions);
 Top=Super::NativePaint(Args,G,Cull,Out,Top+2,Style,Enabled);
 // Clip custom map overlays away from the information panel as well as the viewport edge.
 // Draw symbols first; labels then use a deterministic collision budget.
 TArray<FVector2D> Occupied,OccupiedSizes;Occupied.Add({0,0});OccupiedSizes.Add(FVector2D(G.GetLocalSize().X,132));Occupied.Add({G.GetLocalSize().X-322,82});OccupiedSizes.Add({306,180});
 auto Size=G.GetLocalSize();auto* Pawn=Cast<AMapCameraPawn>(GetOwningPlayer()->GetPawn());double Height=Pawn?Pawn->GetZoomHeight():21000;
 for(const auto& C:Data.Cities)
 {
  FVector2D P;if(!Project(C.Position,P)||P.X<8||P.Y<8||P.X>Size.X-8||P.Y>Size.Y-8||InsidePanel(G.LocalToAbsolute(P)))continue;
  const bool Selected=C.Id==Selection.Id;
  const auto* Brush=FCoreStyle::Get().GetBrush("WhiteBrush");
  if(Selected)FSlateDrawElement::MakeBox(Out,Top+1,G.ToPaintGeometry(FVector2f(20,20),FSlateLayoutTransform(FVector2f(P-FVector2D(10,10)))),Brush,ESlateDrawEffect::None,Gold);
  FSlateDrawElement::MakeBox(Out,Top+2,G.ToPaintGeometry(FVector2f(12,12),FSlateLayoutTransform(FVector2f(P-FVector2D(6,6)))),Brush,ESlateDrawEffect::None,Orange);

 }
 for(int Pass=0;Pass<2;++Pass)for(const auto& C:Data.Cities)
 {
  bool Selected=C.Id==Selection.Id;if(Selected!=(Pass==0)||(!Selected&&Height>16000))continue;
  FVector2D P;if(!Project(C.Position,P))continue;FVector2D TextSize(C.Name.Len()*8+12,22),At=P+FVector2D(12,-12);
  if(At.X+TextSize.X>Size.X-8)At.X=P.X-TextSize.X-12;
  if(At.X<8||At.Y<8||At.Y+TextSize.Y>Size.Y-8)continue;
  bool Blocked=false;for(int I=0;I<Occupied.Num();++I)if(CoISelection::Overlaps(At,TextSize,Occupied[I],OccupiedSizes[I])){Blocked=true;break;}if(Blocked)continue;
  Occupied.Add(At);OccupiedSizes.Add(TextSize);
  FSlateDrawElement::MakeBox(Out,Top+2,G.ToPaintGeometry(FVector2f(TextSize),FSlateLayoutTransform(FVector2f(At))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(.015f,.025f,.035f,.95f));
  FSlateDrawElement::MakeText(Out,Top+3,G.ToPaintGeometry(FVector2f(TextSize),FSlateLayoutTransform(FVector2f(At+FVector2D(5,1)))),C.Name,Font,ESlateDrawEffect::None,Selected?Gold:FLinearColor::White);
 }
 if(View!=ECoIMapView::Countries&&Height<12000)for(const auto& R:Data.Regions)
 {
  double Largest=0;FVector2D Anchor=FVector2D::ZeroVector;
  for(int I=0;I<R.Triangles.Num();I+=3)
  {auto A=R.Vertices[R.Triangles[I]],B=R.Vertices[R.Triangles[I+1]],C=R.Vertices[R.Triangles[I+2]];double Area=FMath::Abs((B.X-A.X)*(C.Y-A.Y)-(B.Y-A.Y)*(C.X-A.X));if(Area>Largest){Largest=Area;Anchor=(A+B+C)/3;}}
  FVector2D At;if(!Project(Anchor,At))continue;
  FString Label=R.Name+(View==ECoIMapView::Ownership?TEXT(" | nicht zugewiesen"):TEXT(" | TEST"));FVector2D TextSize(Label.Len()*7+12,22);At-=TextSize/2;
  if(At.X<8||At.Y<8||At.X+TextSize.X>Size.X-8||At.Y+TextSize.Y>Size.Y-8)continue;
  bool Blocked=false;for(int I=0;I<Occupied.Num();++I)if(CoISelection::Overlaps(At,TextSize,Occupied[I],OccupiedSizes[I])){Blocked=true;break;}if(Blocked)continue;
  Occupied.Add(At);OccupiedSizes.Add(TextSize);
  FSlateDrawElement::MakeBox(Out,Top+2,G.ToPaintGeometry(FVector2f(TextSize),FSlateLayoutTransform(FVector2f(At))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(.015f,.025f,.035f,.95f));
  FSlateDrawElement::MakeText(Out,Top+3,G.ToPaintGeometry(FVector2f(TextSize),FSlateLayoutTransform(FVector2f(At+FVector2D(5,1)))),Label,FCoreStyle::GetDefaultFontStyle("Regular",11),ESlateDrawEffect::None,FLinearColor(.75f,.87f,.77f));
 }
 return Top+3;
}

void UCoIMapInterface::ToggleSimulation(){if(Simulation)Simulation->TogglePause();}
void UCoIMapInterface::SpeedOne(){if(Simulation)Simulation->SetSpeed(1);}
void UCoIMapInterface::SpeedFour(){if(Simulation)Simulation->SetSpeed(4);}
void UCoIMapInterface::SpeedSixteen(){if(Simulation)Simulation->SetSpeed(16);}
void UCoIMapInterface::RefreshSimulation()
{
 if(!SimulationLabel||!SimulationDiagnostics)return;
 if(!Simulation||!Simulation->IsReady())
 {SimulationLabel->SetText(FText::FromString(Simulation?Simulation->GetError():TEXT("Simulation nicht verfügbar")));return;}
 const auto& S=Simulation->GetSnapshot();const auto& D=Simulation->GetDiagnostics();
 if(PauseControl)if(auto* Caption=Cast<UTextBlock>(PauseControl->GetContent()))Caption->SetText(FText::FromString(Simulation->IsPaused()?TEXT("Start"):TEXT("Pause")));
 SimulationLabel->SetText(FText::FromString(FString::Printf(TEXT("%s | %s | %dx\n%s: %.2f USD (reserviert %.2f)\nLager %lld/%lld Punkte | Lkw: %lld\nFIKTIVE TESTDATEN | AP05 aktiv"),*S.Date,Simulation->IsPaused()?TEXT("PAUSE"):TEXT("LÄUFT"),Simulation->GetSpeed(),*S.PlayerName,S.PlayerMoneyCents/100.,S.PlayerReservedCents/100.,S.PlayerStorageUsed,S.PlayerStorageCapacity,S.PlayerTrucks)));
 SimulationDiagnostics->SetText(FText::FromString(FString::Printf(TEXT("Schritt %lld | Version %llu | %.1f h/s tatsächlich\nSchritt %.0f µs | GT %.0f µs | Snapshot %.0f µs\nRückstau %lld | Befehle %lld | %s"),S.Step,S.Version,Simulation->IsPaused()?0.:D.ActualHoursPerSecond,D.StepMicros,Simulation->GetGameThreadMicros(),D.SnapshotMicros,D.BacklogSteps,D.PendingCommands,D.BudgetLimited?TEXT("Budget begrenzt"):TEXT("Budget frei"))));
 RefreshInformation();
}

bool UCoIMapInterface::IsKeyboardBlocked() const{return PlayerHUD&&PlayerHUD->KeyboardBlocked();}
bool UCoIMapInterface::HandleEscape(){return PlayerHUD&&PlayerHUD->Escape();}
void UCoIMapInterface::SetMapMode(const FString& Mode){if(Mode==TEXT("countries"))Countries();else if(Mode==TEXT("ownership"))Ownership();else Regions();}
