#include "MapNavigation.h"
#include "SimArchive.h"
namespace CoISim {
FString MapObjectCity(const FPlayerView& P,const FString& Object){if(Find(P.Catalog.Places,Object))return Object;if(const auto* W=Find(P.Stores,Object))return W->CityId;if(const auto* I=Find(P.Stocks,Object))if(const auto* W=Find(P.Stores,I->StorageId))return W->CityId;if(const auto* J=Find(P.Shipments,Object))return J->Status==TEXT("planned")?J->FromCityId:J->ToCityId;if(const auto* R=Find(P.Routes,Object))return R->ToCityId;for(const auto& C:P.Catalog.Places)if(C.RegionId==Object)return C.Id;return {};}
FMapNavigation MapNavigation(const FPlayerView& P,const FString& Object,const FString& Good){FMapNavigation N;N.CityId=MapObjectCity(P,Object);if(const auto* C=Find(P.Catalog.Places,N.CityId))N.RegionId=C->RegionId;
 const FInventory* Source=nullptr;
 auto Usable=[&](const FInventory& I){return I.QuantityKg>I.ReservedKg&&Find(P.Stores,I.StorageId);};
 if(const auto* I=Find(P.Stocks,Object);I&&Usable(*I))Source=I;
 if(!Source)for(const auto& I:P.Stocks)if(I.GoodId==Good&&Usable(I)){const auto* W=Find(P.Stores,I.StorageId);if(W->Id==Object||W->CityId==N.CityId){Source=&I;break;}}
 if(!Source&&!N.CityId.IsEmpty())for(const auto& I:P.Stocks)if(I.GoodId==Good&&Usable(I)&&Find(P.Stores,I.StorageId)->Own){Source=&I;break;}
 if(Source)N.SourceInventoryId=Source->Id;
 const auto* From=Source?Find(P.Stores,Source->StorageId):nullptr;const FString Cargo=Source?Source->GoodId:Good;
 auto Access=[&](const FStoreView& W){return !W.Blocked&&(W.Own||W.Public||W.AcceptedGoods.Contains(Cargo))&&(!From||W.Id!=From->Id);};
 if(const auto* W=Find(P.Stores,Object);W&&Access(*W))N.TargetStorageId=W->Id;
 if(N.TargetStorageId.IsEmpty()&&(!From||From->CityId!=N.CityId))for(bool Own:{true,false}){for(const auto& W:P.Stores)if(W.CityId==N.CityId&&W.Own==Own&&Access(W)&&(Own||W.Public)){N.TargetStorageId=W.Id;break;}if(!N.TargetStorageId.IsEmpty())break;}
 return N;
}
}
