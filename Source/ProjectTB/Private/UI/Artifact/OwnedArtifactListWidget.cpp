// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Artifact/OwnedArtifactListWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Blueprint/WidgetLayoutLibrary.h"
#include "TBGameInstance.h"
#include "Components/ScrollBox.h"
#include "Components/Widget.h"
#include "UI/Artifact/ArtifactDescriptionPopupWidget.h"
#include "UI/Artifact/OwnedArtifactSlotWidget.h"

void UOwnedArtifactListWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		GI->OnOwnedArtifactsChanged.AddDynamic(this, &UOwnedArtifactListWidget::HandleOwnedArtifactsChanged);
		UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] Bound OnOwnedArtifactsChanged in UOwnedArtifactListWidget::NativeConstruct"));
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] GameInstance is nullptr in UOwnedArtifactListWidget::NativeConstruct"));
	}

	RefreshOwnedArtifacts();
}

void UOwnedArtifactListWidget::NativeDestruct()
{
	if (UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance()))
	{
		GI->OnOwnedArtifactsChanged.RemoveDynamic(this, &UOwnedArtifactListWidget::HandleOwnedArtifactsChanged);
	}

	Super::NativeDestruct();
}

void UOwnedArtifactListWidget::RefreshOwnedArtifacts()
{
	if (ArtifactScrollBox == nullptr || ArtifactItemWidgetClass == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] ArtifactScrollBox or ArtifactItemWidgetClass is nullptr in UOwnedArtifactListWidget::RefreshOwnedArtifacts"));
		return;
	}

	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (GI == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] GameInstance is nullptr in UOwnedArtifactListWidget::RefreshOwnedArtifacts"));
		return;
	}

	ArtifactScrollBox->ClearChildren();

	TArray<FArtifactEntry> OwnedEntries;
	for (const FName& ArtifactID : GI->PartyArtifactData.EquippedArtifact)
	{
		FArtifactData ArtifactRow;
		if (!GI->GetArtifactRow(ArtifactID, ArtifactRow))
		{
			continue;
		}

		FArtifactEntry Entry;
		Entry.ArtifactID = ArtifactID;
		Entry.ArtifactData = ArtifactRow;
		OwnedEntries.Add(Entry);
	}

	// 아티팩트가 Set이기 때문에 목록에서는 위치를 고정하기 위해 정렬이 필요.
	SortOwnedArtifactsByID(OwnedEntries);
	UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] RefreshOwnedArtifacts found %d owned entries"), OwnedEntries.Num());

	for (const FArtifactEntry& Entry : OwnedEntries)
	{
		UOwnedArtifactSlotWidget* ItemWidget = CreateWidget<UOwnedArtifactSlotWidget>(this, ArtifactItemWidgetClass);
		if (ItemWidget == nullptr)
		{
			UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] Failed to create UOwnedArtifactSlotWidget for %s"), *Entry.ArtifactID.ToString());
			continue;
		}

		ItemWidget->InitializeArtifactSlot(Entry, this);
		ArtifactScrollBox->AddChild(ItemWidget);
	}
}

void UOwnedArtifactListWidget::HandleOwnedArtifactsChanged()
{
	RefreshOwnedArtifacts();
}

void UOwnedArtifactListWidget::MoveToViewportPosition(FVector2D Position)
{
	SetPositionInViewport(Position, false);
	UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] MoveToViewportPosition to X=%.2f Y=%.2f"), Position.X, Position.Y);
}

void UOwnedArtifactListWidget::RestoreFieldLayout()
{
	SetRenderScale(FVector2D(1.f, 1.f));
	MoveToViewportPosition(FVector2D::ZeroVector);
}

void UOwnedArtifactListWidget::MoveToWidgetAnchor(UWidget* AnchorWidget)
{
	if (AnchorWidget == nullptr)
	{
		UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] AnchorWidget is nullptr in UOwnedArtifactListWidget::MoveToWidgetAnchor"));
		return;
	}

	const FGeometry& AnchorGeometry = AnchorWidget->GetCachedGeometry();
	const FVector2D AnchorLocalSize = AnchorGeometry.GetLocalSize();
	const FVector2D AnchorLocalTopRight(AnchorGeometry.GetLocalSize().X, 0.f);
	const FVector2D AnchorAbsoluteTopRight = AnchorGeometry.LocalToAbsolute(AnchorLocalTopRight);

	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::AbsoluteToViewport(this, AnchorAbsoluteTopRight, PixelPosition, ViewportPosition);

	const FVector2D ViewportSize = UWidgetLayoutLibrary::GetViewportSize(this);
	const FVector2D DesiredSize = GetDesiredSize();
	FVector2D FinalPosition = ViewportPosition + FVector2D(ShopAnchorOffset, 0.f);
	FinalPosition.X = FMath::Clamp(FinalPosition.X, 0.f, FMath::Max(0.f, ViewportSize.X - DesiredSize.X));
	FinalPosition.Y = FMath::Clamp(FinalPosition.Y, 0.f, FMath::Max(0.f, ViewportSize.Y - DesiredSize.Y));

	SetRenderScale(ShopRenderScale);
	SetPositionInViewport(FinalPosition, false);
	UE_LOG(LogTemp, Warning, TEXT("[ArtifactListDebug] MoveToWidgetAnchor anchor size=(%.2f, %.2f) widget desired size=(%.2f, %.2f) viewport size=(%.2f, %.2f) raw viewport position=(%.2f, %.2f) final position=(%.2f, %.2f) offset=%.2f scale=(%.2f, %.2f)"),
		AnchorLocalSize.X, AnchorLocalSize.Y,
		DesiredSize.X, DesiredSize.Y,
		ViewportSize.X, ViewportSize.Y,
		ViewportPosition.X, ViewportPosition.Y,
		FinalPosition.X, FinalPosition.Y,
		ShopAnchorOffset, ShopRenderScale.X, ShopRenderScale.Y);
}

void UOwnedArtifactListWidget::SortOwnedArtifactsByID(TArray<FArtifactEntry>& OwnedEntries) const
{
	OwnedEntries.Sort([](const FArtifactEntry& A, const FArtifactEntry& B)
	{
		return A.ArtifactID.LexicalLess(B.ArtifactID);
	});
}

void UOwnedArtifactListWidget::HandleArtifactItemClicked(const FArtifactEntry& InArtifactEntry, const FVector2D& ClickPosition)
{
	if (ArtifactDescriptionPopupClass == nullptr)
	{
		return;
	}

	if (ActivePopup)
	{
		ActivePopup->RemoveFromParent();
		ActivePopup = nullptr;
	}

	APlayerController* PC = GetOwningPlayer();
	if (PC == nullptr)
	{
		PC = GetWorld() ? GetWorld()->GetFirstPlayerController() : nullptr;
	}

	if (PC == nullptr)
	{
		return;
	}

	ActivePopup = CreateWidget<UArtifactDescriptionPopupWidget>(PC, ArtifactDescriptionPopupClass);
	if (ActivePopup == nullptr)
	{
		return;
	}

	ActivePopup->AddToViewport(200);
	ActivePopup->InitializePopup(InArtifactEntry, ClickPosition);
}
