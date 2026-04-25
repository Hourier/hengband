#pragma once

#include <memory>

// Activation Execution.
class ItemEntity;
class PlayerType;
struct ae_type {
    int dir = 0;
    bool success = false;
    std::shared_ptr<ItemEntity> item;
    int lev;
    int chance = 0;
    int fail = 0;

    ae_type(PlayerType *player_ptr, short i_idx);

    void decide_activation_level();
};
