#include "Item/Lib/ESUIHelperLibrary.h"

FLinearColor UESUIHelperLibrary::GetColorForQuality(EESItemQuality Quality)
{
	switch (Quality)
	{
	case EESItemQuality::Normal:    return FLinearColor(0.75f, 0.75f, 0.75f); // 灰白色
	case EESItemQuality::Rare:      return FLinearColor(0.0f, 0.5f, 1.0f);    // 蓝色
	case EESItemQuality::Legendary: return FLinearColor(1.0f, 0.5f, 0.0f);    // 橙色
	default: return FLinearColor::White;
	}
}

FLinearColor UESUIHelperLibrary::GetLockedColor()
{
	return FLinearColor(0.4f, 0.4f, 0.4f); // 深灰色
}

FText UESUIHelperLibrary::GetStatName(EESStatType StatType)
{
	switch (StatType)
	{
	case EESStatType::AttackPower:    return FText::FromString(TEXT("攻击力"));
	case EESStatType::MaxHealth:      return FText::FromString(TEXT("最大生命值"));
	case EESStatType::Defense:        return FText::FromString(TEXT("防御力"));
	case EESStatType::CritRate:       return FText::FromString(TEXT("暴击率"));
	case EESStatType::CritDamage:     return FText::FromString(TEXT("暴击伤害"));
	case EESStatType::MoveSpeed:      return FText::FromString(TEXT("移动速度"));
	case EESStatType::CooldownReduction: return FText::FromString(TEXT("冷却缩减"));
	default: return FText::FromString(TEXT("未知属性"));
	}
}

FText UESUIHelperLibrary::FormatEntryText(EESStatType StatType, float Value, bool bIsPercentage)
{
	FText StatName = GetStatName(StatType);
	FString ValueString;

	if (bIsPercentage)
	{
		// 百分比属性：把0.1转成10%显示
		ValueString = FString::Printf(TEXT("+%.0f%%"), Value * 100.0f);
	}
	else
	{
		// 普通属性：直接显示数值
		ValueString = FString::Printf(TEXT("+%.0f"), Value);
	}

	return FText::FromString(FString::Printf(TEXT("%s %s"), *StatName.ToString(), *ValueString));
}

FText UESUIHelperLibrary::GetItemTypeName(EESItemType Type)
{
	switch (Type)
	{
	case EESItemType::Weapon:    return FText::FromString(TEXT("武器"));
	case EESItemType::Armor:     return FText::FromString(TEXT("防具"));
	case EESItemType::Accessory: return FText::FromString(TEXT("饰品"));
	default: return FText::FromString(TEXT("未知"));
	}
}

FText UESUIHelperLibrary::GetQualityName(EESItemQuality Quality)
{
	switch (Quality)
	{
	case EESItemQuality::Normal:    return FText::FromString(TEXT("普通"));
	case EESItemQuality::Rare:      return FText::FromString(TEXT("稀有"));
	case EESItemQuality::Legendary: return FText::FromString(TEXT("传说"));
	default: return FText::FromString(TEXT("未知"));
	}
}