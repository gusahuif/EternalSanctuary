#include "Common/SpawnPoint/ESMonsterSpawnManagerComponent.h"
#include "Character/ESEnemyBase.h"
#include "Kismet/GameplayStatics.h"
#include "Math/UnrealMathUtility.h"

UESMonsterSpawnManagerComponent::UESMonsterSpawnManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false; // 不需要Tick
}

void UESMonsterSpawnManagerComponent::BeginPlay()
{
	Super::BeginPlay();
}

void UESMonsterSpawnManagerComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
}

// ==========================================
// PUBLIC 函数实现
// ==========================================

void UESMonsterSpawnManagerComponent::InitializeSpawnSystem(UESSpawnWaveConfig* InSpawnConfig)
{
	// 1. 检查配置是否有效
	if (!InSpawnConfig)
	{
		UE_LOG(LogTemp, Error, TEXT("【刷怪系统】初始化失败：SpawnConfig 为空！"));
		return;
	}

	SpawnConfig = InSpawnConfig;
	CurrentWaveIndex = 0;
	SpawnedCountInCurrentWave = 0;
	bHasWon = false;
	bIsInWaveInterval = false;

	// 2. 找到世界里所有的刷怪点
	TArray<AActor*> FoundActors;
	UGameplayStatics::GetAllActorsOfClass(GetWorld(), SpawnPointClass, FoundActors);
	
	for (AActor* Actor : FoundActors)
	{
		if (Actor)
		{
			SpawnPoints.Add(Actor);
		}
	}

	if (SpawnPoints.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("【刷怪系统】警告：世界里没有找到刷怪点！"));
	}

	// 3. 给每个刷怪点一个初始冷却
	for (AActor* Point : SpawnPoints)
	{
		float InitialCooldown = FMath::FRandRange(0.0f, 1.0f);
		SpawnPointCooldowns.Add(Point, InitialCooldown);
	}

	// 4. 启动主循环定时器
	GetWorld()->GetTimerManager().SetTimer(
		CheckTimerHandle,
		this,
		&UESMonsterSpawnManagerComponent::CheckAndSpawnMonster,
		SpawnConfig->CheckInterval,
		true, // 循环
		0.0f
	);

	UE_LOG(LogTemp, Log, TEXT("【刷怪系统】初始化完成！找到 %d 个刷怪点，配置了 %d 波"), SpawnPoints.Num(), GetTotalWaveCount());
	
	// 5. 直接开始第一波
	StartNextWave();
}

void UESMonsterSpawnManagerComponent::StartNextWave()
{
	if (!SpawnConfig) return;

	// 检查是否所有波次都打完了
	if (CurrentWaveIndex >= SpawnConfig->WaveConfigs.Num())
	{
		UE_LOG(LogTemp, Log, TEXT("【刷怪系统】所有波次已完成！"));
		CheckVictoryCondition();
		return;
	}

	bIsInWaveInterval = false;
	SpawnedCountInCurrentWave = 0;

	UE_LOG(LogTemp, Log, TEXT("【刷怪系统】========== 开始第 %d 波 =========="), GetCurrentWaveNumber());
}

int32 UESMonsterSpawnManagerComponent::GetTotalWaveCount() const
{
	if (SpawnConfig)
	{
		return SpawnConfig->WaveConfigs.Num();
	}
	return 0;
}

int32 UESMonsterSpawnManagerComponent::GetRemainingMonstersInCurrentWave() const
{
	if (!SpawnConfig || !SpawnConfig->WaveConfigs.IsValidIndex(CurrentWaveIndex))
	{
		return 0;
	}
	return SpawnConfig->WaveConfigs[CurrentWaveIndex].TotalMonstersInWave - SpawnedCountInCurrentWave;
}

// ==========================================
// PROTECTED 函数实现
// ==========================================

void UESMonsterSpawnManagerComponent::CheckAndSpawnMonster()
{
	if (!SpawnConfig || bHasWon) return;

	// === 步骤0：如果正在波次间隔中，什么都不做 ===
	if (bIsInWaveInterval)
	{
		return;
	}

	// === 步骤1：冷却时间流逝 ===
	for (auto& Elem : SpawnPointCooldowns)
	{
		Elem.Value -= SpawnConfig->CheckInterval;
	}

	// === 步骤2：清理尸体 ===
	CleanupDeadMonsters();

	// === 步骤3：检查当前波次是否刷完 ===
	if (SpawnConfig->WaveConfigs.IsValidIndex(CurrentWaveIndex))
	{
		const FESSingleWaveConfig& CurrentWave = SpawnConfig->WaveConfigs[CurrentWaveIndex];
		
		// 如果这一波刷够了
		if (SpawnedCountInCurrentWave >= CurrentWave.TotalMonstersInWave)
		{
			// 检查场上是否还有怪
			if (CurrentAliveMonsters.Num() == 0)
			{
				// 这波清完了，进入间隔
				EnterWaveInterval();
			}
			return;
		}
	}

	// === 步骤4：检查场上存活上限 ===
	if (CurrentAliveMonsters.Num() >= SpawnConfig->MaxAliveLimit)
	{
		return;
	}

	// === 步骤5：排序刷怪点（近的在前） ===
	FVector PlayerLoc = GetPlayerLocation();
	if (PlayerLoc.IsZero()) return;

	TArray<AActor*> SortedPoints = SpawnPoints;
	SortSpawnPointsByDistance(SortedPoints);

	// === 步骤6：找最近且冷却好的点刷怪 ===
	for (AActor* Point : SortedPoints)
	{
		float* RemainingCooldown = SpawnPointCooldowns.Find(Point);
		if (!RemainingCooldown || *RemainingCooldown > 0.0f)
		{
			continue;
		}

		// 找到了，刷怪！
		SpawnMonsterAtPoint(Point);
		break; // 每次只刷一只，保持节奏
	}
}

void UESMonsterSpawnManagerComponent::CleanupDeadMonsters()
{
	// 从后往前遍历数组，安全删除元素
	for (int32 i = CurrentAliveMonsters.Num() - 1; i >= 0; --i)
	{
		AESEnemyBase* Monster = CurrentAliveMonsters[i];
		
		// 双重判定：指针无效 或 怪物已死亡，都移除
		if (!IsValid(Monster) || !Monster->IsAlive())
		{
			CurrentAliveMonsters.RemoveAt(i);
		}
	}
}

FVector UESMonsterSpawnManagerComponent::GetPlayerLocation() const
{
	APawn* PlayerPawn = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
	if (PlayerPawn)
	{
		return PlayerPawn->GetActorLocation();
	}
	return FVector::ZeroVector;
}

void UESMonsterSpawnManagerComponent::SortSpawnPointsByDistance(TArray<AActor*>& InOutPoints) const
{
	FVector PlayerLoc = GetPlayerLocation();
	int32 Count = InOutPoints.Num();

	if (Count < 2) return;

	// 简单的冒泡排序逻辑：两两比较，把近的往前放
	for (int32 i = 0; i < Count - 1; ++i)
	{
		for (int32 j = 0; j < Count - 1 - i; ++j)
		{
			AActor* PointA = InOutPoints[j];
			AActor* PointB = InOutPoints[j + 1];

			if (!PointA || !PointB) continue;

			float DistA = (PointA->GetActorLocation() - PlayerLoc).SizeSquared();
			float DistB = (PointB->GetActorLocation() - PlayerLoc).SizeSquared();

			if (DistA > DistB)
			{
				InOutPoints.Swap(j, j + 1);
			}
		}
	}
}

void UESMonsterSpawnManagerComponent::SpawnMonsterAtPoint(AActor* SpawnPoint)
{
	if (!SpawnPoint || !SpawnConfig) return;

	// 1. 根据权重选一个怪物
	TSubclassOf<AESEnemyBase> SelectedClass = SelectMonsterByWeight();
	if (!SelectedClass) return;

	// 2. 生成
	FVector SpawnLoc = SpawnPoint->GetActorLocation();
	FRotator SpawnRot = SpawnPoint->GetActorRotation();
	
	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;

	AESEnemyBase* NewMonster = GetWorld()->SpawnActor<AESEnemyBase>(SelectedClass, SpawnLoc, SpawnRot, Params);

	if (NewMonster)
	{
		// 3. 【核心】计算并应用怪物属性成长
		float DifficultyMult = SpawnConfig->GetDifficultyMultiplier();
		float WaveMult = 1.0f + (CurrentWaveIndex * 0.1f); // 公式：1 + 波次 * 0.1
		
		// 调用敌人基类的初始化函数（我们下一步写）
		NewMonster->InitializeWithWaveData(DifficultyMult, WaveMult);

		// 4. 记录数据
		CurrentAliveMonsters.Add(NewMonster);
		SpawnedCountInCurrentWave++;
		RefreshPointCooldown(SpawnPoint);

		UE_LOG(LogTemp, Log, TEXT("【刷怪】生成怪物: %s | 波次: %d | 难度倍率: %.2f | 波次倍率: %.2f"), 
			*GetNameSafe(SelectedClass), GetCurrentWaveNumber(), DifficultyMult, WaveMult);
	}
}

void UESMonsterSpawnManagerComponent::RefreshPointCooldown(AActor* SpawnPoint)
{
	if (!SpawnPoint || !SpawnConfig) return;
	
	// 简单的固定冷却，也可以改成随机
	float NewCooldown = 1.5f; 
	
	if (SpawnPointCooldowns.Contains(SpawnPoint))
	{
		SpawnPointCooldowns[SpawnPoint] = NewCooldown;
	}
	else
	{
		SpawnPointCooldowns.Add(SpawnPoint, NewCooldown);
	}
}

TSubclassOf<AESEnemyBase> UESMonsterSpawnManagerComponent::SelectMonsterByWeight() const
{
	if (!SpawnConfig || !SpawnConfig->WaveConfigs.IsValidIndex(CurrentWaveIndex)) return nullptr;

	const TMap<TSubclassOf<AESEnemyBase>, float>& WeightMap = SpawnConfig->WaveConfigs[CurrentWaveIndex].MonsterWeights;

	if (WeightMap.Num() == 0) return nullptr;

	// 步骤1：计算总权重
	float TotalWeight = 0.0f;
	for (const auto& Elem : WeightMap)
	{
		if (Elem.Value > 0.0f)
		{
			TotalWeight += Elem.Value;
		}
	}

	if (TotalWeight <= 0.0f) return nullptr;

	// 步骤2：在 0 到 总权重 之间取一个随机数
	float RandomValue = FMath::FRandRange(0.0f, TotalWeight);

	// 步骤3：遍历累加，找到随机数落在哪个区间
	float CurrentSum = 0.0f;
	for (const auto& Elem : WeightMap)
	{
		if (Elem.Value <= 0.0f) continue;

		CurrentSum += Elem.Value;

		if (RandomValue <= CurrentSum)
		{
			return Elem.Key;
		}
	}

	return nullptr;
}

void UESMonsterSpawnManagerComponent::CheckVictoryCondition()
{
	if (bHasWon) return;

	// 胜利条件：
	// 1. 已经刷完了所有波次
	// 2. 场上一只活的都没有了
	if (CurrentWaveIndex >= SpawnConfig->WaveConfigs.Num() && CurrentAliveMonsters.Num() == 0)
	{
		bHasWon = true;
		
		// 停止定时器
		if (CheckTimerHandle.IsValid())
		{
			GetWorld()->GetTimerManager().ClearTimer(CheckTimerHandle);
		}

		UE_LOG(LogTemp, Log, TEXT("【胜利】所有波次完成，怪物全清！"));
		
		// 广播胜利委托
		OnAllWavesCompleted.Broadcast();
	}
}

void UESMonsterSpawnManagerComponent::EnterWaveInterval()
{
	if (bIsInWaveInterval) return;

	bIsInWaveInterval = true;

	UE_LOG(LogTemp, Log, TEXT("【刷怪系统】第 %d 波完成！等待 %.1f 秒后开始下一波..."), GetCurrentWaveNumber(), SpawnConfig->WaveInterval);

	// 【修改】波次索引+1 放在这里，确保 StartNextWave 时 CurrentWaveIndex 已经是新的了
	CurrentWaveIndex++; 

	// 设置定时器，等待结束后自动开始下一波
	GetWorld()->GetTimerManager().SetTimer(
		WaveIntervalTimerHandle,
		this,
		&UESMonsterSpawnManagerComponent::StartNextWave,
		SpawnConfig->WaveInterval,
		false
	);
}