#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Engine/CurveTable.h"
#include "ESInGameProgressionComponent.generated.h"

// 局内升级事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnLevelUp, int32, NewLevel, int32, UpgradePointsGained);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnExperienceChanged, int32, CurrentExp, int32, ExpToNextLevel);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETERNALSANCTUARY_API UESInGameProgressionComponent : public UActorComponent
{
	GENERATED_BODY()

public:	
	UESInGameProgressionComponent();

protected:
	virtual void BeginPlay() override;

public:	
	// ==================== 配置参数 ====================
	/** 经验曲线表（RowName必须是Level_1, Level_2...） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|InGame|Config")
	UCurveTable* ExperienceCurveTable;

	/** 每级给的强化点数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|InGame|Config")
	int32 UpgradePointsPerLevel = 1;

	// ==================== 核心数据（只读） ====================
	/** 当前局内等级 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|InGame|Data")
	int32 CurrentLevel = 1;

	/** 当前经验值 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|InGame|Data")
	int32 CurrentExperience = 0;

	/** 当前强化点数量 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|InGame|Data")
	int32 CurrentUpgradePoints = 0;

	// ==================== 全局事件（UI绑定专用） ====================
	/** 升级时广播 */
	UPROPERTY(BlueprintAssignable, Category = "ES|InGame|Delegates")
	FOnLevelUp OnLevelUp;

	/** 经验变化时广播 */
	UPROPERTY(BlueprintAssignable, Category = "ES|InGame|Delegates")
	FOnExperienceChanged OnExperienceChanged;

	// ==================== 核心功能函数 ====================
	/** 增加经验值（击杀怪物调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|InGame|Progression")
	void AddExperience(int32 Amount);

	/** 获取升到下一级所需的经验值 */
	UFUNCTION(BlueprintPure, Category = "ES|InGame|Progression")
	int32 GetExpToNextLevel() const;

	/** 消耗强化点（解锁装备词条/技能调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|InGame|Progression")
	bool SpendUpgradePoints(int32 Amount);

	/** 增加强化点（特殊奖励调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|InGame|Progression")
	void AddUpgradePoints(int32 Amount);

	/** 重置局内数据（副本结束/回到主城调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|InGame|Progression")
	void ResetInGameData();

private:
	// ==================== 内部辅助函数 ====================
	/** 检查是否满足升级条件，满足则升级 */
	void CheckLevelUp();

	/** 从CurveTable读取指定等级所需的经验 */
	int32 GetRequiredExpForLevel(int32 Level) const;
};