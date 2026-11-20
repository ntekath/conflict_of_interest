#include "MapInterface.h"
#include "UI/PlayerHUD.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GeoCatalog.h"
#include "MapCameraPawn.h"
#include "Simulation/SimulationSubsystem.h"
#include "Simulation/SimArchive.h"
#include "Simulation/RouteDisplay.h"
#include "Misc/App.h"
#include "Blueprint/WidgetTree.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "Components/Border.h"
#include "Components/ScrollBox.h"
#include "Components/HorizontalBox.h"
#include "Components/HorizontalBoxSlot.h"
#include "Components/Button.h"
#include "Components/ButtonSlot.h"
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
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

TSharedRef<SWidget> UCoIMapInterface::RebuildWidget()
{
 if(!WidgetTree)WidgetTree=NewObject<UWidgetTree>(this,TEXT("WidgetTree"),RF_Transient);
 auto* Root=WidgetTree->ConstructWidget<UCanvasPanel>();WidgetTree->RootWidget=Root;
 for(TActorIterator<ACoIGeoReference> It(GetWorld());It;++It)if(It->Catalog){FString Error;Data.Load(It->Catalog->DefinitionJson,Error);break;}
 if(Data.bGeographyOnly&&Data.SimulationFile.IsEmpty())
 {
  Panel=WidgetTree->ConstructWidget<UBorder>();Panel->SetBrushColor(FLinearColor(.035f,.045f,.055f,.97f));Panel->SetPadding(FMargin(14));
  auto* PreviewSlot=Root->AddChildToCanvas(Panel);PreviewSlot->SetPosition({16,16});PreviewSlot->SetSize({300,590});
  auto* PreviewScroll=WidgetTree->ConstructWidget<UScrollBox>();Panel->SetContent(PreviewScroll);
  auto* Box=WidgetTree->ConstructWidget<UVerticalBox>();PreviewScroll->AddChild(Box);
  auto AddText=[&](const FString& Text,int32 Size){auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Text));T->SetAutoWrapText(true);T->SetColorAndOpacity(FLinearColor(.94f,.90f,.79f));T->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",Size));Box->AddChildToVerticalBox(T)->SetPadding(FMargin(0,0,0,10));return T;};
  AddText(TEXT("DEUTSCHLAND · NIEDERLANDE\n01.01.1913"),18);
  ViewLabel=AddText(TEXT("50 Verwaltungsregionen · 50 Hubs"),12);
  auto* Buttons=WidgetTree->ConstructWidget<UHorizontalBox>();Box->AddChildToVerticalBox(Buttons)->SetPadding(FMargin(0,0,0,12));
  auto MakeButton=[&](const TCHAR* Name){auto* B=WidgetTree->ConstructWidget<UCoIMapButton>();auto* T=WidgetTree->ConstructWidget<UTextBlock>();T->SetText(FText::FromString(Name));T->SetFont(FCoreStyle::GetDefaultFontStyle("Regular",10));T->SetColorAndOpacity(FLinearColor(.08f,.08f,.07f));B->SetContent(T);CastChecked<UButtonSlot>(T->Slot)->SetPadding(FMargin(4,3));Buttons->AddChildToHorizontalBox(B)->SetPadding(FMargin(0,0,6,0));return B;};
  MakeButton(TEXT("Regionen"))->OnClicked.AddDynamic(this,&UCoIMapInterface::Regions);
  MakeButton(TEXT("Länder"))->OnClicked.AddDynamic(this,&UCoIMapInterface::Countries);
  MakeButton(TEXT("Leeren"))->OnClicked.AddDynamic(this,&UCoIMapInterface::ClearSelection);
  Information=AddText(TEXT("Region oder Stadthub auswählen."),13);
  AddText(TEXT("Kartenprüfung MAP02\nWirtschaft für diese Karte folgt mit MAP03.\n\nBlau: Gewässer, keine Schiffbarkeitsfreigabe. Grenzen und Küsten sind historische Kartenrekonstruktionen mit dokumentierten Unsicherheiten.\n\nWASD / Pfeile: bewegen\nMausrad: zoomen"),11);
  auto* Credit=AddText(TEXT("Quellen & Geometriehinweise"),11);
  for(TActorIterator<ACoIGeoReference> It(GetWorld());It;++It)if(It->Catalog){TSharedPtr<FJsonObject> Provenance;FString Attribution;if(FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(It->Catalog->ProvenanceJson),Provenance)&&Provenance->TryGetStringField(TEXT("attribution"),Attribution))Credit->SetToolTipText(FText::FromString(Attribution));break;}
  return Super::RebuildWidget();
 }
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
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)if(auto* M=BaseMaterials.Find(It->GetFName()))It->GetStaticMeshComponent()->SetMaterial(0,M->Get());BaseMaterials.Reset();MetricMaterials.Reset();
 if(Simulation)Simulation->OnChanged.Remove(SimulationChanged);SimulationChanged.Reset();Simulation=nullptr;
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)if(auto* Hidden=OriginalHidden.Find(It->GetFName()))It->SetActorHiddenInGame(*Hidden);
 OriginalHidden.Empty();Super::NativeDestruct();
}
void UCoIMapInterface::Regions(){View=ECoIMapView::Regions;ApplyView();}
void UCoIMapInterface::Countries(){View=ECoIMapView::Countries;ApplyView();}
void UCoIMapInterface::Ownership(){View=ECoIMapView::Ownership;ApplyView();}
void UCoIMapInterface::ClearSelection(){Selection.Id.Empty();LastGeoHit={};RefreshInformation();}
void UCoIMapInterface::ApplyView()
{
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It)
  for(const auto& R:Data.Regions)if(It->ActorHasTag(FName(*R.Id)))It->SetActorHiddenInGame(View==ECoIMapView::Countries);
 if(ViewLabel)ViewLabel->SetText(FText::FromString(View==ECoIMapView::Countries?TEXT("Ansicht: Länder"):View==ECoIMapView::Regions?TEXT("Ansicht: Verwaltungsregionen"):TEXT("Besitz: nicht zugewiesen")));
 RefreshInformation();
}
void UCoIMapInterface::RefreshInformation()
{
 if(PlayerHUD){FString Summary;for(const auto& C:Data.Countries)if(C.Id==Selection.Id)Summary=C.Name+TEXT(" · Land\nWerte werden aus Regionen aggregiert.");PlayerHUD->MapSelection(Selection.Id,Summary);return;}
 if(!Information)return;
 if(Data.bGeographyOnly)
 {
  FString Info=TEXT("Region oder Stadthub auswählen.\nLand-/Wasserabfrage und politische Zugehörigkeit sind getrennt.");
  auto FindName=[&](const FString& Id,const TArray<FCoIGeoSurface>& Rows){for(const auto& R:Rows)if(R.Id==Id)return R.Name;return FString();};
  for(const auto& C:Data.Cities)if(C.Id==Selection.Id)Info=FString::Printf(TEXT("STADTHUB\n%s\n\n%s · %s\n%.5f° N, %.5f° E\n\nEin wirtschaftlicher Hub je Region; noch kein Wirtschaftsszenario."),*C.Name,*FindName(C.RegionId,Data.Regions),*FindName(C.CountryId,Data.Countries),C.Latitude,C.Longitude);
  for(const auto& R:Data.Regions)if(R.Id==Selection.Id){Info=R.Name+TEXT("\n")+FindName(R.CountryId,Data.Countries);for(const auto& C:Data.Cities)if(C.RegionId==R.Id)Info+=TEXT("\n\nStadthub: ")+C.Name;}
  for(const auto& C:Data.Countries)if(C.Id==Selection.Id)Info=C.Name+TEXT("\nHistorischer europäischer Szenarioausschnitt.");
  if(!LastGeoHit.WaterId.IsEmpty())Info+=TEXT("\n\nPhysisch: Gewässer\nKeine Schiffbarkeitsfreigabe.");
  else if(LastGeoHit.bLand)Info+=TEXT("\n\nPhysisch: Land");
  Information->SetText(FText::FromString(Info));return;
 }
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
 TArray<TPair<double,FString>> Hits;
 for(const auto& R:ScreenRoutes()){double Dist=1.e9;for(int32 I=1;I<R.Points.Num();++I)Dist=FMath::Min(Dist,CoIRoutes::Distance(Pointer,R.Points[I-1],R.Points[I]));if(Dist<=10)Hits.Add({Dist,R.Id});}
 Hits.Sort([](const auto& A,const auto& B){return FMath::Abs(A.Key-B.Key)>.01?A.Key<B.Key:A.Value<B.Value;});
 if(City==INDEX_NONE&&!Hits.IsEmpty()){RoutePickCycle=(Pointer-LastRoutePointer).Size()<8?RoutePickCycle+1:0;LastRoutePointer=Pointer;Selection.Id=Hits[RoutePickCycle%Hits.Num()].Value;RefreshInformation();return FReply::Handled();}
 if(City!=INDEX_NONE){Selection.Id=Data.Cities[City].Id;LastGeoHit=Data.Query(Data.Cities[City].Position);}
 else
 {
  FVector O,D;auto* PC=GetOwningPlayer();
  // Deproject uses physical viewport pixels; the pointer event may have DPI scale.
  const auto VG=UWidgetLayoutLibrary::GetPlayerScreenWidgetGeometry(PC);
  FVector2D Pixel=VG.AbsoluteToLocal(E.GetScreenSpacePosition())*UWidgetLayoutLibrary::GetViewportScale(this);
  if(PC->DeprojectScreenPositionToWorld(Pixel.X,Pixel.Y,O,D)&&D.Z<-.000001f)
  {auto P=O+D*(-O.Z/D.Z);LastGeoHit=Data.Query({P.X,P.Y});Selection.Id=CoISelection::Pick(Data,{P.X,P.Y},View,Centers,Visible,Pointer);}
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
 Top=Super::NativePaint(Args,G,Cull,Out,Top+2,Style,Enabled);const int32 RouteTop=PaintRoutes(G,Out,Top+1);
 // Clip custom map overlays away from the information panel as well as the viewport edge.
 // Draw symbols first; labels then use a deterministic collision budget.
 TArray<FVector2D> Occupied,OccupiedSizes;Occupied.Add({0,0});OccupiedSizes.Add(FVector2D(G.GetLocalSize().X,132));Occupied.Add({G.GetLocalSize().X-322,82});OccupiedSizes.Add({306,180});
 auto Size=G.GetLocalSize();auto* Pawn=Cast<AMapCameraPawn>(GetOwningPlayer()->GetPawn());double Height=Pawn?Pawn->GetZoomHeight():21000;
 for(const auto& C:Data.Cities)
 {
  FVector2D P;if(!Project(C.Position,P)||P.X<8||P.Y<8||P.X>Size.X-8||P.Y>Size.Y-8||(PlayerHUD?PlayerHUD->OverlayCovered(P):InsidePanel(G.LocalToAbsolute(P))))continue;
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
  bool Blocked=PlayerHUD?PlayerHUD->OverlayCovered(At,TextSize):(InsidePanel(G.LocalToAbsolute(At))||InsidePanel(G.LocalToAbsolute(At+TextSize)));for(int I=0;I<Occupied.Num();++I)if(CoISelection::Overlaps(At,TextSize,Occupied[I],OccupiedSizes[I])){Blocked=true;break;}if(Blocked)continue;
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
  FString Label=R.Name+(View==ECoIMapView::Ownership?TEXT(" | nicht zugewiesen"):(R.bTest?TEXT(" | TEST"):TEXT("")));FVector2D TextSize(Label.Len()*7+12,22);At-=TextSize/2;
  if(At.X<8||At.Y<8||At.X+TextSize.X>Size.X-8||At.Y+TextSize.Y>Size.Y-8)continue;
  bool Blocked=PlayerHUD?PlayerHUD->OverlayCovered(At,TextSize):(InsidePanel(G.LocalToAbsolute(At))||InsidePanel(G.LocalToAbsolute(At+TextSize)));for(int I=0;I<Occupied.Num();++I)if(CoISelection::Overlaps(At,TextSize,Occupied[I],OccupiedSizes[I])){Blocked=true;break;}if(Blocked)continue;
  Occupied.Add(At);OccupiedSizes.Add(TextSize);
  FSlateDrawElement::MakeBox(Out,Top+2,G.ToPaintGeometry(FVector2f(TextSize),FSlateLayoutTransform(FVector2f(At))),FCoreStyle::Get().GetBrush("WhiteBrush"),ESlateDrawEffect::None,FLinearColor(.015f,.025f,.035f,.95f));
  FSlateDrawElement::MakeText(Out,Top+3,G.ToPaintGeometry(FVector2f(TextSize),FSlateLayoutTransform(FVector2f(At+FVector2D(5,1)))),Label,FCoreStyle::GetDefaultFontStyle("Regular",11),ESlateDrawEffect::None,FLinearColor(.75f,.87f,.77f));
 }

 if((MetricLayer==TEXT("supply")||MetricLayer==TEXT("price"))&&Simulation&&Simulation->IsReady()){const auto& S=Simulation->GetSnapshot();for(const auto& R:Data.Regions){double Largest=0;FVector2D Anchor=FVector2D::ZeroVector;for(int32 I=0;I<R.Triangles.Num();I+=3){auto A=R.Vertices[R.Triangles[I]],B=R.Vertices[R.Triangles[I+1]],C=R.Vertices[R.Triangles[I+2]];double Area=FMath::Abs((B.X-A.X)*(C.Y-A.Y)-(B.Y-A.Y)*(C.X-A.X));if(Area>Largest){Largest=Area;Anchor=(A+B+C)/3;}}FVector2D P;if(!Project(Anchor,P)||(PlayerHUD?PlayerHUD->OverlayCovered(P):InsidePanel(G.LocalToAbsolute(P))))continue;auto V=CoISim::MetricValue(S.MapDay,R.Id,MetricGood,MetricKind);FString Label=V.Availability==CoISim::EMetricAvailability::NoNeed?TEXT("N · Kein Bedarf"):V.Availability==CoISim::EMetricAvailability::Missing?(S.MapDay.Complete?TEXT("– · Kein Umsatz"):TEXT("– · Keine Tagesdaten")):MetricKind==CoISim::EMapMetric::Price?FString::Printf(TEXT("%.2f USD/kg"),V.Value/100.):FString::Printf(TEXT("%.0f %%"),V.Value);FVector2D Box(Label.Len()*7+12,22);P-=Box/2;if(PlayerHUD&&PlayerHUD->OverlayCovered(P,Box))continue;if(P.X<8||P.Y<8||P.X+Box.X>G.GetLocalSize().X-8||P.Y+Box.Y>G.GetLocalSize().Y-8)continue;FSlateDrawElement::MakeBox(Out,Top+4,G.ToPaintGeometry(FVector2f(Box),FSlateLayoutTransform(FVector2f(P))),FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),ESlateDrawEffect::None,FLinearColor(.02f,.03f,.04f,.94f));FSlateDrawElement::MakeText(Out,Top+5,G.ToPaintGeometry(FVector2f(Box),FSlateLayoutTransform(FVector2f(P+FVector2D(5,1)))),Label,FCoreStyle::GetDefaultFontStyle("Bold",11),ESlateDrawEffect::None,FLinearColor(.95f,.91f,.8f));}}
 return FMath::Max(Top+5,RouteTop);
}

void UCoIMapInterface::ToggleSimulation(){if(Simulation)Simulation->TogglePause();}
void UCoIMapInterface::SpeedOne(){if(Simulation)Simulation->SetSpeed(1);}
void UCoIMapInterface::SpeedFour(){if(Simulation)Simulation->SetSpeed(4);}
void UCoIMapInterface::SpeedSixteen(){if(Simulation)Simulation->SetSpeed(16);}
void UCoIMapInterface::RefreshSimulation()
{
 if(Simulation&&Simulation->IsReady()&&PresentationGeneration!=Simulation->GetSnapshot().Generation){PresentationGeneration=Simulation->GetSnapshot().Generation;Selection.Id.Empty();RoutePickCycle=0;LastRoutePointer=FVector2D::ZeroVector;RefreshInformation();}
 if(PlayerHUD){ApplyMetricLayer();return;}
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

void UCoIMapInterface::SetMetricLayer(const FString& L,const FString& G,CoISim::EMapMetric K){if(MetricLayer==L&&MetricGood==G&&MetricKind==K)return;MetricLayer=L;MetricGood=G;MetricKind=K;MetricStep=-1;View=L==TEXT("countries")?ECoIMapView::Countries:L==TEXT("ownership")?ECoIMapView::Ownership:ECoIMapView::Regions;ApplyView();ApplyMetricLayer();}
void UCoIMapInterface::ApplyMetricLayer(){if(!Simulation||!Simulation->IsReady())return;const auto& S=Simulation->GetSnapshot();bool Economic=MetricLayer==TEXT("supply")||MetricLayer==TEXT("price");int64 Source=Economic?S.MapDay.CompletedStep:S.Step;if(MetricStep==Source&&MetricGeneration==S.Generation)return;MetricStep=Source;MetricGeneration=S.Generation;const auto* Good=CoISim::Find(S.Player.Catalog.Goods,MetricGood);if(!Good)return;
 auto* Material=Economic||MetricLayer==TEXT("ownership")?LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Materials/M_EconomicLayer.M_EconomicLayer")):nullptr;
 for(TActorIterator<AStaticMeshActor> It(GetWorld());It;++It){auto* Mesh=It->GetStaticMeshComponent();if(!Mesh->GetStaticMesh()||Mesh->GetStaticMesh()->GetName().StartsWith(TEXT("Border_")))continue;for(const auto& R:Data.Regions)if(It->ActorHasTag(FName(*R.Id))){if(!BaseMaterials.Contains(It->GetFName()))BaseMaterials.Add(It->GetFName(),Mesh->GetMaterial(0));if(!Material){Mesh->SetMaterial(0,BaseMaterials[It->GetFName()]);continue;}auto* Dynamic=MetricMaterials.FindRef(It->GetFName()).Get();if(!Dynamic){Dynamic=UMaterialInstanceDynamic::Create(Material,this);MetricMaterials.Add(It->GetFName(),Dynamic);}Mesh->SetMaterial(0,Dynamic);FLinearColor Color;
  if(Economic)Color=CoISim::MetricColor(CoISim::MetricValue(S.MapDay,R.Id,MetricGood,MetricKind),MetricKind,*Good);
  else {bool Owned=false;for(const auto& W:S.Player.Stores)if(W.Own){const auto* Place=CoISim::Find(S.Player.Catalog.Places,W.CityId);Owned|=Place&&Place->RegionId==R.Id;}for(const auto& F:S.Player.Facilities){const auto* Place=CoISim::Find(S.Player.Catalog.Places,F.CityId);Owned|=Place&&Place->RegionId==R.Id;}Color=Owned?FLinearColor(.65f,.45f,.16f):FLinearColor(.22f,.27f,.28f);}
  Dynamic->SetVectorParameterValue(TEXT("RegionColor"),Color);
 }}
}

void UCoIMapInterface::SetRouteDisplay(const FString& Filter,bool Animate){RouteFilter=Filter;RouteAnimate=Animate;}
FString UCoIMapInterface::RouteDiagnostics() const{return FString::Printf(TEXT("Routen %d · Lkw %d · Routenzeichnung %.0f µs · Frame %.2f ms"),DrawnRoutes,DrawnTrucks,RoutePaintMicros,FApp::GetDeltaTime()*1000.);}
TArray<UCoIMapInterface::FScreenRoute> UCoIMapInterface::ScreenRoutes() const {
 TArray<FScreenRoute> Out;if(!Simulation||!Simulation->IsReady()||RouteFilter==TEXT("off"))return Out;
 auto* Pawn=Cast<AMapCameraPawn>(GetOwningPlayer()->GetPawn());const auto& P=Simulation->GetSnapshot().Player;bool Detail=Pawn&&Pawn->GetZoomHeight()<P.Rules.MapRouteDetailZoom;TSet<FString> Bundles;
 for(const auto& R:P.Routes){if(RouteFilter==TEXT("own")&&!R.Own)continue;
  const auto* A=Data.Cities.FindByPredicate([&](const FCoIGeoCity& C){return C.Id==R.FromCityId;});const auto* B=Data.Cities.FindByPredicate([&](const FCoIGeoCity& C){return C.Id==R.ToCityId;});FVector2D From,To;if(!A||!B||!Project(A->Position,From)||!Project(B->Position,To))continue;
  const auto Size=GetCachedGeometry().GetLocalSize();if(FMath::Max(From.X,To.X)<0||FMath::Min(From.X,To.X)>Size.X||FMath::Max(From.Y,To.Y)<0||FMath::Min(From.Y,To.Y)>Size.Y)continue;
  // At overview scale draw one representative per undirected pair and ownership class.
  FString Key=(R.FromCityId<R.ToCityId?R.FromCityId+R.ToCityId:R.ToCityId+R.FromCityId)+(R.Own?TEXT("own"):TEXT("public"));if(!Detail&&Bundles.Contains(Key))continue;Bundles.Add(Key);
  FScreenRoute X;X.Id=R.Id;X.Own=R.Own;for(int32 I=0;I<=16;++I)X.Points.Add(CoIRoutes::Point(From,To,I/16.,R.Own,Detail));Out.Add(X);
 }return Out;
}
int32 UCoIMapInterface::PaintRoutes(const FGeometry& G,FSlateWindowElementList& Out,int32 Layer) const {
 double Start=FPlatformTime::Seconds();DrawnTrucks=DrawnRoutes=0;if(!Simulation||!Simulation->IsReady())return Layer;
 const auto& P=Simulation->GetSnapshot().Player;auto* Pawn=Cast<AMapCameraPawn>(GetOwningPlayer()->GetPawn());double Zoom=Pawn?Pawn->GetZoomHeight():22000;
 auto Clear=[&](FVector2D At,FVector2D Size){return At.X>=0&&At.Y>=0&&At.X+Size.X<G.GetLocalSize().X&&At.Y+Size.Y<G.GetLocalSize().Y&&(!PlayerHUD||!PlayerHUD->OverlayCovered(At,Size));};
 auto Line=[&](FVector2D A,FVector2D B,FLinearColor C,float W){if(Clear(A,FVector2D::ZeroVector)&&Clear(B,FVector2D::ZeroVector))FSlateDrawElement::MakeLines(Out,Layer,G.ToPaintGeometry(),{A,B},ESlateDrawEffect::None,C,true,W);};
 for(const auto& R:ScreenRoutes()){const auto* DataRow=CoISim::Find(P.Routes,R.Id);if(!DataRow)continue;++DrawnRoutes;FLinearColor Color=R.Id==Selection.Id?FLinearColor(1,.86f,.3f):R.Own?FLinearColor(.9f,.62f,.18f):FLinearColor(.55f,.72f,.8f);
  for(int32 I=1;I<R.Points.Num();++I)Line(R.Points[I-1],R.Points[I],Color,R.Own?3:1.5f);
  auto Center=R.Points[9],D=(R.Points[10]-R.Points[8]).GetSafeNormal(),N=FVector2D(-D.Y,D.X);Line(Center,Center-D*9+N*5,Color,2);Line(Center,Center-D*9-N*5,Color,2);
  int32 Count=CoIRoutes::TruckCount(*DataRow,P.Rules,Zoom,(int32)P.Rules.MapTrucksTotal-DrawnTrucks,RouteAnimate);
  for(int32 I=0;I<Count;++I){double Phase=CoIRoutes::ExamplePhase(*DataRow,I,P),Index=Phase*16;int32 Seg=FMath::Min(15,(int32)Index);FVector2D At=FMath::Lerp(R.Points[Seg],R.Points[Seg+1],Index-Seg)-FVector2D(8,5);if(!Clear(At,{18,13}))continue;++DrawnTrucks;
   auto Box=[&](FVector2D Pos,FVector2D Size,FLinearColor C){FSlateDrawElement::MakeBox(Out,Layer+1,G.ToPaintGeometry(FVector2f(Size),FSlateLayoutTransform(FVector2f(Pos))),FCoreStyle::Get().GetBrush(TEXT("WhiteBrush")),ESlateDrawEffect::None,C);};
   Box(At,{11,8},Color);Box(At+FVector2D(12,2),{5,6},Color);Box(At+FVector2D(2,8),{4,3},FLinearColor::Black);Box(At+FVector2D(12,8),{4,3},FLinearColor::Black);
  }
 }RoutePaintMicros=(FPlatformTime::Seconds()-Start)*1.e6;return Layer+2;
}

void UCoIMapInterface::FocusObject(const FString& CityId,const FString& ObjectId){if(const auto* C=Data.Cities.FindByPredicate([&](const FCoIGeoCity& X){return X.Id==CityId;})){if(auto* Pawn=Cast<AMapCameraPawn>(GetOwningPlayer()->GetPawn()))Pawn->FocusCity(C->Position);Selection.Id=ObjectId;RefreshInformation();}}
