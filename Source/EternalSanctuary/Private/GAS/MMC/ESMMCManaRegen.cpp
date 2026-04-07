// Private/GAS/MMC/ESMMCManaRegen.cpp
#include "GAS/MMC/ESMMCManaRegen.h"
#include "GAS/AS/ESAttributeSet.h"

UESMMCManaRegen::UESMMCManaRegen()
{
    MaxManaDef = FGameplayEffectAttributeCaptureDefinition(
        UESAttributeSet::GetMaxManaAttribute(),
        EGameplayEffectAttributeCaptureSource::Target,
        false
    );
    RelevantAttributesToCapture.Add(MaxManaDef);
}

float UESMMCManaRegen::CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const
{
    FAggregatorEvaluateParameters EvalParams;
    EvalParams.TargetTags = Spec.CapturedTargetTags.GetAggregatedTags();

    float MaxMana = 0.f;
    GetCapturedAttributeMagnitude(MaxManaDef, Spec, EvalParams, MaxMana);
    MaxMana = FMath::Max(0.f, MaxMana);

    static const FGameplayTag ManaPercentTag = 
        FGameplayTag::RequestGameplayTag(FName("Regen.ManaPercent"), false);
    float RegenPercent = Spec.GetSetByCallerMagnitude(ManaPercentTag, false, 0.f);

    static const FGameplayTag FlatManaTag = 
        FGameplayTag::RequestGameplayTag(FName("Regen.FlatMana"), false);
    float FlatRegen = Spec.GetSetByCallerMagnitude(FlatManaTag, false, 0.f);

    const float ManaRegen = (MaxMana * RegenPercent) + FlatRegen;

    UE_LOG(LogTemp, Verbose, 
        TEXT("[MMCManaRegen] MaxMana=%.1f Percent=%.2f Flat=%.2f Final=%.1f"),
        MaxMana, RegenPercent, FlatRegen, ManaRegen);

    return ManaRegen;
}
