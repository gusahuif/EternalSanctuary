// Public/GAS/MMC/ESMMCDamage.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ESMMCDamage.generated.h"

/**
 * 伤害 MMC
 * 
 * TODO:等级带入到公式中
 * 
 * 最终伤害公式：
 *   RawDamage = AttackPower * (1 + DamageBonus) * (1 + AbilityTypeBonus)
 *   DefenseMultiplier = 1 - Defense / (Defense + 100)
 *   FinalDamage = RawDamage * DefenseMultiplier * (1 - DamageReduction) * CritMultiplier * (1 + Vulnerable)
 * 
 * CritMultiplier:
 *   Random(0,1) < CritRate → CritDamage 倍率
 *   否则 → 1.0
 * 
 * 说明：
 *   - AbilityTypeBonus 通过 SetByCaller Tag 传入（可选）
 *   - Vulnerable 是目标身上的易伤 Debuff
 *   - 最终结果写入 IncomingDamage，由 PostGameplayEffectExecute 消费
 */
UCLASS()
class ETERNALSANCTUARY_API UESMMCDamage : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UESMMCDamage();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    // 攻击方属性捕获声明
    FGameplayEffectAttributeCaptureDefinition AttackPowerDef;
    FGameplayEffectAttributeCaptureDefinition DamageBonusDef;
    FGameplayEffectAttributeCaptureDefinition CritRateDef;
    FGameplayEffectAttributeCaptureDefinition CritDamageDef;

    // 防御方属性捕获声明
    FGameplayEffectAttributeCaptureDefinition DefenseDef;
    FGameplayEffectAttributeCaptureDefinition DamageReductionDef;
    FGameplayEffectAttributeCaptureDefinition VulnerableDef;
};
