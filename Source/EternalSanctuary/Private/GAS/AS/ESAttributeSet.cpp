// Private/GAS/AS/ESAttributeSet.cpp

#include "GAS/AS/ESAttributeSet.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "GameplayEffectExtension.h"
#include "GameplayTagContainer.h"
#include "Character/ESCharacterBase.h"
#include "Character/ESPlayerBase.h"
#include "GameFramework/Actor.h"

UESAttributeSet::UESAttributeSet()
{
}


void UESAttributeSet::PreAttributeChange(const FGameplayAttribute& Attribute, float& NewValue)
{
	Super::PreAttributeChange(Attribute, NewValue);

	// 生命/法力/护盾的运行时约束
	if (Attribute == GetHealthAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetManaAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxMana());
	}
	else if (Attribute == GetShieldAttribute())
	{
		NewValue = FMath::Clamp(NewValue, 0.0f, GetMaxHealth());
	}
	else if (Attribute == GetMaxHealthAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetMaxManaAttribute())
	{
		NewValue = FMath::Max(0.0f, NewValue);
	}
	else if (Attribute == GetDamageReductionAttribute())
	{
		// 百分比减伤限制在 0~0.9，避免出现100%免伤导致数值失控
		NewValue = FMath::Clamp(NewValue, 0.0f, 0.9f);
	}
	else if (Attribute == GetCooldownReductionAttribute())
	{
		// CDR 限制在 0~0.9
		NewValue = FMath::Clamp(NewValue, 0.0f, 0.9f);
	}
	else if (Attribute == GetCritRateAttribute())
	{
		// 暴击率限制在 0~1
		NewValue = FMath::Clamp(NewValue, 0.0f, 1.0f);
	}
}

bool UESAttributeSet::PreGameplayEffectExecute(FGameplayEffectModCallbackData& Data)
{
	Super::PreGameplayEffectExecute(Data);

	// 无敌穿透逻辑：
	// 当目标拥有 State.Invincible Tag 且本次修改的是 IncomingDamage 时，直接清空伤害并跳过后续伤害结算
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		UAbilitySystemComponent* TargetASC = Data.Target.AbilityActorInfo.IsValid()
			? Data.Target.AbilityActorInfo->AbilitySystemComponent.Get()
			: nullptr;

		if (TargetASC)
		{
			const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(FName(TEXT("State.Invincible")), false);
			if (InvincibleTag.IsValid() && TargetASC->HasMatchingGameplayTag(InvincibleTag))
			{
				Data.EvaluatedData.Magnitude = 0.0f;
				SetIncomingDamage(0.0f);
				return false;
			}

			// ── 被动技能减伤（通用接口，不感知具体被动）──
				AActor* TargetAvatar = Data.Target.AbilityActorInfo.IsValid()
					? Data.Target.AbilityActorInfo->AvatarActor.Get()
					: nullptr;
				AESPlayerBase* TargetPlayer = Cast<AESPlayerBase>(TargetAvatar);
				if (TargetPlayer)
				{
					AActor* InstigatorActor = Data.EffectSpec.GetContext().GetOriginalInstigator();
					if (!InstigatorActor)
					{
						InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
					}
					if (InstigatorActor)
					{
						const float DistanceCm = FVector::Dist(TargetAvatar->GetActorLocation(), InstigatorActor->GetActorLocation());
						const float DistanceMeters = DistanceCm / 100.0f;
						const float Reduction = TargetPlayer->CalculatePassiveDamageReduction(DistanceMeters);
						if (Reduction > 0.f)
						{
							float CurrentDamage = Data.EvaluatedData.Magnitude;
							CurrentDamage *= (1.0f - Reduction);
							Data.EvaluatedData.Magnitude = CurrentDamage;
							
							UE_LOG(LogTemp, Warning, 
								TEXT("[PassiveReduction] 距离=%.1fm, 减伤=%.0f%%, 剩余伤害=%.1f"),
								DistanceMeters, Reduction * 100.f, CurrentDamage);
						}
					}
				}
			}
		}

	return true;
}

void UESAttributeSet::PostGameplayEffectExecute(const FGameplayEffectModCallbackData& Data)
{
	Super::PostGameplayEffectExecute(Data);

	AActor* TargetActor = nullptr;
	AActor* InstigatorActor = nullptr;

	if (Data.Target.AbilityActorInfo.IsValid())
	{
		TargetActor = Data.Target.AbilityActorInfo->AvatarActor.Get();
	}

	if (Data.EffectSpec.GetContext().GetOriginalInstigator())
	{
		InstigatorActor = Data.EffectSpec.GetContext().GetOriginalInstigator();
	}
	else if (Data.EffectSpec.GetContext().GetInstigator())
	{
		InstigatorActor = Data.EffectSpec.GetContext().GetInstigator();
	}

	// 1. 处理伤害结算
	if (Data.EvaluatedData.Attribute == GetIncomingDamageAttribute())
	{
		const float RawIncomingDamage = GetIncomingDamage();
		SetIncomingDamage(0.0f);

		// 如果目标已经死亡，直接跳过所有伤害处理
		if (TargetActor)
		{
			AESCharacterBase* TargetChar = Cast<AESCharacterBase>(TargetActor);
			if (TargetChar && !TargetChar->IsAlive())
			{
				UE_LOG(LogTemp, Log, TEXT("目标已死亡，跳过伤害处理"));
				return;
			}
		}
		// ==========================================================================
		
		if (RawIncomingDamage > 0.0f)
		{
			const float FinalDamage = RawIncomingDamage;

			if (FinalDamage > 0.0f)
			{
				const float OldShield = GetShield();
				const float OldHealth = GetHealth();

				float RemainingDamage = FinalDamage;

				// 先扣护盾
				if (OldShield > 0.0f)
				{
					const float DamageToShield = FMath::Min(OldShield, RemainingDamage);
					const float NewShield = FMath::Clamp(OldShield - DamageToShield, 0.0f, GetMaxHealth());
					SetShield(NewShield);
					RemainingDamage -= DamageToShield;

					// 护盾从大于0变成0，派发护盾破碎事件
					if (OldShield > 0.0f && NewShield <= 0.0f && TargetActor)
					{
						const FGameplayTag ShieldBrokenTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Event.OnShieldBroken")), false);
						if (ShieldBrokenTag.IsValid())
						{
							SendGameplayEventToTarget(TargetActor, ShieldBrokenTag, InstigatorActor, TargetActor);
						}
					}
				}

				// 护盾不够时再扣生命
				float ActualHealthDamage = 0.0f;
				if (RemainingDamage > 0.0f)
				{
					ActualHealthDamage = FMath::Min(GetHealth(), RemainingDamage);
					const float NewHealth = FMath::Clamp(GetHealth() - RemainingDamage, 0.0f, GetMaxHealth());
					SetHealth(NewHealth);
				}

				// 只要造成了实际伤害（包括扣护盾），就派发受伤/伤害事件
				if (FinalDamage > 0.0f && TargetActor)
				{
					const FGameplayTag OnDamageDealtTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Event.OnDamageDealt")), false);
					if (OnDamageDealtTag.IsValid())
					{
						SendGameplayEventToTarget(TargetActor, OnDamageDealtTag, InstigatorActor, TargetActor);
					}
				}

				// =============== 直接在这里判断并触发受击 ===============
				if (TargetActor)
				{
					AESCharacterBase* TargetChar = Cast<AESCharacterBase>(TargetActor);
					if (TargetChar && TargetChar->IsAlive())
					{
						// 1. 先判断是不是DOT伤害
						const FGameplayTag DotDamageTag = FGameplayTag::RequestGameplayTag(FName("Damage.Type.DOT"), false);
						bool bIsDotDamage = false;
					
						FGameplayEffectSpec& Spec = const_cast<FGameplayEffectSpec&>(Data.EffectSpec);
						FGameplayTagContainer AssetTags;
						Spec.GetAllAssetTags(AssetTags);
						bIsDotDamage = AssetTags.HasTag(DotDamageTag);

						// 2. 只有非DOT伤害才触发受击
						if (!bIsDotDamage)
						{
							TargetChar->HandleHitReact(InstigatorActor);
						}
					}
				}
				// ======================================================================

				// 生命降到0时，派发死亡事件
				if (OldHealth > 0.0f && GetHealth() <= 0.0f && TargetActor)
				{
					const FGameplayTag OnDeathTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Event.OnDeath")), false);
					if (OnDeathTag.IsValid())
					{
						SendGameplayEventToTarget(TargetActor, OnDeathTag, InstigatorActor, TargetActor);
					}

					// 击杀事件发给伤害来源，且目标不能是自己
					if (InstigatorActor && InstigatorActor != TargetActor)
					{
						const FGameplayTag OnKillTag = FGameplayTag::RequestGameplayTag(FName(TEXT("Event.OnKill")), false);
						if (OnKillTag.IsValid())
						{
							SendGameplayEventToTarget(InstigatorActor, OnKillTag, InstigatorActor, TargetActor);
						}
					}
				}
				// ==========================================
				// 【修复】显示伤害数字
				// ==========================================
				if (TargetActor && FinalDamage > 0.0f)
				{
					AESCharacterBase* TargetChar = Cast<AESCharacterBase>(TargetActor);
					if (TargetChar)
					{
						// 1. 尝试获取受击点位置
						FVector HitLocation = FVector::ZeroVector;
						if (Data.EffectSpec.GetContext().GetHitResult())
						{
							HitLocation = Data.EffectSpec.GetContext().GetHitResult()->ImpactPoint;
						}

						// 2. 【新增】检查是否暴击（读取MMC加的Tag）
						bool bWasCrit = false;
						FGameplayTag CritTag = FGameplayTag::RequestGameplayTag(FName("Damage.WasCrit"));
						if (CritTag.IsValid())
						{
							bWasCrit = Data.EffectSpec.DynamicGrantedTags.HasTag(CritTag);
						}

						// 3. 调用显示函数
						TargetChar->ShowDamageNumber(FinalDamage, bWasCrit, HitLocation);
					}
				}
			}
		}
	}
	// 2. 处理治疗结算
	else if (Data.EvaluatedData.Attribute == GetIncomingHealAttribute())
	{
		const float HealAmount = GetIncomingHeal();
		SetIncomingHeal(0.0f);

		if (HealAmount > 0.0f)
		{
			const float NewHealth = FMath::Clamp(GetHealth() + HealAmount, 0.0f, GetMaxHealth());
			SetHealth(NewHealth);
		}
	}
	// 3. 处理护盾结算
	else if (Data.EvaluatedData.Attribute == GetIncomingShieldAttribute())
	{
		const float ShieldAmount = GetIncomingShield();
		SetIncomingShield(0.0f);

		if (ShieldAmount > 0.0f)
		{
			// 实际护盾 = IncomingShield * (1 + ShieldBonus)
			const float FinalShieldGain = ShieldAmount * (1.0f + GetShieldBonus());
			const float NewShield = FMath::Clamp(GetShield() + FinalShieldGain, 0.0f, GetMaxHealth());
			SetShield(NewShield);
		}
	}
	
	// 4. 对 Health / Mana / Shield 做最终 Clamp
	ClampVitalAttributes();
}

void UESAttributeSet::PostAttributeChange(const FGameplayAttribute& Attribute, float OldValue, float NewValue)
{
	Super::PostAttributeChange(Attribute, OldValue, NewValue);

	// 【核心逻辑】当最大生命值增加时，同步增加当前生命值
	if (Attribute == GetMaxHealthAttribute())
	{
		const float HealthDelta = NewValue - OldValue;
		if (HealthDelta > 0.f) // 只有增加时才同步（减少时不同步，避免死亡后回血）
		{
			// 直接加上增量，并确保不超过新的最大生命值
			const float NewHealth = FMath::Clamp(GetHealth() + HealthDelta, 0.f, NewValue);
			SetHealth(NewHealth);
		}
	}
}

void UESAttributeSet::SendGameplayEventToTarget(
	AActor* TargetActor,
	const FGameplayTag& EventTag,
	AActor* InstigatorActor,
	AActor* OptionalTargetActor) const
{
	if (!TargetActor || !EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = InstigatorActor;
	EventData.Target = OptionalTargetActor ? OptionalTargetActor : TargetActor;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(TargetActor, EventTag, EventData);
}

void UESAttributeSet::ClampVitalAttributes()
{
	SetHealth(FMath::Clamp(GetHealth(), 0.0f, GetMaxHealth()));
	SetMana(FMath::Clamp(GetMana(), 0.0f, GetMaxMana()));
	SetShield(FMath::Clamp(GetShield(), 0.0f, GetMaxHealth()));
}
