#include "Item/Widget/ESItemSlotWidget.h"
#include "Components/Image.h"
#include "Components/Border.h"
#include "Components/TextBlock.h"
#include "GameFramework/PlayerState.h"
#include "Item/Data/ESEquipItemBase.h"
#include "Item/DragDrop/ESItemDragDropOp.h"
#include "Item/Manager/ESInventoryManagerComponent.h"
#include "GAS/ASC/ESAbilitySystemComponent.h"
#include "Item/Pickup/ESPickupEquip.h"

void UESItemSlotWidget::OnDragEnded_Implementation(bool bWasSuccess)
{
}

void UESItemSlotWidget::DropItemToWorld()
{
    UE_LOG(LogTemp, Log, TEXT("【丢弃物品】开始执行，当前SlotIndex：%d，SlotContext：%d"), SlotIndex, (int32)SlotContext);

    // 1. 基础校验
    if (!IsValid(CurrentItem))
    {
        UE_LOG(LogTemp, Error, TEXT("【丢弃物品】失败：CurrentItem无效！"));
        return;
    }

    UESInventoryManagerComponent* InventoryManager = GetInventoryManager();
    if (!InventoryManager)
    {
        UE_LOG(LogTemp, Error, TEXT("【丢弃物品】失败：InventoryManager无效！"));
        return;
    }

    // 2. 获取玩家位置
    APlayerController* PC = GetOwningPlayer();
    if (!PC) return;
    APawn* Pawn = PC->GetPawn();
    if (!Pawn) return;

    FVector SpawnLocation = Pawn->GetActorLocation();
    FRotator SpawnRotation = Pawn->GetActorRotation();

    UE_LOG(LogTemp, Log, TEXT("【丢弃物品】准备生成拾取物，位置：%s"), *SpawnLocation.ToString());

    // 3. 先从背包/仓库移除物品
    bool bRemoveSuccess = false;
    if (SlotContext == EESSlotContext::Inventory)
    {
        if (InventoryManager->GetInventoryItemAt(SlotIndex) == CurrentItem)
        {
            InventoryManager->ClearInventorySlot(SlotIndex);
            bRemoveSuccess = true;
        }
    }
    else if (SlotContext == EESSlotContext::Warehouse)
    {
        if (InventoryManager->GetWarehouseItemAt(SlotIndex) == CurrentItem)
        {
            InventoryManager->ClearWarehouseSlot(SlotIndex);
            bRemoveSuccess = true;
        }
    }
    else if (SlotContext == EESSlotContext::Equipment)
    {
        // 装备栏丢弃：先卸下，再生成拾取物
        InventoryManager->UnequipItem(BindEquipType, SlotIndex);
        bRemoveSuccess = true;
    }

    if (!bRemoveSuccess)
    {
        UE_LOG(LogTemp, Error, TEXT("【丢弃物品】失败：从背包/仓库移除物品失败！"));
        // 移除失败，恢复图标显示
        if (Img_ItemIcon)
        {
            Img_ItemIcon->SetVisibility(ESlateVisibility::Visible);
        }
        return;
    }

        // 4. 生成拾取物
        UWorld* World = GetWorld();
        if (!World) return;
    
        // 判断是装备还是普通物品
        UESEquipItemBase* EquipItem = Cast<UESEquipItemBase>(CurrentItem);
        if (IsValid(EquipItem))
        {
            // 生成装备拾取物
            FActorSpawnParameters SpawnParams;
            SpawnParams.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButDontSpawnIfColliding;
            SpawnParams.Owner = Pawn;
    
            // ✅ 【核心修改】优先使用配置的蓝图类，没有则用C++基类兜底
            TSubclassOf<AESPickupEquip> ClassToSpawn = AESPickupEquip::StaticClass();
            
            // 先尝试从InventoryManager获取配置
            if (InventoryManager && InventoryManager->PickupEquipClass)
            {
                ClassToSpawn = InventoryManager->PickupEquipClass;
            }
            
            AESPickupEquip* PickupEquip = World->SpawnActor<AESPickupEquip>(ClassToSpawn, SpawnLocation, SpawnRotation, SpawnParams);
            if (IsValid(PickupEquip))
            {
                PickupEquip->InitializeWithEquip(EquipItem);
                UE_LOG(LogTemp, Log, TEXT("【丢弃物品】成功！生成拾取物类：%s，装备：%s"), *GetNameSafe(ClassToSpawn), *EquipItem->Name.ToString());
            }
            else
            {
                UE_LOG(LogTemp, Error, TEXT("【丢弃物品】失败：生成拾取物Actor失败！"));
            }
        }

    // 5. 刷新UI
    if (SlotContext == EESSlotContext::Inventory)
    {
        InventoryManager->TriggerInventoryRefresh();
    }
    else if (SlotContext == EESSlotContext::Warehouse)
    {
        InventoryManager->OnWarehouseUpdated.Broadcast();
    }
    
    InventoryManager->SaveGame();

    // 6. 清空当前Slot
    CurrentItem = nullptr;
    ClearSlot();
}

void UESItemSlotWidget::NativeOnInitialized()
{
    Super::NativeOnInitialized();
    ClearSlot();
}

FReply UESItemSlotWidget::NativeOnMouseButtonDown(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent)
{
    FReply Reply = Super::NativeOnMouseButtonDown(InGeometry, InMouseEvent);

    // 左键点击 + 有物品 → 启动拖拽
    if (InMouseEvent.GetEffectingButton() == EKeys::LeftMouseButton && IsValid(CurrentItem))
    {
        // ✅ 终极修复：TSharedPtr -> TSharedRef 转换
        TSharedPtr<SWidget> CachedSlateWidget = GetCachedWidget();
        if (CachedSlateWidget.IsValid())
        {
            return FReply::Handled().DetectDrag(CachedSlateWidget.ToSharedRef(), InMouseEvent.GetEffectingButton());
        }
    }

    // 右键点击
    if (InMouseEvent.GetEffectingButton() == EKeys::RightMouseButton)
    {
        OnRightClicked.Broadcast(this);
        return FReply::Handled();
    }

    return Reply;
}

void UESItemSlotWidget::NativeOnDragDetected(const FGeometry& InGeometry, const FPointerEvent& InMouseEvent, UDragDropOperation*& OutOperation)
{
    Super::NativeOnDragDetected(InGeometry, InMouseEvent, OutOperation);

    if (!IsValid(CurrentItem)) return;

    // 1. 创建我们自定义的拖拽操作对象
    UESItemDragDropOp* DragOp = NewObject<UESItemDragDropOp>(this);
    
    // 2. 填充拖拽数据
    DragOp->DraggedItem = CurrentItem;
    DragOp->SourceSlot = this;
    DragOp->SourceContext = SlotContext;
    DragOp->SourceEquipType = BindEquipType;
    DragOp->SourceSlotIndex = this->SlotIndex;

    // 3. ✅ 【修复】创建拖拽预览图标（简化版，不报错）
    if (Img_ItemIcon)
    {
        UImage* DragVisual = NewObject<UImage>(this);
        DragVisual->SetBrushFromSoftTexture(CurrentItem->Icon);
        // 删掉了报错的 SetDesiredSizeInVector
        DragVisual->SetColorAndOpacity(FLinearColor(1.f, 1.f, 1.f, 0.9f));
        
        DragOp->DefaultDragVisual = DragVisual;
        DragOp->Pivot = EDragPivot::CenterCenter;
    }

    // 4. 隐藏当前槽位
    if (Img_ItemIcon)
    {
        Img_ItemIcon->SetVisibility(ESlateVisibility::Hidden);
    }
    OnDragStarted();

    OutOperation = DragOp;
}

bool UESItemSlotWidget::NativeOnDrop(const FGeometry& InGeometry, const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    Super::NativeOnDrop(InGeometry, InDragDropEvent, InOperation);

    // 1. 转换为我们的自定义拖拽类型
    UESItemDragDropOp* DragOp = Cast<UESItemDragDropOp>(InOperation);
    if (!DragOp || !IsValid(DragOp->DraggedItem))
    {
        return false;
    }

    // 2. 获取InventoryManager
    UESInventoryManagerComponent* InventoryManager = GetInventoryManager();
    if (!InventoryManager)
    {
        return false;
    }

    // 3. 【核心】根据来源和目标上下文，执行不同逻辑
    bool bSuccess = false;

    // ==========================================
    // Case 1: 从背包 拖到 背包 (支持空格子和交换)
    // ==========================================
    if (DragOp->SourceContext == EESSlotContext::Inventory && SlotContext == EESSlotContext::Inventory)
    {
        int32 TargetIdx = this->SlotIndex;
        int32 SourceIdx = DragOp->SourceSlotIndex;

        if (SourceIdx != TargetIdx && SourceIdx != -1 && TargetIdx != -1)
        {
            UESItemBase* SourceItem = DragOp->DraggedItem;
            UESItemBase* TargetItem = InventoryManager->GetInventoryItemAt(TargetIdx);

            // 先清空来源槽位
            InventoryManager->ClearInventorySlot(SourceIdx);

            // 情况A：目标槽位是空的 -> 直接放
            if (!IsValid(TargetItem))
            {
                InventoryManager->SetItemToInventorySlot(SourceItem, TargetIdx);
                bSuccess = true;
            }
            // 情况B：目标槽位有物品 -> 互相交换
            else
            {
                InventoryManager->ClearInventorySlot(TargetIdx);
                InventoryManager->SetItemToInventorySlot(TargetItem, SourceIdx);
                InventoryManager->SetItemToInventorySlot(SourceItem, TargetIdx);
                bSuccess = true;
            }

            if (bSuccess)
            {
                InventoryManager->TriggerInventoryRefresh();
            }
        }
    }
    // ==========================================
    // Case 2: 从背包 拖到 装备栏 (穿戴)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Inventory && SlotContext == EESSlotContext::Equipment)
    {
        UESEquipItemBase* EquipToWear = Cast<UESEquipItemBase>(DragOp->DraggedItem);
        if (IsValid(EquipToWear) && EquipToWear->Type == BindEquipType)
        {
            // 类型匹配，执行穿戴（我们已经改了EquipItem，会留空原槽位）
            bSuccess = InventoryManager->EquipItem(EquipToWear);
        }
    }
    // ==========================================
    // Case 3: 从装备栏 拖到 背包 (直接放到松手的槽位)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Equipment && SlotContext == EESSlotContext::Inventory)
    {
        int32 TargetIdx = this->SlotIndex;
        if (TargetIdx != -1)
        {
            UESItemBase* TargetItem = InventoryManager->GetInventoryItemAt(TargetIdx);
            if (!IsValid(TargetItem))
            {
                UESEquipItemBase** EquippedItem = InventoryManager->Equipment.Find(DragOp->SourceEquipType);
                if (EquippedItem && IsValid(*EquippedItem))
                {
                    UESEquipItemBase* ItemToUnequip = *EquippedItem;
                    
                    // 1. 从装备栏移除
                    InventoryManager->Equipment.Remove(DragOp->SourceEquipType);
                    
                    // 2. 直接放到拖拽目标的槽位
                    InventoryManager->SetItemToInventorySlot(ItemToUnequip, TargetIdx);
                    
                    // 3. 处理GAS效果移除
                    UESAbilitySystemComponent* ASC = Cast<UESAbilitySystemComponent>(InventoryManager->GetPlayerASC());
                    if (ASC)
                    {
                        if (ItemToUnequip->EquipEffectHandle.IsValid())
                        {
                            ASC->RemoveActiveGameplayEffect(ItemToUnequip->EquipEffectHandle);
                            ItemToUnequip->EquipEffectHandle.Invalidate();
                        }
                        for (const FGameplayAbilitySpecHandle& Handle : ItemToUnequip->GrantAbilityHandles)
                        {
                            if (Handle.IsValid())
                            {
                                ASC->ClearAbility(Handle);
                            }
                        }
                        ItemToUnequip->GrantAbilityHandles.Empty();
                    }
                    
                    // 4. 刷新UI
                    InventoryManager->TriggerInventoryRefresh();
                    InventoryManager->TriggerEquipmentRefresh();
                    InventoryManager->SaveGame();
                    bSuccess = true;
                }
            }
        }
    }
    // ==========================================
    // Case 4: 从背包 拖到 仓库 (✅ 修复：直接放到松手的位置，不自动排序)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Inventory && SlotContext == EESSlotContext::Warehouse)
    {
        int32 TargetIdx = this->SlotIndex;
        int32 SourceIdx = DragOp->SourceSlotIndex;

        if (SourceIdx != -1 && TargetIdx != -1)
        {
            UESItemBase* SourceItem = DragOp->DraggedItem;
            UESItemBase* TargetItem = InventoryManager->GetWarehouseItemAt(TargetIdx);

            // 先清空背包来源槽位
            InventoryManager->ClearInventorySlot(SourceIdx);

            // 情况A：仓库目标槽位是空的 -> 直接放
            if (!IsValid(TargetItem))
            {
                InventoryManager->SetItemToWarehouseSlot(SourceItem, TargetIdx);
                bSuccess = true;
            }
            // 情况B：仓库目标槽位有物品 -> 互相交换
            else
            {
                InventoryManager->ClearWarehouseSlot(TargetIdx);
                InventoryManager->SetItemToInventorySlot(TargetItem, SourceIdx);
                InventoryManager->SetItemToWarehouseSlot(SourceItem, TargetIdx);
                bSuccess = true;
            }

            if (bSuccess)
            {
                InventoryManager->TriggerInventoryRefresh();
                InventoryManager->OnWarehouseUpdated.Broadcast();
                InventoryManager->SaveGame();
            }
        }
    }
    // ==========================================
    // Case 5: 从仓库 拖到 背包 (✅ 修复：直接放到松手的位置)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Warehouse && SlotContext == EESSlotContext::Inventory)
    {
        int32 TargetIdx = this->SlotIndex;
        int32 SourceIdx = DragOp->SourceSlotIndex;

        if (SourceIdx != -1 && TargetIdx != -1)
        {
            UESItemBase* SourceItem = DragOp->DraggedItem;
            UESItemBase* TargetItem = InventoryManager->GetInventoryItemAt(TargetIdx);

            // 先清空仓库来源槽位
            InventoryManager->ClearWarehouseSlot(SourceIdx);

            // 情况A：背包目标槽位是空的 -> 直接放
            if (!IsValid(TargetItem))
            {
                InventoryManager->SetItemToInventorySlot(SourceItem, TargetIdx);
                bSuccess = true;
            }
            // 情况B：背包目标槽位有物品 -> 互相交换
            else
            {
                InventoryManager->ClearInventorySlot(TargetIdx);
                InventoryManager->SetItemToWarehouseSlot(TargetItem, SourceIdx);
                InventoryManager->SetItemToInventorySlot(SourceItem, TargetIdx);
                bSuccess = true;
            }

            if (bSuccess)
            {
                InventoryManager->TriggerInventoryRefresh();
                InventoryManager->OnWarehouseUpdated.Broadcast();
            }
        }
    }
    // ==========================================
    // Case 6: 从仓库 拖到 仓库 (✅ 修复：仓库内部拖拽，支持空格子和交换)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Warehouse && SlotContext == EESSlotContext::Warehouse)
    {
        int32 TargetIdx = this->SlotIndex;
        int32 SourceIdx = DragOp->SourceSlotIndex;

        if (SourceIdx != TargetIdx && SourceIdx != -1 && TargetIdx != -1)
        {
            UESItemBase* SourceItem = DragOp->DraggedItem;
            UESItemBase* TargetItem = InventoryManager->GetWarehouseItemAt(TargetIdx);

            // 先清空来源槽位
            InventoryManager->ClearWarehouseSlot(SourceIdx);

            // 情况A：目标槽位是空的 -> 直接放
            if (!IsValid(TargetItem))
            {
                InventoryManager->SetItemToWarehouseSlot(SourceItem, TargetIdx);
                bSuccess = true;
            }
            // 情况B：目标槽位有物品 -> 互相交换
            else
            {
                InventoryManager->ClearWarehouseSlot(TargetIdx);
                InventoryManager->SetItemToWarehouseSlot(TargetItem, SourceIdx);
                InventoryManager->SetItemToWarehouseSlot(SourceItem, TargetIdx);
                bSuccess = true;
            }

            if (bSuccess)
            {
                InventoryManager->OnWarehouseUpdated.Broadcast();
                InventoryManager->SaveGame();
            }
        }
    }
        // ==========================================
    // Case 7: 从仓库 拖到 装备栏 (✅ 彻底修复复制bug，正确替换)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Warehouse && SlotContext == EESSlotContext::Equipment)
    {
        UESEquipItemBase* EquipToWear = Cast<UESEquipItemBase>(DragOp->DraggedItem);
        // 校验：装备类型匹配、来源索引有效
        if (IsValid(EquipToWear) && EquipToWear->Type == BindEquipType && DragOp->SourceSlotIndex != -1)
        {
            int32 WarehouseSourceIdx = DragOp->SourceSlotIndex;
            UESAbilitySystemComponent* ASC = Cast<UESAbilitySystemComponent>(InventoryManager->GetPlayerASC());
            if (!ASC) return false;

            // 1. 【关键】先从仓库彻底移除要穿戴的新装备，避免复制
            InventoryManager->ClearWarehouseSlot(WarehouseSourceIdx);

            // 2. 处理装备栏已有旧装备的情况
            UESEquipItemBase** ExistingEquip = InventoryManager->Equipment.Find(EquipToWear->Type);
            if (ExistingEquip && IsValid(*ExistingEquip))
            {
                UESEquipItemBase* OldEquip = *ExistingEquip;
                
                // 从装备栏移除旧装备
                InventoryManager->Equipment.Remove(EquipToWear->Type);
                
                // 把旧装备放回仓库的来源槽位
                InventoryManager->SetItemToWarehouseSlot(OldEquip, WarehouseSourceIdx);
                
                // 移除旧装备的GAS效果
                if (OldEquip->EquipEffectHandle.IsValid())
                {
                    ASC->RemoveActiveGameplayEffect(OldEquip->EquipEffectHandle);
                    OldEquip->EquipEffectHandle.Invalidate();
                }
                for (const FGameplayAbilitySpecHandle& Handle : OldEquip->GrantAbilityHandles)
                {
                    if (Handle.IsValid())
                    {
                        ASC->ClearAbility(Handle);
                    }
                }
                OldEquip->GrantAbilityHandles.Empty();
            }

            // 3. 把新装备加入装备栏
            InventoryManager->Equipment.Add(EquipToWear->Type, EquipToWear);
            
            // 4. 应用新装备的GAS效果
            InventoryManager->ApplyEquipStats(ASC, EquipToWear);

            // 5. 刷新UI和存档
            InventoryManager->OnWarehouseUpdated.Broadcast();
            InventoryManager->TriggerEquipmentRefresh();
            InventoryManager->SaveGame();
            
            bSuccess = true;
        }
    }
    // ==========================================
    // Case 8: 从装备栏 拖到 仓库 (✅ 新增：装备直接存入仓库)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Equipment && SlotContext == EESSlotContext::Warehouse)
    {
        int32 TargetIdx = this->SlotIndex;
        if (TargetIdx != -1)
        {
            UESItemBase* TargetItem = InventoryManager->GetWarehouseItemAt(TargetIdx);
            if (!IsValid(TargetItem))
            {
                UESEquipItemBase** EquippedItem = InventoryManager->Equipment.Find(DragOp->SourceEquipType);
                if (EquippedItem && IsValid(*EquippedItem))
                {
                    UESEquipItemBase* ItemToUnequip = *EquippedItem;
                    
                    // 1. 从装备栏移除
                    InventoryManager->Equipment.Remove(DragOp->SourceEquipType);
                    
                    // 2. 直接放到仓库目标槽位
                    InventoryManager->SetItemToWarehouseSlot(ItemToUnequip, TargetIdx);
                    
                    // 3. 处理GAS效果移除
                    UESAbilitySystemComponent* ASC = Cast<UESAbilitySystemComponent>(InventoryManager->GetPlayerASC());
                    if (ASC)
                    {
                        if (ItemToUnequip->EquipEffectHandle.IsValid())
                        {
                            ASC->RemoveActiveGameplayEffect(ItemToUnequip->EquipEffectHandle);
                            ItemToUnequip->EquipEffectHandle.Invalidate();
                        }
                        for (const FGameplayAbilitySpecHandle& Handle : ItemToUnequip->GrantAbilityHandles)
                        {
                            if (Handle.IsValid())
                            {
                                ASC->ClearAbility(Handle);
                            }
                        }
                        ItemToUnequip->GrantAbilityHandles.Empty();
                    }
                    
                    // 4. 刷新UI
                    InventoryManager->OnWarehouseUpdated.Broadcast();
                    InventoryManager->TriggerEquipmentRefresh();
                    InventoryManager->SaveGame();
                    bSuccess = true;
                }
            }
        }
    }
        // ==========================================
    // Case 9: 从商店 拖到 背包 (✅ 新增：拖拽购买)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Shop && SlotContext == EESSlotContext::Inventory)
    {
        int32 TargetIdx = this->SlotIndex;
        if (TargetIdx != -1)
        {
            UESItemBase* TargetItem = InventoryManager->GetInventoryItemAt(TargetIdx);
            if (!IsValid(TargetItem))
            {
                // 目标槽位是空的，执行购买
                UESItemBase* ShopItem = DragOp->DraggedItem;
                if (InventoryManager->BuyItem(ShopItem))
                {
                    // 购买成功，把物品放到指定槽位（BuyItem已经处理了数据转移，这里只需要确保位置正确）
                    // 注意：BuyItem内部是Add到末尾，我们这里微调一下，放到拖拽的目标槽位
                    int32 LastIdx = InventoryManager->FindFirstEmptyInventorySlot() - 1;
                    if (LastIdx != INDEX_NONE && LastIdx != TargetIdx)
                    {
                        UESItemBase* BoughtItem = InventoryManager->GetInventoryItemAt(LastIdx);
                        if (IsValid(BoughtItem))
                        {
                            InventoryManager->ClearInventorySlot(LastIdx);
                            InventoryManager->SetItemToInventorySlot(BoughtItem, TargetIdx);
                            InventoryManager->TriggerInventoryRefresh();
                        }
                    }
                    bSuccess = true;
                }
            }
        }
    }
    // ==========================================
    // Case 10: 从背包 拖到 商店 (✅ 新增：拖拽出售)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Inventory && SlotContext == EESSlotContext::Shop)
    {
        UESItemBase* ItemToSell = DragOp->DraggedItem;
        if (IsValid(ItemToSell))
        {
            // 直接调用出售函数
            bSuccess = InventoryManager->SellItem(ItemToSell);
        }
    }
    // ==========================================
    // Case 11: 从商店 拖到 商店 (✅ 新增：禁止商店内部拖拽)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Shop && SlotContext == EESSlotContext::Shop)
    {
        // 商店内部不允许拖拽交换，直接返回false，什么都不做
        bSuccess = false;
    }
    // ==========================================
    // Case 12: 从商店 拖到 仓库 (✅ 新增：禁止直接拖到仓库)
    // ==========================================
    else if (DragOp->SourceContext == EESSlotContext::Shop && SlotContext == EESSlotContext::Warehouse)
    {
        // 商店不能直接拖到仓库，必须先买再存
        bSuccess = false;
    }

    // 4. 广播拖拽结束事件
    OnDragEnded(bSuccess);

    // ✅ 【新增】如果放置成功，确保来源槽位的CurrentItem已经处理好了
    if (bSuccess)
    {
        // 什么都不用做，因为我们在放置逻辑里已经处理了数据
        // 这里只是为了确保逻辑清晰
    }

    
    return bSuccess;
}

void UESItemSlotWidget::NativeOnDragCancelled(const FDragDropEvent& InDragDropEvent, UDragDropOperation* InOperation)
{
    Super::NativeOnDragCancelled(InDragDropEvent, InOperation);

    UE_LOG(LogTemp, Log, TEXT("【拖拽】NativeOnDragCancelled 触发，执行丢弃物品"));

    // ✅ 【核心逻辑】拖到UI外，直接丢弃物品！
    DropItemToWorld();
}

UESInventoryManagerComponent* UESItemSlotWidget::GetInventoryManager() const
{
    APlayerState* PState = GetOwningPlayerState();
    if (PState)
    {
        return PState->FindComponentByClass<UESInventoryManagerComponent>();
    }
    return nullptr;
}

void UESItemSlotWidget::RefreshSlot(UESItemBase* NewItem)
{
    UE_LOG(LogTemp, Log, TEXT("[Slot Refresh] 被调用！传入物品：%s"), *GetNameSafe(NewItem)); // 【Debug 日志】

    if (!IsValid(NewItem))
    {
        ClearSlot();
        return;
    }

    CurrentItem = NewItem;

    // 刷新图标
    if (Img_ItemIcon)
    {
        Img_ItemIcon->SetBrushFromSoftTexture(NewItem->Icon);
        Img_ItemIcon->SetVisibility(ESlateVisibility::Visible);
    }

    // 刷新品质边框颜色
    if (Border_Quality)
    {
        Border_Quality->SetBrushColor(NewItem->GetQualityColor());
        Border_Quality->SetVisibility(ESlateVisibility::Visible);
    }

    // 刷新名称
    if (Text_ItemName)
    {
        Text_ItemName->SetText(NewItem->Name);
        Text_ItemName->SetVisibility(ESlateVisibility::Visible);
    }
}

void UESItemSlotWidget::ClearSlot()
{
    CurrentItem = nullptr;

    if (Img_ItemIcon)
    {
        Img_ItemIcon->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (Border_Quality)
    {
        Border_Quality->SetVisibility(ESlateVisibility::Collapsed);
    }

    if (Text_ItemName)
    {
        Text_ItemName->SetVisibility(ESlateVisibility::Collapsed);
    }
}

void UESItemSlotWidget::OnDragStarted_Implementation()
{
}
