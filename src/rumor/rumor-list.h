#pragma once

#include "rumor/rumor-definition.h"
#include "util/enum-range.h"
#include "util/probability-table.h"
#include <filesystem>
#include <map>
#include <string>
#include <utility>
#include <vector>

//!< @details 数値はレアリティの重み。Lowが一番出やすく、Highが一番出にくい.
enum class RumorRarity {
    LOW = 3,
    MEDIUM = 2,
    HIGH = 1,
    MAX,
};

class RumorDefinition;
class RumorList {
public:
    RumorList(RumorList &&) = delete;
    RumorList(const RumorList &) = delete;
    RumorList &operator=(const RumorList &) = delete;
    RumorList &operator=(RumorList &&) = delete;

    static RumorList &get_instance();
    const RumorDefinition &get_random_rumor() const;
    const RumorDefinition &get_rumor(RumorRarity rt) const;

    void read_rumors(const std::filesystem::path &path);

    //!< @todo RumorService::retouch() からのみ呼び出される.
    void add_towns();
    void add_shallow_dungeons();
    void add_normal_monsters();
    void add_shallow_artifacts();
    void add_deep_dungeons();
    void add_unique_monsters();
    void add_deep_artifacts();
    void make_table();

private:
    RumorList();

    static RumorList instance;

    std::map<RumorRarity, std::map<RumorType, std::vector<RumorDefinition>>> rumor_definitions;
    std::map<RumorType, std::string_view> type_template;
    ProbabilityTable<int> rumor_tables;
    std::vector<RumorDefinition> random_rumors;
};
