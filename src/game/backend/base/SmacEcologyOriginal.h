#pragma once
#include "SmacEcology.h"

namespace game::backend::base {
// Same ecology algorithm with the original literal DWORD arithmetic, for raw
// record execution. Existing public checked functions retain their admission.
SmacEcologyBurden CalculateOriginalSmacEcologyBurden(const SmacEcologyBurdenInput& input);
SmacEcologyDamage CalculateOriginalSmacEcologyDamage(const SmacEcologyBurden& burden,const SmacEcologyScalingInput& input);
}
