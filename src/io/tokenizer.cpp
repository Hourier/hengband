#include "io/tokenizer.h"

/*!
 * @brief 各種データテキストをトークン単位に分解する
 * @param buf データテキスト
 * @param num 最大トークン数
 * @param should_check_quote シングルクォートで括られた部分を区切り文字として扱うか否か
 * @return トークン文字列の配列
 */
std::vector<std::string> tokenize(std::string_view buf, size_t num, bool should_check_quote)
{
    std::vector<std::string> tokens;
    if (num == 0) {
        return tokens;
    }

    tokens.reserve(num);
    auto remaining = buf;
    while ((tokens.size() < num - 1) && !remaining.empty()) {
        size_t token_end = 0;
        auto in_quote = false;
        for (; token_end < remaining.length(); token_end++) {
            const auto c = remaining.at(token_end);
            if (!in_quote && (c == ':' || c == '/')) {
                break;
            }

            if (should_check_quote && (c == '\\') && (token_end + 1 < remaining.length())) {
                token_end++;
                continue;
            }

            if (should_check_quote && (c == '\'')) {
                in_quote = !in_quote;
            }
        }

        tokens.emplace_back(remaining.substr(0, token_end));
        if (token_end >= remaining.length()) {
            return tokens;
        }

        remaining = remaining.substr(token_end + 1);
    }

    if (!remaining.empty()) {
        tokens.emplace_back(remaining);
    }

    return tokens;
}
