#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Character/ESEnemyBase.h"
#include "ESSpawnWaveConfig.generated.h"

/**
 * 单波怪物的配置数据
 */
USTRUCT(BlueprintType)
struct FESSingleWaveConfig
{
	GENERATED_BODY()

	/** 这一波要生成的总怪物数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	int32 TotalMonstersInWave = 10;

	/** 这一波的怪物权重配置（Key:怪物类, Value:权重） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wave")
	TMap<TSubclassOf<AESEnemyBase>, float> MonsterWeights;
};

/**
 * 游戏难度枚举
 */
UENUM(BlueprintType)
enum class EESGameDifficulty : uint8
{
	Easy		UMETA(DisplayName = "简单 (1.0x)"),
	Normal		UMETA(DisplayName = "普通 (1.0x)"),
	Hard		UMETA(DisplayName = "困难 (1.3x)"),
	Hell		UMETA(DisplayName = "地狱 (1.6x)")
};

/**
 * 副本刷怪配置总表 (DataAsset)
 * 在内容浏览器右键 -> 杂项 -> Data Asset -> 选择 ESSpawnWaveConfig 创建
 */
UCLASS()
class ETERNALSANCTUARY_API UESSpawnWaveConfig : public UDataAsset
{
	GENERATED_BODY()

public:
	// ==================== 基础配置 ====================
	/** 游戏难度（影响怪物属性） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Base")
	EESGameDifficulty Difficulty = EESGameDifficulty::Normal;

	/** 波次之间的等待时间（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Base", meta = (MinValue = 1.0f))
	float WaveInterval = 5.0f;

	/** 场上同时最多存活多少只怪物 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Base", meta = (MinValue = 1))
	int32 MaxAliveLimit = 8;

	/** 管理器每隔多久检查一次（秒） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Base")
	float CheckInterval = 0.5f;

	// ==================== 波次配置 ====================
	/** 每一波的具体配置（数组长度=总波数） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Config|Waves")
	TArray<FESSingleWaveConfig> WaveConfigs;

	// ==================== 辅助函数 ====================
	/** 获取难度系数（简单=1.0, 困难=1.3...） */
	UFUNCTION(BlueprintPure, Category = "Config")
	float GetDifficultyMultiplier() const;
};