#pragma once

/*!
 * @brief モンスター種族に関するプレイ中の動的な記録を管理するクラス
 * @author Hourier
 * @date 2026/05/26
 */

class MonraceRecord {
public:
    MonraceRecord() = default;
    MonraceRecord(const MonraceRecord &other) = delete;
    MonraceRecord(MonraceRecord &&other) noexcept = delete;
    MonraceRecord &operator=(const MonraceRecord &other) = delete;
    MonraceRecord &operator=(MonraceRecord &&other) noexcept = delete;
    ~MonraceRecord() = default;

    bool has_been_seen() const;
    void increment_seen_count();
    void reset_seen_count();
    int get_seen_count() const; //!< セーブ用.
    void set_seen_count(int count); //!< ロード用.

private:
    int seen_count = 0; //!< これまでにプレイヤーが目撃した数.
};
