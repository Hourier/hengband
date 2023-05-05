﻿#include "core/disturbance.h"
#include "action/travel-execution.h"
#include "core/player-redraw-types.h"
#include "game-option/disturbance-options.h"
#include "game-option/map-screen-options.h"
#include "io/input-key-requester.h"
#include "player/attack-defense-types.h"
#include "status/action-setter.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "target/target-checker.h"
#include "term/screen-processor.h"

/*
 * Something has happened to disturb the player.
 * The first arg indicates a major disturbance, which affects search.
 * The second arg is currently unused, but could induce output flush.
 * All disturbance cancels repeated commands, resting, and running.
 */
void disturb(PlayerType *player_ptr, bool stop_search, bool stop_travel)
{
    auto &rfu = RedrawingFlagsUpdater::get_instance();
    if (command_rep) {
        command_rep = 0;
        player_ptr->redraw |= PR_ACTION;
    }

    if ((player_ptr->action == ACTION_REST) || (player_ptr->action == ACTION_FISH) || (stop_search && (player_ptr->action == ACTION_SEARCH))) {
        set_action(player_ptr, ACTION_NONE);
    }

    if (player_ptr->running) {
        player_ptr->running = 0;
        if (center_player && !center_running) {
            verify_panel(player_ptr);
        }

        const auto &flags = EnumClassFlagGroup<StatusRedrawingFlag>{
            StatusRedrawingFlag::TORCH,
            StatusRedrawingFlag::FLOW,
        };
        rfu.set_flags(flags);
    }

    if (stop_travel) {
        travel.run = 0;
        if (center_player && !center_running) {
            verify_panel(player_ptr);
        }

        rfu.set_flag(StatusRedrawingFlag::TORCH);
    }

    if (flush_disturb) {
        flush();
    }
}
