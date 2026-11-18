#include "CityHubs.h"
#include "SimArchive.h"
namespace CoISim {
bool MigrateCityHubs(FScenario& S,FString& E){
 if(S.HubSchema==1)return true;
 if(S.HubSchema!=0||!S.Definitions.ResourceSites.IsEmpty()||!S.InitialState.ResourceSites.IsEmpty()){E=TEXT("Unsupported or mixed hub schema");return false;}
 auto& D=S.Definitions;auto& W=S.InitialState;
 for(auto& C:D.Companies){C.HeadquartersCityId=C.StartCityId;if(C.HeadquartersCityId.IsEmpty()&&!C.AccountingOnly){for(const auto& X:W.Warehouses)if(X.OwnerId==C.Id){C.HeadquartersCityId=X.CityId;break;}if(C.HeadquartersCityId.IsEmpty())for(const auto& X:W.Vehicles)if(X.OwnerId==C.Id){C.HeadquartersCityId=X.CityId;break;}}}
 for(auto& V:W.Vehicles)V.HomeCityId=V.CityId;
 for(const auto& R:D.Regions){TArray<FString> Cities;for(const auto& P:D.Places)if(P.RegionId==R.Id)Cities.Add(P.Id);Cities.Sort();if(Cities.IsEmpty())continue;
 for(int32 I=0;I<Cities.Num();++I)for(const FString Kind:{FString(TEXT("farmland")),FString(TEXT("forest")),FString(TEXT("ore"))}){
 int64 Total=Kind==TEXT("forest")?R.ForestKg:Kind==TEXT("ore")?R.OreKg:0;
 int64 Kg=Total/Cities.Num()+(I<Total%Cities.Num()?1:0);if(Kind==TEXT("farmland")?R.FarmlandPermille<=0:Kg<=0)continue;
 FResourceSiteDef Site;Site.Id=TEXT("site|")+Cities[I]+TEXT("|")+Kind;Site.CityId=Cities[I];Site.Kind=Kind;Site.InitialKg=Kg;Site.SuitabilityPermille=Kind==TEXT("farmland")?R.FarmlandPermille:1000;
 int64 Claims=0;for(auto& F:W.Facilities){const auto* T=Find(D.FacilityTypes,F.TypeId);if(T&&F.CityId==Site.CityId&&T->ResourceRequirement==Kind){F.ResourceSiteId=Site.Id;F.ResourceBatchesPerHour=T->CapacityBatchesPerHour;Claims+=F.ResourceBatchesPerHour;}}
 Site.MaxBatchesPerHour=FMath::Max(1LL,Claims);D.ResourceSites.Add(Site);
 }}D.ResourceSites.Sort([](const FResourceSiteDef& A,const FResourceSiteDef& B){return A.Id<B.Id;});S.HubSchema=1;return true;
}
FString AssetCity(const FState& W,const FString& Id){if(const auto* F=Find(W.Facilities,Id))return F->CityId;if(const auto* S=Find(W.Warehouses,Id))return S->CityId;if(const auto* V=Find(W.Vehicles,Id))return V->CityId;return {};}
bool ValidateCityHubs(const FScenario& S,const FState& W,bool Initial,FString& E){
 const auto& D=S.Definitions;auto Fail=[&](const FString& Id){E=TEXT("Invalid hub/resource assignment: ")+Id;return false;};if(S.HubSchema!=1)return Fail(TEXT("HubSchema"));
 for(const auto& C:D.Companies)if((!C.AccountingOnly||!C.HeadquartersCityId.IsEmpty())&&!Find(D.Places,C.HeadquartersCityId))return Fail(C.Id);
 for(const auto& X:W.Warehouses)if(!Find(D.Places,X.CityId)||!Find(D.Companies,X.OwnerId))return Fail(X.Id);
 for(const auto& V:W.Vehicles){if(!Find(D.Places,V.HomeCityId)||!Find(D.Companies,V.OwnerId))return Fail(V.Id);const auto* J=Find(W.Transports,V.TransportId);if(V.CityId.IsEmpty()){if(!J||J->Status!=TEXT("moving")||J->VehicleId!=V.Id)return Fail(V.Id);}else if(!Find(D.Places,V.CityId)||(J&&J->Status==TEXT("moving")))return Fail(V.Id);}
 TSet<FString> Seen;for(const auto& X:D.ResourceSites){if(Seen.Contains(X.Id)||X.Id.IsEmpty()||!Find(D.Places,X.CityId)||(X.Kind!=TEXT("farmland")&&X.Kind!=TEXT("forest")&&X.Kind!=TEXT("ore"))||X.InitialKg<0||X.InitialKg>1000000000||X.SuitabilityPermille<=0||X.SuitabilityPermille>1000||X.MaxBatchesPerHour<1||X.MaxBatchesPerHour>100000||(X.Kind==TEXT("farmland")&&X.InitialKg!=0))return Fail(X.Id);Seen.Add(X.Id);int64 Claims=0;for(const auto& F:W.Facilities)if(F.ResourceSiteId==X.Id)Claims+=F.ResourceBatchesPerHour;if(Claims>X.MaxBatchesPerHour)return Fail(X.Id+TEXT(" overclaimed"));}
 for(const auto& F:W.Facilities){const auto* T=Find(D.FacilityTypes,F.TypeId);const auto* Store=Find(W.Warehouses,F.StorageId);if(!T||!Find(D.Places,F.CityId)||!Store||Store->CityId!=F.CityId||Store->OwnerId!=F.OwnerId)return Fail(F.Id);if(T->ResourceRequirement==TEXT("none")){if(!F.ResourceSiteId.IsEmpty()||F.ResourceBatchesPerHour)return Fail(F.Id);}else {const auto* X=Find(D.ResourceSites,F.ResourceSiteId);if(!X||X->CityId!=F.CityId||X->Kind!=T->ResourceRequirement||F.ResourceBatchesPerHour<1||F.ResourceBatchesPerHour>T->CapacityBatchesPerHour)return Fail(F.Id);}}
 for(const auto& R:D.Regions){int64 Forest=0,Ore=0;for(const auto& X:D.ResourceSites)if(Find(D.Places,X.CityId)->RegionId==R.Id){if(X.Kind==TEXT("forest"))Forest+=X.InitialKg;if(X.Kind==TEXT("ore"))Ore+=X.InitialKg;if(X.Kind==TEXT("farmland")&&X.SuitabilityPermille>R.FarmlandPermille)return Fail(X.Id);}if(Forest!=R.ForestKg||Ore!=R.OreKg)return Fail(R.Id+TEXT(" duplicated/missing resources"));}
 if(Initial){if(!W.ResourceSites.IsEmpty())return Fail(TEXT("initial resource counters must be empty"));return true;}
 if(W.ResourceSites.Num()!=D.ResourceSites.Num())return Fail(TEXT("resource state coverage"));Seen.Reset();for(const auto& X:W.ResourceSites){const auto* T=Find(D.ResourceSites,X.Id);if(!T||Seen.Contains(X.Id)||X.RemainingKg<0||X.ExtractedKg<0||X.RemainingKg+X.ExtractedKg!=T->InitialKg)return Fail(X.Id);Seen.Add(X.Id);}
 for(const auto& R:W.Regions){int64 Forest=0,Ore=0;for(const auto& X:W.ResourceSites){const auto* T=Find(D.ResourceSites,X.Id);if(Find(D.Places,T->CityId)->RegionId==R.Id){if(T->Kind==TEXT("forest"))Forest+=X.RemainingKg;if(T->Kind==TEXT("ore"))Ore+=X.RemainingKg;}}if(Forest!=R.RemainingForestKg||Ore!=R.RemainingOreKg)return Fail(R.Id);}
 return true;
}
}
