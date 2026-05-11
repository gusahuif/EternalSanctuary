#include "AI/Service/BTS_UpdateCombatData.h"
#include "Character/ESEnemyBase.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"

UBTS_UpdateCombatData::UBTS_UpdateCombatData()
{
    NodeName = "Update Combat Data";
    // 每0.1秒更新一次，性能与实时性平衡
    Interval = 0.1f;
    RandomDeviation = 0.02f;
}

void UBTS_UpdateCombatData::TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickNode(OwnerComp, NodeMemory, DeltaSeconds);

    AAIController* AIController = OwnerComp.GetAIOwner();
    AESEnemyBase* Enemy = Cast<AESEnemyBase>(AIController->GetPawn());
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!Enemy || !Blackboard)
    {
        return;
    }

    // 1. 更新距离
    if (Enemy->GetCurrentTarget())
    {
        const float Distance = FVector::Dist2D(Enemy->GetActorLocation(), Enemy->GetCurrentTarget()->GetActorLocation());
        Blackboard->SetValueAsFloat("DistanceToTarget", Distance);
    }

    // 2. 更新 bCanAct
    Blackboard->SetValueAsBool("bCanAct", Enemy->CanAct());

    // 3. 更新 bHasValidTarget
    Blackboard->SetValueAsBool("bHasValidTarget", Enemy->HasValidTarget());

    // 4. 更新 bIsAttacking
    Blackboard->SetValueAsBool("bIsAttacking", Enemy->bIsAttacking);

    // 5. 更新 bIsDead (利用你ASC的IsDead)
    UESAbilitySystemComponent* ASC = Enemy->GetESAbilitySystemComponent();
    if (ASC)
    {
        Blackboard->SetValueAsBool("bIsDead", ASC->IsDead());
    }
}