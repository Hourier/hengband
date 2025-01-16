#include "floor/object-allocator.h"
#include "dungeon/quest.h"
#include "floor/cave.h"
#include "floor/dungeon-tunnel-util.h"
#include "floor/floor-allocation-types.h"
#include "floor/floor-generator-util.h"
#include "game-option/birth-options.h"
#include "game-option/cheat-types.h"
#include "grid/object-placer.h"
#include "grid/trap.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/enums/terrain/terrain-tag.h"
#include "system/floor/floor-info.h"
#include "system/grid-type-definition.h"
#include "system/monrace/monrace-definition.h"
#include "system/player-type-definition.h"
#include "system/terrain/terrain-definition.h"
#include "system/terrain/terrain-list.h"
#include "util/bit-flags-calculator.h"
#include "wizard/wizard-messages.h"

/*!
 * @brief 上下左右の外壁数をカウントする / Count the number of walls adjacent to the given grid.
 * @param y 基準のy座標
 * @param x 基準のx座標
 * @return 隣接する外壁の数
 * @note Assumes "in_bounds()"
 * @details We count only granite walls and permanent walls.
 */
static int next_to_walls(FloorType *floor_ptr, POSITION y, POSITION x)
{
    int k = 0;
    if (in_bounds(floor_ptr, y + 1, x) && floor_ptr->grid_array[y + 1][x].is_extra()) {
        k++;
    }

    if (in_bounds(floor_ptr, y - 1, x) && floor_ptr->grid_array[y - 1][x].is_extra()) {
        k++;
    }

    if (in_bounds(floor_ptr, y, x + 1) && floor_ptr->grid_array[y][x + 1].is_extra()) {
        k++;
    }

    if (in_bounds(floor_ptr, y, x - 1) && floor_ptr->grid_array[y][x - 1].is_extra()) {
        k++;
    }

    return k;
}

/*!
 * @brief alloc_stairs()の補助として指定の位置に階段を生成できるかの判定を行う / Helper function for alloc_stairs(). Is this a good location for stairs?
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param y 基準のy座標
 * @param x 基準のx座標
 * @param walls 最低減隣接させたい外壁の数
 * @return 階段を生成して問題がないならばTRUEを返す。
 */
static bool alloc_stairs_aux(PlayerType *player_ptr, POSITION y, POSITION x, int walls)
{
    auto *floor_ptr = player_ptr->current_floor_ptr;
    auto *g_ptr = &floor_ptr->grid_array[y][x];
    if (!g_ptr->is_floor() || pattern_tile(floor_ptr, y, x) || !g_ptr->o_idx_list.empty() || g_ptr->has_monster() || next_to_walls(floor_ptr, y, x) < walls) {
        return false;
    }

    return true;
}

/*!
 * @brief 外壁に隣接させて階段を生成する / Places some staircases near walls
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param feat 配置したい地形ID
 * @param num 配置したい階段の数
 * @param walls 最低減隣接させたい外壁の数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
bool alloc_stairs(PlayerType *player_ptr, FEAT_IDX feat, int num, int walls)
{
    int shaft_num = 0;
    const auto &terrain = TerrainList::get_instance().get_terrain(feat);
    auto &floor = *player_ptr->current_floor_ptr;
    const auto &dungeon = floor.get_dungeon_definition();
    if (terrain.flags.has(TerrainCharacteristics::LESS)) {
        if (ironman_downward || !floor.is_underground()) {
            return true;
        }

        if (floor.dun_level > dungeon.mindepth) {
            shaft_num = (randint1(num + 1)) / 2;
        }
    } else if (terrain.flags.has(TerrainCharacteristics::MORE)) {
        auto quest_id = floor.get_quest_id();
        const auto &quests = QuestList::get_instance();
        if (floor.dun_level > 1 && inside_quest(quest_id)) {
            const auto &monrace = quests.get_quest(quest_id).get_bounty();
            if (!monrace.is_dead_unique()) {
                return true;
            }
        }

        if (floor.dun_level >= dungeon.maxdepth) {
            return true;
        }

        if ((floor.dun_level < dungeon.maxdepth - 1) && !inside_quest(floor.get_quest_id(1))) {
            shaft_num = (randint1(num) + 1) / 2;
        }
    } else {
        return false;
    }

    for (int i = 0; i < num; i++) {
        while (true) {
            Grid *g_ptr;
            int candidates = 0;
            const POSITION max_x = floor.width - 1;
            for (POSITION y = 1; y < floor.height - 1; y++) {
                for (POSITION x = 1; x < max_x; x++) {
                    if (alloc_stairs_aux(player_ptr, y, x, walls)) {
                        candidates++;
                    }
                }
            }

            if (!candidates) {
                if (walls <= 0) {
                    return false;
                }

                walls--;
                continue;
            }

            int pick = randint1(candidates);
            POSITION y;
            POSITION x = max_x;
            for (y = 1; y < floor.height - 1; y++) {
                for (x = 1; x < floor.width - 1; x++) {
                    if (alloc_stairs_aux(player_ptr, y, x, walls)) {
                        pick--;
                        if (pick == 0) {
                            break;
                        }
                    }
                }

                if (pick == 0) {
                    break;
                }
            }

            g_ptr = &floor.grid_array[y][x];
            g_ptr->mimic = 0;
            g_ptr->feat = (i < shaft_num) ? dungeon.convert_terrain_id(feat, TerrainCharacteristics::SHAFT) : feat;
            g_ptr->info &= ~(CAVE_FLOOR);
            break;
        }
    }

    return true;
}

/*!
 * @brief フロア上のランダム位置に各種オブジェクトを配置する / Allocates some objects (using "place" and "type")
 * @param set 配置したい地形の種類
 * @param typ 配置したいオブジェクトの種類
 * @param num 配置したい数
 * @return 規定数通りに生成に成功したらTRUEを返す。
 */
void alloc_object(PlayerType *player_ptr, dap_type set, dungeon_allocation_type typ, int num)
{
    auto dummy = 0;
    auto &floor = *player_ptr->current_floor_ptr;
    num = num * floor.height * floor.width / (MAX_HGT * MAX_WID) + 1;
    for (auto k = 0; k < num; k++) {
        Pos2D pos(0, 0);
        while (dummy < SAFE_MAX_ATTEMPTS) {
            dummy++;
            pos.y = randint0(floor.height);
            pos.x = randint0(floor.width);
            const auto &grid = floor.get_grid(pos);
            if (!grid.is_floor() || !grid.o_idx_list.empty() || grid.has_monster()) {
                continue;
            }

            if (player_ptr->is_located_at(pos)) {
                continue;
            }

            auto is_room = grid.is_room();
            if (((set == ALLOC_SET_CORR) && is_room) || ((set == ALLOC_SET_ROOM) && !is_room)) {
                continue;
            }

            break;
        }

        if (dummy >= SAFE_MAX_ATTEMPTS) {
            msg_print_wizard(player_ptr, CHEAT_DUNGEON, _("アイテムの配置に失敗しました。", "Failed to place object."));
            return;
        }

        switch (typ) {
        case ALLOC_TYP_RUBBLE:
            floor.set_terrain_id_at(pos, TerrainTag::RUBBLE);
            floor.get_grid(pos).info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_TRAP:
            place_trap(&floor, pos.y, pos.x);
            floor.get_grid(pos).info &= ~(CAVE_FLOOR);
            break;
        case ALLOC_TYP_GOLD:
            place_gold(player_ptr, pos.y, pos.x);
            break;
        case ALLOC_TYP_OBJECT:
            place_object(player_ptr, pos.y, pos.x, 0L);
            break;
        default:
            break;
        }
    }
}
