#include "Item/Data/ESInventoryTypes.h"
#include "Item/Data/ESItemBase.h"
#include "Item/Data/ESEquipItemBase.h"

// ==================== FItemSaveData 构造函数实现 ====================
FItemSaveData::FItemSaveData(UESItemBase* Item)
{
    if (!IsValid(Item)) return;

    // 基础数据
    ItemGUID = Item->ItemGUID;
    Name = Item->Name;
    Description = Item->Description;
    Icon = Item->Icon;
    Type = Item->Type;
    Quality = Item->Quality;
    BuyPrice = Item->BuyPrice;
    SellPrice = Item->SellPrice;
    bIsEquipItem = false;

    // 检查是否为装备
    UESEquipItemBase* EquipItem = Cast<UESEquipItemBase>(Item);
    if (EquipItem)
    {
        bIsEquipItem = true;
        // 保存主词条
        MainEntryData = FEntrySaveData(EquipItem->MainEntry);
        // 保存副词条
        for (const FESRuntimeEntry& SubEntry : EquipItem->SubEntries)
        {
            SubEntriesData.Add(FEntrySaveData(SubEntry));
        }
    }
}