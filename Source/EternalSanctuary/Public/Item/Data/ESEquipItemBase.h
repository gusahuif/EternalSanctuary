#pragma once

#include "CoreMinimal.h"
#include "Item/Data/ESItemBase.h"
#include "GameplayEffect.h"
#include "Item/Data/ESInventoryTypes.h"
#include "ESEquipItemBase.generated.h"

class UESAttributeSet;

/**
 * 装备专属基类（武器/防具/饰品），集成GAS属性与技能逻辑
 */
UCLASS(BlueprintType, Blueprintable)
class ETERNALSANCTUARY_API UESEquipItemBase : public UESItemBase
{
    GENERATED_BODY()

public:
    UESEquipItemBase();

	/** 主词条 (直接生效，无需解锁) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Equip|Entries")
	FESRuntimeEntry MainEntry;

	/** 副词条列表 (默认全锁，需按顺序解锁) */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Equip|Entries")
	TArray<FESRuntimeEntry> SubEntries;
	
    // ==================== 装备属性词条 ====================
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Equip|Stats")
    TArray<FESItemStat> StatList;

    // ==================== GAS 效果绑定 ====================
    /** 装备持续生效的GameplayEffect（无限持续，脱下自动移除） */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Equip|GAS")
    TSubclassOf<UGameplayEffect> EquipEffect;

    /** 装备授予的技能列表 */
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Equip|GAS")
    TArray<FESItemAbilityInfo> GrantAbilities;

    // ==================== 运行时数据（请勿手动修改） ====================
    /** 装备生效时的GE句柄（用于脱下时移除效果） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Equip|Runtime")
    FActiveGameplayEffectHandle EquipEffectHandle;

    /** 装备授予的技能句柄列表（用于脱下时移除技能） */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Equip|Runtime")
    TArray<FGameplayAbilitySpecHandle> GrantAbilityHandles;
};