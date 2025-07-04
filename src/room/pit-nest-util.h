#pragma once

#include "monster-race/race-ability-flags.h"
#include "util/flag-group.h"
#include <map>
#include <string>
#include <tl/optional.hpp>

enum class NestKind {
    CLONE = 0,
    JELLY = 1,
    SYMBOL_GOOD = 2,
    SYMBOL_EVIL = 3,
    MIMIC = 4,
    HORROR = 5,
    KENNEL = 6,
    ANIMAL = 7,
    CHAPEL = 8,
    UNDEAD = 9,
    MAX,
};

enum class PitKind {
    ORC = 0,
    TROLL = 1,
    GIANT = 2,
    HORROR = 3,
    SYMBOL_GOOD = 4,
    SYMBOL_EVIL = 5,
    CHAPEL = 6,
    DRAGON = 7,
    DEMON = 8,
    DARK_ELF = 9,
    MAX,
};

enum class PitNestHook {
    NONE,
    CLONE,
    DRAGON,
    SYMBOL,
};

/*! pit/nest型情報の構造体定義 */
enum class MonraceHook;
enum class MonraceId : short;
class PlayerType;
struct nest_pit_type {
    std::string name; //<! 部屋名
    MonraceHook monrace_hook; //<! モンスター種別フィルタ
    PitNestHook pn_hook; //<! シンボル/能力フィルタ
    int level; //<! 相当階
    int chance; //!< 生成確率

    void prepare_filter(PlayerType *player_ptr) const;
};

enum class MonraceId : short;
enum class NestKind;
enum class PitKind;
class PitNestFilter {
public:
    ~PitNestFilter() = default;
    PitNestFilter(const PitNestFilter &) = delete;
    PitNestFilter(PitNestFilter &&) = delete;
    PitNestFilter &operator=(const PitNestFilter &) = delete;
    PitNestFilter &operator=(PitNestFilter &&) = delete;
    static PitNestFilter &get_instance();

    MonraceId get_monrace_id() const;
    char get_monrace_symbol() const;
    const EnumClassFlagGroup<MonsterAbilityType> &get_dragon_breaths() const;
    std::string pit_subtype(PitKind type) const;
    std::string nest_subtype(NestKind type) const;

    void set_monrace_id(MonraceId id);
    void set_monrace_symbol(char symbol);
    void set_dragon_breaths();

private:
    PitNestFilter() = default;

    static PitNestFilter instance;

    MonraceId monrace_id{}; //<! 通常pit/nest生成時のモンスターの構成条件ID.
    char monrace_symbol = '\0'; //!< 単一シンボルpit/nest生成時の指定シンボル.
    EnumClassFlagGroup<MonsterAbilityType> dragon_breaths{}; //!< ブレス属性に基づくドラゴンpit生成時条件マスク.
};

/*! デバッグ時にnestのモンスター情報を確認するための構造体 / A struct for nest monster information with cheat_hear */
class MonraceDefinition;
class NestMonsterInfo {
public:
    NestMonsterInfo() = default;
    MonraceId monrace_id{}; //!< モンスター種族ID
    bool used = false; //!< 既に選んだかどうか
    bool order_nest(const NestMonsterInfo &other) const;
    const MonraceDefinition &get_monrace() const;
};

class FloorType;
class MonsterEntity;
tl::optional<NestKind> pick_nest_type(const FloorType &floor, const std::map<NestKind, nest_pit_type> &np_types);
tl::optional<PitKind> pick_pit_type(const FloorType &floor, const std::map<PitKind, nest_pit_type> &np_types);
tl::optional<MonraceId> select_pit_nest_monrace_id(PlayerType *player_ptr, MonsterEntity &align, int boost);
