/*!
 * @brief 自動拾いの設定からアイテムへ銘を刻む処理のテスト
 */

#include "autopick/autopick-util.h"

#include "system/item/item-entity.h"
#include "test/scoped-restore.h"

#include <doctest/doctest.h>

TEST_CASE("auto_inscribe_item returns false for invalid index or empty insc")
{
    const auto restore = test::scoped_restore(autopick_list);
    autopick_list.clear();
    autopick_list.emplace_back(); // insc空のエントリを用意

    ItemEntity item;
    // idx < 0
    CHECK_FALSE(auto_inscribe_item(item, -1));
    CHECK_FALSE(item.is_inscribed());

    // insc空
    CHECK_FALSE(auto_inscribe_item(item, 0));
    CHECK_FALSE(item.is_inscribed());
}

TEST_CASE("auto_inscribe_item does not overwrite existing inscription and returns false")
{
    const auto restore = test::scoped_restore(autopick_list);
    autopick_list.clear();
    autopick_type entry;
    entry.insc = "test-insc";
    autopick_list.push_back(entry);

    ItemEntity item;
    item.inscription = "existing";
    REQUIRE(item.is_inscribed());

    CHECK_FALSE(auto_inscribe_item(item, 0));
    CHECK(item.inscription == "existing"); // 上書きされない
}

TEST_CASE("auto_inscribe_item sets inscription when empty and returns true")
{
    const auto restore = test::scoped_restore(autopick_list);
    autopick_list.clear();
    autopick_type entry;
    entry.insc = "new-insc";
    autopick_list.push_back(entry);

    ItemEntity item;
    REQUIRE_FALSE(item.is_inscribed());

    CHECK(auto_inscribe_item(item, 0));
    CHECK(item.inscription == "new-insc");
}
