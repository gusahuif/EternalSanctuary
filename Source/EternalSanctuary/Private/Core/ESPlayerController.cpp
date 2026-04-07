#include "Core/ESPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Character/ESPlayerBase.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"

AESPlayerController::AESPlayerController()
{
	bShowMouseCursor = true;
	bEnableClickEvents = false;
	bEnableMouseOverEvents = false;

	DefaultMouseCursor = EMouseCursor::Default;
	
}

void AESPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (ULocalPlayer* LocalPlayer = GetLocalPlayer())
	{
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>())
		{
			if (DefaultInputMappingContext)
			{
				InputSubsystem->AddMappingContext(DefaultInputMappingContext, 0);
			}
		}
	}
}

void AESPlayerController::SetupInputComponent()
{
	Super::SetupInputComponent();

	UEnhancedInputComponent* EnhancedInputComponent = Cast<UEnhancedInputComponent>(InputComponent);
	if (!EnhancedInputComponent)
	{
		return;
	}

	if (IA_LeftClick)
	{
		EnhancedInputComponent->BindAction(IA_LeftClick, ETriggerEvent::Started, this, &AESPlayerController::OnLeftClickStarted);
		EnhancedInputComponent->BindAction(IA_LeftClick, ETriggerEvent::Triggered, this, &AESPlayerController::OnLeftClickTriggered);
		EnhancedInputComponent->BindAction(IA_LeftClick, ETriggerEvent::Completed, this, &AESPlayerController::OnLeftClickReleased);
	}
	
	// 在已有的 IA_LeftClick 绑定下方添加：

	// 右键绑定
	if (IA_RightClick)
	{
		EnhancedInputComponent->BindAction(IA_RightClick, ETriggerEvent::Started, this, &AESPlayerController::OnRightClickStarted);
		EnhancedInputComponent->BindAction(IA_RightClick, ETriggerEvent::Triggered, this, &AESPlayerController::OnRightClickTriggered);
		EnhancedInputComponent->BindAction(IA_RightClick, ETriggerEvent::Completed, this, &AESPlayerController::OnRightClickReleased);
	}

	// 强制攻击 (Shift)
	if (IA_ForceAttack)
	{
		EnhancedInputComponent->BindAction(IA_ForceAttack, ETriggerEvent::Started, this, &AESPlayerController::OnForceAttackStarted);
		EnhancedInputComponent->BindAction(IA_ForceAttack, ETriggerEvent::Completed, this, &AESPlayerController::OnForceAttackCompleted);
	}

	// 药水、闪避、技能
	if (IA_Potion) EnhancedInputComponent->BindAction(IA_Potion, ETriggerEvent::Started, this, &AESPlayerController::OnPotionTriggered);
	if (IA_Dodge)  EnhancedInputComponent->BindAction(IA_Dodge, ETriggerEvent::Triggered, this, &AESPlayerController::OnDodgeTriggered);

	// 技能绑定（使用 Lambda 转发槽位号）
	if (IA_Skill1) EnhancedInputComponent->BindAction(IA_Skill1, ETriggerEvent::Started, this, &AESPlayerController::OnSkillTriggered, 1);
	if (IA_Skill2) EnhancedInputComponent->BindAction(IA_Skill2, ETriggerEvent::Started, this, &AESPlayerController::OnSkillTriggered, 2);
	if (IA_Skill3) EnhancedInputComponent->BindAction(IA_Skill3, ETriggerEvent::Started, this, &AESPlayerController::OnSkillTriggered, 3);
	if (IA_Skill4) EnhancedInputComponent->BindAction(IA_Skill4, ETriggerEvent::Started, this, &AESPlayerController::OnSkillTriggered, 4);

}

void AESPlayerController::PlayerTick(float DeltaTime)
{
	Super::PlayerTick(DeltaTime);

	if (bLeftClickPressed)
	{
		AESPlayerBase* PlayerCharacter = GetESPlayerCharacter();
		if (!PlayerCharacter) return;

		// 1. 强制攻击逻辑：Shift 按住时，每帧更新朝向并尝试攻击
		if (bIsForceAttackPressed)
		{
			FHitResult GroundHit;
			if (GetHitResultUnderCursorByChannel(GroundTraceChannel, true, GroundHit))
			{
				PlayerCharacter->AttackAtLocation(GroundHit.ImpactPoint);
			}
			return;
		}

		// 2. 攻击锁定逻辑：初始点中了敌人
		if (bIsInitialClickOnEnemy)
		{
			AActor* CurrentTarget = PlayerCharacter->GetAttackTarget();
			
			// 如果目标死了或无效，尝试扫描鼠标下是否有新目标（暗黑式自动切换）
			if (!IsTargetAlive(CurrentTarget))
			{
				RescanEnemyUnderCursor();
			}
			// 注意：只要有有效目标，PlayerBase 的 TickAttack 会自动处理追击和攻击循环
		}
		// 3. 移动锁定逻辑：初始点中了地面
		else if (bInHoldMode && bIsInitialClickOnGround)
		{
			HandleHoldAction();
		}
	}
}

/*void AESPlayerController::OnLeftClickStarted(const FInputActionValue& Value)
{
	bLeftClickPressed = true;
	bInHoldMode = false;
	LeftClickPressedTime = GetWorld()->GetTimeSeconds();

	AESPlayerBase* PlayerCharacter = GetESPlayerCharacter();
	if (!PlayerCharacter) return;

	// 初始判定：点下那一刻决定本次长按的“意图”
	if (bIsForceAttackPressed)
	{
		bIsInitialClickOnEnemy = false;
		bIsInitialClickOnGround = false;
		HandleTapAction(); // 执行强制攻击
	}
	else
	{
		FHitResult Hit;
		// 这里的 TargetTraceChannel 已经在你构造函数里改成了 ECC_GameTraceChannel1
		const bool bHitEnemy = GetHitResultUnderCursor(TargetTraceChannel, true, Hit);

		if (bHitEnemy && IsEnemyActor(Hit.GetActor()))
		{
			bIsInitialClickOnEnemy = true;
			bIsInitialClickOnGround = false;
			PlayerCharacter->SetAttackTarget(Hit.GetActor());
			
			// 如果在射程内，立即打一发
			if (FVector::Dist2D(PlayerCharacter->GetActorLocation(), Hit.ImpactPoint) <= PlayerCharacter->GetAttackRange())
			{
				PlayerCharacter->TriggerBasicAttack();
			}
		}
		else
		{
			bIsInitialClickOnEnemy = false;
			bIsInitialClickOnGround = true;
			HandleTapAction(); // 执行地面寻路移动
		}
	}
}
*/
void AESPlayerController::OnLeftClickStarted(const FInputActionValue& Value)
{
	bLeftClickPressed = true;
	bInHoldMode = false;
	LeftClickPressedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	bIsInitialClickOnEnemy = false;
	bIsInitialClickOnGround = false;

	AESPlayerBase* PlayerCharacter = GetESPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}

	FHitResult HitResult;
	const bool bHitSomething = GetHitResultUnderCursor(
		TargetTraceChannel,
		false,
		HitResult
	);

	AActor* HitActor = bHitSomething ? HitResult.GetActor() : nullptr;

	// 1. 优先处理“点敌人”
	if (HitActor && IsEnemyActor(HitActor) && IsTargetAlive(HitActor))
	{
		bIsInitialClickOnEnemy = true;

		// 业务：锁定当前攻击目标
		PlayerCharacter->SetAttackTarget(HitActor);

		// 先尝试触发槽位 0（LMB 槽）
		bool bActivatedSlotAbility = false;
		if (UESAbilitySystemComponent* ASC = PlayerCharacter->GetESAbilitySystemComponent())
		{
			bActivatedSlotAbility = ASC->TryActivateAbilityBySlotIndex(0);
		}

		// 如果槽位 0 没装技能，或当前不能激活，则回退到原有基础攻击逻辑
		if (!bActivatedSlotAbility)
		{
			PlayerCharacter->TriggerBasicAttack();
		}

		return;
	}

	// 2. 未点中敌人，则按“点地移动”处理
	FHitResult GroundHitResult;
	const bool bHitGround = GetHitResultUnderCursorByChannel(
		GroundTraceChannel,
		false,
		GroundHitResult
	);

	if (bHitGround)
	{
		bIsInitialClickOnGround = true;

		// 保持原有点击地面移动逻辑
		HandleTapAction();
	}
}

/*void AESPlayerController::OnLeftClickTriggered(const FInputActionValue& Value)
{
	if (!bLeftClickPressed) return;

	const float HeldTime = GetWorld()->GetTimeSeconds() - LeftClickPressedTime;

	if (!bInHoldMode && HeldTime >= HoldThreshold)
	{
		bInHoldMode = true;
		
		// 只有在移动锁定模式下才进入 HoldMove 状态
		if (bIsInitialClickOnGround)
		{
			if (AESPlayerBase* PlayerCharacter = GetESPlayerCharacter())
			{
				PlayerCharacter->EnterHoldMove();
			}
		}
	}
}

void AESPlayerController::OnLeftClickReleased(const FInputActionValue& Value)
{
	if (!bLeftClickPressed)
	{
		return;
	}

	if (bInHoldMode)
	{
		if (AESPlayerBase* PlayerCharacter = GetESPlayerCharacter())
		{
			PlayerCharacter->StopHoldMove();
		}
	}
	else
	{
		//HandleTapAction();
	}

	bLeftClickPressed = false;
	bInHoldMode = false;
	LeftClickPressedTime = 0.f;
}*/
void AESPlayerController::OnLeftClickTriggered(const FInputActionValue& Value)
{
	if (!bLeftClickPressed || bInHoldMode)
	{
		return;
	}

	if (!GetWorld())
	{
		return;
	}

	// 只有“初始点击在地面上”时，才允许进入长按移动
	if (!bIsInitialClickOnGround)
	{
		return;
	}

	const float HeldTime = GetWorld()->GetTimeSeconds() - LeftClickPressedTime;
	if (HeldTime >= HoldThreshold)
	{
		bInHoldMode = true;
		HandleHoldAction();
	}
}
void AESPlayerController::OnLeftClickReleased(const FInputActionValue& Value)
{
	if (bInHoldMode)
	{
		if (AESPlayerBase* PlayerCharacter = GetESPlayerCharacter())
		{
			PlayerCharacter->StopHoldMove();
		}
	}

	bLeftClickPressed = false;
	bInHoldMode = false;
	bIsInitialClickOnEnemy = false;
	bIsInitialClickOnGround = false;
	LeftClickPressedTime = 0.f;
}

void AESPlayerController::InterruptCurrentCommand()
{
	if (AESPlayerBase* PlayerCharacter = GetESPlayerCharacter())
	{
		//PlayerCharacter->StopAllMovement();
		PlayerCharacter->ClearAttackTarget();
	}
}

void AESPlayerController::HandleTapAction()
{
	AESPlayerBase* PlayerCharacter = GetESPlayerCharacter();
	if (!PlayerCharacter || !PlayerCharacter->CanAcceptMoveCommand())
	{
		return;
	}
	
	if (bIsForceAttackPressed)
	{
		// 如果按住 Shift，无论点哪都视为原地攻击（朝向鼠标点）
		FHitResult GroundHit;
		if (GetHitResultUnderCursorByChannel(GroundTraceChannel, true, GroundHit))
		{
			// 这里可以先用 SetAttackTarget(nullptr) 触发 Idle 攻击朝向
			// 或者调用一个新的接口：PlayerCharacter->AttackAtLocation(GroundHit.ImpactPoint);
			PlayerCharacter->AttackAtLocation(GroundHit.ImpactPoint);
		}
		return; 
	}
	
	// 第一步：先尝试检测是否点击到了敌对目标
	FHitResult TargetHitResult;
	const bool bHitTarget = GetHitResultUnderCursor(TargetTraceChannel, true, TargetHitResult);

	if (bHitTarget && TargetHitResult.GetActor() && IsEnemyActor(TargetHitResult.GetActor()))
	{
		PlayerCharacter->SetAttackTarget(TargetHitResult.GetActor());
		// 【新增】如果已经在攻击范围内，立即触发攻击
		const float Dist = FVector::Dist2D(
			PlayerCharacter->GetActorLocation(), 
			TargetHitResult.GetActor()->GetActorLocation()
		);
		if (Dist <= PlayerCharacter->GetAttackRange())
		{
			PlayerCharacter->TriggerBasicAttack();
		}
		return;
	}

	// 第二步：处理地面点击寻路
	FHitResult GroundHitResult;
	const bool bHitGround = GetHitResultUnderCursorByChannel(GroundTraceChannel, true, GroundHitResult);
	if (!bHitGround)
	{
		return;
	}

	UWorld* World = GetWorld();
	if (!World)
	{
		return;
	}

	UNavigationSystemV1* NavigationSystem = FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
	if (!NavigationSystem)
	{
		return;
	}

	FNavLocation ProjectedNavLocation;
	const bool bProjected = NavigationSystem->ProjectPointToNavigation(
		GroundHitResult.ImpactPoint,
		ProjectedNavLocation
	);

	if (!bProjected)
	{
		return;
	}

	UNavigationPath* NavigationPath = NavigationSystem->FindPathToLocationSynchronously(
		World,
		PlayerCharacter->GetActorLocation(),
		ProjectedNavLocation.Location,
		PlayerCharacter
	);

	if (!NavigationPath || NavigationPath->PathPoints.Num() == 0)
	{
		return;
	}

	PlayerCharacter->ClearAttackTarget();
	PlayerCharacter->ExecutePathMove(NavigationPath->PathPoints);
}

void AESPlayerController::HandleHoldAction()
{
	AESPlayerBase* PlayerCharacter = GetESPlayerCharacter();
	if (!PlayerCharacter || !PlayerCharacter->CanAcceptMoveCommand())
	{
		return;
	}

	FHitResult GroundHitResult;
	const bool bHitGround = GetHitResultUnderCursorByChannel(GroundTraceChannel, true, GroundHitResult);
	if (!bHitGround)
	{
		return;
	}

	FVector MoveDirection = GroundHitResult.ImpactPoint - PlayerCharacter->GetActorLocation();
	MoveDirection.Z = 0.f;

	if (MoveDirection.IsNearlyZero())
	{
		return;
	}

	PlayerCharacter->UpdateHoldMoveDirection(MoveDirection.GetSafeNormal());
}

bool AESPlayerController::IsEnemyActor(AActor* InActor) const
{
	if (!InActor)
	{
		return false;
	}
    
	// 方法 1：简单检查 - 如果 Actor 的 CollisionChannel 是 Enemy 则直接返回 true
	// 这需要敌人蓝图正确设置了 Collision Response
    
	// 方法 2：使用 LineTrace 验证（更可靠，推荐）
	FHitResult HitResult;
	const FVector Start = GetPawn()->GetActorLocation();
	const FVector End = InActor->GetActorLocation();
    
	FCollisionQueryParams QueryParams;
	QueryParams.AddIgnoredActor(GetPawn());
	QueryParams.bTraceComplex = false;
    
	// 使用 Enemy 通道（ECC_GameTraceChannel1 是第一个自定义通道）
	const bool bHit = GetWorld()->LineTraceSingleByChannel(
		HitResult,
		Start,
		End,
		ECC_GameTraceChannel1,  // ⚠️ 这就是你创建的 Enemy 通道
		QueryParams
	);
    
	// 如果 trace  hit 到了这个 Actor，说明它是敌人
	return bHit && (HitResult.GetActor() == InActor);
}

bool AESPlayerController::IsTargetAlive(AActor* InActor) const
{
	if (!InActor) return false;
	// 这里你需要根据你的敌人逻辑判断，比如是否有 Health 且 > 0
	// 暂时简单判断 Actor 是否正在被销毁
	AESCharacterBase* EnemyCharacter = Cast<AESCharacterBase>(InActor);
	if (!EnemyCharacter) return false;
	return EnemyCharacter->IsAlive();
}

void AESPlayerController::RescanEnemyUnderCursor()
{
	AESPlayerBase* PlayerCharacter = GetESPlayerCharacter();
	if (!PlayerCharacter) return;

	FHitResult Hit;
	if (GetHitResultUnderCursor(TargetTraceChannel, true, Hit))
	{
		if (IsEnemyActor(Hit.GetActor()))
		{
			PlayerCharacter->SetAttackTarget(Hit.GetActor());
			return;
		}
	}
	
	// 鼠标下没新敌人的话，清理目标，让玩家停下来
	PlayerCharacter->ClearAttackTarget();
}

void AESPlayerController::OnForceAttackStarted() { bIsForceAttackPressed = true; }
void AESPlayerController::OnForceAttackCompleted() { bIsForceAttackPressed = false; }

void AESPlayerController::OnPotionTriggered()
{
	if (AESPlayerBase* PlayerCharacter = GetESPlayerCharacter()) 
		PlayerCharacter->UsePotion(); // 需要在 Base 中声明
}

void AESPlayerController::OnDodgeTriggered()
{
	if (AESPlayerBase* PlayerCharacter = GetESPlayerCharacter())
		PlayerCharacter->ExecuteDodge(); // 需要在 Base 中声明
}

void AESPlayerController::OnSkillTriggered(int32 SkillSlot)
{
	// SkillSlot 传入 1~4
	// 槽位映射：
	// 1 -> SlotIndex 2
	// 2 -> SlotIndex 3
	// 3 -> SlotIndex 4
	// 4 -> SlotIndex 5
	if (SkillSlot < 1 || SkillSlot > 4)
	{
		return;
	}

	const int32 SlotIndex = SkillSlot + 1;

	if (AESPlayerBase* PlayerCharacter = GetESPlayerCharacter())
	{
		if (UESAbilitySystemComponent* ASC = PlayerCharacter->GetESAbilitySystemComponent())
		{
			ASC->TryActivateAbilityBySlotIndex(SlotIndex);
		}
	}
}


// 右键逻辑参考左键，但通常直接指向技能或重击，不触发寻路移动
void AESPlayerController::OnRightClickStarted(const FInputActionValue& Value)
{
	bRightClickPressed = true;
	bRightInHoldMode = false;
	RightClickPressedTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.f;

	// 右键作为主动技能输入，先打断当前移动/旧指令，保证手感干脆
	InterruptCurrentCommand();

	if (AESPlayerBase* PlayerCharacter = GetESPlayerCharacter())
	{
		if (UESAbilitySystemComponent* ASC = PlayerCharacter->GetESAbilitySystemComponent())
		{
			ASC->TryActivateAbilityBySlotIndex(1);
		}
	}
}
void AESPlayerController::OnRightClickTriggered(const FInputActionValue& Value) { /* 实现右键长按逻辑 */ }
void AESPlayerController::OnRightClickReleased(const FInputActionValue& Value)
{
	bRightClickPressed = false;
	bRightInHoldMode = false;
	RightClickPressedTime = 0.f;
}


AESPlayerBase* AESPlayerController::GetESPlayerCharacter() const
{
	return Cast<AESPlayerBase>(GetPawn());
}
