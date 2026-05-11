#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "GAS/Data/ESAbilityMetaData.h" // 【新增】
#include "ESAbilitySystemComponent.generated.h"

class UESAttributeSet;
class AActor;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FESOnHealthChangedSignature, float, NewValue, float, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FESOnShieldChangedSignature, float, NewValue, float, OldValue);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FESOnAttackSpeedChangedSignature, float, NewValue, float, OldValue);

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETERNALSANCTUARY_API UESAbilitySystemComponent : public UAbilitySystemComponent
{
	GENERATED_BODY()

public:
	UESAbilitySystemComponent();

public:
	UPROPERTY(BlueprintAssignable, Category="ES|AbilitySystem")
	FESOnHealthChangedSignature OnHealthChanged;

	UPROPERTY(BlueprintAssignable, Category="ES|AbilitySystem")
	FESOnShieldChangedSignature OnShieldChanged;

	UPROPERTY(BlueprintAssignable, Category="ES|AbilitySystem")
	FESOnAttackSpeedChangedSignature OnAttackSpeedChanged;

	// ==========================================
	// 全局技能数据表
	// ==========================================

	/** 全局技能数据表（在蓝图里配置） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Data")
	UDataTable* SkillDataTable;

	/** 根据 SkillID 获取元数据 */
	UFUNCTION(BlueprintCallable, Category = "ES|Data")
	bool GetSkillMetaData(FName SkillID, FES_SkillMetaData& OutMetaData) const;

	// ==========================================
	// 统一冷却管理 (给 GA 和 UI 用)
	// ==========================================

	/** 设置技能冷却 */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	void SetSkillCooldown(FName InSkillID, float Duration);

	/** 检查技能是否在冷却 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|AbilitySystem")
	bool IsSkillOnCooldown(FName InSkillID) const;

	/** 获取技能剩余冷却时间 (UI用) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|AbilitySystem")
	float GetSkillCooldownRemaining(FName InSkillID) const;

	/** 获取技能总冷却时间 (UI用) */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|AbilitySystem")
	float GetSkillCooldownTotal(FName InSkillID) const;
protected:
	/** 存储冷却结束时间：SkillID -> WorldTimeSeconds */
	UPROPERTY()
	TMap<FName, float> SkillCooldownMap;
	
public:
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

public:
public:
	// ==========================================
	// 【技能/符文系统】UI 统一接口
	// ==========================================

	/** 获取所有已授予的技能 GA 实例（用于 UI 遍历） */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem|UI")
	TArray<UESGameplayAbilityBase*> GetAllESAbilities() const;

	/** 
	 * 尝试解锁技能（消耗强化点）
	 * @param Ability 要解锁的技能
	 * @param Cost 消耗点数
	 * @return 是否成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem|Unlock")
	bool TryUnlockSkill(UESGameplayAbilityBase* Ability, int32 Cost = 1);

	/** 
	 * 尝试解锁并激活符文（消耗强化点）
	 * @param Ability 所属技能
	 * @param RuneTag 要激活的符文 Tag
	 * @param Cost 消耗点数
	 * @return 是否成功
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem|Unlock")
	bool TryUnlockAndActivateRune(UESGameplayAbilityBase* Ability, FGameplayTag RuneTag, int32 Cost = 1);

	/** 
	 * 获取组合后的描述文本（技能描述 + 已激活符文描述）
	 * @param Ability 技能
	 * @return 最终显示的描述
	 */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|AbilitySystem|UI")
	FText GetFullAbilityDescription(UESGameplayAbilityBase* Ability) const;
	
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	void SetAbilitySlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass);

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	TSubclassOf<UGameplayAbility> GetAbilitySlot(FGameplayTag SlotTag) const;

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	void ClearAbilitySlot(FGameplayTag SlotTag);

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool GiveAbilityToSlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 Level = 1);

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool RemoveAbilityFromSlot(FGameplayTag SlotTag);
	
	UFUNCTION(BlueprintCallable, Category = "Ability|Slot")
	void AssignAbilityToSlot(TSubclassOf<UGameplayAbility> AbilityClass, int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Ability|Slot")
	UGameplayAbility* GetAbilityInSlot(int32 SlotIndex) const;

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	void GiveDefaultAbilities(const TArray<TSubclassOf<UGameplayAbility>>& DefaultAbilities);

	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	bool TryActivateAbilityByTag(FGameplayTag AbilityTag);

	UFUNCTION(BlueprintPure, Category = "ES|AbilitySystem")
	bool IsAbilityActiveByTag(FGameplayTag AbilityTag) const;
	
	bool IsAbilityActiveBySlotIndex(int32 SlotIndex) const;
	
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	bool TryActivateAbilityBySlotIndex(int32 SlotIndex);

	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	bool SetAbilityToSlot(int32 SlotIndex, TSubclassOf<UGameplayAbility> AbilityClass, int32 Level = 1);

	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	bool SwapAbilitySlots(int32 SlotA, int32 SlotB);

	UFUNCTION(BlueprintPure, Category = "ES|AbilitySystem")
	int32 GetActiveSlotCount() const { return 6; }

public:
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool IsInvincible() const;

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool IsDead() const;

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool HasSuperArmor() const;

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool IsStunned() const;

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool IsSilenced() const;

public:
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	void SendGameplayEventToSelf(FGameplayTag EventTag, AActor* Target = nullptr, float Magnitude = 0.f);

	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	void SendGameplayEventToTarget(FGameplayTag EventTag, AActor* Target, float Magnitude = 0.f);

	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	void InitializeAttributeChangeListeners();

protected:
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	void HandleShieldChanged(const FOnAttributeChangeData& ChangeData);
	void HandleAttackSpeedChanged(const FOnAttributeChangeData& ChangeData);
	
	TArray<FGameplayTag> GetDefaultSlotOrder() const;
	
protected:
	FGameplayTag GetSlotTagFromIndex(int32 SlotIndex) const;
	bool bAttributeListenersInitialized = false;

protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ES|AbilitySystem")
	TMap<FGameplayTag, TSubclassOf<UGameplayAbility>> AbilitySlots;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ES|AbilitySystem")
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> AbilityHandles;
};