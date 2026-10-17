#include "SmacBaseAllocationState.h"
#include "fixtures/SmacBaseAllocationState.h"
#include "game/backend/base/SmacAutomaticWorkerAllocation.h"
#include "task/gsetests/GSETests.h"

#include <stdexcept>
#include <vector>

namespace gse::tests {
namespace {
using namespace game::backend::base;
using State = SmacBaseAllocationState;
namespace fixture = smac_base_allocation_fixture;
template< typename F > bool Rejects( F&& call ) {
	try { call(); } catch ( const std::exception& ) { return true; } return false;
}
types::Buffer Frame( int64_t version, int64_t selected, int64_t count, const std::string& slots ) {
	types::Buffer buffer; buffer.WriteInt( version ); buffer.WriteInt( selected ); buffer.WriteInt( count ); buffer.WriteString( slots ); return buffer;
}
}

void AddSmacBaseAllocationStateTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC Base allocation codec preserves 49 captured complete slot records", GT() {
		size_t count = 0;
		for ( const auto& row : fixture::captures ) {
			const auto encoded = row.state.Serialize(); const auto bytes = encoded.ToString();
			const auto decoded = State::Deserialize( encoded );
			GT_ASSERT( encoded.lenw == State::WIRE_BYTES && decoded == row.state, "captured authority changed: " + std::string(row.id) );
			GT_ASSERT( decoded.Serialize().ToString() == bytes && encoded.lenr == 0 && encoded.ToString() == bytes, "decode changed frame or logical byte order" );
			++count;
		}
		GT_ASSERT( count == 49, "authority capture coverage changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC Base allocation restore preserves six original high-bit and inactive-slot continuations", GT() {
		for ( const auto& row : fixture::controls ) {
			const auto encoded = row.input.Serialize();
			const auto restored = State::Deserialize( encoded );
			for ( const auto& state : { row.input, restored } ) {
				SmacAutomaticWorkerInput input{};
				input.population = row.upkeep ? 2 : 1; input.turn = 100; input.nutrients_per_citizen = 2;
				input.production = SmacWorkerProduction::Unit; input.priorities.human = !row.upkeep; input.upkeep = row.upkeep;
				input.selected = state.selected; input.specialists = state.specialists;
				// Actual unavailable output is a 21-bit mask. Its upper bits are
				// zero, so diagnostic selected bits21/31 reach admission intact.
				input.unavailable = 0x1ffff8u;
				input.catalog.minimum_base_size = 5;
				input.catalog.rows = {{ {3,0,0,false,false}, {0,2,0,true,false}, {0,0,3,false,false},
					{3,0,2,false,false}, {2,2,0,false,false}, {0,1,3,false,false}, {2,2,4,false,false} }};
				const auto result = SmacAutomaticWorkerAllocation::Allocate( input, [ & ]( int32_t index, const SmacWorkerStepState& ) {
					if ( index < 0 || index > 2 ) { throw std::runtime_error("Unexpected authority-control candidate"); }
					if ( row.upkeep ) { return SmacWorkerTileYield{ index == 0 ? std::array<int32_t,3>{50,1,1} : std::array<int32_t,3>{}, false }; }
					return SmacWorkerTileYield{ index == 0 ? std::array<int32_t,3>{2,1,1} : index == 1 ? std::array<int32_t,3>{2,3,4} : std::array<int32_t,3>{4,1,2}, false };
				} );
				const State final{ result.workers.visited, result.specialists };
				GT_ASSERT( final == row.expected && result.workers.intake == row.intake, "original continuation changed after persistence: " + std::string(row.id) );
			}
			GT_ASSERT( restored == row.input && encoded.lenr == 0, "restoring normalized authoritative state" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Base allocation codec retains full storage domain without inventing slots", GT() {
		State state{}; state.selected = UINT32_MAX;
		for ( size_t i = 0; i < 16; ++i ) { state.specialists.types[i] = static_cast<uint8_t>(i % 7); }
		for ( const auto count : {0,1,16,17,127} ) {
			state.specialists.total_count = count;
			GT_ASSERT( State::Deserialize(state.Serialize()) == state, "upper selected bits or inactive/overflow storage changed" );
		}
		for ( size_t i = 0; i < 16; ++i ) {
			auto bad = state; bad.specialists.types[i] = 7;
			GT_ASSERT( Rejects([&]{ bad.Serialize(); }), "invalid stored type admitted" );
		}
		for ( const auto count : {-1,128} ) {
			auto bad = state; bad.specialists.total_count = count;
			GT_ASSERT( Rejects([&]{ bad.Serialize(); }), "invalid persistent count admitted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC Base allocation codec rejects malformed frames without consuming input", GT() {
		const auto valid = fixture::controls[0].input.Serialize().ToString();
		for ( size_t size = 0; size < valid.size(); ++size ) {
			const types::Buffer truncated( valid.substr(0,size) );
			GT_ASSERT( Rejects([&]{ State::Deserialize(truncated); }) && truncated.lenr == 0, "truncated frame consumed or admitted" );
		}
		const types::Buffer trailing(valid + "x");
		GT_ASSERT( Rejects([&]{ State::Deserialize(trailing); }) && trailing.lenr == 0, "trailing frame admitted" );
		const std::string slots(16,'\0');
		const std::vector<types::Buffer> malformed{
			Frame(0,1,1,slots), Frame(2,1,1,slots), Frame(1,-1,1,slots), Frame(1,int64_t(UINT32_MAX)+1,1,slots),
			Frame(1,1,-1,slots), Frame(1,1,128,slots), Frame(1,1,1,std::string(15,'\0')), Frame(1,1,1,std::string(17,'\0')),
			Frame(1,1,1,std::string(16,'\x07')), Frame(1,1,1,std::string(16,'\xff'))
		};
		for ( const auto& bad : malformed ) {
			const auto before = bad.ToString();
			GT_ASSERT( Rejects([&]{ State::Deserialize(bad); }) && bad.lenr == 0 && bad.ToString() == before, "malformed frame changed caller input" );
		}
		types::Buffer consumed(valid); consumed.ReadInt(); const auto cursor = consumed.lenr;
		GT_ASSERT( Rejects([&]{ State::Deserialize(consumed); }) && consumed.lenr == cursor, "partial isolated frame accepted" );
		GT_OK();
	} );
}

}
