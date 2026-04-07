#pragma once
    
    #include "CoreMinimal.h"
    #include "GameFramework/GameStateBase.h"
    #include "ESGameState.generated.h"
    
    UENUM(BlueprintType)
    enum class EESGamePhase : uint8
    {
        None,
        Preparation,
        Combat,
        Reward,
        GameOver
    };
    
    /**
     * ESGameState
     * 全局游戏状态基类，保存波次进度与当前游戏阶段。
     */
    UCLASS(Blueprintable)
    class ETERNALSANCTUARY_API AESGameState : public AGameStateBase
    {
        GENERATED_BODY()
    
    public:
        AESGameState();
    
    protected:
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
        int32 CurrentWaveIndex = 0;
    
        UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Game State")
        EESGamePhase CurrentGamePhase = EESGamePhase::None;
    
	virtual void BeginPlay() override;
    public:
        UFUNCTION(BlueprintCallable, Category = "Game State")
        virtual void SetCurrentWaveIndex(int32 InWaveIndex);
    
        UFUNCTION(BlueprintCallable, Category = "Game State")
        virtual void SetGamePhase(EESGamePhase InGamePhase);
    };
    
