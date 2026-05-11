#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Item/Data/ESInventoryTypes.h"
#include "Item/Data/ESItemBase.h"
#include "ESItemSlotWidget.generated.h"

// 槽位点击事件委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotLeftClicked, UESItemSlotWidget*, SlotWidget);

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnSlotRightClicked, UESItemSlotWidget*, SlotWidget);

UCLASS()
class ETERNALSANCTUARY_API UESItemSlotWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	// ==================== 核心绑定控件（蓝图里必须绑定同名控件） ====================
	/** 物品图标 */
	UPROPERTY(meta = (BindWidget))
	class UImage* Img_ItemIcon;

	/** 物品品质边框 */
	UPROPERTY(meta = (BindWidget))
	class UBorder* Border_Quality;

	/** 物品名称文本 */
	UPROPERTY(meta = (BindWidget))
	class UTextBlock* Text_ItemName;

	// ==================== 槽位数据 ====================
	/** 当前槽位绑定的物品 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Slot")
	UESItemBase* CurrentItem;
	
	/** 当前槽位的索引（背包/仓库用，初始化时由父面板设置） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Slot")
	int32 SlotIndex = -1;
	
	/** 当前槽位的上下文类型 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Slot")
	EESSlotContext SlotContext;

	/** 如果是装备栏槽位，这里绑定对应的装备类型（Weapon/Armor/Accessory） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Slot|Equipment")
	EESItemType BindEquipType;

	/** 是否处于商店模式（背包槽位右键变为出售） */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Slot")
	bool bIsInShopMode = false;

	// ==================== 点击事件（蓝图可绑定） ====================
	UPROPERTY(BlueprintAssignable, Category = "ES|Slot|Events")
	FOnSlotLeftClicked OnLeftClicked;

	UPROPERTY(BlueprintAssignable, Category = "ES|Slot|Events")
	FOnSlotRightClicked OnRightClicked;

	// ==================== 核心函数 ====================
	/** 刷新槽位UI显示 */
	UFUNCTION(BlueprintCallable, Category = "ES|Slot")
	void RefreshSlot(UESItemBase* NewItem);

	/** 清空槽位显示 */
	UFUNCTION(BlueprintCallable, Category = "ES|Slot")
	void ClearSlot();

	/** 开始拖拽时调用（蓝图可覆盖） */
	UFUNCTION(BlueprintNativeEvent, Category = "ES|Slot|DragDrop")
	void OnDragStarted();
	virtual void OnDragStarted_Implementation();

	/** 拖拽结束时调用（不管成功还是取消） */
	UFUNCTION(BlueprintNativeEvent, Category = "ES|Slot|DragDrop")
	void OnDragEnded(bool bWasSuccess);
	virtual void OnDragEnded_Implementation(bool bWasSuccess);

	/** 拖拽到UI外时丢弃物品 */
	UFUNCTION(BlueprintCallable, Category = "ES|Slot")
	void DropItemToWorld();
	
protected:
	virtual void NativeOnInitialized() override;
	virtual FReply NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent) override;

	// ==================== 重写原生拖拽事件 ====================
	virtual void NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation) override;
	virtual bool NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
	virtual void NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation) override;
private:
	// ==================== 【新增】内部辅助：获取InventoryManager ====================
	class UESInventoryManagerComponent* GetInventoryManager() const;
};
