#include "mspell/improper-mspell-remover.h"
#include "game-option/birth-options.h"
#include "monster/smart-learn-types.h"
#include "mspell/element-resistance-checker.h"
#include "mspell/high-resistance-checker.h"
#include "mspell/smart-mspell-util.h"
#include "system/floor/floor-info.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"

static void add_cheat_remove_flags(PlayerType *player_ptr, msr_type *msr_ptr)
{
    if (!smart_cheat) {
        return;
    }

    add_cheat_remove_flags_element(player_ptr, msr_ptr);
    add_cheat_remove_flags_others(player_ptr, msr_ptr);
}

/*!
 * @brief モンスターの魔法一覧から戦術的に適さない魔法を除外する /
 * Remove the "bad" spells from a spell list
 * @param m_idx モンスターの構造体参照ポインタ
 * @param f4p モンスター魔法のフラグリスト1
 * @param f5p モンスター魔法のフラグリスト2
 * @param f6p モンスター魔法のフラグリスト3
 */
void remove_bad_spells(MONSTER_IDX m_idx, PlayerType *player_ptr, EnumClassFlagGroup<MonsterAbilityType> &ability_flags)
{
    msr_type tmp_msr(player_ptr, m_idx, ability_flags);
    auto *msr_ptr = &tmp_msr;
    if (msr_ptr->r_ptr->behavior_flags.has(MonsterBehaviorType::STUPID)) {
        return;
    }

    if (!smart_cheat && !smart_learn) {
        return;
    }

    auto &monster = player_ptr->current_floor_ptr->m_list[m_idx];
    if (smart_learn) {
        /* 時々学習情報を忘れる */
        if (one_in_(100)) {
            monster.smart.clear();
        }

        msr_ptr->smart_flags = monster.smart;
    }

    add_cheat_remove_flags(player_ptr, msr_ptr);
    if (msr_ptr->smart_flags.none()) {
        return;
    }

    check_element_resistance(msr_ptr);
    check_high_resistances(player_ptr, msr_ptr);
    ability_flags = msr_ptr->ability_flags;
}
