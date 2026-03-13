#include "UI/DamageNumberWidget.h"
#include "Components/TextBlock.h"
#include "Kismet/GameplayStatics.h"
#include "Blueprint/WidgetLayoutLibrary.h"

void UDamageNumberWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetRenderTranslation(FVector2D::ZeroVector);
}

void UDamageNumberWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	// 타겟 따라가기 (카메라 이동 대응)
	UpdateScreenPosition();

	// 스택 오프셋 애니메이션 처리
	if (!FMath::IsNearlyEqual(CurrentOffsetY, TargetOffsetY, 0.5f))
	{
		CurrentOffsetY = FMath::FInterpTo(CurrentOffsetY, TargetOffsetY, InDeltaTime, 10.f);
	}

	// 페이드 아웃 처리
	if (bFadingOut)
	{
		FadeOutTimer += InDeltaTime;
		const float Alpha = FMath::Clamp(1.f - (FadeOutTimer / FadeOutDuration), 0.f, 1.f);
		SetRenderOpacity(Alpha);

		if (Alpha <= 0.f)
		{
			OnFinished.ExecuteIfBound();
			RemoveFromParent();
		}
	}
}

void UDamageNumberWidget::UpdateScreenPosition()
{
	if (!FollowTarget.IsValid()) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	// 타겟의 월드 위치 → 스크린 좌표
	FVector WorldLocation = FollowTarget->GetActorLocation() + FVector(0.f, 0.f, HeightOffset);
	FVector2D ScreenPos;

	if (UGameplayStatics::ProjectWorldToScreen(PC, WorldLocation, ScreenPos))
	{
		// 뷰포트 스케일 고려
		float Scale = UWidgetLayoutLibrary::GetViewportScale(this);
		ScreenPos /= Scale;

		// 스택 오프셋 적용 (위로 이동)
		ScreenPos.Y += CurrentOffsetY;

		// 위젯 중앙 정렬을 위해 약간 왼쪽으로
		SetPositionInViewport(ScreenPos - FVector2D(30.f, 0.f), false);
	}
}

void UDamageNumberWidget::SetFollowTarget(AActor* InTarget, float InHeightOffset)
{
	FollowTarget = InTarget;
	HeightOffset = InHeightOffset;
}

void UDamageNumberWidget::SetDamage(float Damage, bool bIsCritical)
{
	if (DamageText)
	{
		if (bIsCritical)
		{
			DamageText->SetColorAndOpacity(FSlateColor(FLinearColor::Yellow));
		}
		else
		{
			DamageText->SetColorAndOpacity(FSlateColor(FLinearColor::Red));
		}
		DamageText->SetText(FText::AsNumber(FMath::RoundToInt(Damage)));
	}
}

void UDamageNumberWidget::SetHeal(float Heal)
{
	if (DamageText)
	{
		DamageText->SetColorAndOpacity(FSlateColor(FLinearColor::Green));
		DamageText->SetText(FText::FromString(FString::Printf(TEXT("+%d"), FMath::RoundToInt(Heal))));
	}
}

void UDamageNumberWidget::MoveUp(float Distance)
{
	// 음수 Y = 위로 이동 (Screen Space)
	TargetOffsetY -= Distance;
}

void UDamageNumberWidget::StartLifespan(float Duration)
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			LifespanTimer,
			this,
			&UDamageNumberWidget::OnLifespanExpired,
			Duration,
			false
		);
	}
}

void UDamageNumberWidget::OnLifespanExpired()
{
	bFadingOut = true;
}
