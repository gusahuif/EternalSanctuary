#pragma once

#include "CoreMinimal.h"
#include "GAS/GA/ESGameplayAbilityBase.h"
#include "ESHunterInstinct.generated.h"

/**
 * 猎手本能 - 弓箭手被动技能
 * 
 * 增伤：箭矢每飞行超过10米，伤害提升15%，最多4层（最高60%）
 * 减伤：与攻击者距离每超过5米，受到伤害降低5%，最多4层（最高20%）
 * 
 * 架构说明：
 *   - 增伤通过投射物碰撞时检查被动Tag，动态设置 SetByCaller
 *   - 减伤通过 AttributeSet::PreGameplayEffectExecute 检查被动Tag
 *   - 被动GA本身只负责：授予Tag、提供配置参数
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ETERNALSANCTUARY_API UESHunterInstinct : public UESGameplayAbilityBase
{
	GENERATED_BODY()

public:
	UESHunterInstinct();

	// ── 增伤配置 ──

	/** 每层增伤所需的飞行距离（米） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|HunterInstinct|Offensive")
	float DistancePerStack = 10.0f;

	/** 每层增伤百分比（0.15 = 15%） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|HunterInstinct|Offensive")
	float DamageBonusPerStack = 0.15f;

	/** 最大增伤层数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|HunterInstinct|Offensive")
	int32 MaxOffensiveStacks = 4;

	// ── 减伤配置 ──

	/** 每层减伤所需的距离（米） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|HunterInstinct|Defensive")
	float DefenderDistancePerStack = 5.0f;

	/** 每层减伤百分比（0.05 = 5%） */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|HunterInstinct|Defensive")
	float DamageReductionPerStack = 0.05f;

	/** 最大减伤层数 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|HunterInstinct|Defensive")
	int32 MaxDefensiveStacks = 4;

		// ── 被动身份Tag ──

		/** 获取猎手本能被动Tag */
		static FGameplayTag GetHunterInstinctTag();

	// ── 被动技能通用接口实现 ──
	virtual float GetPassiveDamageBonus(float FlightDistanceMeters) const override;
	virtual float GetPassiveDamageReduction(float AttackerDistanceMeters) const override;

protected:
	virtual void ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
								  const FGameplayAbilityActorInfo* ActorInfo, 
								  const FGameplayAbilityActivationInfo ActivationInfo, 
								  const FGameplayEventData* TriggerEventData) override;

	// 被动技能跳过蓝量/CD检查
	virtual bool CanActivateAbility(
		const FGameplayAbilitySpecHandle Handle,
		const FGameplayAbilityActorInfo* ActorInfo,
		const FGameplayTagContainer* SourceTags,
		const FGameplayTagContainer* TargetTags,
		FGameplayTagContainer* OptionalRelevantTags) const override;

	virtual void EndAbility(const FGameplayAbilitySpecHandle Handle, 
							 const FGameplayAbilityActorInfo* ActorInfo, 
							 const FGameplayAbilityActivationInfo ActivationInfo, 
							 bool bReplicateEndAbility, bool bWasCancelled) override;

private:
	/** 被动激活时授予的Tag */
	FGameplayTag GrantedTag;

	/** 是否已授予Tag */
	bool bTagGranted = false;
};
