#include "Projectile/ESProjectileBase.h"

#include "AbilitySystemBlueprintLibrary.h"
#include "AbilitySystemComponent.h"
#include "Components/BoxComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "GameplayEffect.h"
#include "GameplayTagsManager.h"


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
	
	// 默认伤害 GE（可在蓝图覆盖）
	// 注意：这里不能直接写路径，需要在蓝图中设置
	DamageEffectClass = nullptr;  // 默认空，蓝图中设置为 GE_Damage
	AbilityTypeBonus = 0.f;

}

void AESProjectileBase::BeginPlay()
{
	Super::BeginPlay();

	CollisionComponent->SetBoxExtent(CollisionBoxExtent);
	ProjectileMovementComponent->InitialSpeed = InitialSpeed;
	ProjectileMovementComponent->MaxSpeed = MaxSpeed;
	ProjectileMovementComponent->bIsHomingProjectile = bEnableHoming;
	ProjectileMovementComponent->HomingAccelerationMagnitude = HomingAccelerationMagnitude;
	SetLifeSpan(LifeSeconds);

	if (GetOwner() && UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(GetOwner())) SetOwnerAbilitySystem(
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

void AESProjectileBase::OnProjectileBeginOverlap(
	UPrimitiveComponent* OverlappedComponent,
	AActor* OtherActor,
	UPrimitiveComponent* OtherComp,
	int32 OtherBodyIndex,
	bool bFromSweep,
	const FHitResult& SweepResult)
{
	if (!OtherActor || OtherActor == this || OtherActor == OwnerActor.Get())
	{
		return;
	}

	// 广播事件（保留，方便蓝图监听）
	OnProjectileCollision.Broadcast(this, OtherActor, OtherComp, bFromSweep, SweepResult);
	
	// 处理碰撞逻辑（Niagara 特效等）
	HandleProjectileCollision(OtherActor, OtherComp, bFromSweep, SweepResult);

	// ── 新增：直接应用伤害 GE ──
	if (OwnerASC.IsValid() && DamageEffectClass)
	{
		UAbilitySystemComponent* TargetASC = UAbilitySystemBlueprintLibrary::GetAbilitySystemComponent(OtherActor);
		
		if (TargetASC)
		{
			// 创建 EffectContext
			FGameplayEffectContextHandle Context = OwnerASC->MakeEffectContext();
			Context.AddSourceObject(this);
			
			// 创建 EffectSpec
			FGameplayEffectSpecHandle SpecHandle = OwnerASC->MakeOutgoingSpec(DamageEffectClass, 1.f, Context);
			
			if (SpecHandle.IsValid())
			{
				// 设置 SetByCaller: Damage.AbilityTypeBonus
				if (AbilityTypeBonus != 0.f)
				{
					static const FGameplayTag AbilityTypeBonusTag = 
						FGameplayTag::RequestGameplayTag(FName("Damage.AbilityTypeBonus"), false);
					if (AbilityTypeBonusTag.IsValid())
					{
						SpecHandle.Data->SetSetByCallerMagnitude(AbilityTypeBonusTag, AbilityTypeBonus);
					}
				}
				
				// 应用伤害到目标
				OwnerASC->ApplyGameplayEffectSpecToTarget(*SpecHandle.Data.Get(), TargetASC);
				
				UE_LOG(LogTemp, Verbose, 
					TEXT("[ESProjectile] 应用伤害 GE: %s, AbilityTypeBonus: %.2f, 目标: %s"),
					*DamageEffectClass->GetName(), AbilityTypeBonus, *OtherActor->GetName());
			}
		}
	}

	// ── 新增：触发 GameplayCue ──
	if (OwnerASC.IsValid() && HitCueTag.IsValid())
	{
		FGameplayCueParameters CueParams;
		CueParams.Location = SweepResult.ImpactPoint;
		CueParams.Normal = SweepResult.ImpactNormal;
		CueParams.Instigator = OwnerActor.Get();      // ASC 的拥有者
		CueParams.EffectCauser = this;                 // 投射物本身
		CueParams.SourceObject = this;                 // 来源对象
		CueParams.AggregatedSourceTags = FGameplayTagContainer(SourceAbilityTag);
		
		// UE 5.7 使用 ExecuteGameplayCue 方法
		OwnerASC->ExecuteGameplayCue(HitCueTag, CueParams);
		
		UE_LOG(LogTemp, Verbose, 
			TEXT("[ESProjectile] 触发 GameplayCue: %s, 目标: %s"),
			*HitCueTag.ToString(), *OtherActor->GetName());
	}

	// 销毁投射物
	Destroy();
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
