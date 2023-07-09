﻿#pragma once

/*
 * Bit flags for the place_monster_???() (etc)
 */
enum place_monster_type {
    PM_NONE = 0x00000000, /*!< 特になし*/
    PM_ALLOW_SLEEP = 0x00000001, /*!< モンスター生成フラグ: 眠っている状態で生成されても良い */
    PM_ALLOW_GROUP = 0x00000002, /*!< モンスター生成フラグ: 集団生成されても良い */
    PM_FORCE_FRIENDLY = 0x00000004, /*!< モンスター生成フラグ: 必ず友好的に生成される */
    PM_FORCE_PET = 0x00000008, /*!< モンスター生成フラグ: 必ずペットとして生成される */
    PM_NO_KAGE = 0x00000010, /*!< モンスター生成フラグ: 必ずあやしい影としては生成されない */
    PM_NO_PET = 0x00000020, /*!< モンスター生成フラグ: 必ずペットとして生成されない */
    PM_ALLOW_UNIQUE = 0x00000040, /*!< モンスター生成フラグ: ユニークの選択生成を許可する */
    PM_IGNORE_TERRAIN = 0x00000080, /*!< モンスター生成フラグ: 侵入可能地形を考慮せずに生成する */
    PM_HASTE = 0x00000100, /*!< モンスター生成フラグ: 加速状態で生成する */
    PM_KAGE = 0x00000200, /*!< モンスター生成フラグ: 必ずあやしい影として生成する */
    PM_MULTIPLY = 0x00000400, /*!< モンスター生成フラグ: 増殖処理時として生成する */
    PM_JURAL = 0x00000800, /*!< モンスター生成フラグ: ジュラル星人として誤認生成する */
    PM_NO_QUEST = 0x00001000, /*!< モンスター生成フラグ: クエスト除外モンスターを生成しない */
    PM_CLONE = 0x00002000, /*!< モンスター生成フラグ: クローンとして生成する */
    PM_ARENA = 0x00004000, /*!< モンスター生成フラグ: アリーナ用の生成 */

};
