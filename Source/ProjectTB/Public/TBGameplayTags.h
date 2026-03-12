#pragma once

#include "NativeGameplayTags.h"

// ─── Battle Phase ───────────────────────────────────────────────────────────
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Battle_Phase_None)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Battle_Phase_Start)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Battle_Phase_PlayerTurn)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Battle_Phase_EnemyTurn)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Battle_Phase_Executing)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Battle_Phase_Victory)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Battle_Phase_Defeat)

// ─── Combatant State ────────────────────────────────────────────────────────
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Combatant_State_Dead)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Combatant_State_Stunned)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Combatant_State_Acting)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Combatant_State_Defending)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Combatant_State_ParrySuccess)

// ─── Ability Type ───────────────────────────────────────────────────────────
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Type_Physical)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Type_Magic)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Type_Support)

// ─── Ability Cost ───────────────────────────────────────────────────────────
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Cost_Stamina)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Cost_MP)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Ability_Cost_HP)

// ─── Effect ─────────────────────────────────────────────────────────────────
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Effect_Damage_Physical)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Effect_Damage_Magic)
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Effect_Heal)

// ─── Status Effect ──────────────────────────────────────────────────────────
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Burn)     // 화상: 턴당 데미지 + 받는 데미지 +25%
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Poison)   // 독: 턴당 데미지(HP%+스탯) + Speed 30% 감소
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Regen)    // 재생: 턴당 힐
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Stun)     // 스턴: 턴 스킵
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Status_Taunt)    // 도발: 시전자를 우선 타겟

// ─── SetByCaller Data ───────────────────────────────────────────────────────
// 데미지 GE에서 SetByCaller로 값을 주입할 때 사용
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_AbilityMultiplier)

// ─── Artifact Data ──────────────────────────────────────────────────────────
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Artifact_Resurrection)	// 부활