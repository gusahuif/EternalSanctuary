#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "Item/Data/ESInventoryTypes.h"
#include "ESItemBase.generated.h"

/**
 * 所有物品的通用基类（装备、材料、消耗品均继承此基类）
 */
UCLASS(BlueprintType, Blueprintable)
class ETERNALSANCTUARY_API UESItemBase : public UObject
{
    GENERATED_BODY()

public:
    UESItemBase();

    // 全局唯一标识符（管理器统一生成）
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Item|Base")
    FGuid ItemGUID;

    // 物品类型
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Item|Base")
    EESItemType Type;

    // 物品品质
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Item|Base")
    EESItemQuality Quality = EESItemQuality::Normal;

    // 物品名称
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Item|Base")
    FText Name;

    // 物品图标（软引用省内存）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Item|Base")
    TSoftObjectPtr<UTexture2D> Icon;

    // 物品描述
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Item|Base")
    FText Description;

    // 出售价格（0=不可出售）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Item|Base")
    int32 SellPrice = 0;

    // 购买价格（0=不可购买）
    UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "ES|Item|Base")
    int32 BuyPrice = 0;

    // 获取品质对应的颜色（UI用）
    UFUNCTION(BlueprintCallable, Category = "ES|Item|Util")
    FLinearColor GetQualityColor() const;
};