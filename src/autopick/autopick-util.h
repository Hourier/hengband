#pragma once

#include "autopick/autopick-methods-table.h"
#include "system/angband.h"
#include "util/flag-group.h"
#include <string>
#include <vector>

constexpr auto MAX_LINELEN = 1024;

enum class PickType {
    DEFAULT = 0, //!< picktype.prf
    WITH_PNAME = 1, //!< picktype-<playername>.prf
};

/*!
 * @struct autopick_type
 * @brief 自動拾い/破壊設定データの構造体 / A structure type for entry of auto-picker/destroyer
 */
struct autopick_type {
    std::string name = ""; /*!< 自動拾い/破壊定義の名称一致基準 / Items which have 'name' as part of its name match */
    std::string insc = ""; /*!< 対象となったアイテムに自動で刻む内容 / Items will be auto-inscribed as 'insc' */
    BIT_FLAGS flags[2]{}; /*!< キーワードに関する汎用的な条件フラグ / Misc. keyword to be matched */
    EnumClassFlagGroup<AutopickMethod> action{}; /*!< 対象のアイテムを拾う/破壊/放置するかの指定フラグ / Auto-pickup or Destroy or Leave items */
    byte dice = 0; /*!< 武器のダイス値基準値 / Weapons which have more than 'dice' dice match */
    byte bonus = 0; /*!< アイテムのボーナス基準値 / Items which have more than 'bonus' magical bonus match */
    bool has(int flag) const;
    void add(int flag);
    void remove(int flag);
};

class ItemEntity;

/*
 *  List for auto-picker/destroyer entries
 */
extern std::vector<autopick_type> autopick_list;
extern ItemEntity autopick_last_destroyed_object;

bool auto_inscribe_item(ItemEntity &item, int idx);
