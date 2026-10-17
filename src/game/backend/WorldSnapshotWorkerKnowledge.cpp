#include "WorldSnapshotWorkerKnowledge.h"

#include <stdexcept>

namespace game::backend::world_snapshot {
void WriteWorkerKnowledge( types::Buffer& output, const std::optional< map::SmacWorkerKnowledge >& knowledge, bool smac ) {
	if ( knowledge && !smac ) { throw std::runtime_error( "Worker knowledge requires the SMAC ruleset" ); }
	const auto wire = knowledge ? std::optional< std::string >( knowledge->Serialize() ) : std::nullopt;
	output.WriteBool( wire.has_value() );
	if ( wire ) { output.WriteString( *wire ); }
}

std::optional< map::SmacWorkerKnowledge > ReadWorkerKnowledge( types::Buffer& input, bool smac ) {
	if ( !input.ReadBool() ) { return {}; }
	if ( !smac ) { throw std::runtime_error( "Unexpected worker knowledge for this ruleset" ); }
	return map::SmacWorkerKnowledge::Deserialize( input.ReadString( map::SmacWorkerKnowledge::MAX_WIRE_BYTES ) );
}

void ValidateWorkerKnowledgeGeometry( const std::optional< map::SmacWorkerKnowledge >& knowledge, const std::string& map_payload ) {
	if ( !knowledge ) { return; }
	types::Buffer input( map_payload );
	const auto width = input.ReadInt(), height = input.ReadInt();
	const auto data = knowledge->CopyData();
	if ( width != data.width || height != data.height || !data.horizontal_wrap ) {
		throw std::runtime_error( "Downloaded worker knowledge differs from periodic-X map geometry" );
	}
}
}
