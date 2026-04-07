// Public/GAS/MMC/ESMMCManaRegen.h
#pragma once

#include "CoreMinimal.h"
#include "GameplayModMagnitudeCalculation.h"
#include "ESMMCManaRegen.generated.h"

UCLASS()
class ETERNALSANCTUARY_API UESMMCManaRegen : public UGameplayModMagnitudeCalculation
{
    GENERATED_BODY()

public:
    UESMMCManaRegen();
    virtual float CalculateBaseMagnitude_Implementation(const FGameplayEffectSpec& Spec) const override;

private:
    FGameplayEffectAttributeCaptureDefinition MaxManaDef;
};
