/*!
 * @brief 自動拾いエディタ画面でキーを押した時の挙動一式
 * @date 2020/04/26
 * @author Hourier
 * @todo これ単体で700行を超えているので要分割
 */

#include "autopick/autopick-editor-command.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-drawer.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-finder.h"
#include "autopick/autopick-flags-table.h"
#include "autopick/autopick-methods-table.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-text-body.h"
#include "autopick/autopick-util.h"
#include "core/asking-player.h"
#include "core/show-file.h"
#include "game-option/input-options.h"
#include "game-option/keymap-directory-getter.h"
#include "io/input-context.h"
#include "io/macro-configurations-store.h"
#include "locale/language-switcher.h"
#include "locale/multibyte-char.h"
#include "player-info/class-info.h"
#include "player-info/race-info.h"
#include "system/player-type-definition.h"
#include "term/term-color-types.h"
#include "util/string-processor.h"
#include <fmt/format.h>
#include <sstream>
#include <string>
#include <string_view>

/*!
 * @brief
 * @param player_ptr プレイヤーへの参照ポインタ
 * @param tb 自動拾いの構文
 * @param com_id エディタ内で打ったコマンド
 * @return
 * @details Execute a single editor command
 */
ape_quittance do_editor_command(PlayerType *player_ptr, AutopickTextBody *tb, EditorCommandId com_id)
{
    switch (com_id) {
    case EditorCommandId::QUIT: {
        if (tb->changed) {
            if (!input_check(_("全ての変更を破棄してから終了します。よろしいですか？ ", "Discard all changes and quit. Are you sure? "))) {
                break;
            }
        }

        return APE_QUIT_WITHOUT_SAVE;
    }
    case EditorCommandId::SAVEQUIT:
        return APE_QUIT_AND_SAVE;
    case EditorCommandId::REVERT: {
        if (!input_check(_("全ての変更を破棄して元の状態に戻します。よろしいですか？ ", "Discard all changes and revert to original file. Are you sure? "))) {
            break;
        }

        auto [filename_mode, lines] = read_pickpref_text_lines(player_ptr->base_name);
        tb->filename_mode = filename_mode;
        tb->set_lines(std::move(lines));

        tb->dirty_flags |= DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
        tb->cx = tb->cy = 0;
        tb->marks.clear();
        tb->changed = false;
        break;
    }
    case EditorCommandId::HELP: {
        FileDisplayer(player_ptr->name).display(true, _("jeditor.txt", "editor.txt"), 0, 0);
        tb->dirty_flags |= DIRTY_SCREEN;
        break;
    }
    case EditorCommandId::RETURN: {
        if (tb->marks.any()) {
            tb->marks.clear();
            tb->dirty_flags |= DIRTY_ALL;
        }

        tb->insert_return_code();
        tb->cy++;
        tb->cx = 0;
        tb->dirty_flags |= DIRTY_ALL;
        break;
    }
    case EditorCommandId::LEFT: {
        if (0 < tb->cx) {
            tb->cx--;
            const int len = tb->lines[tb->cy].line.length();
            if (len < tb->cx) {
                tb->cx = len;
            }

            tb->cx = static_cast<int>(mb_char_start_pos(tb->lines[tb->cy].line, tb->cx));
        } else if (tb->cy > 0) {
            tb->cy--;
            tb->cx = tb->lines[tb->cy].line.length();
        }

        break;
    }
    case EditorCommandId::DOWN: {
        if (tb->is_last_line(tb->cy)) {
            if (!tb->add_empty_line()) {
                break;
            }
        }

        tb->cy++;
        break;
    }
    case EditorCommandId::UP:
        if (tb->cy > 0) {
            tb->cy--;
        }
        break;
    case EditorCommandId::RIGHT: {
        const int len = tb->lines[tb->cy].line.length();
        tb->cx += static_cast<int>(mb_char_length_at(tb->lines[tb->cy].line, tb->cx));

        if (len < tb->cx) {
            tb->cx = len;
            if (tb->is_last_line(tb->cy)) {
                if (!tb->add_empty_line()) {
                    break;
                }
            }

            tb->cy++;
            tb->cx = 0;
        }

        break;
    }
    case EditorCommandId::BOL:
        tb->cx = 0;
        break;
    case EditorCommandId::EOL:
        tb->cx = tb->lines[tb->cy].line.length();
        break;
    case EditorCommandId::PGUP:
        while (0 < tb->cy && tb->upper <= tb->cy) {
            tb->cy--;
        }

        while (0 < tb->upper && tb->cy + 1 < tb->upper + tb->hgt) {
            tb->upper--;
        }

        break;
    case EditorCommandId::PGDOWN:
        while (tb->cy < tb->upper + tb->hgt) {
            if (tb->is_last_line(tb->cy)) {
                if (!tb->add_empty_line()) {
                    break;
                }
            }

            tb->cy++;
        }

        tb->upper = tb->cy;
        break;
    case EditorCommandId::TOP:
        tb->cy = 0;
        break;
    case EditorCommandId::BOTTOM:
        tb->add_empty_line();
        tb->cy = static_cast<int>(tb->lines.size()) - 1;
        tb->cx = 0;
        break;
    case EditorCommandId::CUT: {
        tb->copy_text_to_yank();
        if (tb->my == tb->cy) {
            const auto bx1 = std::min(tb->mx, tb->cx);
            auto bx2 = std::max(tb->mx, tb->cx);
            const int len = tb->lines[tb->cy].line.length();
            if (bx2 > len) {
                bx2 = len;
            }

            tb->kill_line_segment(tb->cy, bx1, bx2, true);
            tb->cx = bx1;
        } else {
            const auto by1 = std::min(tb->my, tb->cy);
            const auto by2 = std::max(tb->my, tb->cy);
            for (auto y = by2; y >= by1; y--) {
                const int len = tb->lines[y].line.length();
                tb->kill_line_segment(y, 0, len, true);
            }

            tb->cy = by1;
            tb->cx = 0;
        }

        tb->marks.clear();
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EditorCommandId::COPY: {
        tb->copy_text_to_yank();

        /*
         * Move cursor position to the end of the selection
         *
         * Pressing ^C ^V correctly duplicates the selection.
         */
        if (tb->my != tb->cy) {
            tb->cy = std::max(tb->cy, tb->my);
            if (tb->is_last_line(tb->cy)) {
                if (!tb->add_empty_line()) {
                    break;
                }
            }

            tb->cy++;
            break;
        }

        tb->cx = std::max(tb->cx, tb->mx);
        if (tb->lines[tb->cy].line[tb->cx] == '\0') {
            if (tb->is_last_line(tb->cy)) {
                if (!tb->add_empty_line()) {
                    break;
                }
            }

            tb->cy++;
            tb->cx = 0;
        }

        break;
    }
    case EditorCommandId::PASTE: {
        const int len = tb->lines[tb->cy].line.length();
        if (tb->yank.empty()) {
            break;
        }
        if (tb->cx > len) {
            tb->cx = len;
        }

        if (tb->marks.any()) {
            tb->marks.clear();
            tb->dirty_flags |= DIRTY_ALL;
        }

        for (auto i = 0U; i < tb->yank.size(); ++i) {
            const std::string_view line(tb->lines[tb->cy].line);
            // 行の上限を超える分は、多バイト文字を分断しないように切り詰める
            auto buf = str_substr(std::string(line.substr(0, tb->cx)) + tb->yank[i], 0, MAX_LINELEN - 1);

            const auto is_last_yank_line = i + 1 == tb->yank.size();
            if (!is_last_yank_line || tb->yank_eol) {
                tb->insert_return_code();
                tb->lines[tb->cy].line = std::move(buf);
                tb->cx = 0;
                tb->cy++;
                continue;
            }

            const auto rest = line.substr(tb->cx);
            tb->cx = buf.length();
            buf.append(rest);
            tb->lines[tb->cy].line = str_substr(std::move(buf), 0, MAX_LINELEN - 1);
            break;
        }

        tb->dirty_flags |= DIRTY_ALL;
        tb->dirty_flags |= DIRTY_EXPRESSION;
        tb->changed = true;
        break;
    }
    case EditorCommandId::BLOCK: {
        if (tb->marks.any()) {
            tb->marks.clear();
            tb->dirty_flags |= DIRTY_ALL;
            break;
        }

        tb->marks.set(AutopickMark::MARK);
        if (com_id == tb->old_com_id) {
            int tmp = tb->cy;
            tb->cy = tb->my;
            tb->my = tmp;
            tmp = tb->cx;
            tb->cx = tb->mx;
            tb->mx = tmp;
            tb->dirty_flags |= DIRTY_ALL;
            break;
        }

        const int len = tb->lines[tb->cy].line.length();

        tb->my = tb->cy;
        tb->mx = tb->cx;
        if (tb->cx > len) {
            tb->mx = len;
        }
        break;
    }
    case EditorCommandId::KILL_LINE: {
        const int len = tb->lines[tb->cy].line.length();
        if (tb->cx > len) {
            tb->cx = len;
        }

        if (tb->marks.any()) {
            tb->marks.clear();
            tb->dirty_flags |= DIRTY_ALL;
        }

        if (tb->old_com_id != com_id) {
            tb->kill_yank_chain();
        }

        if (tb->cx < len) {
            tb->add_str_to_yank(&(tb->lines[tb->cy].line[tb->cx]));
            tb->kill_line_segment(tb->cy, tb->cx, len, false);
            tb->dirty_line = tb->cy;
            break;
        }

        if (tb->yank_eol) {
            tb->add_str_to_yank("");
        }

        tb->yank_eol = true;
        do_editor_command(player_ptr, tb, EditorCommandId::DELETE_CHAR);
        break;
    }
    case EditorCommandId::DELETE_CHAR: {
        if (tb->marks.any()) {
            tb->marks.clear();
            tb->dirty_flags |= DIRTY_ALL;
        }

        tb->cx += static_cast<int>(mb_char_length_at(tb->lines[tb->cy].line, tb->cx));
        const int len = tb->lines[tb->cy].line.length();
        if (len >= tb->cx) {
            do_editor_command(player_ptr, tb, EditorCommandId::BACKSPACE);
            break;
        }

        if (!tb->is_last_line(tb->cy)) {
            tb->cy++;
            tb->cx = 0;
        } else {
            tb->cx = len;
            break;
        }

        do_editor_command(player_ptr, tb, EditorCommandId::BACKSPACE);
        break;
    }
    case EditorCommandId::BACKSPACE: {
        if (tb->marks.any()) {
            tb->marks.clear();
            tb->dirty_flags |= DIRTY_ALL;
        }

        const int len = tb->lines[tb->cy].line.length();
        if (len < tb->cx) {
            tb->cx = len;
        }

        if (tb->cx == 0) {
            if (tb->cy == 0) {
                break;
            }
            tb->cx = tb->lines[tb->cy - 1].line.length();
            tb->lines[tb->cy - 1].line.append(tb->lines[tb->cy].line);
            tb->lines.erase(tb->lines.begin() + tb->cy);
            tb->cy--;
            tb->dirty_flags |= DIRTY_ALL;
            tb->dirty_flags |= DIRTY_EXPRESSION;
            tb->changed = true;
            break;
        }

        const auto prev_cx = static_cast<int>(mb_prev_char_pos(tb->lines[tb->cy].line, tb->cx));
        tb->lines[tb->cy].line.erase(prev_cx, tb->cx - prev_cx);
        tb->cx = prev_cx;
        tb->dirty_line = tb->cy;
        tb->check_expression_line(tb->cy);
        tb->changed = true;
        break;
    }
    case EditorCommandId::SEARCH_STR: {
        tb->dirty_flags |= DIRTY_SCREEN;
        AutopickSearch as(tb->search_item, tb->search_str);
        const auto as_result = get_string_for_search(player_ptr, as);
        tb->search_item = as_result.item;
        tb->search_str = as_result.search_str;
        if (as_result.result == AutopickSearchResult::CANCEL) {
            break;
        }

        const auto command = as_result.result == AutopickSearchResult::FORWARD ? EditorCommandId::SEARCH_FORW : EditorCommandId::SEARCH_BACK;
        do_editor_command(player_ptr, tb, command);
        break;
    }
    case EditorCommandId::SEARCH_FORW:
        if (tb->search_item) {
            search_for_object(player_ptr, tb, tb->search_item.get(), true);
            break;
        }

        if (!tb->search_str.empty()) {
            tb->search_for_string(tb->search_str, true);
            break;
        }

        tb->dirty_flags |= DIRTY_NO_SEARCH;
        break;

    case EditorCommandId::SEARCH_BACK: {
        if (tb->search_item) {
            search_for_object(player_ptr, tb, tb->search_item.get(), false);
            break;
        }

        if (!tb->search_str.empty()) {
            tb->search_for_string(tb->search_str, false);
            break;
        }

        tb->dirty_flags |= DIRTY_NO_SEARCH;
        break;
    }
    case EditorCommandId::SEARCH_OBJ: {
        tb->dirty_flags |= DIRTY_SCREEN;
        AutopickSearch as(tb->search_item, tb->search_str);
        if (!get_object_for_search(player_ptr, as)) {
            break;
        }

        tb->search_str = as.search_str;
        do_editor_command(player_ptr, tb, EditorCommandId::SEARCH_FORW);
        break;
    }
    case EditorCommandId::SEARCH_DESTROYED: {
        AutopickSearch as(tb->search_item, tb->search_str);
        if (!get_destroyed_object_for_search(player_ptr, as)) {
            tb->dirty_flags |= DIRTY_NO_SEARCH;
            break;
        }

        tb->search_str = as.search_str;
        do_editor_command(player_ptr, tb, EditorCommandId::SEARCH_FORW);
        break;
    }
    case EditorCommandId::INSERT_OBJECT: {
        autopick_type an_entry, *entry = &an_entry;
        if (!entry_from_choosed_object(player_ptr, entry)) {
            tb->dirty_flags |= DIRTY_SCREEN;
            break;
        }

        tb->cx = 0;
        tb->insert_return_code();
        tb->lines[tb->cy].line = autopick_line_from_entry(*entry);
        tb->dirty_flags |= DIRTY_SCREEN;
        break;
    }
    case EditorCommandId::INSERT_DESTROYED: {
        if (tb->last_destroyed.empty()) {
            break;
        }

        tb->cx = 0;
        tb->insert_return_code();
        tb->lines[tb->cy].line = tb->last_destroyed;
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EditorCommandId::INSERT_BLOCK: {
        const auto expression = fmt::format("?:[AND [EQU $RACE {}] [EQU $CLASS {}] [GEQ $LEVEL {:02}]]", rp_ptr->title.en_string(), cp_ptr->title.en_string(), player_ptr->lev);
        tb->cx = 0;
        tb->insert_return_code();
        tb->lines[tb->cy].line = expression;
        tb->cy++;
        tb->insert_return_code();
        tb->lines[tb->cy].line = "?:1";
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EditorCommandId::INSERT_MACRO: {
        draw_text_editor(player_ptr, tb);
        term_erase(0, tb->cy - tb->upper + 1, tb->wid);
        constexpr auto prompt = _("P:<トリガーキー>: ", "P:<Trigger key>: ");
        term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW, prompt);
        const InputContextScope context_scope(InputKind::COMMAND_CHAR, prompt);
        if (!tb->insert_macro_line()) {
            break;
        }

        tb->cx = 2;
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EditorCommandId::INSERT_KEYMAP: {
        draw_text_editor(player_ptr, tb);
        term_erase(0, tb->cy - tb->upper + 1, tb->wid);
        const auto mode = rogue_like_commands ? KeymapMode::ROGUE : KeymapMode::ORIGINAL;
        const auto mes = fmt::format("C:{}:<{}>: ", enum2i(mode), _("コマンドキー", "Keypress"));
        term_putstr(0, tb->cy - tb->upper + 1, tb->wid - 1, TERM_YELLOW, mes);
        const InputContextScope context_scope(InputKind::COMMAND_CHAR, mes);
        if (!tb->insert_keymap_line()) {
            break;
        }

        tb->cx = 2;
        tb->dirty_flags |= DIRTY_ALL;
        tb->changed = true;
        break;
    }
    case EditorCommandId::CL_AUTOPICK:
        tb->toggle_command_letter(AutopickMethod::AUTOPICK);
        break;
    case EditorCommandId::CL_DESTROY:
        tb->toggle_command_letter(AutopickMethod::AUTODESTROY);
        break;
    case EditorCommandId::CL_LEAVE:
        tb->toggle_command_letter(AutopickMethod::NOT_AUTOPICK);
        break;
    case EditorCommandId::CL_QUERY:
        tb->toggle_command_letter(AutopickMethod::QUERY_AUTOPICK);
        break;
    case EditorCommandId::CL_NO_DISP:
        tb->toggle_command_letter(AutopickMethod::DISPLAY);
        break;
    case EditorCommandId::IK_UNAWARE:
        tb->toggle_keyword(FLG_UNAWARE);
        break;
    case EditorCommandId::IK_UNIDENTIFIED:
        tb->toggle_keyword(FLG_UNIDENTIFIED);
        break;
    case EditorCommandId::IK_IDENTIFIED:
        tb->toggle_keyword(FLG_IDENTIFIED);
        break;
    case EditorCommandId::IK_STAR_IDENTIFIED:
        tb->toggle_keyword(FLG_STAR_IDENTIFIED);
        break;
    case EditorCommandId::KK_WEAPONS:
        tb->toggle_keyword(FLG_WEAPONS);
        break;
    case EditorCommandId::KK_FAVORITE_WEAPONS:
        tb->toggle_keyword(FLG_FAVORITE_WEAPONS);
        break;
    case EditorCommandId::KK_ARMORS:
        tb->toggle_keyword(FLG_ARMORS);
        break;
    case EditorCommandId::KK_MISSILES:
        tb->toggle_keyword(FLG_MISSILES);
        break;
    case EditorCommandId::KK_DEVICES:
        tb->toggle_keyword(FLG_DEVICES);
        break;
    case EditorCommandId::KK_LIGHTS:
        tb->toggle_keyword(FLG_LIGHTS);
        break;
    case EditorCommandId::KK_JUNKS:
        tb->toggle_keyword(FLG_JUNKS);
        break;
    case EditorCommandId::KK_CORPSES:
        tb->toggle_keyword(FLG_CORPSES);
        break;
    case EditorCommandId::KK_SPELLBOOKS:
        tb->toggle_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::KK_SHIELDS:
        tb->toggle_keyword(FLG_SHIELDS);
        break;
    case EditorCommandId::KK_BOWS:
        tb->toggle_keyword(FLG_BOWS);
        break;
    case EditorCommandId::KK_RINGS:
        tb->toggle_keyword(FLG_RINGS);
        break;
    case EditorCommandId::KK_AMULETS:
        tb->toggle_keyword(FLG_AMULETS);
        break;
    case EditorCommandId::KK_SUITS:
        tb->toggle_keyword(FLG_SUITS);
        break;
    case EditorCommandId::KK_CLOAKS:
        tb->toggle_keyword(FLG_CLOAKS);
        break;
    case EditorCommandId::KK_HELMS:
        tb->toggle_keyword(FLG_HELMS);
        break;
    case EditorCommandId::KK_GLOVES:
        tb->toggle_keyword(FLG_GLOVES);
        break;
    case EditorCommandId::KK_BOOTS:
        tb->toggle_keyword(FLG_BOOTS);
        break;
    case EditorCommandId::OK_COLLECTING:
        tb->toggle_keyword(FLG_COLLECTING);
        break;
    case EditorCommandId::OK_BOOSTED:
        tb->toggle_keyword(FLG_BOOSTED);
        break;
    case EditorCommandId::OK_MORE_DICE:
        tb->toggle_keyword(FLG_MORE_DICE);
        break;
    case EditorCommandId::OK_MORE_BONUS:
        tb->toggle_keyword(FLG_MORE_BONUS);
        break;
    case EditorCommandId::OK_WORTHLESS:
        tb->toggle_keyword(FLG_WORTHLESS);
        break;
    case EditorCommandId::OK_ARTIFACT:
        tb->toggle_keyword(FLG_ARTIFACT);
        break;
    case EditorCommandId::OK_EGO:
        tb->toggle_keyword(FLG_EGO);
        break;
    case EditorCommandId::OK_GOOD:
        tb->toggle_keyword(FLG_GOOD);
        break;
    case EditorCommandId::OK_NAMELESS:
        tb->toggle_keyword(FLG_NAMELESS);
        break;
    case EditorCommandId::OK_AVERAGE:
        tb->toggle_keyword(FLG_AVERAGE);
        break;
    case EditorCommandId::OK_RARE:
        tb->toggle_keyword(FLG_RARE);
        break;
    case EditorCommandId::OK_COMMON:
        tb->toggle_keyword(FLG_COMMON);
        break;
    case EditorCommandId::OK_WANTED:
        tb->toggle_keyword(FLG_WANTED);
        break;
    case EditorCommandId::OK_UNIQUE:
        tb->toggle_keyword(FLG_UNIQUE);
        break;
    case EditorCommandId::OK_HUMAN:
        tb->toggle_keyword(FLG_HUMAN);
        break;
    case EditorCommandId::OK_UNREADABLE:
        tb->toggle_keyword(FLG_UNREADABLE);
        tb->add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_REALM1:
        tb->toggle_keyword(FLG_REALM1);
        tb->add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_REALM2:
        tb->toggle_keyword(FLG_REALM2);
        tb->add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_FIRST:
        tb->toggle_keyword(FLG_FIRST);
        tb->add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_SECOND:
        tb->toggle_keyword(FLG_SECOND);
        tb->add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_THIRD:
        tb->toggle_keyword(FLG_THIRD);
        tb->add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_FOURTH:
        tb->toggle_keyword(FLG_FOURTH);
        tb->add_keyword(FLG_SPELLBOOKS);
        break;
    default:
        break;
    }

    tb->old_com_id = com_id;
    return APE_QUIT;
}
