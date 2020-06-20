﻿/*!
 * @brief モンスター情報のアップデート処理
 * @date 2020/03/08
 * @author Hourier
 */

#include "monster/monster-update.h"
#include "dungeon/dungeon.h"
#include "game-option/birth-options.h"
#include "game-option/disturbance-options.h"
#include "grid/grid.h"
#include "mind/drs-types.h"
#include "monster-race/monster-race.h"
#include "monster-race/race-flags1.h"
#include "monster-race/race-flags2.h"
#include "monster-race/race-flags3.h"
#include "monster-race/race-flags7.h"
#include "monster-race/race-indice-types.h"
#include "monster/monster-flag-types.h"
#include "monster/monster-info.h"
#include "monster/monster-status.h"
#include "monster/smart-learn-types.h"
#include "player/eldritch-horror.h"
#include "player/player-move.h"

/*!
 * @brief 騎乗中のモンスター情報を更新する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_idx モンスターID
 * @param oy 移動前の、モンスターのY座標
 * @param ox 移動前の、モンスターのX座標
 * @param ny 移動後の、モンスターのY座標
 * @param ox 移動後の、モンスターのX座標
 * @return アイテム等に影響を及ぼしたらTRUE
 */
bool update_riding_monster(player_type *target_ptr, turn_flags *turn_flags_ptr, MONSTER_IDX m_idx, POSITION oy, POSITION ox, POSITION ny, POSITION nx)
{
	monster_type *m_ptr = &target_ptr->current_floor_ptr->m_list[m_idx];
	grid_type *g_ptr;
	g_ptr = &target_ptr->current_floor_ptr->grid_array[ny][nx];
	monster_type *y_ptr;
	y_ptr = &target_ptr->current_floor_ptr->m_list[g_ptr->m_idx];
	if (turn_flags_ptr->is_riding_mon)
		return move_player_effect(target_ptr, ny, nx, MPE_DONT_PICKUP);

	target_ptr->current_floor_ptr->grid_array[oy][ox].m_idx = g_ptr->m_idx;
	if (g_ptr->m_idx)
	{
		y_ptr->fy = oy;
		y_ptr->fx = ox;
		update_monster(target_ptr, g_ptr->m_idx, TRUE);
	}

	g_ptr->m_idx = m_idx;
	m_ptr->fy = ny;
	m_ptr->fx = nx;
	update_monster(target_ptr, m_idx, TRUE);

	lite_spot(target_ptr, oy, ox);
	lite_spot(target_ptr, ny, nx);
	return TRUE;
}


/*!
 * @brief updateフィールドを更新する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @return なし
 */
void update_player_type(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_race *r_ptr)
{
	if (turn_flags_ptr->do_view)
	{
		target_ptr->update |= (PU_FLOW);
		target_ptr->window |= (PW_OVERHEAD | PW_DUNGEON);
	}

	if (turn_flags_ptr->do_move && ((r_ptr->flags7 & (RF7_SELF_LD_MASK | RF7_HAS_DARK_1 | RF7_HAS_DARK_2))
		|| ((r_ptr->flags7 & (RF7_HAS_LITE_1 | RF7_HAS_LITE_2)) && !target_ptr->phase_out)))
	{
		target_ptr->update |= (PU_MON_LITE);
	}
}


/*!
 * @brief モンスターのフラグを更新する
 * @param target_ptr プレーヤーへの参照ポインタ
 * @param turn_flags_ptr ターン経過処理フラグへの参照ポインタ
 * @param m_ptr モンスターへの参照ポインタ
 * @return なし
 */
void update_monster_race_flags(player_type *target_ptr, turn_flags *turn_flags_ptr, monster_type *m_ptr)
{
	monster_race *r_ptr = &r_info[m_ptr->r_idx];
	if (!is_original_ap_and_seen(target_ptr, m_ptr)) return;

	if (turn_flags_ptr->did_open_door) r_ptr->r_flags2 |= (RF2_OPEN_DOOR);
	if (turn_flags_ptr->did_bash_door) r_ptr->r_flags2 |= (RF2_BASH_DOOR);
	if (turn_flags_ptr->did_take_item) r_ptr->r_flags2 |= (RF2_TAKE_ITEM);
	if (turn_flags_ptr->did_kill_item) r_ptr->r_flags2 |= (RF2_KILL_ITEM);
	if (turn_flags_ptr->did_move_body) r_ptr->r_flags2 |= (RF2_MOVE_BODY);
	if (turn_flags_ptr->did_pass_wall) r_ptr->r_flags2 |= (RF2_PASS_WALL);
	if (turn_flags_ptr->did_kill_wall) r_ptr->r_flags2 |= (RF2_KILL_WALL);
}


/*!
 * @brief モンスターフラグの更新に基づき、モンスター表示を更新する
 * @param monster_race_idx モンスターID
 * @param window ウィンドウフラグ
 * @param old_race_flags_ptr モンスターフラグへの参照ポインタ
 * @return なし
 */
void update_player_window(player_type *target_ptr, old_race_flags *old_race_flags_ptr)
{
	monster_race *r_ptr;
	r_ptr = &r_info[target_ptr->monster_race_idx];
	if ((old_race_flags_ptr->old_r_flags1 != r_ptr->r_flags1) ||
		(old_race_flags_ptr->old_r_flags2 != r_ptr->r_flags2) ||
		(old_race_flags_ptr->old_r_flags3 != r_ptr->r_flags3) ||
		(old_race_flags_ptr->old_r_flags4 != r_ptr->r_flags4) ||
		(old_race_flags_ptr->old_r_flags5 != r_ptr->r_flags5) ||
		(old_race_flags_ptr->old_r_flags6 != r_ptr->r_flags6) ||
		(old_race_flags_ptr->old_r_flagsr != r_ptr->r_flagsr) ||
		(old_race_flags_ptr->old_r_blows0 != r_ptr->r_blows[0]) ||
		(old_race_flags_ptr->old_r_blows1 != r_ptr->r_blows[1]) ||
		(old_race_flags_ptr->old_r_blows2 != r_ptr->r_blows[2]) ||
		(old_race_flags_ptr->old_r_blows3 != r_ptr->r_blows[3]) ||
		(old_race_flags_ptr->old_r_cast_spell != r_ptr->r_cast_spell))
	{
		target_ptr->window |= (PW_MONSTER);
	}
}

/*!
 * @brief モンスターの各情報を更新する / This function updates the monster record of the given monster
 * @param m_idx 更新するモンスター情報のID
 * @param full プレイヤーとの距離更新を行うならばtrue
 * @return なし
 */
void update_monster(player_type *subject_ptr, MONSTER_IDX m_idx, bool full)
{
    monster_type *m_ptr = &subject_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    bool do_disturb = disturb_move;

    POSITION fy = m_ptr->fy;
    POSITION fx = m_ptr->fx;

    bool flag = FALSE;
    bool easy = FALSE;
    bool in_darkness = (d_info[subject_ptr->dungeon_idx].flags1 & DF1_DARKNESS) && !subject_ptr->see_nocto;
    if (disturb_high) {
        monster_race *ap_r_ptr = &r_info[m_ptr->ap_r_idx];
        if (ap_r_ptr->r_tkills && ap_r_ptr->level >= subject_ptr->lev)
            do_disturb = TRUE;
    }

    POSITION distance;
    if (full) {
        int dy = (subject_ptr->y > fy) ? (subject_ptr->y - fy) : (fy - subject_ptr->y);
        int dx = (subject_ptr->x > fx) ? (subject_ptr->x - fx) : (fx - subject_ptr->x);

        distance = (dy > dx) ? (dy + (dx >> 1)) : (dx + (dy >> 1));
        if (distance > 255)
            distance = 255;
        if (!distance)
            distance = 1;

        m_ptr->cdis = distance;
    } else {
        distance = m_ptr->cdis;
    }

    if (m_ptr->mflag2 & (MFLAG2_MARK))
        flag = TRUE;

    if (distance <= (in_darkness ? MAX_SIGHT / 2 : MAX_SIGHT)) {
        if (!in_darkness || (distance <= MAX_SIGHT / 4)) {
            if (subject_ptr->special_defense & KATA_MUSOU) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image) {
                    if (r_ptr->flags2 & (RF2_SMART))
                        r_ptr->r_flags2 |= (RF2_SMART);
                    if (r_ptr->flags2 & (RF2_STUPID))
                        r_ptr->r_flags2 |= (RF2_STUPID);
                }
            } else if (subject_ptr->telepathy) {
                if (r_ptr->flags2 & (RF2_EMPTY_MIND)) {
                    if (is_original_ap(m_ptr) && !subject_ptr->image)
                        r_ptr->r_flags2 |= (RF2_EMPTY_MIND);
                } else if (r_ptr->flags2 & (RF2_WEIRD_MIND)) {
                    if ((m_idx % 10) == 5) {
                        flag = TRUE;
                        if (is_original_ap(m_ptr) && !subject_ptr->image) {
                            r_ptr->r_flags2 |= (RF2_WEIRD_MIND);
                            if (r_ptr->flags2 & (RF2_SMART))
                                r_ptr->r_flags2 |= (RF2_SMART);
                            if (r_ptr->flags2 & (RF2_STUPID))
                                r_ptr->r_flags2 |= (RF2_STUPID);
                        }
                    }
                } else {
                    flag = TRUE;
                    if (is_original_ap(m_ptr) && !subject_ptr->image) {
                        if (r_ptr->flags2 & (RF2_SMART))
                            r_ptr->r_flags2 |= (RF2_SMART);
                        if (r_ptr->flags2 & (RF2_STUPID))
                            r_ptr->r_flags2 |= (RF2_STUPID);
                    }
                }
            }

            if ((subject_ptr->esp_animal) && (r_ptr->flags3 & (RF3_ANIMAL))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_ANIMAL);
            }

            if ((subject_ptr->esp_undead) && (r_ptr->flags3 & (RF3_UNDEAD))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_UNDEAD);
            }

            if ((subject_ptr->esp_demon) && (r_ptr->flags3 & (RF3_DEMON))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_DEMON);
            }

            if ((subject_ptr->esp_orc) && (r_ptr->flags3 & (RF3_ORC))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_ORC);
            }

            if ((subject_ptr->esp_troll) && (r_ptr->flags3 & (RF3_TROLL))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_TROLL);
            }

            if ((subject_ptr->esp_giant) && (r_ptr->flags3 & (RF3_GIANT))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_GIANT);
            }

            if ((subject_ptr->esp_dragon) && (r_ptr->flags3 & (RF3_DRAGON))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_DRAGON);
            }

            if ((subject_ptr->esp_human) && (r_ptr->flags2 & (RF2_HUMAN))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags2 |= (RF2_HUMAN);
            }

            if ((subject_ptr->esp_evil) && (r_ptr->flags3 & (RF3_EVIL))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_EVIL);
            }

            if ((subject_ptr->esp_good) && (r_ptr->flags3 & (RF3_GOOD))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_GOOD);
            }

            if ((subject_ptr->esp_nonliving) && ((r_ptr->flags3 & (RF3_DEMON | RF3_UNDEAD | RF3_NONLIVING)) == RF3_NONLIVING)) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags3 |= (RF3_NONLIVING);
            }

            if ((subject_ptr->esp_unique) && (r_ptr->flags1 & (RF1_UNIQUE))) {
                flag = TRUE;
                if (is_original_ap(m_ptr) && !subject_ptr->image)
                    r_ptr->r_flags1 |= (RF1_UNIQUE);
            }
        }

        if (player_has_los_bold(subject_ptr, fy, fx) && !subject_ptr->blind) {
            bool do_invisible = FALSE;
            bool do_cold_blood = FALSE;

            if (subject_ptr->concent >= CONCENT_RADAR_THRESHOLD) {
                easy = flag = TRUE;
            }

            if (distance <= subject_ptr->see_infra) {
                if ((r_ptr->flags2 & (RF2_COLD_BLOOD | RF2_AURA_FIRE)) == RF2_COLD_BLOOD) {
                    do_cold_blood = TRUE;
                } else {
                    easy = flag = TRUE;
                }
            }

            if (player_can_see_bold(subject_ptr, fy, fx)) {
                if (r_ptr->flags2 & (RF2_INVISIBLE)) {
                    do_invisible = TRUE;
                    if (subject_ptr->see_inv) {
                        easy = flag = TRUE;
                    }
                } else {
                    easy = flag = TRUE;
                }
            }

            if (flag) {
                if (is_original_ap(m_ptr) && !subject_ptr->image) {
                    if (do_invisible)
                        r_ptr->r_flags2 |= (RF2_INVISIBLE);
                    if (do_cold_blood)
                        r_ptr->r_flags2 |= (RF2_COLD_BLOOD);
                }
            }
        }
    }

    /* The monster is now visible */
    if (flag) {
        if (!m_ptr->ml) {
            m_ptr->ml = TRUE;
            lite_spot(subject_ptr, fy, fx);

            if (subject_ptr->health_who == m_idx)
                subject_ptr->redraw |= (PR_HEALTH);
            if (subject_ptr->riding == m_idx)
                subject_ptr->redraw |= (PR_UHEALTH);

            if (!subject_ptr->image) {
                if ((m_ptr->ap_r_idx == MON_KAGE) && (r_info[MON_KAGE].r_sights < MAX_SHORT))
                    r_info[MON_KAGE].r_sights++;
                else if (is_original_ap(m_ptr) && (r_ptr->r_sights < MAX_SHORT))
                    r_ptr->r_sights++;
            }

            if (r_info[m_ptr->ap_r_idx].flags2 & RF2_ELDRITCH_HORROR) {
                sanity_blast(subject_ptr, m_ptr, FALSE);
            }

            if (disturb_near
                && (projectable(subject_ptr, m_ptr->fy, m_ptr->fx, subject_ptr->y, subject_ptr->x)
                    && projectable(subject_ptr, subject_ptr->y, subject_ptr->x, m_ptr->fy, m_ptr->fx))) {
                if (disturb_pets || is_hostile(m_ptr))
                    disturb(subject_ptr, TRUE, TRUE);
            }
        }
    }

    /* The monster is not visible */
    else {
        if (m_ptr->ml) {
            m_ptr->ml = FALSE;
            lite_spot(subject_ptr, fy, fx);

            if (subject_ptr->health_who == m_idx)
                subject_ptr->redraw |= (PR_HEALTH);
            if (subject_ptr->riding == m_idx)
                subject_ptr->redraw |= (PR_UHEALTH);
            if (do_disturb) {
                if (disturb_pets || is_hostile(m_ptr))
                    disturb(subject_ptr, TRUE, TRUE);
            }
        }
    }

    /* The monster is now easily visible */
    if (easy) {
        if (!(m_ptr->mflag & (MFLAG_VIEW))) {
            m_ptr->mflag |= (MFLAG_VIEW);
            if (do_disturb) {
                if (disturb_pets || is_hostile(m_ptr))
                    disturb(subject_ptr, TRUE, TRUE);
            }
        }

        return;
    }

    /* The monster is not easily visible */
    /* Change */
    if (!(m_ptr->mflag & (MFLAG_VIEW)))
        return;

    /* Mark as not easily visible */
    m_ptr->mflag &= ~(MFLAG_VIEW);

    if (do_disturb) {
        if (disturb_pets || is_hostile(m_ptr))
            disturb(subject_ptr, TRUE, TRUE);
    }
}

/*!
 * @param player_ptr プレーヤーへの参照ポインタ
 * @brief 単純に生存している全モンスターの更新処理を行う / This function simply updates all the (non-dead) monsters (see above).
 * @param full 距離更新を行うならtrue
 * @return なし
 */
void update_monsters(player_type *player_ptr, bool full)
{
    floor_type *floor_ptr = player_ptr->current_floor_ptr;
    for (MONSTER_IDX i = 1; i < floor_ptr->m_max; i++) {
        monster_type *m_ptr = &floor_ptr->m_list[i];
        if (!monster_is_valid(m_ptr))
            continue;
        update_monster(player_ptr, i, full);
    }
}

/*!
 * @brief SMART(適格に攻撃を行う)モンスターの学習状況を更新する / Learn about an "observed" resistance.
 * @param m_idx 更新を行う「モンスター情報ID
 * @param what 学習対象ID
 * @return なし
 */
void update_smart_learn(player_type *player_ptr, MONSTER_IDX m_idx, int what)
{
    monster_type *m_ptr = &player_ptr->current_floor_ptr->m_list[m_idx];
    monster_race *r_ptr = &r_info[m_ptr->r_idx];

    if (!smart_learn)
        return;
    if (r_ptr->flags2 & (RF2_STUPID))
        return;
    if (!(r_ptr->flags2 & (RF2_SMART)) && (randint0(100) < 50))
        return;

    switch (what) {
    case DRS_ACID:
        if (player_ptr->resist_acid)
            m_ptr->smart |= (SM_RES_ACID);
        if (is_oppose_acid(player_ptr))
            m_ptr->smart |= (SM_OPP_ACID);
        if (player_ptr->immune_acid)
            m_ptr->smart |= (SM_IMM_ACID);
        break;

    case DRS_ELEC:
        if (player_ptr->resist_elec)
            m_ptr->smart |= (SM_RES_ELEC);
        if (is_oppose_elec(player_ptr))
            m_ptr->smart |= (SM_OPP_ELEC);
        if (player_ptr->immune_elec)
            m_ptr->smart |= (SM_IMM_ELEC);
        break;

    case DRS_FIRE:
        if (player_ptr->resist_fire)
            m_ptr->smart |= (SM_RES_FIRE);
        if (is_oppose_fire(player_ptr))
            m_ptr->smart |= (SM_OPP_FIRE);
        if (player_ptr->immune_fire)
            m_ptr->smart |= (SM_IMM_FIRE);
        break;

    case DRS_COLD:
        if (player_ptr->resist_cold)
            m_ptr->smart |= (SM_RES_COLD);
        if (is_oppose_cold(player_ptr))
            m_ptr->smart |= (SM_OPP_COLD);
        if (player_ptr->immune_cold)
            m_ptr->smart |= (SM_IMM_COLD);
        break;

    case DRS_POIS:
        if (player_ptr->resist_pois)
            m_ptr->smart |= (SM_RES_POIS);
        if (is_oppose_pois(player_ptr))
            m_ptr->smart |= (SM_OPP_POIS);
        break;

    case DRS_NETH:
        if (player_ptr->resist_neth)
            m_ptr->smart |= (SM_RES_NETH);
        break;

    case DRS_LITE:
        if (player_ptr->resist_lite)
            m_ptr->smart |= (SM_RES_LITE);
        break;

    case DRS_DARK:
        if (player_ptr->resist_dark)
            m_ptr->smart |= (SM_RES_DARK);
        break;

    case DRS_FEAR:
        if (player_ptr->resist_fear)
            m_ptr->smart |= (SM_RES_FEAR);
        break;

    case DRS_CONF:
        if (player_ptr->resist_conf)
            m_ptr->smart |= (SM_RES_CONF);
        break;

    case DRS_CHAOS:
        if (player_ptr->resist_chaos)
            m_ptr->smart |= (SM_RES_CHAOS);
        break;

    case DRS_DISEN:
        if (player_ptr->resist_disen)
            m_ptr->smart |= (SM_RES_DISEN);
        break;

    case DRS_BLIND:
        if (player_ptr->resist_blind)
            m_ptr->smart |= (SM_RES_BLIND);
        break;

    case DRS_NEXUS:
        if (player_ptr->resist_nexus)
            m_ptr->smart |= (SM_RES_NEXUS);
        break;

    case DRS_SOUND:
        if (player_ptr->resist_sound)
            m_ptr->smart |= (SM_RES_SOUND);
        break;

    case DRS_SHARD:
        if (player_ptr->resist_shard)
            m_ptr->smart |= (SM_RES_SHARD);
        break;

    case DRS_FREE:
        if (player_ptr->free_act)
            m_ptr->smart |= (SM_IMM_FREE);
        break;

    case DRS_MANA:
        if (!player_ptr->msp)
            m_ptr->smart |= (SM_IMM_MANA);
        break;

    case DRS_REFLECT:
        if (player_ptr->reflect)
            m_ptr->smart |= (SM_IMM_REFLECT);
        break;
    }
}
