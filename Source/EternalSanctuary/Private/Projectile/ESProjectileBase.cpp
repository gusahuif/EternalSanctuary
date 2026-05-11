#include "Projectile/ESProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameplayEffect.h"
#include "GameplayTagsManager.h"
#include "Character/ESCharacterBase.h"
#include "Character/ESPlayerBase.h"

AESProjectileBase::AESProjectileBase()
{
	PrimaryActorTick.bCanEverTick = true;

	RootSceneComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(RootSceneComponent);

	CollisionComponent = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionComponent"));
	CollisionComponent->SetupAttachment(RootSceneComponent);
	CollisionComponent->SetBoxExtent(CollisionBoxExtent);
	CollisionComponent->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionComponent->SetCollisionObjectType(ECC_WorldDynamic);
	CollisionComponent->SetCollisionResponseToAllChannels(ECR_Ignore);
	CollisionComponent->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);
	CollisionComponent->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Overlap);
	CollisionComponent->SetGenerateOverlapEvents(true);
	CollisionComponent->OnComponentBeginOverlap.AddDynamic(this, &AESProjectileBase::OnProjectileBeginOverlap);

	ProjectileMovementComponent = CreateDefaultSubobject<UProjectileMovementComponent>(
		TEXT("ProjectileMovementComponent"));
	ProjectileMovementComponent->UpdatedComponent = CollisionComponent;
	InitialSpeed = 2000.0f;
	MaxSpeed = 2000.0f;
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = MaxSpeed;
	ProjectileMovementComponent->bRotationFollowsVelocity = true;
	ProjectileMovementComponent->bShouldBounce = false;

	NiagaraComponent = CreateDefaultSubobject<UNiagaraComponent>(TEXT("NiagaraComponent"));
	NiagaraComponent->SetupAttachment(RootSceneComponent);

	bEnableHoming = false;
	HomingAccelerationMagnitude = 8000.0f;
	LifeSeconds = 5.0f;
	InitialLifeSpan = LifeSeconds;
	CollisionComponent->SetBoxExtent(CollisionBoxExtent);

	// 默认伤害 GE（可在蓝图覆盖）
	// 注意：这里不能直接写路径，需要在蓝图中设置
	DamageEffectClass = nullptr; // 默认空，蓝图中设置为 GE_Damage
	AbilityTypeBonus = 0.f;
}

void AESProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	// 记录生成位置，用于猎手本能等基于飞行距离的被动计算
	SpawnLocation = GetActorLocation();

	//ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = MaxSpeed;
	ProjectileMovementComponent->bIsHomingProjectile = bEnableHoming;
	ProjectileMovementComponent->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
	// 手动给箭矢设置初始速度，彻底解决InitialSpeed无效的问题
	// 速度方向：箭矢的前向向量 * 你设置的初始速度
	FVector InitialVelocity = GetActorForwardVector() * InitialSpeed;
	ProjectileMovementComponent->Velocity = InitialVelocity;

	SetLifeSpan(LifeSeconds);

	// 【必加】打印穿透数，确认蓝图参数有没有传进来
	UE_LOG(LogTemp, Log, TEXT("【箭矢初始化】当前穿透次数: %d | 初始速度: %.2f"), PenetrationCount, InitialSpeed);


	if (GetOwner() && UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()))
		SetOwnerAbilitySystem(
			UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner()));
}

void AESProjectileBase::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
}

void AESProjectileBase::SetHomingTarget(USceneComponent* NewHomingTarget)
{
	if (!ProjectileMovementComponent)
	{
		return;
	}

	ProjectileMovementComponent->bIsHomingProjectile = bEnableHoming && NewHomingTarget != nullptr;
	ProjectileMovementComponent->HomingTargetComponent = NewHomingTarget;
}

void AESProjectileBase::SetOwnerAbilitySystem(UAbilitySystemComponent* InOwnerASC)
{
	OwnerASC = InOwnerASC;
}

void AESProjectileBase::SetProjectileOwnerActor(AActor* InOwnerActor)
{
	OwnerActor = InOwnerActor;
	SetOwner(InOwnerActor);
}

void AESProjectileBase::SetCollisionNotifyTag(FGameplayTag InCollisionNotifyTag)
{
	CollisionNotifyTag = InCollisionNotifyTag;
}

// ---------------------------------------------------------
//  【新增】延迟销毁逻辑：先停碰撞、停飞、停特效，3秒后彻底销毁
// ---------------------------------------------------------
void AESProjectileBase::DelayedDestroy()
{
	// 1. 安全检查：如果已经在销毁流程中了，直接返回，避免重复执行
	if (DelayedDestroyTimerHandle.IsValid())
	{
		return;
	}

	// 2. 关闭碰撞：防止箭矢在拖尾消散期间再次碰到东西触发逻辑
	if (CollisionComponent)
	{
		CollisionComponent->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	// 3. 停止飞行：让箭矢不再继续往前飞
	if (ProjectileMovementComponent)
	{
		ProjectileMovementComponent->StopMovementImmediately();
		ProjectileMovementComponent->ProjectileGravityScale = 0.0f; // 顺便关掉重力，避免箭矢下坠
	}

	// 4. 停止Niagara特效：不再生成新粒子，让现有的拖尾自然消散
	if (NiagaraComponent)
	{
		NiagaraComponent->Deactivate(); // 关键：Deactivate而不是Destroy，让现有粒子播完
	}
	
	BlueprintDelayedDestroy();
	
	// 5. 【修复】设置定时器：使用标准的UE定时器绑定语法
	// 拖尾大概1秒消散，3秒足够保险
	FTimerDelegate TimerDelegate;
	TimerDelegate.BindUObject(this, &AESProjectileBase::OnDelayedDestroyTimer);

	GetWorld()->GetTimerManager().SetTimer(
		DelayedDestroyTimerHandle,
		TimerDelegate,
		3.0f, // 延迟3秒
		false // 不循环
	);
}

void AESProjectileBase::BlueprintDelayedDestroy_Implementation()
{
}

// ---------------------------------------------------------
//  【新增】定时器回调函数：真正执行销毁
// ---------------------------------------------------------
void AESProjectileBase::OnDelayedDestroyTimer()
{
	// 清空定时器句柄
	DelayedDestroyTimerHandle.Invalidate();

	// 执行真正的销毁
	UE_LOG(LogTemp, Log, TEXT("【箭矢销毁】延迟结束，执行彻底销毁"));
	Destroy();
}

void AESProjectileBase::OnProjectileBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	// 基础过滤：忽略自己、忽略发射者
	if (!OtherActor || OtherActor == this || OtherActor == OwnerActor.Get())
	{
		return;
	}

	UE_LOG(LogTemp, Log, TEXT("【箭矢碰撞】碰到目标: %s | 剩余穿透次数: %d"), *OtherActor->GetName(), PenetrationCount);

	// =============== 穿透逻辑核心 ===============
	bool bShouldProcessHit = false;
	bool bShouldDestroyAfterHit = true; // 默认碰到就销毁，只有符合穿透条件才不销毁

	// 1. 先判断碰到的是不是角色
	AESCharacterBase* HitCharacter = Cast<AESCharacterBase>(OtherActor);
	if (HitCharacter)
	{
		// 2. 过滤：已经打过的目标，直接跳过，不处理也不销毁
		bool bAlreadyHit = AlreadyHitCharacters.Contains(HitCharacter);
		if (bAlreadyHit)
		{
			UE_LOG(LogTemp, Log, TEXT("【箭矢穿透】目标 %s 已经命中过，跳过"), *OtherActor->GetName());
			return;
		}

		// 3. 敌对判断：加了多层兜底，避免Owner无效导致判断失败
		bool bIsHostile = false;
		if (OwnerActor.IsValid())
		{
			AESCharacterBase* OwnerChar = Cast<AESCharacterBase>(OwnerActor.Get());
			if (OwnerChar)
			{
				bIsHostile = OwnerChar->IsHostileToActor(OtherActor);
			}
			else
			{
				// 兜底：如果Owner不是Character，就默认所有Enemy阵营都是敌对的
				bIsHostile = HitCharacter->GetFaction() == EESFaction::Enemy;
			}
		}
		else
		{
			// 终极兜底：Owner无效时，默认Enemy阵营是敌对的
			bIsHostile = HitCharacter->GetFaction() == EESFaction::Enemy;
		}

		// 4. 只有敌对目标才处理伤害和穿透
		if (bIsHostile)
		{
			bShouldProcessHit = true;
			// 记录已经命中的目标，防止重复伤害
			AlreadyHitCharacters.Add(HitCharacter);

			// 逻辑：PenetrationCount>0 → 还有穿透次数，这次不销毁，然后扣次数
			// 逻辑：PenetrationCount<=0 → 没有穿透次数了，这次命中后销毁
			if (PenetrationCount > 0)
			{
				bShouldDestroyAfterHit = false;
				PenetrationCount--; // 扣掉一次穿透次数
				UE_LOG(LogTemp, Log, TEXT("【箭矢穿透】命中敌对目标: %s | 剩余穿透次数: %d"), *OtherActor->GetName(), PenetrationCount);
			}
			else
			{
				bShouldDestroyAfterHit = true;
				UE_LOG(LogTemp, Log, TEXT("【箭矢穿透】穿透次数用尽，命中后销毁: %s"), *OtherActor->GetName());
			}
		}
		else
		{
			// 非敌对目标，直接跳过，不销毁箭矢
			UE_LOG(LogTemp, Log, TEXT("【箭矢穿透】非敌对目标: %s，跳过"), *OtherActor->GetName());
			return;
		}
	}
	else
	{
		// 碰到的不是角色（墙壁、地形等），直接处理并销毁
		bShouldProcessHit = true;
		bShouldDestroyAfterHit = true;
		UE_LOG(LogTemp, Log, TEXT("【箭矢碰撞】碰到非角色物体，销毁"));
	}

	// =============== 只有有效命中才执行伤害、特效逻辑 ===============
	if (bShouldProcessHit)
	{
		// 广播事件（保留蓝图监听）
		OnProjectileCollision.Broadcast(this, OtherActor, OtherComp, bFromSweep, SweepResult);

		// 碰撞特效
		HandleProjectileCollision(OtherActor, OtherComp, bFromSweep, SweepResult);

		// 应用伤害GE
		if (OwnerASC.IsValid() && DamageEffectClass)
		{
			UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
			if (TargetASC)
			{
				FGameplayEffectContextHandle Context = OwnerASC->MakeEffectContext();
				Context.AddSourceObject(this);
				FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);

				if (SpecHandle.IsValid())
				{
					if (AbilityTypeBonus != 0.f)
					{
						static const FGameplayTag AbilityTypeBonusTag = FGameplayTag::RequestGameplayTag(
							FName("Damage.AbilityTypeBonus"), false);
						if (AbilityTypeBonusTag.IsValid())
						{
							SpecHandle.Data->SetSetByCallerMagnitude(AbilityTypeBonusTag, AbilityTypeBonus);
						}
					}

					// 计算被动技能增伤（通用接口，投射物不感知具体被动）
					AESPlayerBase* PlayerChar = Cast<AESPlayerBase>(OwnerActor.Get());
					if (PlayerChar)
					{
						const float FlightDistMeters = FVector::Dist(SweepResult.ImpactPoint, SpawnLocation) / 100.0f;
						const float PassiveBonus = PlayerChar->CalculatePassiveDamageBonus(FlightDistMeters);
						if (PassiveBonus > 0.f)
						{
							static const FGameplayTag PassiveBonusTag = FGameplayTag::RequestGameplayTag(
								FName("Damage.PassiveBonus"), false);
							if (PassiveBonusTag.IsValid())
							{
								SpecHandle.Data->SetSetByCallerMagnitude(PassiveBonusTag, PassiveBonus);
							}
						}
					}

					OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
				}
			}
		}

		// 触发GameplayCue
		if (OwnerASC.IsValid() && HitCueTag.IsValid())
		{
			FGameplayCueParameters CueParams;
			CueParams.Location = SweepResult.ImpactPoint;
			CueParams.Normal = SweepResult.ImpactNormal;
			CueParams.Instigator = OwnerActor.Get();
			CueParams.EffectCauser = this;
			CueParams.SourceObject = this;
			CueParams.AggregatedSourceTags = FGameplayTagContainer(SourceAbilityTag);
			OwnerASC->ExecuteGameplayCue(HitCueTag, CueParams);
		}
	}

	// =============== 只有判定需要销毁时才销毁 ===============
	if (bShouldDestroyAfterHit)
	{
		UE_LOG(LogTemp, Log, TEXT("【箭矢销毁】触发延迟销毁，3秒后彻底消失"));

		// -------------------- 不再立即销毁，改为延迟销毁 --------------------
		DelayedDestroy(); // 调用新的延迟销毁函数
	}
}

void AESProjectileBase::HandleProjectileCollision_Implementation(AActor* OtherActor, UPrimitiveComponent* OtherComp,
                                                                 bool bFromSweep, const FHitResult& SweepResult)
{
	if (HitEffectTemplate)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			this,
			HitEffectTemplate,
			SweepResult.ImpactPoint,
			SweepResult.ImpactNormal.Rotation());
	}
}

float AESProjectileBase::GetFlightDistance() const
{
	return FVector::Dist(SpawnLocation, CollisionComponent->GetComponentLocation()) / 100.0f;
}

