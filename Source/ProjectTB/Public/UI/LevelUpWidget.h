#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LevelDataTypes.h"
#include "LevelUpWidget.generated.h"

class ATBBattleHUD;

USTRUCT(BlueprintType)
struct FPartyCurrentStatusData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	FName CharacterId = NAME_None;

	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	int32 Level = 1;

	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	float CurrentHP = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	float CurrentMP = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	float CurrentStamina = 0.f;

	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	int32 DiceFaceBonus = 0;

	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	int32 DiceMinFace = -10;

	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	int32 DiceMaxFace = 10;
};

/**
 * 레벨업 화면 위젯.
 * 3캐릭터 스탯 증가량을 한번에 표시.
 * 1초 후 입력 활성화, 엔터 시 월드 복귀.
 */

/**
 * 레벨업 화면 위젯 대신 원정대 상태창 UI로 변경됨
 * 이름이나 구조를 의도에 맞게 수정하지는 못함
 */

UCLASS(Blueprintable)
class PROJECTTB_API ULevelUpWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ─── 초기화 ───────────────────────────────────────────────────────────────

	/** 위젯 데이터 설정 및 표시 */
	UFUNCTION(BlueprintCallable, Category="LevelUp")
	void Initialize(const TArray<FLevelUpDisplayData>& InLevelUpData);

	/** 현재 게임 상태를 읽어 파티 상태창 데이터를 다시 계산한다. */
	UFUNCTION(BlueprintCallable, Category="PartyStatus")
	void RefreshFromGameState();

	// ─── 데이터 (Blueprint에서 바인딩) ─────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	TArray<FLevelUpDisplayData> LevelUpData;

	// WBP에서 기존 스탯 표시 노드에 그대로 연결할 최종 수치
	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	TArray<FCharacterLevelStats> FinalPartyStats;

	// 현재/최대가 분리되는 값과 주사위 보정용 데이터
	UPROPERTY(BlueprintReadOnly, Category="PartyStatus")
	TArray<FPartyCurrentStatusData> CurrentPartyStatus;

	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	bool bInputEnabled = false;

	// ─── HUD 참조 ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadWrite, Category="LevelUp")
	TObjectPtr<ATBBattleHUD> OwningHUD;

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 1초 후 입력 활성화 */
	UFUNCTION()
	void EnableInput();

	/** 확인 입력 처리 */
	UFUNCTION(BlueprintNativeEvent, Category="LevelUp")
	void OnConfirm();
	void OnConfirm_Implementation();

	// WBP에서 RefreshStats를 이 이벤트에 연결해 사용
	UFUNCTION(BlueprintImplementableEvent, Category="PartyStatus")
	void OnPartyStatusUpdated();

	// 현재 보유 재화량
	UPROPERTY(BlueprintReadOnly, Category="Money")
	int32 CurrentMoney = 0;
	
private:
	FTimerHandle InputEnableTimerHandle;
};
