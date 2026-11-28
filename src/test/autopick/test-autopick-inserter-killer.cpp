/*!
 * @brief 自動拾いエディタの行の挿入と削除のテスト
 */

#include "autopick/autopick-inserter-killer.h"

#include "autopick/autopick-text-body.h"
#include "autopick/autopick-util.h"
#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

using namespace test;

namespace {

//! 以前の行数の上限 (5998 行) を超える行数
constexpr auto MANY_LINES = 7000;

/*!
 * @brief 空でない行を指定の行数だけ持つエディタの状態を作る
 * @param num_lines 行数
 * @return エディタの状態
 */
AutopickTextBody make_text_body(int num_lines)
{
    AutopickTextBody tb(0, 0);
    tb.set_lines(std::vector<std::string>(num_lines, "a"));

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
    auto tb = make_text_body(1);
    tb.lines[0].line = line;
    tb.cx = cx;
    return tb;
}

#ifdef JP
//! 行末に多バイト文字の先頭バイトだけが残った行
const std::string LINE_WITH_LONE_LEAD_BYTE = cat("ab", KANJI_KAN.substr(0, 1));
#endif

}

TEST_CASE("insert_return_code splits the line at the cursor")
{
    auto tb = make_text_body_with_line("abcd", 2);

    insert_return_code(&tb);
    CHECK(tb.lines[0].line == "ab");
    REQUIRE(tb.lines.size() == 2);
    CHECK(tb.lines[1].line == "cd");
}

TEST_CASE("insert_return_code shifts the following lines down")
{
    AutopickTextBody tb(1, 1);
    tb.set_lines({ "ab", "cd", "ef" });

    insert_return_code(&tb);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ab", "c", "d", "ef" });
}

TEST_CASE("insert_return_code splits the last line of many lines")
{
    std::vector<std::string> lines(MANY_LINES - 1, "a");
    lines.push_back("bc");
    AutopickTextBody tb(1, MANY_LINES - 1);
    tb.set_lines(lines);

    insert_return_code(&tb);
    lines.back() = "b";
    lines.push_back("c");
    CHECK(tb.get_entry_lines() == lines);
}

TEST_CASE("kill_line_segment removes a whole line and shifts the following lines up")
{
    AutopickTextBody tb(0, 0);
    tb.set_lines({ "ab", "cd", "ef" });

    kill_line_segment(&tb, 1, 0, 2, true);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ab", "ef" });
}

TEST_CASE("kill_line_segment empties the last line instead of removing it")
{
    AutopickTextBody tb(0, 0);
    tb.set_lines({ "ab", "cd" });

    kill_line_segment(&tb, 1, 0, 2, true);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ab", "" });
}

#ifdef JP
TEST_CASE("insert_return_code does not split a double-byte character")
{
    auto tb = make_text_body_with_line(cat(KANJI_KAN, KANJI_JI), 1);

    insert_return_code(&tb);
    CHECK(tb.lines[0].line == KANJI_KAN);
    REQUIRE(tb.lines.size() == 2);
    CHECK(tb.lines[1].line == KANJI_JI);
}

TEST_CASE("insert_return_code splits a line ending with a lone lead byte at its end")
{
    auto tb = make_text_body_with_line(LINE_WITH_LONE_LEAD_BYTE, 3);

    insert_return_code(&tb);
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

    insert_single_letter(&tb, 'x');
    CHECK(tb.lines[0].line == "x" + padding + repeat(KANJI_KAN, count - 1));
}
#endif
