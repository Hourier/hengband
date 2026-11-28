#pragma once

#include "autopick/autopick-util.h"
#include "util/flag-group.h"
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

enum class EditorCommandId;
class ItemEntity;

enum class AutopickMark {
    MARK = 0,
    BY_SHIFT = 1,
    MAX,
};

enum class AutopickEntryStatus {
    BYPASS = 0,
    EXPRESSION = 1,
    AUTOREGISTER = 2,
    EXPRESSION_ERROR = 3,
    INCLUDE = 4,
    MAX,
};

struct AutopickEntryLine {
    std::string line;
    EnumClassFlagGroup<AutopickEntryStatus> state;
};

/*!
 * @brief 自動拾い設定ファイル用テキストエディタの状態を保持するクラス
 *
 * 編集中の行データ・各行の状態・カーソル位置・画面スクロール位置・選択範囲・検索条件・ヤンクバッファ・再描画用ダーティ情報・変更フラグなどをまとめて管理する。
 * ユーザがエディタを操作する度に、このクラスのインスタンスを通じて状態が更新される。
 */
class AutopickTextBody {
public:
    AutopickTextBody(int cx, int cy); // 画面表示はX→Yの順番であることが多いので一応揃える.

    int cx;
    int cy;
    int rec_cx = 0; //!< 上下に移動する間に覚えておくカーソルの表示桁 (0 なら覚えていない)
    int wid = 0;
    int hgt = 0;
    int upper = 0;
    int left = 0;
    int old_wid = -1;
    int old_hgt = -1;
    int old_cy = -1;
    int old_upper = -1;
    int old_left = -1;
    int mx = 0;
    int my = 0;
    EnumClassFlagGroup<AutopickMark> marks{};

    std::shared_ptr<ItemEntity> search_item = nullptr;
    std::string search_str = "";
    std::string last_destroyed = "";

    std::vector<std::string> yank{};
    bool yank_eol = false;

    std::vector<AutopickEntryLine> lines;

    uint16_t dirty_flags = 0;
    int dirty_line = -1;
    PickType filename_mode{};
    EditorCommandId old_com_id{};

    bool changed = false;

    static bool is_inside_multibyte_char(std::string_view line, int pos);

    bool has_line(int y) const;
    bool is_last_line(int y) const;
    std::vector<std::string> get_entry_lines() const;
    int get_cursor_column() const;
    bool is_cursor_inside_multibyte_char() const;

    void set_lines(std::vector<std::string> new_lines);
    void adjust_cursor_column();
    void update_cursor_column_record(EditorCommandId com_id, int cx_before, int column_before);
    EditorCommandId analyze_move_key(uint32_t skey);
    void search_for_string(std::string_view pattern, bool forward);
    void toggle_keyword(BIT_FLAGS flg);
    void toggle_command_letter(AutopickMethod am);
    void add_keyword(BIT_FLAGS flg);
    void kill_yank_chain();
    void add_str_to_yank(std::string_view str);
    void copy_text_to_yank();
    void check_expression_line(int y);
    bool add_empty_line();
    void insert_return_code();
    void insert_single_letter(int key);
    bool insert_macro_line();
    bool insert_keymap_line();
    void kill_line_segment(int y, int x0, int x1, bool whole);
};
