#include "system/artifact-type-definition.h"
#include "artifact/fixed-art-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#include "system/item-entity.h"
#include "util/string-processor.h"
#include <sstream>

ArtifactType::ArtifactType()
    : bi_key(BaseitemKey(ItemKindType::NONE))
{
}

/*!
 * @brief アーティファクトが生成可能か否かを確認する
 * @param bi_key 生成しようとするアーティファクトのベースアイテムキー
 * @param level プレイヤーが今いる階層
 */
bool ArtifactType::can_generate(const BaseitemKey &generaing_bi_key) const
{
    if (this->is_generated) {
        return false;
    }

    if (this->gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
        return false;
    }

    if (this->gen_flags.has(ItemGenerationTraitType::INSTA_ART)) {
        return false;
    }

    return this->bi_key == generaing_bi_key;
}

/*!
 * @brief INSTA_ART型の固定アーティファクト生成を試みる
 * @param 生成基準階層 (現在フロアそのものではなくボーナスつき)
 * @param fa_id 固定アーティファクトID
 * @return 生成に成功したらそのアイテム、失敗したらnullopt
 */
tl::optional<BaseitemKey> ArtifactType::try_make_instant_artifact(int making_level) const
{
    if (!this->can_make_instant_artifact()) {
        return tl::nullopt;
    }

    if (!this->evaluate_shallow_instant_artifact(making_level)) {
        return tl::nullopt;
    }

    if (!this->evaluate_rarity()) {
        return tl::nullopt;
    }

    if (!this->evaluate_shallow_baseitem(making_level)) {
        return tl::nullopt;
    }

    return this->bi_key;
}

std::string ArtifactType::get_full_name() const
{
#ifdef JP
    constexpr auto start = "『";
    constexpr auto end = "』";
    if (this->flags.has(tr_type::TR_FULL_NAME)) {
        return this->name;
    }

    if (this->name.starts_with(start)) {
        std::stringstream ss;
        const auto &baseitems = BaseitemList::get_instance();
        ss << baseitems.lookup_baseitem(this->bi_key).name << this->name;
        return ss.str();
    }

    std::stringstream ss;
    const auto &baseitems = BaseitemList::get_instance();
    ss << start << this->name << baseitems.lookup_baseitem(this->bi_key).name << end;
    return ss.str();
#else
    constexpr auto definite_article_lower = "the ";
    const auto singular_name_with_plural = str_replace(this->name, "&", "");
    const auto singular_name = str_replace(singular_name_with_plural, "~", "");
    constexpr auto start = "'";
    constexpr auto end = "'";
    if (this->flags.has(tr_type::TR_FULL_NAME)) {
        std::stringstream ss;
        ss << definite_article_lower << singular_name;
        return ss.str();
    }

    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem = baseitems.lookup_baseitem(this->bi_key);
    const auto baseitem_name_with_plural = str_replace(baseitem.name, "&", "");
    const auto baseitem_full_name = str_replace(baseitem_name_with_plural, "~", "");
    if (this->name.starts_with(start)) {
        std::stringstream ss;
        ss << definite_article_lower << baseitem_full_name << singular_name;
        return ss.str();
    }

    constexpr auto definite_article_upper = "The ";
    std::stringstream ss;
    ss << start << definite_article_upper << baseitem_full_name << singular_name << end;
    return ss.str();
#endif
}

/*!
 * @brief INSTA_ARTフラグ付きアーティファクトの生成可否を判定する
 * @return 生成可否
 * @details 生成済、クエスト属性付き、非INSTA_ARTはfalse、普通のINSTA_ARTはtrue
 */
bool ArtifactType::can_make_instant_artifact() const
{
    auto can_make = !this->is_generated;
    can_make &= this->gen_flags.has_not(ItemGenerationTraitType::QUESTITEM);
    can_make &= this->gen_flags.has(ItemGenerationTraitType::INSTA_ART);
    return can_make;
}

/*!
 * @brief 標準生成階層より浅い階層での生成制限を判定する
 * @return 生成可否
 * @details 1/(不足階層*2) を満たさないと生成しない
 */
bool ArtifactType::evaluate_shallow_instant_artifact(int making_level) const
{
    if (this->level <= making_level) {
        return true;
    }

    return one_in_((this->level - making_level) * 2);
}

/*!
 * @brief レアリティによる生成制限を判定する
 * @return 生成可否
 */
bool ArtifactType::evaluate_rarity() const
{
    return one_in_(this->rarity);
}

/*!
 * @brief 標準生成階層より浅い階層でのベースアイテム生成制限を判定する
 * @return 生成可否
 * @details 1/(不足階層*5) を満たさないと生成しない
 */
bool ArtifactType::evaluate_shallow_baseitem(int making_level) const
{
    const auto &baseitems = BaseitemList::get_instance();
    const auto &baseitem = baseitems.lookup_baseitem(this->bi_key);
    if (baseitem.level <= making_level) {
        return true;
    }

    return one_in_((baseitem.level - making_level) * 5);
}

ArtifactList ArtifactList::instance{};

ArtifactType ArtifactList::dummy{};

ArtifactList &ArtifactList::get_instance()
{
    return instance;
}

const ArtifactType &ArtifactList::get_artifact(const FixedArtifactId fa_id) const
{
    if (fa_id == FixedArtifactId::NONE) {
        return dummy;
    }

    return this->artifacts.at(fa_id);
}

ArtifactType &ArtifactList::get_artifact(const FixedArtifactId fa_id)
{
    if (fa_id == FixedArtifactId::NONE) {
        return dummy;
    }

    return this->artifacts.at(fa_id);
}

bool ArtifactList::order(const FixedArtifactId id1, const FixedArtifactId id2) const
{
    const auto &artifact1 = this->get_artifact(id1);
    const auto &artifact2 = this->get_artifact(id2);
    if (artifact1.bi_key < artifact2.bi_key) {
        return true;
    }

    if (artifact1.bi_key > artifact2.bi_key) {
        return false;
    }

    if (artifact1.level < artifact2.level) {
        return true;
    }

    if (artifact1.level > artifact2.level) {
        return false;
    }

    return id1 < id2;
}

void ArtifactList::emplace(const FixedArtifactId fa_id, ArtifactType &&artifact)
{
    this->artifacts.emplace(fa_id, std::move(artifact));
}

void ArtifactList::reset_generated_flags()
{
    for (auto &[_, artifact] : this->artifacts) {
        artifact.is_generated = false;
    }
}

tl::optional<ItemEntity> ArtifactList::try_make_instant_artifact(int making_level) const
{
    for (const auto &[fa_id, artifact] : this->artifacts) {
        const auto bi_key = artifact.try_make_instant_artifact(making_level);
        if (bi_key) {
            ItemEntity instant_artifact(*bi_key);
            instant_artifact.fa_id = fa_id;
            return instant_artifact;
        }
    }

    return tl::nullopt;
}
