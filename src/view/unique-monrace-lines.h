#pragma once

#include <string>
#include <string_view>
#include <vector>

class MonraceDefinition;

/*!
 * @brief ユニーク一覧の1体分の表示行
 *
 * 知識コマンドのユニーク一覧と、キャラクターダンプの撃破ユニーク一覧は、
 * 名前の折り返し幅・レベルの書き方・撃破時刻の書き方が同じである。
 * 行頭の字下げと、時刻の「時」を何桁で揃えるかだけが違う。
 */
class UniqueMonraceLines {
public:
    static constexpr auto NAME_WIDTH = 40; //!< 名前の欄の表示幅

    UniqueMonraceLines(std::string_view indent, bool show_defeat_time, int hour_digits = 0);
    std::vector<std::string> format(const MonraceDefinition &monrace) const;

private:
    std::string indent;
    bool show_defeat_time;
    int hour_digits;

    std::string defeat_text(const MonraceDefinition &monrace) const;
};
