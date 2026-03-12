// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Data/ArtifactDataTypes.h"
#include "OwnedArtifactListWidget.generated.h"

class UArtifactDescriptionPopupWidget;
class UOwnedArtifactSlotWidget;
class UScrollBox;
class UWidget;

UCLASS()
class PROJECTTB_API UOwnedArtifactListWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	virtual void NativeConstruct() override;
	virtual void NativeDestruct() override;

	UFUNCTION(BlueprintCallable, Category="Artifact")
	void RefreshOwnedArtifacts();

	UFUNCTION(BlueprintCallable, Category="Artifact|Layout")
	void MoveToViewportPosition(FVector2D Position);

	UFUNCTION(BlueprintCallable, Category="Artifact|Layout")
	void MoveToWidgetAnchor(UWidget* AnchorWidget);

	UFUNCTION(BlueprintCallable, Category="Artifact|Layout")
	void RestoreFieldLayout();

	void HandleArtifactItemClicked(const FArtifactEntry& InArtifactEntry, const FVector2D& ClickPosition);

protected:
	UPROPERTY(meta=(BindWidget))
	TObjectPtr<UScrollBox> ArtifactScrollBox;

	// 아티팩트 아이콘이 그려진 하나의 버튼 (ArtifactScrollBox 스크롤 박스의 한 슬롯)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact")
	TSubclassOf<UOwnedArtifactSlotWidget> ArtifactItemWidgetClass;

	// 아티팩트 효과 설명 팝업 위젯
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Artifact")
	TSubclassOf<UArtifactDescriptionPopupWidget> ArtifactDescriptionPopupClass;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact|Layout")
	float ShopAnchorOffset = 0.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Artifact|Layout")
	FVector2D ShopRenderScale = FVector2D(1.f, 1.f);

private:
	UFUNCTION()
	void HandleOwnedArtifactsChanged();

	void SortOwnedArtifactsByID(TArray<FArtifactEntry>& OwnedEntries) const;

	UPROPERTY(Transient)
	TObjectPtr<UArtifactDescriptionPopupWidget> ActivePopup;
};
