#pragma once

#include "CoreMinimal.h"
#include "Character/ESCharacterBase.h"
#include "ESEnemyBase.generated.h"

class UESEnemyAISkillData;

UENUM(BlueprintType)
enum class EEnemyAIState : uint8
{
	Idle UMETA(DisplayName = "Idle"), // 待机
	Chase UMETA(DisplayName = "Chase"), // 追击
	Attack UMETA(DisplayName = "Attack"), // 攻击
	LeashReturn UMETA(DisplayName = "LeashReturn"), // 脱战返回
	Dead UMETA(DisplayName = "Dead") // 死亡
};

/** 仇恨条目：单个目标的仇恨值 */
USTRUCT(BlueprintType)
struct FAggroEntry
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* Target = nullptr;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float AggroValue = 0.0f;
};

/**
 * ESEnemyBase
 * 敌人角色基类，扩展敌人 AI 与战斗行为入口。
 */
UCLASS(Blueprintable)
class ETERNALSANCTUARY_API AESEnemyBase : public AESCharacterBase
{
	GENERATED_BODY()

public:
	AESEnemyBase();

protected:
	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Enemy")
	virtual void InitializeEnemyCharacter();

public:
	/** 设置当前目标 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual void SetCurrentTarget(AActor* NewTarget);

	/** 获取当前目标 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	AActor* GetCurrentTarget() const { return CurrentTarget.Get(); }

	/** 清空当前目标 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual void ClearCurrentTarget();

	/** 判断目标是否有效 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual bool HasValidTarget() const;

	/** 判断目标是否进入追击范围 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual bool IsTargetInChaseRange() const;

	/** 判断目标是否进入攻击范围 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual bool IsTargetInAttackRange() const;

	/** 判断是否可以立刻发起一次攻击 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual bool CanStartAttack() const;

	/** 攻击真正开始时调用，记录冷却和状态 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual void MarkAttackStarted();

	/** 攻击结束时调用，恢复状态 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual void MarkAttackFinished();

	/** 更新 AI 状态 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual void SetAIState(EEnemyAIState NewState);

	/** 是否应该脱战 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual bool ShouldLoseTarget() const;

	/** 是否已经回到出生点附近 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual bool IsNearSpawnLocation() const;

	//---------------------仇恨机制----------------------------------------
	/** 添加仇恨值 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI|仇恨")
	virtual void AddAggro(AActor* Target, float Value);

	/** 设置仇恨值（覆盖式，用于嘲讽） */
	UFUNCTION(BlueprintCallable, Category = "ES|AI|仇恨")
	virtual void SetAggro(AActor* Target, float Value);

	/** 清除某个目标的仇恨 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI|仇恨")
	virtual void ClearAggro(AActor* Target);

	/** 获取当前仇恨最高的目标（会自动处理嘲讽） */
	UFUNCTION(BlueprintCallable, Category = "ES|AI|仇恨")
	virtual AActor* GetCurrentAggroTarget();

	/** 判断是否有有效仇恨目标 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI|仇恨")
	virtual bool HasValidAggroTarget() const;
	//-------------------------------------------------------------------

	UFUNCTION(BlueprintCallable, Category = "ES|AI")
	virtual bool TryActivateMeleeAttackAbility();

	/** 角色死亡：默认添加 Dead Tag、停止移动、关闭基础移动能力 */
	virtual void Die() override;

	virtual void HandleHitReact(AActor* DamageCauser) override;

public:
	/** 当前锁定目标。通常由感知系统或 BT Service 更新 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AI")
	TObjectPtr<AActor> CurrentTarget;

	/** 敌人出生位置。用于脱战后返回 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AI")
	FVector SpawnLocation = FVector::ZeroVector;

	/** 进入战斗/开始追击的最大距离 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|AI")
	float ChaseRange = 1200.0f;

	/** 允许执行近战攻击的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|AI")
	float AttackRange = 180.0f;

	/** 攻击判定时额外的容错距离，避免边界抖动 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|AI")
	float AttackRangeTolerance = 30.0f;

	/** 丢失目标或脱战的最大距离 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|AI")
	float LoseTargetRange = 1800.0f;

	/** 返回出生点时认为“已归位”的距离 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|AI")
	float ReturnAcceptanceRadius = 80.0f;

	/** 攻击冷却时间，防止每帧都触发攻击 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|AI")
	float AttackCooldown = 1.5f;

	/** 上次攻击时间 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AI")
	float LastAttackTime = -1000.0f;

	/** 当前 AI 状态，供 BT / 动画 / 调试读取 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AI")
	EEnemyAIState AIState = EEnemyAIState::Idle;

	/** 是否正在执行攻击流程，避免重复下发攻击命令 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AI")
	bool bIsAttacking = false;

	/** 是否允许被感知系统自动切换目标 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|AI")
	bool bAllowSwitchTarget = true;

	// ----------------------仇恨机制----------------------
	/** 仇恨列表：目标 -> 仇恨值 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AI|仇恨")
	TMap<TObjectPtr<AActor>, float> AggroMap;

	/** 上次更新仇恨目标的时间，避免每帧计算 */
	float LastAggroUpdateTime = 0.0f;

	/** 缓存的当前仇恨目标 */
	UPROPERTY(Transient)
	TObjectPtr<AActor> CachedAggroTarget;
	// --------------------------------------------------------------

public:
	// ... 现有代码 ...

	// ---------------------- AI 技能系统扩展 ----------------------

	/** AI可用技能列表，在蓝图中配置 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|AI|技能配置")
	TArray<TObjectPtr<UESEnemyAISkillData>> AISkillList;

	/** 获取当前所有可用技能（CD就绪、距离符合、可行动） */
	UFUNCTION(BlueprintCallable, Category = "ES|AI|技能")
	TArray<UESEnemyAISkillData*> GetAvailableAISkills();

	/** 根据权重选择最优技能 */
	UFUNCTION(BlueprintCallable, Category = "ES|AI|技能")
	UESEnemyAISkillData* SelectBestSkillByWeight();

	/** 直接通过Tag激活技能（封装ASC的方法，方便BT调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|AI|技能")
	bool TryActivateAISkillByTag(FGameplayTag AbilityTag);

	/** 获取AI的最优追击位置（近战贴脸，远程保持距离） */
	UFUNCTION(BlueprintCallable, Category = "ES|AI|移动")
	FVector GetOptimalChaseLocation() const;
public:
	/**
	 * 用波次数据初始化怪物属性
	 * @param InDifficultyMult 难度系数 (1.0 / 1.3 / 1.6)
	 * @param InWaveMult 波次系数 (1.0 + 波次*0.1)
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|Enemy")
	virtual void InitializeWithWaveData(float InDifficultyMult, float InWaveMult);

protected:
	/** 缓存的难度系数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Enemy|Data")
	float CurrentDifficultyMult = 1.0f;

	/** 缓存的波次系数 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Enemy|Data")
	float CurrentWaveMult = 1.0f;

	// 用于缩放怪物属性的 GameplayEffect
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Enemy")
	TSubclassOf<UGameplayEffect> EnemyScalingGE;
	
protected:
	// 记录每个技能的上次释放时间，用于AI层CD校验
	UPROPERTY(Transient, VisibleAnywhere, Category = "ES|AI|技能")
	TMap<FGameplayTag, float> SkillLastUseTimeMap;
};
