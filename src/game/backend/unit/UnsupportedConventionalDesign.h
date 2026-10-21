#pragma once
#include <stdexcept>

namespace game::backend::unit {
// Distinguish an unimplemented physical Unit profile from corrupt or missing
// authority. Production may exclude the former; it must propagate the latter.
class UnsupportedConventionalDesign final : public std::runtime_error {
public:
    using std::runtime_error::runtime_error;
};
}
