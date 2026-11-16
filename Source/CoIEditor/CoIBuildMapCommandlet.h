#pragma once
#include "CoreMinimal.h"
#include "Commandlets/Commandlet.h"
#include "CoIBuildMapCommandlet.generated.h"
UCLASS()
class UCoIBuildMapCommandlet : public UCommandlet
{ GENERATED_BODY() public: UCoIBuildMapCommandlet();virtual int32 Main(const FString& Params) override; };
