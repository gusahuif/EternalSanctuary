#pragma once

#include "CoreMinimal.h"
#include "Item/Pickup/ESPickupBase.h"
#include "ESPickupGold.generated.h"

UCLASS()
class ETERNALSANCTUARY_API AESPickupGold : public AESPickupBase
{
	GENERATED_BODY()
	
public:	
	AESPickupGold();

protected:
	virtual void BeginPlay() override;

public:	
	// ==================== 金币数据 ====================
	/** 金币数量 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Gold")
	int32 GoldAmount = 100;

	// ==================== 自动拾取碰撞 ====================
	/** 碰撞体（用于自动拾取） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|Gold")
	USphereComponent* AutoPickupCollision;

	// ==================== 碰撞回调 ====================
	UFUNCTION()
	void OnAutoPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult);

	// ==================== 交互逻辑 ====================
	virtual void OnInteract_Implementation(APawn* InteractingPawn) override;
};