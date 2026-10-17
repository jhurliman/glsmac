#pragma once

#include "map/SmacWorkerKnowledge.h"
#include "types/Buffer.h"

namespace game::backend::world_snapshot {
void WriteWorkerKnowledge( types::Buffer& output, const std::optional< map::SmacWorkerKnowledge >& knowledge, bool smac );
std::optional< map::SmacWorkerKnowledge > ReadWorkerKnowledge( types::Buffer& input, bool smac );
void ValidateWorkerKnowledgeGeometry( const std::optional< map::SmacWorkerKnowledge >& knowledge, const std::string& map_payload );
}
