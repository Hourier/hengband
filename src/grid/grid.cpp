/*!
 * @brief グリッドの実装 / low level dungeon routines -BEN-
 * @date 2013/12/30
 * @author
 * Copyright (c) 1997 Ben Harrison, James E. Wilson, Robert A. Koeneke\n
 *\n
 * This software may be copied and distributed for educational, research,\n
 * and not for profit purposes provided that this copyright and statement\n
 * are included in all such copies.  Other copyrights may also apply.\n
 * \n
 * Support for Adam Bolt's tileset, lighting and transparency effects\n
 * by Robert Ruehlmann (rr9@angband.org)\n
 * \n
 * 2013 Deskull Doxygen向けのコメント整理\n
 */

#include "grid/grid.h"
#include "effect/effect-characteristics.h"
#include "effect/effect-processor.h"
#include "game-option/map-screen-options.h"
#include "grid/object-placer.h"
#include "io/screen-util.h"
#include "mind/mind-ninja.h" //!< @todo 相互依存、後で消す.
#include "monster-floor/monster-remover.h"
#include "monster/monster-info.h"
#include "monster/monster-update.h"
#include "player/player-status-flags.h"
#include "player/player-status.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/grid-flow.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/item-entity.h"
#include "system/monster-entity.h"
#include "system/player-type-definition.h"
#include "system/redrawing-flags-updater.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "term/gameterm.h"
#include "timed-effect/timed-effects.h"
#include "view/display-map.h"
#include "view/display-messages.h"
#include "window/main-window-util.h"
#include "world/world.h"
#include <queue>

bool GridTemplate::matches(const Grid &grid) const
{
    auto is_matched = this->info == grid.info;
    is_matched &= this->feat == grid.feat;
    is_matched &= this->mimic == grid.mimic;
    is_matched &= this->special == grid.special;
    return is_matched;
}

void set_terrain_id_to_grid(PlayerType *player_ptr, const Pos2D &pos, TerrainTag tag)
{
    set_terrain_id_to_grid(player_ptr, pos, TerrainList::get_instance().get_terrain_id(tag));
}

/*
 * Change the "feat" flag for a grid, and notice/redraw the grid
 */
void set_terrain_id_to_grid(PlayerType *player_ptr, const Pos2D &pos, short terrain_id)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);
    const auto &terrain = TerrainList::get_instance().get_terrain(terrain_id);
    const auto &dungeon = floor.get_dungeon_definition();
    if (!AngbandWorld::get_instance().character_dungeon) {
        grid.set_terrain_id(terrain_id);
        grid.set_terrain_id(TerrainTag::NONE, TerrainKind::MIMIC);
        if (terrain.flags.has(TerrainCharacteristics::GLOW) && dungeon.flags.has_not(DungeonFeatureType::DARKNESS)) {
            for (const auto &d : Direction::directions()) {
                const auto pos_neighbor = pos + d.vec();
                if (!floor.contains(pos_neighbor, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
                    continue;
                }

                floor.get_grid(pos_neighbor).info |= CAVE_GLOW;
            }
        }

        return;
    }

    const auto old_los = floor.has_terrain_characteristics(pos, TerrainCharacteristics::LOS);
    const auto old_mirror = grid.is_mirror();
    grid.set_terrain_id(terrain_id);
    grid.set_terrain_id(TerrainTag::NONE, TerrainKind::MIMIC);
    grid.info &= ~(CAVE_OBJECT);
    if (old_mirror && dungeon.flags.has(DungeonFeatureType::DARKNESS)) {
        grid.info &= ~(CAVE_GLOW);
        if (!view_torch_grids) {
            grid.info &= ~(CAVE_MARK);
        }

        update_local_illumination(player_ptr, pos);
    }

    if (terrain.flags.has_not(TerrainCharacteristics::REMEMBER)) {
        grid.info &= ~(CAVE_MARK);
    }

    if (grid.has_monster()) {
        update_monster(player_ptr, grid.m_idx, false);
    }

    note_spot(player_ptr, pos);
    lite_spot(player_ptr, pos);
    if (old_los ^ terrain.flags.has(TerrainCharacteristics::LOS)) {
        static constexpr auto flags = {
            StatusRecalculatingFlag::VIEW,
            StatusRecalculatingFlag::LITE,
            StatusRecalculatingFlag::MONSTER_LITE,
            StatusRecalculatingFlag::MONSTER_STATUSES,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);
    }

    if (terrain.flags.has_not(TerrainCharacteristics::GLOW) || dungeon.flags.has(DungeonFeatureType::DARKNESS)) {
        return;
    }

    for (const auto &d : Direction::directions()) {
        const auto pos_neighbor = pos + d.vec();
        if (!floor.contains(pos_neighbor, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
            continue;
        }

        auto &grid_neighbor = floor.get_grid(pos_neighbor);
        grid_neighbor.info |= CAVE_GLOW;
        if (grid_neighbor.is_view()) {
            if (grid_neighbor.has_monster()) {
                update_monster(player_ptr, grid_neighbor.m_idx, false);
            }

            note_spot(player_ptr, pos_neighbor);
            lite_spot(player_ptr, pos_neighbor);
        }

        update_local_illumination(player_ptr, pos_neighbor);
    }

    if (floor.get_grid(player_ptr->get_position()).info & CAVE_GLOW) {
        set_superstealth(player_ptr, false);
    }
}

/*!
 * @brief 新規フロアに入りたてのプレイヤーをランダムな場所に配置する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @return 配置に成功したらその座標、失敗したらnullopt
 */
tl::optional<Pos2D> new_player_spot(PlayerType *player_ptr)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    auto max_attempts = 10000;
    Pos2D pos(0, 0);
    while (max_attempts--) {
        pos.y = rand_range(1, floor.height - 2);
        pos.x = rand_range(1, floor.width - 2);
        const auto &grid = floor.get_grid(pos);
        if (grid.has_monster()) {
            continue;
        }

        if (floor.is_underground()) {
            const auto &terrain = grid.get_terrain();
            if (max_attempts > 5000) { /* Rule 1 */
                if (terrain.flags.has_not(TerrainCharacteristics::FLOOR)) {
                    continue;
                }
            } else { /* Rule 2 */
                if (terrain.flags.has_not(TerrainCharacteristics::MOVE)) {
                    continue;
                }

                if (terrain.flags.has(TerrainCharacteristics::HIT_TRAP)) {
                    continue;
                }
            }

            /* Refuse to start on anti-teleport grids in dungeon */
            if (terrain.flags.has_not(TerrainCharacteristics::TELEPORTABLE)) {
                continue;
            }
        }

        if (!player_can_enter(player_ptr, grid.feat, 0)) {
            continue;
        }

        if (!floor.contains(pos)) {
            continue;
        }

        /* Refuse to start on anti-teleport grids */
        if (grid.is_icky()) {
            continue;
        }

        break;
    }

    if (max_attempts < 1) { /* Should be -1, actually if we failed... */
        return tl::nullopt;
    }

    return pos;
}

/*!
 * @brief 指定された座標のマスが現在照らされているかを返す。 / Check for "local" illumination
 * @param y y座標
 * @param x x座標
 * @return 指定された座標に照明がかかっているならTRUEを返す。。
 */
bool check_local_illumination(PlayerType *player_ptr, POSITION y, POSITION x)
{
    const auto yy = (y < player_ptr->y) ? (y + 1) : (y > player_ptr->y) ? (y - 1)
                                                                        : y;
    const auto xx = (x < player_ptr->x) ? (x + 1) : (x > player_ptr->x) ? (x - 1)
                                                                        : x;
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid_yyxx = floor.grid_array[yy][xx];
    const auto &grid_yxx = floor.grid_array[y][xx];
    const auto &grid_yyx = floor.grid_array[yy][x];
    auto is_illuminated = grid_yyxx.has_los_terrain(TerrainKind::MIMIC) && (grid_yyxx.info & CAVE_GLOW);
    is_illuminated |= grid_yxx.has_los_terrain(TerrainKind::MIMIC) && (grid_yxx.info & CAVE_GLOW);
    is_illuminated |= grid_yyx.has_los_terrain(TerrainKind::MIMIC) && (grid_yyx.info & CAVE_GLOW);
    return is_illuminated;
}

/*!
 * @brief 対象座標のマスの照明状態を更新する
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 更新したいマスの座標
 */
static void update_local_illumination_aux(PlayerType *player_ptr, const Pos2D &pos)
{
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    if (!grid.has_los()) {
        return;
    }

    if (grid.has_monster()) {
        update_monster(player_ptr, grid.m_idx, false);
    }

    note_spot(player_ptr, pos);
    lite_spot(player_ptr, pos);
}

/*!
 * @brief 指定された座標の照明状態を更新する / Update "local" illumination
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param pos 視界先座標
 */
void update_local_illumination(PlayerType *player_ptr, const Pos2D &pos)
{
    if (!player_ptr->current_floor_ptr->contains(pos)) {
        return;
    }

    const auto p_pos = player_ptr->get_position();
    if ((pos.y != p_pos.y) && (pos.x != p_pos.x)) {
        const auto yy = (pos.y < p_pos.y) ? (pos.y - 1) : (pos.y + 1);
        const auto xx = (pos.x < p_pos.x) ? (pos.x - 1) : (pos.x + 1);
        update_local_illumination_aux(player_ptr, { yy, xx });
        update_local_illumination_aux(player_ptr, { pos.y, xx });
        update_local_illumination_aux(player_ptr, { yy, pos.x });
        return;
    }

    if (pos.x != p_pos.x) { //!< y == player_ptr->y
        const auto xx = (pos.x < p_pos.x) ? (pos.x - 1) : (pos.x + 1);
        int yy;
        for (auto i = -1; i <= 1; i++) {
            yy = pos.y + i;
            update_local_illumination_aux(player_ptr, { yy, xx });
        }

        yy = pos.y - 1;
        update_local_illumination_aux(player_ptr, { yy, pos.x });
        yy = pos.y + 1;
        update_local_illumination_aux(player_ptr, { yy, pos.x });
        return;
    }

    if (pos.y != p_pos.y) { //!< x == player_ptr->x
        const auto yy = (pos.y < player_ptr->y) ? (pos.y - 1) : (pos.y + 1);
        int xx;
        for (auto i = -1; i <= 1; i++) {
            xx = pos.x + i;
            update_local_illumination_aux(player_ptr, { yy, xx });
        }

        xx = pos.x - 1;
        update_local_illumination_aux(player_ptr, { pos.y, xx });
        xx = pos.x + 1;
        update_local_illumination_aux(player_ptr, { pos.y, xx });
    }

    if (p_pos != pos) {
        return;
    }

    for (const auto &d : Direction::directions_8()) {
        update_local_illumination_aux(player_ptr, pos + d.vec());
    }
}

/*!
 * @brief 指定された座標をプレイヤー収められていない状態かどうか / Returns true if the player's grid is dark
 * @return 視覚に収められていないならTRUEを返す
 * @details player_can_see_bold()関数の返り値の否定を返している。
 */
bool no_lite(PlayerType *player_ptr)
{
    return !player_can_see_bold(player_ptr, player_ptr->y, player_ptr->x);
}

/*
 * Place an attr/char pair at the given map coordinate, if legal.
 */
void print_rel(PlayerType *player_ptr, const DisplaySymbol &symbol, POSITION y, POSITION x)
{
    /* Only do "legal" locations */
    if (panel_contains(y, x)) {
        const auto color = get_monochrome_display_color(player_ptr).value_or(symbol.color);
        term_queue_bigchar(panel_col_of(x), y - panel_row_prt, { { color, symbol.character }, {} });
    }
}

void print_bolt_pict(PlayerType *player_ptr, POSITION y, POSITION x, POSITION ny, POSITION nx, AttributeType typ)
{
    const auto symbol = bolt_pict(y, x, ny, nx, typ);
    print_rel(player_ptr, symbol, ny, nx);
}

/*!
 * Memorize interesting viewable object/features in the given grid
 *
 * This function should only be called on "legal" grids.
 *
 * This function will memorize the object and/or feature in the given
 * grid, if they are (1) viewable and (2) interesting.  Note that all
 * objects are interesting, all terrain features except floors (and
 * invisible traps) are interesting, and floors (and invisible traps)
 * are interesting sometimes (depending on various options involving
 * the illumination of floor grids).
 *
 * The automatic memorization of all objects and non-floor terrain
 * features as soon as they are displayed allows incredible amounts
 * of optimization in various places, especially "map_info()".
 *
 * Note that the memorization of objects is completely separate from
 * the memorization of terrain features, preventing annoying floor
 * memorization when a detected object is picked up from a dark floor,
 * and object memorization when an object is dropped into a floor grid
 * which is memorized but out-of-sight.
 *
 * This function should be called every time the "memorization" of
 * a grid (or the object in a grid) is called into question, such
 * as when an object is created in a grid, when a terrain feature
 * "changes" from "floor" to "non-floor", when any grid becomes
 * "illuminated" or "viewable", and when a "floor" grid becomes
 * "torch-lit".
 *
 * Note the relatively efficient use of this function by the various
 * "update_view()" and "update_lite()" calls, to allow objects and
 * terrain features to be memorized (and drawn) whenever they become
 * viewable or illuminated in any way, but not when they "maintain"
 * or "lose" their previous viewability or illumination.
 *
 * Note the butchered "internal" version of "player_can_see_bold()",
 * optimized primarily for the most common cases, that is, for the
 * non-marked floor grids.
 */
void note_spot(PlayerType *player_ptr, const Pos2D &pos)
{
    auto &floor = *player_ptr->current_floor_ptr;
    auto &grid = floor.get_grid(pos);

    /* Blind players see nothing */
    if (player_ptr->effects()->blindness().is_blind()) {
        return;
    }

    /* Analyze non-torch-lit grids */
    if (!(grid.info & (CAVE_LITE | CAVE_MNLT))) {
        /* Require line of sight to the grid */
        if (!(grid.info & (CAVE_VIEW))) {
            return;
        }

        /* Require "perma-lite" of the grid */
        if ((grid.info & (CAVE_GLOW | CAVE_MNDK)) != CAVE_GLOW) {
            /* Not Ninja */
            if (!player_ptr->see_nocto) {
                return;
            }
        }
    }

    /* Hack -- memorize objects */
    for (const auto this_o_idx : grid.o_idx_list) {
        auto &item = *floor.o_list[this_o_idx];
        item.marked.set(OmType::FOUND);
        RedrawingFlagsUpdater::get_instance().set_flag(SubWindowRedrawingFlag::FOUND_ITEMS);
    }

    /* Hack -- memorize grids */
    if (!grid.is_mark()) {
        /* Feature code (applying "mimic" field) */
        const auto &terrain = grid.get_terrain(TerrainKind::MIMIC);

        /* Memorize some "boring" grids */
        if (terrain.flags.has_not(TerrainCharacteristics::REMEMBER)) {
            /* Option -- memorize all torch-lit floors */
            if (view_torch_grids && ((grid.info & (CAVE_LITE | CAVE_MNLT)) || player_ptr->see_nocto)) {
                grid.info |= (CAVE_MARK);
            }

            /* Option -- memorize all perma-lit floors */
            else if (view_perma_grids && ((grid.info & (CAVE_GLOW | CAVE_MNDK)) == CAVE_GLOW)) {
                grid.info |= (CAVE_MARK);
            }
        }

        /* Memorize normal grids */
        else if (terrain.flags.has(TerrainCharacteristics::LOS)) {
            grid.info |= (CAVE_MARK);
        }

        /* Memorize torch-lit walls */
        else if (grid.info & (CAVE_LITE | CAVE_MNLT)) {
            grid.info |= (CAVE_MARK);
        }

        /* Memorize walls seen by noctovision of Ninja */
        else if (player_ptr->see_nocto) {
            grid.info |= (CAVE_MARK);
        }

        /* Memorize certain non-torch-lit wall grids */
        else if (check_local_illumination(player_ptr, pos.y, pos.x)) {
            grid.info |= (CAVE_MARK);
        }
    }

    /* Memorize terrain of the grid */
    grid.info |= (CAVE_KNOWN);
}

/*
 * Redraw (on the screen) a given MAP location
 *
 * This function should only be called on "legal" grids
 */
void lite_spot(PlayerType *player_ptr, const Pos2D &pos)
{
    if (panel_contains(pos.y, pos.x) && player_ptr->current_floor_ptr->contains(pos, FloorBoundary::OUTER_WALL_INCLUSIVE)) {
        auto symbol_pair = map_info(player_ptr, pos);
        symbol_pair.symbol_foreground.color = get_monochrome_display_color(player_ptr).value_or(symbol_pair.symbol_foreground.color);

        term_queue_bigchar(panel_col_of(pos.x), pos.y - panel_row_prt, symbol_pair);
        static constexpr auto flags = {
            SubWindowRedrawingFlag::OVERHEAD,
            SubWindowRedrawingFlag::DUNGEON,
        };
        RedrawingFlagsUpdater::get_instance().set_flags(flags);
    }
}

/*
 * Some comments on the grid flags.  -BEN-
 *
 *
 * One of the major bottlenecks in previous versions of Angband was in
 * the calculation of "line of sight" from the player to various grids,
 * such as monsters.  This was such a nasty bottleneck that a lot of
 * silly things were done to reduce the dependancy on "line of sight",
 * for example, you could not "see" any grids in a lit room until you
 * actually entered the room, and there were all kinds of bizarre grid
 * flags to enable this behavior.  This is also why the "call light"
 * spells always lit an entire room.
 *
 * The code below provides functions to calculate the "field of view"
 * for the player, which, once calculated, provides extremely fast
 * calculation of "line of sight from the player", and to calculate
 * the "field of torch lite", which, again, once calculated, provides
 * extremely fast calculation of "which grids are lit by the player's
 * lite source".  In addition to marking grids as "GRID_VIEW" and/or
 * "GRID_LITE", as appropriate, these functions maintain an array for
 * each of these two flags, each array containing the locations of all
 * of the grids marked with the appropriate flag, which can be used to
 * very quickly scan through all of the grids in a given set.
 *
 * To allow more "semantically valid" field of view semantics, whenever
 * the field of view (or the set of torch lit grids) changes, all of the
 * grids in the field of view (or the set of torch lit grids) are "drawn"
 * so that changes in the world will become apparent as soon as possible.
 * This has been optimized so that only grids which actually "change" are
 * redrawn, using the "temp" array and the "GRID_TEMP" flag to keep track
 * of the grids which are entering or leaving the relevent set of grids.
 *
 * These new methods are so efficient that the old nasty code was removed.
 *
 * Note that there is no reason to "update" the "viewable space" unless
 * the player "moves", or walls/doors are created/destroyed, and there
 * is no reason to "update" the "torch lit grids" unless the field of
 * view changes, or the "light radius" changes.  This means that when
 * the player is resting, or digging, or doing anything that does not
 * involve movement or changing the state of the dungeon, there is no
 * need to update the "view" or the "lite" regions, which is nice.
 *
 * Note that the calls to the nasty "los()" function have been reduced
 * to a bare minimum by the use of the new "field of view" calculations.
 *
 * I wouldn't be surprised if slight modifications to the "update_view()"
 * function would allow us to determine "reverse line-of-sight" as well
 * as "normal line-of-sight", which would allow monsters to use a more
 * "correct" calculation to determine if they can "see" the player.  For
 * now, monsters simply "cheat" somewhat and assume that if the player
 * has "line of sight" to the monster, then the monster can "pretend"
 * that it has "line of sight" to the player.
 *
 *
 * The "update_lite()" function maintains the "CAVE_LITE" flag for each
 * grid and maintains an array of all "CAVE_LITE" grids.
 *
 * This set of grids is the complete set of all grids which are lit by
 * the players light source, which allows the "player_can_see_bold()"
 * function to work very quickly.
 *
 * Note that every "CAVE_LITE" grid is also a "CAVE_VIEW" grid, and in
 * fact, the player (unless blind) can always "see" all grids which are
 * marked as "CAVE_LITE", unless they are "off screen".
 *
 *
 * The "update_view()" function maintains the "CAVE_VIEW" flag for each
 * grid and maintains an array of all "CAVE_VIEW" grids.
 *
 * This set of grids is the complete set of all grids within line of sight
 * of the player, allowing the "player_has_los_bold()" macro to work very
 * quickly.
 *
 *
 * The current "update_view()" algorithm uses the "CAVE_XTRA" flag as a
 * temporary internal flag to mark those grids which are not only in view,
 * but which are also "easily" in line of sight of the player.  This flag
 * is always cleared when we are done.
 *
 *
 * The current "update_lite()" and "update_view()" algorithms use the
 * "CAVE_TEMP" flag, and the array of grids which are marked as "CAVE_TEMP",
 * to keep track of which grids were previously marked as "CAVE_LITE" or
 * "CAVE_VIEW", which allows us to optimize the "screen updates".
 *
 * The "CAVE_TEMP" flag, and the array of "CAVE_TEMP" grids, is also used
 * for various other purposes, such as spreading lite or darkness during
 * "lite_room()" / "unlite_room()", and for calculating monster flow.
 *
 *
 * Any grid can be marked as "CAVE_GLOW" which means that the grid itself is
 * in some way permanently lit.  However, for the player to "see" anything
 * in the grid, as determined by "player_can_see()", the player must not be
 * blind, the grid must be marked as "CAVE_VIEW", and, in addition, "wall"
 * grids, even if marked as "perma lit", are only illuminated if they touch
 * a grid which is not a wall and is marked both "CAVE_GLOW" and "CAVE_VIEW".
 *
 *
 * To simplify various things, a grid may be marked as "CAVE_MARK", meaning
 * that even if the player cannot "see" the grid, he "knows" the terrain in
 * that grid.  This is used to "remember" walls/doors/stairs/floors when they
 * are "seen" or "detected", and also to "memorize" floors, after "wiz_lite()",
 * or when one of the "memorize floor grids" options induces memorization.
 *
 * Objects are "memorized" in a different way, using a special "marked" flag
 * on the object itself, which is set when an object is observed or detected.
 *
 *
 * A grid may be marked as "CAVE_ROOM" which means that it is part of a "room",
 * and should be illuminated by "lite room" and "darkness" spells.
 *
 *
 * A grid may be marked as "CAVE_ICKY" which means it is part of a "vault",
 * and should be unavailable for "teleportation" destinations.
 *
 *
 * The "view_perma_grids" allows the player to "memorize" every perma-lit grid
 * which is observed, and the "view_torch_grids" allows the player to memorize
 * every torch-lit grid.  The player will always memorize important walls,
 * doors, stairs, and other terrain features, as well as any "detected" grids.
 *
 * Note that the new "update_view()" method allows, among other things, a room
 * to be "partially" seen as the player approaches it, with a growing cone of
 * floor appearing as the player gets closer to the door.  Also, by not turning
 * on the "memorize perma-lit grids" option, the player will only "see" those
 * floor grids which are actually in line of sight.
 *
 * And my favorite "plus" is that you can now use a special option to draw the
 * "floors" in the "viewable region" brightly (actually, to draw the *other*
 * grids dimly), providing a "pretty" effect as the player runs around, and
 * to efficiently display the "torch lite" in a special color.
 *
 *
 * Some comments on the "update_view()" algorithm...
 *
 * The algorithm is very fast, since it spreads "obvious" grids very quickly,
 * and only has to call "los()" on the borderline cases.  The major axes/diags
 * even terminate early when they hit walls.  I need to find a quick way
 * to "terminate" the other scans.
 *
 * Note that in the worst case (a big empty area with say 5% scattered walls),
 * each of the 1500 or so nearby grids is checked once, most of them getting
 * an "instant" rating, and only a small portion requiring a call to "los()".
 *
 * The only time that the algorithm appears to be "noticeably" too slow is
 * when running, and this is usually only important in town, since the town
 * provides about the worst scenario possible, with large open regions and
 * a few scattered obstructions.  There is a special "efficiency" option to
 * allow the player to reduce his field of view in town, if needed.
 *
 * In the "best" case (say, a normal stretch of corridor), the algorithm
 * makes one check for each viewable grid, and makes no calls to "los()".
 * So running in corridors is very fast, and if a lot of monsters are
 * nearby, it is much faster than the old methods.
 *
 * Note that resting, most normal commands, and several forms of running,
 * plus all commands executed near large groups of monsters, are strictly
 * more efficient with "update_view()" that with the old "compute los() on
 * demand" method, primarily because once the "field of view" has been
 * calculated, it does not have to be recalculated until the player moves
 * (or a wall or door is created or destroyed).
 *
 * Note that we no longer have to do as many "los()" checks, since once the
 * "view" region has been built, very few things cause it to be "changed"
 * (player movement, and the opening/closing of doors, changes in wall status).
 * Note that door/wall changes are only relevant when the door/wall itself is
 * in the "view" region.
 *
 * The algorithm seems to only call "los()" from zero to ten times, usually
 * only when coming down a corridor into a room, or standing in a room, just
 * misaligned with a corridor.  So if, say, there are five "nearby" monsters,
 * we will be reducing the calls to "los()".
 *
 * I am thinking in terms of an algorithm that "walks" from the central point
 * out to the maximal "distance", at each point, determining the "view" code
 * (above).  For each grid not on a major axis or diagonal, the "view" code
 * depends on the "cave_los_bold()" and "view" of exactly two other grids
 * (the one along the nearest diagonal, and the one next to that one, see
 * "update_view_aux()"...).
 *
 * We "memorize" the viewable space array, so that at the cost of under 3000
 * bytes, we reduce the time taken by "forget_view()" to one assignment for
 * each grid actually in the "viewable space".  And for another 3000 bytes,
 * we prevent "erase + redraw" ineffiencies via the "seen" set.  These bytes
 * are also used by other routines, thus reducing the cost to almost nothing.
 *
 * A similar thing is done for "forget_lite()" in which case the savings are
 * much less, but save us from doing bizarre maintenance checking.
 *
 * In the worst "normal" case (in the middle of the town), the reachable space
 * actually reaches to more than half of the largest possible "circle" of view,
 * or about 800 grids, and in the worse case (in the middle of a dungeon level
 * where all the walls have been removed), the reachable space actually reaches
 * the theoretical maximum size of just under 1500 grids.
 *
 * Each grid G examines the "state" of two (?) other (adjacent) grids, G1 & G2.
 * If G1 is lite, G is lite.  Else if G2 is lite, G is half.  Else if G1 and G2
 * are both half, G is half.  Else G is dark.  It only takes 2 (or 4) bits to
 * "name" a grid, so (for MAX_RAD of 20) we could use 1600 bytes, and scan the
 * entire possible space (including initialization) in one step per grid.  If
 * we do the "clearing" as a separate step (and use an array of "view" grids),
 * then the clearing will take as many steps as grids that were viewed, and the
 * algorithm will be able to "stop" scanning at various points.
 * Oh, and outside of the "torch radius", only "lite" grids need to be scanned.
 */

/*
 * Hack - speed up the update_flow algorithm by only doing
 * it everytime the player moves out of LOS of the last
 * "way-point".
 */
static POSITION flow_x = 0;
static POSITION flow_y = 0;

/*
 * Hack -- fill in the "cost" field of every grid that the player
 * can "reach" with the number of steps needed to reach that grid.
 * This also yields the "distance" of the player from every grid.
 *
 * In addition, mark the "when" of the grids that can reach
 * the player with the incremented value of "flow_n".
 *
 * Hack -- use the "seen" array as a "circular queue".
 *
 * We do not need a priority queue because the cost from grid
 * to grid is always "one" and we process them in order.
 */
void update_flow(PlayerType *player_ptr)
{
    auto &floor = *player_ptr->current_floor_ptr;

    /* The last way-point is on the map */
    const Pos2D flow(flow_y, flow_x);
    if (player_ptr->running && floor.contains(flow)) {
        /* The way point is in sight - do not update.  (Speedup) */
        if (floor.get_grid(flow).info & CAVE_VIEW) {
            return;
        }
    }

    /* Erase all of the current flow information */
    for (const auto &pos : floor.get_area()) {
        auto &grid = floor.get_grid(pos);
        grid.reset_costs();
        grid.reset_dists();
    }

    /* Save player position */
    flow_y = player_ptr->y;
    flow_x = player_ptr->x;

    for (const auto gf : GRID_FLOW_RANGE) {
        // 幅優先探索用のキュー。
        std::queue<Pos2D> que;
        que.emplace(player_ptr->y, player_ptr->x);

        /* Now process the queue */
        while (!que.empty()) {
            const Pos2D pos = std::move(que.front());
            que.pop();
            const auto &grid = floor.get_grid(pos);

            /* Add the "children" */
            for (const auto &d : Direction::directions_8()) {
                uint8_t m = grid.costs.at(gf) + 1;
                const uint8_t n = grid.dists.at(gf) + 1;
                const auto pos_neighbor = pos + d.vec();

                /* Ignore player's grid */
                if (player_ptr->is_located_at(pos_neighbor)) {
                    continue;
                }

                if (floor.has_closed_door_at(pos_neighbor)) {
                    m += 3;
                }

                /* Ignore "pre-stamped" entries */
                auto &grid_neighbor = floor.get_grid(pos_neighbor);
                auto &cost_neighbor = grid_neighbor.costs.at(gf);
                auto &dist_neighbor = grid_neighbor.dists.at(gf);
                if ((dist_neighbor != 0) && (dist_neighbor <= n) && (cost_neighbor <= m)) {
                    continue;
                }

                /* Ignore "walls", "holes" and "rubble" */
                auto can_move = false;
                switch (gf) {
                case GridFlow::CAN_FLY:
                    can_move = grid_neighbor.has(TerrainCharacteristics::MOVE) || grid_neighbor.has(TerrainCharacteristics::CAN_FLY);
                    break;
                default:
                    can_move = grid_neighbor.has(TerrainCharacteristics::MOVE);
                    break;
                }

                if (!can_move && !floor.has_closed_door_at(pos_neighbor)) {
                    continue;
                }

                /* Save the flow cost */
                if (cost_neighbor == 0 || (cost_neighbor > m)) {
                    cost_neighbor = m;
                }
                if (dist_neighbor == 0 || (dist_neighbor > n)) {
                    dist_neighbor = n;
                }

                // 敵のプレイヤーに対する移動道のりの最大値(この値以上は処理を打ち切る).
                constexpr auto monster_flow_depth = 32;
                if (n == monster_flow_depth) {
                    continue;
                }

                que.emplace(pos_neighbor);
            }
        }
    }
}

/*
 * Takes a location and action and changes the feature at that
 * location through applying the given action.
 */
void cave_alter_feat(PlayerType *player_ptr, POSITION y, POSITION x, TerrainCharacteristics action)
{
    const Pos2D pos(y, x);
    auto &floor = *player_ptr->current_floor_ptr;
    const auto old_terrain_id = floor.get_grid(pos).feat;
    const auto &dungeon = floor.get_dungeon_definition();
    const auto new_terrain_id = dungeon.convert_terrain_id(old_terrain_id, action);
    if (new_terrain_id == old_terrain_id) {
        return;
    }

    /* Set the new feature */
    set_terrain_id_to_grid(player_ptr, pos, new_terrain_id);
    const auto &terrains = TerrainList::get_instance();
    const auto &world = AngbandWorld::get_instance();
    if (!TerrainType::has(action, TerrainAction::NO_DROP)) {
        const auto &old_terrain = terrains.get_terrain(old_terrain_id);
        const auto &new_terrain = terrains.get_terrain(new_terrain_id);
        auto found = false;

        /* Handle gold */
        if (old_terrain.flags.has(TerrainCharacteristics::HAS_GOLD) && new_terrain.flags.has_not(TerrainCharacteristics::HAS_GOLD)) {
            /* Place some gold */
            place_gold(player_ptr, pos);
            found = true;
        }

        /* Handle item */
        if (old_terrain.flags.has(TerrainCharacteristics::HAS_ITEM) && new_terrain.flags.has_not(TerrainCharacteristics::HAS_ITEM) && evaluate_percent(15 - floor.dun_level / 2)) {
            /* Place object */
            place_object(player_ptr, pos, 0);
            found = true;
        }

        if (found && world.character_dungeon && player_can_see_bold(player_ptr, pos.y, pos.x)) {
            msg_print(_("何かを発見した！", "You have found something!"));
        }
    }

    if (TerrainType::has(action, TerrainAction::CRASH_GLASS)) {
        const auto &old_terrain = terrains.get_terrain(old_terrain_id);
        if (old_terrain.flags.has(TerrainCharacteristics::GLASS) && world.character_dungeon) {
            project(player_ptr, PROJECT_WHO_GLASS_SHARDS, 1, pos.y, pos.x, std::min(floor.dun_level, 100) / 4, AttributeType::SHARDS,
                (PROJECT_GRID | PROJECT_ITEM | PROJECT_KILL | PROJECT_HIDE | PROJECT_JUMP | PROJECT_NO_HANGEKI));
        }
    }
}

/*!
 * @brief 指定されたマスがモンスターのテレポート可能先かどうかを判定する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param m_idx モンスターID
 * @param y 移動先Y座標
 * @param x 移動先X座標
 * @param mode オプション
 * @return テレポート先として妥当ならばtrue
 */
bool cave_monster_teleportable_bold(PlayerType *player_ptr, MONSTER_IDX m_idx, POSITION y, POSITION x, teleport_flags mode)
{
    const Pos2D pos(y, x);
    const auto &floor = *player_ptr->current_floor_ptr;
    const auto &grid = floor.get_grid(pos);
    const auto &terrain = grid.get_terrain();

    /* Require "teleportable" space */
    if (terrain.flags.has_not(TerrainCharacteristics::TELEPORTABLE)) {
        return false;
    }

    if (grid.has_monster() && (grid.m_idx != m_idx)) {
        return false;
    }
    if (player_ptr->is_located_at(pos)) {
        return false;
    }

    /* Hack -- no teleport onto rune of protection */
    if (grid.is_rune_protection()) {
        return false;
    }
    if (grid.is_rune_explosion()) {
        return false;
    }

    if (any_bits(mode, TELEPORT_PASSIVE)) {
        return true;
    }

    const auto &monster = floor.m_list[m_idx];
    return monster_can_cross_terrain(player_ptr, grid.feat, monster.get_monrace(), 0);
}

/*!
 * @brief 指定されたマスにプレイヤーがテレポート可能かどうかを判定する。
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 移動先Y座標
 * @param x 移動先X座標
 * @param mode オプション
 * @return テレポート先として妥当ならばtrue
 */
bool cave_player_teleportable_bold(PlayerType *player_ptr, POSITION y, POSITION x, teleport_flags mode)
{
    const Pos2D pos(y, x);
    const auto &grid = player_ptr->current_floor_ptr->get_grid(pos);
    const auto &terrain = grid.get_terrain();

    /* Require "teleportable" space */
    if (terrain.flags.has_not(TerrainCharacteristics::TELEPORTABLE)) {
        return false;
    }

    /* No magical teleporting into vaults and such */
    if (!(mode & TELEPORT_NONMAGICAL) && grid.is_icky()) {
        return false;
    }
    const auto &floor = *player_ptr->current_floor_ptr;
    if (grid.has_monster() && !floor.m_list[grid.m_idx].is_riding()) {
        return false;
    }

    /* don't teleport on a trap. */
    if (terrain.flags.has(TerrainCharacteristics::HIT_TRAP)) {
        return false;
    }

    if (any_bits(mode, TELEPORT_PASSIVE)) {
        return true;
    }

    if (!player_can_enter(player_ptr, grid.feat, 0)) {
        return false;
    }

    if (terrain.flags.has_all_of({ TerrainCharacteristics::WATER, TerrainCharacteristics::DEEP })) {
        if (!player_ptr->levitation && !player_ptr->can_swim) {
            return false;
        }
    }

    if (terrain.flags.has_not(TerrainCharacteristics::LAVA) || has_immune_fire(player_ptr) || is_invuln(player_ptr)) {
        return true;
    }

    /* Always forbid deep lava */
    if (terrain.flags.has(TerrainCharacteristics::DEEP)) {
        return false;
    }

    /* Forbid shallow lava when the player don't have levitation */
    return player_ptr->levitation != 0;
}

/*!
 * @brief プレイヤーが地形踏破可能かを返す
 * @param feature 判定したい地形ID
 * @param mode 移動に関するオプションフラグ
 * @return 移動可能ならばTRUEを返す
 */
bool player_can_enter(PlayerType *player_ptr, FEAT_IDX feature, BIT_FLAGS16 mode)
{
    const auto &terrain = TerrainList::get_instance().get_terrain(feature);
    if (player_ptr->riding) {
        return monster_can_cross_terrain(
            player_ptr, feature, player_ptr->current_floor_ptr->m_list[player_ptr->riding].get_monrace(), mode | CEM_RIDING);
    }

    if (terrain.flags.has(TerrainCharacteristics::PATTERN)) {
        if (!(mode & CEM_P_CAN_ENTER_PATTERN)) {
            return false;
        }
    }

    if (terrain.flags.has(TerrainCharacteristics::CAN_FLY) && player_ptr->levitation) {
        return true;
    }
    if (terrain.flags.has(TerrainCharacteristics::CAN_SWIM) && player_ptr->can_swim) {
        return true;
    }
    if (terrain.flags.has(TerrainCharacteristics::CAN_PASS) && has_pass_wall(player_ptr)) {
        return true;
    }

    if (terrain.flags.has_not(TerrainCharacteristics::MOVE)) {
        return false;
    }

    return true;
}

void place_grid(PlayerType *player_ptr, Grid &grid, grid_bold_type gb_type)
{
    const auto &dungeon = player_ptr->current_floor_ptr->get_dungeon_definition();
    switch (gb_type) {
    case GB_FLOOR: {
        grid.set_terrain_id(dungeon.select_floor_terrain_id());
        grid.info &= ~(CAVE_MASK);
        grid.info |= CAVE_FLOOR;
        break;
    }
    case GB_EXTRA: {
        grid.set_terrain_id(dungeon.select_wall_terrain_id());
        grid.info &= ~(CAVE_MASK);
        grid.info |= CAVE_EXTRA;
        break;
    }
    case GB_EXTRA_PERM: {
        grid.set_terrain_id(TerrainTag::PERMANENT_WALL);
        grid.info &= ~(CAVE_MASK);
        grid.info |= CAVE_EXTRA;
        break;
    }
    case GB_INNER: {
        grid.set_terrain_id(dungeon.inner_wall);
        grid.info &= ~(CAVE_MASK);
        grid.info |= CAVE_INNER;
        break;
    }
    case GB_INNER_PERM: {
        grid.set_terrain_id(TerrainTag::PERMANENT_WALL);
        grid.info &= ~(CAVE_MASK);
        grid.info |= CAVE_INNER;
        break;
    }
    case GB_OUTER: {
        grid.set_terrain_id(dungeon.outer_wall);
        grid.info &= ~(CAVE_MASK);
        grid.info |= CAVE_OUTER;
        break;
    }
    case GB_OUTER_NOPERM: {
        const auto &terrain = TerrainList::get_instance().get_terrain(dungeon.outer_wall);
        if (terrain.is_permanent_wall()) {
            const auto terrain_id = dungeon.convert_terrain_id(dungeon.outer_wall, TerrainCharacteristics::UNPERM);
            grid.set_terrain_id(terrain_id);
        } else {
            grid.set_terrain_id(dungeon.outer_wall);
        }

        grid.info &= ~(CAVE_MASK);
        grid.info |= (CAVE_OUTER | CAVE_VAULT);
        break;
    }
    case GB_SOLID: {
        grid.set_terrain_id(dungeon.outer_wall);
        grid.info &= ~(CAVE_MASK);
        grid.info |= CAVE_SOLID;
        break;
    }
    case GB_SOLID_PERM: {
        grid.set_terrain_id(TerrainTag::PERMANENT_WALL);
        grid.info &= ~(CAVE_MASK);
        grid.info |= CAVE_SOLID;
        break;
    }
    case GB_SOLID_NOPERM: {
        const auto &terrain = TerrainList::get_instance().get_terrain(dungeon.outer_wall);
        if ((grid.info & CAVE_VAULT) && terrain.is_permanent_wall()) {
            const auto terrain_id = dungeon.convert_terrain_id(dungeon.outer_wall, TerrainCharacteristics::UNPERM);
            grid.set_terrain_id(terrain_id);
        } else {
            grid.set_terrain_id(dungeon.outer_wall);
        }
        grid.info &= ~(CAVE_MASK);
        grid.info |= CAVE_SOLID;
        break;
    }
    default:
        // 未知の値が渡されたら何もしない。
        return;
    }

    if (grid.has_monster()) {
        delete_monster_idx(player_ptr, grid.m_idx);
    }
}

void place_bold(PlayerType *player_ptr, POSITION y, POSITION x, grid_bold_type gb_type)
{
    auto &grid = player_ptr->current_floor_ptr->grid_array[y][x];
    place_grid(player_ptr, grid, gb_type);
}

/*
 * This function allows us to efficiently add a grid to the "lite" array,
 * note that we are never called for illegal grids, or for grids which
 * have already been placed into the "lite" array, and we are never
 * called when the "lite" array is full.
 */
void cave_lite_hack(FloorType &floor, POSITION y, POSITION x)
{
    auto &grid = floor.grid_array[y][x];
    if (grid.is_lite()) {
        return;
    }

    grid.info |= CAVE_LITE;
    floor.lite_y[floor.lite_n] = y;
    floor.lite_x[floor.lite_n++] = x;
}

/*
 * For delayed visual update
 */
void cave_redraw_later(FloorType &floor, POSITION y, POSITION x)
{
    auto &grid = floor.grid_array[y][x];
    if (grid.is_redraw()) {
        return;
    }

    grid.info |= CAVE_REDRAW;
    floor.redraw_y[floor.redraw_n] = y;
    floor.redraw_x[floor.redraw_n++] = x;
}

/*
 * For delayed visual update
 */
void cave_note_and_redraw_later(FloorType &floor, POSITION y, POSITION x)
{
    floor.grid_array[y][x].info |= CAVE_NOTE;
    cave_redraw_later(floor, y, x);
}

void cave_view_hack(FloorType &floor, POSITION y, POSITION x)
{
    auto &grid = floor.grid_array[y][x];
    if (grid.is_view()) {
        return;
    }

    grid.info |= CAVE_VIEW;
    floor.view_y[floor.view_n] = y;
    floor.view_x[floor.view_n] = x;
    floor.view_n++;
}
