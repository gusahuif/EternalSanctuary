#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "ESEnemyAISkillData.generated.h"

/**
 * AI技能配置数据资产
 * 用于定义每个AI技能的释放范围、权重曲线、冷却逻辑等
 */
UCLASS()
class ETERNALSANCTUARY_API UESEnemyAISkillData : public UDataAsset
{
	GENERATED_BODY()

public:
	// ---------------------- 基础配置 ----------------------

	/** 用于激活GA的技能Tag（必须与GA的AbilityTags匹配） */
	UPROPERTY(EditAnywhere, Category = "基础配置", meta = (DisplayName = "技能激活Tag"))
	FGameplayTag AbilityActivationTag;

	/** 技能类型标签（用于分类，如近战/远程/治疗/突进） */
	UPROPERTY(EditAnywhere, Category = "基础配置", meta = (DisplayName = "技能类型Tag"))
	FGameplayTag SkillTypeTag;

	/** 技能CD时间（秒），用于GAS冷却GE之外的AI层二次校验（可选） */
	UPROPERTY(EditAnywhere, Category = "基础配置", meta = (DisplayName = "技能冷却时间", MinValue = 0.0f))
	float CooldownTime = 1.0f;

	// ---------------------- 释放范围 ----------------------

	/** 技能最大释放距离（2D平面距离） */
	UPROPERTY(EditAnywhere, Category = "释放范围", meta = (DisplayName = "最大释放距离", MinValue = 0.0f))
	float MaxUseRange = 200.0f;

	/** 技能最小释放距离（例如突进技能需要离目标一定距离才释放） */
	UPROPERTY(EditAnywhere, Category = "释放范围", meta = (DisplayName = "最小释放距离", MinValue = 0.0f))
	float MinUseRange = 0.0f;

	// ---------------------- 权重配置 ----------------------

	/** 技能基础权重，数值越高越容易被AI选中 */
	UPROPERTY(EditAnywhere, Category = "权重配置", meta = (DisplayName = "基础权重", MinValue = 0.0f))
	float BaseWeight = 100.0f;

	/** 
	 * 距离权重曲线（可选）
	 * X轴：当前距离 / 最大释放距离（0.0 ~ 1.0）
	 * Y轴：权重倍率（0.0 ~ 2.0 为宜）
	 * 示例：
	 * - 近战技能：X越小Y越大（贴脸权重高）
	 * - 远程技能：X在0.6~0.8时Y最大（保持安全距离）
	 */
	UPROPERTY(EditAnywhere, Category = "权重配置", meta = (DisplayName = "距离权重曲线"))
	UCurveFloat* DistanceWeightCurve = nullptr;

	/** 
	 * 血量权重曲线（可选）
	 * X轴：自身当前血量百分比（0.0 ~ 1.0）
	 * Y轴：权重倍率
	 * 示例：治疗技能在X<0.3时Y大幅提升
	 */
	UPROPERTY(EditAnywhere, Category = "权重配置", meta = (DisplayName = "血量权重曲线"))
	UCurveFloat* HealthWeightCurve = nullptr;

	// ---------------------- 释放设置 ----------------------

	/** 释放技能前是否需要面向目标 */
	UPROPERTY(EditAnywhere, Category = "释放设置", meta = (DisplayName = "需要面向目标"))
	bool bNeedFaceTarget = true;

	/** 释放技能时是否停止移动 */
	UPROPERTY(EditAnywhere, Category = "释放设置", meta = (DisplayName = "释放时停止移动"))
	bool bStopMovementOnUse = true;

	/** 临时缓存的计算权重，仅用于BT决策帧 */
	UPROPERTY(Transient)
	float CachedWeight = 0.0f;
};
