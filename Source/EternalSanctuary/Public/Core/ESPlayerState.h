#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerState.h"
#include "ESPlayerState.generated.h"

/**
 * ESPlayerState
 * 玩家状态数据基类，保存分数、资源与局内状态数据。
 */
UCLASS(Blueprintable)
class ETERNALSANCTUARY_API AESPlayerState : public APlayerState
{
	GENERATED_BODY()

public:
	AESPlayerState();

protected:
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player State")
	int32 ScoreValue = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Player State")
	int32 ResourceCount = 0;

	virtual void BeginPlay() override;

public:
	UFUNCTION(BlueprintCallable, Category = "Player State")
	virtual void AddScoreValue(int32 InScore);

	UFUNCTION(BlueprintCallable, Category = "Player State")
	virtual void AddResource(int32 InAmount);
};
