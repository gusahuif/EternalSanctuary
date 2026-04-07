#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"

/**
 * FESGameplayTags
 * 项目全局 GameplayTag 静态管理类
 * 禁止在逻辑代码中硬编码 Tag 字符串，统一通过此类访问
 *
 * 用法：
 *   #include "GAS/GameplayTag/ESGameplayTags.h"
 *   FGameplayTag Tag = FESGameplayTags::State_Dead;
 */
struct ETERNALSANCTUARY_API FESGameplayTags
{
public:
	/** 初始化所有静态 Tag，在 GameInstance 或 Module 启动时调用一次 */
	static void InitializeNativeTags();

	/** 通过 Tag 字符串查找（找不到时输出 Warning） */
	static FGameplayTag FindTag(const FName& TagName);

public:
	// ===================== Ability =====================
	static FGameplayTag Ability;
	static FGameplayTag Ability_Class;
	static FGameplayTag Ability_Class_Archer;
	static FGameplayTag Ability_Class_Archer_Basic;
	static FGameplayTag Ability_Class_Archer_Core;
	static FGameplayTag Ability_Class_Archer_Defense;
	static FGameplayTag Ability_Class_Archer_Mobility;
	static FGameplayTag Ability_Class_Archer_Passive;
	static FGameplayTag Ability_Class_Archer_Status;
	static FGameplayTag Ability_Class_Archer_Ultimate;
	static FGameplayTag Ability_Class_Mage;
	static FGameplayTag Ability_Class_Mage_Basic;
	static FGameplayTag Ability_Class_Mage_Core;
	static FGameplayTag Ability_Class_Mage_Defense;
	static FGameplayTag Ability_Class_Mage_Mobility;
	static FGameplayTag Ability_Class_Mage_Passive;
	static FGameplayTag Ability_Class_Mage_Status;
	static FGameplayTag Ability_Class_Mage_Ultimate;
	static FGameplayTag Ability_Class_Warrior;
	static FGameplayTag Ability_Class_Warrior_Basic;
	static FGameplayTag Ability_Class_Warrior_Core;
	static FGameplayTag Ability_Class_Warrior_Defense;
	static FGameplayTag Ability_Class_Warrior_Mobility;
	static FGameplayTag Ability_Class_Warrior_Passive;
	static FGameplayTag Ability_Class_Warrior_Status;
	static FGameplayTag Ability_Class_Warrior_Ultimate;
	static FGameplayTag Ability_Slot;
	static FGameplayTag Ability_Slot_Basic;
	static FGameplayTag Ability_Slot_Core;
	static FGameplayTag Ability_Slot_Defense;
	static FGameplayTag Ability_Slot_Mobility;
	static FGameplayTag Ability_Slot_Passive;
	static FGameplayTag Ability_Slot_Status;
	static FGameplayTag Ability_Slot_Ultimate;

	// ===================== Character =====================
	static FGameplayTag Character;
	static FGameplayTag Character_Class;
	static FGameplayTag Character_Class_Archer;
	static FGameplayTag Character_Class_Mage;
	static FGameplayTag Character_Class_Warrior;
	static FGameplayTag Character_Enemy;
	static FGameplayTag Character_Enemy_Boss;
	static FGameplayTag Character_Enemy_Elite;
	static FGameplayTag Character_Player;

	// ===================== Damage =====================
	static FGameplayTag Damage;
	static FGameplayTag Damage_Fire;
	static FGameplayTag Damage_Holy;
	static FGameplayTag Damage_Ice;
	static FGameplayTag Damage_Lightning;
	static FGameplayTag Damage_Physical;
	static FGameplayTag Damage_Shadow;
	static FGameplayTag Damage_True;

	// ===================== Effect =====================
	static FGameplayTag Effect;
	static FGameplayTag Effect_Buff;
	static FGameplayTag Effect_Control;
	static FGameplayTag Effect_Debuff;
	static FGameplayTag Effect_Duration;
	static FGameplayTag Effect_Heal;
	static FGameplayTag Effect_Instant;
	static FGameplayTag Effect_Periodic;
	static FGameplayTag Effect_Shield;

	// ===================== Event =====================
	static FGameplayTag Event;
	static FGameplayTag Event_OnAbilityHit;
	static FGameplayTag Event_OnCriticalHit;
	static FGameplayTag Event_OnDamageDealt;
	static FGameplayTag Event_OnDeath;
	static FGameplayTag Event_OnHeal;
	static FGameplayTag Event_OnKill;
	static FGameplayTag Event_OnRevive;
	static FGameplayTag Event_OnShieldBroken;

	// ===================== Input =====================
	static FGameplayTag Input;
	static FGameplayTag Input_Ability;
	static FGameplayTag Input_Ability_Basic;
	static FGameplayTag Input_Ability_Core;
	static FGameplayTag Input_Ability_Defense;
	static FGameplayTag Input_Ability_Mobility;
	static FGameplayTag Input_Ability_Passive;
	static FGameplayTag Input_Ability_Status;
	static FGameplayTag Input_Ability_Ultimate;
	static FGameplayTag Input_Dash;
	static FGameplayTag Input_Interact;
	static FGameplayTag Input_Move;

	// ===================== State =====================
	static FGameplayTag State;
	static FGameplayTag State_Burning;
	static FGameplayTag State_Dead;
	static FGameplayTag State_Frozen;
	static FGameplayTag State_Invincible;
	static FGameplayTag State_Poisoned;
	static FGameplayTag State_Silenced;
	static FGameplayTag State_Slowed;
	static FGameplayTag State_Stunned;
	static FGameplayTag State_SuperArmor;
	static FGameplayTag State_Taunted;
};
