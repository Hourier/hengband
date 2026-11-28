#include "autopick/autopick-text-body.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-editor-util.h"
#include "locale/multibyte-char.h"
#include <algorithm>
#include <iterator>
#include <ranges>
#include <set>
#include <utility>

AutopickTextBody::AutopickTextBody(int cx, int cy)
    : cx(cx)
    , cy(cy)
    , lines(1)
{
}

namespace {
/*!
 * @brief 行の中で表示桁にあたる位置を返す
 * @param line 行
 * @param column 表示桁
 * @return 表示桁にあたる位置 (バイト)。表示桁が全角文字の右半分にあたるときは、表示桁が奇数ならその文字の先頭、
 * 偶数なら次の文字の先頭 (描画の前にカーソルを文字の境界へ動かすときと同じ)。行より右なら行の長さ
 */
int find_column_position(std::string_view line, int column)
{
    size_t pos = 0;
    auto col = 0;
    while ((pos < line.length()) && (col < column)) {
        const auto ch = mb_first_char(line.substr(pos));
        const auto width = static_cast<int>(str_display_width(ch));
        if (col + width > column) {
            return static_cast<int>((column & 1) ? pos : pos + ch.length());
        }

        pos += ch.length();
        col += width;
    }

    return static_cast<int>(pos);
}
}

/*!
 * @brief コマンドを処理する前にカーソル列とマーク列を行の範囲内に収める
 * @details 上下に移動している間は、覚えておいた表示桁にカーソルを置く。
 * キーワードの切り替え等はマークを残したまま行を短くするため、マーク中はマーク列も補正する。
 * マークを外した後の my / mx は存在しない行を指していることがあるため触らない。
 */
void AutopickTextBody::adjust_cursor_column()
{
    if (this->rec_cx > 0) {
        this->cx = find_column_position(this->lines[this->cy].line, this->rec_cx);
    }

    this->cx = std::min<int>(this->lines[this->cy].line.length(), this->cx);
    if (this->marks.any()) {
        this->mx = std::min<int>(this->lines[this->my].line.length(), this->mx);
    }
}

/*!
 * @brief 上下に移動するコマンドなら、カーソルの表示桁を覚える
 * @param com_id 処理したコマンド
 * @param cx_before コマンドを処理する前のカーソルの位置 (バイト)
 * @param column_before コマンドを処理する前のカーソルの表示桁
 * @details カーソルの位置を変えないコマンド (上下の移動) なら処理する前の行での表示桁を、
 * 変えたコマンド (行頭へ移る BOTTOM) なら移った先の行での表示桁を覚える。
 */
void AutopickTextBody::update_cursor_column_record(EditorCommandId com_id, int cx_before, int column_before)
{
    static const std::set<EditorCommandId> record_cursor_column_commands = { EditorCommandId::UP, EditorCommandId::DOWN, EditorCommandId::PGUP, EditorCommandId::PGDOWN, EditorCommandId::TOP, EditorCommandId::BOTTOM };
    if (!record_cursor_column_commands.contains(com_id)) {
        this->rec_cx = 0;
        return;
    }

    const auto column = (this->cx == cx_before) ? column_before : this->get_cursor_column();
    this->rec_cx = std::max(column, this->rec_cx);
}

/*!
 * @brief カーソルの表示桁を返す
 * @return カーソルより左にある文字の表示幅
 */
int AutopickTextBody::get_cursor_column() const
{
    const std::string_view line(this->lines[this->cy].line);
    return static_cast<int>(str_display_width(line.substr(0, std::min<size_t>(this->cx, line.length()))));
}

/*!
 * @brief カーソルが多バイト文字の途中にあるかどうかを判定する
 * @return カーソルのある行で、カーソルの位置が多バイト文字の後続のバイトであれば true
 */
bool AutopickTextBody::is_cursor_inside_multibyte_char() const
{
    return is_inside_multibyte_char(this->lines[this->cy].line, this->cx);
}

/*!
 * @brief 指定の行があるかどうかを判定する
 * @param y 行番号
 * @return 指定の行があれば true
 */
bool AutopickTextBody::has_line(int y) const
{
    return (0 <= y) && (y < std::ssize(this->lines));
}

/*!
 * @brief 指定の行が最終行かどうかを判定する
 * @param y 行番号
 * @return 指定の行より後ろに行が無ければ true
 */
bool AutopickTextBody::is_last_line(int y) const
{
    return !this->has_line(y + 1);
}

/*!
 * @brief 行をすべて差し替える
 * @param new_lines 新しい行
 * @details 行が無ければ空行を 1 行だけ持つ。各行の状態は初期化するので、描き直しの前に DIRTY_EXPRESSION で求め直すこと。
 */
void AutopickTextBody::set_lines(std::vector<std::string> new_lines)
{
    this->lines.clear();
    this->lines.reserve(new_lines.size());
    for (auto &line : new_lines) {
        this->lines.emplace_back().line = std::move(line);
    }

    if (this->lines.empty()) {
        this->lines.emplace_back();
    }
}

std::vector<std::string> AutopickTextBody::get_entry_lines() const
{
    return this->lines | std::views::transform(&AutopickEntryLine::line) | std::ranges::to<std::vector>();
}
