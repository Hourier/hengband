/*!
 * @brief ユニーク一覧の表示行のテスト
 */

#include "view/unique-monrace-lines.h"

#include "system/monrace/monrace-definition.h"
#include "test/string-helpers.h"

#include <doctest/doctest.h>
#include <fmt/format.h>

#include <string>
#include <vector>

using namespace test;

namespace {

#ifdef JP
constexpr std::string_view LEVEL = "\xe3\x83\xac\xe3\x83\x99\xe3\x83\xab"; //!< レベル
#endif

MonraceDefinition make_monrace(std::string_view name, int level, int defeat_level = 0, REAL_TIME defeat_time = 0)
{
    MonraceDefinition monrace;
    const auto name_text = std::string(name);
    monrace.name = LocalizedString(name_text, name_text);
    monrace.level = level;
    monrace.defeat_level = static_cast<PLAYER_LEVEL>(defeat_level);
    monrace.defeat_time = defeat_time;
    return monrace;
}

std::string level_text(int level)
{
#ifdef JP
    return cat(" (", LEVEL, fmt::format("{:3})", level));
#else
    return fmt::format(" (level {:3})", level);
#endif
}

// 半角だけの名前は表示幅とバイト数が一致するので、残りは空白で埋める
std::string line_of(std::string_view indent, std::string_view name, int level, std::string_view defeat = {})
{
    auto padded = std::string(name);
    if (padded.size() < static_cast<std::size_t>(UniqueMonraceLines::NAME_WIDTH)) {
        padded.append(static_cast<std::size_t>(UniqueMonraceLines::NAME_WIDTH) - padded.size(), ' ');
    }

    return cat(indent, padded, level_text(level), defeat);
}

#ifdef JP
std::string defeat_of(int defeat_level, std::string_view clock)
{
    const auto level_field = fmt::format("{:2}", defeat_level);
    return cat(" - ", LEVEL, level_field, " - ", clock);
}
#else
std::string defeat_of(int defeat_level, std::string_view clock)
{
    return fmt::format(" - level {:2} - {}", defeat_level, clock);
}
#endif

}

TEST_CASE("UniqueMonraceLines pads a short name and omits the defeat time")
{
    // 知識コマンドの生存ユニーク一覧は、5桁の字下げで撃破時刻を出さない
    const auto lines = UniqueMonraceLines("     ", false, 4).format(make_monrace("Orc", 8, 12, 3723));
    CHECK(lines == std::vector<std::string>{ line_of("     ", "Orc", 8) });
}

TEST_CASE("UniqueMonraceLines appends an unpadded defeat time")
{
    // 3723 秒は 1:02:03。知識コマンドの撃破済み一覧は「時」を桁揃えしない
    const auto lines = UniqueMonraceLines("     ", true).format(make_monrace("Orc", 8, 15, 3723));
    CHECK(lines == std::vector<std::string>{ line_of("     ", "Orc", 8, defeat_of(15, "1:02:03")) });
}

TEST_CASE("UniqueMonraceLines right-aligns defeat hours to the given width")
{
    // キャラクターダンプは 2 桁の字下げで、一覧に出す最大の「時」に揃える
    const auto narrow = UniqueMonraceLines("  ", true, 2).format(make_monrace("Orc", 8, 20, 5 * 60 * 60));
    const auto wide = UniqueMonraceLines("  ", true, 3).format(make_monrace("Orc", 8, 20, 10 * 60 * 60));
    CHECK(narrow == std::vector<std::string>{ line_of("  ", "Orc", 8, defeat_of(20, " 5:00:00")) });
    CHECK(wide == std::vector<std::string>{ line_of("  ", "Orc", 8, defeat_of(20, " 10:00:00")) });
}

TEST_CASE("UniqueMonraceLines omits a defeat time that was not recorded")
{
    SUBCASE("the defeat level is zero")
    {
        const auto lines = UniqueMonraceLines("  ", true, 2).format(make_monrace("Orc", 8, 0, 3723));
        CHECK(lines == std::vector<std::string>{ line_of("  ", "Orc", 8) });
    }

    SUBCASE("the defeat time is zero")
    {
        const auto lines = UniqueMonraceLines("  ", true, 2).format(make_monrace("Orc", 8, 15, 0));
        CHECK(lines == std::vector<std::string>{ line_of("  ", "Orc", 8) });
    }
}

TEST_CASE("UniqueMonraceLines keeps a name that fills the column on one line")
{
    const auto name = std::string(UniqueMonraceLines::NAME_WIDTH, 'A');
    const auto lines = UniqueMonraceLines("  ", false).format(make_monrace(name, 3));
    CHECK(lines == std::vector<std::string>{ line_of("  ", name, 3) });
}

TEST_CASE("UniqueMonraceLines wraps a name and indents the following lines")
{
    const auto name = std::string(UniqueMonraceLines::NAME_WIDTH + 1, 'A');
    const auto lines = UniqueMonraceLines("  ", true).format(make_monrace(name, 3, 4, 61));
    const auto head = std::string(name, 0, UniqueMonraceLines::NAME_WIDTH);
    const auto tail = std::string(name, UniqueMonraceLines::NAME_WIDTH);
    CHECK(lines == std::vector<std::string>{
                       line_of("  ", head, 3, defeat_of(4, "0:01:01")),
                       "  " + tail,
                   });
}

TEST_CASE("UniqueMonraceLines prints the level for an empty name")
{
    const auto lines = UniqueMonraceLines("", false).format(make_monrace("", 1));
    CHECK(lines == std::vector<std::string>{ line_of("", "", 1) });
}

#ifdef JP
TEST_CASE("UniqueMonraceLines pads and wraps a multibyte name by display width")
{
    // 漢は UTF-8 で 3 バイト、表示幅 2 桁。バイト数ではなく表示幅で欄を埋める
    const auto short_name = std::string(KANJI_KAN);
    const auto short_lines = UniqueMonraceLines("", true).format(make_monrace(short_name, 4, 5, 65));
    // 表示幅 2 桁なので、欄の残り 38 桁を空白で埋める (バイト数 3 では 37 桁になる)
    const auto short_expected = cat(short_name, std::string(38, ' '), level_text(4), defeat_of(5, "0:01:05"));
    CHECK(short_lines == std::vector<std::string>{ short_expected });

    // 表示幅 42 桁は、欄の 40 桁と残り 2 桁に分かれる
    const auto long_name = repeat(KANJI_KAN, 21);
    const auto long_lines = UniqueMonraceLines("  ", false).format(make_monrace(long_name, 7));
    CHECK(long_lines == std::vector<std::string>{
                            line_of("  ", repeat(KANJI_KAN, 20), 7),
                            cat("  ", KANJI_KAN),
                        });
}
#endif
