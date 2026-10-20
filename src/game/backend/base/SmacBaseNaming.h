#pragma once
#include "game/backend/faction/SmacStartupFaction.h"

namespace game::backend::base {
// Already parsed ordinary single-byte text, supplied from owned installed data.
struct SmacBaseNameCatalog {std::string noun;std::vector<std::string> land,sea,generic;};
struct SmacBaseNameStore {uint32_t offset,value,caller;};
using SmacBaseNameObserver=std::function<void(const SmacBaseNameStore&)>;
// Original 4f9950, nonnegative cursors and ordinary installed labels. Preview
// computes text without advancing. Output retains all bytes outside native
// copies; this API does not use registered-name uniqueness or gameplay RNG.
std::string GenerateSmacBaseName(faction::SmacStartupFaction& actor,std::array<uint8_t,128>& output,
    const SmacBaseNameCatalog& catalog,int32_t owner,int32_t pattern,bool advance,bool sea,
    const SmacBaseNameObserver& observer={});
}
