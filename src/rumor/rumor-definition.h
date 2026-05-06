#pragma once
/*!
 * @brief 噂定義
 * @date 2026/05/06
 * @author Hourier
 */

#include <string>

class RumorDefinition {
public:
    RumorDefinition(const std::string &description);

    const std::string &get_description() const;

private:
    std::string description;
};
