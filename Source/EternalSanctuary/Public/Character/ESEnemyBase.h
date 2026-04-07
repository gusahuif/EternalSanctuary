#pragma once
    
    #include "CoreMinimal.h"
    #include "Character/ESCharacterBase.h"
    #include "ESEnemyBase.generated.h"
    
    /**
     * ESEnemyBase
     * 敌人角色基类，扩展敌人 AI 与战斗行为入口。
     */
    UCLASS(Blueprintable)
    class ETERNALSANCTUARY_API AESEnemyBase : public AESCharacterBase
    {
        GENERATED_BODY()
    
    public:
        AESEnemyBase();
    
    protected:
        virtual void BeginPlay() override;
    
    public:
        UFUNCTION(BlueprintCallable, Category = "Enemy")
        virtual void InitializeEnemyCharacter();
    };
    
