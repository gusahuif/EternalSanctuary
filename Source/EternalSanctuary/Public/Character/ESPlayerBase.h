#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "ESCharacterBase.h"
#include "ESPlayerBase.generated.h"

class USpringArmComponent;
class UCameraComponent;
/**
 * 命令状态机 - 定义玩家角色当前正在执行的主命令
 */
UENUM(BlueprintType)
enum class ECommandState : uint8
{
	Idle UMETA(DisplayName = "Idle"),
	PathMove UMETA(DisplayName = "PathMove"),
	HoldMove UMETA(DisplayName = "HoldMove"),
	Attack UMETA(DisplayName = "Attack"),
	Cast UMETA(DisplayName = "Cast"),
	Dead UMETA(DisplayName = "Dead"),
};

UCLASS()
class ETERNALSANCTUARY_API AESPlayerBase : public AESCharacterBase
{
	GENERATED_BODY()

public:
	AESPlayerBase();

protected:
	virtual void BeginPlay() override;

public:
	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;


	// -------------------------------------------------------
	// 摄像机
	// -------------------------------------------------------

	/** 摄像机弹簧臂 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Camera")
	TObjectPtr<USpringArmComponent> CameraBoom;

	/** 跟随摄像机 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Camera")
	TObjectPtr<UCameraComponent> FollowCamera;


	// -------------------------------------------------------
	// 命令状态机 - 公开接口
	// -------------------------------------------------------

	/** 统一状态切换入口，返回是否切换成功 */
	UFUNCTION(BlueprintCallable, Category = "ES|StateMachine")
	bool ChangeCommandState(ECommandState NewState);

	/** 查询当前状态 */
	UFUNCTION(BlueprintPure, Category = "ES|StateMachine")
	ECommandState GetCurrentState() const { return CommandState; }

	/** 快速判断是否处于某状态 */
	UFUNCTION(BlueprintPure, Category = "ES|StateMachine")
	bool IsInState(ECommandState State) const { return CommandState == State; }

	/** 判断当前是否可以接受移动命令 */
	UFUNCTION(BlueprintPure, Category = "ES|StateMachine")
	bool CanAcceptMoveCommand() const;

	// -------------------------------------------------------
	// 移动接口
	// -------------------------------------------------------

	/**
	 * 执行路径移动（点击地面寻路）
	 * @param Waypoints Controller 算好的路径点序列
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|Move")
	void ExecutePathMove(const TArray<FVector>& Waypoints);

	/**
	 * 更新长按移动方向（每帧由 Controller 调用）
	 * @param WorldDirection 归一化的世界空间方向向量
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|Move")
	void UpdateHoldMoveDirection(const FVector& WorldDirection);

	/** 进入长按移动状态 */
	UFUNCTION(BlueprintCallable, Category = "ES|Move")
	void EnterHoldMove();

	/** 停止一切移动，回到 Idle */
	UFUNCTION(BlueprintCallable, Category = "ES|Move")
	void StopAllMovement();

	/** 停止长按移动，兼容旧接口（等价于 StopAllMovement） */
	UFUNCTION(BlueprintCallable, Category = "ES|Move")
	void StopHoldMove();

	/** 修改当前的打断状态（供 AnimNotifyState 调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|StateMachine")
	void SetInterruptible(bool bNewInterruptible) { bCanBeInterrupted = bNewInterruptible; }

	void StopCurrentAnimation(float BlendOutTime = 0.2f);
	
	// -------------------------------------------------------
	// 战斗接口
	// -------------------------------------------------------

	/**
	 * 设置攻击目标
	 * 在范围内直接切 Attack，否则切 PathMove 追击
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	void SetAttackTarget(AActor* NewTarget);

	/** 清除攻击目标 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	void ClearAttackTarget();

	/** 获取当前攻击目标 */
	UFUNCTION(BlueprintPure, Category = "ES|Combat")
	AActor* GetAttackTarget() const { return CurrentAttackTarget; }

	/** 尝试执行普通攻击（由 TickAttack 或输入直接调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	void TriggerBasicAttack();

	/** 强制朝指定位置攻击（Shift+ 左键） */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	void AttackAtLocation(const FVector& TargetLocation);

	/** 获取当前攻击射程 */
	UFUNCTION(BlueprintPure, Category = "ES|Combat")
	float GetAttackRange() const { return AttackRange; }

	// -------------------------------------------------------
	// 技能接口
	// -------------------------------------------------------

	/**
	 * 开始施法
	 * @param bInterruptible 该技能是否可被移动/攻击打断
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|Skill")
	void BeginCast(bool bInterruptible = false);

	/** 结束施法，回到 Idle */
	UFUNCTION(BlueprintCallable, Category = "ES|Skill")
	void EndCast();

	
	// -------------------------------------------------------
	// 死亡接口
	// -------------------------------------------------------

	/** 触发死亡状态，之后不再响应任何命令 */
	UFUNCTION(BlueprintCallable, Category = "ES|Character")
	void TriggerDead();

public:
	/** 药水逻辑 */
	UFUNCTION(BlueprintCallable, Category = "ES|Action")
	void UsePotion();

	/** 闪避逻辑 */
	UFUNCTION(BlueprintCallable, Category = "ES|Action")
	void ExecuteDodge();

	/** 技能激活逻辑 */
	UFUNCTION(BlueprintCallable, Category = "ES|Action")
	void ActivateSkill(int32 Slot);

protected:
	/** 判断是否允许从当前状态切换到目标状态 */
	bool CanEnterState(ECommandState NewState) const;

	/** Tick 中按状态分发逻辑 */
	void TickStateMachine(float DeltaTime);

	/** Tick 中执行 PathMove 路径跟随 */
	void TickPathMove(float DeltaTime);

	/** Tick 中执行 HoldMove 持续移动 */
	void TickHoldMove(float DeltaTime);

	/** Tick 中执行 Attack 追击/出手 */
	void TickAttack(float DeltaTime);

private:
	// -------------------------------------------------------
	// 状态机数据
	// -------------------------------------------------------

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|StateMachine", meta = (AllowPrivateAccess = "true"))
	ECommandState CommandState = ECommandState::Idle;

	/** Cast 状态下，当前技能是否允许被打断 */
	bool bCanBeInterrupted = false;

	// -------------------------------------------------------
	// 移动数据
	// -------------------------------------------------------

	/** PathMove 路径点序列（由 Controller 传入） */
	TArray<FVector> PathWaypoints;

	/** PathMove 当前目标路径点索引 */
	int32 CurrentWaypointIndex = 0;

	/** PathMove 到达路径点的判定距离 */
	UPROPERTY(EditAnywhere, Category = "ES|Move", meta = (AllowPrivateAccess = "true"))
	float WaypointAcceptanceRadius = 50.0f;

	/** HoldMove 当前移动方向 */
	FVector HoldMoveDirection = FVector::ZeroVector;

	// -------------------------------------------------------
	// 战斗数据
	// -------------------------------------------------------

	/** 当前攻击目标 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Combat", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<AActor> CurrentAttackTarget = nullptr;

	/** 普通攻击范围 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Combat", meta = (AllowPrivateAccess = "true"))
	float AttackRange = 800.0f;
};
