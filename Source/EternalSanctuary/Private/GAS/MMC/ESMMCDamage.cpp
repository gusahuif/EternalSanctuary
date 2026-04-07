// Private/GAS/MMC/ESMMCDamage.cpp
#include "GAS/MMC/ESMMCDamage.h"
#include "GAS/AS/ESAttributeSet.h"
#include "AbilitySystemComponent.h"

UESMMCDamage::UESMMCDamage()
{
    // 捕获攻击方属性（Source）
    AttackPowerDef = FGameplayEffectAttributeCaptureDefinition(
        UESAttributeSet::GetAttackPowerAttribute(),
        EGameplayEffectAttributeCaptureSource::Source,
        false  // bSnapshot: false = 使用执行时的实时值
    );

    DamageBonusDef = FGameplayEffectAttributeCaptureDefinition(
        UESAttributeSet::GetDamageBonusAttribute(),
        EGameplayEffectAttributeCaptureSource::Source,
        false
    );

    CritRateDef = FGameplayEffectAttributeCaptureDefinition(
        UESAttributeSet::GetCritRateAttribute(),
        EGameplayEffectAttributeCaptureSource::Source,
        false
    );

    CritDamageDef = FGameplayEffectAttributeCaptureDefinition(
        UESAttributeSet::GetCritDamageAttribute(),
        EGameplayEffectAttributeCaptureSource::Source,
        false
    );

    // 捕获防御方属性（Target）
    DefenseDef = FGameplayEffectAttributeCaptureDefinition(
        UESAttributeSet::GetDefenseAttribute(),
        EGameplayEffectAttributeCaptureSource::Target,
        false
    );

    DamageReductionDef = FGameplayEffectAttributeCaptureDefinition(
        UESAttributeSet::GetDamageReductionAttribute(),
        EGameplayEffectAttributeCaptureSource::Target,
        false
    );

    // 注册所有捕获声明
    RelevantAttributesToCapture.Add(AttackPowerDef);
    RelevantAttributesToCapture.Add(DamageBonusDef);
    RelevantAttributesToCapture.Add(CritRateDef);
    RelevantAttributesToCapture.Add(CritDamageDef);
    RelevantAttributesToCapture.Add(DefenseDef);
    RelevantAttributesToCapture.Add(DamageReductionDef);
}

float UESMMCDamage::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    // 构建属性捕获参数
    FAggregatorEvaluateParameters EvalParams;
    EvalParams.SourceTags = Spec.CapturedSourceTags.GetAggregatedTags();
    EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    // ── 读取攻击方属性 ──
    float AttackPower = 0.f;
    GetCapturedAttributeMagnitude(AttackPowerDef, Spec, EvalParams, AttackPower);
    AttackPower = FMath::Max(0.f, AttackPower);

    float DamageBonus = 0.f;
    GetCapturedAttributeMagnitude(DamageBonusDef, Spec, EvalParams, DamageBonus);

    float CritRate = 0.f;
    GetCapturedAttributeMagnitude(CritRateDef, Spec, EvalParams, CritRate);
    CritRate = FMath::Clamp(CritRate, 0.f, 1.f);

    float CritDamageMultiplier = 1.5f;
    GetCapturedAttributeMagnitude(CritDamageDef, Spec, EvalParams, CritDamageMultiplier);

    // ── 读取防御方属性 ──
    float Defense = 0.f;
    GetCapturedAttributeMagnitude(DefenseDef, Spec, EvalParams, Defense);
    Defense = FMath::Max(0.f, Defense);

    float DamageReduction = 0.f;
    GetCapturedAttributeMagnitude(DamageReductionDef, Spec, EvalParams, DamageReduction);
    DamageReduction = FMath::Clamp(DamageReduction, 0.f, 0.9f);

    // ── 读取技能类型增伤（SetByCaller，可选）──
    // Tag: Damage.AbilityTypeBonus，不存在时默认 0
    static const FGameplayTag AbilityTypeBonusTag = 
        FGameplayTag::RequestGameplayTag(FName("Damage.AbilityTypeBonus"), false);
    float AbilityTypeBonus = 0.f;
    if (AbilityTypeBonusTag.IsValid())
    {
        AbilityTypeBonus = Spec.GetSetByCallerMagnitude(AbilityTypeBonusTag, false, 0.f);
    }

    // ── 伤害公式 ──
    // 1. 基础伤害（含增伤加成）
    const float RawDamage = AttackPower * (1.f + DamageBonus) * (1.f + AbilityTypeBonus);

    // 2. 防御减伤（D/(D+100) 软上限公式，防御越高减得越少）
    //    Defense=100 → 50% 减伤
    //    Defense=300 → 75% 减伤
    //    Defense=900 → 90% 减伤（已被 DamageReduction Clamp 到 0.9）
    const float DefenseMultiplier = 1.f - (Defense / (Defense + 100.f));

    // 3. 暴击判定（使用确定性随机，避免网络同步问题）
    const float CritRoll = FMath::FRand();
    const float CritMultiplier = (CritRoll < CritRate) ? CritDamageMultiplier : 1.f;

    // 4. 最终伤害
    const float FinalDamage = RawDamage * DefenseMultiplier * (1.f - DamageReduction) * CritMultiplier;

    // 输出 Debug 日志（打包时会自动去掉）
    UE_LOG(LogTemp, Verbose, 
        TEXT("[MMCDamage] ATK=%.1f Bonus=%.2f AbilityBonus=%.2f DEF=%.1f DR=%.2f CritRoll=%.3f/%.3f Final=%.1f"),
        AttackPower, DamageBonus, AbilityTypeBonus, Defense, DamageReduction, CritRoll, CritRate, FinalDamage);

    return FinalDamage;
}
