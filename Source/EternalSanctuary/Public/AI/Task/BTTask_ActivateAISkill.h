#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "GameplayTagContainer.h"
#include "BTTask_ActivateAISkill.generated.h"

class AESEnemyBase; // 前向声明
class UESEnemyAISkillData; // 前向声明

/**
 * 激活选中的AI技能，并等待技能结束
 */
UCLASS()
class ETERNALSANCTUARY_API UBTTask_ActivateAISkill : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_ActivateAISkill();

    /** 从哪个黑板Key读取技能数据 */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector SelectedSkillKey;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
    virtual void TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;

private:
    // 缓存的技能Tag，用于Tick中检查是否释放完
    FGameplayTag CachedAbilityTag;
    TWeakObjectPtr<AESEnemyBase> CachedEnemy;
};