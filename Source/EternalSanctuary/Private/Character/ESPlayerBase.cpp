#include "Character/ESPlayerBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"

AESPlayerBase::AESPlayerBase()
{
	PrimaryActorTick.bCanEverTick = true;
	// 创建弹簧臂
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 800.0f;       // 摄像机距离，可在蓝图里调
	CameraBoom->bUsePawnControlRotation = false; // RTS 风格不跟随控制器旋转
	CameraBoom->SetRelativeRotation(FRotator(-60.0f, 0.0f, 0.0f)); // 俯视角

	// 创建摄像机
	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

}

void AESPlayerBase::BeginPlay()
{
	Super::BeginPlay();
}

void AESPlayerBase::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	TickStateMachine(DeltaTime);
}

void AESPlayerBase::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
}

// -------------------------------------------------------
// 状态机核心
// -------------------------------------------------------

bool AESPlayerBase::CanEnterState(ECommandState NewState) const
{
	// Dead 是终止态，拒绝一切切换
	if (CommandState == ECommandState::Dead)
	{
		return false;
	}

	// Cast 中：只有技能允许被打断时才能切换
	if (CommandState == ECommandState::Cast)
	{
		return bCanBeInterrupted;
	}

	return true;
}

bool AESPlayerBase::CanAcceptMoveCommand() const
{
	if (CommandState == ECommandState::Dead) return false;
	
	// 如果在施法，必须处于“可打断”窗口才能接受移动指令
	if (CommandState == ECommandState::Cast)
	{
		return bCanBeInterrupted;
	}
	
	return true;
}

bool AESPlayerBase::ChangeCommandState(ECommandState NewState)
{
	if (!CanEnterState(NewState))
	{
		return false;
	}

	// 【新增逻辑】如果我们正准备从 攻击/施法 切换到 移动/闲置，说明指令已生效，必须停止当前的动画
	if ((CommandState == ECommandState::Cast || CommandState == ECommandState::Attack) && 
		(NewState == ECommandState::PathMove || NewState == ECommandState::HoldMove || NewState == ECommandState::Idle))
	{
		StopCurrentAnimation();
	}

	CommandState = NewState;
	return true;
}

// -------------------------------------------------------
// Tick 状态分发
// -------------------------------------------------------

void AESPlayerBase::TickStateMachine(float DeltaTime)
{
	switch (CommandState)
	{
	case ECommandState::PathMove:
		TickPathMove(DeltaTime);
		break;
	case ECommandState::HoldMove:
		TickHoldMove(DeltaTime);
		break;
	case ECommandState::Attack:
		TickAttack(DeltaTime);
		break;
	default:
		break;
	}
}

void AESPlayerBase::TickPathMove(float DeltaTime)
{
	if (PathWaypoints.Num() == 0 || CurrentWaypointIndex >= PathWaypoints.Num())
	{
		// 路径走完，回到 Idle
		ChangeCommandState(ECommandState::Idle);
		return;
	}

	const FVector CurrentTarget = PathWaypoints[CurrentWaypointIndex];
	const FVector MyLocation = GetActorLocation();
	const float Dist = FVector::Dist2D(MyLocation, CurrentTarget);

	if (Dist <= WaypointAcceptanceRadius)
	{
		// 到达当前路径点，前进到下一个
		++CurrentWaypointIndex;
		if (CurrentWaypointIndex >= PathWaypoints.Num())
		{
			ChangeCommandState(ECommandState::Idle);
			return;
		}
	}

	// 计算朝向并移动
	const FVector Direction = (PathWaypoints[CurrentWaypointIndex] - MyLocation).GetSafeNormal2D();
	AddMovementInput(Direction, 1.0f);
}

void AESPlayerBase::TickHoldMove(float DeltaTime)
{
	if (!HoldMoveDirection.IsNearlyZero())
	{
		AddMovementInput(HoldMoveDirection, 1.0f);
	}
}

void AESPlayerBase::TickAttack(float DeltaTime)
{
	if (!IsValid(CurrentAttackTarget))
	{
		ClearAttackTarget();
		ChangeCommandState(ECommandState::Idle);
		return;
	}

	const float Dist = FVector::Dist2D(GetActorLocation(), CurrentAttackTarget->GetActorLocation());

	if (Dist > AttackRange)
	{
		// 追击
		TArray<FVector> ChaseWaypoints;
		ChaseWaypoints.Add(CurrentAttackTarget->GetActorLocation());
		ExecutePathMove(ChaseWaypoints);
		return;
	}

	// 在攻击范围内：面向目标
	FVector Direction = (CurrentAttackTarget->GetActorLocation() - GetActorLocation()).GetSafeNormal2D();
	if (!Direction.IsNearlyZero())
	{
		FRotator TargetRotation = Direction.Rotation();
		SetActorRotation(FRotator(0.f, TargetRotation.Yaw, 0.f));
	}
    
	// 【关键】检查技能是否正在播放（通过 ASC 查询）
	UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponent());
	if (ESASC)
	{
		static const FGameplayTag BasicSlotTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.Basic"));
        
		// 如果技能正在播放，不重复触发
		if (ESASC->IsAbilityActiveByTag(BasicSlotTag))
		{
			return;
		}
	}
    
	// 可以攻击
	TriggerBasicAttack();
}

// -------------------------------------------------------
// 移动接口实现
// -------------------------------------------------------

void AESPlayerBase::ExecutePathMove(const TArray<FVector>& Waypoints)
{
	if (!CanAcceptMoveCommand())
	{
		return;
	}

	if (Waypoints.Num() == 0)
	{
		return;
	}

	// 清理旧状态
	HoldMoveDirection = FVector::ZeroVector;

	// 写入路径数据
	PathWaypoints = Waypoints;
	CurrentWaypointIndex = 0;

	// 如果当前不是 PathMove，才切换状态（避免重复触发）
	if (!IsInState(ECommandState::PathMove))
	{
		ChangeCommandState(ECommandState::PathMove);
	}
}

void AESPlayerBase::UpdateHoldMoveDirection(const FVector& WorldDirection)
{
	if (!CanAcceptMoveCommand())
	{
		return;
	}

	const FVector FlatDirection = WorldDirection.GetSafeNormal2D();
	if (FlatDirection.IsNearlyZero())
	{
		return;
	}

	HoldMoveDirection = FlatDirection;

	// 如果还没进入 HoldMove 状态，先切换
	if (!IsInState(ECommandState::HoldMove))
	{
		EnterHoldMove();
	}
}

void AESPlayerBase::EnterHoldMove()
{
	if (!CanAcceptMoveCommand())
	{
		return;
	}

	// 停止路径寻路数据
	PathWaypoints.Empty();
	CurrentWaypointIndex = 0;

	ChangeCommandState(ECommandState::HoldMove);
}

void AESPlayerBase::StopAllMovement()
{
	// 清空路径数据
	PathWaypoints.Empty();
	CurrentWaypointIndex = 0;

	// 清空长按方向
	HoldMoveDirection = FVector::ZeroVector;

	// 立即停止物理移动
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
	}

	// 回到 Idle（Dead/Cast 中不强制覆盖）
	if (CommandState == ECommandState::PathMove || CommandState == ECommandState::HoldMove)
	{
		ChangeCommandState(ECommandState::Idle);
	}
}

void AESPlayerBase::StopHoldMove()
{
	StopAllMovement();
}

// -------------------------------------------------------
// 战斗接口实现
// -------------------------------------------------------

void AESPlayerBase::StopCurrentAnimation(float BlendOutTime)
{
	if (UAnimInstance* AnimInstance = GetMesh()->GetAnimInstance())
	{
		// 停止当前所有蒙太奇播放
		AnimInstance->Montage_Stop(BlendOutTime);
	}
}

void AESPlayerBase::SetAttackTarget(AActor* NewTarget)
{
	if (!IsValid(NewTarget))
	{
		ClearAttackTarget();
		return;
	}

	CurrentAttackTarget = NewTarget;

	// Cast 不可打断时，先记住目标，等技能结束后再处理
	if (CommandState == ECommandState::Cast && !bCanBeInterrupted)
	{
		return;
	}

	const float Dist = FVector::Dist2D(GetActorLocation(), CurrentAttackTarget->GetActorLocation());

	if (Dist <= AttackRange)
	{
		// 在攻击范围内，直接进入 Attack
		StopAllMovement();
		ChangeCommandState(ECommandState::Attack);
	}
	else
	{
		// 不在范围内，追击
		TArray<FVector> ChaseWaypoints;
		ChaseWaypoints.Add(CurrentAttackTarget->GetActorLocation());
		ExecutePathMove(ChaseWaypoints);
	}
}

void AESPlayerBase::ClearAttackTarget()
{
	CurrentAttackTarget = nullptr;
}

void AESPlayerBase::TriggerBasicAttack()
{
	if (IsInState(ECommandState::Dead) || !CanAcceptMoveCommand())
	{
		return;
	}

	// 检查是否有攻击目标
	if (!IsValid(CurrentAttackTarget))
	{
		return;
	}

	// 检查距离
	const float Dist = FVector::Dist2D(GetActorLocation(), CurrentAttackTarget->GetActorLocation());
	if (Dist > AttackRange)
	{
		return;
	}

	// 获取 ASC
	UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!ESASC)
	{
		return;
	}

	// 激活 Basic 槽位的技能
	static const FGameplayTag BasicSlotTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.Basic"));
	if (ESASC->IsAbilityActiveByTag(BasicSlotTag))
	{
		// 技能正在播放，忽略本次请求
		return;
	}
	
	// 方法 1：通过 Tag 激活（推荐）
	ESASC->TryActivateAbilityByTag(BasicSlotTag);

}

void AESPlayerBase::AttackAtLocation(const FVector& TargetLocation)
{
	if (IsInState(ECommandState::Dead))
	{
		return;
	}

	// 获取 ASC
	UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!ESASC)
	{
		return;
	}

	// 设置朝向
	FRotator TargetRotation = (TargetLocation - GetActorLocation()).ToOrientationRotator();
	TargetRotation.Pitch = 0.f;
	SetActorRotation(TargetRotation);

	// 激活技能
	static const FGameplayTag BasicSlotTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.Basic"));
	ESASC->TryActivateAbilityByTag(BasicSlotTag);
}

// -------------------------------------------------------
// 技能接口实现
// -------------------------------------------------------

void AESPlayerBase::BeginCast(bool bInterruptible)
{
	if (!ChangeCommandState(ECommandState::Cast))
	{
		return;
	}

	bCanBeInterrupted = bInterruptible;

	// 施法期间停止移动
	StopAllMovement();

	// 子类/技能系统在此之后负责播放动画、计算效果
}

void AESPlayerBase::EndCast()
{
	if (!IsInState(ECommandState::Cast))
	{
		return;
	}

	bCanBeInterrupted = false;
	ChangeCommandState(ECommandState::Idle);

	// 如果有缓存的攻击目标，施法结束后继续追击
	if (IsValid(CurrentAttackTarget))
	{
		SetAttackTarget(CurrentAttackTarget);
	}
}

// -------------------------------------------------------
// 死亡
// -------------------------------------------------------

void AESPlayerBase::TriggerDead()
{
	StopAllMovement();
	ClearAttackTarget();
	CommandState = ECommandState::Dead; // 直接赋值绕过 CanEnterState 检查
}

void AESPlayerBase::UsePotion()
{
	if (IsInState(ECommandState::Dead)) return;

	// 此处后续接入 GAS (Gameplay Ability System)
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, TEXT("使用了药水！"));
}

void AESPlayerBase::ExecuteDodge()
{
	if (!CanAcceptMoveCommand()) return;

	// 强制打断当前移动/攻击
	StopAllMovement();
	
	// 切换到 Cast 状态（不可打断），模拟闪避动作
	BeginCast(false); 
	
	// 这里可以添加 LaunchCharacter 或直接播放闪避动画
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, TEXT("执行了闪避！"));

	// 模拟闪避结束（实际应由动画通知调用 EndCast）
	FTimerHandle DodgeTimer;
	GetWorldTimerManager().SetTimer(DodgeTimer, this, &AESPlayerBase::EndCast, 0.4f, false);
}

void AESPlayerBase::ActivateSkill(int32 Slot)
{
	if (!CanAcceptMoveCommand()) return;

	FString SkillMsg = FString::Printf(TEXT("释放了技能 %d"), Slot);
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, SkillMsg);

	// 同样进入 Cast 状态
	//BeginCast(true); // 技能通常设为可被打断

	// 模拟技能释放完成
	//EndCast();
}
