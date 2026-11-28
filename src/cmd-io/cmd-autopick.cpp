#include "cmd-io/cmd-autopick.h"
#include "autopick/autopick-command-menu.h"
#include "autopick/autopick-commands-table.h"
#include "autopick/autopick-dirty-flags.h"
#include "autopick/autopick-drawer.h"
#include "autopick/autopick-editor-command.h"
#include "autopick/autopick-entry.h"
#include "autopick/autopick-initializer.h"
#include "autopick/autopick-menu-data-table.h"
#include "autopick/autopick-pref-processor.h"
#include "autopick/autopick-reader-writer.h"
#include "autopick/autopick-text-body.h"
#include "autopick/autopick-util.h"
#include "cmd-io/cmd-save.h"
#include "core/asking-player.h"
#include "io/input-context.h"
#include "io/input-key-acceptor.h"
#include "io/read-pref-file.h"
#include "system/item/item-entity.h"
#include "system/player-type-definition.h"
#include "term/screen-processor.h"
#include "util/finalizer.h"
#include "util/int-char-converter.h"
#include "world/world.h"
#include <fmt/format.h>

/*
 * In-game editor of Object Auto-picker/Destoryer
 * @param player_ptr プレイヤーへの参照ポインタ
 */
void do_cmd_edit_autopick(PlayerType *player_ptr)
{
    static int cx_save = 0;
    static int cy_save = 0;
    autopick_type an_entry, *entry = &an_entry;
    int key = -1;
    static int32_t old_autosave_turn = 0L;
    ape_quittance quit = APE_QUIT;

    AutopickTextBody atb(cx_save, cy_save);
    atb.dirty_flags = DIRTY_ALL | DIRTY_MODE | DIRTY_EXPRESSION;
    atb.filename_mode = PickType::DEFAULT;
    auto &world = AngbandWorld::get_instance();
    world.play_time.pause();
    const auto unpauser = util::make_finalizer([&world]() { world.play_time.unpause(); });
    if (world.game_turn < old_autosave_turn) {
        while (old_autosave_turn > world.game_turn) {
            old_autosave_turn -= TURNS_PER_DAY;
        }
    }

    if (world.game_turn > old_autosave_turn + 100L) {
        do_cmd_save_game(player_ptr, true);
        old_autosave_turn = world.game_turn;
    }

    init_autopick();
    if (autopick_last_destroyed_object.is_valid()) {
        autopick_entry_from_object(player_ptr, entry, &autopick_last_destroyed_object);
        atb.last_destroyed = autopick_line_from_entry(*entry);
    }

    auto [filename_mode, lines] = read_pickpref_text_lines(player_ptr->base_name);
    atb.filename_mode = filename_mode;
    atb.set_lines(std::move(lines));

    // 前回閉じたときのカーソルの行が、読み込み直したファイルに無ければ先頭に戻す
    if (!atb.has_line(atb.cy)) {
        atb.cy = atb.cx = 0;
    }

    TermCenteredOffsetSetter tcos(tl::nullopt, tl::nullopt);

    screen_save();
    while (quit == APE_QUIT) {
        auto com_id = EditorCommandId::NONE;
        atb.adjust_cursor_column();
        draw_text_editor(player_ptr, atb);
        constexpr auto key_guide = _("(^Q:終了 ^W:セーブして終了, ESC:メニュー, その他:入力)", "(^Q:Quit, ^W:Save&Quit, ESC:Menu, Other:Input text)");
        prt(key_guide, 0, 0);
        if (atb.marks.none()) {
            prt(fmt::format("({},{})", atb.cx, atb.cy), 0, 60);
        } else {
            prt(fmt::format("({},{})-({},{})", atb.mx, atb.my, atb.cx, atb.cy), 0, 60);
        }

        const auto cx_before = atb.cx;
        const auto cursor_column = atb.get_cursor_column();
        term_gotoxy(cursor_column - atb.left, atb.cy - atb.upper + 1);
        atb.dirty_flags = 0;
        atb.dirty_line = -1;
        atb.old_cy = atb.cy;
        atb.old_upper = atb.upper;
        atb.old_left = atb.left;
        atb.old_wid = atb.wid;
        atb.old_hgt = atb.hgt;

        key = input_special_key_with_context(InputKind::AUTOPICK_EDITOR, key_guide);

        if (key & SKEY_MASK) {
            com_id = atb.analyze_move_key(static_cast<uint32_t>(key));
        } else if (key == ESCAPE) {
            com_id = do_command_menu(0, 0);
            atb.dirty_flags |= DIRTY_SCREEN;
        } else if (!iscntrl((unsigned char)key)) {
            atb.insert_single_letter(key);
        } else {
            com_id = CommandMenuData::get_instance().get_com_id((char)key);
        }

        if (com_id > EditorCommandId::NONE) {
            quit = do_editor_command(player_ptr, atb, com_id);
        }

        atb.update_cursor_column_record(com_id, cx_before, cursor_column);
    }

    screen_load();
    const auto filename = pickpref_filename(player_ptr->base_name, atb.filename_mode);

    if (quit == APE_QUIT_AND_SAVE) {
        write_text_lines(filename, atb.get_entry_lines());
    }

    process_autopick_file(player_ptr, filename);
    cx_save = atb.cx;
    cy_save = atb.cy;
}
