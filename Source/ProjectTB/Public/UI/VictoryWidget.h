#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LevelDataTypes.h"
#include "VictoryWidget.generated.h"

class ATBBattleHUD;

/**
 * 전투 승리 화면 위젯.
 * 캐릭터별 경험치 표시, 1초 후 입력 활성화.
 * 첫 엔터: 경험치 획득 후 상태 표시
 * 두 번째 엔터: 레벨업 있으면 LevelUpWidget, 없으면 월드 복귀.
 */
UCLASS(Blueprintable)
class PROJECTTB_API UVictoryWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ─── 초기화 ───────────────────────────────────────────────────────────────

	/** 위젯 데이터 설정 및 표시 */
	UFUNCTION(BlueprintCallable, Category="Victory")
	void Initialize(const TArray<FCharacterExpData>& InBeforeExpData,
	                const TArray<FCharacterExpData>& InAfterExpData,
	                const TArray<FLevelUpDisplayData>& InLevelUpData);

	// ─── 데이터 (Blueprint에서 바인딩) ─────────────────────────────────────────

	/** 경험치 획득 전 상태 */
	UPROPERTY(BlueprintReadOnly, Category="Victory")
	TArray<FCharacterExpData> BeforeExpData;

	/** 경험치 획득 후 상태 */
	UPROPERTY(BlueprintReadOnly, Category="Victory")
	TArray<FCharacterExpData> AfterExpData;

	/** 현재 표시 중인 데이터 (Blueprint에서 UI 바인딩용) */
	UPROPERTY(BlueprintReadOnly, Category="Victory")
	TArray<FCharacterExpData> CurrentExpData;

	/** 레벨업 정보 */
	UPROPERTY(BlueprintReadOnly, Category="Victory")
	TArray<FLevelUpDisplayData> LevelUpData;

	/** 입력 활성화 여부 */
	UPROPERTY(BlueprintReadOnly, Category="Victory")
	bool bInputEnabled = false;

	/** 획득 후 상태를 표시 중인지 */
	UPROPERTY(BlueprintReadOnly, Category="Victory")
	bool bShowingAfter = false;

	/** 레벨업 발생 여부 */
	UPROPERTY(BlueprintReadOnly, Category="Victory")
	bool bHasLevelUp = false;

	// ─── HUD 참조 ─────────────────────────────────────────────────────────────

	UPROPERTY(BlueprintReadWrite, Category="Victory")
	TObjectPtr<ATBBattleHUD> OwningHUD;

protected:
	virtual void NativeConstruct() override;
	virtual FReply NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent) override;

	/** 1초 후 입력 활성화 */
	UFUNCTION()
	void EnableInput();

	/** 확인 입력 처리 */
	UFUNCTION(BlueprintNativeEvent, Category="Victory")
	void OnConfirm();
	void OnConfirm_Implementation();

	/** UI 갱신 (Blueprint에서 오버라이드) */
	UFUNCTION(BlueprintImplementableEvent, Category="Victory")
	void RefreshUI();

private:
	FTimerHandle InputEnableTimerHandle;
};
