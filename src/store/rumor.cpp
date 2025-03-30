#include "store/rumor.h"
#include "flavor/flavor-describer.h"
#include "flavor/object-flavor-types.h"
#include "io/files-util.h"
#include "io/tokenizer.h"
#include "object-enchant/special-object-flags.h"
#include "system/angband-exceptions.h"
#include "system/artifact-type-definition.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/dungeon/dungeon-definition.h"
#include "system/dungeon/dungeon-list.h"
#include "system/dungeon/dungeon-record.h"
#include "system/floor/town-info.h"
#include "system/floor/town-list.h"
#include "system/item-entity.h"
#include "system/monrace/monrace-definition.h"
#include "system/monrace/monrace-list.h"
#include "system/player-type-definition.h"
#include "view/display-messages.h"
#include "world/world.h"
#include <algorithm>
#include <cstdlib>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <utility>

/*
 * @brief 固定アーティファクト、モンスター、町 をランダムに1つ選ぶ
 * @param zz 検索文字列
 * @param max_idx briefに挙げた各リストにおける最大数
 * @details rumor.txt (rumor_j.txt) の定義により、常にランダム ("*")。但し拡張性のため固定値の場合も残す.
 */
template <typename T>
static T get_rumor_num(std::string_view zz, short max_idx)
{
    if (zz == "*") {
        return static_cast<T>(randint1(max_idx));
    }

    return static_cast<T>(std::stoi(zz.data()));
}

static std::string bind_rumor_name(std::string_view base, std::string_view item_name)
{
    if (const auto pos = base.find("{Name}");
        pos != std::string::npos) {
        const auto head = base.substr(0, pos);
        const auto tail = base.substr(pos + 6);
        std::stringstream ss;
        ss << head << item_name << tail;
        return ss.str();
    }

    return std::string(base);
}

/*
 * @brief 噂の、町やモンスターを表すトークンを得る
 * @param rumor rumor.txt (rumor_j.txt)の1行
 * @return トークン読み込み成否 とトークン群の配列
 * @todo tmp_tokensを使わず単なるsplitにすればもっと簡略化できそう
 */
static std::optional<std::vector<std::string>> get_rumor_tokens(std::string rumor)
{
    constexpr auto num_tokens = 3;
    char *tmp_tokens[num_tokens];
    if (tokenize(rumor.data() + 2, num_tokens, tmp_tokens, TOKENIZE_CHECKQUOTE) != num_tokens) {
        msg_print(_("この情報は間違っている。", "This information is wrong."));
        return std::nullopt;
    }

    return std::vector<std::string>(std::begin(tmp_tokens), std::end(tmp_tokens));
}

/*!
 * @brief 固定アーティファクト番号とその定義を、ランダムに抽選する
 * @param artifact_name rumor.txt (rumor_j.txt)の定義により、常に"*" (ランダム)
 */
static std::pair<FixedArtifactId, const ArtifactType *> get_artifact_definition(std::string_view artifact_name)
{
    const auto &artifacts = ArtifactList::get_instance();
    const auto max_idx = enum2i(artifacts.rbegin()->first);
    const auto fa_id = get_rumor_num<FixedArtifactId>(artifact_name.data(), max_idx);
    const auto &artifact = artifacts.get_artifact(fa_id);
    return { fa_id, &artifact };
}

void display_rumor(PlayerType *player_ptr, bool ex)
{
    int section = (ex && (randint0(3) == 0)) ? 1 : 0;
#ifdef JP
    auto opt_rumor = get_random_line_ja_only("rumors_j.txt", section, 10);
#else
    auto opt_rumor = get_random_line("rumors.txt", section);
#endif
    std::string rumor;
    if (opt_rumor) {
        rumor = std::move(*opt_rumor);
    } else {
        rumor = _("嘘の噂もある。", "Some rumors are wrong.");
    }

    if (!rumor.starts_with("R:")) {
        msg_print(rumor);
        return;
    }

    const auto tokens = get_rumor_tokens(rumor);
    if (!tokens) {
        return;
    }

    std::string rumor_format;
    std::string full_name;
    const auto &category = tokens->at(0);
    if (category == "ARTIFACT") {
        const auto &artifact_name = tokens->at(1);
        const auto &[a_idx, a_ptr] = get_artifact_definition(artifact_name);
        const auto bi_id = BaseitemList::get_instance().lookup_baseitem_id(a_ptr->bi_key);
        ItemEntity item(bi_id);
        item.fa_id = a_idx;
        item.ident = IDENT_STORE;
        full_name = describe_flavor(player_ptr, item, OD_NAME_ONLY);
    } else if (category == "MONSTER") {
        const auto &monster_name = tokens->at(1);

        // @details プレイヤーもダミーで入っているので、1つ引いておかないと数が合わなくなる.
        auto &monraces = MonraceList::get_instance();
        const auto monraces_size = static_cast<short>(monraces.size() - 1);
        auto monrace_id = get_rumor_num<MonraceId>(monster_name, monraces_size);
        auto &monrace = monraces.get_monrace(monrace_id);
        full_name = monrace.name;
        if (!monrace.r_sights) {
            monrace.r_sights++;
        }
    } else if (category == "DUNGEON") {
        const auto &dungeons = DungeonList::get_instance();
        const auto dungeon_id = i2enum<DungeonId>(std::stoi(tokens->at(1)));
        const auto &dungeon = dungeons.get_dungeon(dungeon_id);
        full_name = dungeon.name;
        auto &dungeon_record = DungeonRecords::get_instance().get_record(dungeon_id);
        if (!dungeon_record.has_entered()) {
            dungeon_record.set_max_level(dungeon.mindepth);
            rumor_format = _("%sに帰還できるようになった。", "You can recall to %s.");
        }
    } else if (category == "TOWN") {
        short town_id;
        const auto &town_name = tokens->at(1);
        while (true) {
            town_id = get_rumor_num<short>(town_name, VALID_TOWNS);
            if (!towns_info[town_id].name.empty()) {
                break;
            }
        }

        full_name = towns_info[town_id].name;
        const auto visit = 1U << (town_id - 1);
        if ((town_id != SECRET_TOWN) && !(player_ptr->visit & visit)) {
            player_ptr->visit |= visit;
            rumor_format = _("%sに行ったことがある気がする。", "You feel you have been to %s.");
        }
    } else {
        THROW_EXCEPTION(std::runtime_error, "Unknown token exists in rumor.txt");
    }

    const auto rumor_msg = bind_rumor_name(tokens->at(2), full_name);
    msg_print(rumor_msg);
    if (!rumor_format.empty()) {
        msg_print(nullptr);
        msg_format(rumor_format.data(), full_name.data());
    }
}
