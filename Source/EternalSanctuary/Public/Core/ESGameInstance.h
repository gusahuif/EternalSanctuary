#pragma once

#include "CoreMinimal.h"
#include "Engine/GameInstance.h"
#include "ESGameInstance.generated.h"

/**
 * UESGameInstance
 * 项目全局 GameInstance
 * 负责生命周期最长的全局初始化逻辑，包括 GameplayTag 静态缓存初始化
 */
UCLASS()
class ETERNALSANCTUARY_API UESGameInstance : public UGameInstance
{
	GENERATED_BODY()

public:
	UESGameInstance();

public:
	/**
	 * 游戏启动时的全局初始化入口
	 * 在此处完成项目级别的单次初始化工作
	 */
	virtual void Init() override;

	/**
	 * 游戏关闭时的清理入口
	 */
	virtual void Shutdown() override;

	// ==================== 关卡切换数据存储 ====================
	/** 下一个要加载的目标关卡名称 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Level")
	FName NextLevelName;

	// ==================== 核心函数 ====================
	/** 设置下一个要加载的关卡（在主城NPC处调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|Level")
	void SetNextLevel(FName LevelName);

	/** 获取下一个要加载的关卡（在LoadingMap调用） */
	UFUNCTION(BlueprintPure, Category = "ES|Level")
	FName GetNextLevel() const { return NextLevelName; }
};
