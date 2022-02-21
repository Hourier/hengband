﻿#include "object-enchant/weapon/abstract-weapon-enchanter.h"
#include "object-enchant/object-boost.h"
#include "object/tval-types.h"
#include "sv-definition/sv-weapon-types.h"
#include "system/object-type-definition.h"

AbstractWeaponEnchanter::AbstractWeaponEnchanter(ObjectType *o_ptr, DEPTH level, int power)
    : o_ptr(o_ptr)
    , level(level)
    , power(power)
{
    this->decide_skip();
    if (this->should_skip) {
        return;
    }

    auto tohit1 = static_cast<short>(randint1(5) + m_bonus(5, this->level));
    auto todam1 = static_cast<short>(randint1(5) + m_bonus(5, this->level));
    auto tohit2 = static_cast<short>(m_bonus(10, this->level));
    auto todam2 = static_cast<short>(m_bonus(10, this->level));

    if ((this->o_ptr->tval == ItemKindType::BOLT) || (this->o_ptr->tval == ItemKindType::ARROW) || (this->o_ptr->tval == ItemKindType::SHOT)) {
        tohit2 = (tohit2 + 1) / 2;
        todam2 = (todam2 + 1) / 2;
    }

    if (this->power > 0) {
        this->o_ptr->to_h += tohit1;
        this->o_ptr->to_d += todam1;
        if (this->power > 1) {
            this->o_ptr->to_h += tohit2;
            this->o_ptr->to_d += todam2;
        }

        return;
    }

    if (this->power < 0) {
        this->o_ptr->to_h -= tohit1;
        this->o_ptr->to_d -= todam1;
        if (this->power < -1) {
            this->o_ptr->to_h -= tohit2;
            this->o_ptr->to_d -= todam2;
        }

        if (this->o_ptr->to_h + this->o_ptr->to_d < 0) {
            this->o_ptr->curse_flags.set(CurseTraitType::CURSED);
        }
    }
}

void AbstractWeaponEnchanter::decide_skip()
{
    this->should_skip = (this->o_ptr->tval == ItemKindType::SWORD) && (this->o_ptr->sval == SV_DIAMOND_EDGE);
    this->should_skip |= (this->o_ptr->tval == ItemKindType::SWORD) && (this->o_ptr->sval == SV_POISON_NEEDLE) && (this->power != 0);
    this->should_skip |= (this->o_ptr->tval == ItemKindType::POLEARM) && (this->o_ptr->sval == SV_DEATH_SCYTHE) && (this->power != 0);
    auto other_weapons_enchant = this->o_ptr->tval == ItemKindType::DIGGING;
    other_weapons_enchant |= this->o_ptr->tval == ItemKindType::HAFTED;
    other_weapons_enchant |= this->o_ptr->tval == ItemKindType::BOW;
    other_weapons_enchant |= this->o_ptr->tval == ItemKindType::SHOT;
    other_weapons_enchant |= this->o_ptr->tval == ItemKindType::ARROW;
    other_weapons_enchant |= this->o_ptr->tval == ItemKindType::BOLT;
    other_weapons_enchant &= this->power != 0;
    this->should_skip |= !other_weapons_enchant;
}
