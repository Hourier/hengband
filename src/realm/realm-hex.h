﻿#pragma once

#include "system/angband.h"
#include "spell/spells-util.h"

/* 1st book */
#define HEX_BLESS             0
#define HEX_CURE_LIGHT        1
#define HEX_DEMON_AURA        2
#define HEX_STINKING_MIST     3
#define HEX_XTRA_MIGHT        4
#define HEX_CURSE_WEAPON      5
#define HEX_DETECT_EVIL       6
#define HEX_PATIENCE          7
/* 2nd book */
#define HEX_ICE_ARMOR         8
#define HEX_CURE_SERIOUS      9
#define HEX_INHAIL           10
#define HEX_VAMP_MIST        11
#define HEX_RUNESWORD        12
#define HEX_CONFUSION        13
#define HEX_BUILDING         14
#define HEX_ANTI_TELE        15
/* 3rd book */
#define HEX_SHOCK_CLOAK      16
#define HEX_CURE_CRITICAL    17
#define HEX_RECHARGE         18
#define HEX_RAISE_DEAD       19
#define HEX_CURSE_ARMOUR     20
#define HEX_SHADOW_CLOAK     21
#define HEX_PAIN_TO_MANA     22
#define HEX_EYE_FOR_EYE      23
/* 4th book */
#define HEX_ANTI_MULTI       24
#define HEX_RESTORE          25
#define HEX_DRAIN_CURSE      26
#define HEX_VAMP_BLADE       27
#define HEX_STUN_MONSTERS    28
#define HEX_SHADOW_MOVE      29
#define HEX_ANTI_MAGIC       30
#define HEX_REVENGE          31

concptr do_hex_spell(player_type *caster_ptr, SPELL_IDX spell, spell_type mode);
