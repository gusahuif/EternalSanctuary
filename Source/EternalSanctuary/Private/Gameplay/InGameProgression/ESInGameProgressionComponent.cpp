#include "Gameplay/InGameProgression/ESInGameProgressionComponent.h"

UESInGameProgressionComponent::UESInGameProgressionComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UESInGameProgressionComponent::BeginPlay()
{
	Super::BeginPlay();
}

// ==================== 核心功能：增加经验 ====================
void UESInGameProgressionComponent::AddExperience(int32 Amount)
{
	if (Amount <= 0) return;

	CurrentExperience += Amount;
	UE_LOG(LogTemp, Log, TEXT("【局内成长】获得经验：%d，当前经验：%d/%d"), Amount, CurrentExperience, GetExpToNextLevel());

	// 检查升级
	CheckLevelUp();

	// 广播经验变化
	OnExperienceChanged.Broadcast(CurrentExperience, GetExpToNextLevel());
}

// ==================== 核心功能：检查升级 ====================
void UESInGameProgressionComponent::CheckLevelUp()
{
	int32 ExpToNext = GetExpToNextLevel();

	// 循环检查，防止一次获得大量经验连升多级
	while (CurrentExperience >= ExpToNext && ExpToNext > 0)
	{
		// 1. 扣除升级所需经验
		CurrentExperience -= ExpToNext;

		// 2. 提升等级
		CurrentLevel++;

		// 3. 获得强化点
		int32 PointsGained = UpgradePointsPerLevel;
		CurrentUpgradePoints += PointsGained;

		UE_LOG(LogTemp, Log, TEXT("【局内成长】升级！当前等级：%d，获得强化点：%d，总强化点：%d"), CurrentLevel, PointsGained,
		       CurrentUpgradePoints);

		// 4. 广播升级事件
		OnLevelUp.Broadcast(CurrentLevel, PointsGained);

		// 5. 更新下一级所需经验
		ExpToNext = GetExpToNextLevel();
	}
}

// ==================== 辅助：获取下一级所需经验 ====================
int32 UESInGameProgressionComponent::GetExpToNextLevel() const
{
	return GetRequiredExpForLevel(CurrentLevel);
}

// ==================== 辅助：从CurveTable读取经验（✅ 修复版，加安全校验+调试日志） ====================
int32 UESInGameProgressionComponent::GetRequiredExpForLevel(int32 Level) const
{
	if (!ExperienceCurveTable)
	{
		UE_LOG(LogTemp, Error, TEXT("【局内成长】错误：ExperienceCurveTable未配置！"));
		return 0;
	}

	// 构造RowName：Level_1, Level_2... 和你的CurveTable完全一致
	FName RowName = *FString::Printf(TEXT("Level_%d"), Level);

	static const FString ContextString(TEXT("ESInGameProgression ExpCurve"));
	FRealCurve* CurveRow = ExperienceCurveTable->FindCurve(RowName, ContextString);
	FCurveTableRowHandle Handle;
	Handle.CurveTable = ExperienceCurveTable;
	Handle.RowName = RowName;
	float OutXY;
	bool found = Handle.Eval(0.0f, &OutXY, ContextString);
	return OutXY;
	
	/*if (CurveRow)
	{
		// 读取Curve的默认值
		float ExpValue = CurveRow->GetDefaultValue();
		UE_LOG(LogTemp, Log, TEXT("【局内成长】读取等级%d的经验行[%s]成功，值：%f"), Level, *RowName.ToString(), ExpValue);

		// ✅ 安全校验：如果是NaN/无效值，返回0，避免溢出
		if (FMath::IsNaN(ExpValue) || ExpValue < 0.f)
		{
			UE_LOG(LogTemp, Error, TEXT("【局内成长】等级%d的经验值无效！"), Level);
			return 0;
		}

		return FMath::RoundToInt(ExpValue);
	}
	else
	{
		// 如果找不到对应等级的Row，说明已达最高级
		UE_LOG(LogTemp, Warning, TEXT("【局内成长】找不到等级%d的经验行[%s]，已达最高级"), Level, *RowName.ToString());
		return 0;
	}*/
}

// ==================== 核心功能：消耗强化点 ====================
bool UESInGameProgressionComponent::SpendUpgradePoints(int32 Amount)
{
	if (Amount <= 0 || CurrentUpgradePoints < Amount)
	{
		return false;
	}

	CurrentUpgradePoints -= Amount;
	UE_LOG(LogTemp, Log, TEXT("【局内成长】消耗强化点：%d，剩余：%d"), Amount, CurrentUpgradePoints);
	return true;
}

// ==================== 核心功能：增加强化点 ====================
void UESInGameProgressionComponent::AddUpgradePoints(int32 Amount)
{
	if (Amount <= 0) return;

	CurrentUpgradePoints += Amount;
	UE_LOG(LogTemp, Log, TEXT("【局内成长】获得强化点：%d，当前：%d"), Amount, CurrentUpgradePoints);
}

// ==================== 核心功能：重置局内数据 ====================
void UESInGameProgressionComponent::ResetInGameData()
{
	CurrentLevel = 1;
	CurrentExperience = 0;
	CurrentUpgradePoints = 0;

	UE_LOG(LogTemp, Log, TEXT("【局内成长】重置局内数据完成"));

	// 广播事件刷新UI
	OnExperienceChanged.Broadcast(CurrentExperience, GetExpToNextLevel());
}
