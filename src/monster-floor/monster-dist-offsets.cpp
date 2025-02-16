#include "monster-floor/monster-dist-offsets.h"

/*!
 * @brief 基準座標からの距離がnとなる座標の基準座標からのオフセットのリスト
 *
 * DIST_OFFSET[n]は、基準座標からの距離がnとなる基準座標からのオフセットのリストを保持する。
 * 距離nとなる座標について毎回 Grid::calc_distance() で調べるのを避けるため、距離0～9について事前に計算しておいたもの。
 */
const std::array<std::vector<Pos2DVec>, 10> DIST_OFFSETS = {
    {
        // 自動整形でインデントを揃えるためのコメント
        { { 0, 0 } },
        { { -1, -1 }, { -1, 0 }, { -1, 1 }, { 0, -1 }, { 0, 1 }, { 1, -1 }, { 1, 0 }, { 1, 1 } },
        { { -1, -2 }, { -1, 2 }, { -2, -1 }, { -2, 0 }, { -2, 1 }, { 0, -2 }, { 0, 2 }, { 1, -2 }, { 1, 2 }, { 2, -1 }, { 2, 0 }, { 2, 1 } },
        { { -1, -3 }, { -1, 3 }, { -2, -2 }, { -2, 2 }, { -3, -1 }, { -3, 0 }, { -3, 1 }, { 0, -3 }, { 0, 3 }, { 1, -3 }, { 1, 3 }, { 2, -2 }, { 2, 2 }, { 3, -1 }, { 3, 0 }, { 3, 1 } },
        { { -1, -4 }, { -1, 4 }, { -2, -3 }, { -2, 3 }, { -3, -2 }, { -3, -3 }, { -3, 2 }, { -3, 3 }, { -4, -1 }, { -4, 0 }, { -4, 1 }, { 0, -4 }, { 0, 4 }, { 1, -4 }, { 1, 4 }, { 2, -3 }, { 2, 3 }, { 3, -2 }, { 3, -3 }, { 3, 2 }, { 3, 3 }, { 4, -1 }, { 4, 0 }, { 4, 1 } },
        { { -1, -5 }, { -1, 5 }, { -2, -4 }, { -2, 4 }, { -3, -4 }, { -3, 4 }, { -4, -2 }, { -4, -3 }, { -4, 2 }, { -4, 3 }, { -5, -1 }, { -5, 0 }, { -5, 1 }, { 0, -5 }, { 0, 5 }, { 1, -5 }, { 1, 5 }, { 2, -4 }, { 2, 4 }, { 3, -4 }, { 3, 4 }, { 4, -2 }, { 4, -3 }, { 4, 2 }, { 4, 3 }, { 5, -1 }, { 5, 0 }, { 5, 1 } },
        { { -1, -6 }, { -1, 6 }, { -2, -5 }, { -2, 5 }, { -3, -5 }, { -3, 5 }, { -4, -4 }, { -4, 4 }, { -5, -2 }, { -5, -3 }, { -5, 2 }, { -5, 3 }, { -6, -1 }, { -6, 0 }, { -6, 1 }, { 0, -6 }, { 0, 6 }, { 1, -6 }, { 1, 6 }, { 2, -5 }, { 2, 5 }, { 3, -5 }, { 3, 5 }, { 4, -4 }, { 4, 4 }, { 5, -2 }, { 5, -3 }, { 5, 2 }, { 5, 3 }, { 6, -1 }, { 6, 0 }, { 6, 1 } },
        { { -1, -7 }, { -1, 7 }, { -2, -6 }, { -2, 6 }, { -3, -6 }, { -3, 6 }, { -4, -5 }, { -4, 5 }, { -5, -4 }, { -5, -5 }, { -5, 4 }, { -5, 5 }, { -6, -2 }, { -6, -3 }, { -6, 2 }, { -6, 3 }, { -7, -1 }, { -7, 0 }, { -7, 1 }, { 0, -7 }, { 0, 7 }, { 1, -7 }, { 1, 7 }, { 2, -6 }, { 2, 6 }, { 3, -6 }, { 3, 6 }, { 4, -5 }, { 4, 5 }, { 5, -4 }, { 5, -5 }, { 5, 4 }, { 5, 5 }, { 6, -2 }, { 6, -3 }, { 6, 2 }, { 6, 3 }, { 7, -1 }, { 7, 0 }, { 7, 1 } },
        { { -1, -8 }, { -1, 8 }, { -2, -7 }, { -2, 7 }, { -3, -7 }, { -3, 7 }, { -4, -6 }, { -4, 6 }, { -5, -6 }, { -5, 6 }, { -6, -4 }, { -6, -5 }, { -6, 4 }, { -6, 5 }, { -7, -2 }, { -7, -3 }, { -7, 2 }, { -7, 3 }, { -8, -1 }, { -8, 0 }, { -8, 1 }, { 0, -8 }, { 0, 8 }, { 1, -8 }, { 1, 8 }, { 2, -7 }, { 2, 7 }, { 3, -7 }, { 3, 7 }, { 4, -6 }, { 4, 6 }, { 5, -6 }, { 5, 6 }, { 6, -4 }, { 6, -5 }, { 6, 4 }, { 6, 5 }, { 7, -2 }, { 7, -3 }, { 7, 2 }, { 7, 3 }, { 8, -1 }, { 8, 0 }, { 8, 1 } },
        { { -1, -9 }, { -1, 9 }, { -2, -8 }, { -2, 8 }, { -3, -8 }, { -3, 8 }, { -4, -7 }, { -4, 7 }, { -5, -7 }, { -5, 7 }, { -6, -6 }, { -6, 6 }, { -7, -4 }, { -7, -5 }, { -7, 4 }, { -7, 5 }, { -8, -2 }, { -8, -3 }, { -8, 2 }, { -8, 3 }, { -9, -1 }, { -9, 0 }, { -9, 1 }, { 0, -9 }, { 0, 9 }, { 1, -9 }, { 1, 9 }, { 2, -8 }, { 2, 8 }, { 3, -8 }, { 3, 8 }, { 4, -7 }, { 4, 7 }, { 5, -7 }, { 5, 7 }, { 6, -6 }, { 6, 6 }, { 7, -4 }, { 7, -5 }, { 7, 4 }, { 7, 5 }, { 8, -2 }, { 8, -3 }, { 8, 2 }, { 8, 3 }, { 9, -1 }, { 9, 0 }, { 9, 1 } }
        // 自動整形でインデントを揃えるためのコメント
    },
};
