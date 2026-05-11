#include "Item/Pickup/ESPickupEquip.h"

#include "GameFramework/PlayerState.h"
#include "Item/Data/ESEquipItemBase.h"
#include "Item/Manager/ESInventoryManagerComponent.h"
#include "Kismet/GameplayStatics.h"

AESPickupEquip::AESPickupEquip()
{
}

void AESPickupEquip::BeginPlay()
{
	Super::BeginPlay();
}

void AESPickupEquip::InitializeWithEquip(UESEquipItemBase* EquipItem)
{
	if (!IsValid(EquipItem)) return;

	SavedEquipItem = EquipItem;
	DisplayName = EquipItem->Name;

	// 根据品质设置颜色
	switch (EquipItem->Quality)
	{
	case EESItemQuality::Normal: DisplayColor = FLinearColor::White; break;
	case EESItemQuality::Rare: DisplayColor = FLinearColor::Blue; break;
	case EESItemQuality::Legendary: DisplayColor = FLinearColor(1.0f, 0.5f, 0.0f); break;
	default: DisplayColor = FLinearColor::Gray; break;
	}
}

void AESPickupEquip::OnInteract_Implementation(APawn* InteractingPawn)
{
	if (!IsValid(InteractingPawn) || !IsValid(SavedEquipItem)) return;

	// 获取InventoryManager
	UESInventoryManagerComponent* InventoryManager = InteractingPawn->FindComponentByClass<UESInventoryManagerComponent>();
	// 如果Pawn身上没有，尝试从PlayerState获取（根据你的架构调整）
	if (!InventoryManager)
	{
		APlayerState* PState = InteractingPawn->GetPlayerState();
		if (PState)
		{
			InventoryManager = PState->FindComponentByClass<UESInventoryManagerComponent>();
		}
	}

	if (!InventoryManager) return;

	// 尝试放入背包
	int32 EmptySlot = InventoryManager->FindFirstEmptyInventorySlot();
	if (EmptySlot != -1)
	{
		InventoryManager->SetItemToInventorySlot(SavedEquipItem, EmptySlot);
		InventoryManager->TriggerInventoryRefresh();
		InventoryManager->SaveGame();
		
		// 拾取成功，销毁拾取物
		DestroyPickup();
	}
	else
	{
		// 背包已满，提示（你可以自己加UI提示）
		GEngine->AddOnScreenDebugMessage(-1, 2.f, FColor::Red, TEXT("背包已满！"));
	}
}