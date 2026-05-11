#include "Core/ESGameInstance.h"
#include "GAS/GameplayTag/ESGameplayTags.h"

UESGameInstance::UESGameInstance()
{
	// 初始化为空
	NextLevelName = NAME_None;
}

void UESGameInstance::Init()
{
	Super::Init();

	// 初始化项目全局 GameplayTag 静态缓存
	// 所有 FESGameplayTags::Xxx 成员在此之后均可安全使用
	FESGameplayTags::InitializeNativeTags();

	UE_LOG(LogTemp, Log, TEXT("[ESGameInstance] Initialized. GameplayTags cached successfully."));
}

void UESGameInstance::Shutdown()
{
	Super::Shutdown();

	UE_LOG(LogTemp, Log, TEXT("[ESGameInstance] Shutdown."));
}

void UESGameInstance::SetNextLevel(FName LevelName)
{
	NextLevelName = LevelName;
	UE_LOG(LogTemp, Log, TEXT("已设置下一个关卡为：%s"), *LevelName.ToString());
}
