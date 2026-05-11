#include "GAS/ASC/ESAbilitySystemComponent.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "GameplayAbilitySpec.h"
#include "GameplayEffectTypes.h"
#include "GameplayTagContainer.h"
#include "GAS/AS/ESAttributeSet.h"
#include "GameFramework/Actor.h"
#include "Gameplay/InGameProgression/ESInGameProgressionComponent.h"
#include "GAS/GA/ESGameplayAbilityBase.h"

UESAbilitySystemComponent::UESAbilitySystemComponent()
{
	SkillDataTable = nullptr;
}

void UESAbilitySystemComponent::InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor)
{
	Super::InitAbilityActorInfo(InOwnerActor, InAvatarActor);
	InitializeAttributeChangeListeners();
}

// ==========================================
// 【技能/符文系统】实现
// ==========================================

TArray<UESGameplayAbilityBase*> UESAbilitySystemComponent::GetAllESAbilities() const
{
    TArray<UESGameplayAbilityBase*> Result;

    // 遍历所有 Activatable Abilities
    for (const FGameplayAbilitySpec& Spec : GetActivatableAbilities())
    {
        if (Spec.Ability)
        {
            UESGameplayAbilityBase* ESAbility = Cast<UESGameplayAbilityBase>(Spec.Ability);
            if (ESAbility)
            {
                Result.Add(ESAbility);
            }
        }
    }

    return Result;
}

bool UESAbilitySystemComponent::TryUnlockSkill(UESGameplayAbilityBase* Ability, int32 Cost)
{
    if (!Ability || Ability->IsUnlocked())
    {
        return false;
    }

    // 1. 获取强化点组件
    UESInGameProgressionComponent* Progression = GetOwner()->FindComponentByClass<UESInGameProgressionComponent>();
    if (!Progression)
    {
        return false;
    }

    // 2. 尝试消耗点数
    if (!Progression->SpendUpgradePoints(Cost))
    {
        return false;
    }

    // 3. 读取 MetaData，获取 UnlockTag
    FES_SkillMetaData MetaData;
    if (Ability->GetSkillMetaData(MetaData) && MetaData.UnlockTag.IsValid())
    {
        // 4. 添加 UnlockTag
        AddLooseGameplayTag(MetaData.UnlockTag);
        return true;
    }

    // 如果没有配置 UnlockTag，默认视为解锁成功（或者你可以返回 false）
    return true;
}

bool UESAbilitySystemComponent::TryUnlockAndActivateRune(UESGameplayAbilityBase* Ability, FGameplayTag RuneTag, int32 Cost)
{
    if (!Ability || !RuneTag.IsValid())
    {
        return false;
    }

    // 1. 如果符文已经激活了，直接返回 false
    if (Ability->IsRuneActive(RuneTag))
    {
        return false;
    }

    // 2. 获取强化点组件
    UESInGameProgressionComponent* Progression = GetOwner()->FindComponentByClass<UESInGameProgressionComponent>();
    if (!Progression)
    {
        return false;
    }

    // 3. 尝试消耗点数
    if (!Progression->SpendUpgradePoints(Cost))
    {
        return false;
    }

    // 4. 激活符文
    return Ability->TryActivateRune(RuneTag);
}

FText UESAbilitySystemComponent::GetFullAbilityDescription(UESGameplayAbilityBase* Ability) const
{
    if (!Ability)
    {
        return FText::GetEmpty();
    }

    FString FinalDesc;

    // 1. 获取技能基础描述
    FES_SkillMetaData MetaData;
    if (Ability->GetSkillMetaData(MetaData))
    {
        FinalDesc += MetaData.SkillDesc.ToString();
    }
    else
    {
        // 如果 DataTable 里没有，用 GA 里的旧字段兜底
        FinalDesc += Ability->AbilityDescription.ToString();
    }

    // 2. 获取已激活的符文，追加到描述后面
    FGameplayTagContainer ActiveRunes = Ability->GetActiveRunes();
    TArray<FESRuneData> AvailableRunes = Ability->GetAvailableRunes();

    if (ActiveRunes.Num() > 0)
    {
        FinalDesc += "\n\n<RichTextBlock.Bold>已激活符文：</>"; // 可以用富文本

        for (const FESRuneData& Rune : AvailableRunes)
        {
            if (ActiveRunes.HasTag(Rune.RuneTag))
            {
                FinalDesc += FString::Printf(TEXT("\n• %s：%s"), 
                    *Rune.RuneName.ToString(), 
                    *Rune.RuneDescription.ToString());
            }
        }
    }

    return FText::FromString(FinalDesc);
}

// ==========================================
// 查表函数
// ==========================================

bool UESAbilitySystemComponent::GetSkillMetaData(FName SkillID, FES_SkillMetaData& OutMetaData) const
{
	if (!SkillDataTable || SkillID == NAME_None)
	{
		return false;
	}

	static const FString ContextString = TEXT("ESAbilitySystemComponent::GetSkillMetaData");
	FES_SkillMetaData* Row = SkillDataTable->FindRow<FES_SkillMetaData>(SkillID, ContextString);
	
	if (Row)
	{
		OutMetaData = *Row;
		return true;
	}

	return false;
}
// ==========================================
// 冷却系统实现
// ==========================================

void UESAbilitySystemComponent::SetSkillCooldown(FName InSkillID, float Duration)
{
	if (InSkillID == NAME_None || Duration <= 0.0f)
	{
		return;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float EndTime = CurrentTime + Duration;
	SkillCooldownMap.Add(InSkillID, EndTime);
}

bool UESAbilitySystemComponent::IsSkillOnCooldown(FName InSkillID) const
{
	if (InSkillID == NAME_None)
	{
		return false;
	}

	const float* EndTimePtr = SkillCooldownMap.Find(InSkillID);
	if (!EndTimePtr)
	{
		return false;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	return CurrentTime < *EndTimePtr;
}

float UESAbilitySystemComponent::GetSkillCooldownRemaining(FName InSkillID) const
{
	if (InSkillID == NAME_None)
	{
		return 0.0f;
	}

	const float* EndTimePtr = SkillCooldownMap.Find(InSkillID);
	if (!EndTimePtr)
	{
		return 0.0f;
	}

	float CurrentTime = GetWorld()->GetTimeSeconds();
	float Remaining = *EndTimePtr - CurrentTime;
	
	return FMath::Max(Remaining, 0.0f);
}

float UESAbilitySystemComponent::GetSkillCooldownTotal(FName InSkillID) const
{
	FES_SkillMetaData Data;
	if (GetSkillMetaData(InSkillID, Data))
	{
		return Data.CooldownTime;
	}
	return 0.0f;
}

void UESAbilitySystemComponent::SetAbilitySlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass)
{
	if (!SlotTag.IsValid()) return;
	if (AbilityClass) AbilitySlots.FindOrAdd(SlotTag) = AbilityClass;
	else AbilitySlots.Remove(SlotTag);
}

TSubclassOf<UGameplayAbility> UESAbilitySystemComponent::GetAbilitySlot(FGameplayTag SlotTag) const
{
	if (!SlotTag.IsValid()) return nullptr;
	const TSubclassOf<UGameplayAbility>* FoundAbilityClass = AbilitySlots.Find(SlotTag);
	return FoundAbilityClass ? *FoundAbilityClass : nullptr;
}

void UESAbilitySystemComponent::ClearAbilitySlot(FGameplayTag SlotTag)
{
	if (!SlotTag.IsValid()) return;
	RemoveAbilityFromSlot(SlotTag);
	AbilitySlots.Remove(SlotTag);
}

bool UESAbilitySystemComponent::GiveAbilityToSlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 Level)
{
	if (!SlotTag.IsValid() || !AbilityClass) return false;

	RemoveAbilityFromSlot(SlotTag);
	AbilitySlots.FindOrAdd(SlotTag) = AbilityClass;

	if (!IsOwnerActorAuthoritative()) return false;

	FGameplayAbilitySpec AbilitySpec(AbilityClass, Level);
	AbilitySpec.GetDynamicSpecSourceTags().AddTag(SlotTag);

	const FGameplayAbilitySpecHandle GrantedHandle = GiveAbility(AbilitySpec);
	if (!GrantedHandle.IsValid()) return false;

	AbilityHandles.FindOrAdd(SlotTag) = GrantedHandle;
	return true;
}

bool UESAbilitySystemComponent::RemoveAbilityFromSlot(FGameplayTag SlotTag)
{
	if (!SlotTag.IsValid()) return false;
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
		bRemoved = AbilitySlots.Contains(SlotTag);
	}

	AbilitySlots.Remove(SlotTag);
	return bRemoved;
}

void UESAbilitySystemComponent::AssignAbilityToSlot(TSubclassOf<UGameplayAbility> AbilityClass, int32 SlotIndex)
{
	FGameplayTag SlotTag = GetSlotTagFromIndex(SlotIndex);
	for (FGameplayAbilitySpec& Spec : GetActivatableAbilities())
	{
		if (Spec.Ability->GetClass() == AbilityClass)
		{
			Spec.GetDynamicSpecSourceTags().AddTag(SlotTag);
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
	if (!AbilityTag.IsValid()) return false;

	TArray<FGameplayAbilitySpec*> FoundSpecs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(AbilityTag), FoundSpecs);

	if (FoundSpecs.Num() == 0) return false;

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
	if (!AbilityTag.IsValid()) return false;

	TArray<FGameplayAbilitySpec*> FoundSpecs;
	GetActivatableGameplayAbilitySpecsByAllMatchingTags(FGameplayTagContainer(AbilityTag), FoundSpecs);

	for (const FGameplayAbilitySpec* Spec : FoundSpecs)
	{
		if (Spec && Spec->IsActive())
		{
			return true;
		}
	}
	return false;
}

bool UESAbilitySystemComponent::IsAbilityActiveBySlotIndex(int32 SlotIndex) const
{
	FGameplayTag SlotTag = FGameplayTag::RequestGameplayTag(FName(FString::Printf(TEXT("Ability.Slot.%d"), SlotIndex)));
	return IsAbilityActiveByTag(SlotTag);
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
    if (SlotIndex < 0 || SlotIndex >= 6) return FGameplayTag();
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
    
    if (AbilityA) SetAbilitySlot(TagB, AbilityA);
    else ClearAbilitySlot(TagB);
    
    if (AbilityB) SetAbilitySlot(TagA, AbilityB);
    else ClearAbilitySlot(TagA);
    
    RemoveAbilityFromSlot(TagA);
    RemoveAbilityFromSlot(TagB);
    
    if (AbilityB) GiveAbilityToSlot(TagA, AbilityB, 1);
    if (AbilityA) GiveAbilityToSlot(TagB, AbilityA, 1);
    
    return true;
}

TArray<FGameplayTag> UESAbilitySystemComponent::GetDefaultSlotOrder() const
{
    return
    {
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.0")),
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.1")),
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.2")),
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.3")),
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.4")),
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.5")),
        FGameplayTag::RequestGameplayTag(TEXT("Ability.Slot.Passive"))
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
	if (!MyOwnerActor || !EventTag.IsValid()) return;

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = MyOwnerActor;
	EventData.Target = Target;
	EventData.EventMagnitude = Magnitude;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(MyOwnerActor, EventTag, EventData);
}

void UESAbilitySystemComponent::SendGameplayEventToTarget(FGameplayTag EventTag, AActor* Target, float Magnitude)
{
	AActor* MyOwnerActor = GetOwner();
	if (!MyOwnerActor || !Target || !EventTag.IsValid()) return;

	FGameplayEventData EventData;
	EventData.EventTag = EventTag;
	EventData.Instigator = MyOwnerActor;
	EventData.Target = Target;
	EventData.EventMagnitude = Magnitude;

	UAbilitySystemBlueprintLibrary::SendGameplayEventToActor(Target, EventTag, EventData);
}

void UESAbilitySystemComponent::InitializeAttributeChangeListeners()
{
	if (bAttributeListenersInitialized) return;

	const UESAttributeSet* AttributeSet = GetSet<UESAttributeSet>();
	if (!AttributeSet) return;

	GetGameplayAttributeValueChangeDelegate(UESAttributeSet::GetHealthAttribute()).AddUObject(this, &UESAbilitySystemComponent::HandleHealthChanged);
	GetGameplayAttributeValueChangeDelegate(UESAttributeSet::GetShieldAttribute()).AddUObject(this, &UESAbilitySystemComponent::HandleShieldChanged);
	GetGameplayAttributeValueChangeDelegate(UESAttributeSet::GetAttackSpeedAttribute()).AddUObject(this, &UESAbilitySystemComponent::HandleAttackSpeedChanged);
	
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