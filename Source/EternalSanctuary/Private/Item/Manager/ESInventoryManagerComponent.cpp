#include "Item/Manager/ESInventoryManagerComponent.h"
#include "AbilitySystemComponent.h"
#include "GAS/AS/ESAttributeSet.h"
#include "GAS/ASC/ESAbilitySystemComponent.h" // 引入你的自定义ASC
#include "GameFramework/PlayerState.h"
#include "Item/Data/ESInventoryTypes.h"
#include "SaveGame/ESGameSave.h"
#include "Kismet/GameplayStatics.h"

UESInventoryManagerComponent::UESInventoryManagerComponent()
{
	PrimaryComponentTick.bCanEverTick = false;
}

void UESInventoryManagerComponent::BeginPlay()
{
	Super::BeginPlay();
	LoadGame();
}

// ==================== 核心生成函数 ====================
UESItemBase* UESInventoryManagerComponent::CreateItem(TSubclassOf<UESItemBase> ItemClass)
{
	if (!IsValid(ItemClass)) return nullptr;

	// 创建物品实例，Outer设为管理器，避免GC回收
	UESItemBase* NewItem = NewObject<UESItemBase>(this, ItemClass);

	// 自动生成全局唯一GUID
	NewItem->ItemGUID = FGuid::NewGuid();

	return NewItem;
}

// ==================== 装备&卸下核心逻辑（GAS联动，完全适配你的ASC） ====================
bool UESInventoryManagerComponent::EquipItem(UESEquipItemBase* EquipItem, int32 SourceSlotIndex)
{
	// 基础校验
	if (!IsValid(EquipItem)) return false;
	if (!Inventory.Contains(EquipItem)) return false;

	// 获取ASC
	UESAbilitySystemComponent* ASC = Cast<UESAbilitySystemComponent>(GetPlayerASC());
	if (!ASC) return false;

	// 确定新装备在背包里的索引
	int32 ItemIndex = SourceSlotIndex;
	if (ItemIndex == -1)
	{
		ItemIndex = Inventory.Find(EquipItem);
	}
	if (ItemIndex == INDEX_NONE) return false;

	// ==========================================
	// ✅ 【关键修复】调整执行顺序
	// ==========================================
	
	// 1. 【第一步】先把新装备从背包拿出来，清空槽位
	// 这样旧装备才有地方放！
	ClearInventorySlot(ItemIndex);

	// 2. 【第二步】处理已有旧装备：现在槽位空了，可以放心卸下来放进去
	UESEquipItemBase** ExistingEquip = Equipment.Find(EquipItem->Type);
	if (ExistingEquip && IsValid(*ExistingEquip))
	{
		// 关键：指定旧装备放回刚才清空的来源槽位
		UnequipItem(EquipItem->Type, ItemIndex);
	}

	// 3. 【第三步】把新装备加入装备栏
	Equipment.Add(EquipItem->Type, EquipItem);
	
	// 4. 应用新装备的GAS效果
	ApplyEquipStats(ASC, EquipItem);

	// 5. 刷新UI和存档
	NotifyInventoryChanged();
	NotifyEquipmentChanged();
	SaveGame();
	return true;
}

bool UESInventoryManagerComponent::UnequipItem(EESItemType EquipType, int32 TargetSlotIndex)
{
	// 基础校验
	UESEquipItemBase** EquippedItem = Equipment.Find(EquipType);
	if (!EquippedItem || !IsValid(*EquippedItem)) return false;

	// 确定目标槽位
	int32 TargetIdx = TargetSlotIndex;
	// 如果没有指定索引/索引无效，自动找第一个空位
	if (TargetIdx == -1 || TargetIdx < 0 || TargetIdx >= MaxInventorySize)
	{
		TargetIdx = FindFirstEmptyInventorySlot();
	}
	// 检查目标槽位是否有效且为空
	if (TargetIdx == -1 || IsValid(GetInventoryItemAt(TargetIdx)))
	{
		return false;
	}

	// 获取ASC
	UESAbilitySystemComponent* ASC = Cast<UESAbilitySystemComponent>(GetPlayerASC());
	if (!ASC) return false;

	UESEquipItemBase* ItemToUnequip = *EquippedItem;

	// 1. 从装备栏移除
	Equipment.Remove(EquipType);
	
	// 2. 放到指定的槽位
	SetItemToInventorySlot(ItemToUnequip, TargetIdx);
	
	// 3. 移除GAS效果
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

	// 4. 刷新UI和存档
	NotifyInventoryChanged();
	NotifyEquipmentChanged();
	SaveGame();
	return true;
}

// ==================== 内部辅助：应用装备属性 ====================
void UESInventoryManagerComponent::ApplyEquipStats(UESAbilitySystemComponent* ASC, UESEquipItemBase* EquipItem)
{
	if (!ASC || !EquipItem) return;

	// 1. 先移除旧的GE (如果有的话)
	if (EquipItem->EquipEffectHandle.IsValid())
	{
		ASC->RemoveActiveGameplayEffect(EquipItem->EquipEffectHandle);
		EquipItem->EquipEffectHandle.Invalidate();
	}

	if (!EquipGEClass)
	{
		UE_LOG(LogTemp, Error, TEXT("【装备GE错误】找不到通用装备GE，请检查EquipGEClass配置！"));
		return;
	}

	// 2. 构建GE Spec
	FGameplayEffectSpecHandle SpecHandle = ASC->MakeOutgoingSpec(EquipGEClass, 1, ASC->MakeEffectContext());
	FGameplayEffectSpec* Spec = SpecHandle.Data.Get();
	if (!Spec)
	{
		UE_LOG(LogTemp, Error, TEXT("【装备GE错误】创建GE Spec失败！"));
		return;
	}

	// --------------------------
	// 【安全防护1】强制初始化所有SetByCaller的安全默认值，彻底杜绝归零
	// --------------------------
	// 固定值属性：默认0.0，加0无影响
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.AttackPower.Flat")), 0.f);
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.MaxHealth.Flat")), 0.f);
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.Defense.Flat")), 0.f);
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.MoveSpeed.Flat")), 0.f);
	
	// 【核心修改】百分比属性：默认1.0，乘1无影响，绝对不会归零！
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.AttackPower.Percent")), 1.f);
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.MaxHealth.Percent")), 1.f);
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.Defense.Percent")), 1.f);
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.MoveSpeed.Percent")), 1.f);
	
	// 百分比专属属性：默认0.0，加0无影响
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.CritRate")), 0.f);
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.CritDamage")), 0.f);
	Spec->SetSetByCallerMagnitude(FGameplayTag::RequestGameplayTag(FName("Stat.CooldownReduction")), 0.f);

	// 3. 收集所有需要应用的词条 (主词条 + 已解锁的副词条)
	TArray<FESRuntimeEntry*> AllActiveEntries;
	AllActiveEntries.Add(&EquipItem->MainEntry);
	for (FESRuntimeEntry& SubEntry : EquipItem->SubEntries)
	{
		if (SubEntry.bIsUnlocked)
		{
			AllActiveEntries.Add(&SubEntry);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("【装备GE】开始应用装备[%s]的属性，共%d个激活词条"), *EquipItem->Name.ToString(), AllActiveEntries.Num());

	// 4. 遍历词条，按规则注入数值
	for (FESRuntimeEntry* Entry : AllActiveEntries)
	{
		if (Entry->EntryType != EESEntryType::Attribute) continue;

		// 【安全防护2】跳过0值无效词条，避免错误注入
		if (FMath::IsNearlyZero(Entry->FinalValue))
		{
			UE_LOG(LogTemp, Warning, TEXT("【装备GE】跳过无效词条[%s]，数值为0"), *Entry->SourceEntryID.ToString());
			continue;
		}

		// --------------------------
		// 规则1：处理基础属性（双模式：固定值/百分比）
		// --------------------------
		bool bIsBaseAttribute = true;
		FGameplayTag FlatTag;
		FGameplayTag PercentTag;

		switch (Entry->TargetStat)
		{
		case EESStatType::AttackPower:
			FlatTag = FGameplayTag::RequestGameplayTag(FName("Stat.AttackPower.Flat"));
			PercentTag = FGameplayTag::RequestGameplayTag(FName("Stat.AttackPower.Percent"));
			break;
		case EESStatType::MaxHealth:
			FlatTag = FGameplayTag::RequestGameplayTag(FName("Stat.MaxHealth.Flat"));
			PercentTag = FGameplayTag::RequestGameplayTag(FName("Stat.MaxHealth.Percent"));
			break;
		case EESStatType::Defense:
			FlatTag = FGameplayTag::RequestGameplayTag(FName("Stat.Defense.Flat"));
			PercentTag = FGameplayTag::RequestGameplayTag(FName("Stat.Defense.Percent"));
			break;
		case EESStatType::MoveSpeed:
			FlatTag = FGameplayTag::RequestGameplayTag(FName("Stat.MoveSpeed.Flat"));
			PercentTag = FGameplayTag::RequestGameplayTag(FName("Stat.MoveSpeed.Percent"));
			break;
		default:
			bIsBaseAttribute = false;
			break;
		}

		// 基础属性：按bIsPercentage判断注入哪个标签
		if (bIsBaseAttribute)
		{
			if (Entry->bIsPercentage)
			{
				// 【核心修改】百分比模式：数值转换为 1 + FinalValue
				// 比如词条59%防御，FinalValue=0.59，传1.59给Multiply (Additive)
				if (PercentTag.IsValid())
				{
					float FinalMagnitude = 1.f + Entry->FinalValue;
					Spec->SetSetByCallerMagnitude(PercentTag, FinalMagnitude);
					UE_LOG(LogTemp, Log, TEXT("  -> 注入百分比词条：%s，属性：%s，数值：%f (%.1f%%)"), 
						*Entry->SourceEntryID.ToString(), *PercentTag.ToString(), FinalMagnitude, Entry->FinalValue * 100.f);
				}
			}
			else
			{
				// 固定值模式：直接传FinalValue（已四舍五入为整数）
				if (FlatTag.IsValid())
				{
					Spec->SetSetByCallerMagnitude(FlatTag, Entry->FinalValue);
					UE_LOG(LogTemp, Log, TEXT("  -> 注入固定值词条：%s，属性：%s，数值：%.0f"), 
						*Entry->SourceEntryID.ToString(), *FlatTag.ToString(), Entry->FinalValue);
				}
			}
			continue;
		}

		// --------------------------
		// 规则2：处理百分比专属属性（单模式，永远直接加算）
		// --------------------------
		FGameplayTag PercentOnlyTag;
		switch (Entry->TargetStat)
		{
		case EESStatType::CritRate:
			PercentOnlyTag = FGameplayTag::RequestGameplayTag(FName("Stat.CritRate"));
			break;
		case EESStatType::CritDamage:
			PercentOnlyTag = FGameplayTag::RequestGameplayTag(FName("Stat.CritDamage"));
			break;
		case EESStatType::CooldownReduction:
			PercentOnlyTag = FGameplayTag::RequestGameplayTag(FName("Stat.CooldownReduction"));
			break;
		default:
			UE_LOG(LogTemp, Warning, TEXT("【装备GE】未识别的属性类型：%d，词条：%s"), (int32)Entry->TargetStat, *Entry->SourceEntryID.ToString());
			continue;
		}

		// 百分比专属属性：直接注入FinalValue（0.27=27%，直接加算）
		if (PercentOnlyTag.IsValid())
		{
			Spec->SetSetByCallerMagnitude(PercentOnlyTag, Entry->FinalValue);
			UE_LOG(LogTemp, Log, TEXT("  -> 注入百分比专属词条：%s，属性：%s，数值：%f (%.1f%%)"), 
				*Entry->SourceEntryID.ToString(), *PercentOnlyTag.ToString(), Entry->FinalValue, Entry->FinalValue * 100.f);
		}
	}

	// 5. 应用GE并保存句柄
	EquipItem->EquipEffectHandle = ASC->ApplyGameplayEffectSpecToSelf(*Spec);

	if (EquipItem->EquipEffectHandle.WasSuccessfullyApplied())
	{
		UE_LOG(LogTemp, Log, TEXT("【装备GE】装备[%s]的GE应用成功！"), *EquipItem->Name.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("【装备GE】装备[%s]的GE应用失败！"), *EquipItem->Name.ToString());
	}
}

// ==================== 商店逻辑 ====================
bool UESInventoryManagerComponent::BuyItem(UESItemBase* ShopItem)
{
	if (!IsValid(ShopItem)) return false;
	if (!ShopItems.Contains(ShopItem)) return false;
	if (!HasInventorySpace()) return false;
	if (CurrentGold < ShopItem->BuyPrice) return false;

	// 1. 扣除金币
	ModifyGold(-ShopItem->BuyPrice);

	// 2. 物品转移：商店→背包
	ShopItems.Remove(ShopItem);
	Inventory.Add(ShopItem);

	// 3. 移除重复广播，只保留必要的
	NotifyInventoryChanged();
	OnShopItemsUpdated.Broadcast();
	SaveGame();

	UE_LOG(LogTemp, Log, TEXT("【购买成功】物品[%s]已加入背包，当前背包物品数量：%d"), *ShopItem->Name.ToString(), Inventory.Num());
	return true;
}

bool UESInventoryManagerComponent::SellItem(UESItemBase* Item)
{
	if (!IsValid(Item)) return false;
	if (!Inventory.Contains(Item)) return false;
	if (Item->SellPrice <= 0) return false;

	// 1. 增加金币
	ModifyGold(Item->SellPrice);

	// 2. ✅ 核心修改：找到物品索引，清空槽位（不删除数组元素）
	int32 ItemIndex = Inventory.Find(Item);
	if (ItemIndex != INDEX_NONE)
	{
		ClearInventorySlot(ItemIndex);
	}
	// 加回商店
	ShopItems.Add(Item);

	// 3. 只保留必要的广播
	NotifyInventoryChanged();
	OnShopItemsUpdated.Broadcast();
	SaveGame();

	UE_LOG(LogTemp, Log, TEXT("【出售成功】物品[%s]已从背包移至商店，当前背包物品数量：%d，商店物品数量：%d"), 
		*Item->Name.ToString(), Inventory.Num(), ShopItems.Num());
	return true;
}

// ==================== 商店刷新：生成随机商品 ====================
void UESInventoryManagerComponent::RefreshShopItems(UDataTable* EntryLibrary, UDataTable* EquipTemplateTable,
                                                    int32 ItemCount)
{
	// 1. 校验参数
	if (!EntryLibrary || !EquipTemplateTable)
	{
		UE_LOG(LogTemp, Error, TEXT("RefreshShopItems 失败：缺少 DataTable！"));
		return;
	}

	// 2. 清空旧商品
	ShopItems.Empty();

	// 3. 获取所有可用的装备模板
	TArray<FName> AllTemplateNames = EquipTemplateTable->GetRowNames();
	if (AllTemplateNames.Num() == 0)
	{
		UE_LOG(LogTemp, Error, TEXT("RefreshShopItems 失败：装备模板表为空！"));
		return;
	}

	// 4. 循环生成商品
	for (int32 i = 0; i < ItemCount; ++i)
	{
		// 4.1 随机选一个装备模板
		int32 RandomIndex = FMath::RandRange(0, AllTemplateNames.Num() - 1);
		FName RandomTemplateID = AllTemplateNames[RandomIndex];

		// 4.2 随机一个品质（普通70%，稀有25%，传说5%）
		EESItemQuality RandomQuality = EESItemQuality::Normal;
		int32 QualityRoll = FMath::RandRange(1, 100);
		if (QualityRoll > 95)
		{
			RandomQuality = EESItemQuality::Legendary;
		}
		else if (QualityRoll > 70)
		{
			RandomQuality = EESItemQuality::Rare;
		}

		// 4.3 调用我们已经写好的 GenerateRandomEquip 生成装备
		UESEquipItemBase* NewShopItem = GenerateRandomEquip(EntryLibrary, EquipTemplateTable, RandomTemplateID,
		                                                    RandomQuality);
		if (NewShopItem)
		{
			// 4.4 稍微调整一下价格（商店价格可以比背包里的贵一点）
			NewShopItem->BuyPrice = NewShopItem->BuyPrice * 1.2f;
			NewShopItem->SellPrice = NewShopItem->BuyPrice * 0.5f;

			// 4.5 加入商店数组
			// 注意：GenerateRandomEquip 内部会自动加到背包，我们要把它移出来
			if (Inventory.Contains(NewShopItem))
			{
				Inventory.Remove(NewShopItem);
			}
			ShopItems.Add(NewShopItem);
		}
	}

	// 5. 广播商店变化
	OnShopItemsUpdated.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("商店刷新成功！共生成 %d 件商品"), ShopItems.Num());
}

void UESInventoryManagerComponent::TriggerInventoryRefresh()
{
	NotifyInventoryChanged();
}

void UESInventoryManagerComponent::TriggerEquipmentRefresh()
{
	NotifyEquipmentChanged();
}

UESItemBase* UESInventoryManagerComponent::GetWarehouseItemAt(int32 Index)
{
	if (Warehouse.IsValidIndex(Index))
	{
		return Warehouse[Index];
	}
	// 索引无效时返回nullptr，由UI处理空Slot
	return nullptr;
}

UESItemBase* UESInventoryManagerComponent::GetInventoryItemAt(int32 Index)
{
	if (Inventory.IsValidIndex(Index))
	{
		return Inventory[Index];
	}
	return nullptr;
}

// ==================== 蓝图安全获取商店物品 ====================
UESItemBase* UESInventoryManagerComponent::GetShopItemAt(int32 Index)
{
	if (ShopItems.IsValidIndex(Index))
	{
		return ShopItems[Index];
	}
	return nullptr;
}

// ==================== 仓库逻辑 ====================
bool UESInventoryManagerComponent::DepositToWarehouse(UESItemBase* Item)
{
	if (!IsValid(Item) || !Inventory.Contains(Item)) return false;
	if (FindFirstEmptyWarehouseSlot() == -1) return false;

	// ✅ 核心修改：清空背包对应槽位，不删除元素
	int32 ItemIndex = Inventory.Find(Item);
	if (ItemIndex != INDEX_NONE)
	{
		ClearInventorySlot(ItemIndex);
	}

	// 仓库找空位放入，不直接Add
	int32 WarehouseEmptySlot = FindFirstEmptyWarehouseSlot();
	if (WarehouseEmptySlot != -1)
	{
		SetItemToWarehouseSlot(Item, WarehouseEmptySlot);
	}

	NotifyInventoryChanged();
	OnWarehouseUpdated.Broadcast();
	SaveGame();
	return true;
}

bool UESInventoryManagerComponent::RetrieveFromWarehouse(UESItemBase* Item)
{
	if (!IsValid(Item) || !Warehouse.Contains(Item)) return false;
	if (FindFirstEmptyInventorySlot() == -1) return false;

	// ✅ 核心修改：清空仓库对应槽位，不删除元素
	int32 WarehouseItemIndex = Warehouse.Find(Item);
	if (WarehouseItemIndex != INDEX_NONE)
	{
		ClearWarehouseSlot(WarehouseItemIndex);
	}

	// 背包找空位放入
	int32 EmptySlot = FindFirstEmptyInventorySlot();
	if (EmptySlot != -1)
	{
		SetItemToInventorySlot(Item, EmptySlot);
	}

	NotifyInventoryChanged();
	OnWarehouseUpdated.Broadcast();
	SaveGame();
	return true;
}

// --------------------------
// 【新增】核心辅助：找第一个空位
// --------------------------
int32 UESInventoryManagerComponent::FindFirstEmptyInventorySlot() const
{
    // 1. 先遍历现有数组，找nullptr空位
    for (int32 i = 0; i < Inventory.Num(); ++i)
    {
        if (!IsValid(Inventory[i]))
        {
            return i;
        }
    }
    // 2. 如果数组没满，返回下一个索引
    if (Inventory.Num() < MaxInventorySize)
    {
        return Inventory.Num();
    }
    // 3. 背包已满
    return -1;
}

int32 UESInventoryManagerComponent::FindFirstEmptyWarehouseSlot() const
{
    for (int32 i = 0; i < Warehouse.Num(); ++i)
    {
        if (!IsValid(Warehouse[i]))
        {
            return i;
        }
    }
    if (Warehouse.Num() < MaxWarehouseSize)
    {
        return Warehouse.Num();
    }
    return -1;
}

// --------------------------
// 【新增】核心辅助：把物品放到指定槽位
// --------------------------
bool UESInventoryManagerComponent::SetItemToInventorySlot(UESItemBase* Item, int32 SlotIndex)
{
    if (!IsValid(Item)) return false;
    if (SlotIndex < 0 || SlotIndex >= MaxInventorySize) return false;

    // 1. 如果索引超过当前数组长度，先扩容（填充nullptr）
    while (Inventory.Num() <= SlotIndex)
    {
        Inventory.Add(nullptr);
    }

    // 2. 如果目标槽位已有物品，返回失败（或者你可以改成自动交换，看需求）
    if (IsValid(Inventory[SlotIndex]))
    {
        return false;
    }

    // 3. 放入物品
    Inventory[SlotIndex] = Item;
    return true;
}

bool UESInventoryManagerComponent::SetItemToWarehouseSlot(UESItemBase* Item, int32 SlotIndex)
{
    if (!IsValid(Item)) return false;
    if (SlotIndex < 0 || SlotIndex >= MaxWarehouseSize) return false;

    while (Warehouse.Num() <= SlotIndex)
    {
        Warehouse.Add(nullptr);
    }

    if (IsValid(Warehouse[SlotIndex]))
    {
        return false;
    }

    Warehouse[SlotIndex] = Item;
    return true;
}

// --------------------------
// 【新增】核心辅助：清空指定槽位，返回被清空的物品
// --------------------------
UESItemBase* UESInventoryManagerComponent::ClearInventorySlot(int32 SlotIndex)
{
    if (SlotIndex < 0 || SlotIndex >= Inventory.Num()) return nullptr;

    UESItemBase* ItemToReturn = Inventory[SlotIndex];
    Inventory[SlotIndex] = nullptr;
    return ItemToReturn;
}

UESItemBase* UESInventoryManagerComponent::ClearWarehouseSlot(int32 SlotIndex)
{
    if (SlotIndex < 0 || SlotIndex >= Warehouse.Num()) return nullptr;

    UESItemBase* ItemToReturn = Warehouse[SlotIndex];
    Warehouse[SlotIndex] = nullptr;
    return ItemToReturn;
}

void UESInventoryManagerComponent::SortInventory()
{
	TArray<UESItemBase*> ValidItems;
	// 1. 收集所有有效物品
	for (UESItemBase* Item : Inventory)
	{
		if (IsValid(Item))
		{
			ValidItems.Add(Item);
		}
	}
	// 2. 清空背包
	Inventory.Empty();
	// 3. 重新放入（自动往前排）
	for (UESItemBase* Item : ValidItems)
	{
		int32 EmptySlot = FindFirstEmptyInventorySlot();
		if (EmptySlot != -1)
		{
			SetItemToInventorySlot(Item, EmptySlot);
		}
	}
	// 4. 广播刷新
	NotifyInventoryChanged();
	UE_LOG(LogTemp, Log, TEXT("【整理背包】完成，共整理%d件物品"), ValidItems.Num());
}

void UESInventoryManagerComponent::SortWarehouse()
{
	TArray<UESItemBase*> ValidItems;
	for (UESItemBase* Item : Warehouse)
	{
		if (IsValid(Item))
		{
			ValidItems.Add(Item);
		}
	}
	Warehouse.Empty();
	for (UESItemBase* Item : ValidItems)
	{
		int32 EmptySlot = FindFirstEmptyWarehouseSlot();
		if (EmptySlot != -1)
		{
			SetItemToWarehouseSlot(Item, EmptySlot);
		}
	}
	OnWarehouseUpdated.Broadcast();
	UE_LOG(LogTemp, Log, TEXT("【整理仓库】完成，共整理%d件物品"), ValidItems.Num());
}


// ==================== 金币&辅助函数 ====================
bool UESInventoryManagerComponent::ModifyGold(int32 Delta)
{
	if (Delta < 0 && CurrentGold + Delta < 0) return false;

	CurrentGold += Delta;
	OnGoldChanged.Broadcast(CurrentGold);
	return true;
}

UAbilitySystemComponent* UESInventoryManagerComponent::GetPlayerASC() const
{
	AActor* OwnerActor = GetOwner();
	if (!OwnerActor) return nullptr;

	// 1. 优先尝试：直接从 Owner（PlayerState）获取自定义ASC
	UESAbilitySystemComponent* CustomASC = OwnerActor->FindComponentByClass<UESAbilitySystemComponent>();
	if (CustomASC)
	{
		return CustomASC;
	}

	// 2. 兜底尝试：通过标准接口获取（兼容PlayerState架构）
	if (OwnerActor->Implements<UAbilitySystemInterface>())
	{
		UAbilitySystemComponent* ASC = Cast<IAbilitySystemInterface>(OwnerActor)->GetAbilitySystemComponent();
		if (ASC) return ASC;
	}

	// ==================== 【关键修改】适配你的单机架构 ====================
	// 3. 如果是PlayerState，尝试获取其控制的 Pawn (Character)
	APlayerState* PState = Cast<APlayerState>(OwnerActor);
	if (PState)
	{
		APawn* Pawn = PState->GetPawn();
		if (Pawn)
		{
			// 3.1 从 Pawn (Character) 身上找你的自定义 ASC
			UESAbilitySystemComponent* PawnASC = Pawn->FindComponentByClass<UESAbilitySystemComponent>();
			if (PawnASC)
			{
				UE_LOG(LogTemp, Log, TEXT("[InventoryManager] 成功从 Character 身上获取到 ASC！")); // 【Debug 日志】
				return PawnASC;
			}

			// 3.2 兜底：从 Pawn 身上通过标准接口找
			if (Pawn->Implements<UAbilitySystemInterface>())
			{
				return Cast<IAbilitySystemInterface>(Pawn)->GetAbilitySystemComponent();
			}
		}
	}

	UE_LOG(LogTemp, Error, TEXT("[InventoryManager] 获取 ASC 失败！请检查 ASC 是挂载在 Character 还是 PlayerState 上。"));
	return nullptr;
}

bool UESInventoryManagerComponent::HasInventorySpace() const
{
	return Inventory.Num() < MaxInventorySize;
}

bool UESInventoryManagerComponent::HasWarehouseSpace() const
{
	return Warehouse.Num() < MaxWarehouseSize;
}

void UESInventoryManagerComponent::NotifyInventoryChanged()
{
	UE_LOG(LogTemp, Log, TEXT("[InventoryManager] 广播 OnInventoryUpdated 事件！")); // 【Debug 日志】
	OnInventoryUpdated.Broadcast();
}

void UESInventoryManagerComponent::NotifyEquipmentChanged()
{
	UE_LOG(LogTemp, Log, TEXT("[InventoryManager] 广播 OnEquipmentUpdated 事件！")); // 【Debug 日志】
	OnEquipmentUpdated.Broadcast();
}

// 调试用：生成测试装备
void UESInventoryManagerComponent::AddTestEquipItems()
{
	NotifyInventoryChanged();
}

UESEquipItemBase* UESInventoryManagerComponent::CreateEquipFromDataTable(UDataTable* DataTable, FName RowName)
{
	if (!DataTable) return nullptr;

	// 1. 从表中读取数据行
	static const FString ContextString(TEXT("ESInventoryManager CreateEquip"));
	FESEquipDataTableRow* RowData = DataTable->FindRow<FESEquipDataTableRow>(RowName, ContextString);
	if (!RowData)
	{
		UE_LOG(LogTemp, Error, TEXT("找不到装备行：%s"), *RowName.ToString());
		return nullptr;
	}

	// 2. 创建装备实例（直接创建 UESEquipItemBase，或者你可以创建蓝图子类）
	UESEquipItemBase* NewEquip = NewObject<UESEquipItemBase>(this, UESEquipItemBase::StaticClass());

	// 3. 分配 GUID
	NewEquip->ItemGUID = FGuid::NewGuid();

	// 4. 从表中复制数据到装备实例
	NewEquip->Name = RowData->Name;
	NewEquip->Description = RowData->Description;
	NewEquip->Icon = RowData->Icon;
	NewEquip->Type = RowData->Type;
	NewEquip->Quality = RowData->Quality;
	NewEquip->BuyPrice = RowData->BuyPrice;
	NewEquip->SellPrice = RowData->SellPrice;
	NewEquip->StatList = RowData->StatList;
	NewEquip->EquipEffect = RowData->EquipEffect;
	NewEquip->GrantAbilities = RowData->GrantAbilities;

	// 5. 放入背包
	Inventory.Add(NewEquip);
	NotifyInventoryChanged();

	UE_LOG(LogTemp, Log, TEXT("成功从表生成装备：%s"), *RowName.ToString());
	return NewEquip;
}

// ==================== 装备生成核心逻辑 ====================
UESEquipItemBase* UESInventoryManagerComponent::GenerateRandomEquip(UDataTable* EntryLibrary,
                                                                    UDataTable* EquipTemplateTable, FName TemplateID,
                                                                    EESItemQuality Quality)
{
	if (!EntryLibrary || !EquipTemplateTable) return nullptr;

	static const FString TemplateContext(TEXT("GenerateEquip_Template"));
	FESEquipTemplateTableRow* Template = EquipTemplateTable->FindRow<FESEquipTemplateTableRow>(
		TemplateID, TemplateContext);
	if (!Template)
	{
		UE_LOG(LogTemp, Error, TEXT("找不到装备模板：%s"), *TemplateID.ToString());
		return nullptr;
	}

	UESEquipItemBase* NewEquip = NewObject<UESEquipItemBase>(this, UESEquipItemBase::StaticClass());
	NewEquip->ItemGUID = FGuid::NewGuid();
	NewEquip->Name = Template->Name;
	NewEquip->Description = Template->Description;
	NewEquip->Icon = Template->Icon;
	NewEquip->Type = Template->Type;
	NewEquip->Quality = Quality;
	NewEquip->BuyPrice = Template->BasePrice;
	NewEquip->SellPrice = Template->BasePrice / 2;

	// 1. 决定词条等级和数量
	int32 TargetLevel = 1;
	int32 SubEntryCount = 1;
	switch (Quality)
	{
	case EESItemQuality::Normal: TargetLevel = 1;
		SubEntryCount = 1;
		break;
	case EESItemQuality::Rare: TargetLevel = 2;
		SubEntryCount = 2;
		break;
	case EESItemQuality::Legendary: TargetLevel = 3;
		SubEntryCount = 3;
		break;
	}

	// ==================== 【核心新增】去重用的列表 ====================
	TArray<FName> UsedEntryIDs;

	// 2. 随机主词条
	FESRuntimeEntry GeneratedMainEntry;
	if (RollAndFillEntry(EntryLibrary, Template->AllowedMainEntryIDs, TargetLevel, UsedEntryIDs, GeneratedMainEntry))
	{
		GeneratedMainEntry.bIsUnlocked = true;
		NewEquip->MainEntry = GeneratedMainEntry;
		UsedEntryIDs.Add(GeneratedMainEntry.SourceEntryID); // 把主词条加入已使用列表
	}

	// 3. 随机副词条 (分阶段池子 + 去重)
	for (int32 i = 0; i < SubEntryCount; ++i)
	{
		// 根据索引，选择对应的副词条池子
		const TArray<FName>* CurrentPool = nullptr;
		switch (i)
		{
		case 0: CurrentPool = &Template->AllowedSubEntryIDs_L1;
			break; // 第1个副词条：L1池
		case 1: CurrentPool = &Template->AllowedSubEntryIDs_L2;
			break; // 第2个副词条：L2池
		case 2: CurrentPool = &Template->AllowedSubEntryIDs_L3;
			break; // 第3个副词条：L3池 (毕业词条)
		}

		if (!CurrentPool) continue;

		FESRuntimeEntry GeneratedSubEntry;
		// 传入 UsedEntryIDs，确保不会随机到重复词条
		if (RollAndFillEntry(EntryLibrary, *CurrentPool, TargetLevel, UsedEntryIDs, GeneratedSubEntry))
		{
			GeneratedSubEntry.bIsUnlocked = false; // 默认锁定
			NewEquip->SubEntries.Add(GeneratedSubEntry);
			UsedEntryIDs.Add(GeneratedSubEntry.SourceEntryID); // 加入已使用列表
		}
	}

	// 5. 【修改】用新逻辑放入背包：找第一个空位
	int32 EmptySlot = FindFirstEmptyInventorySlot();
	if (EmptySlot != -1)
	{
		SetItemToInventorySlot(NewEquip, EmptySlot);
		NotifyInventoryChanged();
		UE_LOG(LogTemp, Log, TEXT("成功生成装备：%s，放入背包槽位：%d"), *Template->Name.ToString(), EmptySlot);
		return NewEquip;
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("背包已满，无法生成装备！"));
		return nullptr;
	}
}

// 内部辅助：随机并填充词条
bool UESInventoryManagerComponent::RollAndFillEntry(UDataTable* EntryLibrary, const TArray<FName>& AllowedIDs,
                                                    int32 Level, TArray<FName>& OutUsedIDs, FESRuntimeEntry& OutEntry)
{
	if (!EntryLibrary) return false;

	// 1. 获取所有候选词条
	TArray<FESEntryLibraryTableRow*> Candidates;
	TArray<int32> CumulativeWeights;
	int32 TotalWeight = 0;

	static const FString EntryContext(TEXT("RollEntry"));
	TArray<FName> AllRowNames = EntryLibrary->GetRowNames();

	for (FName RowName : AllRowNames)
	{
		FESEntryLibraryTableRow* Row = EntryLibrary->FindRow<FESEntryLibraryTableRow>(RowName, EntryContext);
		if (!Row) continue;

		// 如果装备模板指定了允许的词条ID，则过滤
		if (AllowedIDs.Num() > 0 && !AllowedIDs.Contains(Row->EntryID))
		{
			continue;
		}

		// ==================== 【核心新增】去重过滤 ====================
		if (OutUsedIDs.Contains(Row->EntryID))
		{
			continue; // 这个词条已经用过了，跳过
		}

		Candidates.Add(Row);
		TotalWeight += Row->Weight;
		CumulativeWeights.Add(TotalWeight);
	}

	if (Candidates.Num() == 0) return false;

	// 2. 权重随机
	int32 Roll = FMath::RandRange(1, TotalWeight);
	FESEntryLibraryTableRow* SelectedRow = nullptr;
	for (int32 i = 0; i < CumulativeWeights.Num(); ++i)
	{
		if (Roll <= CumulativeWeights[i])
		{
			SelectedRow = Candidates[i];
			break;
		}
	}

	if (!SelectedRow) return false;

	// 3. 填充运行时数据
	OutEntry.SourceEntryID = SelectedRow->EntryID;
	OutEntry.EntryType = SelectedRow->EntryType;
	OutEntry.EntryLevel = Level;
	OutEntry.TargetStat = SelectedRow->TargetStat;

	const FESEntryLevelData& LevelData = SelectedRow->GetDataForLevel(Level);
	OutEntry.bIsPercentage = LevelData.bIsPercentage;
	float RawRandomValue = RandomValueInRange(LevelData.MinValue, LevelData.MaxValue);

	// ----------------【核心重写：按规则做精度控制】----------------
	// 1. 基础属性-固定值模式：四舍五入到整数
	if (!OutEntry.bIsPercentage && 
		(OutEntry.TargetStat == EESStatType::AttackPower ||
		 OutEntry.TargetStat == EESStatType::MaxHealth ||
		 OutEntry.TargetStat == EESStatType::Defense ||
		 OutEntry.TargetStat == EESStatType::MoveSpeed))
	{
		OutEntry.FinalValue = FMath::RoundToFloat(RawRandomValue);
	}
	// 2. 所有百分比数值（基础属性百分比+百分比专属属性）：四舍五入到千分位
	else
	{
		OutEntry.FinalValue = FMath::RoundToFloat(RawRandomValue * 1000.f) / 1000.f;
	}

	return true;
}

float UESInventoryManagerComponent::RandomValueInRange(float Min, float Max)
{
	return FMath::FRandRange(Min, Max);
}

// ==================== 强化解锁接口实现 ====================
int32 UESInventoryManagerComponent::GetNextUnlockableSubEntryIndex(UESEquipItemBase* EquipItem)
{
	if (!IsValid(EquipItem)) return -1;

	for (int32 i = 0; i < EquipItem->SubEntries.Num(); ++i)
	{
		if (!EquipItem->SubEntries[i].bIsUnlocked)
		{
			return i;
		}
	}
	return -1; // 全部已解锁
}

bool UESInventoryManagerComponent::TryUnlockNextSubEntry(UESEquipItemBase* EquipItem)
{
	if (!IsValid(EquipItem)) return false;

	int32 Index = GetNextUnlockableSubEntryIndex(EquipItem);
	if (Index == -1) return false;

	// 必须按顺序解锁：检查前面的是否都解锁了
	for (int32 i = 0; i < Index; ++i)
	{
		if (!EquipItem->SubEntries[i].bIsUnlocked)
		{
			UE_LOG(LogTemp, Warning, TEXT("无法解锁索引 %d：请先解锁前面的词条！"), Index);
			return false;
		}
	}

	// 执行解锁
	EquipItem->SubEntries[Index].bIsUnlocked = true;
	UE_LOG(LogTemp, Log, TEXT("成功解锁副词条：%s (索引：%d)"), *EquipItem->SubEntries[Index].SourceEntryID.ToString(), Index);

	// ==================== 【新增】如果装备正在穿戴中，重新应用属性 ====================
	UESAbilitySystemComponent* ASC = Cast<UESAbilitySystemComponent>(GetPlayerASC());
	if (ASC)
	{
		// 检查这个装备是否正在穿戴
		UESEquipItemBase** EquippedItem = Equipment.Find(EquipItem->Type);
		if (EquippedItem && *EquippedItem == EquipItem)
		{
			UE_LOG(LogTemp, Log, TEXT("装备正在穿戴中，重新应用属性！"));
			ApplyEquipStats(ASC, EquipItem);
		}
	}

	// TODO: 这里可以加一个委托广播，通知UI刷新，或者重新应用GAS效果
	return true;
}

bool UESInventoryManagerComponent::IsSubEntryUnlocked(UESEquipItemBase* EquipItem, int32 Index)
{
	if (!IsValid(EquipItem)) return false;
	if (Index < 0 || Index >= EquipItem->SubEntries.Num()) return false;
	return EquipItem->SubEntries[Index].bIsUnlocked;
}

// ==================== 重置单个装备的词条锁定 ====================
void UESInventoryManagerComponent::ResetAllSubEntryLocks(UESEquipItemBase* EquipItem)
{
	if (!IsValid(EquipItem)) return;

	// 遍历所有副词条，全部设为锁定
	for (FESRuntimeEntry& SubEntry : EquipItem->SubEntries)
	{
		SubEntry.bIsUnlocked = false;
	}

	UE_LOG(LogTemp, Log, TEXT("已重置装备 [%s] 的所有词条锁定"), *EquipItem->Name.ToString());

	// 如果装备正在穿戴，重新应用GAS属性（移除已解锁的属性加成）
	UESAbilitySystemComponent* ASC = Cast<UESAbilitySystemComponent>(GetPlayerASC());
	if (ASC)
	{
		UESEquipItemBase** EquippedItem = Equipment.Find(EquipItem->Type);
		if (EquippedItem && *EquippedItem == EquipItem)
		{
			ApplyEquipStats(ASC, EquipItem);
		}
	}
}

// ==================== 重置背包和装备栏里所有装备的词条锁定 ====================
void UESInventoryManagerComponent::ResetAllEquipmentsInInventory()
{
	UE_LOG(LogTemp, Log, TEXT("========== 开始重置所有装备词条锁定 =========="));

	// 1. 重置背包里的所有装备
	int32 ResetCount = 0;
	for (UESItemBase* Item : Inventory)
	{
		UESEquipItemBase* EquipItem = Cast<UESEquipItemBase>(Item);
		if (EquipItem)
		{
			ResetAllSubEntryLocks(EquipItem);
			ResetCount++;
		}
	}

	// 2. 重置装备栏里的所有装备
	for (const auto& Pair : Equipment)
	{
		UESEquipItemBase* EquipItem = Pair.Value;
		if (IsValid(EquipItem))
		{
			ResetAllSubEntryLocks(EquipItem);
			ResetCount++;
		}
	}

	// 3. 广播刷新UI
	NotifyInventoryChanged();
	NotifyEquipmentChanged();

	UE_LOG(LogTemp, Log, TEXT("========== 重置完成，共重置 %d 件装备 =========="), ResetCount);
}

// ==================== 保存游戏核心逻辑 ====================
void UESInventoryManagerComponent::SaveGame()
{
	UE_LOG(LogTemp, Log, TEXT("========== 开始保存游戏 =========="));

	// 1. 创建 SaveGame 实例
	// 原理：USaveGame 是 UE 专门用来序列化的容器
	UESGameSave* SaveInstance = Cast<UESGameSave>(UGameplayStatics::CreateSaveGameObject(UESGameSave::StaticClass()));
	if (!SaveInstance)
	{
		UE_LOG(LogTemp, Error, TEXT("SaveGame 失败：无法创建 SaveGame 实例！"));
		return;
	}

	// 2. 填充基础数据
	SaveInstance->PlayerGold = CurrentGold;
	UE_LOG(LogTemp, Log, TEXT("  -> 保存金币：%d"), CurrentGold);

	// 3. 保存背包：记录槽位索引
	SaveInstance->InventorySaveData.Empty();
	for (int32 i = 0; i < Inventory.Num(); ++i)
	{
		UESItemBase* Item = Inventory[i];
		if (IsValid(Item))
		{
			FItemSaveData ItemData(Item);
			ItemData.SavedSlotIndex = i; // ✅ 关键：保存当前索引
			SaveInstance->InventorySaveData.Add(ItemData);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("  -> 保存背包物品：%d 件"), SaveInstance->InventorySaveData.Num());

	// 4. 保存装备栏
	SaveInstance->EquipmentSaveData.Empty();
	for (const auto& Pair : Equipment)
	{
		EESItemType Type = Pair.Key;
		UESEquipItemBase* EquipItem = Pair.Value;
		if (IsValid(EquipItem))
		{
			SaveInstance->EquipmentSaveData.Add(Type, FItemSaveData(EquipItem));
		}
	}
	UE_LOG(LogTemp, Log, TEXT("  -> 保存装备栏物品：%d 件"), SaveInstance->EquipmentSaveData.Num());

	// 5. 保存仓库：记录槽位索引
	SaveInstance->WarehouseSaveData.Empty();
	for (int32 i = 0; i < Warehouse.Num(); ++i)
	{
		UESItemBase* Item = Warehouse[i];
		if (IsValid(Item))
		{
			FItemSaveData ItemData(Item);
			ItemData.SavedSlotIndex = i; // ✅ 关键：保存当前索引
			SaveInstance->WarehouseSaveData.Add(ItemData);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("  -> 保存仓库物品：%d 件"), SaveInstance->WarehouseSaveData.Num());

	// 6. 写入硬盘
	// 原理：UGameplayStatics::SaveGameToSlot 自动处理文件IO
	bool bSuccess = UGameplayStatics::SaveGameToSlot(SaveInstance, SaveInstance->SaveSlotName, SaveInstance->SaveIndex);

	if (bSuccess)
	{
		UE_LOG(LogTemp, Log, TEXT("========== 保存游戏成功！ =========="));
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("========== 保存游戏失败！ =========="));
	}
}

// ==================== 读取游戏核心逻辑 ====================
void UESInventoryManagerComponent::LoadGame()
{
	UE_LOG(LogTemp, Log, TEXT("========== 开始读取游戏 =========="));

	// 1. 从硬盘读取 SaveGame
	UESGameSave* SaveInstance = Cast<UESGameSave>(UGameplayStatics::LoadGameFromSlot("PlayerSave01", 0));
	if (!SaveInstance)
	{
		UE_LOG(LogTemp, Warning, TEXT("没有找到存档，使用初始数据（这是玩家第一次玩）"));
		return;
	}

	// 2. 清空旧数据
	Inventory.Empty();
	Equipment.Empty();
	Warehouse.Empty();

	// 3. 恢复金币
	CurrentGold = SaveInstance->PlayerGold;
	UE_LOG(LogTemp, Log, TEXT("  -> 恢复金币：%d"), CurrentGold);

	// ==========================================
	// 4. 【核心修复】恢复背包：直接放到存档时的索引位置
	// ==========================================
	for (const FItemSaveData& ItemData : SaveInstance->InventorySaveData)
	{
		UESItemBase* NewItem = nullptr;

		if (ItemData.bIsEquipItem)
		{
			NewItem = NewObject<UESEquipItemBase>(this, UESEquipItemBase::StaticClass());
		}
		else
		{
			NewItem = NewObject<UESItemBase>(this, UESItemBase::StaticClass());
		}

		if (!NewItem) continue;

		// 填充基础数据
		NewItem->ItemGUID = ItemData.ItemGUID;
		NewItem->Name = ItemData.Name;
		NewItem->Description = ItemData.Description;
		NewItem->Icon = ItemData.Icon;
		NewItem->Type = ItemData.Type;
		NewItem->Quality = ItemData.Quality;
		NewItem->BuyPrice = ItemData.BuyPrice;
		NewItem->SellPrice = ItemData.SellPrice;

		// 填充词条
		if (ItemData.bIsEquipItem)
		{
			UESEquipItemBase* NewEquip = Cast<UESEquipItemBase>(NewItem);
			if (NewEquip)
			{
				NewEquip->MainEntry = ItemData.MainEntryData.ToRuntimeEntry();
				NewEquip->SubEntries.Empty();
				for (const FEntrySaveData& SubData : ItemData.SubEntriesData)
				{
					NewEquip->SubEntries.Add(SubData.ToRuntimeEntry());
				}
			}
		}

		// ✅ 核心修改：直接放到存档时的索引位置，不Add到末尾
		int32 TargetIdx = ItemData.SavedSlotIndex;
		if (TargetIdx != -1)
		{
			SetItemToInventorySlot(NewItem, TargetIdx);
		}
		else
		{
			// 兜底：如果没有存档索引，找第一个空位
			int32 EmptySlot = FindFirstEmptyInventorySlot();
			if (EmptySlot != -1)
			{
				SetItemToInventorySlot(NewItem, EmptySlot);
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("  -> 恢复背包物品：%d 件"), Inventory.Num());

	// ==========================================
	// 5. 【核心修复】恢复装备栏：和背包一样，直接重建
	// ==========================================
	for (const auto& Pair : SaveInstance->EquipmentSaveData)
	{
		EESItemType Type = Pair.Key;
		const FItemSaveData& ItemData = Pair.Value;

		UESItemBase* NewItem = nullptr;
		if (ItemData.bIsEquipItem)
		{
			NewItem = NewObject<UESEquipItemBase>(this, UESEquipItemBase::StaticClass());
		}
		else
		{
			NewItem = NewObject<UESItemBase>(this, UESItemBase::StaticClass());
		}

		if (!NewItem) continue;

		// 填充数据
		NewItem->ItemGUID = ItemData.ItemGUID;
		NewItem->Name = ItemData.Name;
		NewItem->Description = ItemData.Description;
		NewItem->Icon = ItemData.Icon;
		NewItem->Type = ItemData.Type;
		NewItem->Quality = ItemData.Quality;
		NewItem->BuyPrice = ItemData.BuyPrice;
		NewItem->SellPrice = ItemData.SellPrice;

		// 填充词条
		if (ItemData.bIsEquipItem)
		{
			UESEquipItemBase* NewEquip = Cast<UESEquipItemBase>(NewItem);
			if (NewEquip)
			{
				NewEquip->MainEntry = ItemData.MainEntryData.ToRuntimeEntry();
				NewEquip->SubEntries.Empty();
				for (const FEntrySaveData& SubData : ItemData.SubEntriesData)
				{
					NewEquip->SubEntries.Add(SubData.ToRuntimeEntry());
				}
			}
		}

		// 加入装备栏
		UESEquipItemBase* NewEquip = Cast<UESEquipItemBase>(NewItem);
		if (NewEquip)
		{
			Equipment.Add(Type, NewEquip);
		}
	}
	UE_LOG(LogTemp, Log, TEXT("  -> 恢复装备栏物品：%d 件"), Equipment.Num());

	// ==========================================
	// 6. 【核心修复】恢复仓库：直接放到存档时的索引位置
	// ==========================================
	for (const FItemSaveData& ItemData : SaveInstance->WarehouseSaveData)
	{
		UESItemBase* NewItem = nullptr;

		if (ItemData.bIsEquipItem)
		{
			NewItem = NewObject<UESEquipItemBase>(this, UESEquipItemBase::StaticClass());
		}
		else
		{
			NewItem = NewObject<UESItemBase>(this, UESItemBase::StaticClass());
		}

		if (!NewItem) continue;

		// 填充数据
		NewItem->ItemGUID = ItemData.ItemGUID;
		NewItem->Name = ItemData.Name;
		NewItem->Description = ItemData.Description;
		NewItem->Icon = ItemData.Icon;
		NewItem->Type = ItemData.Type;
		NewItem->Quality = ItemData.Quality;
		NewItem->BuyPrice = ItemData.BuyPrice;
		NewItem->SellPrice = ItemData.SellPrice;

		// 填充词条
		if (ItemData.bIsEquipItem)
		{
			UESEquipItemBase* NewEquip = Cast<UESEquipItemBase>(NewItem);
			if (NewEquip)
			{
				NewEquip->MainEntry = ItemData.MainEntryData.ToRuntimeEntry();
				NewEquip->SubEntries.Empty();
				for (const FEntrySaveData& SubData : ItemData.SubEntriesData)
				{
					NewEquip->SubEntries.Add(SubData.ToRuntimeEntry());
				}
			}
		}

		// ✅ 核心修改：直接放到存档时的索引位置
		int32 TargetIdx = ItemData.SavedSlotIndex;
		if (TargetIdx != -1)
		{
			SetItemToWarehouseSlot(NewItem, TargetIdx);
		}
		else
		{
			int32 EmptySlot = FindFirstEmptyWarehouseSlot();
			if (EmptySlot != -1)
			{
				SetItemToWarehouseSlot(NewItem, EmptySlot);
			}
		}
	}
	UE_LOG(LogTemp, Log, TEXT("  -> 恢复仓库物品：%d 件"), Warehouse.Num());

	// 7. 广播所有变化事件，刷新UI
	NotifyInventoryChanged();
	NotifyEquipmentChanged();
	OnGoldChanged.Broadcast(CurrentGold);
	OnWarehouseUpdated.Broadcast();

	// 8. 如果有装备正在穿戴，重新应用GAS属性
	UESAbilitySystemComponent* ASC = Cast<UESAbilitySystemComponent>(GetPlayerASC());
	if (ASC)
	{
		for (const auto& Pair : Equipment)
		{
			UESEquipItemBase* EquipItem = Pair.Value;
			if (IsValid(EquipItem))
			{
				ApplyEquipStats(ASC, EquipItem);
			}
		}
	}

	UE_LOG(LogTemp, Log, TEXT("========== 读取游戏成功！ =========="));
}
