#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "GameplayEffect.h"
#include "GameplayAbilitySpec.h"
#include "ESInventoryTypes.generated.h"

class UGameplayAbility;
class UESItemBase;
class UESEquipItemBase;

// ==================== 核心物品类型枚举====================
/** 物品/装备核心类型 */
UENUM(BlueprintType)
enum class EESItemType : uint8
{
    Weapon     UMETA(DisplayName = "武器"),
    Armor      UMETA(DisplayName = "防具"),
    Accessory  UMETA(DisplayName = "饰品")
};

// ==================== 其他枚举定义 ====================
/** 物品品质（对应普通/稀有/传说） */
UENUM(BlueprintType)
enum class EESItemQuality : uint8
{
    Normal    UMETA(DisplayName = "普通"),
    Rare      UMETA(DisplayName = "稀有"),
    Legendary UMETA(DisplayName = "传说")
};


/** 物品槽位上下文（用于UI右键逻辑判断） */
UENUM(BlueprintType)
enum class EESSlotContext : uint8
{
    Inventory  UMETA(DisplayName = "背包栏"),
    Equipment  UMETA(DisplayName = "装备栏"),
    Shop       UMETA(DisplayName = "商店栏"),
    Warehouse  UMETA(DisplayName = "仓库栏")
};

// ==================== 词条类型枚举 ====================
UENUM(BlueprintType)
enum class EESEntryType : uint8
{
    Attribute   UMETA(DisplayName = "属性词条"), // 主词条/基础属性
    Skill       UMETA(DisplayName = "技能词条")  // 特殊技能/特效
};

// ==================== 装备属性词条类型（完全匹配你的UESAttributeSet） ====================
UENUM(BlueprintType)
enum class EESStatType : uint8
{
    AttackPower    UMETA(DisplayName = "攻击力"),
    MaxHealth      UMETA(DisplayName = "最大生命值"),
    Defense        UMETA(DisplayName = "防御力"),
    CritRate       UMETA(DisplayName = "暴击率"),
    CritDamage     UMETA(DisplayName = "暴击伤害"),
    MoveSpeed      UMETA(DisplayName = "移动速度"),
    CooldownReduction UMETA(DisplayName = "冷却缩减")
    // 你可以随时在这里扩展你的AS里有的其他属性
};

// ==================== 单个词条等级的详细数据（1级/2级/3级） ====================
USTRUCT(BlueprintType)
struct FESEntryLevelData
{
    GENERATED_BODY()

    /** 数值下限 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry")
    float MinValue = 0.f;

    /** 数值上限 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry")
    float MaxValue = 0.f;

    /** 是否为百分比加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry")
    bool bIsPercentage = false;
};
// ==================== 【核心】词条库数据表行结构（移除了ValidItemTypes） ====================
USTRUCT(BlueprintType)
struct FESEntryLibraryTableRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 词条唯一ID (如 "Attr_AttackPower") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry|Base")
    FName EntryID;

    /** 词条显示名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry|Base")
    FText DisplayName;

    /** 词条类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry|Base")
    EESEntryType EntryType = EESEntryType::Attribute;

    /** 该词条的权重 (影响随机出现的概率) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry|Base", meta = (MinValue = 1))
    int32 Weight = 100;

    // -------------------- 属性词条专属 --------------------
    /** 目标属性类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry|Attribute")
    EESStatType TargetStat = EESStatType::AttackPower;

    // -------------------- 技能词条专属 (占位，后续你自己填) --------------------
    /** 授予的技能类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry|Skill")
    TSubclassOf<UGameplayAbility> GrantAbility;

    // -------------------- 分等级数据 --------------------
    /** 1级词条数据 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry|Levels")
    FESEntryLevelData Level1Data;

    /** 2级词条数据 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry|Levels")
    FESEntryLevelData Level2Data;

    /** 3级词条数据 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Entry|Levels")
    FESEntryLevelData Level3Data;

    // ==================== 工具函数 ====================
    FORCEINLINE const FESEntryLevelData& GetDataForLevel(int32 Level) const
    {
        switch (Level)
        {
        case 1: return Level1Data;
        case 2: return Level2Data;
        case 3: return Level3Data;
        default: return Level1Data;
        }
    }
};

// ==================== 【新增核心】装备模板数据表行结构（差异化的根源） ====================
USTRUCT(BlueprintType)
struct FESEquipTemplateTableRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 装备模板ID (如 "Sword_Berserker") */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Base")
    FName TemplateID;

    /** 装备名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Base")
    FText Name;

    /** 装备描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Base")
    FText Description;

    /** 装备图标 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Base")
    TSoftObjectPtr<UTexture2D> Icon;

    /** 装备类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Base")
    EESItemType Type;

    /** 基础价格 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Base")
    int32 BasePrice = 100;

    // ==================== 【关键】该装备的专属词条池 ====================
    
    /** 允许出现的主词条ID列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Entries")
    TArray<FName> AllowedMainEntryIDs;

    /** 【第1个副词条】允许的词条ID列表 (解锁第1个时随机) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Entries|Sub")
    TArray<FName> AllowedSubEntryIDs_L1;

    /** 【第2个副词条】允许的词条ID列表 (解锁第2个时随机) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Entries|Sub")
    TArray<FName> AllowedSubEntryIDs_L2;

    /** 【第3个副词条】允许的词条ID列表 (解锁第3个时随机，可放毕业词条) */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Entries|Sub")
    TArray<FName> AllowedSubEntryIDs_L3;
};

// ==================== 运行时的词条实例 ====================
USTRUCT(BlueprintType)
struct FESRuntimeEntry
{
    GENERATED_BODY()

    /** 关联的词条库ID */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Entry")
    FName SourceEntryID;

    /** 词条类型 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Entry")
    EESEntryType EntryType = EESEntryType::Attribute;

    /** 词条等级 (1/2/3，决定了数值范围) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Entry")
    int32 EntryLevel = 1;

    /** 是否已解锁 (仅副词条有效，主词条默认解锁) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Entry")
    bool bIsUnlocked = false;

    /** 随机出来的最终数值 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Entry|Attribute")
    float FinalValue = 0.f;

    /** 是否为百分比 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Entry|Attribute")
    bool bIsPercentage = false;

    /** 目标属性 */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Entry|Attribute")
    EESStatType TargetStat = EESStatType::AttackPower;

    /** 授予的技能句柄 (运行时缓存) */
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "ES|Entry|Skill")
    FGameplayAbilitySpecHandle GrantedAbilityHandle;
};


// ==================== 结构体定义 ====================
/** 装备属性词条结构体 */
USTRUCT(BlueprintType)
struct FESItemStat
{
    GENERATED_BODY()

    /** 词条类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EESStatType StatType = EESStatType::AttackPower;

    /** 词条数值 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float Value = 0.f;

    /** 词条是否为百分比加成 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bIsPercentage = false;
};

/** 装备授予的技能信息结构体 */
USTRUCT(BlueprintType)
struct FESItemAbilityInfo
{
    GENERATED_BODY()

    /** 技能蓝图类 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSubclassOf<UGameplayAbility> AbilityClass;

    /** 技能等级（随品质自动调整） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 AbilityLevel = 1;

    /** 技能激活所需的标签（可选） */
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTagContainer ActivationTags;
};

// ==================== DataTable 配置用结构体 ====================
/** 装备数据表行结构 */
USTRUCT(BlueprintType)
struct FESEquipDataTableRow : public FTableRowBase
{
    GENERATED_BODY()

    /** 装备名称 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip")
    FText Name;

    /** 装备描述 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip")
    FText Description;

    /** 装备图标 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip")
    TSoftObjectPtr<UTexture2D> Icon;

    /** 装备类型 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip")
    EESItemType Type;

    /** 装备品质 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip")
    EESItemQuality Quality;

    /** 购买价格 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip")
    int32 BuyPrice = 100;

    /** 出售价格 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip")
    int32 SellPrice = 50;

    /** 装备属性词条列表 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|Stats")
    TArray<FESItemStat> StatList;

    /** 装备持续生效的GE */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|GAS")
    TSubclassOf<UGameplayEffect> EquipEffect;

    /** 装备授予的技能 */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "ES|Equip|GAS")
    TArray<FESItemAbilityInfo> GrantAbilities;
};

// ... (在文件末尾，#include 之前)

// ==================== 【存档系统】单个词条的存档数据 ====================
USTRUCT(BlueprintType)
struct FEntrySaveData
{
    GENERATED_BODY()

    /** 词条库ID */
    UPROPERTY()
    FName SourceEntryID;

    /** 词条类型 */
    UPROPERTY()
    EESEntryType EntryType;

    /** 词条等级 */
    UPROPERTY()
    int32 EntryLevel;

    /** 是否已解锁 */
    UPROPERTY()
    bool bIsUnlocked;

    /** 最终数值 */
    UPROPERTY()
    float FinalValue;

    /** 是否为百分比 */
    UPROPERTY()
    bool bIsPercentage;

    /** 目标属性 */
    UPROPERTY()
    EESStatType TargetStat;

    // 默认构造函数
    FEntrySaveData() {}

    // 从运行时词条转换
    explicit FEntrySaveData(const FESRuntimeEntry& RuntimeEntry)
    {
        SourceEntryID = RuntimeEntry.SourceEntryID;
        EntryType = RuntimeEntry.EntryType;
        EntryLevel = RuntimeEntry.EntryLevel;
        bIsUnlocked = RuntimeEntry.bIsUnlocked;
        FinalValue = RuntimeEntry.FinalValue;
        bIsPercentage = RuntimeEntry.bIsPercentage;
        TargetStat = RuntimeEntry.TargetStat;
    }

    // 转换回运行时词条
    FESRuntimeEntry ToRuntimeEntry() const
    {
        FESRuntimeEntry Out;
        Out.SourceEntryID = SourceEntryID;
        Out.EntryType = EntryType;
        Out.EntryLevel = EntryLevel;
        Out.bIsUnlocked = bIsUnlocked;
        Out.FinalValue = FinalValue;
        Out.bIsPercentage = bIsPercentage;
        Out.TargetStat = TargetStat;
        return Out;
    }
};

// ==================== 【存档系统】单个物品的存档数据 ====================
USTRUCT(BlueprintType)
struct FItemSaveData
{
    GENERATED_BODY()

    /** 物品唯一GUID（用于区分不同物品） */
    UPROPERTY()
    FGuid ItemGUID;

    /** 物品名称 */
    UPROPERTY()
    FText Name;

    /** 物品描述 */
    UPROPERTY()
    FText Description;

    /** 物品图标路径 */
    UPROPERTY()
    TSoftObjectPtr<UTexture2D> Icon;

    /** 物品类型 */
    UPROPERTY()
    EESItemType Type;

    /** 物品品质 */
    UPROPERTY()
    EESItemQuality Quality;

    /** 买价 */
    UPROPERTY()
    int32 BuyPrice;

    /** 卖价 */
    UPROPERTY()
    int32 SellPrice;

    /** 存档时所在的槽位索引（背包/仓库用） */
    UPROPERTY()
    int32 SavedSlotIndex = -1;

    // ==================== 装备专属数据 ====================
    /** 是否为装备 */
    UPROPERTY()
    bool bIsEquipItem;

    /** 主词条数据 */
    UPROPERTY()
    FEntrySaveData MainEntryData;

    /** 副词条数据数组 */
    UPROPERTY()
    TArray<FEntrySaveData> SubEntriesData;

    // 默认构造函数
    FItemSaveData() : BuyPrice(0), SellPrice(0), SavedSlotIndex(-1), bIsEquipItem(false) {}

    // 从运行时物品转换
    explicit FItemSaveData(UESItemBase* Item);
};