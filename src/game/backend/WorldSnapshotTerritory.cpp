#include "WorldSnapshotTerritory.h"
#include <stdexcept>

namespace game::backend::world_snapshot {
void WriteTerritoryTopology( types::Buffer& output, const std::optional< map::GlsmacTerritoryTopology >& topology, bool smac ) {
	if ( topology && !smac ) { throw std::runtime_error("Territory topology requires SMAC"); }
	const auto bytes = topology ? std::optional<std::string>(topology->Serialize()) : std::nullopt;
	output.WriteBool(bytes.has_value());
	if (bytes) { output.WriteString(*bytes); }
}
std::optional< map::GlsmacTerritoryTopology > ReadTerritoryTopology( types::Buffer& input, bool smac ) {
	if (!input.ReadBool()) { return {}; }
	if (!smac) { throw std::runtime_error("Unexpected territory topology"); }
	return map::GlsmacTerritoryTopology::Deserialize(input.ReadString(map::GlsmacTerritoryTopology::MAX_WIRE_BYTES));
}
void ValidateTerritoryGeometry( const std::optional< map::GlsmacTerritoryTopology >& topology, const std::string& map_payload, bool has_knowledge ) {
	if (has_knowledge && !topology) { throw std::runtime_error("Worker knowledge requires saved territory topology"); }
	if (!topology) { return; }
	types::Buffer input(map_payload);
	const auto width=input.ReadInt(), height=input.ReadInt();
	if (width!=topology->Width() || height!=topology->Height() || !topology->Wraps()) {
		throw std::runtime_error("Territory topology differs from periodic-X map geometry");
	}
}
}
