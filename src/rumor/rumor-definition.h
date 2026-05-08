#pragma once
/*!
 * @brief 噂定義
 * @date 2026/05/06
 * @author Hourier
 */

#include <string>
#include <tl/optional.hpp>

enum class RumorType {
    GOSSIP = 0,
    TOWN = 1,
    SHALLOW_DUNGEON = 2,
    NORMAL_MONSTER = 3,
    SHALLOW_ARTIFACT = 4,
    DEEP_DUNGEON = 5,
    UNIQUE_MONSTER = 6,
    DEEP_ARTIFACT = 7,
    MAX,
};

class RumorDefinition {
public:
    RumorDefinition(RumorType type, int id, const std::string &description);

    RumorType get_type() const;
    int get_id() const;
    const std::string &get_description() const;

private:
    RumorType type;
    int id; //!< ダンジョン、モンスター、アーティファクトのID。噂の種類によって意味が異なる.
    std::string description;
};
