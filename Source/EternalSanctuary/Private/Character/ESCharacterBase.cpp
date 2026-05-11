#include "Character/ESCharacterBase.h"

#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "GAS/AS/ESAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

AESCharacterBase::AESCharacterBase(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	PrimaryActorTick.bCanEverTick = false;

	// 创建 ASC：作为角色核心战斗组件，玩家和敌人都可以复用
	AbilitySystemComponent = CreateDefaultSubobject<UESAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	check(AbilitySystemComponent);

	// ASC 建议开启复制，Mixed 模式适合大多数 Character 场景
	AbilitySystemComponent->SetIsReplicated(true);
	AbilitySystemComponent->SetReplicationMode(EGameplayEffectReplicationMode::Mixed);

	// 创建 AttributeSet：属性数据统一由 GAS 驱动
	//AttributeSet = CreateDefaultSubobject<UESAttributeSet>(TEXT("AttributeSet"));
	//check(AttributeSet);

	// ==========================================
	// 创建伤害数字锚点
	// ==========================================
	DamageNumberAnchor = CreateDefaultSubobject<USceneComponent>(TEXT("DamageNumberAnchor"));
	DamageNumberAnchor->SetupAttachment(GetMesh()); // 挂在Mesh上
	//DamageNumberAnchor->SetRelativeLocation(FVector(0, 0, 100.0f)); // 头顶上方100单位
}

void AESCharacterBase::BeginPlay()
{
	Super::BeginPlay();

	// AI / 敌人初始化链路：
	// 没有 PlayerState 承载 ASC 时，直接使用自身作为 Owner 和 Avatar
	if (!IsPlayerControlled())
	{
		InitializeAbilityActorInfo(this, this);
	}
}

void AESCharacterBase::PossessedBy(AController* NewController)
{
	Super::PossessedBy(NewController);

	// 玩家服务端初始化链路：
	// 如果项目未来把 ASC 放到 PlayerState，这里仍然是标准入口
	InitializeAbilityActorInfo(this, this);

	// 默认属性和默认技能建议只在服务端初始化
	InitializeDefaultAttributes();
	GiveDefaultAbilities();
}

void AESCharacterBase::OnRep_PlayerState()
{
	Super::OnRep_PlayerState();

	// 玩家客户端初始化链路：
	// GAS 在客户端需要重新设置 ActorInfo，否则本地 Ability/属性监听不完整
	InitializeAbilityActorInfo(this, this);
}

UAbilitySystemComponent* AESCharacterBase::GetAbilitySystemComponent() const
{
	return AbilitySystemComponent;
}

bool AESCharacterBase::HasMatchingGameplayTag(FGameplayTag TagToCheck) const
{
	const UAbilitySystemComponent* ASC = GetESAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	return ASC->HasMatchingGameplayTag(TagToCheck);
}

void AESCharacterBase::InitializeAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	if (!AbilitySystemComponent)
	{
		return;
	}

	// 初始化 ASC 上下文：Owner 表示逻辑拥有者，Avatar 表示实际表现角色
	AbilitySystemComponent->InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	BindAttributeDelegates();
	bAbilityActorInfoInitialized = true;

	// ==========================================
	// 绑定伤害数字事件监听
	// ==========================================
	if (AbilitySystemComponent)
	{
		// 监听 "Event.ShowDamageNumber" 事件
		static const FGameplayTag ShowDamageTag = FGameplayTag::RequestGameplayTag(FName("Event.ShowDamageNumber"));
		AbilitySystemComponent->RegisterGameplayTagEvent(ShowDamageTag, EGameplayTagEventType::AnyCountChange).AddUObject(
			this, &AESCharacterBase::HandleShowDamageNumberEvent
		);
	}
}

void AESCharacterBase::BindAttributeDelegates()
{
	if (bAttributeDelegatesBound || !AbilitySystemComponent)
	{
		return;
	}

	// 监听 Health 变化，并转发到项目 ASC 的广播体系
	AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(UESAttributeSet::GetHealthAttribute())
		.AddUObject(this, &AESCharacterBase::HandleHealthChanged);

	// 监听 Shield 变化，并转发到项目 ASC 的广播体系
	AbilitySystemComponent
		->GetGameplayAttributeValueChangeDelegate(UESAttributeSet::GetShieldAttribute())
		.AddUObject(this, &AESCharacterBase::HandleShieldChanged);
	
	bAttributeDelegatesBound = true;
}

void AESCharacterBase::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	if (AbilitySystemComponent)
	{
		// 广播给 UI、受击表现、状态机等上层系统使用
		AbilitySystemComponent->OnHealthChanged.Broadcast(ChangeData.OldValue, ChangeData.NewValue);
	}

	// 基础死亡判定：血量小于等于 0 时进入死亡
	if (ChangeData.NewValue <= 0.f && IsAlive())
	{
		Die();
	}
}

void AESCharacterBase::HandleShieldChanged(const FOnAttributeChangeData& ChangeData)
{
	if (AbilitySystemComponent)
	{
		// 广播给 UI、护盾特效等系统使用
		AbilitySystemComponent->OnShieldChanged.Broadcast(ChangeData.OldValue, ChangeData.NewValue);
	}
}

void AESCharacterBase::HandleHitReact(AActor* DamageCauser)
{
	if (!AbilitySystemComponent || !IsAlive())
	{
		return;
	}

	// 霸体状态下跳过硬直
	static const FGameplayTag SuperArmorTag = FGameplayTag::RequestGameplayTag(TEXT("State.SuperArmor"));
	if (AbilitySystemComponent->HasMatchingGameplayTag(SuperArmorTag))
	{
		return;
	}

	// 添加硬直状态 Tag
	static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("State.HitReact"));
	AbilitySystemComponent->AddLooseGameplayTag(HitReactTag);

	// 触发受击技能
	static const FGameplayTag HitReactAbilityTag = FGameplayTag::RequestGameplayTag(TEXT("Ability.HitReact"));
	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(HitReactAbilityTag);
	AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags);
}
void AESCharacterBase::InitializeDefaultAttributes()
{
	
}

void AESCharacterBase::GiveDefaultAbilities()
{
	// 基类留空，由子类重写：
	// 通常在这里 GiveAbility 或批量授予默认技能
}

bool AESCharacterBase::IsHostileToActor(const AActor* OtherActor) const
{
	if (!OtherActor || OtherActor == this)
	{
		return false;
	}

	const AESCharacterBase* OtherCharacter = Cast<AESCharacterBase>(OtherActor);
	if (!OtherCharacter)
	{
		return false;
	}

	// 中立阵营默认不主动敌对，避免误伤逻辑混乱
	if (Faction == EESFaction::Neutral || OtherCharacter->Faction == EESFaction::Neutral)
	{
		return false;
	}

	return Faction != OtherCharacter->Faction;
}

bool AESCharacterBase::IsDead() const
{
	// 推荐优先从死亡 Tag 判断，bDead 作为兜底
	// 例如：State.Dead
	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));
	return HasMatchingGameplayTag(DeadTag);
}

bool AESCharacterBase::IsInHitReact() const
{
	// 例如：State.HitReact
	static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("State.HitReact"));
	return HasMatchingGameplayTag(HitReactTag);
}

bool AESCharacterBase::HasSuperArmor() const
{
	// 例如：State.SuperArmor
	static const FGameplayTag SuperArmorTag = FGameplayTag::RequestGameplayTag(TEXT("State.SuperArmor"));
	return HasMatchingGameplayTag(SuperArmorTag);
}

bool AESCharacterBase::CanAct() const
{
	// 死亡时不可行动
	if (IsDead())
	{
		return false;
	}

	// 硬直期间不可行动（后续如果你想允许“受击中缓慢转向”，可以放宽）
	if (IsInHitReact())
	{
		return false;
	}

	// 如果后续有 Stun、Knockdown、Frozen，也建议统一在这里扩展
	static const FGameplayTag StunTag = FGameplayTag::RequestGameplayTag(TEXT("State.Stun"));
	if (HasMatchingGameplayTag(StunTag))
	{
		return false;
	}

	return true;
}

bool AESCharacterBase::CanBeTargeted() const
{
	// 死亡单位不允许继续作为锁定目标
	if (IsDead())
	{
		return false;
	}

	// 如果你后面有隐身、不可选中，也在这里加 Tag 判断
	static const FGameplayTag UntargetableTag = FGameplayTag::RequestGameplayTag(TEXT("State.Untargetable"));
	if (HasMatchingGameplayTag(UntargetableTag))
	{
		return false;
	}

	return true;
}

bool AESCharacterBase::CanBeInterrupted() const
{
	// 有霸体则不能被普通受击打断
	if (HasSuperArmor())
	{
		return false;
	}

	// 死亡也没必要再谈打断
	if (IsDead())
	{
		return false;
	}

	return true;
}

bool AESCharacterBase::IsAlive() const
{
	if (!AbilitySystemComponent)
	{
		return true;
	}

	// 使用 Dead Tag 判断更稳定：
	// 避免单纯依赖 Health 数值导致复活、中间态、假死状态处理混乱
	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName(TEXT("State.Dead")));
	return !AbilitySystemComponent->HasMatchingGameplayTag(DeadTag);
}

void AESCharacterBase::Die()
{
	if (!AbilitySystemComponent || !IsAlive())
	{
		return;
	}

	// 添加死亡状态 Tag
	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));
	AbilitySystemComponent->AddLooseGameplayTag(DeadTag);

	// 清理可能残留的硬直状态
	static const FGameplayTag HitReactTag = FGameplayTag::RequestGameplayTag(TEXT("State.HitReact"));
	if (AbilitySystemComponent->HasMatchingGameplayTag(HitReactTag))
	{
		AbilitySystemComponent->RemoveLooseGameplayTag(HitReactTag);
	}

	// 停止移动并关闭碰撞
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}
	SetActorEnableCollision(false);

	// 触发死亡技能（按你的要求通过 AI.State.Dead 标签）
	static const FGameplayTag DeadAbilityTag = FGameplayTag::RequestGameplayTag(TEXT("AI.State.Dead"));
	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(DeadAbilityTag);
	AbilitySystemComponent->TryActivateAbilitiesByTag(AbilityTags);
}

float AESCharacterBase::GetHealth() const
{
	return AttributeSet ? AttributeSet->GetHealth() : 0.f;
}

float AESCharacterBase::GetMaxHealth() const
{
	return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f;
}

float AESCharacterBase::GetShield() const
{
	return AttributeSet ? AttributeSet->GetShield() : 0.f;
}

float AESCharacterBase::GetMaxShield() const
{
	return AttributeSet ? AttributeSet->GetMaxHealth() : 0.f;
}

// ==========================================
// 【新增】伤害数字系统实现
// ==========================================

void AESCharacterBase::ShowDamageNumber(float DamageValue, bool bIsCritical, const FVector& WorldLocation)
{
	if (!DamageNumberNiagara)
	{
		UE_LOG(LogTemp, Warning, TEXT("【伤害数字】错误：DamageNumberNiagara 未在蓝图配置！"));
		return;
	}

	// 1. 确定显示位置
	FVector SpawnLocation = WorldLocation;
	if (SpawnLocation.IsZero())
	{
		// 如果没有传入位置，用头顶锚点
		SpawnLocation = GetDamageNumberAnchorLocation();
	}

	// 2. 生成Niagara
	UNiagaraComponent* NiagaraComp = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
		GetWorld(),
		DamageNumberNiagara,
		SpawnLocation
	);

	if (NiagaraComp)
	{
		// 3. 设置Niagara参数
		// DamageValue：正数显示红色（伤害），负数显示绿色（治疗）
		NiagaraComp->SetVariableFloat(FName("DamageValue"), DamageValue);
		NiagaraComp->SetVariableBool(FName("bIsCritical"), bIsCritical);
		
		NiagaraComp->SetRenderCustomDepth(true);
		NiagaraComp->SetCustomDepthStencilValue(2);
		
		// 简单的随机偏移，避免数字完全重叠
		FVector RandomOffset = FVector(
			FMath::FRandRange(-30.0f, 30.0f),
			FMath::FRandRange(-30.0f, 30.0f),
			FMath::FRandRange(0.0f, 20.0f)
		);
		NiagaraComp->AddRelativeLocation(RandomOffset);
	}
}

FVector AESCharacterBase::GetDamageNumberAnchorLocation() const
{
	if (DamageNumberAnchor)
	{
		return DamageNumberAnchor->GetComponentLocation();
	}
	// 兜底：返回Actor位置+100
	return GetActorLocation() + FVector(0, 0, 100.0f);
}

void AESCharacterBase::HandleShowDamageNumberEvent(FGameplayTag Tag, int32 NewCount)
{
	// 这个函数由ASC的Tag事件触发
	// 实际数据通过GameplayEffectContext的Payload传递
	// 我们需要在AttributeSet里发送一个带数据的GameplayEvent
}
