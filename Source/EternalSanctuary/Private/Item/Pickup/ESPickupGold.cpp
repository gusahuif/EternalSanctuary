#include "Item/Pickup/ESPickupGold.h"
#include "Components/SphereComponent.h"
#include "Item/Manager/ESInventoryManagerComponent.h"
#include "GameFramework/PlayerState.h"

AESPickupGold::AESPickupGold()
{
	// 创建自动拾取碰撞体（更小的半径）
	AutoPickupCollision = CreateDefaultSubobject<USphereComponent>(TEXT("AutoPickupCollision"));
	AutoPickupCollision->SetupAttachment(RootScene);
	AutoPickupCollision->SetSphereRadius(100.f);
	AutoPickupCollision->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	AutoPickupCollision->SetCollisionResponseToAllChannels(ECR_Ignore);
	AutoPickupCollision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Overlap);

	// 金币默认名字
	DisplayName = FText::FromString("金币");
	DisplayColor = FLinearColor::Yellow;
}

void AESPickupGold::BeginPlay()
{
	Super::BeginPlay();

	// 绑定自动拾取碰撞
	AutoPickupCollision->OnComponentBeginOverlap.AddDynamic(this, &AESPickupGold::OnAutoPickupOverlap);
}

void AESPickupGold::OnAutoPickupOverlap(UPrimitiveComponent* OverlappedComponent, AActor* OtherActor, UPrimitiveComponent* OtherComp, int32 OtherBodyIndex, bool bFromSweep, const FHitResult& SweepResult)
{
	// 自动拾取金币
	APawn* Pawn = Cast<APawn>(OtherActor);
	if (IsValid(Pawn))
	{
		OnInteract(Pawn);
	}
}

void AESPickupGold::OnInteract_Implementation(APawn* InteractingPawn)
{
	if (!IsValid(InteractingPawn)) return;

	// 获取InventoryManager
	UESInventoryManagerComponent* InventoryManager = InteractingPawn->FindComponentByClass<UESInventoryManagerComponent>();
	if (!InventoryManager)
	{
		APlayerState* PS = InteractingPawn->GetPlayerState();
		if (PS)
		{
			InventoryManager = PS->FindComponentByClass<UESInventoryManagerComponent>();
		}
	}

	if (!InventoryManager) return;

	// 增加金币
	InventoryManager->ModifyGold(GoldAmount);
	InventoryManager->SaveGame();

	// 拾取成功，销毁拾取物
	DestroyPickup();
}