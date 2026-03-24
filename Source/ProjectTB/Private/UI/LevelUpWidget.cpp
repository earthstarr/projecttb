#include "UI/LevelUpWidget.h"

#include "Attributes/TBAttributeSet.h"
#include "Data/ArtifactDataTypes.h"
#include "TBGameInstance.h"
#include "UI/TBBattleHUD.h"
#include "TimerManager.h"

namespace
{
bool DoesArtifactAffectMember(const FArtifactData& ArtifactRow, const FPartyMemberData& Member)
{
	switch (ArtifactRow.TargetMode)
	{
	case EArtifactTargetMode::AllParty:
		return true;

	case EArtifactTargetMode::MatchingJob:
		return ArtifactRow.TargetJobIds.Contains(Member.CharacterId);
	}

	return false;
}

void ApplyArtifactModifier(
	const FArtifactStatModifier& Modifier,
	FCharacterLevelStats& InOutFinalStats,
	FPartyCurrentStatusData& InOutCurrentStatus)
{
	const float Value = Modifier.Value;

	if (Modifier.ModifierOp == EGameplayModOp::Override)
	{
		if (Modifier.Attribute == UTBAttributeSet::GetMaxHPAttribute())              { InOutFinalStats.MaxHP = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetHPAttribute())                 { InOutCurrentStatus.CurrentHP = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetMaxMPAttribute())              { InOutFinalStats.MaxMP = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetMPAttribute())                 { InOutCurrentStatus.CurrentMP = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetMaxStaminaAttribute())         { InOutFinalStats.MaxStamina = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetStaminaAttribute())            { InOutCurrentStatus.CurrentStamina = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetPhysicalAttackAttribute())     { InOutFinalStats.PhysicalAttack = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetMagicAttackAttribute())        { InOutFinalStats.MagicAttack = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetPhysicalDefenseAttribute())    { InOutFinalStats.PhysicalDefense = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetMagicDefenseAttribute())       { InOutFinalStats.MagicDefense = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetSpeedAttribute())              { InOutFinalStats.Speed = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetCriticalChanceAttribute())     { InOutFinalStats.CriticalChance = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetCriticalMultiplierAttribute()) { InOutFinalStats.CriticalMultiplier = Value; return; }
		if (Modifier.Attribute == UTBAttributeSet::GetDiceFaceBonusAttribute())      { InOutCurrentStatus.DiceFaceBonus = FMath::RoundToInt(Value); return; }
		if (Modifier.Attribute == UTBAttributeSet::GetDiceMinFaceAttribute())        { InOutCurrentStatus.DiceMinFace = FMath::RoundToInt(Value); return; }
		if (Modifier.Attribute == UTBAttributeSet::GetDiceMaxFaceAttribute())        { InOutCurrentStatus.DiceMaxFace = FMath::RoundToInt(Value); return; }
		return;
	}

	if (Modifier.Attribute == UTBAttributeSet::GetMaxHPAttribute())              { InOutFinalStats.MaxHP += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetHPAttribute())                 { InOutCurrentStatus.CurrentHP += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetMaxMPAttribute())              { InOutFinalStats.MaxMP += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetMPAttribute())                 { InOutCurrentStatus.CurrentMP += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetMaxStaminaAttribute())         { InOutFinalStats.MaxStamina += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetStaminaAttribute())            { InOutCurrentStatus.CurrentStamina += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetPhysicalAttackAttribute())     { InOutFinalStats.PhysicalAttack += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetMagicAttackAttribute())        { InOutFinalStats.MagicAttack += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetPhysicalDefenseAttribute())    { InOutFinalStats.PhysicalDefense += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetMagicDefenseAttribute())       { InOutFinalStats.MagicDefense += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetSpeedAttribute())              { InOutFinalStats.Speed += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetCriticalChanceAttribute())     { InOutFinalStats.CriticalChance += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetCriticalMultiplierAttribute()) { InOutFinalStats.CriticalMultiplier += Value; return; }
	if (Modifier.Attribute == UTBAttributeSet::GetDiceFaceBonusAttribute())      { InOutCurrentStatus.DiceFaceBonus += FMath::RoundToInt(Value); return; }
	if (Modifier.Attribute == UTBAttributeSet::GetDiceMinFaceAttribute())        { InOutCurrentStatus.DiceMinFace += FMath::RoundToInt(Value); return; }
	if (Modifier.Attribute == UTBAttributeSet::GetDiceMaxFaceAttribute())        { InOutCurrentStatus.DiceMaxFace += FMath::RoundToInt(Value); return; }
}
}

void ULevelUpWidget::NativeConstruct()
{
	Super::NativeConstruct();
	SetIsFocusable(true);
}

void ULevelUpWidget::Initialize(const TArray<FLevelUpDisplayData>& InLevelUpData)
{
	LevelUpData   = InLevelUpData;
	bInputEnabled = false;

	// 1초 후 입력 활성화
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(
			InputEnableTimerHandle,
			this,
			&ULevelUpWidget::EnableInput,
			1.0f,
			false
		);
	}
}

void ULevelUpWidget::RefreshFromGameState()
{
	FinalPartyStats.Reset();
	CurrentPartyStatus.Reset();

	UTBGameInstance* GI = Cast<UTBGameInstance>(GetGameInstance());
	if (GI == nullptr)
	{
		return;
	}

	CurrentMoney = GI->GetCurrentMoney();
	
	for (const FPartyMemberData& Member : GI->PartyData)
	{
		FCharacterLevelStats FinalStats;
		if (!GI->GetLevelStats(Member.CharacterId, Member.Level, FinalStats))
		{
			continue;
		}

		FinalStats.CharacterId = Member.CharacterId;
		FinalStats.Level = Member.Level;

		FPartyCurrentStatusData CurrentStatus;
		CurrentStatus.CharacterId = Member.CharacterId;
		CurrentStatus.Level = Member.Level;
		CurrentStatus.CurrentHP = (Member.CurrentHP < 0.f)
			? FinalStats.MaxHP
			: FMath::Clamp(Member.CurrentHP, 0.f, FinalStats.MaxHP);
		CurrentStatus.CurrentMP = (Member.CurrentMP < 0.f)
			? FinalStats.MaxMP
			: FMath::Clamp(Member.CurrentMP, 0.f, FinalStats.MaxMP);
		CurrentStatus.CurrentStamina = (Member.CurrentStamina < 0.f)
			? FinalStats.MaxStamina
			: FMath::Clamp(Member.CurrentStamina, 0.f, FinalStats.MaxStamina);

		for (const FName& ArtifactID : GI->PartyArtifactData.EquippedArtifact)
		{
			FArtifactData ArtifactRow;
			if (!GI->GetArtifactRow(ArtifactID, ArtifactRow))
			{
				continue;
			}

			if (!DoesArtifactAffectMember(ArtifactRow, Member))
			{
				continue;
			}

			for (const FArtifactStatModifier& Modifier : ArtifactRow.StatModifiers)
			{
				ApplyArtifactModifier(Modifier, FinalStats, CurrentStatus);
			}
		}

		CurrentStatus.CurrentHP = FMath::Clamp(CurrentStatus.CurrentHP, 0.f, FinalStats.MaxHP);
		CurrentStatus.CurrentMP = FMath::Clamp(CurrentStatus.CurrentMP, 0.f, FinalStats.MaxMP);
		CurrentStatus.CurrentStamina = FMath::Clamp(CurrentStatus.CurrentStamina, 0.f, FinalStats.MaxStamina);

		if (CurrentStatus.DiceMinFace > CurrentStatus.DiceMaxFace)
		{
			Swap(CurrentStatus.DiceMinFace, CurrentStatus.DiceMaxFace);
		}

		FinalPartyStats.Add(FinalStats);
		CurrentPartyStatus.Add(CurrentStatus);
	}

	OnPartyStatusUpdated();
}

void ULevelUpWidget::EnableInput()
{
	bInputEnabled = true;

	// 포커스 설정
	if (APlayerController* PC = GetOwningPlayer())
	{
		SetUserFocus(PC);
	}
}

FReply ULevelUpWidget::NativeOnKeyDown(const FGeometry& InGeometry, const FKeyEvent& InKeyEvent)
{
	if (!bInputEnabled)
		return FReply::Handled();

	const FKey Key = InKeyEvent.GetKey();

	if (Key == EKeys::Enter || Key == EKeys::SpaceBar)
	{
		OnConfirm();
		return FReply::Handled();
	}

	return Super::NativeOnKeyDown(InGeometry, InKeyEvent);
}

void ULevelUpWidget::OnConfirm_Implementation()
{
	if (!OwningHUD) return;

	// 월드 복귀
	OwningHUD->ReturnToWorld();

	// 이 위젯 숨기기
	RemoveFromParent();
}
