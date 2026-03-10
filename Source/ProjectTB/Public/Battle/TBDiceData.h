#pragma once

#include "CoreMinimal.h"
#include "TBDiceData.generated.h"

/**
 * 주사위 아이템 정의.
 * BaseFaces에서 랜덤으로 면을 뽑고, FaceBonus를 더해 최종 배율로 변환.
 * 변환 공식: Multiplier = 1.0 + FinalFace * 0.1
 *   예) -3→×0.7 / 0→×1.0 / +3→×1.3
 */
USTRUCT(BlueprintType)
struct FDiceData
{
	GENERATED_BODY()

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	FText DiceName = FText::FromString(TEXT("기본 주사위"));

	// 주사위 면 값 목록 (기본: -3 ~ +3, 7면)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	TArray<int32> BaseFaces = {-3, -2, -1, 0, 1, 2, 3};

	// BaseFaces를 문자열로 반환 (예: "[-3, -2, -1, 0, 1, 2, 3]")
	FString GetFacesAsString() const
	{
		if (BaseFaces.Num() == 0) return TEXT("[]");

		FString Result = TEXT("[");
		for (int32 i = 0; i < BaseFaces.Num(); ++i)
		{
			if (i > 0) Result += TEXT(", ");
			const int32 Face = BaseFaces[i];
			Result += (Face == 0)
				? TEXT("0")
				: FString::Printf(TEXT("%+d"), Face);  // 0은 부호 생략
		}
		Result += TEXT("]");
		return Result;
	}
};

/**
 * 유물/버프로 쌓이는 주사위 보정값 (파티 전체 적용).
 * GameInstance에 저장. 유물마다 FaceBonus를 누적.
 *
 * 롤 계산:
 *   FinalFace = Clamp(RolledFace + FaceBonus, MinFace, MaxFace)
 *   Multiplier = 1.0 + FinalFace * 0.1
 */
USTRUCT(BlueprintType)
struct FDiceModifier
{
	GENERATED_BODY()

	// 모든 면에 더하는 값 (유물 "주사위 숫자 +1" → FaceBonus=1)
	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 FaceBonus = 0;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 MinFace = -10;

	UPROPERTY(EditDefaultsOnly, BlueprintReadWrite)
	int32 MaxFace = 10;
};
