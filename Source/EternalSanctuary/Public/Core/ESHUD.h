#pragma once
    
    #include "CoreMinimal.h"
    #include "GameFramework/HUD.h"
    #include "ESHUD.generated.h"
    
    /**
     * ESHUD
     * HUD 基类，用于承载后续 UMG 与调试界面逻辑。
     */
    UCLASS(Blueprintable)
    class ETERNALSANCTUARY_API AESHUD : public AHUD
    {
        GENERATED_BODY()
    
    public:
        AESHUD();
    
    protected:
        virtual void BeginPlay() override;
    
    public:
        UFUNCTION(BlueprintCallable, Category = "HUD")
        virtual void InitializeHUD();
    };
    
