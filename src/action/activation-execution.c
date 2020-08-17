#include "action/activation-execution.h"
#include "action/action-limited.h"
#include "artifact/artifact-info.h"
#include "core/window-redrawer.h"
#include "effect/spells-effect-util.h"
#include "floor/geometry.h"
#include "game-option/disturbance-options.h"
#include "game-option/input-options.h"
#include "main/sound-definitions-table.h"
#include "main/sound-of-music.h"
#include "monster-floor/monster-generator.h"
#include "monster-floor/place-monster-types.h"
#include "monster-race/monster-race.h"
#include "monster/monster-info.h"
#include "monster/monster-util.h"
#include "object-enchant/object-ego.h"
#include "object-hook/hook-enchant.h"
#include "object/object-info.h"
#include "object/object-kind.h"
#include "racial/racial-android.h"
#include "spell-kind/spells-launcher.h"
#include "spell-kind/spells-teleport.h"
#include "spell-realm/spells-hex.h"
#include "spell/spell-types.h"
#include "sv-definition/sv-lite-types.h"
#include "system/artifact-type-definition.h"
#include "system/floor-type-definition.h"
#include "system/monster-type-definition.h"
#include "system/object-type-definition.h"
#include "target/target-getter.h"
#include "term/screen-processor.h"
#include "util/quarks.h"
#include "util/sort.h"
#include "view/display-messages.h"
#include "world/world.h"

/*!
 * @brief 装備を発動するコマンドのサブルーチン /
 * Activate a wielded object.  Wielded objects never stack.
 * And even if they did, activatable objects never stack.
 * @param item 発動するオブジェクトの所持品ID
 * @return なし
 * @details
 * <pre>
 * Currently, only (some) artifacts, and Dragon Scale Mail, can be activated.
 * But one could, for example, easily make an activatable "Ring of Plasma".
 * Note that it always takes a turn to activate an artifact, even if
 * the user hits "escape" at the "direction" prompt.
 * </pre>
 */
void exe_activate(player_type *user_ptr, INVENTORY_IDX item)
{
    DIRECTION dir;
    bool success;
    object_type *o_ptr = ref_item(user_ptr, item);
    take_turn(user_ptr, 100);
    DEPTH lev = k_info[o_ptr->k_idx].level;

    if (object_is_fixed_artifact(o_ptr)) {
        lev = a_info[o_ptr->name1].level;
    } else if (object_is_random_artifact(o_ptr)) {
        const activation_type *const act_ptr = find_activation_info(user_ptr, o_ptr);
        if (act_ptr)
            lev = act_ptr->level;
    } else if (((o_ptr->tval == TV_RING) || (o_ptr->tval == TV_AMULET)) && o_ptr->name2)
        lev = e_info[o_ptr->name2].level;

    int chance = user_ptr->skill_dev;
    if (user_ptr->confused)
        chance = chance / 2;

    int fail = lev + 5;
    if (chance > fail)
        fail -= (chance - fail) * 2;
    else
        chance -= (fail - chance) * 2;

    if (fail < USE_DEVICE)
        fail = USE_DEVICE;

    if (chance < USE_DEVICE)
        chance = USE_DEVICE;

    if (cmd_limit_time_walk(user_ptr))
        return;

    if (user_ptr->pclass == CLASS_BERSERKER)
        success = FALSE;
    else if (chance > fail) {
        if (randint0(chance * 2) < fail)
            success = FALSE;
        else
            success = TRUE;
    } else {
        if (randint0(fail * 2) < chance)
            success = TRUE;
        else
            success = FALSE;
    }

    if (!success) {
        if (flush_failure)
            flush();

        msg_print(_("うまく始動させることができなかった。", "You failed to activate it properly."));
        sound(SOUND_FAIL);
        return;
    }

    if (o_ptr->timeout) {
        msg_print(_("それは微かに音を立て、輝き、消えた...", "It whines, glows and fades..."));
        return;
    }

    if (!o_ptr->xtra4 && (o_ptr->tval == TV_FLASK) && ((o_ptr->sval == SV_LITE_TORCH) || (o_ptr->sval == SV_LITE_LANTERN))) {
        msg_print(_("燃料がない。", "It has no fuel."));
        free_turn(user_ptr);
        return;
    }

    msg_print(_("始動させた...", "You activate it..."));
    sound(SOUND_ZAP);
    if (activation_index(user_ptr, o_ptr)) {
        (void)activate_artifact(user_ptr, o_ptr);
        user_ptr->window |= PW_INVEN | PW_EQUIP;
        return;
    } else if (o_ptr->tval == TV_WHISTLE) {
        if (music_singing_any(user_ptr))
            stop_singing(user_ptr);
        if (hex_spelling_any(user_ptr))
            stop_hex_spell_all(user_ptr);

        MONSTER_IDX pet_ctr;
        MONSTER_IDX *who;
        int max_pet = 0;
        C_MAKE(who, current_world_ptr->max_m_idx, MONSTER_IDX);
        for (pet_ctr = user_ptr->current_floor_ptr->m_max - 1; pet_ctr >= 1; pet_ctr--)
            if (is_pet(&user_ptr->current_floor_ptr->m_list[pet_ctr]) && (user_ptr->riding != pet_ctr))
                who[max_pet++] = pet_ctr;

        u16b dummy_why;
        ang_sort(user_ptr, who, &dummy_why, max_pet, ang_sort_comp_pet, ang_sort_swap_hook);
        for (MONSTER_IDX i = 0; i < max_pet; i++) {
            pet_ctr = who[i];
            teleport_monster_to(user_ptr, pet_ctr, user_ptr->y, user_ptr->x, 100, TELEPORT_PASSIVE);
        }

        C_KILL(who, current_world_ptr->max_m_idx, MONSTER_IDX);
        o_ptr->timeout = 100 + randint1(100);
        return;
    } else if (o_ptr->tval == TV_CAPTURE) {
        if (!o_ptr->pval) {
            bool old_target_pet = target_pet;
            target_pet = TRUE;
            if (!get_aim_dir(user_ptr, &dir)) {
                target_pet = old_target_pet;
                return;
            }

            target_pet = old_target_pet;
            if (fire_ball(user_ptr, GF_CAPTURE, dir, 0, 0)) {
                o_ptr->pval = (PARAMETER_VALUE)cap_mon;
                o_ptr->xtra3 = (XTRA8)cap_mspeed;
                o_ptr->xtra4 = (XTRA16)cap_hp;
                o_ptr->xtra5 = (XTRA16)cap_maxhp;
                if (cap_nickname) {
                    concptr t;
                    char *s;
                    char buf[80] = "";
                    if (o_ptr->inscription)
                        strcpy(buf, quark_str(o_ptr->inscription));

                    s = buf;
                    for (s = buf; *s && (*s != '#'); s++) {
#ifdef JP
                        if (iskanji(*s))
                            s++;
#endif
                    }

                    *s = '#';
                    s++;
#ifdef JP
#else
                    *s++ = '\'';
#endif
                    t = quark_str(cap_nickname);
                    while (*t) {
                        *s = *t;
                        s++;
                        t++;
                    }
#ifdef JP
#else
                    *s++ = '\'';
#endif
                    *s = '\0';
                    o_ptr->inscription = quark_add(buf);
                }
            }
        } else {
            success = FALSE;
            if (!get_direction(user_ptr, &dir, FALSE, FALSE))
                return;

            if (monster_can_enter(user_ptr, user_ptr->y + ddy[dir], user_ptr->x + ddx[dir], &r_info[o_ptr->pval], 0)) {
                if (place_monster_aux(user_ptr, 0, user_ptr->y + ddy[dir], user_ptr->x + ddx[dir], o_ptr->pval, PM_FORCE_PET | PM_NO_KAGE)) {
                    if (o_ptr->xtra3)
                        user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].mspeed = o_ptr->xtra3;

                    if (o_ptr->xtra5)
                        user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].max_maxhp = o_ptr->xtra5;

                    if (o_ptr->xtra4)
                        user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].hp = o_ptr->xtra4;

                    user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].maxhp = user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].max_maxhp;
                    if (o_ptr->inscription) {
                        char buf[80];
#ifdef JP
#else
                        bool quote = FALSE;
#endif
                        concptr t = quark_str(o_ptr->inscription);
                        for (t = quark_str(o_ptr->inscription); *t && (*t != '#'); t++) {
#ifdef JP
                            if (iskanji(*t))
                                t++;
#endif
                        }

                        if (*t) {
                            char *s = buf;
                            t++;
#ifdef JP
#else
                            if (*t == '\'') {
                                t++;
                                quote = TRUE;
                            }
#endif
                            while (*t) {
                                *s = *t;
                                t++;
                                s++;
                            }
#ifdef JP
#else
                            if (quote && *(s - 1) == '\'')
                                s--;
#endif
                            *s = '\0';
                            user_ptr->current_floor_ptr->m_list[hack_m_idx_ii].nickname = quark_add(buf);
                            t = quark_str(o_ptr->inscription);
                            s = buf;
                            while (*t && (*t != '#')) {
                                *s = *t;
                                t++;
                                s++;
                            }

                            *s = '\0';
                            o_ptr->inscription = quark_add(buf);
                        }
                    }

                    o_ptr->pval = 0;
                    o_ptr->xtra3 = 0;
                    o_ptr->xtra4 = 0;
                    o_ptr->xtra5 = 0;
                    success = TRUE;
                }
            }

            if (!success)
                msg_print(_("おっと、解放に失敗した。", "Oops.  You failed to release your pet."));
        }

        calc_android_exp(user_ptr);
        return;
    }

    msg_print(_("おっと、このアイテムは始動できない。", "Oops.  That object cannot be activated."));
}
