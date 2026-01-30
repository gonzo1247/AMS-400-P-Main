#pragma once

#include <cstdint>
#include <vector>

#include "SharedDefines.h"


class CompanyContactDataManager
{
   public:
    static std::vector<CompanyContactEntry> LoadAll(bool includeDeleted);
};
