/*!
 * @file sound-definitions-table.cpp
 * @brief 設定ファイル用の効果音名定義
 */

#include "main/sound-definitions-table.h"

const std::map<SoundKind, std::string> sound_names = {
    { SoundKind::HIT, "hit" },
    { SoundKind::ENEMY_HIT, "enemy_hit" },
    { SoundKind::MISS, "miss" },
    { SoundKind::ATTACK_FAILED, "attack_failed" },
    { SoundKind::FLEE, "flee" },
    { SoundKind::DROP, "drop" },
    { SoundKind::KILL, "kill" },
    { SoundKind::LEVEL, "level" },
    { SoundKind::DEATH, "death" },
    { SoundKind::STUDY, "study" },
    { SoundKind::TELEPORT, "teleport" },
    { SoundKind::SHOOT, "shoot" },
    { SoundKind::SHOOT_HIT, "shoot_hit" },
    { SoundKind::QUAFF, "quaff" },
    { SoundKind::ZAP, "zap" },
    { SoundKind::WALK, "walk" },
    { SoundKind::TPOTHER, "tpother" },
    { SoundKind::HITWALL, "hitwall" },
    { SoundKind::EAT, "eat" },
    { SoundKind::STORE1, "store1" },
    { SoundKind::STORE2, "store2" },
    { SoundKind::STORE3, "store3" },
    { SoundKind::STORE4, "store4" },
    { SoundKind::DIG, "dig" },
    { SoundKind::DIG_THROUGH, "dig_through" },
    { SoundKind::OPENDOOR, "opendoor" },
    { SoundKind::SHUTDOOR, "shutdoor" },
    { SoundKind::TPLEVEL, "tplevel" },
    { SoundKind::SCROLL, "scroll" },
    { SoundKind::BUY, "buy" },
    { SoundKind::SELL, "sell" },
    { SoundKind::WARN, "warn" },
    { SoundKind::ROCKET, "rocket" },
    { SoundKind::N_KILL, "n_kill" },
    { SoundKind::U_KILL, "u_kill" },
    { SoundKind::QUEST, "quest" },
    { SoundKind::HEAL, "heal" },
    { SoundKind::X_HEAL, "x_heal" },
    { SoundKind::BITE, "bite" },
    { SoundKind::CLAW, "claw" },
    { SoundKind::M_SPELL, "m_spell" },
    { SoundKind::SUMMON, "summon" },
    { SoundKind::BREATH, "breath" },
    { SoundKind::BALL, "ball" },
    { SoundKind::M_HEAL, "m_heal" },
    { SoundKind::ATK_SPELL, "atkspell" },
    { SoundKind::EVIL, "evil" },
    { SoundKind::TOUCH, "touch" },
    { SoundKind::STING, "sting" },
    { SoundKind::CRUSH, "crush" },
    { SoundKind::SLIME, "slime" },
    { SoundKind::WAIL, "wail" },
    { SoundKind::WINNER, "winner" },
    { SoundKind::FIRE, "fire" },
    { SoundKind::ACID, "acid" },
    { SoundKind::ELEC, "elec" },
    { SoundKind::COLD, "cold" },
    { SoundKind::ILLEGAL, "illegal" },
    { SoundKind::FAIL, "fail" },
    { SoundKind::WAKEUP, "wakeup" },
    { SoundKind::INVULN, "invuln" },
    { SoundKind::FALL, "fall" },
    { SoundKind::PAIN, "pain" },
    { SoundKind::DESTITEM, "destitem" },
    { SoundKind::MOAN, "moan" },
    { SoundKind::SHOW, "show" },
    { SoundKind::UNUSED, "unused" },
    { SoundKind::EXPLODE, "explode" },
    { SoundKind::GLASS, "glass" },
    { SoundKind::REFLECT, "reflect" },
    { SoundKind::HUNGRY, "hungry" },
    { SoundKind::WEAK, "weak" },
    { SoundKind::FAINT, "faint" },
    { SoundKind::GOOD_HIT, "good_hit" },
    { SoundKind::GREAT_HIT, "great_hit" },
    { SoundKind::SUPERB_HIT, "superb_hit" },
    { SoundKind::STAR_GREAT_HIT, "star_great_hit" },
    { SoundKind::STAR_SUPERB_HIT, "star_superb_hit" },
    { SoundKind::GOUGE_HIT, "gouge_hit" },
    { SoundKind::MAIM_HIT, "maim_hit" },
    { SoundKind::CARVE_HIT, "carve_hit" },
    { SoundKind::CLEAVE_HIT, "cleave_hit" },
    { SoundKind::SMITE_HIT, "smite_hit" },
    { SoundKind::EVISCERATE_HIT, "eviscerate_hit" },
    { SoundKind::SHRED_HIT, "shred_hit" },
    { SoundKind::WIELD, "wield" },
    { SoundKind::TAKE_OFF, "take_off" },
    { SoundKind::TERRAIN_DAMAGE, "terrain_damage" },
    { SoundKind::DAMAGE_OVER_TIME, "damage_over_time" },
    { SoundKind::STAIRWAY, "stairway" },
};
