﻿#pragma once

#include "system/angband.h"

/* MONster-Attack-Player、地図のMAPと紛らわしいのでmonapとした */
typedef struct monap_type {
#ifdef JP
    int abbreviate; // 2回目以降の省略表現フラグ.
#endif
    MONSTER_IDX m_idx;
    monster_type *m_ptr;
    concptr act;
    int do_cut;
    int do_stun;
    bool touched;
    rbm_type method;
    bool explode;
    DEPTH rlev;
    GAME_TEXT m_name[MAX_NLEN];
    bool do_silly_attack;
    int d_dice;
    int d_side;
    object_type *o_ptr;
    bool obvious;
    HIT_POINT damage;
    rbe_type effect;
    bool blinked;
    GAME_TEXT o_name[MAX_NLEN];
    HIT_POINT get_damage;
    GAME_TEXT ddesc[80];
    ARMOUR_CLASS ac;
} monap_type;

monap_type *initialize_monap_type(player_type *target_ptr, monap_type *monap_ptr, MONSTER_IDX m_idx);
