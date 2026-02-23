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

// ─── SetByCaller Data ───────────────────────────────────────────────────────
// 데미지 GE에서 SetByCaller로 값을 주입할 때 사용
UE_DECLARE_GAMEPLAY_TAG_EXTERN(TAG_Data_AbilityMultiplier)
