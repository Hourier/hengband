/*
 * @brief 装備の耐性を表示する
 * @date 2020/04/20
 * @author Hourier
 */

#include "knowledge/knowledge-inventory.h"
#include "core/show-file.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "inventory/inventory-slot-types.h"
#include "io-dump/dump-util.h"
#include "object-enchant/special-object-flags.h"
#include "object-enchant/tr-types.h"
#include "object-hook/hook-weapon.h"
#include "object/tval-types.h"
#include "perception/object-perception.h"
#include "store/store-util.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/item-entity.h"
#include "system/player-type-definition.h"
#include "util/angband-files.h"
#include "util/bit-flags-calculator.h"
#include "util/string-processor.h"
#include <sstream>
#include <string>

namespace {
const std::string INVENTORY_RESISTANCE_LABELS = _(
    "                               酸電火冷毒閃暗破轟獄因沌劣 盲恐乱麻視経感遅活浮\n",
    "                               AcElFiCoPoLiDkShSoNtNxCaDi BlFeCfFaSiHlEpSdRgLv\n");
constexpr auto HAS_FLAG_STRENGTH = _("＋", "+ ");
constexpr auto HAS_NO_FLAG_STRENGTH = _("・", ". ");
}

/*!
 * @brief 4元素耐性を表示する
 * @param immunity 4元素耐性の種類 (二重？)
 * @param resistance 4元素耐性
 * @param flags 耐性配列へのポインタ
 * @param fff 一時ファイルへのポインタ
 */
static void print_im_or_res_flag(tr_type immunity, tr_type resistance, const TrFlags &flags, FILE *fff)
{
    constexpr auto immunity_flag_strength = _("＊", "* ");
    fputs(flags.has(immunity) ? immunity_flag_strength : (flags.has(resistance) ? HAS_FLAG_STRENGTH : HAS_NO_FLAG_STRENGTH), fff);
}

/*!
 * @brief 4元素以外の耐性を表示する
 * @param tr 耐性
 * @param flags 耐性配列へのポインタ
 * @param fff 一時ファイルへのポインタ
 */
static void print_flag(tr_type tr, const TrFlags &flags, FILE *fff)
{
    fputs(flags.has(tr) ? HAS_FLAG_STRENGTH : HAS_NO_FLAG_STRENGTH, fff);
}

/*!
 * @brief 鑑定済アイテムの耐性を表示する
 * @param item アイテムへの参照
 * @param fff 一時ファイルへの参照ポインタ
 * @todo ここの関数から表示用の関数に移したい
 */
static void display_identified_resistances_flag(const ItemEntity &item, FILE *fff)
{
    auto flags = item.get_flags_known();

    print_im_or_res_flag(TR_IM_ACID, TR_RES_ACID, flags, fff);
    print_im_or_res_flag(TR_IM_ELEC, TR_RES_ELEC, flags, fff);
    print_im_or_res_flag(TR_IM_FIRE, TR_RES_FIRE, flags, fff);
    print_im_or_res_flag(TR_IM_COLD, TR_RES_COLD, flags, fff);
    print_flag(TR_RES_POIS, flags, fff);
    print_flag(TR_RES_LITE, flags, fff);
    print_flag(TR_RES_DARK, flags, fff);
    print_flag(TR_RES_SHARDS, flags, fff);
    print_flag(TR_RES_SOUND, flags, fff);
    print_flag(TR_RES_NETHER, flags, fff);
    print_flag(TR_RES_NEXUS, flags, fff);
    print_flag(TR_RES_CHAOS, flags, fff);
    print_flag(TR_RES_DISEN, flags, fff);

    fputs(" ", fff);

    print_flag(TR_RES_BLIND, flags, fff);
    print_flag(TR_RES_FEAR, flags, fff);
    print_flag(TR_RES_CONF, flags, fff);
    print_flag(TR_FREE_ACT, flags, fff);
    print_flag(TR_SEE_INVIS, flags, fff);
    print_flag(TR_HOLD_EXP, flags, fff);
    print_flag(TR_TELEPATHY, flags, fff);
    print_flag(TR_SLOW_DIGEST, flags, fff);
    print_flag(TR_REGEN, flags, fff);
    print_flag(TR_LEVITATION, flags, fff);

    fputc('\n', fff);
}

/*!
 * @brief アイテム1つ当たりの耐性を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param fff 一時ファイルへの参照ポインタ
 * @param item アイテムへの参照
 * @param where アイテムの場所 (手持ち、家等) を示す文字列への参照ポインタ
 * @details 28文字ちょうどになるまで右側をスペースでパディングする
 */
static void do_cmd_knowledge_inventory_aux(PlayerType *player_ptr, FILE *fff, const ItemEntity &item, std::string_view where)
{
    constexpr auto max_item_length = 26;
    std::stringstream ss;
    ss << describe_flavor(player_ptr, item, OD_NAME_ONLY, max_item_length);
    const int item_length = ss.tellp();
    constexpr auto max_display_length = 28;
    for (auto i = item_length; i < max_display_length; i++) {
        ss << ' ';
    }

    fmt::print(fff, "{} {}", where, ss.str());
    if (!item.is_fully_known()) {
        fputs(_("-------不明--------------- -------不明---------\n", "-------unknown------------ -------unknown------\n"), fff);
        return;
    }

    display_identified_resistances_flag(item, fff);
}

/*!
 * @brief 9行おきにラベルを追加する
 * @param label_number 現在の行数
 * @param fff 一時ファイルへの参照ポインタ
 */
static int add_res_label(int label_number_initial, FILE *fff)
{
    auto label_number = label_number_initial + 1;
    if (label_number == 9) {
        label_number = 0;
        fputs(INVENTORY_RESISTANCE_LABELS.data(), fff);
    }

    return label_number;
}

/*!
 * @brief 9行ごとに行数をリセットする
 * @param label_number 現在の行数
 * @param fff 一時ファイルへの参照ポインタ
 */
static void reset_label_number(int label_number, FILE *fff)
{
    if (label_number == 0) {
        return;
    }

    for (auto i = label_number; label_number < 9; i++) {
        fputc('\n', fff);
    }

    fputs(INVENTORY_RESISTANCE_LABELS.data(), fff);
}

/*!
 * 装備中のアイテムについて、耐性を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param tval アイテム主分類番号
 * @param label_number 現在の行数
 * @param fff ファイルへの参照ポインタ
 * @return 画面表示後の行数
 */
static int show_wearing_equipment_resistances(PlayerType *player_ptr, ItemKindType tval, int label_number_initial, FILE *fff)
{
    auto label_number = label_number_initial;
    for (short i = INVEN_MAIN_HAND; i < INVEN_TOTAL; i++) {
        const auto &item = *player_ptr->inventory[i];
        if (!item.has_knowledge(tval)) {
            continue;
        }

        do_cmd_knowledge_inventory_aux(player_ptr, fff, item, _("装", "E "));
        label_number = add_res_label(label_number, fff);
    }

    return label_number;
}

/*!
 * 手持ち中のアイテムについて、耐性を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param tval アイテム主分類番号
 * @param label_number 現在の行数
 * @param fff ファイルへの参照ポインタ
 * @return 画面表示後の行数
 */
static int show_holding_equipment_resistances(PlayerType *player_ptr, ItemKindType tval, int label_number_initial, FILE *fff)
{
    auto label_number = label_number_initial;
    for (short i = 0; i < INVEN_PACK; i++) {
        const auto &item = *player_ptr->inventory[i];
        if (!item.has_knowledge(tval)) {
            continue;
        }

        do_cmd_knowledge_inventory_aux(player_ptr, fff, item, _("持", "I "));
        label_number = add_res_label(label_number, fff);
    }

    return label_number;
}

/*!
 * 我が家のアイテムについて、耐性を表示する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param tval アイテム主分類番号
 * @param label_number 現在の行数
 * @param fff ファイルへの参照ポインタ
 * @return 画面表示後の行数
 */
static int show_home_equipment_resistances(PlayerType *player_ptr, ItemKindType tval, int label_number_initial, FILE *fff)
{
    auto label_number = label_number_initial;
    const auto &store = towns_info[1].get_store(StoreSaleType::HOME);
    for (short i = 0; i < store.stock_num; i++) {
        const auto &item = *store.stock[i];
        if (!item.has_knowledge(tval)) {
            continue;
        }

        do_cmd_knowledge_inventory_aux(player_ptr, fff, item, _("家", "H "));
        label_number = add_res_label(label_number, fff);
    }

    return label_number;
}

/*
 * @brief Display *ID* ed weapons/armors's resistances
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_knowledge_inventory(PlayerType *player_ptr)
{
    FILE *fff = nullptr;
    GAME_TEXT file_name[FILE_NAME_SIZE];
    if (!open_temporary_file(&fff, file_name)) {
        return;
    }

    fputs(INVENTORY_RESISTANCE_LABELS.data(), fff);
    auto label_number = 0;
    for (auto tval : TV_WEARABLE_RANGE) {
        reset_label_number(label_number, fff);
        label_number = show_wearing_equipment_resistances(player_ptr, tval, 0, fff);
        label_number = show_holding_equipment_resistances(player_ptr, tval, label_number, fff);
        label_number = show_home_equipment_resistances(player_ptr, tval, label_number, fff);
    }

    angband_fclose(fff);
    FileDisplayer(player_ptr->name).display(true, file_name, 0, 0, _("*鑑定*済み武器/防具の耐性リスト", "Resistances of *identified* equipment"));
    fd_kill(file_name);
}
