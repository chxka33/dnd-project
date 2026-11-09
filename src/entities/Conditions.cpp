#include "entities/Conditions.hpp"

#include <algorithm>

namespace dnd::entities
{

    bool isStandardCondition(const std::string &name)
    {
        return std::any_of(kStandardConditions.begin(), kStandardConditions.end(),
                           [&name](const char *standard) { return name == standard; });
    }

} // namespace dnd::entities
