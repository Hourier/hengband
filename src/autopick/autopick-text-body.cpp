#include "autopick/autopick-text-body.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-flags-table.h"
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
 * @brief 行の指定の位置が多バイト文字の途中 (先頭バイトより後ろ) かどうかを判定する
 * @param line 行
 * @param pos 位置 (バイト)
 * @return pos より前から多バイト文字が始まり、pos がその文字の後続のバイトであれば true
 * @details
 * 行末に後続のバイトが欠けた文字が残っている場合、その直後 (行の長さの位置) は文字の途中とみなさない。
 * 英語版では常に false を返す。
 */
bool AutopickTextBody::is_inside_multibyte_char(std::string_view line, int pos)
{
    if ((pos <= 0) || std::cmp_greater_equal(pos, line.length())) {
        return false;
    }

    return !is_mb_char_boundary(line, pos);
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

/*!
 * @brief キーワードを付けるか外す
 * @param flg 切り替えるキーワード
 * @details マークが無ければカーソル行だけを、あればマークからカーソルまでの行を対象にする。
 * 付けるか外すかは、最初に解釈できた行がそのキーワードを持っているかで決める。
 */
void AutopickTextBody::toggle_keyword(BIT_FLAGS flg)
{
    int by1, by2;
    bool add = true;
    bool fixed = false;
    if (this->marks.any()) {
        by1 = std::min(this->my, this->cy);
        by2 = std::max(this->my, this->cy);
    } else {
        by1 = by2 = this->cy;
    }

    for (int y = by1; y <= by2; y++) {
        autopick_type an_entry, *entry = &an_entry;
        if (!autopick_new_entry(entry, this->lines[y].line, !fixed)) {
            continue;
        }

        if (!fixed) {
            if (!entry->has(flg)) {
                add = true;
            } else {
                add = false;
            }

            fixed = true;
        }

        if (FLG_NOUN_BEGIN <= flg && flg <= FLG_NOUN_END) {
            int i;
            for (i = FLG_NOUN_BEGIN; i <= FLG_NOUN_END; i++) {
                entry->remove(i);
            }
        } else if (FLG_UNAWARE <= flg && flg <= FLG_STAR_IDENTIFIED) {
            int i;
            for (i = FLG_UNAWARE; i <= FLG_STAR_IDENTIFIED; i++) {
                entry->remove(i);
            }
        } else if (FLG_ARTIFACT <= flg && flg <= FLG_AVERAGE) {
            int i;
            for (i = FLG_ARTIFACT; i <= FLG_AVERAGE; i++) {
                entry->remove(i);
            }
        } else if (FLG_RARE <= flg && flg <= FLG_COMMON) {
            int i;
            for (i = FLG_RARE; i <= FLG_COMMON; i++) {
                entry->remove(i);
            }
        }

        if (add) {
            entry->add(flg);
        } else {
            entry->remove(flg);
        }

        this->lines[y].line = autopick_line_from_entry(*entry);
        this->dirty_flags |= DIRTY_ALL;
        this->changed = true;
    }
}

/*!
 * @brief コマンド文字を切り替える
 * @param am 切り替える動作
 * @details マークが無ければカーソル行だけを、あればマークからカーソルまでの行を対象にする。
 * 付けるか外すかは、最初に解釈できた行がその動作を持っているかで決める。
 * カーソル行では、行頭のコマンド文字の増減に合わせてカーソルを動かす。
 */
void AutopickTextBody::toggle_command_letter(AutopickMethod am)
{
    autopick_type an_entry;
    autopick_type *entry = &an_entry;
    int by1, by2;
    bool add = true;
    bool fixed = false;
    if (this->marks.any()) {
        by1 = std::min(this->my, this->cy);
        by2 = std::max(this->my, this->cy);
    } else {
        by1 = by2 = this->cy;
    }

    for (int y = by1; y <= by2; y++) {
        // メンバーの wid (画面幅) を隠さない。コマンド文字の増減によるカーソルの移動量。
        auto width_diff = 0;
        if (!autopick_new_entry(entry, this->lines[y].line, false)) {
            continue;
        }

        if (!fixed) {
            add = entry->action.has_not(am);
            fixed = true;
        }

        if (entry->action.has(AutopickMethod::NOT_AUTOPICK)) {
            width_diff--;
        } else if (entry->action.has(AutopickMethod::AUTODESTROY)) {
            width_diff--;
        } else if (entry->action.has(AutopickMethod::QUERY_AUTOPICK)) {
            width_diff--;
        }

        if (entry->action.has_not(AutopickMethod::DISPLAY)) {
            width_diff--;
        }

        if (am != AutopickMethod::DISPLAY) {
            entry->action.reset({ AutopickMethod::AUTOPICK, AutopickMethod::NOT_AUTOPICK, AutopickMethod::AUTODESTROY, AutopickMethod::QUERY_AUTOPICK });
            entry->action.set(add ? am : AutopickMethod::AUTOPICK);
        } else {
            entry->action.reset(AutopickMethod::DISPLAY);
            if (add) {
                entry->action.set(am);
            }
        }

        if (this->cy == y) {
            if (entry->action.has(AutopickMethod::NOT_AUTOPICK)) {
                width_diff++;
            } else if (entry->action.has(AutopickMethod::AUTODESTROY)) {
                width_diff++;
            } else if (entry->action.has(AutopickMethod::QUERY_AUTOPICK)) {
                width_diff++;
            }

            if (entry->action.has_not(AutopickMethod::DISPLAY)) {
                width_diff++;
            }

            if (width_diff > 0) {
                this->cx++;
            }

            if (width_diff < 0 && this->cx > 0) {
                this->cx--;
            }
        }

        this->lines[y].line = autopick_line_from_entry(*entry);
        this->dirty_flags |= DIRTY_ALL;
        this->changed = true;
    }
}

/*!
 * @brief キーワードを足す
 * @param flg 足すキーワード
 * @details 既にそのキーワードを持つ行は変えない。名詞のキーワードは、他の名詞を外してから足す。
 */
void AutopickTextBody::add_keyword(BIT_FLAGS flg)
{
    int by1, by2;
    if (this->marks.any()) {
        by1 = std::min(this->my, this->cy);
        by2 = std::max(this->my, this->cy);
    } else {
        by1 = by2 = this->cy;
    }

    for (int y = by1; y <= by2; y++) {
        autopick_type an_entry, *entry = &an_entry;
        if (!autopick_new_entry(entry, this->lines[y].line, false)) {
            continue;
        }

        if (entry->has(flg)) {
            continue;
        }

        if (FLG_NOUN_BEGIN <= flg && flg <= FLG_NOUN_END) {
            int i;
            for (i = FLG_NOUN_BEGIN; i <= FLG_NOUN_END; i++) {
                entry->remove(i);
            }
        }

        entry->add(flg);
        this->lines[y].line = autopick_line_from_entry(*entry);
        this->dirty_flags |= DIRTY_ALL;
        this->changed = true;
    }
}

/*!
 * @brief ファイルの末尾に空行を足す
 * @return 空行を足した場合 true、最終行が既に空の場合は false
 */
bool AutopickTextBody::add_empty_line()
{
    if (this->lines.back().line.empty()) {
        return false;
    }

    this->lines.emplace_back();
    this->dirty_flags |= DIRTY_EXPRESSION;
    this->changed = true;
    return true;
}

/*!
 * @brief ヤンクバッファを空にする
 */
void AutopickTextBody::kill_yank_chain()
{
    this->yank.clear();
    this->yank_eol = true;
}

/*!
 * @brief ヤンクバッファに文字列を足す
 * @param str 足す文字列
 */
void AutopickTextBody::add_str_to_yank(std::string_view str)
{
    this->yank_eol = false;
    this->yank.emplace_back(str);
}

/*!
 * @brief 選択範囲をヤンクバッファへコピーする
 * @details マークが無ければ、カーソル行の全体をコピーする。
 */
void AutopickTextBody::copy_text_to_yank()
{
    const int len = this->lines[this->cy].line.length();
    if (this->cx > len) {
        this->cx = len;
    }

    if (this->marks.none()) {
        this->cx = 0;
        this->my = this->cy;
        this->mx = len;
    }

    this->kill_yank_chain();
    if (this->my != this->cy) {
        int by1 = std::min(this->my, this->cy);
        int by2 = std::max(this->my, this->cy);

        for (int y = by1; y <= by2; y++) {
            this->add_str_to_yank(this->lines[y].line);
        }

        this->add_str_to_yank("");
        this->marks.clear();
        this->dirty_flags |= DIRTY_ALL;
        return;
    }

    const auto bx1 = std::min(this->mx, this->cx);
    auto bx2 = std::max(this->mx, this->cx);
    if (bx2 > len) {
        bx2 = len;
    }

    if ((bx1 == 0) && (bx2 == len)) {
        this->add_str_to_yank(this->lines[this->cy].line);
        this->add_str_to_yank("");
    } else {
        const std::string_view buf(this->lines[this->cy].line);
        this->add_str_to_yank(buf.substr(bx1, bx2 - bx1));
    }

    this->marks.clear();
    this->dirty_flags |= DIRTY_ALL;
}
