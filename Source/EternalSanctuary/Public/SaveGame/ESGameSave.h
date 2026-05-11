#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "Item/Data/ESInventoryTypes.h" // 引入我们刚才写的存档数据结构体
#include "ESGameSave.generated.h"

UCLASS()
class ETERNALSANCTUARY_API UESGameSave : public USaveGame
{
	GENERATED_BODY()

public:
    // ==================== 存档槽位信息 ====================
    /** 存档槽位名称 */
    UPROPERTY()
    FString SaveSlotName = "PlayerSave01";

    /** 存档索引 */
    UPROPERTY()
    int32 SaveIndex = 0;

    // ==================== 玩家核心数据 ====================
    /** 玩家金币 */
    UPROPERTY()
    int32 PlayerGold = 0;

    /** 背包物品存档数据数组 */
    UPROPERTY()
    TArray<FItemSaveData> InventorySaveData;

    /** 装备栏存档数据：类型 -> 物品存档数据 */
    UPROPERTY()
    TMap<EESItemType, FItemSaveData> EquipmentSaveData;

    /** 仓库物品存档数据数组 */
    UPROPERTY()
    TArray<FItemSaveData> WarehouseSaveData;

    // 默认构造函数
    UESGameSave();
};