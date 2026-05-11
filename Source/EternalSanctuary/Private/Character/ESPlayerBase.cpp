#include "Character/ESPlayerBase.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "Abilities/GameplayAbility.h"
#include "Engine/DataTable.h"
#include "GAS/GA/ESGameplayAbilityBase.h"
//#include "GAS/Data/ESAbilityMetaData.h"

AESPlayerBase::AESPlayerBase()
{
	PrimaryActorTick.bCanEverTick = true;
	// 创建弹簧臂
	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 1200.0f; // 摄像机距离，可在蓝图里调
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
		// 检查0号插槽技能是否正在播放，避免重复触发
		if (ESASC->IsAbilityActiveBySlotIndex(0))
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
	// 1. 死亡/无法接受指令时，直接返回
	if (IsInState(ECommandState::Dead) || !CanAcceptMoveCommand())
	{
		return;
	}

	// 2. 检查是否有攻击目标（左键攻击需要锁定目标）
	if (!IsValid(CurrentAttackTarget))
	{
		return;
	}

	// 3. 检查与目标的距离，超出攻击范围则不释放技能
	const float Dist = FVector::Dist2D(GetActorLocation(), CurrentAttackTarget->GetActorLocation());
	if (Dist > AttackRange)
	{
		return;
	}

	// 4. 获取玩家的AbilitySystemComponent（GAS核心组件）
	UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!ESASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("TriggerBasicAttack: 无法获取ESAbilitySystemComponent"));
		return;
	}

	// 5. 定义左键对应的技能插槽索引（0号插槽）
	const int32 LeftClickSkillSlot = 0;
	// 6. 检查0号插槽技能是否正在播放，避免重复释放
	if (ESASC->IsAbilityActiveBySlotIndex(LeftClickSkillSlot)) // 需确保ESASC有该方法，若没有则先注释并替换为临时逻辑
	{
		return;
	}

	// 7. 核心修改：按插槽索引激活0号技能（替代原Tag激活方式）
	// 注意：需确保UESAbilitySystemComponent实现了TryActivateAbilityBySlotIndex方法
	// 若未实现，可先使用临时调试逻辑，后续补充GAS插槽激活逻辑
	bool bActivated = ESASC->TryActivateAbilityBySlotIndex(LeftClickSkillSlot);
	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning, TEXT("TriggerBasicAttack: 0号插槽技能激活失败"));
	}
}

void AESPlayerBase::AttackAtLocation(const FVector& TargetLocation)
{
	// 1. 死亡状态下禁止释放技能
	if (IsInState(ECommandState::Dead))
	{
		return;
	}

	// 2. 获取GAS核心组件
	UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!ESASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackAtLocation: 无法获取ESAbilitySystemComponent"));
		return;
	}

	// 3. 调整玩家朝向到点击位置（保证技能朝向正确）
	FRotator TargetRotation = (TargetLocation - GetActorLocation()).ToOrientationRotator();
	TargetRotation.Pitch = 0.f; // 只修改Yaw（水平）旋转，保持Pitch（垂直）为0
	SetActorRotation(TargetRotation);

	// 4. 定义点击位置攻击对应的技能插槽（0号插槽）
	const int32 ClickLocationSkillSlot = 0;
	// 5. 检查0号插槽技能是否正在播放
	//if (ESASC->IsAbilityActiveBySlotIndex(ClickLocationSkillSlot))
	{
		//return;
	}

	// 6. 按插槽索引激活0号技能
	bool bActivated = ESASC->TryActivateAbilityBySlotIndex(ClickLocationSkillSlot);
	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning, TEXT("AttackAtLocation: 0号插槽技能激活失败"));
	}
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

// --------------------------
// 新增：加载技能元数据表（从蓝图DT读取到SkillMetaDataMap）
// --------------------------
void AESPlayerBase::LoadSkillMetaData(UDataTable* SkillDataTable)
{
	if (!SkillDataTable)
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, TEXT("技能元数据表为空！"));
		return;
	}

	// 清空旧数据
	SkillMetaDataMap.Empty();

	// 遍历DataTable所有行，存入Map（SkillID → 元数据）
	TArray<FES_SkillMetaData*> Rows;
	SkillDataTable->GetAllRows<FES_SkillMetaData>("", Rows);
	for (FES_SkillMetaData* Row : Rows)
	{
		if (Row && Row->SkillID != NAME_None)
		{
			SkillMetaDataMap.Add(Row->SkillID, *Row);
		}
	}

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
	                                 FString::Printf(TEXT("成功加载%d个技能元数据"), SkillMetaDataMap.Num()));
}

// --------------------------
// 新增：绑定技能到槽位
// --------------------------
bool AESPlayerBase::BindSkillToSlot(ESkillSlotID SlotID, FName SkillID)
{
	// 被动槽位走专用接口
	if (SlotID == ESkillSlotID::Slot_Passive)
	{
		return EquipPassiveSkill(SkillID);
	}

	// 检查技能ID是否存在于元数据表
	if (!SkillMetaDataMap.Contains(SkillID))
	{
		GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red,
		                                 FString::Printf(TEXT("技能ID %s 不存在！"), *SkillID.ToString()));
		return false;
	}

	// 获取技能元数据
	FES_SkillMetaData SkillMeta = SkillMetaDataMap[SkillID];

	// 构建绑定数据
	FES_SkillSlotBinding BindingData;
	BindingData.SlotID = SlotID;
	BindingData.BoundSkillID = SkillID;
	BindingData.BoundSkillTag = SkillMeta.SkillTag;

	if (!SkillSlotBindings.Find(SlotID))
	{
		GetESAbilitySystemComponent()->GiveAbility(SkillMeta.AbilityClass);
	}
	// 更新绑定表
	SkillSlotBindings.Add(SlotID, BindingData);

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Green,
	                                 FString::Printf(TEXT("技能 %s 绑定到槽位 %d"), *SkillID.ToString(), (uint8)SlotID));
	return true;
}

// --------------------------
// 被动技能：装备/卸载
// --------------------------
bool AESPlayerBase::EquipPassiveSkill(FName SkillID)
{
	// 1. 检查技能ID是否存在于元数据表
	if (!SkillMetaDataMap.Contains(SkillID))
	{
		UE_LOG(LogTemp, Error, TEXT("[EquipPassive] 技能ID %s 不存在！"), *SkillID.ToString());
		return false;
	}

	FES_SkillMetaData SkillMeta = SkillMetaDataMap[SkillID];

	// 调试：打印SkillTag信息
	UE_LOG(LogTemp, Warning, TEXT("[EquipPassive] SkillID=%s, SkillTag=%s, AbilityClass=%s"),
	       *SkillID.ToString(),
	       SkillMeta.SkillTag.IsValid() ? *SkillMeta.SkillTag.ToString() : TEXT("INVALID"),
	       SkillMeta.AbilityClass ? *SkillMeta.AbilityClass->GetName() : TEXT("NULL"));

	// 2. 如果已有被动，先移除旧的
	if (EquippedPassiveSkillID != NAME_None)
	{
		UnequipPassiveSkill();
	}

	// 3. 授予新被动GA，保存SpecHandle
	UESAbilitySystemComponent* ESASC = GetESAbilitySystemComponent();
	if (!ESASC || !SkillMeta.AbilityClass)
	{
		UE_LOG(LogTemp, Error, TEXT("[EquipPassive] ASC无效或AbilityClass为空！"));
		return false;
	}

	PassiveAbilitySpecHandle = ESASC->GiveAbility(FGameplayAbilitySpec(SkillMeta.AbilityClass, 1, INDEX_NONE, this));

	// 4. 绑定到被动槽位
	FES_SkillSlotBinding BindingData;
	BindingData.SlotID = ESkillSlotID::Slot_Passive;
	BindingData.BoundSkillID = SkillID;
	BindingData.BoundSkillTag = SkillMeta.SkillTag;
	SkillSlotBindings.Add(ESkillSlotID::Slot_Passive, BindingData);

	// 5. 记录当前被动
	EquippedPassiveSkillID = SkillID;

	// 6. 直接授予被动Tag（不依赖GA激活，确保管线能检测到）
	if (SkillMeta.SkillTag.IsValid())
	{
		ESASC->AddLooseGameplayTag(SkillMeta.SkillTag);

		// 验证Tag确实被添加
		bool bHasTag = ESASC->HasMatchingGameplayTag(SkillMeta.SkillTag);
		UE_LOG(LogTemp, Warning, TEXT("[EquipPassive] 授予Tag: %s, 验证=%s"),
		       *SkillMeta.SkillTag.ToString(), bHasTag ? TEXT("成功") : TEXT("失败"));
	}
	else
	{
		UE_LOG(LogTemp, Error,
		       TEXT(
			       "[EquipPassive] SkillTag无效！检查DataTable中SK_Archer_13的SkillTag是否为Ability.Archer.HuntersInstinct.Passive"
		       ));
	}

	// 7. 延迟一帧激活GA（GiveAbility当帧Spec可能未完全注册）
	FTimerHandle TempHandle;
	GetWorldTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this, [this, ESASC]()
	{
		if (PassiveAbilitySpecHandle.IsValid())
		{
			bool bResult = ESASC->TryActivateAbility(PassiveAbilitySpecHandle);
			UE_LOG(LogTemp, Warning, TEXT("[EquipPassive] 延迟激活GA结果: %s"), bResult ? TEXT("成功") : TEXT("失败"));
		}
	}));

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Cyan,
	                                 FString::Printf(TEXT("装备被动技能: %s"), *SkillID.ToString()));
	return true;
}

bool AESPlayerBase::UnequipPassiveSkill()
{
	if (EquippedPassiveSkillID == NAME_None)
	{
		return false;
	}

	UESAbilitySystemComponent* ESASC = GetESAbilitySystemComponent();
	FES_SkillSlotBinding* Binding = SkillSlotBindings.Find(ESkillSlotID::Slot_Passive);
	if (Binding && Binding->BoundSkillTag.IsValid() && ESASC)
	{
		// 1. 先移除被动Tag（直接移除，不依赖GA的EndAbility）
		ESASC->RemoveLooseGameplayTag(Binding->BoundSkillTag);

		// 2. 移除被动GA Spec（使用保存的Handle精确移除）
		if (PassiveAbilitySpecHandle.IsValid())
		{
			ESASC->ClearAbility(PassiveAbilitySpecHandle);
		}
	}

	SkillSlotBindings.Remove(ESkillSlotID::Slot_Passive);
	EquippedPassiveSkillID = NAME_None;
	PassiveAbilitySpecHandle = FGameplayAbilitySpecHandle();

	GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Yellow, TEXT("卸载被动技能"));
	UE_LOG(LogTemp, Log, TEXT("[UnequipPassive] 卸载被动技能"));
	return true;
}

FName AESPlayerBase::GetEquippedPassiveSkillID() const
{
	return EquippedPassiveSkillID;
}

bool AESPlayerBase::RemoveSkillToSlot(ESkillSlotID SlotID)
{
	// 被动槽位走专用接口
	if (SlotID == ESkillSlotID::Slot_Passive)
	{
		return UnequipPassiveSkill();
	}

	if (SkillSlotBindings.Find(SlotID) && SkillSlotBindings.Find(SlotID)->BoundSkillTag.IsValid())
	{
		GetESAbilitySystemComponent()->RemoveAbilityFromSlot(SkillSlotBindings.Find(SlotID)->BoundSkillTag);
		return true;
	}
	return false;
}

bool AESPlayerBase::SwapSkillToSlot(ESkillSlotID SourceSlotID, ESkillSlotID TargetSlotID)
{
	// 被动槽位不支持与主动槽位互换，请使用EquipPassiveSkill
	if (SourceSlotID == ESkillSlotID::Slot_Passive || TargetSlotID == ESkillSlotID::Slot_Passive)
	{
		UE_LOG(LogTemp, Warning, TEXT("[SwapSkill] 被动槽位不支持互换，请使用EquipPassiveSkill"));
		return false;
	}

	if (SkillSlotBindings.Find(SourceSlotID) && SkillSlotBindings.Find(TargetSlotID) && SkillSlotBindings.
		Find(SourceSlotID)->BoundSkillTag.IsValid() && SkillSlotBindings.Find(TargetSlotID)->BoundSkillTag.IsValid())
	{
		FES_SkillSlotBinding TempValue = *SkillSlotBindings.Find(SourceSlotID);
		SkillSlotBindings.Add(SourceSlotID, *SkillSlotBindings.Find(TargetSlotID));
		SkillSlotBindings.Add(TargetSlotID, TempValue);
		return true;
	}
	return false;
}

// --------------------------
// 根据槽位ID获取技能Tag
// --------------------------
FGameplayTag AESPlayerBase::GetSkillTagBySlotID(ESkillSlotID SlotID) const
{
	const FES_SkillSlotBinding* Binding = SkillSlotBindings.Find(SlotID);
	return Binding ? Binding->BoundSkillTag : FGameplayTag::EmptyTag;
}

void AESPlayerBase::OnSkillSlotTrigger(ESkillSlotID SlotID)
{
	// 被动技能不需要按键触发（装备时已自动激活）
	if (SlotID == ESkillSlotID::Slot_Passive)
	{
		return;
	}

	// 基础校验：死亡/不可接受指令时拒绝
	if (IsInState(ECommandState::Dead)) // || !CanAcceptMoveCommand())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("无法释放技能：死亡/不可打断施法"));
		return;
	}

	// 1. 获取槽位绑定的SkillTag
	FGameplayTag SkillTag = GetSkillTagBySlotID(SlotID);
	if (!SkillTag.IsValid())
	{
		FString Msg = FString::Printf(TEXT("槽位 %d 未绑定技能！"), (uint8)SlotID);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, Msg);
		return;
	}

	// 2. 获取ASC并激活技能
	UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!ESASC)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("无法获取ASC，技能释放失败！"));
		return;
	}

	// 3. 可选：检查技能是否正在播放（也可交给GA处理）
	/*if (ESASC->IsAbilityActiveByTag(SkillTag))
	{
		FString Msg = FString::Printf(TEXT("槽位 %d 技能正在播放！"), (uint8)SlotID);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Yellow, Msg);
		return;
	}*/

	// 4. 核心：激活技能（CD/Cost由GA处理）
	bool bActivated = ESASC->TryActivateAbilityByTag(SkillTag);
	if (bActivated)
	{
		FString Msg = FString::Printf(TEXT("成功释放槽位 %d 技能！"), (uint8)SlotID);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Green, Msg);

		// 可选：进入Cast状态（技能默认可打断）
		// BeginCast(true);

		// 模拟技能结束（实际由GA/动画通知调用EndCast）
		//FTimerHandle SkillTimer;
		//GetWorldTimerManager().SetTimer(SkillTimer, this, &AESPlayerBase::EndCast, 0.8f, false);
	}
	else
	{
		FString Msg = FString::Printf(TEXT("槽位 %d 技能激活失败（GA未配置/资源不足）！"), (uint8)SlotID);
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, Msg);
	}
}

void AESPlayerBase::OnSkillSlotComplate(ESkillSlotID SlotID)
{
	// 被动技能不需要释放回调
	if (SlotID == ESkillSlotID::Slot_Passive)
	{
		return;
	}

	FGameplayTag SkillTag = GetSkillTagBySlotID(SlotID);
	if (!SkillTag.IsValid())
	{
		return;
	}
	TArray<FGameplayAbilitySpec*> FoundSpecs;
	GetESAbilitySystemComponent()->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		FGameplayTagContainer(SkillTag),
		FoundSpecs
	);
	UESGameplayAbilityBase* GAInstance = Cast<UESGameplayAbilityBase>(FoundSpecs[0]->GetAbilityInstances()[0]);
	GAInstance->OnSkillKeyboardRelease();
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

/*void AESPlayerBase::ActivateSkill(int32 Slot)
{
	// 1. 无法接受指令（如死亡/不可打断的施法中），直接返回
	if (!CanAcceptMoveCommand())
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateSkill: 无法接受指令，插槽%d技能释放失败"), Slot);
		return;
	}

	// 2. 获取GAS核心组件
	UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponent());
	if (!ESASC)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateSkill: 无法获取ESAbilitySystemComponent，插槽%d技能释放失败"), Slot);
		return;
	}

	// 3. 检查插槽索引合法性（避免负数/超出最大插槽数）
	const int32 MaxSkillSlot = 4; // 示例：假设最大支持4个技能插槽（0-3），可根据实际需求调整
	if (Slot < 0 || Slot >= MaxSkillSlot)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateSkill: 插槽%d超出合法范围（0-%d）"), Slot, MaxSkillSlot-1);
		return;
	}

	// 4. 检查该插槽技能是否正在播放，避免重复释放
	if (ESASC->IsAbilityActiveBySlotIndex(Slot))
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateSkill: 插槽%d技能正在播放，跳过释放"), Slot);
		return;
	}

	// 5. 进入施法状态（设为可打断，符合多数技能设计）
	BeginCast(true);

	// 6. 调试信息：提示释放的技能插槽
	FString SkillMsg = FString::Printf(TEXT("释放了技能插槽 %d"), Slot);
	GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Cyan, SkillMsg);

	// 7. 核心逻辑：按插槽索引激活技能
	bool bActivated = ESASC->TryActivateAbilityBySlotIndex(Slot);
	if (!bActivated)
	{
		UE_LOG(LogTemp, Warning, TEXT("ActivateSkill: 插槽%d技能激活失败"), Slot);
		// 激活失败时退出Cast状态
		EndCast();
		return;
	}

	// 8. 【可选】模拟技能持续时间（实际项目中应由动画通知/技能逻辑调用EndCast）
	// 示例：假设技能持续0.8秒，结束后退出Cast状态
	//FTimerHandle SkillTimer;
	//GetWorldTimerManager().SetTimer(SkillTimer, this, &AESPlayerBase::EndCast, 0.8f, false);
}*/
void AESPlayerBase::ActivateSkill(int32 Slot)
{
	if (!CanAcceptMoveCommand())
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("无法释放技能：死亡/不可打断施法"));
		return;
	}

	// 将int32 Slot转为ESkillSlotID（0=左键，1=数字1，2=数字2...）
	ESkillSlotID SlotID = ESkillSlotID::Slot_LeftClick;
	switch (Slot)
	{
	case 0: SlotID = ESkillSlotID::Slot_LeftClick;
		break;
	case 1: SlotID = ESkillSlotID::Slot_RightClick;
		break;
	case 2: SlotID = ESkillSlotID::Slot_1;
		break;
	case 3: SlotID = ESkillSlotID::Slot_2;
		break;
	case 4: SlotID = ESkillSlotID::Slot_3;
		break;
	case 5: SlotID = ESkillSlotID::Slot_4;
		break;
	default:
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("无效的技能槽位索引！"));
		return;
	}

	// 复用OnSkillSlotPressed的逻辑
	OnSkillSlotTrigger(SlotID);
}

// ──────────────────────────────────────────────
//  被动技能通用计算接口实现
// ──────────────────────────────────────────────

float AESPlayerBase::CalculatePassiveDamageBonus(float FlightDistanceMeters) const
{
	if (EquippedPassiveSkillID == NAME_None) return 0.0f;

	UESAbilitySystemComponent* ESASC = GetESAbilitySystemComponent();
	if (!ESASC) return 0.0f;

	const FES_SkillSlotBinding* Binding = SkillSlotBindings.Find(ESkillSlotID::Slot_Passive);
	if (!Binding || !Binding->BoundSkillTag.IsValid()) return 0.f;

	TArray<FGameplayAbilitySpec*> FoundSpecs;
	ESASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		FGameplayTagContainer(Binding->BoundSkillTag), FoundSpecs);

	float TotalBonus = 0.f;
	for (FGameplayAbilitySpec* Spec : FoundSpecs)
	{
		if (Spec && Spec->Ability)
		{
			UESGameplayAbilityBase* GA = Cast<UESGameplayAbilityBase>(Spec->Ability);
			if (GA)
			{
				TotalBonus += GA->GetPassiveDamageBonus(FlightDistanceMeters);
			}
		}
	}

	return TotalBonus;
}

float AESPlayerBase::CalculatePassiveDamageReduction(float AttackerDistanceMeters) const
{
	if (EquippedPassiveSkillID == NAME_None) return 0.f;

	UESAbilitySystemComponent* ESASC = GetESAbilitySystemComponent();
	if (!ESASC) return 0.f;

	const FES_SkillSlotBinding* Binding = SkillSlotBindings.Find(ESkillSlotID::Slot_Passive);
	if (!Binding || !Binding->BoundSkillTag.IsValid()) return 0.f;

	TArray<FGameplayAbilitySpec*> FoundSpecs;
	ESASC->GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		FGameplayTagContainer(Binding->BoundSkillTag), FoundSpecs);

	float TotalReduction = 0.f;
	for (FGameplayAbilitySpec* Spec : FoundSpecs)
	{
		if (Spec && Spec->Ability)
		{
			UESGameplayAbilityBase* GA = Cast<UESGameplayAbilityBase>(Spec->Ability);
			if (GA)
			{
				TotalReduction += GA->GetPassiveDamageReduction(AttackerDistanceMeters);
			}
		}
	}

	return TotalReduction;
}
