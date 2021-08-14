﻿#pragma once

/*!
 * @details 既にplayer_raceが存在するので_typeと付けた
 */
enum class player_race_type {
    RACE_HUMAN = 0,
    RACE_HALF_ELF = 1,
    RACE_ELF = 2,
    RACE_HOBBIT = 3,
    RACE_GNOME = 4,
    RACE_DWARF = 5,
    RACE_HALF_ORC = 6,
    RACE_HALF_TROLL = 7,
    RACE_AMBERITE = 8,
    RACE_HIGH_ELF = 9,
    RACE_BARBARIAN = 10,
    RACE_HALF_OGRE = 11,
    RACE_HALF_GIANT = 12,
    RACE_HALF_TITAN = 13,
    RACE_CYCLOPS = 14,
    RACE_YEEK = 15,
    RACE_KLACKON = 16,
    RACE_KOBOLD = 17,
    RACE_NIBELUNG = 18,
    RACE_DARK_ELF = 19,
    RACE_DRACONIAN = 20,
    RACE_MIND_FLAYER = 21,
    RACE_IMP = 22,
    RACE_GOLEM = 23,
    RACE_SKELETON = 24,
    RACE_ZOMBIE = 25,
    RACE_VAMPIRE = 26,
    RACE_SPECTRE = 27,
    RACE_SPRITE = 28,
    RACE_BEASTMAN = 29,
    RACE_ENT = 30,
    RACE_ARCHON = 31,
    RACE_BALROG = 32,
    RACE_DUNADAN = 33,
    RACE_S_FAIRY = 34,
    RACE_KUTAR = 35,
    RACE_ANDROID = 36,
    RACE_MERFOLK = 37,
    MAX,
};

constexpr int MAX_RACES = static_cast<int>(player_race_type::MAX);
