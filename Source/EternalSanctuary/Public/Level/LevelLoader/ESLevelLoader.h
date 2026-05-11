#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Engine/StreamableManager.h"
#include "ESLevelLoader.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoadingProgressUpdated, float, Progress);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnLoadingFinished);

UCLASS(BlueprintType, Blueprintable)
class ETERNALSANCTUARY_API UESLevelLoader : public UObject
{
	GENERATED_BODY()

public:
	UESLevelLoader();

	UPROPERTY(BlueprintAssignable, Category = "ES|Loading")
	FOnLoadingProgressUpdated OnLoadingProgressUpdated;

	UPROPERTY(BlueprintAssignable, Category = "ES|Loading")
	FOnLoadingFinished OnLoadingFinished;

	/** 开始预加载并切换关卡（最终版） */
	UFUNCTION(BlueprintCallable, Category = "ES|Loading")
	void StartLoadAndSwitch(FName LevelName);

	UFUNCTION(BlueprintPure, Category = "ES|Loading")
	float GetCurrentProgress() const { return CurrentProgress; }

private:
	float CurrentProgress = 0.0f;
	FName TargetLevelName;
	FStreamableManager StreamableManager;
	TSharedPtr<FStreamableHandle> LoadHandle;
	FTimerHandle LoadingTimerHandle;

	/** 内部：轮询进度 */
	void PollProgress();

	/** 内部：加载完成 */
	void OnLoadComplete();

	/** 内部：切换关卡 */
	void DoSwitchLevel();
};