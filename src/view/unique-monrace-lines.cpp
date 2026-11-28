/*!
 * @brief ユニーク一覧の1体分の表示行を作る
 */

#include "view/unique-monrace-lines.h"
#include "locale/language-switcher.h"
#include "system/monrace/monrace-definition.h"
#include "util/buffer-shaper.h"
#include <fmt/format.h>

/*!
 * @brief コンストラクタ
 * @param indent 各行の先頭に付ける文字列
 * @param show_defeat_time 撃破した階層と時刻を行末に付けるか
 * @param hour_digits 時刻の「時」を右詰めする桁数。0 以下のときは詰めない
 */
UniqueMonraceLines::UniqueMonraceLines(std::string_view indent, bool show_defeat_time, int hour_digits)
    : indent(indent)
    , show_defeat_time(show_defeat_time)
    , hour_digits(hour_digits)
{
}

/*!
 * @brief 撃破した階層と時刻の文を作る
 * @param monrace 表示するモンスター種族
 * @return 撃破時刻を出さないときは空文字列
 * @details 階層か時刻が 0 のときは、記録が無いものとして空文字列を返す。
 */
std::string UniqueMonraceLines::defeat_text(const MonraceDefinition &monrace) const
{
    if (!this->show_defeat_time || (monrace.defeat_level <= 0) || (monrace.defeat_time == 0)) {
        return {};
    }

    const auto defeat_level = monrace.defeat_level;
    const auto defeat_time = monrace.defeat_time;
    const auto hour = defeat_time / (60 * 60);
    const auto minute = (defeat_time / 60) % 60;
    const auto second = defeat_time % 60;
    if (this->hour_digits > 0) {
        constexpr auto defeat_fmt = _(" - レベル{:2} - {:>{}}:{:02}:{:02}", " - level {:2} - {:>{}}:{:02}:{:02}");
        return fmt::format(defeat_fmt, defeat_level, hour, this->hour_digits, minute, second);
    }

    constexpr auto defeat_fmt = _(" - レベル{:2} - {}:{:02}:{:02}", " - level {:2} - {}:{:02}:{:02}");
    return fmt::format(defeat_fmt, defeat_level, hour, minute, second);
}

/*!
 * @brief ユニーク1体分の表示行を作る
 * @param monrace 表示するモンスター種族
 * @return 先頭行に名前・レベル・撃破時刻を載せ、名前が続くときは同じ字下げの行を後ろに足した一覧
 */
std::vector<std::string> UniqueMonraceLines::format(const MonraceDefinition &monrace) const
{
    // shape_buffer() は表示幅 maxlen - 1 桁ごとに区切るので、名前の欄の幅 + 1 を渡す
    auto names = shape_buffer(monrace.name, NAME_WIDTH + 1);
    if (names.empty()) {
        names.emplace_back();
    }

    const auto defeat = this->defeat_text(monrace);
    constexpr auto line_fmt = _("{}{:<40} (レベル{:3}){}", "{}{:<40} (level {:3}){}");
    std::vector<std::string> lines;
    lines.reserve(names.size());
    lines.emplace_back(fmt::format(line_fmt, this->indent, names.front(), monrace.level, defeat));
    for (std::size_t i = 1; i < names.size(); ++i) {
        lines.emplace_back(fmt::format("{}{}", this->indent, names[i]));
    }

    return lines;
}
