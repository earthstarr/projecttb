#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/LevelDataTypes.h"
#include "LevelUpWidget.generated.h"

class ATBBattleHUD;

/**
 * 레벨업 화면 위젯.
 * 3캐릭터 스탯 증가량을 한번에 표시.
 * 1초 후 입력 활성화, 엔터 시 월드 복귀.
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

	// ─── 데이터 (Blueprint에서 바인딩) ─────────────────────────────────────────

	UPROPERTY(BlueprintReadOnly, Category="LevelUp")
	TArray<FLevelUpDisplayData> LevelUpData;

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

private:
	FTimerHandle InputEnableTimerHandle;
};
