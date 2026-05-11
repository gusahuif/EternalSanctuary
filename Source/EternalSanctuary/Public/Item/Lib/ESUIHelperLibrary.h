#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "Item/Data/ESInventoryTypes.h"
#include "ESUIHelperLibrary.generated.h"

UCLASS()
class ETERNALSANCTUARY_API UESUIHelperLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** 根据稀有度获取对应的颜色 */
	UFUNCTION(BlueprintPure, Category = "ES|UI")
	static FLinearColor GetColorForQuality(EESItemQuality Quality);

	/** 获取未解锁词条的灰色 */
	UFUNCTION(BlueprintPure, Category = "ES|UI")
	static FLinearColor GetLockedColor();

	/** 属性类型转显示文字 */
	UFUNCTION(BlueprintPure, Category = "ES|UI")
	static FText GetStatName(EESStatType StatType);

	/** 格式化词条显示 (比如 "攻击力 +25" 或 "暴击率 +10%") */
	UFUNCTION(BlueprintPure, Category = "ES|UI")
	static FText FormatEntryText(EESStatType StatType, float Value, bool bIsPercentage);

	/** 装备类型转文字 */
	UFUNCTION(BlueprintPure, Category = "ES|UI")
	static FText GetItemTypeName(EESItemType Type);

	/** 稀有度转文字 */
	UFUNCTION(BlueprintPure, Category = "ES|UI")
	static FText GetQualityName(EESItemQuality Quality);
};