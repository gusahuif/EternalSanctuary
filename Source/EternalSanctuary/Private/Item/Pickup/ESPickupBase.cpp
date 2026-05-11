#include "Item/Pickup/ESPickupBase.h"
#include "Components/SphereComponent.h"
#include "Components/SceneComponent.h"

AESPickupBase::AESPickupBase()
{
	PrimaryActorTick.bCanEverTick = false;

	// 创建根组件
	RootScene = CreateDefaultSubobject<USceneComponent>(TEXT("RootScene"));
	SetRootComponent(RootScene);

	// 创建碰撞体
	CollisionSphere = CreateDefaultSubobject<USphereComponent>(TEXT("CollisionSphere"));
	CollisionSphere->SetupAttachment(RootScene);
	CollisionSphere->SetSphereRadius(InteractionDistance);
	CollisionSphere->SetCollisionEnabled(ECollisionEnabled::QueryOnly);
	CollisionSphere->SetCollisionResponseToAllChannels(ECR_Ignore);
}

void AESPickupBase::BeginPlay()
{
	Super::BeginPlay();
}

void AESPickupBase::OnInteract_Implementation(APawn* InteractingPawn)
{
	// 基类不做具体逻辑，子类重写
}

bool AESPickupBase::CanInteract_Implementation() const
{
	return true;
}

void AESPickupBase::DestroyPickup()
{
	Destroy();
}