#pragma once

#include "CoreMinimal.h"
#include "Item/Pickup/ESPickupBase.h"
#include "ESPickupEquip.generated.h"

class UESEquipItemBase;

UCLASS()
class ETERNALSANCTUARY_API AESPickupEquip : public AESPickupBase
{
	GENERATED_BODY()
	
public:	
	AESPickupEquip();

protected:
	virtual void BeginPlay() override;

public:	
	// ==================== 装备数据 ====================
	/** 保存的装备实例 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Equip")
	UESEquipItemBase* SavedEquipItem;

	// ==================== 初始化 ====================
	/** 用装备数据初始化拾取物 */
	UFUNCTION(BlueprintCallable, Category = "Pickup|Equip")
	void InitializeWithEquip(UESEquipItemBase* EquipItem);

	// ==================== 交互逻辑 ====================
	virtual void OnInteract_Implementation(APawn* InteractingPawn) override;
};