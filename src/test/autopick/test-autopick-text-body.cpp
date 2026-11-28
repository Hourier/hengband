/*!
 * @brief 自動拾いエディタのテキスト本体のテスト
 */

#include "autopick/autopick-text-body.h"

#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-methods-table.h"
#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <initializer_list>
#include <string>
#include <string_view>
#include <vector>

using namespace test;

namespace {

//! 先頭から lines の行数だけ文字列を埋めた AutopickTextBody を作る.
AutopickTextBody make_text_body(std::initializer_list<std::string> lines)
{
    AutopickTextBody tb(0, 0);
    tb.set_lines(lines);

    return tb;
}

#ifdef JP
/*!
 * @brief 1行だけを持ち、カーソルがその行にあるエディタの状態を作る
 * @param line 行の内容
 * @param cx カーソルの位置 (バイト)
 * @return エディタの状態
 */
AutopickTextBody make_text_body_with_line(std::string_view line, int cx)
{
    auto tb = make_text_body({ std::string(line) });
    tb.cx = cx;
    return tb;
}

//! 行末に多バイト文字の先頭バイトだけが残った行
const std::string LINE_WITH_LONE_LEAD_BYTE = cat("ab", KANJI_KAN.substr(0, 1));
#endif

}

TEST_CASE("adjust_cursor_column clamps the mark column by the line the mark is on")
{
    auto tb = make_text_body({ "aaaaaaaa", "aa" });
    tb.cx = 8;
    tb.marks.set(AutopickMark::MARK);
    tb.my = 1;
    tb.mx = 5;

    tb.adjust_cursor_column();

    CHECK(tb.mx == 2);
    CHECK(tb.cx == 8);
}

TEST_CASE("adjust_cursor_column does not touch the mark column when nothing is marked")
{
    auto tb = make_text_body({ "aaaa" });
    tb.my = 5;
    tb.mx = 10;

    tb.adjust_cursor_column();

    CHECK(tb.mx == 10);
}

TEST_CASE("adjust_cursor_column keeps the mark column within the line shortened by toggling the command letter")
{
    // 行末にマークを置いたまま、破壊の指定を外して行を短くする
    auto tb = make_text_body({ "!aaaa" });
    tb.cx = 5;
    tb.marks.set(AutopickMark::MARK);
    tb.mx = 5;
    tb.toggle_command_letter(AutopickMethod::AUTOPICK);
    REQUIRE(tb.lines[0].line == "aaaa");

    tb.adjust_cursor_column();

    CHECK(tb.mx == 4);
    CHECK(tb.cx == 4);
}

TEST_CASE("adjust_cursor_column puts the cursor at the recorded column within the line")
{
    auto tb = make_text_body({ "aaaaaa" });

    tb.rec_cx = 3;
    tb.adjust_cursor_column();
    CHECK(tb.cx == 3);

    tb.rec_cx = 10;
    tb.adjust_cursor_column();
    CHECK(tb.cx == 6);
}

TEST_CASE("update_cursor_column_record keeps the widest column while moving up and down")
{
    auto tb = make_text_body({ "aaaaaa" });

    tb.update_cursor_column_record(EditorCommandId::UP, 0, 4);
    CHECK(tb.rec_cx == 4);
    tb.update_cursor_column_record(EditorCommandId::DOWN, 0, 2);
    CHECK(tb.rec_cx == 4);
    tb.update_cursor_column_record(EditorCommandId::LEFT, 0, 4);
    CHECK(tb.rec_cx == 0);
}

TEST_CASE("update_cursor_column_record records the column after the command when the command moves the cursor")
{
    auto tb = make_text_body({ "aaaaaa" });

    // BOTTOM はカーソルを行頭へ移すので、移す前の桁は覚えない
    tb.cx = 0;
    tb.update_cursor_column_record(EditorCommandId::BOTTOM, 4, 4);
    CHECK(tb.rec_cx == 0);
}

TEST_CASE("add_empty_line appends an empty line")
{
    auto tb = make_text_body({ "a" });

    CHECK(tb.add_empty_line());
    REQUIRE(tb.lines.size() == 2);
    CHECK(tb.lines[1].line.empty());
    CHECK((tb.dirty_flags & DIRTY_EXPRESSION) != 0);
    CHECK(tb.changed);
}

TEST_CASE("add_empty_line appends an empty line to many lines")
{
    // 以前の行数の上限 (5998 行) を超える行数
    constexpr auto many_lines = 7000;
    AutopickTextBody tb(0, 0);
    tb.set_lines(std::vector<std::string>(many_lines, "a"));

    CHECK(tb.add_empty_line());
    REQUIRE(tb.lines.size() == many_lines + 1);
    CHECK(tb.lines[many_lines].line.empty());
}

TEST_CASE("add_empty_line does not append a line when the last line is empty")
{
    auto tb = make_text_body({ "" });

    CHECK_FALSE(tb.add_empty_line());
    CHECK(tb.lines.size() == 1);
    CHECK_FALSE(tb.changed);
}

TEST_CASE("is_inside_multibyte_char is false for single-byte characters")
{
    for (auto pos = 0; pos <= 3; pos++) {
        CAPTURE(pos);
        CHECK_FALSE(AutopickTextBody::is_inside_multibyte_char("abc", pos));
    }
}

TEST_CASE("set_lines replaces all the lines")
{
    auto tb = make_text_body({ "aa", "bb", "cc" });

    tb.set_lines({ "dd" });
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "dd" });
}

TEST_CASE("set_lines keeps an empty line when no lines are given")
{
    auto tb = make_text_body({ "aa", "bb" });

    tb.set_lines({});
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "" });
}

#ifdef JP
TEST_CASE("is_inside_multibyte_char detects a trailing byte of a multibyte character")
{
    const auto line = cat(KANJI_KAN, "a");

    CHECK_FALSE(AutopickTextBody::is_inside_multibyte_char(line, 0));
    CHECK(AutopickTextBody::is_inside_multibyte_char(line, 1));
    CHECK_FALSE(AutopickTextBody::is_inside_multibyte_char(line, static_cast<int>(KANJI_KAN.length())));
}

TEST_CASE("is_inside_multibyte_char does not treat the end of a line after a lone lead byte as inside a character")
{
    CHECK_FALSE(AutopickTextBody::is_inside_multibyte_char(LINE_WITH_LONE_LEAD_BYTE, 3));
}

TEST_CASE("is_cursor_inside_multibyte_char checks the cursor position on the cursor line")
{
    const auto line = cat(KANJI_KAN, "a");

    CHECK_FALSE(make_text_body_with_line(line, 0).is_cursor_inside_multibyte_char());
    CHECK(make_text_body_with_line(line, 1).is_cursor_inside_multibyte_char());
    CHECK_FALSE(make_text_body_with_line(line, static_cast<int>(KANJI_KAN.length())).is_cursor_inside_multibyte_char());
    CHECK_FALSE(make_text_body_with_line(line, 10).is_cursor_inside_multibyte_char());
}

TEST_CASE("adjust_cursor_column puts the cursor at the recorded display column")
{
    const auto line = cat("a", KANJI_KAN, "b");
    auto tb = make_text_body_with_line(line, 0);

    SUBCASE("the column is on a character boundary")
    {
        tb.rec_cx = 3;
        tb.adjust_cursor_column();
        CHECK(tb.cx == static_cast<int>(1 + KANJI_KAN.length()));
    }

    SUBCASE("the column is the right half of a double-byte character on an even column")
    {
        tb.rec_cx = 2;
        tb.adjust_cursor_column();
        CHECK(tb.cx == static_cast<int>(1 + KANJI_KAN.length()));
    }

    SUBCASE("the column is the right half of a double-byte character on an odd column")
    {
        tb.lines[0].line = cat("ab", KANJI_KAN);
        tb.rec_cx = 3;
        tb.adjust_cursor_column();
        CHECK(tb.cx == 2);
    }
}

TEST_CASE("get_cursor_column counts the display width before the cursor")
{
    const auto line = cat("a", KANJI_KAN, "b");
    CHECK(make_text_body_with_line(line, 0).get_cursor_column() == 0);
    CHECK(make_text_body_with_line(line, static_cast<int>(1 + KANJI_KAN.length())).get_cursor_column() == 3);
    CHECK(make_text_body_with_line(line, 100).get_cursor_column() == 4);
}
#endif
