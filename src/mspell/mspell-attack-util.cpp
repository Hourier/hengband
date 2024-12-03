#include "mspell/mspell-attack-util.h"
#include "system/floor-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"

msa_type::msa_type(PlayerType *player_ptr, MONSTER_IDX m_idx)
    : m_idx(m_idx)
    , m_ptr(&player_ptr->current_floor_ptr->m_list[m_idx])
    , x(player_ptr->x)
    , y(player_ptr->y)
    , do_spell(DO_SPELL_NONE)
    , thrown_spell(MonsterAbilityType::MAX)
{
    this->r_ptr = &this->m_ptr->get_monrace();
    this->no_inate = !evaluate_percent(this->r_ptr->freq_spell * 2);
    this->ability_flags = this->r_ptr->ability_flags;
}

Pos2D msa_type::get_position() const
{
    return Pos2D(this->y, this->x);
}
