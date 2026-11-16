#include "CoIBuildMapCommandlet.h"
#include "CoIGameMode.h"
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
int32 UCoIBuildMapCommandlet::Main(const FString& Params)
{
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
