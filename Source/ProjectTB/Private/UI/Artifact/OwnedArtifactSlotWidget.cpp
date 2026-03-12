// Fill out your copyright notice in the Description page of Project Settings.


#include "UI/Artifact/OwnedArtifactSlotWidget.h"

#include "Blueprint/SlateBlueprintLibrary.h"
#include "Components/Button.h"
#include "Components/Image.h"
#include "UI/Artifact/OwnedArtifactListWidget.h"

void UOwnedArtifactSlotWidget::NativeConstruct()
{
	Super::NativeConstruct();

	if (ArtifactButton)
	{
		ArtifactButton->OnClicked.AddDynamic(this, &UOwnedArtifactSlotWidget::HandleArtifactButtonClicked);
	}
}

void UOwnedArtifactSlotWidget::InitializeArtifactSlot(const FArtifactEntry& InArtifactEntry, UOwnedArtifactListWidget* InOwnerList)
{
	ArtifactEntry = InArtifactEntry;
	OwnerListWidget = InOwnerList;

	if (ArtifactIcon)
	{
		if (UTexture2D* IconTexture = ArtifactEntry.ArtifactData.Icon.LoadSynchronous())
		{
			ArtifactIcon->SetBrushFromTexture(IconTexture);
		}
	}
}

void UOwnedArtifactSlotWidget::HandleArtifactButtonClicked()
{
	if (OwnerListWidget == nullptr)
	{
		return;
	}

	const FGeometry& SlotGeometry = ArtifactButton ? ArtifactButton->GetCachedGeometry() : GetCachedGeometry();
	const FVector2D SlotLocalSize = SlotGeometry.GetLocalSize();
	const FVector2D SlotAbsoluteTopLeft = SlotGeometry.LocalToAbsolute(FVector2D::ZeroVector);
	const FVector2D SlotAbsoluteTopRight = SlotGeometry.LocalToAbsolute(FVector2D(SlotLocalSize.X, 0.f));

	FVector2D PixelPosition;
	FVector2D ViewportPosition;
	USlateBlueprintLibrary::LocalToViewport(this, SlotGeometry, FVector2D(SlotLocalSize.X, 0.f), PixelPosition, ViewportPosition);

	UE_LOG(LogTemp, Warning, TEXT("[ArtifactPopupDebug] Slot click ArtifactID=%s local size=(%.2f, %.2f) abs TL=(%.2f, %.2f) abs TR=(%.2f, %.2f) pixel=(%.2f, %.2f) viewport=(%.2f, %.2f)"),
		*ArtifactEntry.ArtifactID.ToString(),
		SlotLocalSize.X, SlotLocalSize.Y,
		SlotAbsoluteTopLeft.X, SlotAbsoluteTopLeft.Y,
		SlotAbsoluteTopRight.X, SlotAbsoluteTopRight.Y,
		PixelPosition.X, PixelPosition.Y,
		ViewportPosition.X, ViewportPosition.Y);

	OwnerListWidget->HandleArtifactItemClicked(ArtifactEntry, PixelPosition);
}
