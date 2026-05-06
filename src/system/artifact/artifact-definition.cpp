#include "system/artifact/artifact-definition.h"
#include "artifact/fixed-art-types.h"
#include "object/tval-types.h"
#include "system/baseitem/baseitem-definition.h"
#include "system/baseitem/baseitem-list.h"
#ifdef JP
#else
#include "util/string-processor.h"
#endif

ArtifactDefinition::ArtifactDefinition()
    : bi_key(BaseitemKey(ItemKindType::NONE))
{
}

bool ArtifactDefinition::can_generate(const BaseitemKey &generating_bi_key) const
{
    if (this->gen_flags.has(ItemGenerationTraitType::QUESTITEM)) {
        return false;
    }

    if (this->is_instant_artifact()) {
        return false;
    }

    return this->bi_key == generating_bi_key;
}

bool ArtifactDefinition::is_instant_artifact() const
{
    return this->gen_flags.has(ItemGenerationTraitType::INSTA_ART);
}

std::string ArtifactDefinition::get_full_name() const
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
