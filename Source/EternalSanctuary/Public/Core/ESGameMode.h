#pragma once
    
    #include "CoreMinimal.h"
    #include "GameFramework/GameModeBase.h"
    #include "ESGameMode.generated.h"
    
    /**
     * ESGameMode
     * 游戏模式基类，负责管理游戏状态与波次流程。
     */
    UCLASS(Blueprintable)
    class ETERNALSANCTUARY_API AESGameMode : public AGameModeBase
    {
        GENERATED_BODY()
    
    public:
        AESGameMode();
    
    protected:
        UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Game Flow")
        int32 StartingWaveIndex = 1;
    
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game Flow")
        bool bIsGameStarted = false;
    
	virtual void BeginPlay() override;
    public:
        UFUNCTION(BlueprintCallable, Category = "Game Flow")
        virtual void StartGame();
    
        UFUNCTION(BlueprintCallable, Category = "Game Flow")
        virtual void StartNextWave();
    
        UFUNCTION(BlueprintCallable, Category = "Game Flow")
        virtual void EndGame(bool bWasSuccessful);
    };
    
