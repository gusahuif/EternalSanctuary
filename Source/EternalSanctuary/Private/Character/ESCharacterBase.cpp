#include "Character/ESCharacterBase.h"

#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "GAS/AS/ESAttributeSet.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "GameplayTagContainer.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerState.h"
#include "GameplayEffect.h"

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
}

void AESCharacterBase::BindAttributeDelegates()
{
	if (bAttributeDelegatesBound || !AbilitySystemComponent || !AttributeSet)
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

void AESCharacterBase::InitializeDefaultAttributes()
{
	
}

void AESCharacterBase::GiveDefaultAbilities()
{
	// 基类留空，由子类重写：
	// 通常在这里 GiveAbility 或批量授予默认技能
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

	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(FName(TEXT("State.Dead")));
	AbilitySystemComponent->AddLooseGameplayTag(DeadTag);

	// 停止移动，避免死亡后继续响应移动输入或 AI MoveTo
	if (UCharacterMovementComponent* MoveComp = GetCharacterMovement())
	{
		MoveComp->StopMovementImmediately();
		MoveComp->DisableMovement();
	}

	// 关闭碰撞可按项目需求决定：
	// 如果你后续还要做尸体阻挡、Ragdoll、可拾取交互，可以改成更细粒度配置
	SetActorEnableCollision(false);

	// 这里先给基础逻辑，后续子类可扩展：
	// 例如播放死亡 Montage、触发 dissolve、延迟销毁、掉落战利品等
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
