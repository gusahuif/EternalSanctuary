#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Common/SpawnPoint/ESSpawnWaveConfig.h"
#include "ESMonsterSpawnManagerComponent.generated.h"

// 前置声明
class AESEnemyBase;

// 委托：当所有波次完成且怪物全清时广播（游戏胜利）
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnAllWavesCompleted);

/**
 * 挂在GameMode上的刷怪管理器（重构版）
 * 功能：
 * 1. 支持 DataAsset 配置波次
 * 2. 支持波次间隔等待
 * 3. 支持怪物属性成长
 * 4. 胜利委托通知
 */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent), Blueprintable)
class ETERNALSANCTUARY_API UESMonsterSpawnManagerComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UESMonsterSpawnManagerComponent();

protected:
	// 组件被激活时调用（相当于 BeginPlay）
	virtual void BeginPlay() override;

public:
	// 每帧调用（我们这里不用，用定时器）
	virtual void TickComponent(float DeltaTime, ELevelTick TickType,
	                           FActorComponentTickFunction* ThisTickFunction) override;

	// ==========================================
	// PUBLIC 函数（供外部调用）
	// ==========================================
public:
	/**
	 * 初始化刷怪系统（必须在游戏开始时调用）
	 * @param InSpawnConfig 刷怪配置 DataAsset
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|MonsterSpawn")
	void InitializeSpawnSystem(UESSpawnWaveConfig* InSpawnConfig);

	/**
	 * 手动开始下一波（通常由系统自动调用，也可手动跳过等待）
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|MonsterSpawn")
	void StartNextWave();

	/**
	 * 获取当前是第几波（从1开始）
	 */
	UFUNCTION(BlueprintPure, Category = "ES|MonsterSpawn")
	int32 GetCurrentWaveNumber() const { return CurrentWaveIndex + 1; }

	/**
	 * 获取总波数
	 */
	UFUNCTION(BlueprintPure, Category = "ES|MonsterSpawn")
	int32 GetTotalWaveCount() const;

	/**
	 * 获取当前波次还剩多少只怪没刷
	 */
	UFUNCTION(BlueprintPure, Category = "ES|MonsterSpawn")
	int32 GetRemainingMonstersInCurrentWave() const;

	/**
	 * 获取当前场上存活的怪物数量
	 */
	UFUNCTION(BlueprintPure, Category = "ES|MonsterSpawn")
	int32 GetCurrentAliveCount() const { return CurrentAliveMonsters.Num(); }

	// ==========================================
	// PUBLIC 变量
	// ==========================================
public:
	/** 生成点的类型（场景里手动摆放的Actor类） */
	UPROPERTY(EditAnywhere, Category = "ES|Spawn")
	TSubclassOf<AActor> SpawnPointClass;

	/** 所有波次完成且怪物全清时广播（生成传送门、发奖励等） */
	UPROPERTY(BlueprintAssignable, Category = "ES|Spawn|Delegates")
	FOnAllWavesCompleted OnAllWavesCompleted;

	// ==========================================
	// PROTECTED 函数
	// ==========================================
protected:
	/** 【核心循环】每 CheckInterval 秒执行一次的检查函数 */
	void CheckAndSpawnMonster();

	/** 清理掉已经死亡的怪物引用 */
	void CleanupDeadMonsters();

	/** 获取玩家位置（用于排序刷怪点） */
	FVector GetPlayerLocation() const;

	/** 将刷怪点按“距离玩家从近到远”排序 */
	void SortSpawnPointsByDistance(TArray<AActor*>& InOutPoints) const;

	/** 在指定刷怪点生成一只怪（内部会根据权重选怪并应用属性） */
	void SpawnMonsterAtPoint(AActor* SpawnPoint);

	/** 给刷怪点生成一个新的随机冷却时间 */
	void RefreshPointCooldown(AActor* SpawnPoint);

	/** 根据权重随机选择一个怪物类 */
	TSubclassOf<AESEnemyBase> SelectMonsterByWeight() const;

	/** 检查游戏胜利条件 */
	void CheckVictoryCondition();

	/** 波次结束，开始等待计时 */
	void EnterWaveInterval();

	// ==========================================
	// PROTECTED 变量
	// ==========================================
protected:
	/** 当前使用的刷怪配置 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Spawn|Data")
	UESSpawnWaveConfig* SpawnConfig;

	/** 世界里所有的刷怪点 */
	TArray<AActor*> SpawnPoints;

	/** 当前场上活着的怪物 */
	TArray<AESEnemyBase*> CurrentAliveMonsters;

	/** 当前波次索引（从0开始） */
	int32 CurrentWaveIndex = 0;

	/** 当前波次已经刷了多少只 */
	int32 SpawnedCountInCurrentWave = 0;

	/** 记录每个刷怪点的剩余冷却时间 */
	TMap<AActor*, float> SpawnPointCooldowns;

	/** 主循环定时器句柄 */
	FTimerHandle CheckTimerHandle;

	/** 波次间隔等待定时器句柄 */
	FTimerHandle WaveIntervalTimerHandle;

	/** 标记是否已经触发过胜利，防止重复触发 */
	bool bHasWon = false;

	/** 标记是否正在波次间隔等待中 */
	bool bIsInWaveInterval = false;

	// ==========================================
	// PRIVATE 函数
	// ==========================================
private:
	// (目前无私有函数)

	// ==========================================
	// PRIVATE 变量
	// ==========================================
private:
	// (目前无私有变量)
};