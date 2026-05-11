#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/BTTaskNode.h"
#include "BTTask_SelectAISkill.generated.h"

/**
 * 从Enemy的技能列表中根据权重选择最优技能，写入黑板
 */
UCLASS()
class ETERNALSANCTUARY_API UBTTask_SelectAISkill : public UBTTaskNode
{
    GENERATED_BODY()

public:
    UBTTask_SelectAISkill();

    /** 选中的技能要写入的黑板Key */
    UPROPERTY(EditAnywhere, Category = "Blackboard")
    FBlackboardKeySelector SelectedSkillKey;

protected:
    virtual EBTNodeResult::Type ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory) override;
};