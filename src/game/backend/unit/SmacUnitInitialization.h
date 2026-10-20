#pragma once
#include "SmacNativeUnitRecord.h"

namespace game::backend::unit {
// Original 0x5d8420 only. Count, placement, home and visibility are factory work.
SmacNativeUnitRecord InitializeSmacUnitRecord(int32_t identity,int32_t design,int32_t owner,int32_t definition_morale);
using OriginalUnitBytes=std::array<uint8_t,52>;
// Original memory fields, not the port's validated/versioned save format.
// Pool identity is external. Transient unlinked records can be represented.
OriginalUnitBytes OriginalSmacUnitBytes(const SmacNativeUnitRecord& record) noexcept;
}
