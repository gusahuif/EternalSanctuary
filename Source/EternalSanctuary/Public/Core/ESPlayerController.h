#pragma once

#include "CoreMinimal.h"
#include "GameFramework/PlayerController.h"
#include "ESPlayerController.generated.h"

class AESPickupBase;
enum class ESkillSlotID : uint8;
class UInputAction;
class UInputMappingContext;
class AESPlayerBase;
struct FInputActionValue;

UCLASS()
class ETERNALSANCTUARY_API AESPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	AESPlayerController();

protected:
	virtual void BeginPlay() override;
	virtual void SetupInputComponent() override;
	virtual void PlayerTick(float DeltaTime) override;

protected:
	/** 左键按下开始：记录时间并初始化本次输入状态 */
	void OnLeftClickStarted(const FInputActionValue& Value);

	/** 左键持续触发：超过阈值后进入长按模式，并持续更新方向 */
	void OnLeftClickTriggered(const FInputActionValue& Value);

	/** 左键释放：未进入长按则走单击逻辑，长按中则停止长按移动 */
	void OnLeftClickReleased(const FInputActionValue& Value);

	/** 打断当前命令：新输入开始前统一调用，保证命令切换规则一致 */
	void InterruptCurrentCommand();

	/** 处理单击逻辑：优先判定敌人，其次处理地面点击寻路 */
	void HandleTapAction();

	/** 处理长按逻辑：每帧根据鼠标落点更新角色直线移动方向 */
	void HandleHoldAction();

	/** 判断指定 Actor 是否应被视为敌人：当前预留空实现 */
	bool IsEnemyActor(AActor* InActor) const;

	/** 辅助函数：判断目标是否存活 */
	bool IsTargetAlive(AActor* InActor) const;

	/** 重新扫描鼠标下的敌人（用于当前目标死亡后的自动切换） */
	void RescanEnemyUnderCursor();
	
	/** 动作回调函数 */
	void OnRightClickStarted(const FInputActionValue& Value);
	void OnRightClickTriggered(const FInputActionValue& Value);
	void OnRightClickReleased(const FInputActionValue& Value);

	void OnPotionTriggered();
	void OnDodgeTriggered();
	void OnSkillTriggered(ESkillSlotID SkillSlot);
	void OnSkillReleased(ESkillSlotID SkillSlot);
	/** 强制攻击状态追踪 */
	void OnForceAttackStarted();
	void OnForceAttackCompleted();

	// ==================== 拾取系统 ====================
	/** 当前范围内的所有拾取物 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	TArray<AESPickupBase*> NearbyPickups;

	/** 当前选中的拾取物索引 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup")
	int32 SelectedPickupIndex = 0;

	/** 拾取列表UI类 */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Pickup|UI")
	TSubclassOf<UUserWidget> PickupListWidgetClass;

	/** 拾取列表UI实例 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Pickup|UI")
	UUserWidget* PickupListWidget;

	// ==================== 拾取函数 ====================
	/** 更新附近拾取物列表（由Character或Timer调用） */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void UpdateNearbyPickups();

	/** 滚轮选择下一个 */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void SelectNextPickup();

	/** 滚轮选择上一个 */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void SelectPreviousPickup();

	/** 与选中的拾取物交互 */
	UFUNCTION(BlueprintCallable, Category = "Pickup")
	void InteractWithSelected();
	
	/** 获取当前受控玩家角色 */
	AESPlayerBase* GetESPlayerCharacter() const;

protected:
	/** Enhanced Input 映射上下文：由编辑器配置 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputMappingContext> DefaultInputMappingContext;

	/** 左键输入动作：Started / Triggered / Released 都绑定到它 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputAction> IA_LeftClick;
	
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputAction> IA_RightClick;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputAction> IA_Potion;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputAction> IA_Skill1;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputAction> IA_Skill2;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputAction> IA_Skill3;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputAction> IA_Skill4;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputAction> IA_Dodge;

	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	TObjectPtr<UInputAction> IA_ForceAttack; // Shift 强制攻击
	
	/** 单击与长按的判定阈值，单位秒 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Input")
	float HoldThreshold = 0.18f;

	/** 地面检测通道：默认使用 ETraceTypeQuery::TraceTypeQuery1 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Trace")
	TEnumAsByte<ETraceTypeQuery> GroundTraceChannel = ETraceTypeQuery::TraceTypeQuery1;

	/** 目标检测通道：默认使用 ETraceTypeQuery::TraceTypeQuery2 */
	UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "ES|Trace")
	TEnumAsByte<ECollisionChannel> TargetTraceChannel = ECollisionChannel::ECC_GameTraceChannel1;

	/** 左键是否正处于按住状态 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Input")
	bool bLeftClickPressed = false;

	/** 当前这次按下是否已经进入长按模式 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Input")
	bool bInHoldMode = false;

	/** 本次左键按下的游戏时间，用于单击/长按判定 */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Input")
	float LeftClickPressedTime = 0.f;
	
	/** 初始点击时是否点中了敌人（用于锁定攻击意图） */
	bool bIsInitialClickOnEnemy = false;

	/** 初始点击时是否点中了地面（用于锁定移动意图） */
	bool bIsInitialClickOnGround = false;

	/** 标记 Shift 是否被按下 */
	bool bIsForceAttackPressed = false;

	/** 右键状态记录（逻辑同左键） */
	bool bRightClickPressed = false;
	bool bRightInHoldMode = false;
	float RightClickPressedTime = 0.f;
};
