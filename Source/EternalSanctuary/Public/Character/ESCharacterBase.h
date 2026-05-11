#pragma once

#include "CoreMinimal.h"
#include "AbilitySystemInterface.h"
#include "GameFramework/Character.h"
#include "GameplayTagContainer.h"
#include "ESCharacterBase.generated.h"

class UNiagaraSystem;
struct FGameplayEventData;
class UGameplayEffect;
class UESAbilitySystemComponent;
class UESAttributeSet;
class UAbilitySystemComponent;
struct FOnAttributeChangeData;

UENUM(BlueprintType)
enum class EESFaction : uint8
{
	Player		UMETA(DisplayName = "Player"),		// 玩家阵营
	Enemy		UMETA(DisplayName = "Enemy"),		// 敌人阵营
	Neutral		UMETA(DisplayName = "Neutral")		// 中立阵营
};


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

	/** 统一的 Tag 检查工具函数，避免外部到处直接写 Tag 查询 */
	bool HasMatchingGameplayTag(FGameplayTag TagToCheck) const;
	
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
	/** 获取阵营 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	EESFaction GetFaction() const { return Faction; }

	/** 判断另一个 Actor 是否是敌对单位 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	virtual bool IsHostileToActor(const AActor* OtherActor) const;
	
	/** 判断当前是否死亡 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	virtual bool IsDead() const;

	/** 判断当前是否处于受击硬直状态 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	virtual bool IsInHitReact() const;

	/** 判断当前是否处于霸体状态 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	virtual bool HasSuperArmor() const;

	/** 判断当前是否允许移动/追击/攻击等主动行为 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	virtual bool CanAct() const;

	/** 判断当前是否可以成为 AI 的有效目标 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	virtual bool CanBeTargeted() const;

	/** 判断当前是否可以被普通受击打断 */
	UFUNCTION(BlueprintCallable, Category = "ES|Combat")
	virtual bool CanBeInterrupted() const;
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

public:
	/**
	 * 在指定位置显示伤害/治疗数字
	 * @param DamageValue 伤害值（正数=伤害，负数=治疗）
	 * @param bIsCritical 是否暴击
	 * @param WorldLocation 显示位置（如果是ZeroVector则用锚点）
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|DamageNumber")
	virtual void ShowDamageNumber(float DamageValue, bool bIsCritical, const FVector& WorldLocation);

	/** 获取伤害数字锚点的世界位置 */
	UFUNCTION(BlueprintPure, Category = "ES|DamageNumber")
	FVector GetDamageNumberAnchorLocation() const;

	

public:
	/** 伤害数字Niagara系统（必须在蓝图里配置！） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|DamageNumber")
	UNiagaraSystem* DamageNumberNiagara;
	
protected:
	/** 初始化 ASC 的 ActorInfo，并绑定属性变化监听 */
	virtual void InitializeAbilityActorInfo(AActor* InOwnerActor, AActor* InAvatarActor);

	/** 绑定属性变化回调，避免重复绑定 */
	virtual void BindAttributeDelegates();

	/** 属性变化响应 */
	virtual void HandleHealthChanged(const FOnAttributeChangeData& ChangeData);
	virtual void HandleShieldChanged(const FOnAttributeChangeData& ChangeData);

	virtual void HandleShowDamageNumberEvent(FGameplayTag Tag, int32 NewCount);
public:
	// 受击硬直入口
	virtual void HandleHitReact(AActor* DamageCauser);
public:
	/** 角色所属阵营。用于感知、锁敌、伤害过滤、友军判断 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Combat")
	EESFaction Faction = EESFaction::Neutral;
	
protected:
	/** 当前角色拥有的 ASC，玩家与敌人都共用这套基类 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UESAbilitySystemComponent> AbilitySystemComponent;

	/** 属性集，挂在角色上并由 ASC 管理属性数据 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|AbilitySystem", meta = (AllowPrivateAccess = "true"))
	TObjectPtr<UESAttributeSet> AttributeSet;
	
protected:
	/** 伤害数字锚点（放在头顶） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|DamageNumber")
	TObjectPtr<USceneComponent> DamageNumberAnchor;

	/** 避免重复初始化和重复绑定 */
	UPROPERTY(Transient)
	bool bAbilityActorInfoInitialized = false;

	UPROPERTY(Transient)
	bool bAttributeDelegatesBound = false;
};
