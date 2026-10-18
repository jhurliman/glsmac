#pragma once
#include <cstddef>
#include "gse/Value.h"
namespace game::backend {
class Game;
// Player input boundary. Validation failures are reported as ordinary messages;
// Event and native movement retain their strict, side-effect-free validators.
bool QueueLocalConventionalMove(GSE_CALLABLE, Game& game, size_t unit_id, size_t x, size_t y);
}
