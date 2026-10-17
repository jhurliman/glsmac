#include "Default.h"

namespace game {
namespace backend {
namespace rules {

void Default::InitRules() {
}

// Temporary GLSMAC campaign default: canonical first-Base initialization is
// currently admitted at Librarian. Explicit user selections remain unchanged.
static const std::string s_default_difficulty_level = "Librarian";
const std::string& Default::GetDefaultDifficultyLevel() const {
	return s_default_difficulty_level;
}

}
}
}
