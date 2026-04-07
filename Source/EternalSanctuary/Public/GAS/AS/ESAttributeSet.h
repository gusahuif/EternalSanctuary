// Public/GAS/AS/ESAttributeSet.h

#pragma once

#include "CoreMinimal.h"
#include "AttributeSet.h"
#include "AbilitySystemComponent.h"
#include "ESAttributeSet.generated.h"

#define ATTRIBUTE_ACCESSORS(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_PROPERTY_GETTER(ClassName, PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_GETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_SETTER(PropertyName) \
GAMEPLAYATTRIBUTE_VALUE_INITTER(PropertyName)

/**
 * EternalSanctuary 通用 AttributeSet
 * 方案A：玩家与敌人共用同一套属性
 */
UCLASS()
class ETERNALSANCTUARY_API UESAttributeSet : public UAttributeSet
{
	GENERATED_BODY()

public:
	UESAttributeSet();

	virtual bool PreGameplayEffectExecute(struct FGameplayEffectModCallbackData& Data) override;
	virtual void PostGameplayEffectExecute(const struct FGameplayEffectModCallbackData& Data) override;

	// --------------------
	// Vital 组
	// --------------------

	// 当前生命值
	UPROPERTY(BlueprintReadOnly, Category = "ES|Vital")
	FGameplayAttributeData Health;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, Health)

	// 最大生命值
	UPROPERTY(BlueprintReadOnly, Category = "ES|Vital")
	FGameplayAttributeData MaxHealth;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, MaxHealth)

	// 当前护盾值，上限等于 MaxHealth
	UPROPERTY(BlueprintReadOnly, Category = "ES|Vital")
	FGameplayAttributeData Shield;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, Shield)

	// 护盾加成系数，0.2 = +20%
	UPROPERTY(BlueprintReadOnly, Category = "ES|Vital")
	FGameplayAttributeData ShieldBonus;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, ShieldBonus)

	// 当前法力值
	UPROPERTY(BlueprintReadOnly, Category = "ES|Vital")
	FGameplayAttributeData Mana;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, Mana)

	// 最大法力值
	UPROPERTY(BlueprintReadOnly, Category = "ES|Vital")
	FGameplayAttributeData MaxMana;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, MaxMana)

	// 每秒回血
	UPROPERTY(BlueprintReadOnly, Category = "ES|Vital")
	FGameplayAttributeData HealthRegenRate;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, HealthRegenRate)

	// 每秒回蓝
	UPROPERTY(BlueprintReadOnly, Category = "ES|Vital")
	FGameplayAttributeData ManaRegenRate;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, ManaRegenRate)

	// --------------------
	// Combat 组
	// --------------------

	// 攻击力
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData AttackPower;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, AttackPower)

	// 攻击速度倍率，只影响动画播放速率，不影响技能CD
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData AttackSpeed;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, AttackSpeed)

	// 固定减伤
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData Defense;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, Defense)

	// 百分比减伤，范围建议 0 ~ 0.9
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData DamageReduction;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, DamageReduction)

	// 全局增伤
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData DamageBonus;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, DamageBonus)

	// 基础技能增伤
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData BasicAbilityDamageBonus;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, BasicAbilityDamageBonus)

	// 核心技能增伤
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData CoreAbilityDamageBonus;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, CoreAbilityDamageBonus)

	// 机动技能增伤
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData MobilityAbilityDamageBonus;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, MobilityAbilityDamageBonus)

	// 状态/召唤类技能增伤
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData ConjurationAbilityDamageBonus;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, ConjurationAbilityDamageBonus)

	// 防御技能增伤
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData DefensiveAbilityDamageBonus;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, DefensiveAbilityDamageBonus)

	// 终极技能增伤
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData UltimateAbilityDamageBonus;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, UltimateAbilityDamageBonus)

	// 暴击率，0 ~ 1
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData CritRate;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, CritRate)

	// 暴击倍率，默认 1.5
	UPROPERTY(BlueprintReadOnly, Category = "ES|Combat")
	FGameplayAttributeData CritDamage;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, CritDamage)

	// --------------------
	// Utility 组
	// --------------------

	// 冷却缩减，范围建议 0 ~ 0.9，仅影响有CD技能
	UPROPERTY(BlueprintReadOnly, Category = "ES|Utility")
	FGameplayAttributeData CooldownReduction;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, CooldownReduction)

	// 移动速度
	UPROPERTY(BlueprintReadOnly, Category = "ES|Utility")
	FGameplayAttributeData MoveSpeed;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, MoveSpeed)

	// --------------------
	// Meta 组
	// 中间结算量，不持久化
	// --------------------

	// 原始伤害输入，PostGameplayEffectExecute 中结算后清零
	UPROPERTY(BlueprintReadOnly, Category = "ES|Meta")
	FGameplayAttributeData IncomingDamage;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, IncomingDamage)

	// 治疗量输入，PostGameplayEffectExecute 中结算后清零
	UPROPERTY(BlueprintReadOnly, Category = "ES|Meta")
	FGameplayAttributeData IncomingHeal;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, IncomingHeal)

	// 护盾量输入，PostGameplayEffectExecute 中结算后清零
	UPROPERTY(BlueprintReadOnly, Category = "ES|Meta")
	FGameplayAttributeData IncomingShield;
	ATTRIBUTE_ACCESSORS(UESAttributeSet, IncomingShield)

protected:
	// 属性修改前统一做 Clamp，避免非法值进入运行时
	virtual void PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue) override;

private:
	// 发送 GameplayEvent 到指定目标
	void SendGameplayEventToTarget(AActor* TargetActor, const FGameplayTag& EventTag, AActor* InstigatorActor, AActor* OptionalTargetActor = nullptr) const;

	// 对常用属性做统一 Clamp
	void ClampVitalAttributes();
};
