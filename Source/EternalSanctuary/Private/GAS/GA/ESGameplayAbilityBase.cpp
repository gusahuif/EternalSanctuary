#include "GAS/GA/ESGameplayAbilityBase.h"
#include "AbilitySystemComponent.h"
#include "AbilitySystemGlobals.h"
#include "Character/ESPlayerBase.h"
#include "Core/ESPlayerController.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "GAS/AS/ESAttributeSet.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/Character.h"

static const FString CDTagPrefix = TEXT("Ability.Cooldown");

UESGameplayAbilityBase::UESGameplayAbilityBase()
{
    InstancingPolicy = EGameplayAbilityInstancingPolicy::InstancedPerActor;
    SkillID = NAME_None;
    CurrentPlayingMontage = nullptr;
}

// ==========================================
// 【核心】DataTable 接口
// ==========================================

bool UESGameplayAbilityBase::GetSkillMetaData(FES_SkillMetaData& OutMetaData) const
{
    if (SkillID == NAME_None)
    {
        return false;
    }

    UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ESASC)
    {
        return false;
    }

    return ESASC->GetSkillMetaData(SkillID, OutMetaData);
}

// ==========================================
// 符文系统 (保持原样)
// ==========================================

bool UESGameplayAbilityBase::IsRuneActive(FGameplayTag RuneTag) const
{
    return ActiveRuneTags.HasTag(RuneTag);
}

bool UESGameplayAbilityBase::TryActivateRune(FGameplayTag RuneTag)
{
    if (IsInCombat())
    {
        UE_LOG(LogTemp, Warning, TEXT("[ESAbility] 战斗中无法切换符文：%s"), *RuneTag.ToString());
        return false;
    }

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
        UE_LOG(LogTemp, Warning, TEXT("[ESAbility] 符文 %s 不属于此技能"), *RuneTag.ToString());
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
    for (const FESRuneData& Rune : AvailableRunes)
    {
        if (ActiveRuneTags.HasTag(Rune.RuneTag))
        {
            ActiveRuneTags.RemoveTag(Rune.RuneTag);
            OnRuneChanged(Rune.RuneTag, false);
        }
    }
    
    for (FGameplayTag Tag : RuneTags)
    {
        if (!ActiveRuneTags.HasTag(Tag))
        {
            ActiveRuneTags.AddTag(Tag);
            OnRuneChanged(Tag, true);
        }
    }
}

// ==========================================
// 【GAS】CanActivate：集成 DataTable 检查
// ==========================================

bool UESGameplayAbilityBase::CanActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayTagContainer* SourceTags,
    const FGameplayTagContainer* TargetTags,
    FGameplayTagContainer* OptionalRelevantTags) const
{
    // 1. 先跑父类检查
    if (!Super::CanActivateAbility(Handle, ActorInfo, SourceTags, TargetTags, OptionalRelevantTags))
    {
        return false;
    }

    // 2. 检查通用禁止 Tag
    if (ActorInfo->AbilitySystemComponent.IsValid())
    {
        const UAbilitySystemComponent* ASC = ActorInfo->AbilitySystemComponent.Get();
        if (ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("ES.Status.Block.Skill"))))
        {
            return false;
        }
    }

    // 3. 如果没有配置 SkillID，直接通过
    FES_SkillMetaData Data;
    if (!GetSkillMetaData(Data))
    {
        return true;
    }

    UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ESASC) return false;

    // 4. 检查解锁
    if (Data.UnlockTag.IsValid() && !ESASC->HasMatchingGameplayTag(Data.UnlockTag))
    {
        return false;
    }

    // 5. 【绝对核心】检查 CD (无论是蓄力还是非蓄力，CD 中绝对不能激活)
    if (IsOnCooldown())
    {
        return false;
    }

    // 6. 【关键】如果不是蓄力技能，现在就检查蓝量
    if (!bIsChargeAbility)
    {
        if (!HasEnoughMana())
        {
            return false;
        }
    }

    // 所有检查通过
    return true;
}

// ==========================================
// 【GAS】Activate：集成 CD/Cost
// ==========================================
void UESGameplayAbilityBase::ActivateAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    const FGameplayEventData* TriggerEventData)
{
    FES_SkillMetaData Data;
    if (GetSkillMetaData(Data) && bIsChargeAbility)
    {
        // 如果是蓄力技能，现在就检查蓝！
        if (!HasEnoughMana())
        {
            // 没蓝！直接结束，不调用 Super，不调用 OnAbilityActivated
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }
    }
    
    // 1. Commit (GAS 规范)
    if (!CommitAbility(Handle, ActorInfo, ActivationInfo))
    {
        EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
        return;
    }

    // 2. 如果不是蓄力技能，立即扣蓝上 CD
    //FES_SkillMetaData Data;
    if (GetSkillMetaData(Data) && !bIsChargeAbility)
    {
        // 扣蓝
        if (!ConsumeMana())
        {
            EndAbility(Handle, ActorInfo, ActivationInfo, true, true);
            return;
        }

        // 上 CD
        ApplyCooldownFromDataTable();
    }

    // 3. 通知蓝图 (蓄力技能在这里开始蓄力)
    Super::ActivateAbility(Handle, ActorInfo, ActivationInfo, TriggerEventData);
    OnAbilityActivated();
}

void UESGameplayAbilityBase::EndAbility(
    const FGameplayAbilitySpecHandle Handle,
    const FGameplayAbilityActorInfo* ActorInfo,
    const FGameplayAbilityActivationInfo ActivationInfo,
    bool bReplicateEndAbility,
    bool bWasCancelled)
{
    if (CurrentPlayingMontage != nullptr)
    {
        if (ACharacter* Character = Cast<ACharacter>(GetAvatarActorFromActorInfo()))
        {
            if (UAnimInstance* AnimInstance = Character->GetMesh()->GetAnimInstance())
            {
                FOnMontageEnded EmptyDelegate;
                AnimInstance->Montage_SetEndDelegate(EmptyDelegate, CurrentPlayingMontage.Get());
            }
        }
        CurrentPlayingMontage = nullptr;
    }

    OnAbilityEnded(bWasCancelled);
    Super::EndAbility(Handle, ActorInfo, ActivationInfo, bReplicateEndAbility, bWasCancelled);
}

// ==========================================
// 【内部】CD & Cost 实现
// ==========================================

void UESGameplayAbilityBase::ApplyCooldownFromDataTable()
{
    FES_SkillMetaData Data;
    if (!GetSkillMetaData(Data)) return;

    UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ESASC) return;

    // 计算最终 CD：BaseCD * (1 - CDR)
    float CDR = 0.0f;
    const UESAttributeSet* AS = ESASC->GetSet<UESAttributeSet>();
    if (AS)
    {
        CDR = AS->GetCooldownReduction();
    }

    float FinalCD = Data.CooldownTime * (1.0f - CDR);
    FinalCD = FMath::Max(FinalCD, 0.1f);

    // 【修改】让 ASC 去处理
    ESASC->SetSkillCooldown(SkillID, FinalCD);
}

bool UESGameplayAbilityBase::ConsumeMana()
{
    FES_SkillMetaData Data;
    if (!GetSkillMetaData(Data)) return true; // 没数据默认不耗蓝

    if (Data.ManaCost <= 0.0f) return true;

    UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ESASC) return false;

    UESAttributeSet* AS = const_cast<UESAttributeSet*>(ESASC->GetSet<UESAttributeSet>());
    if (!AS) return false;

    float CurrentMana = AS->GetMana();
    if (CurrentMana < Data.ManaCost)
    {
        return false;
    }

    AS->SetMana(CurrentMana - Data.ManaCost);
    return true;
}

// ==========================================
// UI 接口实现
// ==========================================

float UESGameplayAbilityBase::GetCooldownRemaining() const
{
    UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ESASC) return 0.0f;

    return ESASC->GetSkillCooldownRemaining(SkillID);
}

float UESGameplayAbilityBase::GetCooldownTotal() const
{
    FES_SkillMetaData Data;
    if (GetSkillMetaData(Data))
    {
        return Data.CooldownTime;
    }
    return 0.0f;
}

bool UESGameplayAbilityBase::IsOnCooldown() const
{
    if (SkillID == NAME_None) return false;

    UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ESASC) return false;

    // 【修改】直接问 ASC
    return ESASC->IsSkillOnCooldown(SkillID);
}

bool UESGameplayAbilityBase::HasEnoughMana() const
{
    FES_SkillMetaData Data;
    if (!GetSkillMetaData(Data)) return true;
    if (Data.ManaCost <= 0.0f) return true;

    const UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ESASC) return false;

    const UESAttributeSet* AS = ESASC->GetSet<UESAttributeSet>();
    if (!AS) return false;

    return AS->GetMana() >= Data.ManaCost;
}

bool UESGameplayAbilityBase::IsUnlocked() const
{
    FES_SkillMetaData Data;
    if (!GetSkillMetaData(Data)) return true;
    if (!Data.UnlockTag.IsValid()) return true;

    const UESAbilitySystemComponent* ESASC = Cast<UESAbilitySystemComponent>(GetAbilitySystemComponentFromActorInfo());
    if (!ESASC) return false;

    return ESASC->HasMatchingGameplayTag(Data.UnlockTag);
}

// ==========================================
// 原有工具函数 (保持原样)
// ==========================================

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

// 蓄力技能确认发射
bool UESGameplayAbilityBase::ServerConfirmCharge()
{
    // 0. 如果技能已经结束了，直接返回失败
    if (!IsActive())
    {
        return false;
    }

    // 1. 查 DataTable
    FES_SkillMetaData Data;
    if (!GetSkillMetaData(Data))
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, false);
        return false;
    }

    // 2. 【最后一次严格检查】法力够不够？
    if (!HasEnoughMana())
    {
        // 蓝不够，直接结束技能，返回 false
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return false;
    }

    // 3. 扣蓝
    if (!ConsumeMana())
    {
        EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
        return false;
    }

    // 4. 上 CD
    ApplyCooldownFromDataTable();

    // 5. 【重要】返回 true，告诉蓝图可以生成子弹了
    return true;
}

void UESGameplayAbilityBase::OnSkillKeyboardRelease_Implementation()
{
}

bool UESGameplayAbilityBase::IsInCombat() const
{
    if (UAbilitySystemComponent* ASC = GetAbilitySystemComponentFromActorInfo())
    {
        return ASC->HasMatchingGameplayTag(FGameplayTag::RequestGameplayTag(FName("ES.Status.Block.SkillSwitch")));
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
    AnimInstance->Montage_SetPlayRate(AnimInstance->GetCurrentActiveMontage(), NewValue);
}

void UESGameplayAbilityBase::PlayAbilityMontage(UAnimMontage* Montage, float PlayRate)
{
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

    CurrentPlayingMontage = Montage;
    AnimInstance->Montage_Play(Montage, PlayRate);

    GetESAbilitySystemComponent()->OnAttackSpeedChanged.AddDynamic(this, &UESGameplayAbilityBase::OnAttackSpeedChanged);
    
    FOnMontageEnded EndDelegate;
    EndDelegate.BindUObject(this, &UESGameplayAbilityBase::OnMontageCompleted);
    AnimInstance->Montage_SetEndDelegate(EndDelegate, Montage);
}

void UESGameplayAbilityBase::OnMontageCompleted(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != CurrentPlayingMontage.Get())
    {
        return;
    }

    if (!IsActive())
    {
        return;
    }

    bool bWasCancelled = bInterrupted;
    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, bWasCancelled);
}

void UESGameplayAbilityBase::OnMontageCancelled(UAnimMontage* Montage, bool bInterrupted)
{
    if (Montage != CurrentPlayingMontage.Get())
    {
        return;
    }

    if (!IsActive())
    {
        return;
    }

    EndAbility(CurrentSpecHandle, CurrentActorInfo, CurrentActivationInfo, true, true);
}