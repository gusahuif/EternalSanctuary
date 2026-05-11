#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Item/Data/ESItemBase.h"
#include "Item/Data/ESEquipItemBase.h"
#include "Item/Data/ESInventoryTypes.h"
#include "AbilitySystemInterface.h"
#include "ESInventoryManagerComponent.generated.h"

class AESPickupEquip;
class AESPickupGold;
// UI刷新委托
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnInventoryUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnEquipmentUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnGoldChanged, int32, NewGold);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnWarehouseUpdated);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnShopItemsUpdated);

class UESAbilitySystemComponent;

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ETERNALSANCTUARY_API UESInventoryManagerComponent : public UActorComponent
{
    GENERATED_BODY()

public:    
    UESInventoryManagerComponent();

protected:
    virtual void BeginPlay() override;

public:
    // ==================== 配置参数 ====================
    /** 背包最大容量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Config")
    int32 MaxInventorySize = 40;

    /** 仓库最大容量 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Config")
    int32 MaxWarehouseSize = 200;
    
	/** 装备拾取物蓝图类（如果不填，默认用C++基类） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Config|Pickup")
	TSubclassOf<AESPickupEquip> PickupEquipClass;

	/** 金币拾取物蓝图类（如果不填，默认用C++基类） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Config|Pickup")
	TSubclassOf<AESPickupGold> PickupGoldClass;

    // ==================== 核心数据存储 ====================
    /** 背包物品数组 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Data")
    TArray<UESItemBase*> Inventory;

    /** 装备栏Map：类型->装备实例 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Data")
    TMap<EESItemType, UESEquipItemBase*> Equipment;

    /** 仓库物品数组 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Data")
    TArray<UESItemBase*> Warehouse;

    /** 商店当前售卖物品列表 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Data")
    TArray<UESItemBase*> ShopItems;

    /** 玩家金币 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Data")
    int32 CurrentGold = 0;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Data")
	TSubclassOf<UGameplayEffect> EquipGEClass;

    // ==================== 全局事件（UI绑定专用） ====================
    /** 背包物品变化时广播 */
    UPROPERTY(BlueprintAssignable, Category = "ES|Delegates")
    FOnInventoryUpdated OnInventoryUpdated;

    /** 装备栏变化时广播 */
    UPROPERTY(BlueprintAssignable, Category = "ES|Delegates")
    FOnEquipmentUpdated OnEquipmentUpdated;

    /** 金币变化时广播 */
    UPROPERTY(BlueprintAssignable, Category = "ES|Delegates")
    FOnGoldChanged OnGoldChanged;

	/** 仓库物品变化时广播 */
	UPROPERTY(BlueprintAssignable, Category = "ES|Delegates")
	FOnWarehouseUpdated OnWarehouseUpdated;

	/** 商店物品变化时广播 */
	UPROPERTY(BlueprintAssignable, Category = "ES|Delegates")
	FOnShopItemsUpdated OnShopItemsUpdated;

    // ==================== 核心功能函数 ====================
    /** 统一物品生成入口，自动分配唯一GUID */
    UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Core")
    UESItemBase* CreateItem(TSubclassOf<UESItemBase> ItemClass);

	/** 装备物品（背包->装备栏，自动联动GAS，可选指定来源槽位，旧装备放回该槽位） */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Equip")
	bool EquipItem(UESEquipItemBase* EquipItem, int32 SourceSlotIndex = -1);

	/** 卸下装备（装备栏->背包，自动移除GAS效果，可选指定放回的槽位） */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Equip")
	bool UnequipItem(EESItemType EquipType, int32 TargetSlotIndex = -1);
	
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Equip")
	void ApplyEquipStats(UESAbilitySystemComponent* ASC, UESEquipItemBase* EquipItem);

    /** 购买物品（商店->背包，扣除金币） */
    UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Shop")
    bool BuyItem(UESItemBase* ShopItem);

    /** 出售物品（背包->销毁，增加金币） */
    UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Shop")
    bool SellItem(UESItemBase* Item);

	/** 刷新商店商品（清空旧的，生成新的） */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Shop")
	void RefreshShopItems(UDataTable* EntryLibrary, UDataTable* EquipTemplateTable, int32 ItemCount = 10);

	/** 手动触发背包UI刷新（供外部调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|UI")
	void TriggerInventoryRefresh();

	/** 手动触发装备栏UI刷新（供外部调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|UI")
	void TriggerEquipmentRefresh();
	
	/** 蓝图安全获取仓库物品：索引无效时返回nullptr，避免数组越界 */
	UFUNCTION(BlueprintPure, Category = "ES|Inventory")
	UESItemBase* GetWarehouseItemAt(int32 Index);
	
	/** 蓝图安全获取背包物品：索引无效时返回nullptr，避免数组越界 */
	UFUNCTION(BlueprintPure, Category = "ES|Inventory")
	UESItemBase* GetInventoryItemAt(int32 Index);
	
	/** 蓝图安全获取商店物品 */
	UFUNCTION(BlueprintPure, Category = "ES|Inventory")
	UESItemBase* GetShopItemAt(int32 Index);
	
    /** 存入仓库（背包->仓库） */
    UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Warehouse")
    bool DepositToWarehouse(UESItemBase* Item);

    /** 取出仓库物品（仓库->背包） */
    UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Warehouse")
    bool RetrieveFromWarehouse(UESItemBase* Item);

	/** 找背包第一个空位索引，没有空位返回-1 */
	UFUNCTION(BlueprintPure, Category = "ES|Inventory|Core")
	int32 FindFirstEmptyInventorySlot() const;

	/** 找仓库第一个空位索引，没有空位返回-1 */
	UFUNCTION(BlueprintPure, Category = "ES|Inventory|Core")
	int32 FindFirstEmptyWarehouseSlot() const;

	/** 把物品放到背包指定索引（索引无效或已有物品返回false） */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Core")
	bool SetItemToInventorySlot(UESItemBase* Item, int32 SlotIndex);

	/** 把物品放到仓库指定索引 */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Core")
	bool SetItemToWarehouseSlot(UESItemBase* Item, int32 SlotIndex);

	/** 清空背包指定索引的物品（返回被清空的物品，没有则返回nullptr） */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Core")
	UESItemBase* ClearInventorySlot(int32 SlotIndex);

	/** 清空仓库指定索引的物品 */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Core")
	UESItemBase* ClearWarehouseSlot(int32 SlotIndex);

	/** 一键整理背包：把所有物品往前移，清空后面的空位 */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Sort")
	void SortInventory();

	/** 一键整理仓库 */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Sort")
	void SortWarehouse();
	
    /** 修改玩家金币（负数为扣除） */
    UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Gold")
    bool ModifyGold(int32 Delta);

    /** 获取玩家ASC（GAS核心） */
    UFUNCTION(BlueprintCallable, Category = "ES|Inventory|GAS")
    UAbilitySystemComponent* GetPlayerASC() const;

    // ==================== 调试工具 ====================
    /** 生成测试装备到背包 */
    UFUNCTION(BlueprintCallable, Category = "ES|Debug")
    void AddTestEquipItems();
    
    /**
         * 从 DataTable 生成装备并放入背包
         * @param DataTable 装备数据表
         * @param RowName 表中的行名（如 "Weapon_Sword_01"）
         * @return 生成的装备实例
         */
    UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Data")
    UESEquipItemBase* CreateEquipFromDataTable(UDataTable* DataTable, FName RowName);

	/**
	 * 从词条库随机生成一个全新的装备 (完整版)
	 * @param EntryLibrary 词条库DataTable
	 * @param EquipTemplateTable 装备模板DataTable
	 * @param TemplateID 装备模板行名
	 * @param Quality 装备品质
	 * @return 生成的装备实例
	 */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Entry")
	UESEquipItemBase* GenerateRandomEquip(UDataTable* EntryLibrary, UDataTable* EquipTemplateTable, FName TemplateID, EESItemQuality Quality);

	// ==================== 【核心接口】装备强化系统 ====================
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Upgrade")
	int32 GetNextUnlockableSubEntryIndex(UESEquipItemBase* EquipItem);

	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Upgrade")
	bool TryUnlockNextSubEntry(UESEquipItemBase* EquipItem);

	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Upgrade")
	bool IsSubEntryUnlocked(UESEquipItemBase* EquipItem, int32 Index);

	/** 重置装备的所有副词条为锁定状态（回到主城时调用） */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Upgrade")
	void ResetAllSubEntryLocks(UESEquipItemBase* EquipItem);

	/** 重置背包和装备栏里所有装备的词条锁定 */
	UFUNCTION(BlueprintCallable, Category = "ES|Inventory|Upgrade")
	void ResetAllEquipmentsInInventory();
	
	// ==================== 【存档系统】存档/读档 ====================
	/** 保存游戏 */
	UFUNCTION(BlueprintCallable, Category = "ES|Save")
	void SaveGame();

	/** 读取游戏 */
	UFUNCTION(BlueprintCallable, Category = "ES|Save")
	void LoadGame();
	
private:
	// ==================== 【新增】内部辅助函数声明 (解决红色报错) ====================
	/** 内部辅助：从词条库随机抽取并填充一个词条 */
	bool RollAndFillEntry(UDataTable* EntryLibrary, const TArray<FName>& AllowedIDs, int32 Level, TArray<FName>& OutUsedIDs, FESRuntimeEntry& OutEntry);

	/** 内部辅助：在Min/Max之间随机数值 */
	float RandomValueInRange(float Min, float Max);
private:
    // 内部辅助函数
    bool HasInventorySpace() const;
    bool HasWarehouseSpace() const;
    void NotifyInventoryChanged();
    void NotifyEquipmentChanged();
};