#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "ESCharacterBase.generated.h"

class UGameplayEffect;
class UESAbilitySystemComponent;
class UESAttributeSet;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

/**
 * GAS 角色基类
 * 用于统一承载 ASC、AttributeSet、属性监听、死亡逻辑与初始化链路
 */
UCLASS()
class ETERNALSANCTUARY_API AESCharacterBase : public ACharacter, public IAbilitySystemInterface
{
	GENERATED_BODY()

public:
	AESCharacterBase(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

protected:
	virtual void BeginPlay() override;
	virtual void PossessedBy(AController* NewController) override;
	virtual void OnRep_PlayerState() override;

public:
	// IAbilitySystemInterface
	virtual UAbilitySystemComponent* GetAbilitySystemComponent() const override;

public:
	/** 获取项目自定义 ASC，便于子类直接使用 */
	UFUNCTION(BlueprintPure, Category = "ES|AbilitySystem")
	UESAbilitySystemComponent* GetESAbilitySystemComponent() const { return AbilitySystemComponent; }

	/** 获取 AttributeSet */
	UFUNCTION(BlueprintPure, Category = "ES|AbilitySystem")
	UESAttributeSet* GetESAttributeSet() const { return AttributeSet; }

	/** 是否存活：通过 Dead Tag 判断，而不是仅看 Health 是否大于 0 */
	UFUNCTION(BlueprintPure, Category = "ES|Character")
	virtual bool IsAlive() const;

	/** 角色死亡：默认添加 Dead Tag、停止移动、关闭基础移动能力 */
	UFUNCTION(BlueprintCallable, Category = "ES|Character")
	virtual void Die();

	/** 默认属性初始化：建议子类通过 GameplayEffect 初始化基础数值 */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	virtual void InitializeDefaultAttributes();

	/** 授予默认技能：建议只在服务器调用 */
	UFUNCTION(BlueprintCallable, Category = "ES|AbilitySystem")
	virtual void GiveDefaultAbilities();

public:
	/** 快捷属性访问 */
	UFUNCTION(BlueprintPure, Category = "ES|Attribute")
	float GetHealth() const;

	UFUNCTION(BlueprintPure, Category = "ES|Attribute")
	float GetMaxHealth() const;

	UFUNCTION(BlueprintPure, Category = "ES|Attribute")
	float GetShield() const;

	UFUNCTION(BlueprintPure, Category = "ES|Attribute")
	float GetMaxShield() const;

protected:
	/** 初始化 ASC 的 ActorInfo，并绑定属性变化监听 */
	virtual void InitializeAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor);

	/** 绑定属性变化回调，避免重复绑定 */
	virtual void BindAttributeDelegates();

	/** 属性变化响应 */
	virtual void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleShieldChanged(const FOnAttributeChangeData& ChangeData);

protected:
	/** 当前角色拥有的 ASC，玩家与敌人都共用这套基类 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UESAbilitySystemComponent> AbilitySystemComponent;

	/** 属性集，挂在角色上并由 ASC 管理属性数据 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UESAttributeSet> AttributeSet;

	/** 避免重复初始化和重复绑定 */
	UPROPERTY(Transient)
	bool bAbilityActorInfoInitialized = false;

	UPROPERTY(Transient)
	bool bAttributeDelegatesBound = false;
};
