#include "Core/ESPlayerController.h"

#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "NavigationPath.h"
#include "NavigationSystem.h"
#include "Character/ESPlayerBase.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "Item/Pickup/ESPickupBase.h"
#include "Kismet/GameplayStatics.h"
#include "GAS/GA/ESGameplayAbilityBase.h"

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
		if (UEnhancedInputLocalPlayerSubsystem* InputSubsystem = LocalPlayer->GetSubsystem<
			UEnhancedInputLocalPlayerSubsystem>())
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
		EnhancedInputComponent->BindAction(IA_LeftClick, ETriggerEvent::Started, this,
		                                   &AESPlayerController::OnSkillTriggered, ESkillSlotID::Slot_LeftClick);
		EnhancedInputComponent->BindAction(IA_LeftClick, ETriggerEvent::Completed, this,
										   &AESPlayerController::OnSkillReleased, ESkillSlotID::Slot_LeftClick);
		//EnhancedInputComponent->BindAction(IA_LeftClick, ETriggerEvent::Started, this, &AESPlayerController::OnLeftClickStarted);
		//EnhancedInputComponent->BindAction(IA_LeftClick, ETriggerEvent::Triggered, this, &AESPlayerController::OnLeftClickTriggered);
		//EnhancedInputComponent->BindAction(IA_LeftClick, ETriggerEvent::Completed, this, &AESPlayerController::OnLeftClickReleased);
	}

	// 在已有的 IA_LeftClick 绑定下方添加：

	// 右键绑定
	if (IA_RightClick)
	{
		EnhancedInputComponent->BindAction(IA_RightClick, ETriggerEvent::Started, this,
		                                   &AESPlayerController::OnSkillTriggered, ESkillSlotID::Slot_RightClick);
		EnhancedInputComponent->BindAction(IA_RightClick, ETriggerEvent::Completed, this,
										   &AESPlayerController::OnSkillReleased, ESkillSlotID::Slot_RightClick);
		//EnhancedInputComponent->BindAction(IA_RightClick, ETriggerEvent::Started, this, &AESPlayerController::OnRightClickStarted);
		//EnhancedInputComponent->BindAction(IA_RightClick, ETriggerEvent::Triggered, this, &AESPlayerController::OnRightClickTriggered);
		//EnhancedInputComponent->BindAction(IA_RightClick, ETriggerEvent::Completed, this, &AESPlayerController::OnRightClickReleased);
	}

	// 强制攻击 (Shift)
	if (IA_ForceAttack)
	{
		//EnhancedInputComponent->BindAction(IA_ForceAttack, ETriggerEvent::Started, this, &AESPlayerController::OnForceAttackStarted);
		//EnhancedInputComponent->BindAction(IA_ForceAttack, ETriggerEvent::Completed, this, &AESPlayerController::OnForceAttackCompleted);
	}

	// 药水、闪避、技能
	if (IA_Potion)
		EnhancedInputComponent->BindAction(IA_Potion, ETriggerEvent::Started, this,
		                                   &AESPlayerController::OnPotionTriggered);
	if (IA_Dodge)
		EnhancedInputComponent->BindAction(IA_Dodge, ETriggerEvent::Triggered, this,
		                                   &AESPlayerController::OnDodgeTriggered);

	// 技能绑定（使用 Lambda 转发槽位号）
	if (IA_Skill1)
	{
		EnhancedInputComponent->BindAction(IA_Skill1, ETriggerEvent::Started, this,
		                                   &AESPlayerController::OnSkillTriggered, ESkillSlotID::Slot_1);
		EnhancedInputComponent->BindAction(IA_Skill1, ETriggerEvent::Completed, this,
		&AESPlayerController::OnSkillReleased, ESkillSlotID::Slot_1);
	}
	if (IA_Skill2)
	{
		EnhancedInputComponent->BindAction(IA_Skill2, ETriggerEvent::Started, this,
		                                   &AESPlayerController::OnSkillTriggered, ESkillSlotID::Slot_2);
		EnhancedInputComponent->BindAction(IA_Skill2, ETriggerEvent::Completed, this,
		&AESPlayerController::OnSkillReleased, ESkillSlotID::Slot_2);
	}
	if (IA_Skill3)
	{
		EnhancedInputComponent->BindAction(IA_Skill3, ETriggerEvent::Started, this,
		                                   &AESPlayerController::OnSkillTriggered, ESkillSlotID::Slot_3);
		EnhancedInputComponent->BindAction(IA_Skill3, ETriggerEvent::Completed, this,
		&AESPlayerController::OnSkillReleased, ESkillSlotID::Slot_3);
	}
	if (IA_Skill4)
	{
		EnhancedInputComponent->BindAction(IA_Skill4, ETriggerEvent::Started, this,
		                                   &AESPlayerController::OnSkillTriggered, ESkillSlotID::Slot_4);
		EnhancedInputComponent->BindAction(IA_Skill4, ETriggerEvent::Completed, this,
		&AESPlayerController::OnSkillReleased, ESkillSlotID::Slot_4);
	}
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
		ECC_GameTraceChannel1, // ⚠️ 这就是你创建的 Enemy 通道
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

void AESPlayerController::OnSkillTriggered(ESkillSlotID SkillSlot)
{
	AESPlayerBase* PlayerCharacter = GetESPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}

	// 1. 获取 ASC 和 SkillTag
	UESAbilitySystemComponent* ESASC = PlayerCharacter->GetESAbilitySystemComponent();
	FGameplayTag SkillTag = PlayerCharacter->GetSkillTagBySlotID(SkillSlot);
	
	if (!ESASC || !SkillTag.IsValid())
	{
		return;
	}

	// ==========================================
	// 【核心修改 1】先读取 GA 配置：是否需要朝向鼠标
	// ==========================================
	bool bShouldFace = true; // 默认朝向
	
	TArray<FGameplayAbilitySpec*> FoundSpecs;
	ESASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		FGameplayTagContainer(SkillTag),
		FoundSpecs
	);

	if (FoundSpecs.Num() > 0 && FoundSpecs[0]->Ability)
	{
		UESGameplayAbilityBase* ESAbility = Cast<UESGameplayAbilityBase>(FoundSpecs[0]->Ability);
		if (ESAbility)
		{
			bShouldFace = ESAbility->bShouldFaceMouseOnPress;
		}
	}

	// ==========================================
	// 【核心修改 2】先尝试激活技能！
	// ==========================================
	bool bActivated = ESASC->TryActivateAbilityByTag(SkillTag);

	if (bActivated)
	{
		// ==========================================
		// 【核心修改 3】只有激活成功了，才执行后续逻辑
		// ==========================================
		
		// 通知 Character
		PlayerCharacter->OnSkillSlotTrigger(SkillSlot);

		// 如果 GA 配置了不需要朝向，直接返回
		if (!bShouldFace)
		{
			return;
		}

		// ==========================================
		// 下面是原有的朝向逻辑 (只有激活成功且需要朝向时才执行)
		// ==========================================

		// 获取鼠标下方的 Actor
		FHitResult HitResult;
		const bool bHitSomething = GetHitResultUnderCursor(
			TargetTraceChannel,
			false,
			HitResult
		);

		AActor* HitActor = bHitSomething ? HitResult.GetActor() : nullptr;

		// 1. 判断鼠标下是否有存活的敌人
		if (HitActor && IsEnemyActor(HitActor) && IsTargetAlive(HitActor))
		{
			// 有敌人：锁定目标并朝向敌人
			PlayerCharacter->SetAttackTarget(HitActor);

			// 获取敌人位置并计算朝向（仅 YAW）
			FVector EnemyLocation = HitActor->GetActorLocation();
			FVector ToEnemy = EnemyLocation - PlayerCharacter->GetActorLocation();
			ToEnemy.Z = 0.f; // 清除 Z 轴差值，只保留水平面方向

			if (!ToEnemy.IsNearlyZero())
			{
				// 计算目标 YAW 角度
				FRotator TargetRotation = ToEnemy.Rotation();
				TargetRotation.Pitch = 0.f; // 锁定 Pitch
				TargetRotation.Roll = 0.f; // 锁定 Roll

				// 设置角色朝向
				PlayerCharacter->SetActorRotation(TargetRotation);
			}
		}
		else
		{
			// 2. 没有敌人：朝向鼠标点击的地面方向
			FHitResult GroundHitResult;
			const bool bHitGround = GetHitResultUnderCursorByChannel(
				GroundTraceChannel,
				false,
				GroundHitResult
			);

			if (bHitGround)
			{
				FVector ToMouse = GroundHitResult.ImpactPoint - PlayerCharacter->GetActorLocation();
				ToMouse.Z = 0.f; // 清除 Z 轴，只旋转 YAW

				if (!ToMouse.IsNearlyZero())
				{
					FRotator TargetRotation = ToMouse.Rotation();
					TargetRotation.Pitch = 0.f;
					TargetRotation.Roll = 0.f;

					PlayerCharacter->SetActorRotation(TargetRotation);
				}
			}

			// 清理攻击目标（因为没有点中敌人）
			PlayerCharacter->ClearAttackTarget();
		}
	}
	else
	{
		// 【修改】激活失败（CD/没蓝），什么都不做，也不转向
		FString Msg = FString::Printf(TEXT("技能释放失败 (CD/法力不足)"));
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, Msg);
	}
}

void AESPlayerController::OnSkillReleased(ESkillSlotID SkillSlot)
{
	AESPlayerBase* PlayerCharacter = GetESPlayerCharacter();
	if (!PlayerCharacter)
	{
		return;
	}
	PlayerCharacter->OnSkillSlotComplate(SkillSlot);
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

void AESPlayerController::OnRightClickTriggered(const FInputActionValue& Value)
{
	/* 实现右键长按逻辑 */
}

void AESPlayerController::OnRightClickReleased(const FInputActionValue& Value)
{
	bRightClickPressed = false;
	bRightInHoldMode = false;
	RightClickPressedTime = 0.f;
}


void AESPlayerController::UpdateNearbyPickups()
{
	NearbyPickups.Empty();

	if (!GetPawn()) return;

	FVector PlayerLocation = GetPawn()->GetActorLocation();

	// 1. 遍历世界里所有PickupBase
	TArray<AActor*> AllPickups;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), AESPickupBase::StaticClass(), AllPickups);

	for (AActor* Actor : AllPickups)
	{
		AESPickupBase* Pickup = Cast<AESPickupBase>(Actor);
		if (IsValid(Pickup) && Pickup->CanInteract())
		{
			// 2. 检查距离
			float Distance = FVector::Dist(PlayerLocation, Pickup->GetActorLocation());
			if (Distance <= Pickup->InteractionDistance)
			{
				NearbyPickups.Add(Pickup);
			}
		}
	}

	// ✅ 【关键修复】删掉了Lambda排序代码，避免编译错误
	// 排序不是必须的，如果需要可以后续用简单的冒泡排序代替

	// 3. 修正选中索引
	if (NearbyPickups.Num() > 0)
	{
		SelectedPickupIndex = FMath::Clamp(SelectedPickupIndex, 0, NearbyPickups.Num() - 1);
	}
	else
	{
		SelectedPickupIndex = 0;
	}

	// 4. 更新UI（如果你做了UI的话）
	// if (PickupListWidget) { ... }
}

void AESPlayerController::SelectNextPickup()
{
	if (NearbyPickups.Num() <= 1) return;
	SelectedPickupIndex = (SelectedPickupIndex + 1) % NearbyPickups.Num();
	// 更新UI高亮
}

void AESPlayerController::SelectPreviousPickup()
{
	if (NearbyPickups.Num() <= 1) return;
	SelectedPickupIndex = (SelectedPickupIndex - 1 + NearbyPickups.Num()) % NearbyPickups.Num();
	// 更新UI高亮
}

void AESPlayerController::InteractWithSelected()
{
	if (NearbyPickups.IsValidIndex(SelectedPickupIndex))
	{
		AESPickupBase* Pickup = NearbyPickups[SelectedPickupIndex];
		if (IsValid(Pickup))
		{
			Pickup->OnInteract(GetPawn());
		}
	}
}

AESPlayerBase* AESPlayerController::GetESPlayerCharacter() const
{
	return Cast<AESPlayerBase>(GetPawn());
}
