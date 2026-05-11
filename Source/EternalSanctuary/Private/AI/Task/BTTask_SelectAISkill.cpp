#include "AI/Task/BTTask_SelectAISkill.h"
#include "Character/ESEnemyBase.h"
#include "GAS/Data/ESEnemyAISkillData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTTask_SelectAISkill::UBTTask_SelectAISkill()
{
    NodeName = "Select AI Skill";
}

EBTNodeResult::Type UBTTask_SelectAISkill::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    AESEnemyBase* Enemy = Cast<AESEnemyBase>(AIController->GetPawn());
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!Enemy || !Blackboard)
    {
        return EBTNodeResult::Failed;
    }

    // 调用我们在EnemyBase里写好的权重选择函数
    UESEnemyAISkillData* BestSkill = Enemy->SelectBestSkillByWeight();

    if (BestSkill)
    {
        // 写入黑板（注意：黑板需要有一个Object类型的Key叫SelectedSkillData）
        Blackboard->SetValueAsObject(SelectedSkillKey.SelectedKeyName, Cast<UObject>(BestSkill));
        return EBTNodeResult::Succeeded;
    }

    return EBTNodeResult::Failed;
}