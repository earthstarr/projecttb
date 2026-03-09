#include "TBGameplayTags.h"

// ─── Battle Phase ───────────────────────────────────────────────────────────
UE_DEFINE_GAMEPLAY_TAG(TAG_Battle_Phase_None,       "Battle.Phase.None")
UE_DEFINE_GAMEPLAY_TAG(TAG_Battle_Phase_Start,      "Battle.Phase.Start")
UE_DEFINE_GAMEPLAY_TAG(TAG_Battle_Phase_PlayerTurn, "Battle.Phase.PlayerTurn")
UE_DEFINE_GAMEPLAY_TAG(TAG_Battle_Phase_EnemyTurn,  "Battle.Phase.EnemyTurn")
UE_DEFINE_GAMEPLAY_TAG(TAG_Battle_Phase_Executing,  "Battle.Phase.Executing")
UE_DEFINE_GAMEPLAY_TAG(TAG_Battle_Phase_Victory,    "Battle.Phase.Victory")
UE_DEFINE_GAMEPLAY_TAG(TAG_Battle_Phase_Defeat,     "Battle.Phase.Defeat")

// ─── Combatant State ────────────────────────────────────────────────────────
UE_DEFINE_GAMEPLAY_TAG(TAG_Combatant_State_Dead,       "Combatant.State.Dead")
UE_DEFINE_GAMEPLAY_TAG(TAG_Combatant_State_Stunned,    "Combatant.State.Stunned")
UE_DEFINE_GAMEPLAY_TAG(TAG_Combatant_State_Acting,     "Combatant.State.Acting")
UE_DEFINE_GAMEPLAY_TAG(TAG_Combatant_State_Defending,  "Combatant.State.Defending")
UE_DEFINE_GAMEPLAY_TAG(TAG_Combatant_State_ParrySuccess, "Combatant.State.ParrySuccess")

// ─── Ability Type ───────────────────────────────────────────────────────────
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Type_Physical, "Ability.Type.Physical")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Type_Magic,    "Ability.Type.Magic")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Type_Support,  "Ability.Type.Support")

// ─── Ability Cost ───────────────────────────────────────────────────────────
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Cost_Stamina, "Ability.Cost.Stamina")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Cost_MP,      "Ability.Cost.MP")
UE_DEFINE_GAMEPLAY_TAG(TAG_Ability_Cost_HP,      "Ability.Cost.HP")

// ─── Effect ─────────────────────────────────────────────────────────────────
UE_DEFINE_GAMEPLAY_TAG(TAG_Effect_Damage_Physical, "Effect.Damage.Physical")
UE_DEFINE_GAMEPLAY_TAG(TAG_Effect_Damage_Magic,    "Effect.Damage.Magic")
UE_DEFINE_GAMEPLAY_TAG(TAG_Effect_Heal,            "Effect.Heal")

// ─── Status Effect ──────────────────────────────────────────────────────────
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Burn,   "Status.Burn")
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Poison, "Status.Poison")
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Regen,  "Status.Regen")
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Stun,   "Status.Stun")
UE_DEFINE_GAMEPLAY_TAG(TAG_Status_Taunt,  "Status.Taunt")

// ─── SetByCaller Data ───────────────────────────────────────────────────────
UE_DEFINE_GAMEPLAY_TAG(TAG_Data_AbilityMultiplier, "Data.AbilityMultiplier")
