#include "Item/Data/ESItemBase.h"

UESItemBase::UESItemBase()
{
}

FLinearColor UESItemBase::GetQualityColor() const
{
    switch (Quality)
    {
        case EESItemQuality::Normal:
            return FLinearColor::White; // 普通-白色
        case EESItemQuality::Rare:
            return FLinearColor(0.1f, 0.4f, 1.0f); // 稀有-蓝色
        case EESItemQuality::Legendary:
            return FLinearColor(1.0f, 0.5f, 0.0f); // 传说-橙色
        default:
            return FLinearColor::Gray;
    }
}