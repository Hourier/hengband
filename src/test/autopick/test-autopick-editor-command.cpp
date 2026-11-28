/*!
 * @brief 自動拾いエディタのコマンドのテスト
 */

#include "autopick/autopick-editor-command.h"

#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-text-body.h"
#include "autopick/autopick-util.h"
#include "test/string-helpers.h"

#include <doctest/doctest.h>

#include <string>
#include <string_view>
#include <vector>

using namespace test;

#ifdef JP
namespace {

/*!
 * @brief 1行だけを持ち、カーソルがその行にあるエディタの状態を作る
 * @param line 行の内容
 * @param cx カーソルの位置 (バイト)
 * @return エディタの状態
 */
AutopickTextBody make_text_body_with_line(std::string_view line, int cx)
{
    AutopickTextBody tb(0, 0);
    tb.set_lines({ std::string(line) });
    tb.cx = cx;
    return tb;
}

//! 行末に多バイト文字の先頭バイトだけが残った行
const std::string LINE_WITH_LONE_LEAD_BYTE = cat("ab", KANJI_KAN.substr(0, 1));

}
#endif

TEST_CASE("EditorCommandId::BACKSPACE at the beginning of a line joins it to the previous line")
{
    AutopickTextBody tb(0, 1);
    tb.set_lines({ "ab", "cd", "ef" });

    do_editor_command(nullptr, &tb, EditorCommandId::BACKSPACE);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "abcd", "ef" });
    CHECK(tb.cx == 2);
    CHECK(tb.cy == 0);
}

TEST_CASE("EditorCommandId::DOWN on the last line appends an empty line")
{
    AutopickTextBody tb(0, 0);
    tb.set_lines({ "a" });

    do_editor_command(nullptr, &tb, EditorCommandId::DOWN);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "" });
    CHECK(tb.cy == 1);
}

TEST_CASE("EditorCommandId::DOWN on an empty last line stays there")
{
    AutopickTextBody tb(0, 1);
    tb.set_lines({ "a", "" });

    do_editor_command(nullptr, &tb, EditorCommandId::DOWN);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "" });
    CHECK(tb.cy == 1);
}

TEST_CASE("EditorCommandId::BOTTOM appends an empty line after a non-empty last line and moves there")
{
    AutopickTextBody tb(1, 0);
    tb.set_lines({ "a", "b" });

    do_editor_command(nullptr, &tb, EditorCommandId::BOTTOM);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "b", "" });
    CHECK(tb.cy == 2);
    CHECK(tb.cx == 0);
}

TEST_CASE("EditorCommandId::BOTTOM moves to an empty last line without appending a line")
{
    AutopickTextBody tb(1, 0);
    tb.set_lines({ "a", "" });

    do_editor_command(nullptr, &tb, EditorCommandId::BOTTOM);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "" });
    CHECK(tb.cy == 1);
    CHECK(tb.cx == 0);
}

TEST_CASE("EditorCommandId::CUT of lines up to the last line removes them and empties the last line")
{
    AutopickTextBody tb(1, 2);
    tb.set_lines({ "a", "b", "c" });
    tb.marks.set(AutopickMark::MARK);
    tb.my = 1;
    tb.mx = 0;

    do_editor_command(nullptr, &tb, EditorCommandId::CUT);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "a", "" });
    CHECK(tb.cy == 1);
    CHECK(tb.cx == 0);
}

TEST_CASE("EditorCommandId::PASTE of whole lines inserts them before the rest of the line")
{
    AutopickTextBody tb(1, 0);
    tb.set_lines({ "ab" });
    tb.yank = { "x", "y" };
    tb.yank_eol = true;

    do_editor_command(nullptr, &tb, EditorCommandId::PASTE);
    CHECK(tb.get_entry_lines() == std::vector<std::string>{ "ax", "y", "b" });
    CHECK(tb.cy == 2);
    CHECK(tb.cx == 0);
}

#ifdef JP
TEST_CASE("EditorCommandId::LEFT moves back one double-byte character at a time")
{
    auto tb = make_text_body_with_line(cat(KANJI_KAN, KANJI_JI), static_cast<int>(KANJI_KAN.length() + KANJI_JI.length()));

    do_editor_command(nullptr, &tb, EditorCommandId::LEFT);
    CHECK(tb.cx == static_cast<int>(KANJI_KAN.length()));
    do_editor_command(nullptr, &tb, EditorCommandId::LEFT);
    CHECK(tb.cx == 0);
}

TEST_CASE("EditorCommandId::PASTE does not split a double-byte character pushed past the line limit")
{
    const int count = (MAX_LINELEN - 4) / static_cast<int>(KANJI_KAN.length());
    const std::string padding(MAX_LINELEN - 3 - count * KANJI_KAN.length(), 'a');
    const auto line = padding + repeat(KANJI_KAN, count);
    REQUIRE(line.length() == MAX_LINELEN - 3);
    auto tb = make_text_body_with_line(line, 0);
    tb.yank = { "xyz" };

    do_editor_command(nullptr, &tb, EditorCommandId::PASTE);
    CHECK(tb.lines[0].line == "xyz" + padding + repeat(KANJI_KAN, count - 1));
    CHECK(tb.cx == 3);
}

TEST_CASE("EditorCommandId::LEFT moves back one byte over a lone lead byte at the end of a line")
{
    auto tb = make_text_body_with_line(LINE_WITH_LONE_LEAD_BYTE, 3);

    do_editor_command(nullptr, &tb, EditorCommandId::LEFT);
    CHECK(tb.cx == 2);
}
#endif
