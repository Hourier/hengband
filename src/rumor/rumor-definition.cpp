#include "rumor/rumor-definition.h"

RumorDefinition::RumorDefinition(const std::string &description)
    : description(description)
{
}

const std::string &RumorDefinition::get_description() const
{
    return this->description;
}
