#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameFramework/Actor.h"
#include "GeoCatalog.generated.h"
UCLASS(BlueprintType)
class CONFLICTOFINTEREST_API UCoIGeoCatalog : public UDataAsset
{
 GENERATED_BODY()
public:
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Geography") FString DefinitionJson;
 UPROPERTY(VisibleAnywhere,BlueprintReadOnly,Category="Geography") FString ProvenanceJson;
};
// A single explicit level reference keeps all static definitions reachable by cooking.
UCLASS()
class CONFLICTOFINTEREST_API ACoIGeoReference : public AActor
{GENERATED_BODY() public: UPROPERTY(VisibleAnywhere,Category="Geography") TObjectPtr<UCoIGeoCatalog> Catalog;};
