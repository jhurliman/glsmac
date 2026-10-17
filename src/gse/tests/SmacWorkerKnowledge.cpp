#include "SmacWorkerKnowledge.h"
#include "game/backend/map/SmacWorkerKnowledge.h"
#include "fixtures/SmacWorkerMapFrame.h"
#include "task/gsetests/GSETests.h"

#include <algorithm>
#include <functional>
#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
using Knowledge = game::backend::map::SmacWorkerKnowledge;
using Frame = game::backend::map::SmacWorkerMapFrame;
namespace fixture = smac_worker_frame_fixture;
bool Rejects( const std::function<void()>& f, const std::string& message = {} ) {
	try { f(); } catch ( const std::invalid_argument& e ) { return message.empty() || std::string(e.what()).find(message) != std::string::npos; }
	return false;
}
struct Inputs {
	Knowledge::Data knowledge;
	std::vector<bool> sea;
	std::vector<Frame::Base> bases;
	std::vector<Knowledge::LiveUnit> units;
};
Inputs Split( const Frame::Data& frame ) {
	Inputs input{{frame.width,frame.height,frame.horizontal_wrap,frame.ordinary_local_mode,frame.all_map_revealed,{},{},{}},{},frame.bases,{}};
	input.knowledge.observers[frame.observer] = Knowledge::Observer{frame.foreign_disclosure,frame.observer_relations};
	for ( const auto& tile : frame.tiles ) { input.knowledge.tiles.push_back({tile.explored_by,tile.claimant}); input.sea.push_back(tile.sea); }
	// Explicit test-only port identity assignment, not original Unit identity.
	for ( size_t i=0; i<frame.units.size(); ++i ) {
		const auto id = uint64_t(1001 + 17*i); const auto& unit = frame.units[i];
		input.knowledge.units.push_back({id,unit.visible_to,unit.active_convoy}); input.units.push_back({id,unit.owner,unit.coordinate});
	}
	std::reverse(input.units.begin(),input.units.end()); return input;
}
Inputs Small() {
	Frame::Data data{4,4,1,true,true,false,0,{},{},{{1,{0,0},0}},{{0,{2,0},2,false},{2,{1,1},1,true}}};
	data.tiles.resize(8,{false,2,{}}); return Split(data);
}
Frame Project( const Knowledge& value, const Inputs& input, int32_t observer=1 ) { return value.Project(observer,input.sea,input.bases,input.units); }
void Write( std::string& wire, size_t offset, uint64_t value, unsigned bytes=4 ) {
	for ( unsigned i=0; i<bytes; ++i ) { wire[offset+i] = static_cast<char>((value>>(i*8))&255); }
}
}

void AddSmacWorkerKnowledgeTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC worker knowledge preserves original frame results through explicit ID joins", GT() {
		size_t evaluations=0, roundtrips=0;
		for ( const auto& state : fixture::states ) {
			for ( size_t omission=0; omission<7; ++omission ) {
				auto data = fixture::inputs[state.frame].data;
				for ( const auto& run : fixture::inputs[state.frame].runs ) { data.tiles.insert(data.tiles.end(),run.count,run.tile); }
				if ( omission==1 ) { for ( auto& tile : data.tiles ) { tile.explored_by=0; } }
				if ( omission==2 ) { for ( auto& tile : data.tiles ) { tile.claimant.reset(); } }
				if ( omission==3 ) { for ( auto& unit : data.units ) { unit.visible_to=0; } }
				if ( omission==4 ) { for ( auto& unit : data.units ) { unit.active_convoy=false; } }
				if ( omission==5 ) { for ( auto& base : data.bases ) { base.selected_radius=0; } }
				if ( omission==6 ) { data.observer_relations={}; }
				const auto input = Split(data); const auto owned = Knowledge::Create(input.knowledge);
				const auto restored = Knowledge::Deserialize(owned.Serialize());
				const auto projected = Project(restored,input,data.observer); const auto original = Frame::Create(data);
				GT_ASSERT( restored==owned && projected==original, "knowledge roundtrip or ID ordering changed complete frame" );
				const auto& expected = fixture::inputs[state.frame].actual[omission];
				for ( size_t b=0; b<expected.size(); ++b ) {
					const auto result = projected.Evaluate(b);
					GT_ASSERT( result==original.Evaluate(b) && result.selected_radius==expected[b].selected && result.unavailable==expected[b].unavailable, "knowledge projection changed literal original result" ); ++evaluations;
				}
				++roundtrips;
			}
		}
		GT_ASSERT( roundtrips==245 && evaluations==343, "frame comparison coverage changed" ); GT_OK();
	} );
	task->AddTest( "SMAC worker knowledge preserves absent observers native bits and owned copies", GT() {
		auto input=Small(); input.knowledge.tiles[0]={1,0}; input.knowledge.tiles[1]={128,{}};
		input.knowledge.observers[0]=Knowledge::Observer{true,{}};
		input.knowledge.observers[7]=Knowledge::Observer{false,{}};
		input.knowledge.observers[7]->relations[0]={true,false}; input.knowledge.observers[7]->relations[1]={false,true};
		input.knowledge.all_map_revealed=129; input.knowledge.units[0].id=uint64_t(std::numeric_limits<int64_t>::max());
		const auto original=input.knowledge; const auto value=Knowledge::Create(original); const auto bytes=value.Serialize();
		input.knowledge.tiles.clear(); auto detached=value.CopyData(); detached.observers[1].reset(); detached.units.clear();
		auto copy=value; auto moved=std::move(copy);
		GT_ASSERT( moved.CopyData()==original && value.Serialize()==bytes && Knowledge::Deserialize(bytes)==value, "knowledge aliases caller or loses zero/native/absent fields" );
		const auto result=Knowledge::Deserialize(bytes).CopyData();
		GT_ASSERT( result.tiles[0].claimant==0 && !result.tiles[1].claimant && !result.observers[2] && result.observers[0] && result.observers[7]
			&& result.tiles[0].explored_by==1 && result.tiles[1].explored_by==128 && result.units[1].visible_to==1 && result.units[1].active_convoy, "native and none or observer rows were normalized" ); GT_OK();
	} );
	task->AddTest( "SMAC worker knowledge rejects missing extra and duplicate entity bindings", GT() {
		const auto input=Small(); const auto value=Knowledge::Create(input.knowledge); const auto bytes=value.Serialize();
		for ( int scenario=0; scenario<11; ++scenario ) {
			auto altered=input; int32_t observer=1;
			if(scenario==0){altered.units.pop_back();} if(scenario==1){altered.units.push_back({999,1,{0,0}});}
			if(scenario==2){altered.units[1].id=altered.units[0].id;} if(scenario==3){altered.units[0].id=999;}
			if(scenario==4){altered.units[0].id=0;} if(scenario==5){altered.units[0].id=uint64_t(1)<<63;}
			if(scenario==6){altered.sea.pop_back();} if(scenario==7){observer=0;} if(scenario==8){observer=2;}
			if(scenario==9){altered.units[0].owner=8;} if(scenario==10){altered.units[0].coordinate={0,1};}
			GT_ASSERT( Rejects([&]{Project(value,altered,observer);}) && value.Serialize()==bytes, "invalid live binding admitted or changed knowledge" );
		}
		auto reverse=input; std::reverse(reverse.units.begin(),reverse.units.end());
		GT_ASSERT( Project(value,input)==Project(value,reverse), "live Unit input order replaced knowledge order" );
		reverse.bases[0].selected_radius=1u<<31;
		GT_ASSERT( Rejects([&]{Project(value,reverse);},"selected32"), "projection silently truncated Base authority" ); GT_OK();
	} );
	task->AddTest( "SMAC worker knowledge projects only the requested observer across narrow aliases", GT() {
		Frame::Data frame{2,5,7,true,true,false,128,{},{},{{7,{0,2},0}}, {}};
		frame.tiles.resize(5,{false,128,{}}); frame.tiles[2].claimant=0;
		frame.observer_relations[0]={true,false}; frame.observer_relations[1]={false,true};
		auto input=Split(frame); input.knowledge.observers[1]=Knowledge::Observer{true,{}};
		input.knowledge.observers[1]->relations[0]={false,true};
		const auto owned=Knowledge::Deserialize(Knowledge::Create(input.knowledge).Serialize());
		const auto projected=Project(owned,input,7); const auto result=projected.Evaluate(0);
		GT_ASSERT( projected==Frame::Create(frame) && result.squares[0]==result.squares[2] && result.squares[0]==result.squares[6]
			&& !result.squares[18] && result.squares[0]->claimant==0, "requested observer, single-wrap aliases or native claims changed" );
		const auto other=Project(owned,input,1).CopyData();
		GT_ASSERT( other.foreign_disclosure && !other.observer_relations[0].foreign_unit_exemption
			&& other.observer_relations[0].territory_recognition && !other.observer_relations[1].territory_recognition,
			"stored observer rows were merged or inferred" ); GT_OK();
	} );
	task->AddTest( "SMAC worker knowledge rejects malformed bounded portable frames", GT() {
		const auto input=Small(); const auto value=Knowledge::Create(input.knowledge); const auto saved=value.Serialize();
		const size_t unit=52+input.knowledge.tiles.size()*2;
		for(int scenario=0;scenario<22;++scenario){
			auto wire=saved;
			if(scenario==0){wire[0]='X';} if(scenario==1){Write(wire,4,1);} if(scenario==2){Write(wire,8,0);}
			if(scenario==3){Write(wire,8,3);} if(scenario==4){Write(wire,8,0xffffffffu);} if(scenario==5){Write(wire,12,0);}
			if(scenario==6){wire[16]=2;} if(scenario==7){wire[17]=0;} if(scenario==8){wire[19]=0;}
			if(scenario==9){wire[19]=1;} if(scenario==10){wire[28]=1;} if(scenario==11){wire[31]=2;}
			if(scenario==12){Write(wire,20,0xffffffffu);} if(scenario==13){Write(wire,24,0xffffffffu);}
			if(scenario==14){wire[53]=8;} if(scenario==15){Write(wire,unit,0,8);} if(scenario==16){Write(wire,unit,uint64_t(1)<<63,8);}
			if(scenario==17){Write(wire,unit+10,input.knowledge.units[0].id,8);} if(scenario==18){wire[unit+9]=2;}
			if(scenario==19){wire.pop_back();} if(scenario==20){wire+='x';} if(scenario==21){wire.resize(51);}
			GT_ASSERT( Rejects([&]{Knowledge::Deserialize(wire);}) && value.Serialize()==saved, "malformed knowledge frame admitted" );
		}
		for(int scenario=0;scenario<7;++scenario){
			auto bad=input.knowledge;
			if(scenario==0){bad.observers={};} if(scenario==1){bad.tiles.back().claimant=-1;} if(scenario==2){bad.units[0].id=0;}
			if(scenario==3){bad.units[1].id=bad.units[0].id;} if(scenario==4){bad.ordinary_local_mode=false;}
			if(scenario==5){bad.tiles.pop_back();} if(scenario==6){bad.height=257;}
			GT_ASSERT( Rejects([&]{Knowledge::Create(bad);}), "invalid owned knowledge admitted" );
		} GT_OK();
	} );
	task->AddTest( "SMAC worker knowledge validates exact aggregate cap before projection allocation", GT() {
		Knowledge::Data data{2,5,true,true,0,{},{},{}}; data.observers[1]=Knowledge::Observer{false,{}}; data.tiles.resize(5,{1,0});
		const auto count=(Knowledge::MAX_WIRE_BYTES-53-10)/10;
		for(size_t i=0;i<count;++i){data.units.push_back({uint64_t(i+1),255,false});}
		const auto owned=Knowledge::Create(data); const auto wire=owned.Serialize();
		GT_ASSERT( wire.size()==53+10+count*10 && Knowledge::Deserialize(wire)==owned, "exact knowledge wire cap rejected" );
		GT_ASSERT( Rejects([&]{Knowledge::Deserialize(std::string(Knowledge::MAX_WIRE_BYTES+1,'x'));},"length"), "cap plus one copied into decoder" );
		data.units.push_back({uint64_t(count+1),0,false});
		GT_ASSERT( Rejects([&]{Knowledge::Create(data);},"budget"), "owned aggregate overflow admitted" );
		std::vector<Knowledge::LiveUnit> units(count,{1,1,{0,0}});
		GT_ASSERT( Rejects([&]{owned.Project(1,std::vector<bool>(5,false),{},units);},"budget"), "larger projected frame allocated before budget admission" ); GT_OK();
	} );
}
}
