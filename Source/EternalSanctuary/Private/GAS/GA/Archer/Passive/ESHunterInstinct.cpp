#include "GAS/GA/Archer/Passive/ESHunterInstinct.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "GAS/GameplayTag/ESGameplayTags.h"
#include "AbilitySystemComponent.h"

UESHunterInstinct::UESHunterInstinct()
{
	// 被动技能配置
	InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
	NetExecutionPolicy = EGameplayAbilityNetExecutionPolicy::LocalPredicted;
	
	// 被动默认不需要输入
	InputTag = FGameplayTag::EmptyTag;
}

FGameplayTag UESHunterInstinct::GetHunterInstinctTag()
{
	static FGameplayTag CachedTag;
	if (!CachedTag.IsValid())
	{
		CachedTag = FGameplayTag::RequestGameplayTag(FName("Ability.Archer.HuntersInstinct.Passive"), false);
	}
	return CachedTag;
}

bool UESHunterInstinct::CanActivateAbility(
	const FGameplayAbilitySpecHandle Handle,
	const FGameplayAbilityActorInfo* ActorInfo,
	const FGameplayTagContainer* SourceTags,
	const FGameplayTagContainer* TargetTags,
	FGameplayTagContainer* OptionalRelevantTags) const
{
	// 被动技能跳过蓝量/CD检查，只做基础校验
	return Super::Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags);
}

void UESHunterInstinct::ActivateAbility(const FGameplayAbilitySpecHandle Handle, 
										  const FGameplayAbilityActorInfo* ActorInfo, 
										  const FGameplayAbilityActivationInfo ActivationInfo, 
										  const FGameplayEventData* TriggerEventData)
{
	// 被动技能不需要CommitAbility（无Cost无Cooldown无蓝量消耗）
	// 直接调Super::Super跳过ESGameplayAbilityBase的蓝量/CD逻辑
	Super::Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

	// 授予被动Tag（双重保险：EquipPassiveSkill也会授Tag）
	UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
	if (ASC)
	{
		GrantedTag = GetHunterInstinctTag();
		if (GrantedTag.IsValid())
		{
			if (!ASC->HasMatchingGameplayTag(GrantedTag))
			{
				ASC->AddLooseGameplayTag(GrantedTag);
			}
			bTagGranted = true;
			UE_LOG(LogTemp, Warning, TEXT("[HunterInstinct] GA激活成功，Tag: %s"), *GrantedTag.ToString());
		}
		else
		{
			UE_LOG(LogTemp, Error, TEXT("[HunterInstinct] Tag无效！检查DefaultGameplayTags.ini中是否添加了Ability.Archer.HuntersInstinct.Passive"));
		}
	}
}

void UESHunterInstinct::EndAbility(const FGameplayAbilitySpecHandle Handle, 
									 const FGameplayAbilityActorInfo* ActorInfo, 
									 const FGameplayAbilityActivationInfo ActivationInfo, 
									 bool bReplicateEndAbility, bool bWasCancelled)
{
	// 移除被动Tag
	if (bTagGranted)
	{
		UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo();
		if (ASC && GrantedTag.IsValid())
		{
			ASC->RemoveLooseGameplayTag(GrantedTag);
			bTagGranted = false;
			UE_LOG(LogTemp, Log, TEXT("[HunterInstinct] 被动已停用，移除Tag: %s"), *GrantedTag.ToString());
		}
	}

	Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}


// ── 被动技能通用接口实现 ──
float UESHunterInstinct::GetPassiveDamageBonus(float FlightDistanceMeters) const
{
	const int32 Stacks = FMath::Min(FMath::FloorToInt(FlightDistanceMeters / DistancePerStack), MaxOffensiveStacks);
	return Stacks * DamageBonusPerStack;
}

float UESHunterInstinct::GetPassiveDamageReduction(float AttackerDistanceMeters) const
{
	const int32 Stacks = FMath::Min(FMath::FloorToInt(AttackerDistanceMeters / DefenderDistancePerStack), MaxDefensiveStacks);
	return Stacks * DamageReductionPerStack;
}
