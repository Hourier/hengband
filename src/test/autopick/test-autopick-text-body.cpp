/*!
 * @brief 自動拾いエディタのテキスト本体のテスト
 */

#include "autopick/autopick-text-body.h"

#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-util.h"
#include "io/input-key-acceptor.h"
#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <initializer_list>
#include <string>
#include <string_view>
#include <utility>
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

#ifdef JP
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

TEST_CASE("insert_return_code splits the line at the cursor")
{
    auto tb = make_text_body_with_line("abcd", 2);

    tb.insert_return_code();
    CHECK(tb.lines[0].line == "ab");
    REQUIRE(tb.lines.size() == 2);
    CHECK(tb.lines[1].line == "cd");
}

TEST_CASE("insert_return_code shifts the following lines down")
{
    AutopickTextBody tb(1, 1);
    tb.set_lines({ "ab", "cd", "ef" });

    tb.insert_return_code();
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ab", "c", "d", "ef" });
}

TEST_CASE("insert_return_code splits the last line of many lines")
{
    // 以前の行数の上限 (5998 行) を超える行数
    constexpr auto many_lines = 7000;
    std::vector<std::string> lines(many_lines - 1, "a");
    lines.push_back("bc");
    AutopickTextBody tb(1, many_lines - 1);
    tb.set_lines(lines);

    tb.insert_return_code();
    lines.back() = "b";
    lines.push_back("c");
    CHECK(tb.get_entry_lines() == lines);
}

TEST_CASE("kill_line_segment removes a whole line and shifts the following lines up")
{
    AutopickTextBody tb(0, 0);
    tb.set_lines({ "ab", "cd", "ef" });

    tb.kill_line_segment(1, 0, 2, true);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ab", "ef" });
}

TEST_CASE("kill_line_segment empties the last line instead of removing it")
{
    AutopickTextBody tb(0, 0);
    tb.set_lines({ "ab", "cd" });

    tb.kill_line_segment(1, 0, 2, true);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ab", "" });
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

TEST_CASE("analyze_move_key returns NONE for a key that is not a special key")
{
    auto tb = make_text_body({ "abcd" });
    tb.marks.set(AutopickMark::MARK);

    CHECK(tb.analyze_move_key('a') == EditorCommandId::NONE);
    CHECK(tb.marks.has(AutopickMark::MARK));
    CHECK(tb.dirty_flags == 0);
}

TEST_CASE("analyze_move_key returns NONE for an unknown special key")
{
    auto tb = make_text_body({ "abcd" });

    CHECK(tb.analyze_move_key(SKEY_MASK) == EditorCommandId::NONE);
    CHECK(tb.marks.none());
}

TEST_CASE("analyze_move_key maps cursor keys to movement commands")
{
    auto tb = make_text_body({ "abcd" });
    const std::pair<uint32_t, EditorCommandId> keys[] = {
        { SKEY_DOWN, EditorCommandId::DOWN },
        { SKEY_LEFT, EditorCommandId::LEFT },
        { SKEY_RIGHT, EditorCommandId::RIGHT },
        { SKEY_UP, EditorCommandId::UP },
        { SKEY_PGUP, EditorCommandId::PGUP },
        { SKEY_PGDOWN, EditorCommandId::PGDOWN },
        { SKEY_HOME, EditorCommandId::BOL },
        { SKEY_HOME | SKEY_MOD_CONTROL, EditorCommandId::TOP },
        { SKEY_END, EditorCommandId::EOL },
        { SKEY_END | SKEY_MOD_CONTROL, EditorCommandId::BOTTOM },
    };
    for (const auto &[skey, com_id] : keys) {
        CAPTURE(skey);
        CHECK(tb.analyze_move_key(skey) == com_id);
    }

    CHECK(tb.marks.none());
    CHECK(tb.dirty_flags == 0);
}

TEST_CASE("analyze_move_key clears a selection started with Shift")
{
    auto tb = make_text_body({ "abcd" });
    tb.marks.set(AutopickMark::MARK);
    tb.marks.set(AutopickMark::BY_SHIFT);
    tb.my = 0;
    tb.mx = 2;

    CHECK(tb.analyze_move_key(SKEY_LEFT) == EditorCommandId::LEFT);
    CHECK(tb.marks.none());
    CHECK((tb.dirty_flags & DIRTY_ALL) != 0);
    CHECK(tb.my == 0);
    CHECK(tb.mx == 2);
}

TEST_CASE("analyze_move_key keeps a selection that was not started with Shift")
{
    auto tb = make_text_body({ "abcd" });
    tb.marks.set(AutopickMark::MARK);

    CHECK(tb.analyze_move_key(SKEY_LEFT) == EditorCommandId::LEFT);
    CHECK(tb.marks.has(AutopickMark::MARK));
    CHECK(tb.marks.has_not(AutopickMark::BY_SHIFT));
    CHECK(tb.dirty_flags == 0);
}

TEST_CASE("analyze_move_key starts a selection from the cursor when Shift is held")
{
    auto tb = make_text_body_with_line("ab", 10);

    CHECK(tb.analyze_move_key(SKEY_DOWN | SKEY_MOD_SHIFT) == EditorCommandId::DOWN);
    CHECK(tb.marks.has(AutopickMark::MARK));
    CHECK(tb.marks.has(AutopickMark::BY_SHIFT));
    CHECK(tb.my == 0);
    CHECK(tb.mx == 2);
    CHECK(tb.cx == 10);
    CHECK((tb.dirty_flags & DIRTY_ALL) != 0);
}

TEST_CASE("analyze_move_key records the current line when a horizontal Shift move starts a selection")
{
    auto tb = make_text_body_with_line("abcd", 1);

    CHECK(tb.analyze_move_key(SKEY_RIGHT | SKEY_MOD_SHIFT) == EditorCommandId::RIGHT);
    CHECK(tb.dirty_line == 0);
    CHECK((tb.dirty_flags & DIRTY_ALL) == 0);
    CHECK(tb.mx == 1);
}

TEST_CASE("analyze_move_key does not move the mark origin when a selection already exists")
{
    auto tb = make_text_body_with_line("abcd", 3);
    tb.marks.set(AutopickMark::MARK);
    tb.my = 0;
    tb.mx = 1;

    CHECK(tb.analyze_move_key(SKEY_RIGHT | SKEY_MOD_SHIFT) == EditorCommandId::RIGHT);
    CHECK(tb.my == 0);
    CHECK(tb.mx == 1);
    CHECK(tb.dirty_flags == 0);
    CHECK(tb.dirty_line == -1);
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

TEST_CASE("insert_return_code does not split a double-byte character")
{
    auto tb = make_text_body_with_line(cat(KANJI_KAN, KANJI_JI), 1);

    tb.insert_return_code();
    CHECK(tb.lines[0].line == KANJI_KAN);
    REQUIRE(tb.lines.size() == 2);
    CHECK(tb.lines[1].line == KANJI_JI);
}

TEST_CASE("insert_return_code splits a line ending with a lone lead byte at its end")
{
    auto tb = make_text_body_with_line(LINE_WITH_LONE_LEAD_BYTE, 3);

    tb.insert_return_code();
    CHECK(tb.lines[0].line == LINE_WITH_LONE_LEAD_BYTE);
    REQUIRE(tb.lines.size() == 2);
    CHECK(tb.lines[1].line.empty());
}

TEST_CASE("insert_single_letter does not split a double-byte character pushed past the line limit")
{
    // 行の上限 (MAX_LINELEN - 1 バイト) ちょうどの行の先頭に1文字入れると、末尾の「漢」は丸ごと押し出される
    const int count = (MAX_LINELEN - 2) / static_cast<int>(KANJI_KAN.length());
    const std::string padding(MAX_LINELEN - 1 - count * KANJI_KAN.length(), 'a');
    const auto line = padding + repeat(KANJI_KAN, count);
    REQUIRE(line.length() == MAX_LINELEN - 1);
    auto tb = make_text_body_with_line(line, 0);

    tb.insert_single_letter('x');
    CHECK(tb.lines[0].line == "x" + padding + repeat(KANJI_KAN, count - 1));
}
#endif
