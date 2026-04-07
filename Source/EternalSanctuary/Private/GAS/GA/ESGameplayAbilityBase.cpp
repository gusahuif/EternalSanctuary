#include "GAS/GA/ESGameplayAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/ESPlayerBase.h"
#include "Core/ESPlayerController.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"

UESGameplayAbilityBase::UESGameplayAbilityBase()
{
    // 默认实例化策略：每次激活都创建新实例（支持同时多技能并行）
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    
   
    ManaCost = 0.0f;
    CurrentPlayingMontage = nullptr;
}

// ──────────────────────────────────────────────────────────────────
//  符文系统
// ──────────────────────────────────────────────────────────────────

bool UESGameplayAbilityBase::IsRuneActive(FGameplayTag RuneTag) const
{
    return ActiveRuneTags.HasTag(RuneTag);
}

bool UESGameplayAbilityBase::TryActivateRune(FGameplayTag RuneTag)
{
    // 战斗中禁止切换符文
    if (IsInCombat())
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ESAbility] 战斗中无法切换符文：%s"), *RuneTag.ToString());
        return false;
    }

    // 检查符文是否属于本技能的可用列表
    bool bFound = false;
    for (const FESRuneData& Rune : AvailableRunes)
    {
        if (Rune.RuneTag == RuneTag)
        {
            bFound = true;
            break;
        }
    }

    if (!bFound)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("[ESAbility] 符文 %s 不属于此技能"), *RuneTag.ToString());
        return false;
    }

    ActiveRuneTags.AddTag(RuneTag);
    OnRuneChanged(RuneTag, true);
    return true;
}

void UESGameplayAbilityBase::DeactivateRune(FGameplayTag RuneTag)
{
    if (IsInCombat())
    {
        return;
    }
    ActiveRuneTags.RemoveTag(RuneTag);
    OnRuneChanged(RuneTag, false);
}

void UESGameplayAbilityBase::ForceActivateRuneFromEquipment(FGameplayTag RuneTag)
{
    // 装备触发的符文不受战斗限制
    if (!ActiveRuneTags.HasTag(RuneTag))
    {
        ActiveRuneTags.AddTag(RuneTag);
        OnRuneChanged(RuneTag, true);
    }
}
bool UESGameplayAbilityBase::ToggleRune(FGameplayTag RuneTag)
{
    if (!RuneTag.IsValid()) return false;
    
    if (ActiveRuneTags.HasTag(RuneTag))
    {
        DeactivateRune(RuneTag);
        return false;
    }
    else
    {
        return TryActivateRune(RuneTag);
    }
}

bool UESGameplayAbilityBase::CanToggleRune() const
{
    return !IsInCombat();
}

void UESGameplayAbilityBase::SetRunesFromEquipment(const TArray<FGameplayTag>& RuneTags)
{
    // 先清空所有来自装备的符文
    for (const FESRuneData& Rune : AvailableRunes)
    {
        if (ActiveRuneTags.HasTag(Rune.RuneTag))
        {
            ActiveRuneTags.RemoveTag(Rune.RuneTag);
            OnRuneChanged(Rune.RuneTag, false);
        }
    }
    
    // 再添加新的装备符文
    for (FGameplayTag Tag : RuneTags)
    {
        if (!ActiveRuneTags.HasTag(Tag))
        {
            ActiveRuneTags.AddTag(Tag);
            OnRuneChanged(Tag, true);
        }
    }
}


// ──────────────────────────────────────────────────────────────────
//  GAS 标准覆写
// ──────────────────────────────────────────────────────────────────

bool UESGameplayAbilityBase::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    // 检查是否被禁止使用技能
    if (ActorInfo->AbilitySystemComponent.IsValid())
    {
        const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
        if (ASC->HasMatchingGameplayTag(
            FGameplayTag::RequestGameplayTag(FName("ES.Status.Block.Skill"))))
        {
            return false;
        }
    }

    return true;
}

void UESGameplayAbilityBase::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);

    // 通知蓝图子类执行具体逻辑
    OnAbilityActivated();
}

void UESGameplayAbilityBase::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    // 清理 Montage 委托
    if (CurrentPlayingMontage != nullptr)
    {
        if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
        {
            if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
            {
                // 创建委托变量（不能传临时对象）
                FOnMontageEnded EmptyDelegate;
                AnimInstance->Montage_SetEndDelegate(EmptyDelegate, CurrentPlayingMontage.Get());
            }
        }
        CurrentPlayingMontage = nullptr;
    }

    OnAbilityEnded(bWasCancelled);
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ──────────────────────────────────────────────────────────────────
//  分类规则：自动处理 CD / Cost
// ──────────────────────────────────────────────────────────────────

void UESGameplayAbilityBase::ApplyCategoryRules()
{
    // 注意：CostGameplayEffectClass 和 CooldownGameplayEffectClass 是父类 UGameplayAbility 的属性
    // 直接在蓝图中配置即可，CommitAbility 会自动应用它们
    
    // 尝试提交能力（消耗资源 + 进入 CD）
    CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
}

// ──────────────────────────────────────────────────────────────────
//  工具函数
// ──────────────────────────────────────────────────────────────────

AESPlayerBase* UESGameplayAbilityBase::GetESOwnerCharacter() const
{
    return Cast<AESPlayerBase>(GetAvatarActorFromActorInfo());
}

AESPlayerController* UESGameplayAbilityBase::GetESPlayerController() const
{
    if (AESPlayerBase* Player = GetESOwnerCharacter())
    {
        return Cast<AESPlayerController>(Player->GetController());
    }
    return nullptr;
}

UESAbilitySystemComponent* UESGameplayAbilityBase::GetESAbilitySystemComponent() const
{
    return Cast<UESAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
}

void UESGameplayAbilityBase::AddStatusTag(FGameplayTag Tag)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->AddLooseGameplayTag(Tag);
    }
}

void UESGameplayAbilityBase::RemoveStatusTag(FGameplayTag Tag)
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        ASC->RemoveLooseGameplayTag(Tag);
    }
}

bool UESGameplayAbilityBase::IsInCombat() const
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        return ASC->HasMatchingGameplayTag(
            FGameplayTag::RequestGameplayTag(FName("ES.Status.Block.SkillSwitch")));
    }
    return false;
}

bool UESGameplayAbilityBase::IsActive() const
{
    return Super::IsActive();
}

bool UESGameplayAbilityBase::TryCommitAbility()
{
    return CommitAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo);
}

void UESGameplayAbilityBase::OnAttackSpeedChanged(float NewValue, float OldValue)
{
    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character) return;
    UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
    if (!AnimInstance) return;
    AnimInstance->Montage_SetPlayRate(AnimInstance->GetCurrentActiveMontage(),NewValue);
}

void UESGameplayAbilityBase::PlayAbilityMontage(UAnimMontage* Montage, float PlayRate)
{
    // 如果未指定 Montage，使用默认配置的
    if (!Montage)
    {
        Montage = AbilityMontage;
    }
    
    if (!Montage)
    {
        UE_LOG(LogTemp, Warning, TEXT("[ESAbility] 未指定 Montage，跳过播放"));
        return;
    }

    ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo());
    if (!Character)
    {
        UE_LOG(LogTemp, Error, TEXT("[ESAbility] 无法获取 Character，跳过 Montage 播放"));
        return;
    }

    UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance();
    if (!AnimInstance)
    {
        UE_LOG(LogTemp, Error, TEXT("[ESAbility] 无法获取 AnimInstance，跳过 Montage 播放"));
        return;
    }

    // 记录当前播放的 Montage
    CurrentPlayingMontage = Montage;

    // 播放 Montage
    AnimInstance->Montage_Play(Montage, PlayRate);

    GetESAbilitySystemComponent()->OnAttackSpeedChanged.AddDynamic(this, &UESGameplayAbilityBase::OnAttackSpeedChanged);
    // 绑定结束委托
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UESGameplayAbilityBase::OnMontageCompleted);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
}

void UESGameplayAbilityBase::OnMontageCompleted(UAnimMontage* Montage, bool bInterrupted)
{
    // 只响应当前技能自己播放的 Montage
    if (Montage != CurrentPlayingMontage.Get())
    {
        return;
    }

    // 如果技能已不在激活态，说明可能已被其他逻辑提前结束
    if (!IsActive())
    {
        return;
    }

    // 如果被中断，标记为取消
    bool bWasCancelled = bInterrupted;

    // 正常结束或被中断，都调用 EndAbility
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UESGameplayAbilityBase::OnMontageCancelled(UAnimMontage* Montage, bool bInterrupted)
{
    // 注意：此函数在 UE5.7 中可能不会被调用，因为 Montage_SetCancelDelegate 已移除
    // 取消逻辑统一在 OnMontageCompleted 中通过 bInterrupted 参数处理
    
    // 只响应当前技能自己播放的 Montage
    if (Montage != CurrentPlayingMontage.Get())
    {
        return;
    }

    // 如果技能已不在激活态，说明可能已被其他逻辑提前结束
    if (!IsActive())
    {
        return;
    }

    // 被取消，调用 EndAbility 并标记为已取消
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}
