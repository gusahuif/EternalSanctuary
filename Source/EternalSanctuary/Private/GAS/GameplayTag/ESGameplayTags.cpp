#include "GAS/GameplayTag/ESGameplayTags.h"
#include "GameplayTagsManager.h"

// ===================== 静态成员定义 =====================
FGameplayTag FESGameplayTags::Ability;
FGameplayTag FESGameplayTags::Ability_Class;
FGameplayTag FESGameplayTags::Ability_Class_Archer;
FGameplayTag FESGameplayTags::Ability_Class_Archer_Basic;
FGameplayTag FESGameplayTags::Ability_Class_Archer_Core;
FGameplayTag FESGameplayTags::Ability_Class_Archer_Defense;
FGameplayTag FESGameplayTags::Ability_Class_Archer_Mobility;
FGameplayTag FESGameplayTags::Ability_Class_Archer_Passive;
FGameplayTag FESGameplayTags::Ability_Class_Archer_Status;
FGameplayTag FESGameplayTags::Ability_Class_Archer_Ultimate;
FGameplayTag FESGameplayTags::Ability_Class_Mage;
FGameplayTag FESGameplayTags::Ability_Class_Mage_Basic;
FGameplayTag FESGameplayTags::Ability_Class_Mage_Core;
FGameplayTag FESGameplayTags::Ability_Class_Mage_Defense;
FGameplayTag FESGameplayTags::Ability_Class_Mage_Mobility;
FGameplayTag FESGameplayTags::Ability_Class_Mage_Passive;
FGameplayTag FESGameplayTags::Ability_Class_Mage_Status;
FGameplayTag FESGameplayTags::Ability_Class_Mage_Ultimate;
FGameplayTag FESGameplayTags::Ability_Class_Warrior;
FGameplayTag FESGameplayTags::Ability_Class_Warrior_Basic;
FGameplayTag FESGameplayTags::Ability_Class_Warrior_Core;
FGameplayTag FESGameplayTags::Ability_Class_Warrior_Defense;
FGameplayTag FESGameplayTags::Ability_Class_Warrior_Mobility;
FGameplayTag FESGameplayTags::Ability_Class_Warrior_Passive;
FGameplayTag FESGameplayTags::Ability_Class_Warrior_Status;
FGameplayTag FESGameplayTags::Ability_Class_Warrior_Ultimate;
FGameplayTag FESGameplayTags::Ability_Slot;
FGameplayTag FESGameplayTags::Ability_Slot_Basic;
FGameplayTag FESGameplayTags::Ability_Slot_Core;
FGameplayTag FESGameplayTags::Ability_Slot_Defense;
FGameplayTag FESGameplayTags::Ability_Slot_Mobility;
FGameplayTag FESGameplayTags::Ability_Slot_Passive;
FGameplayTag FESGameplayTags::Ability_Slot_Status;
FGameplayTag FESGameplayTags::Ability_Slot_Ultimate;
FGameplayTag FESGameplayTags::Character;
FGameplayTag FESGameplayTags::Character_Class;
FGameplayTag FESGameplayTags::Character_Class_Archer;
FGameplayTag FESGameplayTags::Character_Class_Mage;
FGameplayTag FESGameplayTags::Character_Class_Warrior;
FGameplayTag FESGameplayTags::Character_Enemy;
FGameplayTag FESGameplayTags::Character_Enemy_Boss;
FGameplayTag FESGameplayTags::Character_Enemy_Elite;
FGameplayTag FESGameplayTags::Character_Player;
FGameplayTag FESGameplayTags::Damage;
FGameplayTag FESGameplayTags::Damage_Fire;
FGameplayTag FESGameplayTags::Damage_Holy;
FGameplayTag FESGameplayTags::Damage_Ice;
FGameplayTag FESGameplayTags::Damage_Lightning;
FGameplayTag FESGameplayTags::Damage_Physical;
FGameplayTag FESGameplayTags::Damage_Shadow;
FGameplayTag FESGameplayTags::Damage_True;
FGameplayTag FESGameplayTags::Effect;
FGameplayTag FESGameplayTags::Effect_Buff;
FGameplayTag FESGameplayTags::Effect_Control;
FGameplayTag FESGameplayTags::Effect_Debuff;
FGameplayTag FESGameplayTags::Effect_Duration;
FGameplayTag FESGameplayTags::Effect_Heal;
FGameplayTag FESGameplayTags::Effect_Instant;
FGameplayTag FESGameplayTags::Effect_Periodic;
FGameplayTag FESGameplayTags::Effect_Shield;
FGameplayTag FESGameplayTags::Event;
FGameplayTag FESGameplayTags::Event_OnAbilityHit;
FGameplayTag FESGameplayTags::Event_OnCriticalHit;
FGameplayTag FESGameplayTags::Event_OnDamageDealt;
FGameplayTag FESGameplayTags::Event_OnDeath;
FGameplayTag FESGameplayTags::Event_OnHeal;
FGameplayTag FESGameplayTags::Event_OnKill;
FGameplayTag FESGameplayTags::Event_OnRevive;
FGameplayTag FESGameplayTags::Event_OnShieldBroken;
FGameplayTag FESGameplayTags::Input;
FGameplayTag FESGameplayTags::Input_Ability;
FGameplayTag FESGameplayTags::Input_Ability_Basic;
FGameplayTag FESGameplayTags::Input_Ability_Core;
FGameplayTag FESGameplayTags::Input_Ability_Defense;
FGameplayTag FESGameplayTags::Input_Ability_Mobility;
FGameplayTag FESGameplayTags::Input_Ability_Passive;
FGameplayTag FESGameplayTags::Input_Ability_Status;
FGameplayTag FESGameplayTags::Input_Ability_Ultimate;
FGameplayTag FESGameplayTags::Input_Dash;
FGameplayTag FESGameplayTags::Input_Interact;
FGameplayTag FESGameplayTags::Input_Move;
FGameplayTag FESGameplayTags::State;
FGameplayTag FESGameplayTags::State_Burning;
FGameplayTag FESGameplayTags::State_Dead;
FGameplayTag FESGameplayTags::State_Frozen;
FGameplayTag FESGameplayTags::State_Invincible;
FGameplayTag FESGameplayTags::State_Poisoned;
FGameplayTag FESGameplayTags::State_Silenced;
FGameplayTag FESGameplayTags::State_Slowed;
FGameplayTag FESGameplayTags::State_Stunned;
FGameplayTag FESGameplayTags::State_SuperArmor;
FGameplayTag FESGameplayTags::State_Taunted;

// ===================== 初始化 =====================
void FESGameplayTags::InitializeNativeTags()
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();

	Ability                        = Manager.RequestGameplayTag(FName("Ability"));
	Ability_Class                  = Manager.RequestGameplayTag(FName("Ability.Class"));
	Ability_Class_Archer           = Manager.RequestGameplayTag(FName("Ability.Class.Archer"));
	Ability_Class_Archer_Basic     = Manager.RequestGameplayTag(FName("Ability.Class.Archer.Basic"));
	Ability_Class_Archer_Core      = Manager.RequestGameplayTag(FName("Ability.Class.Archer.Core"));
	Ability_Class_Archer_Defense   = Manager.RequestGameplayTag(FName("Ability.Class.Archer.Defense"));
	Ability_Class_Archer_Mobility  = Manager.RequestGameplayTag(FName("Ability.Class.Archer.Mobility"));
	Ability_Class_Archer_Passive   = Manager.RequestGameplayTag(FName("Ability.Class.Archer.Passive"));
	Ability_Class_Archer_Status    = Manager.RequestGameplayTag(FName("Ability.Class.Archer.Status"));
	Ability_Class_Archer_Ultimate  = Manager.RequestGameplayTag(FName("Ability.Class.Archer.Ultimate"));
	Ability_Class_Mage             = Manager.RequestGameplayTag(FName("Ability.Class.Mage"));
	Ability_Class_Mage_Basic       = Manager.RequestGameplayTag(FName("Ability.Class.Mage.Basic"));
	Ability_Class_Mage_Core        = Manager.RequestGameplayTag(FName("Ability.Class.Mage.Core"));
	Ability_Class_Mage_Defense     = Manager.RequestGameplayTag(FName("Ability.Class.Mage.Defense"));
	Ability_Class_Mage_Mobility    = Manager.RequestGameplayTag(FName("Ability.Class.Mage.Mobility"));
	Ability_Class_Mage_Passive     = Manager.RequestGameplayTag(FName("Ability.Class.Mage.Passive"));
	Ability_Class_Mage_Status      = Manager.RequestGameplayTag(FName("Ability.Class.Mage.Status"));
	Ability_Class_Mage_Ultimate    = Manager.RequestGameplayTag(FName("Ability.Class.Mage.Ultimate"));
	Ability_Class_Warrior          = Manager.RequestGameplayTag(FName("Ability.Class.Warrior"));
	Ability_Class_Warrior_Basic    = Manager.RequestGameplayTag(FName("Ability.Class.Warrior.Basic"));
	Ability_Class_Warrior_Core     = Manager.RequestGameplayTag(FName("Ability.Class.Warrior.Core"));
	Ability_Class_Warrior_Defense  = Manager.RequestGameplayTag(FName("Ability.Class.Warrior.Defense"));
	Ability_Class_Warrior_Mobility = Manager.RequestGameplayTag(FName("Ability.Class.Warrior.Mobility"));
	Ability_Class_Warrior_Passive  = Manager.RequestGameplayTag(FName("Ability.Class.Warrior.Passive"));
	Ability_Class_Warrior_Status   = Manager.RequestGameplayTag(FName("Ability.Class.Warrior.Status"));
	Ability_Class_Warrior_Ultimate = Manager.RequestGameplayTag(FName("Ability.Class.Warrior.Ultimate"));
	Ability_Slot                   = Manager.RequestGameplayTag(FName("Ability.Slot"));
	Ability_Slot_Basic             = Manager.RequestGameplayTag(FName("Ability.Slot.Basic"));
	Ability_Slot_Core              = Manager.RequestGameplayTag(FName("Ability.Slot.Core"));
	Ability_Slot_Defense           = Manager.RequestGameplayTag(FName("Ability.Slot.Defense"));
	Ability_Slot_Mobility          = Manager.RequestGameplayTag(FName("Ability.Slot.Mobility"));
	Ability_Slot_Passive           = Manager.RequestGameplayTag(FName("Ability.Slot.Passive"));
	Ability_Slot_Status            = Manager.RequestGameplayTag(FName("Ability.Slot.Status"));
	Ability_Slot_Ultimate          = Manager.RequestGameplayTag(FName("Ability.Slot.Ultimate"));

	Character               = Manager.RequestGameplayTag(FName("Character"));
	Character_Class         = Manager.RequestGameplayTag(FName("Character.Class"));
	Character_Class_Archer  = Manager.RequestGameplayTag(FName("Character.Class.Archer"));
	Character_Class_Mage    = Manager.RequestGameplayTag(FName("Character.Class.Mage"));
	Character_Class_Warrior = Manager.RequestGameplayTag(FName("Character.Class.Warrior"));
	Character_Enemy         = Manager.RequestGameplayTag(FName("Character.Enemy"));
	Character_Enemy_Boss    = Manager.RequestGameplayTag(FName("Character.Enemy.Boss"));
	Character_Enemy_Elite   = Manager.RequestGameplayTag(FName("Character.Enemy.Elite"));
	Character_Player        = Manager.RequestGameplayTag(FName("Character.Player"));

	Damage           = Manager.RequestGameplayTag(FName("Damage"));
	Damage_Fire      = Manager.RequestGameplayTag(FName("Damage.Fire"));
	Damage_Holy      = Manager.RequestGameplayTag(FName("Damage.Holy"));
	Damage_Ice       = Manager.RequestGameplayTag(FName("Damage.Ice"));
	Damage_Lightning = Manager.RequestGameplayTag(FName("Damage.Lightning"));
	Damage_Physical  = Manager.RequestGameplayTag(FName("Damage.Physical"));
	Damage_Shadow    = Manager.RequestGameplayTag(FName("Damage.Shadow"));
	Damage_True      = Manager.RequestGameplayTag(FName("Damage.True"));

	Effect          = Manager.RequestGameplayTag(FName("Effect"));
	Effect_Buff     = Manager.RequestGameplayTag(FName("Effect.Buff"));
	Effect_Control  = Manager.RequestGameplayTag(FName("Effect.Control"));
	Effect_Debuff   = Manager.RequestGameplayTag(FName("Effect.Debuff"));
	Effect_Duration = Manager.RequestGameplayTag(FName("Effect.Duration"));
	Effect_Heal     = Manager.RequestGameplayTag(FName("Effect.Heal"));
	Effect_Instant  = Manager.RequestGameplayTag(FName("Effect.Instant"));
	Effect_Periodic = Manager.RequestGameplayTag(FName("Effect.Periodic"));
	Effect_Shield   = Manager.RequestGameplayTag(FName("Effect.Shield"));

	Event                = Manager.RequestGameplayTag(FName("Event"));
	Event_OnAbilityHit   = Manager.RequestGameplayTag(FName("Event.OnAbilityHit"));
	Event_OnCriticalHit  = Manager.RequestGameplayTag(FName("Event.OnCriticalHit"));
	Event_OnDamageDealt  = Manager.RequestGameplayTag(FName("Event.OnDamageDealt"));
	Event_OnDeath        = Manager.RequestGameplayTag(FName("Event.OnDeath"));
	Event_OnHeal         = Manager.RequestGameplayTag(FName("Event.OnHeal"));
	Event_OnKill         = Manager.RequestGameplayTag(FName("Event.OnKill"));
	Event_OnRevive       = Manager.RequestGameplayTag(FName("Event.OnRevive"));
	Event_OnShieldBroken = Manager.RequestGameplayTag(FName("Event.OnShieldBroken"));

	Input                  = Manager.RequestGameplayTag(FName("Input"));
	Input_Ability          = Manager.RequestGameplayTag(FName("Input.Ability"));
	Input_Ability_Basic    = Manager.RequestGameplayTag(FName("Input.Ability.Basic"));
	Input_Ability_Core     = Manager.RequestGameplayTag(FName("Input.Ability.Core"));
	Input_Ability_Defense  = Manager.RequestGameplayTag(FName("Input.Ability.Defense"));
	Input_Ability_Mobility = Manager.RequestGameplayTag(FName("Input.Ability.Mobility"));
	Input_Ability_Passive  = Manager.RequestGameplayTag(FName("Input.Ability.Passive"));
	Input_Ability_Status   = Manager.RequestGameplayTag(FName("Input.Ability.Status"));
	Input_Ability_Ultimate = Manager.RequestGameplayTag(FName("Input.Ability.Ultimate"));
	Input_Dash             = Manager.RequestGameplayTag(FName("Input.Dash"));
	Input_Interact         = Manager.RequestGameplayTag(FName("Input.Interact"));
	Input_Move             = Manager.RequestGameplayTag(FName("Input.Move"));

	State            = Manager.RequestGameplayTag(FName("State"));
	State_Burning    = Manager.RequestGameplayTag(FName("State.Burning"));
	State_Dead       = Manager.RequestGameplayTag(FName("State.Dead"));
	State_Frozen     = Manager.RequestGameplayTag(FName("State.Frozen"));
	State_Invincible = Manager.RequestGameplayTag(FName("State.Invincible"));
	State_Poisoned   = Manager.RequestGameplayTag(FName("State.Poisoned"));
	State_Silenced   = Manager.RequestGameplayTag(FName("State.Silenced"));
	State_Slowed     = Manager.RequestGameplayTag(FName("State.Slowed"));
	State_Stunned    = Manager.RequestGameplayTag(FName("State.Stunned"));
	State_SuperArmor = Manager.RequestGameplayTag(FName("State.SuperArmor"));
	State_Taunted    = Manager.RequestGameplayTag(FName("State.Taunted"));
}

FGameplayTag FESGameplayTags::FindTag(const FName& TagName)
{
	UGameplayTagsManager& Manager = UGameplayTagsManager::Get();
	FGameplayTag Tag = Manager.RequestGameplayTag(TagName, false);
	if (!Tag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[FESGameplayTags::FindTag] Tag not found: %s"), *TagName.ToString());
	}
	return Tag;
}
