#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTS_UpdateCombatData.generated.h"

/**
 * 定时更新AI战斗相关的所有黑板数据
 */
UCLASS()
class ETERNALSANCTUARY_API UBTS_UpdateCombatData : public UBTService_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTS_UpdateCombatData();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};