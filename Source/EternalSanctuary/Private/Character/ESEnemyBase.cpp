#include "Character/ESEnemyBase.h"
#include "GameFramework/Actor.h"
#include "Kismet/GameplayStatics.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "GameplayTagContainer.h"
#include "GAS/Data/ESEnemyAISkillData.h"
#include "GameplayEffect.h"

AESEnemyBase::AESEnemyBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 敌人默认就是 Enemy 阵营，避免每个蓝图都手动配
	Faction = EESFaction::Enemy;
}

void AESEnemyBase::BeginPlay()
{
	Super::BeginPlay();

	// 记录出生点，用于脱战回归
	SpawnLocation = GetActorLocation();
}

void AESEnemyBase::InitializeEnemyCharacter()
{
}

void AESEnemyBase::SetCurrentTarget(AActor* NewTarget)
{
	// 新目标为空则直接清空
	if (!NewTarget)
	{
		ClearCurrentTarget();
		return;
	}

	// 不锁自己
	if (NewTarget == this)
	{
		return;
	}

	AESCharacterBase* TargetCharacter = Cast<AESCharacterBase>(NewTarget);
	if (!TargetCharacter)
	{
		return;
	}

	// 只能锁定敌对阵营
	if (!IsHostileToActor(NewTarget))
	{
		return;
	}

	// 目标必须可被锁定
	if (!TargetCharacter->CanBeTargeted())
	{
		return;
	}

	CurrentTarget = NewTarget;
}

void AESEnemyBase::ClearCurrentTarget()
{
	CurrentTarget = nullptr;
}

bool AESEnemyBase::HasValidTarget() const
{
	if (!CurrentTarget)
	{
		return false;
	}

	const AESCharacterBase* TargetCharacter = Cast<AESCharacterBase>(CurrentTarget.Get());
	if (!TargetCharacter)
	{
		return false;
	}

	return TargetCharacter->CanBeTargeted();
}

bool AESEnemyBase::IsTargetInChaseRange() const
{
	if (!HasValidTarget())
	{
		return false;
	}

	const float Distance = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	return Distance <= ChaseRange;
}

bool AESEnemyBase::IsTargetInAttackRange() const
{
	if (!HasValidTarget())
	{
		return false;
	}

	// 增加容错距离，减少“刚进范围/刚出范围”导致的攻击抖动
	const float Distance = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	return Distance <= (AttackRange + AttackRangeTolerance);
}

bool AESEnemyBase::CanStartAttack() const
{
	// 死亡或硬直时禁止发起攻击
	if (!CanAct())
	{
		return false;
	}

	// 没目标不能攻击
	if (!HasValidTarget())
	{
		return false;
	}

	// 不在攻击范围内不能攻击
	if (!IsTargetInAttackRange())
	{
		return false;
	}

	// 正在攻击时不重复发起
	if (bIsAttacking)
	{
		return false;
	}

	const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
	if (CurrentTime - LastAttackTime < AttackCooldown)
	{
		return false;
	}

	return true;
}

void AESEnemyBase::MarkAttackStarted()
{
	bIsAttacking = true;
	LastAttackTime = GetWorld() ? GetWorld()->GetTimeSeconds() : LastAttackTime;
	SetAIState(EEnemyAIState::Attack);
}

void AESEnemyBase::MarkAttackFinished()
{
	bIsAttacking = false;

	// 攻击结束后如果目标还有效，回到 Chase；否则回 Idle
	if (HasValidTarget())
	{
		SetAIState(EEnemyAIState::Chase);
	}
	else
	{
		SetAIState(EEnemyAIState::Idle);
	}
}

void AESEnemyBase::SetAIState(EEnemyAIState NewState)
{
	AIState = NewState;

	// 这里后续你可以同步到 Gameplay Tag
	// 比如先移除旧 AI.State.* Tag，再添加新的
	// 这样 BT、AnimBP、GA 都可以共用同一套状态源
}

bool AESEnemyBase::ShouldLoseTarget() const
{
	if (!HasValidTarget())
	{
		return true;
	}

	// 超过丢失范围则脱战
	const float Distance = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	return Distance > LoseTargetRange;
}

bool AESEnemyBase::IsNearSpawnLocation() const
{
	const float Distance = FVector::Dist2D(GetActorLocation(), SpawnLocation);
	return Distance <= ReturnAcceptanceRadius;
}

// ---------------------- 仇恨系统实现 ----------------------

void AESEnemyBase::AddAggro(AActor* Target, float Value)
{
    if (!Target || !IsValid(Target))
    {
        return;
    }

    // 只能给敌对单位加仇恨
    if (!IsHostileToActor(Target))
    {
        return;
    }

    float& CurrentAggro = AggroMap.FindOrAdd(Target);
    CurrentAggro += Value;
    
    // 简单的溢出保护
    CurrentAggro = FMath::Clamp(CurrentAggro, 0.0f, 1000000.0f);
}

void AESEnemyBase::SetAggro(AActor* Target, float Value)
{
    if (!Target || !IsValid(Target))
    {
        return;
    }

    if (!IsHostileToActor(Target))
    {
        return;
    }

    AggroMap.FindOrAdd(Target) = Value;
}

void AESEnemyBase::ClearAggro(AActor* Target)
{
    if (Target)
    {
        AggroMap.Remove(Target);
    }
}

AActor* AESEnemyBase::GetCurrentAggroTarget()
{
    UESAbilitySystemComponent* ASC = GetESAbilitySystemComponent();
    if (!ASC)
    {
        return nullptr;
    }

    // ---------------------- 1. 最高优先级：检查嘲讽 ----------------------
    static const FGameplayTag TauntTag = FGameplayTag::RequestGameplayTag(TEXT("State.Taunt"));
    
    // 遍历仇恨列表，找有没有带嘲讽Tag的
    for (auto& Pair : AggroMap)
    {
        AActor* Target = Pair.Key;
        if (!IsValid(Target))
        {
            continue;
        }

        // 检查目标是否有ASC且带有嘲讽Tag
        AESCharacterBase* TargetChar = Cast<AESCharacterBase>(Target);
        if (TargetChar && TargetChar->GetESAbilitySystemComponent())
        {
            if (TargetChar->GetESAbilitySystemComponent()->HasMatchingGameplayTag(TauntTag))
            {
                CachedAggroTarget = Target;
                return Target;
            }
        }
    }

    // ---------------------- 2. 次优先级：找仇恨值最高的 ----------------------
    AActor* BestTarget = nullptr;
    float HighestAggro = 0.0f;

    // 先清理一下无效的目标（死亡或被销毁的）
    TArray<AActor*> InvalidTargets;
    for (auto& Pair : AggroMap)
    {
        AActor* Target = Pair.Key;
        if (!IsValid(Target))
        {
            InvalidTargets.Add(Target);
            continue;
        }

        // 检查目标是否还能被锁定
        AESCharacterBase* TargetChar = Cast<AESCharacterBase>(Target);
        if (TargetChar && !TargetChar->CanBeTargeted())
        {
            continue;
        }

        // 比较仇恨值
        if (Pair.Value > HighestAggro)
        {
            HighestAggro = Pair.Value;
            BestTarget = Target;
        }
    }

    // 移除无效目标
    for (AActor* Invalid : InvalidTargets)
    {
        AggroMap.Remove(Invalid);
    }

    CachedAggroTarget = BestTarget;
    return BestTarget;
}

bool AESEnemyBase::HasValidAggroTarget() const
{
    return IsValid(CachedAggroTarget);
}

bool AESEnemyBase::TryActivateMeleeAttackAbility()
{
	UAbilitySystemComponent* ASC = GetESAbilitySystemComponent();
	if (!ASC)
	{
		return false;
	}

	// 近战攻击能力入口，BT 不关心具体是哪一招
	FGameplayTagContainer AbilityTags;
	AbilityTags.AddTag(FGameplayTag::RequestGameplayTag(TEXT("Ability.Attack.Melee")));

	return ASC->TryActivateAbilitiesByTag(AbilityTags);
}

void AESEnemyBase::Die()
{
	if (!IsAlive()) 
	{
		return; // 防止重复触发
	}
	
	Super::Die(); // 先执行父类通用死亡逻辑
	AIState = EEnemyAIState::Dead;
}

void AESEnemyBase::HandleHitReact(AActor* DamageCauser)
{
	Super::HandleHitReact(DamageCauser);
	bIsAttacking = false;
}
// ... 现有代码 ...


// ---------------------- AI 技能系统实现 ----------------------

TArray<UESEnemyAISkillData*> AESEnemyBase::GetAvailableAISkills()
{
    TArray<UESEnemyAISkillData*> Result;
    if (!HasValidTarget() || !CanAct() || AISkillList.IsEmpty())
    {
        return Result;
    }

    UESAbilitySystemComponent* ASC = GetESAbilitySystemComponent();
    if (!ASC)
    {
        return Result;
    }

    const float CurrentTime = GetWorld() ? GetWorld()->GetTimeSeconds() : 0.0f;
    const float CurrentDistance = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());

    for (UESEnemyAISkillData* SkillData : AISkillList)
    {
        if (!SkillData || !SkillData->AbilityActivationTag.IsValid())
        {
            continue;
        }

        // 1. 检查AI层冷却（作为GAS GE冷却的双重保险）
        const float* LastUseTime = SkillLastUseTimeMap.Find(SkillData->AbilityActivationTag);
        if (LastUseTime && (CurrentTime - *LastUseTime < SkillData->CooldownTime))
        {
            continue;
        }

        // 2. 检查距离是否符合要求
        if (CurrentDistance < SkillData->MinUseRange || CurrentDistance > SkillData->MaxUseRange)
        {
            continue;
        }

        // 3. 检查GAS层面技能是否可用（未被眩晕、沉默等）
        // 这里我们信任GAS的Tag状态，CanAct()已经检查了眩晕/硬直
        
        Result.Add(SkillData);
    }

    return Result;
}

UESEnemyAISkillData* AESEnemyBase::SelectBestSkillByWeight()
{
	TArray<UESEnemyAISkillData*> AvailableSkills = GetAvailableAISkills();
	if (AvailableSkills.IsEmpty())
	{
		return nullptr;
	}

	UESAbilitySystemComponent* ASC = GetESAbilitySystemComponent();
	if (!ASC)
	{
		return AvailableSkills[0];
	}

	const float CurrentDistance = FVector::Dist2D(GetActorLocation(), CurrentTarget->GetActorLocation());
	const float HealthPercent = GetMaxHealth() > 0 ? (GetHealth() / GetMaxHealth()) : 1.0f;

	UESEnemyAISkillData* BestSkill = nullptr;
	float HighestWeight = 0.0f;

	// 不排序了，直接遍历找权重最大的那个，简单高效且不会出错
	for (UESEnemyAISkillData* Skill : AvailableSkills)
	{
		if (!Skill) continue;

		float FinalWeight = Skill->BaseWeight;

		// 1. 应用距离权重曲线
		if (Skill->DistanceWeightCurve && Skill->MaxUseRange > 0.0f)
		{
			const float DistanceRatio = FMath::Clamp(CurrentDistance / Skill->MaxUseRange, 0.0f, 1.0f);
			FinalWeight *= Skill->DistanceWeightCurve->GetFloatValue(DistanceRatio);
		}

		// 2. 应用血量权重曲线
		if (Skill->HealthWeightCurve)
		{
			FinalWeight *= Skill->HealthWeightCurve->GetFloatValue(HealthPercent);
		}

		// 3. 权重不能为负
		FinalWeight = FMath::Max(FinalWeight, 0.01f);

		// 4. 比较并记录最大值
		if (FinalWeight > HighestWeight)
		{
			HighestWeight = FinalWeight;
			BestSkill = Skill;
		}
	}

	return BestSkill;
}

bool AESEnemyBase::TryActivateAISkillByTag(FGameplayTag AbilityTag)
{
    UESAbilitySystemComponent* ASC = GetESAbilitySystemComponent();
    if (!ASC || !AbilityTag.IsValid())
    {
        return false;
    }

    // 记录释放时间
    if (GetWorld())
    {
        SkillLastUseTimeMap.FindOrAdd(AbilityTag) = GetWorld()->GetTimeSeconds();
    }

    // 直接调用你ASC里封装好的方法！
    return ASC->TryActivateAbilityByTag(AbilityTag);
}

FVector AESEnemyBase::GetOptimalChaseLocation() const
{
    if (!HasValidTarget())
    {
        return GetActorLocation();
    }

    // 简单策略：
    // 1. 如果有可用技能，往“能释放最远技能”的位置移动
    // 2. 如果没有可用技能，直接往目标位置移动
    
    float OptimalDistance = AttackRange; // 默认用近战攻击范围

    // 找出所有技能中最远的那个的80%作为最优距离
    for (const UESEnemyAISkillData* Skill : AISkillList)
    {
        if (Skill && Skill->MaxUseRange > OptimalDistance)
        {
            OptimalDistance = Skill->MaxUseRange * 0.8f; // 保持在80%射程，比较安全
        }
    }

    const FVector TargetLoc = CurrentTarget->GetActorLocation();
    const FVector SelfLoc = GetActorLocation();
    const FVector Dir = (SelfLoc - TargetLoc).GetSafeNormal2D();

    // 计算目标点：目标位置 + 方向 * 最优距离
    return TargetLoc + Dir * OptimalDistance;
}

void AESEnemyBase::InitializeWithWaveData(float InDifficultyMult, float InWaveMult)
{
	CurrentDifficultyMult = InDifficultyMult;
	CurrentWaveMult = InWaveMult;

	UESAbilitySystemComponent* ASC = GetESAbilitySystemComponent();
	if (!ASC)
	{
		UE_LOG(LogTemp, Error, TEXT("【敌人初始化】失败：找不到 ASC！"));
		return;
	}

	// ==========================================
	// 【核心】属性计算公式
	// 最终属性 = 基础属性 * 难度系数 * (1 + 波次 * 0.1)
	// ==========================================
	float FinalMult = CurrentDifficultyMult * CurrentWaveMult;

	UE_LOG(LogTemp, Log, TEXT("【敌人初始化】%s 初始化完成，最终倍率: %.2f"), *GetName(), FinalMult);

	// ==========================================
	// 真正应用 GAS GameplayEffect
	// ==========================================
	if (EnemyScalingGE)
	{
		// 1. 创建一个 Spec
		FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EnemyScalingGE, 1, ASC->MakeEffectContext());
		
		if (SpecHandle.IsValid())
		{
			// 2. 设置 SetByCaller 的数值 (对应我们在 GE 里配置的 Tag)
			static const FGameplayTag MultTag = FGameplayTag::RequestGameplayTag(FName("Data.Multiplyer"));
			SpecHandle.Data.Get()->SetSetByCallerMagnitude(MultTag, FinalMult);

			// 3. 应用到自己身上
			ASC->ApplyGameplayEffectSpecToSelf(*SpecHandle.Data.Get());
		}
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("【敌人初始化】%s 没有配置 EnemyScalingGE！"), *GetName());
	}
}