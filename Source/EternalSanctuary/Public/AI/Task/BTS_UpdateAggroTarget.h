#pragma once

#include "CoreMinimal.h"
#include "BehaviorTree/Services/BTService_BlackboardBase.h"
#include "BTS_UpdateAggroTarget.generated.h"

UCLASS()
class ETERNALSANCTUARY_API UBTS_UpdateAggroTarget : public UBTService_BlackboardBase
{
    GENERATED_BODY()

public:
    UBTS_UpdateAggroTarget();

protected:
    virtual void TickNode(UBehaviorTreeComponent& OwnerComp, uint8* NodeMemory, float DeltaSeconds) override;
};