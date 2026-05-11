#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ESPickupBase.generated.h"

class USphereComponent;
class UWidgetComponent;

UCLASS()
class ETERNALSANCTUARY_API AESPickupBase : public AActor
{
	GENERATED_BODY()
	
public:	
	AESPickupBase();

protected:
	virtual void BeginPlay() override;

public:	
	// ==================== 核心组件 ====================
	/** 碰撞体（用于检测范围） */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	USphereComponent* CollisionSphere;

	/** 根场景组件 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	USceneComponent* RootScene;

	// ==================== 配置参数 ====================
	/** 拾取交互距离 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Config")
	float InteractionDistance = 300.f;

	/** 显示在UI上的名字 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Display")
	FText DisplayName;

	/** 显示在UI上的颜色（品质色） */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|Display")
	FLinearColor DisplayColor = FLinearColor::White;

	// ==================== 交互逻辑 ====================
	/** 【子类重写】执行拾取/交互 */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pickup")
	void OnInteract(APawn* InteractingPawn);
	virtual void OnInteract_Implementation(APawn* InteractingPawn);

	/** 【子类重写】是否可以交互（默认true） */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Pickup")
	bool CanInteract() const;
	virtual bool CanInteract_Implementation() const;

	/** 销毁拾取物 */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void DestroyPickup();
};