/*!
 * @brief 自動拾いエディタを表示させる
 * @date 2020/04/26
 * @author Hourier
 */

#include "autopick/autopick-drawer.h"
#include "autopick/autopick-describer.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-text-body.h"
#include "autopick/autopick-util.h"
#include "io/pref-file-expressor.h"
#include "io/read-pref-file.h"
#include "locale/multibyte-char.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "term/term-color-types.h"
#include "view/display-util.h"
#include <algorithm>
#include <fmt/format.h>
#include <string_view>
#include <tuple>

constexpr auto DESCRIPT_HGT = 3;
constexpr std::string_view EXPRESSION_DIRECTIVE_PREFIX = "?:";
constexpr std::string_view AUTOREGISTER_DIRECTIVE = "$AUTOREGISTER";

/*!
 * @brief 条件式の評価結果と、読込エラーによって無効になる後続行の状態を更新する
 */
void update_autopick_expression_states(PlayerType *player_ptr, AutopickTextBody &atb)
{
    const auto edited_line = atb.dirty_line;
    const auto include_edited = atb.has_line(edited_line) &&
                                (atb.lines[edited_line].state.has(AutopickEntryStatus::INCLUDE) || atb.lines[edited_line].line.starts_with("%:"));
    if (((atb.dirty_flags & (DIRTY_EXPRESSION | DIRTY_ALL)) == 0) && !include_edited) {
        return;
    }

    EnumClassFlagGroup<AutopickEntryStatus> state{};
    for (size_t y = 0; y < atb.lines.size(); ++y) {
        std::string_view s(atb.lines[y].line);
        if (state.has_not(AutopickEntryStatus::BYPASS) && s.starts_with("%:") &&
            (check_autopick_file_conditions(player_ptr, s.substr(2)) == PREF_EXPRESSION_ERROR)) {
            state = { AutopickEntryStatus::BYPASS, AutopickEntryStatus::EXPRESSION_ERROR };
        }
        if ((state.has(AutopickEntryStatus::EXPRESSION_ERROR)) || !s.starts_with(EXPRESSION_DIRECTIVE_PREFIX)) {
            atb.lines[y].state = state;
            if (s.starts_with("%:")) {
                atb.lines[y].state.set(AutopickEntryStatus::INCLUDE);
            }
            if (s.starts_with(EXPRESSION_DIRECTIVE_PREFIX)) {
                atb.lines[y].state.set(AutopickEntryStatus::EXPRESSION);
            }
            continue;
        }

        s.remove_prefix(EXPRESSION_DIRECTIVE_PREFIX.length());

        if (s == AUTOREGISTER_DIRECTIVE) {
            state.set(AutopickEntryStatus::AUTOREGISTER);
        }

        const auto result = process_pref_file_expr_checked(player_ptr, s);
        if (!result) {
            state = { AutopickEntryStatus::BYPASS, AutopickEntryStatus::EXPRESSION_ERROR };
        } else if (*result == "0") {
            state.set(AutopickEntryStatus::BYPASS);
        } else {
            state.reset(AutopickEntryStatus::BYPASS);
        }

        EnumClassFlagGroup<AutopickEntryStatus> tmp_state = state;
        tmp_state.set(AutopickEntryStatus::EXPRESSION);
        atb.lines[y].state = tmp_state;
    }

    atb.dirty_flags |= DIRTY_ALL;
}

/*!
 * @brief Draw text
 */
void draw_text_editor(PlayerType *player_ptr, AutopickTextBody &atb)
{
    int by1 = 0, by2 = 0;

    std::tie(atb.wid, atb.hgt) = term_get_size();

    /*
     * Top line (-1), description line (-3), separator (-1)
     *  == -5
     */
    atb.hgt -= 2 + DESCRIPT_HGT;

    // カーソルを多バイト文字の途中に置かないよう、左右どちらかの文字の境界へ動かす
    if (atb.is_cursor_inside_multibyte_char()) {
        const auto &line = atb.lines[atb.cy].line;
        const auto boundary = (atb.cx & 1) ? mb_char_start_pos(line, atb.cx) : mb_next_char_boundary(line, atb.cx);
        atb.cx = static_cast<int>(boundary);
    }
    if (atb.cy < atb.upper || atb.upper + atb.hgt <= atb.cy) {
        atb.upper = atb.cy - (atb.hgt) / 2;
    }
    if (atb.upper < 0) {
        atb.upper = 0;
    }
    const auto cursor_column = atb.get_cursor_column();
    if ((cursor_column < atb.left + 10 && atb.left > 0) || atb.left + atb.wid - 5 <= cursor_column) {
        atb.left = cursor_column - (atb.wid) * 2 / 3;
    }
    if (atb.left < 0) {
        atb.left = 0;
    }

    if (atb.old_wid != atb.wid || atb.old_hgt != atb.hgt) {
        atb.dirty_flags |= DIRTY_SCREEN;
    } else if (atb.old_upper != atb.upper || atb.old_left != atb.left) {
        atb.dirty_flags |= DIRTY_ALL;
    }

    if (atb.dirty_flags & DIRTY_SCREEN) {
        atb.dirty_flags |= (DIRTY_ALL | DIRTY_MODE);
        term_clear();
    }

    if (atb.dirty_flags & DIRTY_MODE) {
        char buf[MAX_LINELEN]{};
        int sepa_length = atb.wid;
        int j = 0;
        for (; j < sepa_length; j++) {
            buf[j] = '-';
        }
        buf[j] = '\0';
        term_putstr(0, atb.hgt + 1, sepa_length, TERM_WHITE, buf);
    }

    update_autopick_expression_states(player_ptr, atb);
    if (atb.marks.any()) {
        atb.dirty_flags |= DIRTY_ALL;

        by1 = std::min(atb.my, atb.cy);
        by2 = std::max(atb.my, atb.cy);
    }

    int i;
    for (i = 0; i < atb.hgt; i++) {
        byte color;
        int y = atb.upper + i;

        if (!(atb.dirty_flags & DIRTY_ALL) && (atb.dirty_line != y)) {
            continue;
        }

        if (!atb.has_line(y)) {
            break;
        }

        // 表示を始める桁より左の文字を除く。全角文字がその桁にまたがるときは、その文字も除いて1桁右から描く
        std::string_view msg(atb.lines[y].line);
        auto skipped_width = 0;
        while (!msg.empty() && (skipped_width < atb.left)) {
            const auto ch = mb_first_char(msg);
            msg.remove_prefix(ch.length());
            skipped_width += static_cast<int>(str_display_width(ch));
        }

        const auto leftcol = (skipped_width > atb.left) ? 1 : 0;

        term_erase(0, i + 1, atb.wid);
        if (atb.lines[y].state.has(AutopickEntryStatus::AUTOREGISTER)) {
            color = TERM_L_RED;
        } else {
            if (atb.lines[y].state.has(AutopickEntryStatus::BYPASS)) {
                color = TERM_SLATE;
            } else {
                color = TERM_WHITE;
            }
        }

        if (atb.marks.none() || (y < by1 || by2 < y)) {
            term_putstr(leftcol, i + 1, atb.wid - 1, color, msg);
        } else if (by1 != by2) {
            term_putstr(leftcol, i + 1, atb.wid - 1, TERM_YELLOW, msg);
        } else {
            /*
             * 選択範囲の前・選択範囲・選択範囲の後に分けて描く。選択範囲は表示を始める位置 x0 より左を除き、
             * 端が全角文字の途中にあれば (行頭に '!' などを足した場合など)、文字を割らないようにその文字を含める。
             */
            const std::string_view line(atb.lines[y].line);
            const auto x0 = line.length() - msg.length();
            const auto bx1 = mb_char_start_pos(line, std::max<size_t>(std::min(atb.mx, atb.cx), x0));
            const auto bx2 = mb_next_char_boundary(line, std::clamp<size_t>(std::max(atb.mx, atb.cx), bx1, line.length()));

            term_gotoxy(leftcol, i + 1);
            term_addstr(-1, color, line.substr(x0, bx1 - x0));
            term_addstr(-1, TERM_YELLOW, line.substr(bx1, bx2 - bx1));
            term_addstr(-1, color, line.substr(bx2));
        }
    }

    for (; i < atb.hgt; i++) {
        term_erase(0, i + 1, atb.wid);
    }

    bool is_dirty_diary = (atb.dirty_flags & (DIRTY_ALL | DIRTY_NOT_FOUND | DIRTY_NO_SEARCH)) != 0;
    bool is_updated = atb.old_cy != atb.cy || is_dirty_diary || atb.dirty_line == atb.cy;
    if (!is_updated) {
        return;
    }

    autopick_type an_entry, *entry = &an_entry;
    std::string str1, str2;
    for (int j = 0; j < DESCRIPT_HGT; j++) {
        term_erase(0, atb.hgt + 2 + j, atb.wid);
    }

    if (atb.dirty_flags & DIRTY_NOT_FOUND) {
        str1 = fmt::format(_("パターンが見つかりません: {}", "Pattern not found: {}"), atb.search_str);
    } else if (atb.dirty_flags & DIRTY_SKIP_INACTIVE) {
        str1 = fmt::format(_("無効状態の行をスキップしました。({}を検索中)", "Some inactive lines are skipped. (Searching {})"), atb.search_str);
    } else if (atb.dirty_flags & DIRTY_INACTIVE) {
        str1 = fmt::format(_("無効状態の行だけが見付かりました。({}を検索中)", "Found only an inactive line. (Searching {})"), atb.search_str);
    } else if (atb.dirty_flags & DIRTY_NO_SEARCH) {
        str1 = _("検索するパターンがありません(^S で検索)。", "No pattern to search. (Press ^S to search.)");
    } else if (atb.lines[atb.cy].state.has(AutopickEntryStatus::EXPRESSION_ERROR)) {
        str1 = _("条件式エラーのため、この行以降は読み込まれません。", "A condition error prevents loading this and subsequent lines.");
    } else if (atb.lines[atb.cy].line[0] == '#') {
        str1 = _("この行はコメントです。", "This line is a comment.");
    } else if (atb.lines[atb.cy].line[0] && atb.lines[atb.cy].line[1] == ':') {
        switch (atb.lines[atb.cy].line[0]) {
        case '?':
            str1 = _("この行は条件分岐式です。", "This line is a Conditional Expression.");
            break;
        case 'A':
            str1 = _("この行はマクロの実行内容を定義します。", "This line defines a Macro action.");
            break;
        case 'P':
            str1 = _("この行はマクロのトリガー・キーを定義します。", "This line defines a Macro trigger key.");
            break;
        case 'C':
            str1 = _("この行はキー配置を定義します。", "This line defines a Keymap.");
            break;
        }

        switch (atb.lines[atb.cy].line[0]) {
        case '?':
            if (atb.lines[atb.cy].state.has(AutopickEntryStatus::BYPASS)) {
                str2 = _("現在の式の値は「偽(=0)」です。", "The expression is 'False'(=0) currently.");
            } else {
                str2 = _("現在の式の値は「真(=1)」です。", "The expression is 'True'(=1) currently.");
            }
            break;

        default:
            if (atb.lines[atb.cy].state.has(AutopickEntryStatus::AUTOREGISTER)) {
                str2 = _("この行は後で削除されます。", "This line will be deleted later.");
            }

            else if (atb.lines[atb.cy].state.has(AutopickEntryStatus::BYPASS)) {
                str2 = _("この行は現在は無効な状態です。", "This line is bypassed currently.");
            }
            break;
        }
    } else if (autopick_new_entry(entry, atb.lines[atb.cy].line, false)) {
        std::stringstream ss;
        ss << describe_autopick(*entry);
        if (atb.lines[atb.cy].state.has(AutopickEntryStatus::AUTOREGISTER)) {
            ss << _("この行は後で削除されます。", "  This line will be deleted later.");
        }

        if (atb.lines[atb.cy].state.has(AutopickEntryStatus::BYPASS)) {
            ss << _("この行は現在は無効な状態です。", "  This line is bypassed currently.");
        }

        display_wrap_around(ss.str(), 81, atb.hgt + 2, 0);
    }

    if (!str1.empty()) {
        prt(str1, atb.hgt + 1 + 1, 0);
    }
    if (!str2.empty()) {
        prt(str2, atb.hgt + 1 + 2, 0);
    }
}
