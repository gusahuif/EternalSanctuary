#pragma once
#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "GameplayTagContainer.h"
#include "Abilities/GameplayAbility.h"
#include "ESAbilityMetaData.generated.h"

// 符文数据（单个符文的配置）
USTRUCT(BlueprintType)
struct FES_RuneData
{
    GENERATED_BODY()

    // 符文唯一 Tag（用于激活检测）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
    FGameplayTag RuneTag;

    // 符文名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
    FText RuneName;

    // 符文描述
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
    FText RuneDescription;

    // 符文图标
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
    UTexture2D* RuneIcon = nullptr;

    // 符文槽位索引 (1, 2, 3)
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Rune")
    int32 RuneSlot = 1;
};

// 技能元数据（存储所有可视化/配置类数据）
USTRUCT(BlueprintType)
struct FES_SkillMetaData : public FTableRowBase
{
    GENERATED_BODY()

    // -------------------------------------------------------
    // 基础信息
    // -------------------------------------------------------

    // 技能唯一ID（主键，必须唯一）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|Base")
    FName SkillID = NAME_None;

    // 技能名称
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|Base")
    FText SkillName;

    // 技能描述
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|Base")
    FText SkillDesc;

    // 技能图标
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|Base")
    UTexture2D* SkillIcon = nullptr;

    // -------------------------------------------------------
    // GAS 逻辑
    // -------------------------------------------------------

    // 对应 GAS 的 GameplayTag（用于激活技能）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|GAS")
    FGameplayTag SkillTag;

    // 对应 GAS 的 Ability Class（实际的技能蓝图类）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|GAS")
    TSubclassOf<UGameplayAbility> AbilityClass;

    // 技能解锁 Tag（进副本后默认没有，需要解锁）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|GAS")
    FGameplayTag UnlockTag;

    // -------------------------------------------------------
    // 数值平衡
    // -------------------------------------------------------

    // 冷却时间（秒）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|Balance", meta = (MinValue = 0.0))
    float CooldownTime = 5.0f;

    // 法力消耗
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|Balance", meta = (MinValue = 0.0))
    float ManaCost = 0.0f;

    // -------------------------------------------------------
    // 符文系统
    // -------------------------------------------------------

    // 此技能支持的所有符文（最多3个）
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "SkillMeta|Runes")
    TArray<FES_RuneData> AvailableRunes;
};