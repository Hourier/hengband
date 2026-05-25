#include "system/monrace/monrace-record.h"
#include <limits>

bool MonraceRecord::has_been_seen() const
{
    return this->seen_count > 0;
}

void MonraceRecord::increment_seen_count()
{
    if (this->seen_count < std::numeric_limits<short>::max()) {
        this->seen_count++;
    }
}

void MonraceRecord::reset_seen_count()
{
    this->seen_count = 0;
}

int MonraceRecord::get_seen_count() const
{
    return this->seen_count;
}

void MonraceRecord::set_seen_count(int count)
{
    this->seen_count = count;
}
