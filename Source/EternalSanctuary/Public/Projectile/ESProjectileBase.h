#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "ESProjectileBase.generated.h"

class AActor;
class UAbilitySystemComponent;
class UBoxComponent;
class UNiagaraComponent;
class UNiagaraSystem;
class UProjectileMovementComponent;
class UPrimitiveComponent;
class UGameplayEffect;
class AESCharacterBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_FiveParams(
	FOnProjectileCollisionSignature,
	AESProjectileBase*, Projectile,
	AActor*, OtherActor,
	UPrimitiveComponent*, OtherComp,
	bool, bFromSweep,
	const FHitResult&, SweepResult
);

UCLASS(Blueprintable)
class ETERNALSANCTUARY_API AESProjectileBase : public AActor
{
	GENERATED_BODY()

public:
	AESProjectileBase();

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void OnProjectileBeginOverlap(
		UPrimitiveComponent* OverlappedComponent,
		AActor* OtherActor,
		UPrimitiveComponent* OtherComp,
		int32 OtherBodyIndex,
		bool bFromSweep,
		const FHitResult& SweepResult
	);

	UFUNCTION(BlueprintNativeEvent, Category = "ES|Projectile")
	void HandleProjectileCollision(AActor* OtherActor, UPrimitiveComponent* OtherComp, bool bFromSweep,
	                               const FHitResult& SweepResult);
	virtual void HandleProjectileCollision_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp,
	                                                      bool bFromSweep, const FHitResult& SweepResult);

public:
	virtual void Tick(float DeltaSeconds) override;

	UFUNCTION(BlueprintCallable, Category = "ES|Projectile")
	void SetHomingTarget(USceneComponent* NewHomingTarget);

	UFUNCTION(BlueprintCallable, Category = "ES|Projectile")
	void SetOwnerAbilitySystem(UAbilitySystemComponent* InOwnerASC);

	UFUNCTION(BlueprintCallable, Category = "ES|Projectile")
	void SetProjectileOwnerActor(AActor* InOwnerActor);

	UFUNCTION(BlueprintCallable, Category = "ES|Projectile")
	void SetCollisionNotifyTag(FGameplayTag InCollisionNotifyTag);

	/** 执行延迟销毁的逻辑 */
	void DelayedDestroy();

	/** 获取投射物已飞行距离（从生成点到当前位置） */
	UFUNCTION(BlueprintCallable, BlueprintPure, Category = "ES|Projectile")
	float GetFlightDistance() const;

	// 删除回调，帮助蓝图销毁东西
	UFUNCTION(BlueprintNativeEvent, Category = "ES|Projectile")
	void BlueprintDelayedDestroy();

	/** 定时器回调：真正执行销毁 */
	void OnDelayedDestroyTimer();
	
	/** 在 GA 蓝图中可绑定该事件，以在投射物发生碰撞时接收通知。 */
	UPROPERTY(BlueprintAssignable, Category = "ES|Projectile")
	FOnProjectileCollisionSignature OnProjectileCollision;

	/** 穿透数量：0=不穿透(碰到就炸)，1=穿1个(共打2个)，以此类推 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Collision", meta = (ExposeOnSpawn = true))
	int32 PenetrationCount = 0;
protected:
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Projectile")
	TObjectPtr<USceneComponent> RootSceneComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Projectile")
	TObjectPtr<UBoxComponent> CollisionComponent;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Projectile")
	TObjectPtr<UProjectileMovementComponent> ProjectileMovementComponent;

	/** 可选的 Niagara 飞行特效组件，可在蓝图中替换或留空。 */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Projectile|Effects")
	TObjectPtr<UNiagaraComponent> NiagaraComponent;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Projectile|Effects")
	TObjectPtr<UNiagaraSystem> HitEffectTemplate;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Movement", meta = (ExposeOnSpawn = true))
	float InitialSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Movement", meta = (ExposeOnSpawn = true))
	float MaxSpeed;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Movement")
	bool bEnableHoming;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Movement",
		meta = (EditCondition = "bEnableHoming"))
	float HomingAccelerationMagnitude;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Movement")
	float LifeSeconds;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Collision")
	FVector CollisionBoxExtent;

	/** 由生成该投射物的 GA 传入，用于碰撞后通知 AbilitySystem。 */
	UPROPERTY(BlueprintReadOnly, Category = "ES|Projectile")
	TWeakObjectPtr<UAbilitySystemComponent> OwnerASC;

	/** 记录投射物发射者，避免命中自己并便于 GA 做归属判断。 */
	UPROPERTY(BlueprintReadOnly, Category = "ES|Projectile")
	TWeakObjectPtr<AActor> OwnerActor;

	/** 可选的碰撞通知 Tag；若 OwnerASC 有效，则在碰撞时尝试发送该 Gameplay Event。 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile")
	FGameplayTag CollisionNotifyTag;
	
	/** 伤害 GE 类，默认使用 GE_Damage */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Damage")
	TSubclassOf<UGameplayEffect> DamageEffectClass;

	/** 技能类型伤害倍率，通过 SetByCaller 传递给 MMC */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Damage", meta = (EditCondition = "DamageEffectClass",ExposeOnSpawn = true))
	float AbilityTypeBonus = 0.f;

	/** 技能来源 Tag，用于标识伤害来源 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Damage")
	FGameplayTag SourceAbilityTag;

	/** 命中 GameplayCue Tag，如 GameplayCue.Hit.Arrow */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Projectile|Effects")
	FGameplayTag HitCueTag;

	/** 投射物生成位置，用于计算飞行距离（被动技能用） */
	UPROPERTY(BlueprintReadOnly, Category = "ES|Projectile")
	FVector SpawnLocation = FVector::ZeroVector;

	/** 记录已经命中过的角色，防止重复伤害 */
	UPROPERTY()
	TArray<TWeakObjectPtr<AESCharacterBase>> AlreadyHitCharacters;

	/** 延迟销毁的定时器句柄 */
	FTimerHandle DelayedDestroyTimerHandle;
};
