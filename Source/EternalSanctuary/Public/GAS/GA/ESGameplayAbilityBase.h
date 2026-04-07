#pragma once

#include "CoreMinimal.h"
#include "Abilities/GameplayAbility.h"
#include "GameplayTagContainer.h"
#include "ESGameplayAbilityBase.generated.h"

class UESAbilitySystemComponent;
class AESPlayerBase;
class AESPlayerController;
class UAnimMontage;
class UGameplayEffect;

// ── 技能分类枚举（设计层标签，影响资源消耗/CD 规则）──
UENUM(BlueprintType)
enum class EESAbilityCategory : uint8
{
    Slot0       UMETA(DisplayName = "槽位 0 (左键)"),
    Slot1       UMETA(DisplayName = "槽位 1 (右键)"),
    Slot2       UMETA(DisplayName = "槽位 2 (技能 1)"),
    Slot3       UMETA(DisplayName = "槽位 3 (技能 2)"),
    Slot4       UMETA(DisplayName = "槽位 4 (技能 3)"),
    Slot5       UMETA(DisplayName = "槽位 5 (技能 4)"),
    Passive     UMETA(DisplayName = "被动技能"),
    Dodge       UMETA(DisplayName = "闪避"),
    Potion      UMETA(DisplayName = "药水"),
    Common      UMETA(DisplayName = "通用技能")  // 不绑定特定槽位
};

// ── 符文数据结构 ──
USTRUCT(BlueprintType)
struct FESRuneData
{
    GENERATED_BODY()

    // 符文唯一 ID（用于装备词条也能激活同一个符文）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    FGameplayTag RuneTag;

    // 符文显示名
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    FText RuneName;

    // 符文描述（显示在 UI）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    FText RuneDescription;

    // 符文图标
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    TObjectPtr<UTexture2D> RuneIcon;

    // 符文槽位（1/2/3）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Rune")
    int32 SlotIndex = 1;

    FESRuneData() : SlotIndex(1) {}
};

/**
 * UESGameplayAbilityBase
 * 所有技能的公共基类，定义分类规则、符文框架、输入绑定、锁血被动接口
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class ETERNALSANCTUARY_API UESGameplayAbilityBase : public UGameplayAbility
{
    GENERATED_BODY()

public:
    UESGameplayAbilityBase();

    // ──────────────────────────────────────────────
    //  【设计配置区】在子类/蓝图中填写
    // ──────────────────────────────────────────────

    // 技能显示名
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Ability")
    FText AbilityName;

    // 技能描述
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Ability")
    FText AbilityDescription;

    // 技能图标
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Ability")
    TObjectPtr<UTexture2D> AbilityIcon;

    // 技能输入 Tag（对应 ES.Input.LMB / RMB / Skill1...）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Ability")
    FGameplayTag InputTag;

    // 技能动画 Montage
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Ability")
    TObjectPtr<UAnimMontage> AbilityMontage;

    // 蓝耗值（如果不用 GE，可直接用这个值）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Ability")
    float ManaCost = 0.0f;

    // ──────────────────────────────────────────────
    //  【符文系统】
    // ──────────────────────────────────────────────

    // 此技能支持的 3 个符文定义（在子类蓝图中配置）
    UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Rune")
    TArray<FESRuneData> AvailableRunes;

    // 当前激活的符文 Tag（可以是 0 个，也可以同时激活多个来自装备的符文）
    UPROPERTY(BlueprintReadOnly, Category = "ES|Rune")
    FGameplayTagContainer ActiveRuneTags;

    // 检查某个符文是否已激活（装备词条和主动选择均通过此方法判断）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|Rune")
    bool IsRuneActive(FGameplayTag RuneTag) const;

    // 激活符文（主城切换时调用，战斗中被 Block.SkillSwitch 阻止）
    UFUNCTION(BlueprintCallable, Category = "ES|Rune")
    bool TryActivateRune(FGameplayTag RuneTag);

    // 停用符文
    UFUNCTION(BlueprintCallable, Category = "ES|Rune")
    void DeactivateRune(FGameplayTag RuneTag);

    // 由外部（装备系统）强制激活符文，不受战斗限制检测
    UFUNCTION(BlueprintCallable, Category = "ES|Rune")
    void ForceActivateRuneFromEquipment(FGameplayTag RuneTag);
    
    // ──────────────────────────────────────────────
    // 【UI 交互接口】供 UI 蓝图调用
    // ──────────────────────────────────────────────

    /** 获取当前技能已激活的所有符文 Tag */
    UFUNCTION(BlueprintCallable, Category = "ES|Rune")
    FGameplayTagContainer GetActiveRunes() const { return ActiveRuneTags; }

    /** 获取此技能可用的所有符文数据 */
    UFUNCTION(BlueprintCallable, Category = "ES|Rune")
    TArray<FESRuneData> GetAvailableRunes() const { return AvailableRunes; }

    /** 切换符文状态（UI 专用，自动判断激活/停用） */
    UFUNCTION(BlueprintCallable, Category = "ES|Rune")
    bool ToggleRune(FGameplayTag RuneTag);

    /** 获取符文当前是否可切换（检查战斗状态等限制） */
    UFUNCTION(BlueprintPure, Category = "ES|Rune")
    bool CanToggleRune() const;

    /** 外部系统（如装备）批量设置符文 */
    UFUNCTION(BlueprintCallable, Category = "ES|Rune")
    void SetRunesFromEquipment(const TArray<FGameplayTag>& RuneTags);

    // ──────────────────────────────────────────────
    //  【GAS 标准覆写】
    // ──────────────────────────────────────────────

    virtual bool CanActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayTagContainer* SourceTags,
        const FGameplayTagContainer* TargetTags,
        FGameplayTagContainer* OptionalRelevantTags) const override;

    virtual void ActivateAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        const FGameplayEventData* TriggerEventData) override;

    virtual void EndAbility(
        const FGameplayAbilitySpecHandle Handle,
        const FGameplayAbilityActorInfo* ActorInfo,
        const FGameplayAbilityActivationInfo ActivationInfo,
        bool bReplicateEndAbility,
        bool bWasCancelled) override;

    // ──────────────────────────────────────────────
    //  【子类实现接口】
    // ──────────────────────────────────────────────

    // 子类实现具体技能逻辑（替代直接覆写 ActivateAbility）
    UFUNCTION(BlueprintImplementableEvent, Category = "ES|Ability")
    void OnAbilityActivated();

    // 技能结束时通知子类
    UFUNCTION(BlueprintImplementableEvent, Category = "ES|Ability")
    void OnAbilityEnded(bool bWasCancelled);

    // 符文状态变化时通知（子类可重写改变技能行为）
    UFUNCTION(BlueprintImplementableEvent, Category = "ES|Rune")
    void OnRuneChanged(FGameplayTag RuneTag, bool bActivated);

    // ──────────────────────────────────────────────
    //  【工具函数】
    // ──────────────────────────────────────────────

    // 获取施法者 PlayerBase
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|Ability")
    AESPlayerBase* GetESOwnerCharacter() const;

    // 获取 PlayerController
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|Ability")
    AESPlayerController* GetESPlayerController() const;

    // 获取 ASC
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|Ability")
    UESAbilitySystemComponent* GetESAbilitySystemComponent() const;

    // 播放 Montage 并在结束时自动 EndAbility
    UFUNCTION(BlueprintCallable, Category = "ES|Ability")
    void PlayAbilityMontage(UAnimMontage* Montage = nullptr, float PlayRate = 1.0f);

    // 添加/移除状态 Tag（统一入口）
    UFUNCTION(BlueprintCallable, Category = "ES|Ability")
    void AddStatusTag(FGameplayTag Tag);

    UFUNCTION(BlueprintCallable, Category = "ES|Ability")
    void RemoveStatusTag(FGameplayTag Tag);

    // 是否处于战斗状态（用于阻止技能/符文切换）
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|Ability")
    bool IsInCombat() const;

    // 检查 Ability 是否处于激活状态
    UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|Ability")
    bool IsActive() const;

    // 尝试提交能力（消耗资源 + 进入 CD）
    UFUNCTION(BlueprintCallable, Category = "ES|Ability")
    bool TryCommitAbility();

    UFUNCTION()
    void OnAttackSpeedChanged(float NewValue, float OldValue);
protected:

    // 根据 AbilityCategory 自动应用 Cost/Cooldown GE
    void ApplyCategoryRules();

    // Montage 结束回调（签名必须匹配 FOnMontageEnded）
    void OnMontageCompleted(UAnimMontage* Montage, bool bInterrupted);
    
    // Montage 被取消回调
    void OnMontageCancelled(UAnimMontage* Montage, bool bInterrupted);

    // 委托句柄
    FDelegateHandle MontageEndedHandle;
    FDelegateHandle MontageCancelledHandle;
    
    // 当前正在播放的 Montage
    TObjectPtr<UAnimMontage> CurrentPlayingMontage;
};
