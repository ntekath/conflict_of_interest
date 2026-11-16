#include "CoIBuildMapCommandlet.h"
#include "CoIGameMode.h"
#include "GeoCatalog.h"
#include "GeoMapData.h"
#include "Engine/StaticMesh.h"
#include "MeshDescription.h"
#include "StaticMeshAttributes.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/World.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/WorldSettings.h"
#include "GameFramework/PlayerStart.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "UObject/SavePackage.h"
#include "Misc/PackageName.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "FileHelpers.h"
UCoIBuildMapCommandlet::UCoIBuildMapCommandlet(){IsClient=false;IsServer=false;IsEditor=true;LogToConsole=true;}
static void SaveAsset(UObject* Object)
{
 auto* P=Object->GetOutermost();P->MarkPackageDirty();FSavePackageArgs Args;Args.TopLevelFlags=RF_Public|RF_Standalone;
 FString Path=FPackageName::LongPackageNameToFilename(P->GetName(),FPackageName::GetAssetPackageExtension());
 if(!UPackage::SavePackage(P,Object,*Path,Args))UE_LOG(LogTemp,Fatal,TEXT("Cannot save %s"),*Path);
 FAssetRegistryModule::AssetCreated(Object);
}
static UMaterial* FlatMaterial(const FString& Name,FLinearColor Color)
{
 FString Path=TEXT("/Game/Materials/")+Name;
 if(auto* Existing=LoadObject<UMaterial>(nullptr,*(Path+TEXT(".")+Name)))return Existing;
 auto* P=CreatePackage(*Path);P->FullyLoad();auto* M=NewObject<UMaterial>(P,*Name,RF_Public|RF_Standalone);
 M->SetShadingModel(MSM_Unlit);M->TwoSided=true;auto* C=NewObject<UMaterialExpressionConstant3Vector>(M);C->Constant=Color;
 M->GetExpressionCollection().AddExpression(C);M->GetEditorOnlyData()->EmissiveColor.Expression=C;
 M->PostEditChange();SaveAsset(M);return M;
}
static void Plane(UWorld* World,FVector Position,FVector Scale,UMaterial* Material)
{
 auto* Actor=World->SpawnActor<AStaticMeshActor>(Position,FRotator::ZeroRotator);
 Actor->GetStaticMeshComponent()->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Plane.Plane")));
 Actor->GetStaticMeshComponent()->SetMaterial(0,Material);Actor->SetActorScale3D(Scale);
 Actor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}

static void GeoMesh(UWorld* W,const FString& Name,const FString& ID,const TArray<FVector3f>& Vertices,const TArray<int32>& Indices,UMaterial* Material)
{
 FMeshDescription D;FStaticMeshAttributes A(D);A.Register();auto P=A.GetVertexPositions();TArray<FVertexID> IDs;
 auto N=A.GetVertexInstanceNormals();auto T=A.GetVertexInstanceTangents();auto B=A.GetVertexInstanceBinormalSigns();auto UV=A.GetVertexInstanceUVs();UV.SetNumChannels(1);
 for(auto V:Vertices){auto Vertex=D.CreateVertex();P[Vertex]=V;IDs.Add(Vertex);}auto Group=D.CreatePolygonGroup();A.GetPolygonGroupMaterialSlotNames()[Group]=TEXT("Map");
 for(int32 I=0;I<Indices.Num();I+=3)
 {TArray<FVertexInstanceID> Corners;for(int32 J=0;J<3;++J){auto V=D.CreateVertexInstance(IDs[Indices[I+J]]);N[V]={0,0,1};T[V]={1,0,0};B[V]=1;UV.Set(V,0,FVector2f(0,0));Corners.Add(V);}D.CreatePolygon(Group,Corners);}
 FString Path=TEXT("/Game/Geography/Meshes/")+Name;auto* Package=CreatePackage(*Path);Package->FullyLoad();
 auto* Mesh=LoadObject<UStaticMesh>(nullptr,*(Path+TEXT(".")+Name));if(!Mesh)Mesh=NewObject<UStaticMesh>(Package,*Name,RF_Public|RF_Standalone);
 Mesh->SetNumSourceModels(0);Mesh->GetStaticMaterials().Reset();Mesh->GetStaticMaterials().Add(FStaticMaterial(Material,TEXT("Map")));
 Mesh->AddSourceModel();Mesh->CreateMeshDescription(0,MoveTemp(D));Mesh->CommitMeshDescription(0);Mesh->Build(false);SaveAsset(Mesh);
 auto* Actor=W->SpawnActor<AStaticMeshActor>();Actor->SetActorLabel(Name);Actor->Tags.Add(FName(*ID));
 Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);Actor->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
}
static void Surface(UWorld* W,const FCoIGeoSurface& S,double Z,UMaterial* Material)
{
 TArray<FVector3f> V;for(auto P:S.Vertices)V.Add(FVector3f(P.X,P.Y,Z));
 GeoMesh(W,S.Id.Replace(TEXT("."),TEXT("_")),S.Id,V,S.Triangles,Material);
}
static void Boundaries(UWorld* W,const FCoIGeoSurface& S,UMaterial* Material)
{
 TArray<FVector3f> V;TArray<int32> I;
 for(const auto& Ring:S.Rings)for(int32 K=1;K<Ring.Num();++K)
 {
  FVector3f P(Ring[K-1].X,Ring[K-1].Y,2),Q(Ring[K].X,Ring[K].Y,2);
  auto Side=FVector3f::CrossProduct(Q-P,FVector3f(0,0,1)).GetSafeNormal()*2.f;int32 N=V.Num();V.Append({P+Side,P-Side,Q-Side,Q+Side});I.Append({N,N+1,N+2,N,N+2,N+3});
 }
 GeoMesh(W,TEXT("Border_")+S.Id.Replace(TEXT("."),TEXT("_")),S.Id,V,I,Material);
}
static int32 BuildUSMap()
{
 FString Json,Provenance,Error;FCoIGeoMapData Data;
 if(!FFileHelper::LoadFileToString(Json,*(FPaths::ProjectDir()/TEXT("GeoData/Generated/map.json")))||!Data.Load(Json,Error))
 {UE_LOG(LogTemp,Error,TEXT("Offline US import required: %s"),*Error);return 3;}
 if(!FFileHelper::LoadFileToString(Provenance,*(FPaths::ProjectDir()/TEXT("GeoData/Generated/validation.json"))))return 4;
 auto* W=UEditorLoadingAndSavingUtils::NewBlankMap(false);if(!W)return 1;
 W->GetWorldSettings()->DefaultGameMode=ACoIGameMode::StaticClass();W->SpawnActor<APlayerStart>(FVector::ZeroVector,FRotator::ZeroRotator);
 auto* Water=FlatMaterial(TEXT("M_GreatLakes"),FLinearColor(.035f,.15f,.25f));
 auto* Base=FlatMaterial(TEXT("M_Land"),FLinearColor(.16f,.26f,.18f));
 auto* Context=FlatMaterial(TEXT("M_CanadaContext"),FLinearColor(.24f,.29f,.27f));
 auto* Border=FlatMaterial(TEXT("M_RegionBoundary"),FLinearColor(.055f,.085f,.07f));
 auto* City=FlatMaterial(TEXT("M_City"),FLinearColor(1,.55f,.10f));
 Plane(W,{Data.Center.X,Data.Center.Y,-5},{300,300,1},FlatMaterial(TEXT("M_Outside"),FLinearColor(.018f,.035f,.05f)));
 for(const auto& S:Data.Countries)Surface(W,S,0,S.Id==TEXT("country.usa")?Base:Context);
 const FLinearColor Colors[]={FLinearColor(.16f,.29f,.20f),FLinearColor(.20f,.32f,.22f),FLinearColor(.25f,.34f,.22f),FLinearColor(.28f,.35f,.25f)};
 for(int32 K=0;K<Data.Regions.Num();++K)
 {const auto& S=Data.Regions[K];Surface(W,S,.5,FlatMaterial(TEXT("M_")+S.Id.Replace(TEXT("."),TEXT("_")),Colors[K%4]));Boundaries(W,S,Border);}
 for(const auto& S:Data.Lakes)Surface(W,S,1,Water);
 TArray<FVector3f> Vertices;TArray<int32> Indices;
 for(const auto& C:Data.Cities)
 {
  int32 Start=Vertices.Num();Vertices.Add({(float)C.Position.X,(float)C.Position.Y,4});
  for(int32 K=0;K<16;++K){double Angle=2*PI*K/16;Vertices.Add({(float)(C.Position.X+45*FMath::Cos(Angle)),(float)(C.Position.Y+45*FMath::Sin(Angle)),4});}
  for(int32 K=0;K<16;++K)Indices.Append({Start,Start+1+K,Start+1+(K+1)%16});
 }
 GeoMesh(W,TEXT("CityMarkers"),TEXT("display.city_markers"),Vertices,Indices,City);
 const FString Path=TEXT("/Game/Geography/Data/GeoCatalog");auto* Package=CreatePackage(*Path);Package->FullyLoad();
 auto* Catalog=LoadObject<UCoIGeoCatalog>(nullptr,*(Path+TEXT(".GeoCatalog")));
 if(!Catalog)Catalog=NewObject<UCoIGeoCatalog>(Package,TEXT("GeoCatalog"),RF_Public|RF_Standalone);
 Catalog->DefinitionJson=Json;Catalog->ProvenanceJson=Provenance;SaveAsset(Catalog);W->SpawnActor<ACoIGeoReference>()->Catalog=Catalog;
 if(!UEditorLoadingAndSavingUtils::SaveMap(W,TEXT("/Game/Maps/GreatLakes1913")))return 2;
 UE_LOG(LogTemp,Display,TEXT("COI_MAP_OK GreatLakes1913 countries=%d lakes=%d regions=%d cities=%d"),Data.Countries.Num(),Data.Lakes.Num(),Data.Regions.Num(),Data.Cities.Num());return 0;
}
int32 UCoIBuildMapCommandlet::Main(const FString& Params)
{
 if(!Params.Contains(TEXT("Prototype")))return BuildUSMap();
 auto* W=UEditorLoadingAndSavingUtils::NewBlankMap(false);if(!W)return 1;
 W->GetWorldSettings()->DefaultGameMode=ACoIGameMode::StaticClass();W->SpawnActor<APlayerStart>(FVector::ZeroVector,FRotator::ZeroRotator);
 auto* Water=FlatMaterial(TEXT("M_Background"),FLinearColor(.025f,.06f,.10f));
 auto* Ground=FlatMaterial(TEXT("M_Placeholder"),FLinearColor(.12f,.24f,.19f));
 auto* Grid=FlatMaterial(TEXT("M_Grid"),FLinearColor(.3f,.42f,.36f));
 Plane(W,{0,0,-5},{250,250,1},Water);Plane(W,{0,0,0},{90,90,1},Ground);
 for(int32 I=-4;I<=4;++I){Plane(W,{I*1000.,0,1},{.025,90,1},Grid);Plane(W,{0,I*1000.,1},{90,.025,1},Grid);}
 if(!UEditorLoadingAndSavingUtils::SaveMap(W,TEXT("/Game/Maps/CameraPrototype")))return 2;
 UE_LOG(LogTemp,Display,TEXT("COI_MAP_OK CameraPrototype"));return 0;
}
