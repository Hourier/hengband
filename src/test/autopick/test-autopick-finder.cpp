/*!
 * @brief 自動拾いエディタの検索のテスト
 */

#include "autopick/autopick-text-body.h"

#include "autopick/autopick-dirty-flags.h"

#include <doctest/doctest.h>

namespace {

/*!
 * @brief 「ab」「cd」「xcd」の 3 行を持ち、カーソルが指定の行の先頭にあるエディタの状態を作る
 * @param cy カーソルの行
 * @return エディタの状態
 */
AutopickTextBody make_text_body(int cy)
{
    AutopickTextBody tb(0, cy);
    tb.set_lines({ "ab", "cd", "xcd" });
    return tb;
}

}

TEST_CASE("search_for_string forward moves the cursor to the next matching line")
{
    auto tb = make_text_body(1);

    tb.search_for_string("cd", true);
    CHECK(tb.cy == 2);
    CHECK(tb.cx == 1);
    CHECK((tb.dirty_flags & DIRTY_NOT_FOUND) == 0);
}

TEST_CASE("search_for_string backward moves the cursor to the previous matching line")
{
    auto tb = make_text_body(2);

    tb.search_for_string("ab", false);
    CHECK(tb.cy == 0);
    CHECK(tb.cx == 0);
    CHECK((tb.dirty_flags & DIRTY_NOT_FOUND) == 0);
}

TEST_CASE("search_for_string forward from the last line finds nothing")
{
    auto tb = make_text_body(2);

    tb.search_for_string("cd", true);
    CHECK(tb.cy == 2);
    CHECK((tb.dirty_flags & DIRTY_NOT_FOUND) != 0);
}

TEST_CASE("search_for_string backward from the first line finds nothing")
{
    auto tb = make_text_body(0);

    tb.search_for_string("ab", false);
    CHECK(tb.cy == 0);
    CHECK((tb.dirty_flags & DIRTY_NOT_FOUND) != 0);
}

TEST_CASE("search_for_string forward skips a bypassed line and stops at the next active match")
{
    auto tb = make_text_body(0);
    tb.set_lines({ "aa", "cd-off", "cd-on" });
    tb.lines[1].state.set(AutopickEntryStatus::BYPASS);

    tb.search_for_string("cd", true);
    CHECK(tb.cy == 2);
    CHECK(tb.cx == 0);
    CHECK((tb.dirty_flags & DIRTY_SKIP_INACTIVE) != 0);
    CHECK((tb.dirty_flags & DIRTY_INACTIVE) == 0);
}

TEST_CASE("search_for_string forward moves to a bypassed line when it is the only match")
{
    auto tb = make_text_body(0);
    tb.set_lines({ "aa", "cd-off", "zz" });
    tb.lines[1].state.set(AutopickEntryStatus::BYPASS);

    tb.search_for_string("cd", true);
    CHECK(tb.cy == 1);
    CHECK(tb.cx == 0);
    CHECK((tb.dirty_flags & DIRTY_INACTIVE) != 0);
    CHECK((tb.dirty_flags & DIRTY_NOT_FOUND) == 0);
}

TEST_CASE("search_for_string treats a bypassed expression line as an active match")
{
    auto tb = make_text_body(0);
    tb.lines[1].state.set(AutopickEntryStatus::BYPASS);
    tb.lines[1].state.set(AutopickEntryStatus::EXPRESSION);

    tb.search_for_string("cd", true);
    CHECK(tb.cy == 1);
    CHECK(tb.cx == 0);
    CHECK(tb.dirty_flags == 0);
}
