#include "SmacWorkerMapFrame.h"
#include "game/backend/map/SmacWorkerMapFrame.h"
#include "game/backend/base/SmacWorkedSquareCollection.h"
#include "fixtures/SmacWorkerMapFrame.h"
#include "task/gsetests/GSETests.h"

#include <functional>
#include <stdexcept>
#include <utility>

namespace gse::tests {
namespace {
using Frame = game::backend::map::SmacWorkerMapFrame;
namespace fixture = smac_worker_frame_fixture;
bool Rejects( const std::function<void()>& operation, const std::string& message = {} ) {
	try { operation(); } catch ( const std::invalid_argument& error ) { return message.empty() || std::string(error.what()).find(message) != std::string::npos; }
	return false;
}
Frame::Data Input( size_t index ) {
	auto data = fixture::inputs[index].data;
	for ( const auto& run : fixture::inputs[index].runs ) { data.tiles.insert( data.tiles.end(), run.count, run.tile ); }
	return data;
}
void Omit( Frame::Data& data, size_t field ) {
	if ( field == 1 ) { for ( auto& tile : data.tiles ) { tile.explored_by = 0; } }
	if ( field == 2 ) { for ( auto& tile : data.tiles ) { tile.claimant.reset(); } }
	if ( field == 3 ) { for ( auto& unit : data.units ) { unit.visible_to = 0; } }
	if ( field == 4 ) { for ( auto& unit : data.units ) { unit.active_convoy = false; } }
	if ( field == 5 ) { for ( auto& base : data.bases ) { base.selected_radius = 0; } }
	if ( field == 6 ) { data.observer_relations = {}; }
}
size_t At( const Frame::Data& data, Frame::Coordinate coordinate ) {
	return static_cast<size_t>(coordinate[1]) * static_cast<size_t>(data.width / 2) + static_cast<size_t>(coordinate[0] / 2);
}
Frame::Data Empty( int32_t width = 16, int32_t height = 16, Frame::Coordinate center = {8,8} ) {
	Frame::Data data{width,height,1,true,true,false,0,{},{},{{1,center,0}}, {}};
	data.tiles.resize( static_cast<size_t>(width) * static_cast<size_t>(height) / 2, {false,2,{}} );
	return data;
}
void Word( std::string& wire, size_t offset, uint32_t value ) {
	for ( unsigned i=0; i<4; ++i ) { wire[offset+i] = static_cast<char>((value >> (i*8)) & 255); }
}
}

void AddSmacWorkerMapFrameTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC worker map frame replays 35 original states and 210 omissions", GT() {
		GT_ASSERT( std::size(fixture::states) == 35 && std::size(fixture::inputs) == 5, "original fixture shape changed" );
		size_t states = 0, evaluations = 0;
		for ( const auto& state : fixture::states ) {
			for ( size_t omission=0; omission<7; ++omission ) {
				auto data = Input(state.frame); Omit(data,omission);
				const auto frame = Frame::Create(data); const auto saved = frame.Serialize();
				const auto restored = Frame::Deserialize(saved);
				GT_ASSERT( restored == frame && restored.Serialize() == saved, "owned codec lost frame: " + std::string(state.id) );
				const auto& expected = fixture::inputs[state.frame].actual[omission];
				GT_ASSERT( expected.size() == data.bases.size(), "missing original Base result" );
				for ( size_t b=0; b<expected.size(); ++b ) {
					const auto actual = restored.Evaluate(b); const auto& row = expected[b];
					GT_ASSERT( actual == frame.Evaluate(b), "restore changed worker query" );
					GT_ASSERT( actual.selected_radius == row.selected && actual.unavailable == row.unavailable, "literal original masks differ: " + std::string(state.id) );
					for ( size_t i=0; i<21; ++i ) {
						const Frame::Coordinate coordinate{row.center[0]+fixture::offsets[i][0],row.center[1]+fixture::offsets[i][1]};
						GT_ASSERT( actual.squares[i] && actual.squares[i]->coordinate == coordinate, "original radius ordering differs" );
						const auto& square = *actual.squares[i];
						GT_ASSERT( square.claimant.value_or(-1) == row.claims[i] && square.recognized_owner.value_or(-1) == row.recognized[i]
							&& square.explored == bool(row.explored & (1u<<i)), "literal original knowledge differs: " + std::string(state.id) );
					}
					++evaluations;
				}
				GT_ASSERT( frame.Serialize() == saved, "detached evaluation mutated frame" ); ++states;
			}
		}
		GT_ASSERT( states == 245 && evaluations == 343, "original replay coverage differs" );
		GT_OK();
	} );
	task->AddTest( "SMAC worker map frame separates raw claims recognition revelation and native visibility", GT() {
		auto data = Empty(); const Frame::Coordinate neighbor{10,8}; auto& tile = data.tiles[At(data,neighbor)];
		tile.claimant = 2;
		auto result = Frame::Create(data).Evaluate(0);
		GT_ASSERT( result.unavailable == 1 && result.squares[2]->claimant == 2 && !result.squares[2]->recognized_owner, "raw foreign claim became recognized" );
		data.observer_relations[1].territory_recognition = true;
		GT_ASSERT( Frame::Create(data).Evaluate(0).unavailable == 1, "reverse/observer relation recognized foreign claim" );
		data.observer_relations[2].territory_recognition = true;
		GT_ASSERT( Frame::Create(data).Evaluate(0).unavailable == 5, "directed recognition did not block foreign claim" );
		data.observer_relations = {}; data.foreign_disclosure = true;
		GT_ASSERT( Frame::Create(data).Evaluate(0).squares[2]->recognized_owner == 2, "disclosure override ignored" );
		tile.claimant = 0;
		GT_ASSERT( !Frame::Create(data).Evaluate(0).squares[2]->recognized_owner, "native claim became recognized territory" );
		tile.claimant = 1; data.foreign_disclosure = false;
		GT_ASSERT( Frame::Create(data).Evaluate(0).squares[2]->recognized_owner == 1, "own claim required a foreign predicate" );
		tile.explored_by = 1; data.units = {{0,neighbor,2,false}};
		GT_ASSERT( Frame::Create(data).Evaluate(0).unavailable == 5, "native knowledge/visibility conflated with observer exploration" );
		data.all_map_revealed = 1;
		GT_ASSERT( Frame::Create(data).Evaluate(0).unavailable == 5, "another faction revelation admitted observer" );
		data.all_map_revealed = 2; data.units[0].visible_to = 0;
		result = Frame::Create(data).Evaluate(0);
		GT_ASSERT( result.unavailable == 1 && !result.squares[2]->explored, "revelation rewrote exploration or invisible native blocked" );
		data.units[0] = {7,neighbor,0,true}; data.observer_relations[7].foreign_unit_exemption = true;
		GT_ASSERT( Frame::Create(data).Evaluate(0).unavailable == 5, "convoy was exempted by visibility or relationship" );
		data.units[0] = {2,neighbor,2,false}; data.observer_relations[2].foreign_unit_exemption = true;
		GT_ASSERT( Frame::Create(data).Evaluate(0).unavailable == 1, "resolved foreign unit exemption lost" );
		tile.claimant = 2; data.observer_relations[2].territory_recognition = true;
		GT_ASSERT( Frame::Create(data).Evaluate(0).unavailable == 5, "unit exemption bypassed territory" );
		GT_OK();
	} );
	task->AddTest( "SMAC worker map frame preserves ordered records complete knowledge and owned lifetimes", GT() {
		auto input = Input(4); const auto original = input;
		const auto frame = Frame::Create(input); const auto saved = frame.Serialize();
		input.tiles.clear(); input.bases.clear(); input.units.clear();
		GT_ASSERT( frame.CopyData() == original, "caller mutation changed owned frame" );
		auto copied = frame.CopyData(); copied.tiles[0] = {true,255,0}; copied.units[0].visible_to = 255;
		GT_ASSERT( frame.Serialize() == saved, "returned data mutates stored frame" );
		auto clone = frame; auto moved = std::move(clone);
		GT_ASSERT( moved == frame && moved.Evaluate(1) == frame.Evaluate(1), "copy/move lost frame identity" );
		const auto changed = Frame::Create(copied); const auto changed_wire = changed.Serialize();
		GT_ASSERT( changed_wire != saved && Frame::Deserialize(changed_wire).CopyData() == copied, "unqueried/native/full-mask data was discarded" );
		std::swap(copied.bases[0],copied.bases[1]); std::swap(copied.units[0],copied.units[1]);
		const auto reordered = Frame::Create(copied);
		GT_ASSERT( reordered.Serialize() != changed_wire && reordered.Evaluate(0) == changed.Evaluate(1), "ordered Base identities changed or unordered storage substituted" );
		GT_ASSERT( Frame::Deserialize(reordered.Serialize()).CopyData() == copied, "Unit ordering was not preserved" );
		copied.observer = 7; copied.horizontal_wrap = false; copied.foreign_disclosure = true; copied.all_map_revealed = 255;
		for ( size_t i=0; i<8; ++i ) { copied.observer_relations[i] = {bool(i&1),!bool(i&1)}; }
		for ( auto& base : copied.bases ) { base.owner = 7; base.selected_radius = 0x155555u; }
		const auto flags = Frame::Deserialize(Frame::Create(copied).Serialize()).CopyData();
		GT_ASSERT( flags == copied && flags.observer == 7 && !flags.horizontal_wrap && flags.foreign_disclosure && flags.all_map_revealed == 255
			&& flags.observer_relations[0].territory_recognition && !flags.observer_relations[0].foreign_unit_exemption
			&& flags.observer_relations[7].foreign_unit_exemption && !flags.observer_relations[7].territory_recognition
			&& flags.bases[1].owner == 7 && flags.bases[1].selected_radius == 0x155555u, "nonzero header predicates or Base metadata changed on restore" );
		GT_OK();
	} );
	task->AddTest( "SMAC worker map frame reuses single wrap clipping aliases and selected21 projection", GT() {
		auto data = Empty(2,5,{0,2}); data.tiles[At(data,{0,2})].explored_by = 0;
		const auto frame = Frame::Create(data); const auto result = frame.Evaluate(0);
		const uint32_t clipped = (1u<<13)|(1u<<16)|(1u<<17)|(1u<<18)|(1u<<19)|(1u<<20);
		GT_ASSERT( result.unavailable == (clipped|1u|4u|64u), "single-adjustment alias/clipping differs" );
		GT_ASSERT( result.squares[0] == result.squares[2] && result.squares[0] == result.squares[6] && !result.squares[18], "dense aliases or clipped position differ" );
		data.bases[0].selected_radius = Frame::RADIUS_MASK;
		GT_ASSERT( Frame::Deserialize(Frame::Create(data).Serialize()).Evaluate(0).selected_radius == Frame::RADIUS_MASK, "selected21 data lost" );
		for ( uint32_t bit : {1u<<21,1u<<31} ) {
			data.bases[0].selected_radius = bit;
			GT_ASSERT( Rejects([&]{Frame::Create(data);},"selected32"), "full selected authority silently truncated" );
		}
		data = Empty(); data.bases.push_back({2,{12,8},1u<<6}); data.tiles[At(data,{10,8})].sea = true;
		GT_ASSERT( Frame::Create(data).Evaluate(0).unavailable == (1u|4u), "foreign sea reservation or marker projection differs" );
		GT_ASSERT( Rejects([&]{Frame::Create(data).Evaluate(1);},"observer"), "foreign Base borrowed observer context" );
		GT_ASSERT( Rejects([&]{Frame::Create(data).Evaluate(2);},"index"), "unknown Base index admitted" );
		GT_OK();
	} );
	task->AddTest( "SMAC worker map frame rejects malformed complete wire and late owned records", GT() {
		auto original = Empty(); original.units.push_back({0,{10,8},2,false});
		const auto frame = Frame::Create(original); const auto saved = frame.Serialize();
		const size_t base = 56 + original.tiles.size()*3, unit = base + 13;
		auto native = original; native.observer_relations[0].foreign_unit_exemption = true;
		GT_ASSERT( Rejects([&]{Frame::Create(native);},"Native identity"), "native exemption admitted at frame creation" );
		auto native_wire = saved; Word(native_wire,36,1);
		GT_ASSERT( Rejects([&]{Frame::Deserialize(native_wire);},"Native identity"), "native exemption admitted at frame decode" );
		for ( int scenario=0; scenario<27; ++scenario ) {
			auto wire = saved;
			if ( scenario == 0 ) { wire[0] = 'X'; }
			if ( scenario == 1 ) { Word(wire,4,0); }
			if ( scenario == 2 ) { Word(wire,4,2); }
			if ( scenario == 3 ) { Word(wire,8,0); }
			if ( scenario == 4 ) { Word(wire,8,3); }
			if ( scenario == 5 ) { Word(wire,8,0xffffffffu); }
			if ( scenario == 6 ) { Word(wire,12,0); }
			if ( scenario == 7 ) { Word(wire,16,2); }
			if ( scenario == 8 ) { Word(wire,20,0); }
			if ( scenario == 9 ) { Word(wire,20,8); }
			if ( scenario == 10 ) { Word(wire,24,0); }
			if ( scenario == 11 ) { Word(wire,28,2); }
			if ( scenario == 12 ) { Word(wire,32,256); }
			if ( scenario == 13 ) { Word(wire,36,256); }
			if ( scenario == 14 ) { Word(wire,40,256); }
			if ( scenario == 15 ) { Word(wire,44,0xffffffffu); }
			if ( scenario == 16 ) { Word(wire,48,0xffffffffu); }
			if ( scenario == 17 ) { Word(wire,52,0xffffffffu); }
			if ( scenario == 18 ) { wire[base-3] = 2; }
			if ( scenario == 19 ) { wire[base-1] = 8; }
			if ( scenario == 20 ) { wire[base] = 0; }
			if ( scenario == 21 ) { Word(wire,base+1,9); }
			if ( scenario == 22 ) { Word(wire,base+9,1u<<31); }
			if ( scenario == 23 ) { wire[unit] = 8; }
			if ( scenario == 24 ) { Word(wire,unit+5,0x80000000u); }
			if ( scenario == 25 ) { wire.back() = 2; }
			if ( scenario == 26 ) { wire += '\0'; }
			GT_ASSERT( Rejects([&]{Frame::Deserialize(wire);}), "malformed frame admitted: " + std::to_string(scenario) );
			GT_ASSERT( frame.Serialize() == saved, "failed decode changed prior frame" );
		}
		for ( size_t size : {size_t(0),size_t(55),saved.size()-1} ) {
			GT_ASSERT( Rejects([&]{Frame::Deserialize(saved.substr(0,size));}), "truncated frame admitted" );
		}
		auto duplicate = saved; duplicate.insert(base+13,saved.substr(base,13)); Word(duplicate,48,2);
		GT_ASSERT( Rejects([&]{Frame::Deserialize(duplicate);},"duplicate Base"), "duplicate Base center admitted" );
		for ( int scenario=0; scenario<7; ++scenario ) {
			auto data = original;
			if ( scenario == 0 ) { data.tiles.pop_back(); }
			if ( scenario == 1 ) { data.observer = -1; }
			if ( scenario == 2 ) { data.tiles.back().claimant = -1; }
			if ( scenario == 3 ) { data.bases.back().owner = 8; }
			if ( scenario == 4 ) { data.units.back().coordinate[0] = -1; }
			if ( scenario == 5 ) { data.ordinary_local_mode = false; }
			if ( scenario == 6 ) { data.bases.push_back(data.bases[0]); }
			GT_ASSERT( Rejects([&]{Frame::Create(data);}), "invalid owned inputs admitted" );
		}
		GT_OK();
	} );
	task->AddTest( "SMAC worker map frame checks aggregate counts before bounded allocation", GT() {
		auto data = Empty(512,256,{0,0}); data.bases.clear();
		// 56 + 65536*3 + 9*13 + 172761*11 == 2 MiB, exact legal cap.
		for ( int32_t i=0; i<9; ++i ) { data.bases.push_back({1,{i*2,0},0}); }
		data.units.resize(172761,{0,{0,0},255,false});
		const auto frame = Frame::Create(data); const auto wire = frame.Serialize();
		GT_ASSERT( wire.size() == Frame::MAX_WIRE_BYTES, "exact-cap fixture is not exact" );
		GT_ASSERT( Frame::Deserialize(wire) == frame, "exact-cap legal frame rejected" );
		GT_ASSERT( Rejects([&]{Frame::Deserialize(wire+'x');},"length exceeds"), "cap+1 failed after length admission" );
		data.units.push_back({0,{0,0},0,false});
		GT_ASSERT( Rejects([&]{Frame::Create(data);},"aggregate"), "oversized owned frame accepted" );
		auto truncated = wire.substr(0,56); Word(truncated,52,0xffffffffu);
		GT_ASSERT( Rejects([&]{Frame::Deserialize(truncated);},"aggregate"), "huge declared count bypassed preallocation budget" );
		GT_OK();
	} );
}
}
