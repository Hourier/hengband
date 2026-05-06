#include "rumor/rumor-list.h"
#include "artifact/fixed-art-types.h"
#include "locale/language-switcher.h"
#include "rumor/rumor-definition.h"
#include "system/angband-exceptions.h"
#include "system/artifact/artifact-definition.h"
#include "system/artifact/artifact-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/floor/town-records.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "term/z-rand.h"
#include "util/enum-converter.h"
#include "util/string-processor.h"
#include <fmt/format.h>
#include <fstream>

namespace {
constexpr auto depth_threshold = 30;

constexpr auto name_template = "{Name}";

RumorType select_rumor_type(RumorRarity rt)
{
    switch (rt) {
    case RumorRarity::LOW:
        return i2enum<RumorType>(rand_range(0, 0));
    case RumorRarity::MEDIUM:
        return i2enum<RumorType>(rand_range(1, 4));
    case RumorRarity::HIGH:
        return i2enum<RumorType>(rand_range(5, 7));
    default:
        THROW_EXCEPTION(std::logic_error, fmt::format("Invalid RumorRarity value: {}", enum2i(rt)));
    }
}
}

RumorList RumorList::instance{};

RumorList::RumorList()
{
    this->rumor_definitions[RumorRarity::LOW][RumorType::GOSSIP] = std::vector<RumorDefinition>{};

    auto &medium = this->rumor_definitions[RumorRarity::MEDIUM];
    medium[RumorType::TOWN] = std::vector<RumorDefinition>{};
    medium[RumorType::SHALLOW_DUNGEON] = std::vector<RumorDefinition>{};
    medium[RumorType::SHALLOW_ARTIFACT] = std::vector<RumorDefinition>{};
    medium[RumorType::NORMAL_MONSTER] = std::vector<RumorDefinition>{};

    auto &high = this->rumor_definitions[RumorRarity::HIGH];
    high[RumorType::DEEP_ARTIFACT] = std::vector<RumorDefinition>{};
    high[RumorType::UNIQUE_MONSTER] = std::vector<RumorDefinition>{};
    high[RumorType::DEEP_DUNGEON] = std::vector<RumorDefinition>{};

    this->type_template[RumorType::SHALLOW_DUNGEON] = _("{Name}の場所はココだ： -続く-", "The location of {Name} is here: -more-");
    this->type_template[RumorType::NORMAL_MONSTER] = _("{Name}というモンスターがいるらしい。", "There is a monster called {Name}.");
    this->type_template[RumorType::SHALLOW_ARTIFACT] = _("{Name}というお宝が地下浅くにあるそうだ。", "There is a treasure called {Name} in shallow dungeons.");
    this->type_template[RumorType::DEEP_DUNGEON] = _("{Name}の場所はココだ： -続く-", "The location of {Name} is here: -more-");
    this->type_template[RumorType::UNIQUE_MONSTER] = _("{Name}というユニーク・モンスターがいるらしい。", "There is a unique monster called {Name}.");
    this->type_template[RumorType::DEEP_ARTIFACT] = _("{Name}というお宝が地下深くにあるそうだ。", "There is a treasure called {Name} in deep dungeons.");
    this->type_template[RumorType::TOWN] = _("{Name}という街に行ったことはあるかい？", "Have you ever been to the town of {Name}?");
}

RumorList &RumorList::get_instance()
{
    return instance;
}

const std::string &RumorList::get_random_rumor() const
{
    std::vector<int> selected_rumors;
    this->rumor_tables.lottery(std::back_inserter(selected_rumors), this->rumor_tables, 1);
    return this->random_rumors.at(selected_rumors.front());
}

const std::string &RumorList::get_rumor(RumorRarity rt) const
{
    const auto &rumors_map = this->rumor_definitions.at(rt);
    const auto type = select_rumor_type(rt);
    const auto &rumors = rumors_map.at(type);
    return rumors.at(randint0(rumors.size())).get_description();
}

void RumorList::read_rumors(const std::filesystem::path &path)
{
    auto file = std::ifstream(path);
    if (!file) {
        THROW_EXCEPTION(std::runtime_error, "Failed to open rumor file: " + path.string());
    }

    std::string line;
    while (std::getline(file, line)) {
        line = utf8_to_local(line);
        if (line.empty() || line.starts_with('#')) {
            continue;
        }

        this->rumor_definitions.at(RumorRarity::LOW).at(RumorType::GOSSIP).emplace_back(line);
    }
}

void RumorList::add_towns()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::TOWN);
    const auto &towns = TownList::get_instance();
    for (auto i = 0; i < std::ssize(towns); ++i) {
        if (i != SECRET_TOWN) {
            const auto town_name = str_replace(this->type_template.at(RumorType::TOWN), name_template, towns.get_town(i).get_name());
            rumors.push_back(town_name);
        }
    }
}

void RumorList::add_shallow_dungeons()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::SHALLOW_DUNGEON);
    for (const auto &[_, dungeon] : DungeonList::get_instance()) {
        if (0 < dungeon->mindepth && dungeon->mindepth <= depth_threshold) {
            const auto dungeon_name = str_replace(this->type_template.at(RumorType::SHALLOW_DUNGEON), name_template, dungeon->name);
            rumors.push_back(dungeon_name);
        }
    }
}

void RumorList::add_normal_monsters()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::NORMAL_MONSTER);
    for (const auto &name : MonraceList::get_instance().get_normal_monster_names()) {
        const auto monster_name = str_replace(this->type_template.at(RumorType::NORMAL_MONSTER), name_template, name.string());
        rumors.push_back(monster_name);
    }
}

/*!
 * @brief 地下浅くにあるアーティファクトの噂を追加する
 */
void RumorList::add_shallow_artifacts()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::MEDIUM).at(RumorType::SHALLOW_ARTIFACT);
    for (const auto &[id, artifact] : ArtifactList::get_instance()) {
        if (id == FixedArtifactId::NONE) {
            continue;
        }

        if (artifact.level <= depth_threshold) {
            const auto artifact_name = str_replace(this->type_template.at(RumorType::SHALLOW_ARTIFACT), name_template, artifact.get_full_name());
            rumors.push_back(artifact_name);
        }
    }
}

void RumorList::add_deep_dungeons()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::HIGH).at(RumorType::DEEP_DUNGEON);
    for (const auto &[_, dungeon] : DungeonList::get_instance()) {
        if (dungeon->mindepth > depth_threshold) {
            const auto dungeon_name = str_replace(this->type_template.at(RumorType::DEEP_DUNGEON), name_template, dungeon->name);
            rumors.push_back(dungeon_name);
        }
    }
}

void RumorList::add_unique_monsters()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::HIGH).at(RumorType::UNIQUE_MONSTER);
    for (const auto &name : MonraceList::get_instance().get_unique_monster_names()) {
        const auto monster_name = str_replace(this->type_template.at(RumorType::UNIQUE_MONSTER), name_template, name.string());
        rumors.push_back(monster_name);
    }
}

/*!
 * @brief 地下深くにあるアーティファクトの噂を追加する
 */
void RumorList::add_deep_artifacts()
{
    auto &rumors = this->rumor_definitions.at(RumorRarity::HIGH).at(RumorType::DEEP_ARTIFACT);
    for (const auto &[id, artifact] : ArtifactList::get_instance()) {
        if (artifact.level > depth_threshold) {
            const auto artifact_name = str_replace(this->type_template.at(RumorType::DEEP_ARTIFACT), name_template, artifact.get_full_name());
            rumors.push_back(artifact_name);
        }
    }
}

void RumorList::make_table()
{
    auto i = 0;
    for (const auto &[rarity, rumors_map] : this->rumor_definitions) {
        for (const auto &[type, rumors] : rumors_map) {
            for (const auto &rumor : rumors) {
                this->rumor_tables.entry_item(i, enum2i(rarity));
                this->random_rumors.push_back(rumor.get_description());
                i++;
            }
        }
    }
}
