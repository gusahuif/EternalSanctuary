#include "GAS/ASC/ESAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GAS/AS/ESAttributeSet.h"
#include "GameFramework/Actor.h"

UESAbilitySystemComponent::UESAbilitySystemComponent()
{
}

void UESAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);

	// ActorInfo 初始化完成后再绑定属性监听，避免 AttributeSet 或 ASC 信息尚未可用
	InitializeAttributeChangeListeners();
}

void UESAbilitySystemComponent::SetAbilitySlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!SlotTag.IsValid())
	{
		return;
	}

	if (AbilityClass)
	{
		AbilitySlots.FindOrAdd(SlotTag) = AbilityClass;
	}
	else
	{
		AbilitySlots.Remove(SlotTag);
	}
}

TSubclassOf<UGameplayAbility> UESAbilitySystemComponent::GetAbilitySlot(FGameplayTag SlotTag) const
{
	if (!SlotTag.IsValid())
	{
		return nullptr;
	}

	const TSubclassOf<UGameplayAbility>* FoundAbilityClass = AbilitySlots.Find(SlotTag);
	return FoundAbilityClass ? *FoundAbilityClass : nullptr;
}

void UESAbilitySystemComponent::ClearAbilitySlot(FGameplayTag SlotTag)
{
	if (!SlotTag.IsValid())
	{
		return;
	}

	// 先移除真正授予到 ASC 的技能，保证槽位配置与运行时状态一致
	RemoveAbilityFromSlot(SlotTag);
	AbilitySlots.Remove(SlotTag);
}

bool UESAbilitySystemComponent::GiveAbilityToSlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 Level)
{
	if (!SlotTag.IsValid() || !AbilityClass)
	{
		return false;
	}

	// 若槽位已有旧技能，先移除，避免重复占用同一槽位
	RemoveAbilityFromSlot(SlotTag);

	// 更新槽位配置映射，便于序列化、调试与 UI 查询
	AbilitySlots.FindOrAdd(SlotTag) = AbilityClass;

	if (!IsOwnerActorAuthoritative())
	{
		// GAS 标准做法：授予技能应由服务器执行
		return false;
	}

	FGameplayAbilitySpec AbilitySpec(AbilityClass, Level);

	// 将槽位 Tag 写入 DynamicAbilityTags，后续可用于查询、UI 显示或激活筛选
	AbilitySpec.GetDynamicSpecSourceTags().AddTag(SlotTag);

	const FGameplayAbilitySpecHandle GrantedHandle = GiveAbility(AbilitySpec);
	if (!GrantedHandle.IsValid())
	{
		return false;
	}

	AbilityHandles.FindOrAdd(SlotTag) = GrantedHandle;
	return true;
}

bool UESAbilitySystemComponent::RemoveAbilityFromSlot(FGameplayTag SlotTag)
{
	if (!SlotTag.IsValid())
	{
		return false;
	}

	bool bRemoved = false;

	if (const FGameplayAbilitySpecHandle* FoundHandle = AbilityHandles.Find(SlotTag))
	{
		if (FoundHandle->IsValid() && IsOwnerActorAuthoritative())
		{
			ClearAbility(*FoundHandle);
			bRemoved = true;
		}

		AbilityHandles.Remove(SlotTag);
	}
	else
	{
		// 如果 Handle 映射丢失，但槽位有记录，也视为执行了清理流程
		bRemoved = AbilitySlots.Contains(SlotTag);
	}

	AbilitySlots.Remove(SlotTag);
	return bRemoved;
}

void UESAbilitySystemComponent::AssignAbilityToSlot(TSubclassOf<UGameplayAbility> AbilityClass, int32 SlotIndex)
{
	// 获取当前槽位的 GameplayTag
	FGameplayTag SlotTag = GetSlotTagFromIndex(SlotIndex);
    
	// 1. 查找是否已有该技能的 Spec
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability->GetClass() == AbilityClass)
		{
			// 2. 清除该 Spec 原有的所有 Slot 标签（防止一个技能占多个槽，除非你允许）
			// 3. 将新的 SlotTag 添加到 Spec 的 DynamicAbilityTags 中
			Spec.GetDynamicSpecSourceTags().AddTag(SlotTag);
            
			// 4. 通知客户端更新（如果涉及同步）
			MarkAbilitySpecDirty(Spec);
			return;
		}
	}
}

UGameplayAbility* UESAbilitySystemComponent::GetAbilityInSlot(int32 SlotIndex) const
{
	FGameplayTag SlotTag = GetSlotTagFromIndex(SlotIndex);
	for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.GetDynamicSpecSourceTags().HasTagExact(SlotTag))
		{
			return Spec.Ability;
		}
	}
	return nullptr;
}


void UESAbilitySystemComponent::GiveDefaultAbilities(const TArray<TSubclassOf<UGameplayAbility>>& DefaultAbilities)
{
	const TArray<FGameplayTag> SlotOrder = GetDefaultSlotOrder();
	const int32 AssignCount = FMath::Min(SlotOrder.Num(), DefaultAbilities.Num());

	for (int32 Index = 0; Index < AssignCount; ++Index)
	{
		if (DefaultAbilities[Index])
		{
			GiveAbilityToSlot(SlotOrder[Index], DefaultAbilities[Index], 1);
		}
	}
}

bool UESAbilitySystemComponent::TryActivateAbilityByTag(FGameplayTag AbilityTag)
{
	if (!AbilityTag.IsValid())
	{
		return false;
	}

	// 查找具有该 Tag 的技能
	TArray<FGameplayAbilitySpec*> FoundSpecs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		FGameplayTagContainer(AbilityTag),
		FoundSpecs
	);

	if (FoundSpecs.Num() == 0)
	{
		return false;
	}

	// 激活第一个匹配的技能
	for (FGameplayAbilitySpec* Spec : FoundSpecs)
	{
		if (Spec && Spec->Ability)
		{
			return TryActivateAbility(Spec->Handle);
		}
	}

	return false;
}

bool UESAbilitySystemComponent::IsAbilityActiveByTag(FGameplayTag AbilityTag) const
{
	if (!AbilityTag.IsValid())
	{
		return false;
	}

	TArray<FGameplayAbilitySpec*> FoundSpecs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(
		FGameplayTagContainer(AbilityTag),
		FoundSpecs
	);

	for (const FGameplayAbilitySpec* Spec : FoundSpecs)
	{
		if (Spec && Spec->IsActive())
		{
			return true;
		}
	}

	return false;
}
bool UESAbilitySystemComponent::TryActivateAbilityBySlotIndex(int32 SlotIndex)
{
	FGameplayTag SlotTag = GetSlotTagFromIndex(SlotIndex);
	if (!SlotTag.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[ESASC] 无效的槽位索引：%d"), SlotIndex);
		return false;
	}
	return TryActivateAbilityByTag(SlotTag);
}

FGameplayTag UESAbilitySystemComponent::GetSlotTagFromIndex(int32 SlotIndex) const
{
    if (SlotIndex < 0 || SlotIndex >= 6)
    {
        return FGameplayTag();
    }
	return FGameplayTag::RequestGameplayTag(FName(FString::Printf(TEXT("Ability.Slot.%d"), SlotIndex)));
}

bool UESAbilitySystemComponent::SetAbilityToSlot(int32 SlotIndex, TSubclassOf<UGameplayAbility> AbilityClass, int32 Level)
{
    if (SlotIndex < 0 || SlotIndex >= 6)
    {
        UE_LOG(LogTemp, Error, TEXT("[ESASC] 槽位索引超出范围：%d (0-5)"), SlotIndex);
        return false;
    }
    
    FGameplayTag SlotTag = GetSlotTagFromIndex(SlotIndex);
    return GiveAbilityToSlot(SlotTag, AbilityClass, Level);
}

bool UESAbilitySystemComponent::SwapAbilitySlots(int32 SlotA, int32 SlotB)
{
    if (SlotA < 0 || SlotA >= 6 || SlotB < 0 || SlotB >= 6)
    {
        UE_LOG(LogTemp, Error, TEXT("[ESASC] 槽位索引超出范围：%d, %d"), SlotA, SlotB);
        return false;
    }
    
    if (SlotA == SlotB) return true;
    
    FGameplayTag TagA = GetSlotTagFromIndex(SlotA);
    FGameplayTag TagB = GetSlotTagFromIndex(SlotB);
    
    TSubclassOf<UGameplayAbility> AbilityA = GetAbilitySlot(TagA);
    TSubclassOf<UGameplayAbility> AbilityB = GetAbilitySlot(TagB);
    
    // 交换槽位配置
    if (AbilityA)
    {
        SetAbilitySlot(TagB, AbilityA);
    }
    else
    {
        ClearAbilitySlot(TagB);
    }
    
    if (AbilityB)
    {
        SetAbilitySlot(TagA, AbilityB);
    }
    else
    {
        ClearAbilitySlot(TagA);
    }
    
    // 重新授予技能（需要移除旧的再添加新的）
    // 注意：这里需要更复杂的逻辑来处理已授予的技能 Handle
    // 简化版：先清空再重新授予
    RemoveAbilityFromSlot(TagA);
    RemoveAbilityFromSlot(TagB);
    
    if (AbilityB) GiveAbilityToSlot(TagA, AbilityB, 1);
    if (AbilityA) GiveAbilityToSlot(TagB, AbilityA, 1);
    
    return true;
}

// 修改 GetDefaultSlotOrder() 返回新的槽位顺序
TArray<FGameplayTag> UESAbilitySystemComponent::GetDefaultSlotOrder() const
{
    return
    {
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.0")),  // LMB
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.1")),  // RMB
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.2")),  // 1
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.3")),  // 2
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.4")),  // 3
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.5")),  // 4
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.Passive"))  // 被动
    };
}


bool UESAbilitySystemComponent::IsInvincible() const
{
	static const FGameplayTag InvincibleTag = FGameplayTag::RequestGameplayTag(TEXT("State.Invincible"));
	return HasMatchingGameplayTag(InvincibleTag);
}

bool UESAbilitySystemComponent::IsDead() const
{
	static const FGameplayTag DeadTag = FGameplayTag::RequestGameplayTag(TEXT("State.Dead"));
	return HasMatchingGameplayTag(DeadTag);
}

bool UESAbilitySystemComponent::HasSuperArmor() const
{
	static const FGameplayTag SuperArmorTag = FGameplayTag::RequestGameplayTag(TEXT("State.SuperArmor"));
	return HasMatchingGameplayTag(SuperArmorTag);
}

bool UESAbilitySystemComponent::IsStunned() const
{
	static const FGameplayTag StunnedTag = FGameplayTag::RequestGameplayTag(TEXT("State.Stunned"));
	return HasMatchingGameplayTag(StunnedTag);
}

bool UESAbilitySystemComponent::IsSilenced() const
{
	static const FGameplayTag SilencedTag = FGameplayTag::RequestGameplayTag(TEXT("State.Silenced"));
	return HasMatchingGameplayTag(SilencedTag);
}

void UESAbilitySystemComponent::SendGameplayEventToSelf(FGameplayTag EventTag, AActor* Target, float Magnitude)
{
	AActor* MyOwnerActor = GetOwner();
	if (!MyOwnerActor || !EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;

	// 业务语义：事件由自己发起，也由自己接收
	EventData.Instigator = MyOwnerActor;
	EventData.Target = Target;

	// 可选数值：常用于伤害值、命中强度、打断强度等
	EventData.EventMagnitude = Magnitude;

	// 事件发给自己的 ASC
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MyOwnerActor, EventTag, EventData);
}

void UESAbilitySystemComponent::SendGameplayEventToTarget(FGameplayTag EventTag, AActor* Target, float Magnitude)
{
	AActor* MyOwnerActor = GetOwner();
	if (!MyOwnerActor || !Target || !EventTag.IsValid())
	{
		return;
	}

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;

	// 业务语义：由自己发起，目标接收
	EventData.Instigator = MyOwnerActor;
	EventData.Target = Target;
	EventData.EventMagnitude = Magnitude;

	// 事件发给目标 ASC
	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, EventTag, EventData);
}

void UESAbilitySystemComponent::InitializeAttributeChangeListeners()
{
	if (bAttributeListenersInitialized)
	{
		return;
	}

	const UESAttributeSet* AttributeSet = GetSet<UESAttributeSet>();
	if (!AttributeSet)
	{
		return;
	}

	// 监听 Health 变化，通常用于血条刷新、受击反馈、死亡判断等
	GetGameplayAttributeValueChangeDelegate(UESAttributeSet::GetHealthAttribute()).AddUObject(
		this,
		&UESAbilitySystemComponent::HandleHealthChanged
	);

	// 监听 Shield 变化，通常用于护盾 UI 和战斗反馈
	GetGameplayAttributeValueChangeDelegate(UESAttributeSet::GetShieldAttribute()).AddUObject(
		this,
		&UESAbilitySystemComponent::HandleShieldChanged
	);

	// 攻速监听
	GetGameplayAttributeValueChangeDelegate(UESAttributeSet::GetAttackSpeedAttribute()).AddUObject(
		this, &UESAbilitySystemComponent::HandleAttackSpeedChanged);
	
	bAttributeListenersInitialized = true;
}

void UESAbilitySystemComponent::HandleHealthChanged(const FOnAttributeChangeData& ChangeData)
{
	OnHealthChanged.Broadcast(ChangeData.NewValue, ChangeData.OldValue);
}

void UESAbilitySystemComponent::HandleShieldChanged(const FOnAttributeChangeData& ChangeData)
{
	OnShieldChanged.Broadcast(ChangeData.NewValue, ChangeData.OldValue);
}

void UESAbilitySystemComponent::HandleAttackSpeedChanged(const FOnAttributeChangeData& ChangeData)
{
	OnAttackSpeedChanged.Broadcast(ChangeData.NewValue, ChangeData.OldValue);
}