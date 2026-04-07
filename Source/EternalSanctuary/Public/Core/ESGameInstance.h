#pragma once
    
    #include "CoreMinimal.h"
    #include "Engine/GameInstance.h"
    #include "ESGameInstance.generated.h"
    
    /**
     * UESGameInstance
     * 项目全局 GameInstance
     * 负责生命周期最长的全局初始化逻辑，包括 GameplayTag 静态缓存初始化
     */
    UCLASS()
    class ETERNALSANCTUARY_API UESGameInstance : public UGameInstance
    {
    	GENERATED_BODY()
    
    public:
    	/**
    	 * 游戏启动时的全局初始化入口
    	 * 在此处完成项目级别的单次初始化工作
    	 */
    	virtual void Init() override;
    
    	/**
    	 * 游戏关闭时的清理入口
    	 */
    	virtual void Shutdown() override;
    };
    