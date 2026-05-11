#include "AI/Task/BTS_UpdateAggroTarget.h"
#include "Character/ESEnemyBase.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"

UBTS_UpdateAggroTarget::UBTS_UpdateAggroTarget()
{
    NodeName = "Update Aggro Target";
    Interval = 0.2f; // 每0.2秒更新一次，性能与反应速度平衡
    RandomDeviation = 0.05f;
}

void UBTS_UpdateAggroTarget::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIController = OwnerComp.GetAIOwner();
    AESEnemyBase* Enemy = Cast<AESEnemyBase>(AIController->GetPawn());
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!Enemy || !Blackboard)
    {
        return;
    }

    // 调用我们刚才写的获取仇恨目标的函数
    AActor* NewTarget = Enemy->GetCurrentAggroTarget();

    // 更新黑板
    if (NewTarget)
    {
        Blackboard->SetValueAsObject(FName("TargetActor"), NewTarget);
        // 同时调用一下你原有的 SetCurrentTarget，保持数据同步
        Enemy->SetCurrentTarget(NewTarget);
    }
    else
    {
        Blackboard->ClearValue(FName("TargetActor"));
        Enemy->ClearCurrentTarget();
    }
}