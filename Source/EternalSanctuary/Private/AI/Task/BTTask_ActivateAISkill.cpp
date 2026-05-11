#include "AI/Task/BTTask_ActivateAISkill.h"
#include "Character/ESEnemyBase.h"
#include "GAS/Data/ESEnemyAISkillData.h"
#include "BehaviorTree/BlackboardComponent.h"
#include "AIController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"

UBTTask_ActivateAISkill::UBTTask_ActivateAISkill()
{
    NodeName = "Activate AI Skill";
    bNotifyTick = true; // 开启Tick
}

EBTNodeResult::Type UBTTask_ActivateAISkill::ExecuteTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory)
{
    AAIController* AIController = OwnerComp.GetAIOwner();
    CachedEnemy = Cast<AESEnemyBase>(AIController->GetPawn());
    UBlackboardComponent* Blackboard = OwnerComp.GetBlackboardComponent();

    if (!CachedEnemy.IsValid() || !Blackboard)
    {
        return EBTNodeResult::Failed;
    }

    // 1. 从黑板读取技能数据
    UESEnemyAISkillData* SkillData = Cast<UESEnemyAISkillData>(Blackboard->GetValueAsObject(SelectedSkillKey.SelectedKeyName));
    if (!SkillData)
    {
        return EBTNodeResult::Failed;
    }

    CachedAbilityTag = SkillData->AbilityActivationTag;

    // 2. 面向目标（如果需要）
    if (SkillData->bNeedFaceTarget && CachedEnemy->GetCurrentTarget())
    {
        // 简单的面向逻辑，或者用UE自带的Rotate To Face BB Entry节点
        FVector Dir = CachedEnemy->GetCurrentTarget()->GetActorLocation() - CachedEnemy->GetActorLocation();
        Dir.Z = 0;
        CachedEnemy->SetActorRotation(Dir.Rotation());
    }

    // 3. 停止移动（如果需要）
    if (SkillData->bStopMovementOnUse)
    {
        CachedEnemy->GetCharacterMovement()->StopMovementImmediately();
    }

    // 4. 标记攻击开始
    CachedEnemy->MarkAttackStarted();

    // 5. 激活技能
    if (CachedEnemy->TryActivateAISkillByTag(CachedAbilityTag))
    {
        return EBTNodeResult::InProgress; // 等待Tick中检查结束
    }
    else
    {
        // 激活失败，结束攻击状态
        CachedEnemy->MarkAttackFinished();
        return EBTNodeResult::Failed;
    }
}

void UBTTask_ActivateAISkill::TickTask(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds)
{
    Super::TickTask(OwnerComp, NodeMemory, DeltaSeconds);

    if (!CachedEnemy.IsValid())
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    UESAbilitySystemComponent* ASC = CachedEnemy->GetESAbilitySystemComponent();
    if (!ASC)
    {
        FinishLatentTask(OwnerComp, EBTNodeResult::Failed);
        return;
    }

    // 检查技能是否还在激活
    // 注意：这里假设GA激活期间会有State.Attacking标签，或者GA结束时会触发回调
    // 简单做法：检查IsAbilityActiveByTag，或者检查bIsAttacking状态
    if (!ASC->IsAbilityActiveByTag(CachedAbilityTag) && !CachedEnemy->bIsAttacking)
    {
        // 技能已经结束
        CachedEnemy->MarkAttackFinished();
        FinishLatentTask(OwnerComp, EBTNodeResult::Succeeded);
    }
}