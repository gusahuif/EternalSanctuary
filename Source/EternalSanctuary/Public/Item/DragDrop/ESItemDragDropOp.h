#pragma once

#include "CoreMinimal.h"
#include "Blueprint/DragDropOperation.h"
#include "Item/Data/ESInventoryTypes.h"
#include "ESItemDragDropOp.generated.h"

class UESItemSlotWidget;
class UESItemBase;

UCLASS()
class ETERNALSANCTUARY_API UESItemDragDropOp : public UDragDropOperation
{
	GENERATED_BODY()

public:
	// ==================== 拖拽核心数据 ====================
	/** 正在拖拽的物品 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|DragDrop")
	UESItemBase* DraggedItem = nullptr;

	/** 来源槽位UI的引用 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|DragDrop")
	UESItemSlotWidget* SourceSlot = nullptr;

	/** 来源槽位的上下文（从哪来的：背包/装备栏/仓库） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|DragDrop")
	EESSlotContext SourceContext = EESSlotContext::Inventory;

	/** 如果来源是装备栏，保存装备类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|DragDrop")
	EESItemType SourceEquipType = EESItemType::Weapon;

	/** 来源槽位的索引（背包/仓库用） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|DragDrop")
	int32 SourceSlotIndex = -1;
};