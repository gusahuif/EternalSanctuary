#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GameplayAbilitySpec.h"
#include "Abilities/GameplayAbility.h"
#include "ESAbilitySystemComponent.generated.h"

class UESAttributeSet;
class AActor;

/**
 * EternalSanctuary 自定义 AbilitySystemComponent
 * 负责技能槽位管理、技能授予/移除、状态 Tag 查询、GameplayEvent 派发以及属性监听
 */
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
	/** 生命变化广播：用于 UI、受击反馈、死亡判断等业务层响应 */
	UPROPERTY(BlueprintAssignable, Category="ES|AbilitySystem")
	FESOnHealthChangedSignature OnHealthChanged;

	/** 护盾变化广播：用于 UI、护盾特效、战斗反馈等业务层响应 */
	UPROPERTY(BlueprintAssignable, Category="ES|AbilitySystem")
	FESOnShieldChangedSignature OnShieldChanged;

	// 监听攻速的变化，用来改变当前技能的播放速率
	UPROPERTY(BlueprintAssignable, Category="ES|AbilitySystem")
	FESOnAttackSpeedChangedSignature OnAttackSpeedChanged;

public:
	/** 重写 ASC 初始化：在 ActorInfo 建立完成后绑定属性变化监听 */
	virtual void InitAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor) override;

public:
	/** 设置槽位映射，仅更新槽位配置，不执行真正授予 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	void SetAbilitySlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass);

	/** 获取槽位中配置的技能类 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	TSubclassOf<UGameplayAbility> GetAbilitySlot(FGameplayTag SlotTag) const;

	/** 清空槽位映射，同时移除已授予的技能 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	void ClearAbilitySlot(FGameplayTag SlotTag);

	/** 向指定槽位授予技能；若已有技能则先移除旧技能 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool GiveAbilityToSlot(FGameplayTag SlotTag, TSubclassOf<UGameplayAbility> AbilityClass, int32 Level = 1);

	/** 从指定槽位移除技能 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool RemoveAbilityFromSlot(FGameplayTag SlotTag);
	
	UFUNCTION(BlueprintCallable, Category = "Ability|Slot")
	void AssignAbilityToSlot(TSubclassOf<UGameplayAbility> AbilityClass, int32 SlotIndex);

	UFUNCTION(BlueprintPure, Category = "Ability|Slot")
	UGameplayAbility* GetAbilityInSlot(int32 SlotIndex) const;

	/** 按 Basic/Core/Mobility/Status/Defense/Ultimate/Passive 顺序批量授予默认技能 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	void GiveDefaultAbilities(const TArray<TSubclassOf<UGameplayAbility>>& DefaultAbilities);

	/** 通过 Tag 激活技能（用于普通攻击） */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	bool TryActivateAbilityByTag(FGameplayTag AbilityTag);

	/** 检查指定 Tag 的技能是否正在激活 */
	UFUNCTION(BlueprintPure, Category = "ES|AbilitySystem")
	bool IsAbilityActiveByTag(FGameplayTag AbilityTag) const;

	/** 按槽位索引激活技能（0-5 对应 L/R/1/2/3/4） */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	bool TryActivateAbilityBySlotIndex(int32 SlotIndex);

	/** 获取槽位索引中的技能类 */
	//UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	//TSubclassOf<UGameplayAbility> GetAbilityInSlot(int32 SlotIndex) const;

	/** 设置技能到指定槽位索引 */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	bool SetAbilityToSlot(int32 SlotIndex, TSubclassOf<UGameplayAbility> AbilityClass, int32 Level = 1);

	/** 交换两个槽位的技能 */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	bool SwapAbilitySlots(int32 SlotA, int32 SlotB);

	/** 获取所有主动槽位的数量（固定为 6） */
	UFUNCTION(BlueprintPure, Category = "ES|AbilitySystem")
	int32 GetActiveSlotCount() const { return 6; }


public:
	/** 是否处于无敌状态 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool IsInvincible() const;

	/** 是否已死亡 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool IsDead() const;

	/** 是否拥有霸体 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool HasSuperArmor() const;

	/** 是否被眩晕 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool IsStunned() const;

	/** 是否被沉默 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	bool IsSilenced() const;

public:
	/**
	 * 事件发送给自己的 ASC
	 * Instigator = 自己
	 * EventData.Target = 传入的 Target
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	void SendGameplayEventToSelf(FGameplayTag EventTag, AActor* Target = nullptr, float Magnitude = 0.f);

	/**
	 * 事件发送给目标 Actor 的 ASC
	 * Instigator = 自己
	 * EventData.Target = 传入的 Target
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	void SendGameplayEventToTarget(FGameplayTag EventTag, AActor* Target, float Magnitude = 0.f);

	/** 初始化属性变化监听；建议仅在 ActorInfo 完整建立后调用 */
	UFUNCTION(BlueprintCallable, Category="ES|AbilitySystem")
	void InitializeAttributeChangeListeners();

protected:
	/** 生命属性变化回调：将 GAS 层变化转发给 UI/角色表现层 */
	void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);

	/** 护盾属性变化回调：将 GAS 层变化转发给 UI/角色表现层 */
	void HandleShieldChanged(const FOnAttributeChangeData& ChangeData);

	// 攻速变化的回调
	void HandleAttackSpeedChanged(const FOnAttributeChangeData& ChangeData);
	
	/** 获取项目中固定的 7 个技能槽位顺序 */
	TArray<FGameplayTag> GetDefaultSlotOrder() const;
	
protected:
	/** 将槽位索引转换为 GameplayTag */
	FGameplayTag GetSlotTagFromIndex(int32 SlotIndex) const;

	/** 用于避免重复绑定属性变化监听 */
	bool bAttributeListenersInitialized = false;

protected:
	/** 槽位 -> 技能类映射，仅表示当前槽位配置 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ES|AbilitySystem")
	TMap<FGameplayTag, TSubclassOf<UGameplayAbility>> AbilitySlots;

	/** 槽位 -> 已授予技能 Handle 映射，用于后续移除/替换 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="ES|AbilitySystem")
	TMap<FGameplayTag, FGameplayAbilitySpecHandle> AbilityHandles;
};
