#include "Level/LevelLoader/ESLevelLoader.h"
#include "Kismet/GameplayStatics.h"

UESLevelLoader::UESLevelLoader()
{
	CurrentProgress = 0.0f;
}

void UESLevelLoader::StartLoadAndSwitch(FName LevelName)
{
	TargetLevelName = LevelName;
	CurrentProgress = 0.0f;

	UE_LOG(LogTemp, Log, TEXT("【加载】开始预加载关卡：%s"), *LevelName.ToString());

	// 1. 构建关卡路径（请根据你实际的文件夹修改 /Game/Maps/）
	FString LevelPathString = FString::Printf(TEXT("/Game/Maps/%s.%s"), *LevelName.ToString(), *LevelName.ToString());
	FSoftObjectPath LevelSoftPath(LevelPathString);

	// 2. 创建资源列表
	TArray<FSoftObjectPath> AssetsToLoad;
	AssetsToLoad.Add(LevelSoftPath);

	// 3. 【核心】预加载资源，只传完成回调
	LoadHandle = StreamableManager.RequestAsyncLoad(
		AssetsToLoad,
		FStreamableDelegate::CreateUObject(this, &UESLevelLoader::OnLoadComplete)
	);

	// 4. 启动定时器轮询进度
	GetWorld()->GetTimerManager().SetTimer(LoadingTimerHandle, this, &UESLevelLoader::PollProgress, 0.05f, true);
}

void UESLevelLoader::PollProgress()
{
	if (!LoadHandle.IsValid()) return;

	// 1. 获取真实进度
	CurrentProgress = LoadHandle->GetProgress();

	// 2. 广播给UI
	OnLoadingProgressUpdated.Broadcast(CurrentProgress);

	// 3. 打印日志（每10%一次）
	static int32 LastLoggedPercent = -1;
	int32 CurrentPercent = (int32)(CurrentProgress * 100);
	if (CurrentPercent != LastLoggedPercent && CurrentPercent % 10 == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("【加载】进度：%d%%"), CurrentPercent);
		LastLoggedPercent = CurrentPercent;
	}
}

void UESLevelLoader::OnLoadComplete()
{
	// 1. 停止轮询
	GetWorld()->GetTimerManager().ClearTimer(LoadingTimerHandle);

	UE_LOG(LogTemp, Log, TEXT("【加载】资源预加载完成！"));

	// 2. 设为100%
	CurrentProgress = 1.0f;
	OnLoadingProgressUpdated.Broadcast(CurrentProgress);
	OnLoadingFinished.Broadcast();

	// 3. 延迟0.5秒切换关卡
	FTimerHandle SwitchTimerHandle;
	GetWorld()->GetTimerManager().SetTimer(SwitchTimerHandle, this, &UESLevelLoader::DoSwitchLevel, 0.5f, false);
}

void UESLevelLoader::DoSwitchLevel()
{
	UE_LOG(LogTemp, Log, TEXT("正在切换到关卡：%s"), *TargetLevelName.ToString());
	UGameplayStatics::OpenLevel(GetWorld(), TargetLevelName);
}