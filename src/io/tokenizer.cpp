#include "io/tokenizer.h"

/*!
 * @brief 各種データテキストをトークン単位に分解する / Extract the first few "tokens" from a buffer
 * @param buf データテキストの参照ポインタ
 * @param num トークンの数
 * @param tokens トークンを保管する文字列参照ポインタ配列
 * @param mode オプション
 * @return 解釈した文字列数
 * @details
 * <pre>
 * This function uses "colon" and "slash" as the delimeter characters.
 * We never extract more than "num" tokens.  The "last" token may include
 * "delimeter" characters, allowing the buffer to include a "string" token.
 * We save pointers to the tokens in "tokens", and return the number found.
 * Hack -- Attempt to handle the 'c' character formalism
 * Hack -- An empty buffer, or a final delimeter, yields an "empty" token.
 * Hack -- We will always extract at least one token
 * </pre>
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

            if (should_check_quote && c == '\'') {
                in_quote = !in_quote;
                continue;
            }

            if (c == '\\' && token_end + 1 < remaining.length()) {
                token_end++;
            }
        }

        tokens.emplace_back(remaining.substr(0, token_end));
        if (token_end >= remaining.length()) {
            return tokens;
        }

        remaining = remaining.substr(token_end + 1);
    }

    tokens.emplace_back(remaining);
    return tokens;
}
