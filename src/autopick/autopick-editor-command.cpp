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
ape_quittance do_editor_command(PlayerType *player_ptr, AutopickTextBody &atb, EditorCommandId com_id)
{
    switch (com_id) {
    case EditorCommandId::QUIT: {
        if (atb.changed) {
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
        atb.filename_mode = filename_mode;
        atb.set_lines(std::move(lines));

        atb.dirty_flags |= DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
        atb.cx = atb.cy = 0;
        atb.marks.clear();
        atb.changed = false;
        break;
    }
    case EditorCommandId::HELP: {
        FileDisplayer(player_ptr->name).display(true, _("jeditor.txt", "editor.txt"), 0, 0);
        atb.dirty_flags |= DIRTY_SCREEN;
        break;
    }
    case EditorCommandId::RETURN: {
        if (atb.marks.any()) {
            atb.marks.clear();
            atb.dirty_flags |= DIRTY_ALL;
        }

        atb.insert_return_code();
        atb.cy++;
        atb.cx = 0;
        atb.dirty_flags |= DIRTY_ALL;
        break;
    }
    case EditorCommandId::LEFT: {
        if (0 < atb.cx) {
            atb.cx--;
            const int len = atb.lines[atb.cy].line.length();
            if (len < atb.cx) {
                atb.cx = len;
            }

            atb.cx = static_cast<int>(mb_char_start_pos(atb.lines[atb.cy].line, atb.cx));
        } else if (atb.cy > 0) {
            atb.cy--;
            atb.cx = atb.lines[atb.cy].line.length();
        }

        break;
    }
    case EditorCommandId::DOWN: {
        if (atb.is_last_line(atb.cy)) {
            if (!atb.add_empty_line()) {
                break;
            }
        }

        atb.cy++;
        break;
    }
    case EditorCommandId::UP:
        if (atb.cy > 0) {
            atb.cy--;
        }
        break;
    case EditorCommandId::RIGHT: {
        const int len = atb.lines[atb.cy].line.length();
        atb.cx += static_cast<int>(mb_char_length_at(atb.lines[atb.cy].line, atb.cx));

        if (len < atb.cx) {
            atb.cx = len;
            if (atb.is_last_line(atb.cy)) {
                if (!atb.add_empty_line()) {
                    break;
                }
            }

            atb.cy++;
            atb.cx = 0;
        }

        break;
    }
    case EditorCommandId::BOL:
        atb.cx = 0;
        break;
    case EditorCommandId::EOL:
        atb.cx = atb.lines[atb.cy].line.length();
        break;
    case EditorCommandId::PGUP:
        while (0 < atb.cy && atb.upper <= atb.cy) {
            atb.cy--;
        }

        while (0 < atb.upper && atb.cy + 1 < atb.upper + atb.hgt) {
            atb.upper--;
        }

        break;
    case EditorCommandId::PGDOWN:
        while (atb.cy < atb.upper + atb.hgt) {
            if (atb.is_last_line(atb.cy)) {
                if (!atb.add_empty_line()) {
                    break;
                }
            }

            atb.cy++;
        }

        atb.upper = atb.cy;
        break;
    case EditorCommandId::TOP:
        atb.cy = 0;
        break;
    case EditorCommandId::BOTTOM:
        atb.add_empty_line();
        atb.cy = static_cast<int>(atb.lines.size()) - 1;
        atb.cx = 0;
        break;
    case EditorCommandId::CUT: {
        atb.copy_text_to_yank();
        if (atb.my == atb.cy) {
            const auto bx1 = std::min(atb.mx, atb.cx);
            auto bx2 = std::max(atb.mx, atb.cx);
            const int len = atb.lines[atb.cy].line.length();
            if (bx2 > len) {
                bx2 = len;
            }

            atb.kill_line_segment(atb.cy, bx1, bx2, true);
            atb.cx = bx1;
        } else {
            const auto by1 = std::min(atb.my, atb.cy);
            const auto by2 = std::max(atb.my, atb.cy);
            for (auto y = by2; y >= by1; y--) {
                const int len = atb.lines[y].line.length();
                atb.kill_line_segment(y, 0, len, true);
            }

            atb.cy = by1;
            atb.cx = 0;
        }

        atb.marks.clear();
        atb.dirty_flags |= DIRTY_ALL;
        atb.changed = true;
        break;
    }
    case EditorCommandId::COPY: {
        atb.copy_text_to_yank();

        /*
         * Move cursor position to the end of the selection
         *
         * Pressing ^C ^V correctly duplicates the selection.
         */
        if (atb.my != atb.cy) {
            atb.cy = std::max(atb.cy, atb.my);
            if (atb.is_last_line(atb.cy)) {
                if (!atb.add_empty_line()) {
                    break;
                }
            }

            atb.cy++;
            break;
        }

        atb.cx = std::max(atb.cx, atb.mx);
        if (atb.lines[atb.cy].line[atb.cx] == '\0') {
            if (atb.is_last_line(atb.cy)) {
                if (!atb.add_empty_line()) {
                    break;
                }
            }

            atb.cy++;
            atb.cx = 0;
        }

        break;
    }
    case EditorCommandId::PASTE: {
        const int len = atb.lines[atb.cy].line.length();
        if (atb.yank.empty()) {
            break;
        }
        if (atb.cx > len) {
            atb.cx = len;
        }

        if (atb.marks.any()) {
            atb.marks.clear();
            atb.dirty_flags |= DIRTY_ALL;
        }

        for (auto i = 0U; i < atb.yank.size(); ++i) {
            const std::string_view line(atb.lines[atb.cy].line);
            // 行の上限を超える分は、多バイト文字を分断しないように切り詰める
            auto buf = str_substr(std::string(line.substr(0, atb.cx)) + atb.yank[i], 0, MAX_LINELEN - 1);

            const auto is_last_yank_line = i + 1 == atb.yank.size();
            if (!is_last_yank_line || atb.yank_eol) {
                atb.insert_return_code();
                atb.lines[atb.cy].line = std::move(buf);
                atb.cx = 0;
                atb.cy++;
                continue;
            }

            const auto rest = line.substr(atb.cx);
            atb.cx = buf.length();
            buf.append(rest);
            atb.lines[atb.cy].line = str_substr(std::move(buf), 0, MAX_LINELEN - 1);
            break;
        }

        atb.dirty_flags |= DIRTY_ALL;
        atb.dirty_flags |= DIRTY_EXPRESSION;
        atb.changed = true;
        break;
    }
    case EditorCommandId::BLOCK: {
        if (atb.marks.any()) {
            atb.marks.clear();
            atb.dirty_flags |= DIRTY_ALL;
            break;
        }

        atb.marks.set(AutopickMark::MARK);
        if (com_id == atb.old_com_id) {
            int tmp = atb.cy;
            atb.cy = atb.my;
            atb.my = tmp;
            tmp = atb.cx;
            atb.cx = atb.mx;
            atb.mx = tmp;
            atb.dirty_flags |= DIRTY_ALL;
            break;
        }

        const int len = atb.lines[atb.cy].line.length();

        atb.my = atb.cy;
        atb.mx = atb.cx;
        if (atb.cx > len) {
            atb.mx = len;
        }
        break;
    }
    case EditorCommandId::KILL_LINE: {
        const int len = atb.lines[atb.cy].line.length();
        if (atb.cx > len) {
            atb.cx = len;
        }

        if (atb.marks.any()) {
            atb.marks.clear();
            atb.dirty_flags |= DIRTY_ALL;
        }

        if (atb.old_com_id != com_id) {
            atb.kill_yank_chain();
        }

        if (atb.cx < len) {
            atb.add_str_to_yank(&(atb.lines[atb.cy].line[atb.cx]));
            atb.kill_line_segment(atb.cy, atb.cx, len, false);
            atb.dirty_line = atb.cy;
            break;
        }

        if (atb.yank_eol) {
            atb.add_str_to_yank("");
        }

        atb.yank_eol = true;
        do_editor_command(player_ptr, atb, EditorCommandId::DELETE_CHAR);
        break;
    }
    case EditorCommandId::DELETE_CHAR: {
        if (atb.marks.any()) {
            atb.marks.clear();
            atb.dirty_flags |= DIRTY_ALL;
        }

        atb.cx += static_cast<int>(mb_char_length_at(atb.lines[atb.cy].line, atb.cx));
        const int len = atb.lines[atb.cy].line.length();
        if (len >= atb.cx) {
            do_editor_command(player_ptr, atb, EditorCommandId::BACKSPACE);
            break;
        }

        if (!atb.is_last_line(atb.cy)) {
            atb.cy++;
            atb.cx = 0;
        } else {
            atb.cx = len;
            break;
        }

        do_editor_command(player_ptr, atb, EditorCommandId::BACKSPACE);
        break;
    }
    case EditorCommandId::BACKSPACE: {
        if (atb.marks.any()) {
            atb.marks.clear();
            atb.dirty_flags |= DIRTY_ALL;
        }

        const int len = atb.lines[atb.cy].line.length();
        if (len < atb.cx) {
            atb.cx = len;
        }

        if (atb.cx == 0) {
            if (atb.cy == 0) {
                break;
            }
            atb.cx = atb.lines[atb.cy - 1].line.length();
            atb.lines[atb.cy - 1].line.append(atb.lines[atb.cy].line);
            atb.lines.erase(atb.lines.begin() + atb.cy);
            atb.cy--;
            atb.dirty_flags |= DIRTY_ALL;
            atb.dirty_flags |= DIRTY_EXPRESSION;
            atb.changed = true;
            break;
        }

        const auto prev_cx = static_cast<int>(mb_prev_char_pos(atb.lines[atb.cy].line, atb.cx));
        atb.lines[atb.cy].line.erase(prev_cx, atb.cx - prev_cx);
        atb.cx = prev_cx;
        atb.dirty_line = atb.cy;
        atb.check_expression_line(atb.cy);
        atb.changed = true;
        break;
    }
    case EditorCommandId::SEARCH_STR: {
        atb.dirty_flags |= DIRTY_SCREEN;
        AutopickSearch as(atb.search_item, atb.search_str);
        const auto as_result = get_string_for_search(player_ptr, as);
        atb.search_item = as_result.item;
        atb.search_str = as_result.search_str;
        if (as_result.result == AutopickSearchResult::CANCEL) {
            break;
        }

        const auto command = as_result.result == AutopickSearchResult::FORWARD ? EditorCommandId::SEARCH_FORW : EditorCommandId::SEARCH_BACK;
        do_editor_command(player_ptr, atb, command);
        break;
    }
    case EditorCommandId::SEARCH_FORW:
        if (atb.search_item) {
            search_for_object(player_ptr, atb, atb.search_item.get(), true);
            break;
        }

        if (!atb.search_str.empty()) {
            atb.search_for_string(atb.search_str, true);
            break;
        }

        atb.dirty_flags |= DIRTY_NO_SEARCH;
        break;

    case EditorCommandId::SEARCH_BACK: {
        if (atb.search_item) {
            search_for_object(player_ptr, atb, atb.search_item.get(), false);
            break;
        }

        if (!atb.search_str.empty()) {
            atb.search_for_string(atb.search_str, false);
            break;
        }

        atb.dirty_flags |= DIRTY_NO_SEARCH;
        break;
    }
    case EditorCommandId::SEARCH_OBJ: {
        atb.dirty_flags |= DIRTY_SCREEN;
        AutopickSearch as(atb.search_item, atb.search_str);
        if (!get_object_for_search(player_ptr, as)) {
            break;
        }

        atb.search_str = as.search_str;
        do_editor_command(player_ptr, atb, EditorCommandId::SEARCH_FORW);
        break;
    }
    case EditorCommandId::SEARCH_DESTROYED: {
        AutopickSearch as(atb.search_item, atb.search_str);
        if (!get_destroyed_object_for_search(player_ptr, as)) {
            atb.dirty_flags |= DIRTY_NO_SEARCH;
            break;
        }

        atb.search_str = as.search_str;
        do_editor_command(player_ptr, atb, EditorCommandId::SEARCH_FORW);
        break;
    }
    case EditorCommandId::INSERT_OBJECT: {
        autopick_type an_entry, *entry = &an_entry;
        if (!entry_from_choosed_object(player_ptr, entry)) {
            atb.dirty_flags |= DIRTY_SCREEN;
            break;
        }

        atb.cx = 0;
        atb.insert_return_code();
        atb.lines[atb.cy].line = autopick_line_from_entry(*entry);
        atb.dirty_flags |= DIRTY_SCREEN;
        break;
    }
    case EditorCommandId::INSERT_DESTROYED: {
        if (atb.last_destroyed.empty()) {
            break;
        }

        atb.cx = 0;
        atb.insert_return_code();
        atb.lines[atb.cy].line = atb.last_destroyed;
        atb.dirty_flags |= DIRTY_ALL;
        atb.changed = true;
        break;
    }
    case EditorCommandId::INSERT_BLOCK: {
        const auto expression = fmt::format("?:[AND [EQU $RACE {}] [EQU $CLASS {}] [GEQ $LEVEL {:02}]]", rp_ptr->title.en_string(), cp_ptr->title.en_string(), player_ptr->lev);
        atb.cx = 0;
        atb.insert_return_code();
        atb.lines[atb.cy].line = expression;
        atb.cy++;
        atb.insert_return_code();
        atb.lines[atb.cy].line = "?:1";
        atb.dirty_flags |= DIRTY_ALL;
        atb.changed = true;
        break;
    }
    case EditorCommandId::INSERT_MACRO: {
        draw_text_editor(player_ptr, atb);
        term_erase(0, atb.cy - atb.upper + 1, atb.wid);
        constexpr auto prompt = _("P:<トリガーキー>: ", "P:<Trigger key>: ");
        term_putstr(0, atb.cy - atb.upper + 1, atb.wid - 1, TERM_YELLOW, prompt);
        const InputContextScope context_scope(InputKind::COMMAND_CHAR, prompt);
        if (!atb.insert_macro_line()) {
            break;
        }

        atb.cx = 2;
        atb.dirty_flags |= DIRTY_ALL;
        atb.changed = true;
        break;
    }
    case EditorCommandId::INSERT_KEYMAP: {
        draw_text_editor(player_ptr, atb);
        term_erase(0, atb.cy - atb.upper + 1, atb.wid);
        const auto mode = rogue_like_commands ? KeymapMode::ROGUE : KeymapMode::ORIGINAL;
        const auto mes = fmt::format("C:{}:<{}>: ", enum2i(mode), _("コマンドキー", "Keypress"));
        term_putstr(0, atb.cy - atb.upper + 1, atb.wid - 1, TERM_YELLOW, mes);
        const InputContextScope context_scope(InputKind::COMMAND_CHAR, mes);
        if (!atb.insert_keymap_line()) {
            break;
        }

        atb.cx = 2;
        atb.dirty_flags |= DIRTY_ALL;
        atb.changed = true;
        break;
    }
    case EditorCommandId::CL_AUTOPICK:
        atb.toggle_command_letter(AutopickMethod::AUTOPICK);
        break;
    case EditorCommandId::CL_DESTROY:
        atb.toggle_command_letter(AutopickMethod::AUTODESTROY);
        break;
    case EditorCommandId::CL_LEAVE:
        atb.toggle_command_letter(AutopickMethod::NOT_AUTOPICK);
        break;
    case EditorCommandId::CL_QUERY:
        atb.toggle_command_letter(AutopickMethod::QUERY_AUTOPICK);
        break;
    case EditorCommandId::CL_NO_DISP:
        atb.toggle_command_letter(AutopickMethod::DISPLAY);
        break;
    case EditorCommandId::IK_UNAWARE:
        atb.toggle_keyword(FLG_UNAWARE);
        break;
    case EditorCommandId::IK_UNIDENTIFIED:
        atb.toggle_keyword(FLG_UNIDENTIFIED);
        break;
    case EditorCommandId::IK_IDENTIFIED:
        atb.toggle_keyword(FLG_IDENTIFIED);
        break;
    case EditorCommandId::IK_STAR_IDENTIFIED:
        atb.toggle_keyword(FLG_STAR_IDENTIFIED);
        break;
    case EditorCommandId::KK_WEAPONS:
        atb.toggle_keyword(FLG_WEAPONS);
        break;
    case EditorCommandId::KK_FAVORITE_WEAPONS:
        atb.toggle_keyword(FLG_FAVORITE_WEAPONS);
        break;
    case EditorCommandId::KK_ARMORS:
        atb.toggle_keyword(FLG_ARMORS);
        break;
    case EditorCommandId::KK_MISSILES:
        atb.toggle_keyword(FLG_MISSILES);
        break;
    case EditorCommandId::KK_DEVICES:
        atb.toggle_keyword(FLG_DEVICES);
        break;
    case EditorCommandId::KK_LIGHTS:
        atb.toggle_keyword(FLG_LIGHTS);
        break;
    case EditorCommandId::KK_JUNKS:
        atb.toggle_keyword(FLG_JUNKS);
        break;
    case EditorCommandId::KK_CORPSES:
        atb.toggle_keyword(FLG_CORPSES);
        break;
    case EditorCommandId::KK_SPELLBOOKS:
        atb.toggle_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::KK_SHIELDS:
        atb.toggle_keyword(FLG_SHIELDS);
        break;
    case EditorCommandId::KK_BOWS:
        atb.toggle_keyword(FLG_BOWS);
        break;
    case EditorCommandId::KK_RINGS:
        atb.toggle_keyword(FLG_RINGS);
        break;
    case EditorCommandId::KK_AMULETS:
        atb.toggle_keyword(FLG_AMULETS);
        break;
    case EditorCommandId::KK_SUITS:
        atb.toggle_keyword(FLG_SUITS);
        break;
    case EditorCommandId::KK_CLOAKS:
        atb.toggle_keyword(FLG_CLOAKS);
        break;
    case EditorCommandId::KK_HELMS:
        atb.toggle_keyword(FLG_HELMS);
        break;
    case EditorCommandId::KK_GLOVES:
        atb.toggle_keyword(FLG_GLOVES);
        break;
    case EditorCommandId::KK_BOOTS:
        atb.toggle_keyword(FLG_BOOTS);
        break;
    case EditorCommandId::OK_COLLECTING:
        atb.toggle_keyword(FLG_COLLECTING);
        break;
    case EditorCommandId::OK_BOOSTED:
        atb.toggle_keyword(FLG_BOOSTED);
        break;
    case EditorCommandId::OK_MORE_DICE:
        atb.toggle_keyword(FLG_MORE_DICE);
        break;
    case EditorCommandId::OK_MORE_BONUS:
        atb.toggle_keyword(FLG_MORE_BONUS);
        break;
    case EditorCommandId::OK_WORTHLESS:
        atb.toggle_keyword(FLG_WORTHLESS);
        break;
    case EditorCommandId::OK_ARTIFACT:
        atb.toggle_keyword(FLG_ARTIFACT);
        break;
    case EditorCommandId::OK_EGO:
        atb.toggle_keyword(FLG_EGO);
        break;
    case EditorCommandId::OK_GOOD:
        atb.toggle_keyword(FLG_GOOD);
        break;
    case EditorCommandId::OK_NAMELESS:
        atb.toggle_keyword(FLG_NAMELESS);
        break;
    case EditorCommandId::OK_AVERAGE:
        atb.toggle_keyword(FLG_AVERAGE);
        break;
    case EditorCommandId::OK_RARE:
        atb.toggle_keyword(FLG_RARE);
        break;
    case EditorCommandId::OK_COMMON:
        atb.toggle_keyword(FLG_COMMON);
        break;
    case EditorCommandId::OK_WANTED:
        atb.toggle_keyword(FLG_WANTED);
        break;
    case EditorCommandId::OK_UNIQUE:
        atb.toggle_keyword(FLG_UNIQUE);
        break;
    case EditorCommandId::OK_HUMAN:
        atb.toggle_keyword(FLG_HUMAN);
        break;
    case EditorCommandId::OK_UNREADABLE:
        atb.toggle_keyword(FLG_UNREADABLE);
        atb.add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_REALM1:
        atb.toggle_keyword(FLG_REALM1);
        atb.add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_REALM2:
        atb.toggle_keyword(FLG_REALM2);
        atb.add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_FIRST:
        atb.toggle_keyword(FLG_FIRST);
        atb.add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_SECOND:
        atb.toggle_keyword(FLG_SECOND);
        atb.add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_THIRD:
        atb.toggle_keyword(FLG_THIRD);
        atb.add_keyword(FLG_SPELLBOOKS);
        break;
    case EditorCommandId::OK_FOURTH:
        atb.toggle_keyword(FLG_FOURTH);
        atb.add_keyword(FLG_SPELLBOOKS);
        break;
    default:
        break;
    }

    atb.old_com_id = com_id;
    return APE_QUIT;
}
