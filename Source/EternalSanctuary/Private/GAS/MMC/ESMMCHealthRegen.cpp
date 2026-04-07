// Private/GAS/MMC/ESMMCHealthRegen.cpp
#include "GAS/MMC/ESMMCHealthRegen.h"
#include "GAS/AS/ESAttributeSet.h"
#include "AbilitySystemComponent.h"

UESMMCHealthRegen::UESMMCHealthRegen()
{
    // 捕获 MaxHealth 属性（Target 端）
    MaxHealthDef = FGameplayEffectAttributeCaptureDefinition(
        UESAttributeSet::GetMaxHealthAttribute(),
        EGameplayEffectAttributeCaptureSource::Target,
        false  // bSnapshot: false = 使用实时值
    );

    // 注册捕获声明
    RelevantAttributesToCapture.Add(MaxHealthDef);
}

float UESMMCHealthRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    // 构建属性捕获参数
    FAggregatorEvaluateParameters EvalParams;
    EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    // ── 读取 MaxHealth ──
    float MaxHealth = 0.f;
    GetCapturedAttributeMagnitude(MaxHealthDef, Spec, EvalParams, MaxHealth);
    MaxHealth = FMath::Max(0.f, MaxHealth);

    // ── 读取 SetByCaller Tag：Regen.HealthPercent ──
    static const FGameplayTag HealthPercentTag = 
        FGameplayTag::RequestGameplayTag(FName("Regen.HealthPercent"), false);
    float RegenPercent = 0.f;
    if (HealthPercentTag.IsValid())
    {
        RegenPercent = Spec.GetSetByCallerMagnitude(HealthPercentTag, false, 0.f);
    }

    // ── 读取 SetByCaller Tag：Regen.FlatHealth ──
    static const FGameplayTag FlatHealthTag = 
        FGameplayTag::RequestGameplayTag(FName("Regen.FlatHealth"), false);
    float FlatRegen = 0.f;
    if (FlatHealthTag.IsValid())
    {
        FlatRegen = Spec.GetSetByCallerMagnitude(FlatHealthTag, false, 0.f);
    }

    // ── 计算最终回复量 ──
    const float HealthRegen = (MaxHealth * RegenPercent) + FlatRegen;

    // 输出 Debug 日志
    UE_LOG(LogTemp, Verbose, 
        TEXT("[MMCHealthRegen] MaxHealth=%.1f Percent=%.2f Flat=%.2f Final=%.1f"),
        MaxHealth, RegenPercent, FlatRegen, HealthRegen);

    return HealthRegen;  // 支持负值（DOT）
}
