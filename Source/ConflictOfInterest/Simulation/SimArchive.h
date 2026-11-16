#pragma once
#include "SimModel.h"
#include "Dom/JsonObject.h"
#include "Misc/SecureHash.h"
namespace CoISim
{
// Strict required-field reader. Integer input is bounded to exact JSON integer range and safe scenario magnitudes.
struct FRead
{
 TSharedPtr<FJsonObject> Object; FString& Error;
 template<class T> void Optional(const TCHAR* Key,T& Value){if(Object->HasField(Key))(*this)(Key,Value);}
 void Fail(const FString& Key){if(Error.IsEmpty())Error=TEXT("Invalid/missing field: ")+Key;}
 template<class T> void operator()(const TCHAR* Key,T& Value)
 {
  auto V=Object->TryGetField(Key);if(!V.IsValid()){Fail(Key);return;}
  Read(V,Value,Key);
 }
 void Read(const TSharedPtr<FJsonValue>& V,FString& X,const FString& Key){if(!V->TryGetString(X)||X.Len()>512)Fail(Key);}
 void Read(const TSharedPtr<FJsonValue>& V,bool& X,const FString& Key){if(!V->TryGetBool(X))Fail(Key);}
 void Read(const TSharedPtr<FJsonValue>& V,double& X,const FString& Key){if(!V->TryGetNumber(X)||!FMath::IsFinite(X))Fail(Key);}
 void Read(const TSharedPtr<FJsonValue>& V,int64& X,const FString& Key)
 {double N=0;if(!V->TryGetNumber(N)||!FMath::IsFinite(N)||FMath::Abs(N)>1000000000000.||FMath::FloorToDouble(N)!=N){Fail(Key);return;}X=(int64)N;}
 void Read(const TSharedPtr<FJsonValue>& V,uint32& X,const FString& Key){int64 N=0;Read(V,N,Key);if(N<0||N>MAX_uint32)Fail(Key);else X=(uint32)N;}
 template<class T> void Read(const TSharedPtr<FJsonValue>& V,TArray<T>& X,const FString& Key)
 {const TArray<TSharedPtr<FJsonValue>>* A=nullptr;if(!V->TryGetArray(A)||A->Num()>10000){Fail(Key);return;}X.Reset();for(const auto& E:*A){T Item;Read(E,Item,Key);X.Add(MoveTemp(Item));}}
 template<class T> void Read(const TSharedPtr<FJsonValue>& V,T& X,const FString& Key)
 {const TSharedPtr<FJsonObject>* O=nullptr;if(!V->TryGetObject(O)){Fail(Key);return;}FRead Nested{*O,Error};X.Fields(Nested);}
};
// Field-order canonical fingerprint: never iterate a TMap, never hash struct padding.
struct FFingerprint
{
 FSHA1 Hash;
 template<class T> void Optional(const TCHAR*,const T& X){Write(X);}
 void Bytes(const void* Data,uint32 Num){Hash.Update((const uint8*)Data,Num);}
 template<class T> void operator()(const TCHAR*,const T& X){Write(X);}
 void Write(const FString& S){FTCHARToUTF8 Text(*S);int64 Length=Text.Length();Write(Length);Bytes(Text.Get(),Text.Length());}
 void Write(int64 X){uint64 V=(uint64)X;for(int I=0;I<8;++I){uint8 B=(V>>(I*8))&255;Bytes(&B,1);}}
 void Write(uint32 X){Write((int64)X);} void Write(bool X){Write((int64)X);}
 void Write(double X){uint64 Bits=0;FMemory::Memcpy(&Bits,&X,8);Write((int64)Bits);}
 template<class T> void Write(const TArray<T>& A){Write((int64)A.Num());for(const auto& X:A)Write(X);}
 template<class T> void Write(const T& X){const_cast<T&>(X).Fields(*this);}
 FString Finish(){Hash.Final();uint8 Digest[20];Hash.GetHash(Digest);return BytesToHex(Digest,20);}
};
template<class T> const T* Find(const TArray<T>& Rows,const FString& Id){return Rows.FindByPredicate([&](const T& Row){return Row.Id==Id;});}
template<class T> T* Find(TArray<T>& Rows,const FString& Id){return Rows.FindByPredicate([&](const T& Row){return Row.Id==Id;});}
}
