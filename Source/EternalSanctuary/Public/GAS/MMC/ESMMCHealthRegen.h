// Public/GAS/MMC/ESMMCHealthRegen.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ESMMCHealthRegen.generated.h"

/**
 * 回血 MMC
 * 
 * 公式：HealthRegen = (MaxHealth × RegenPercent) + FlatRegen
 * 
 * 参数来源：
 *   - MaxHealth：从 AttributeSet 捕获（Target 端）
 *   - RegenPercent：SetByCaller Tag "Regen.HealthPercent"（如 0.02 = 2%）
 *   - FlatRegen：SetByCaller Tag "Regen.FlatHealth"（如 10.0）
 * 
 * 支持负值（用于 DOT 持续伤害）
 */
UCLASS()
class ETERNALSANCTUARY_API UESMMCHealthRegen : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UESMMCHealthRegen();

    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    // MaxHealth 属性捕获声明（Target 端）
    FGameplayEffectAttributeCaptureDefinition MaxHealthDef;
};
