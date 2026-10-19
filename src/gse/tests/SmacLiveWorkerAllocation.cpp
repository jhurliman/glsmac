#include "SmacLiveWorkerAllocation.h"
#include "game/ScoutBattlePresentation.h"
#include "ConventionalUnits.h"
#include "game/backend/map/SmacClimate.h"
#include "fixtures/SmacStartupSource.h"
#include "fixtures/SmacRestartSearch.h"
#include "fixtures/SmacCaptureRestart.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/WorldSnapshotRules.h"
#include "game/backend/WorldSnapshotRandom.h"
#include "game/backend/WorldSnapshotTurn.h"
#include "game/backend/WorldSnapshotWorkerKnowledge.h"
#include "game/backend/WorldSnapshotTerritory.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/SmacBaseUpkeepCache.h"
#include "game/backend/base/SmacNutrientDisplay.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/faction/Faction.h"
#include "game/backend/faction/SmacRankingScore.h"
#include "game/backend/map/Map.h"
#include "game/backend/map/tile/Tiles.h"
#include "game/backend/resource/Resource.h"
#include "game/backend/resource/ResourceManager.h"
#include "game/backend/slot/Slots.h"
#include "game/backend/unit/Unit.h"
#include "game/backend/unit/UnitManager.h"
#include "game/backend/unit/StaticDef.h"
#include "game/backend/unit/MoraleSet.h"
#include "game/backend/unit/CvrScoutRender.h"
#include "game/backend/unit/FormerMarkerRender.h"
#include "game/backend/unit/ColonyMarkerRender.h"
#include "game/backend/unit/SpriteRender.h"
#include "game/backend/base/SmacEcologyNativeSpawn.h"
#include "game/FrontendRequest.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/Null.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include "types/Buffer.h"
#include "ui/BaseResourcePresentation.h"
#include <sstream>
#include <set>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
using namespace game::backend::map::tile;
using Knowledge = map::SmacWorkerKnowledge;

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }

// Base definition registries are unordered. Normalize only their named entries;
// retain exact definitions, Base record order and all remaining world bytes.
std::string ResearchSnapshot(Game* game,const std::string& bytes) {
    types::Buffer world(bytes);const auto& settings=game->GetState()->m_settings.global;
    world_snapshot::ValidateHeader(world,settings);
    (void)world_snapshot::ReadRandomState(world);
    (void)world_snapshot::ReadTurnState(world,game->GetState()->m_slots->GetSlots());
    if(world.ReadBool()) {(void)world.ReadString();}
    (void)map::GlsmacResourceMapPolicy::ReadOptional(world,true);
    (void)world_snapshot::ReadWorkerKnowledge(world,true);
    (void)world_snapshot::ReadTerritoryTopology(world,true);
    for(int i=0;i<3;++i) {(void)world.ReadString();} // map, resources, units
    const auto prefix=bytes.substr(0,world.lenr);types::Buffer bases(world.ReadString()),canonical;
    canonical.WriteString(bases.ReadString());
    for(int table=0;table<2;++table) {
        const auto count=bases.ReadInt();
        if(count<0 || count>1024) {throw std::runtime_error("Invalid research fixture definition count");}
        std::map<std::string,std::string> entries;
        for(int64_t i=0;i<count;++i) {
            std::string name,wire;
            if(table==0) {name=bases.ReadString();wire=bases.ReadString();}
            else {wire=bases.ReadString();types::Buffer definition(wire);name=definition.ReadString();}
            if(!entries.emplace(name,wire).second) {throw std::runtime_error("Duplicate research fixture definition");}
        }
        canonical.WriteInt(count);
        for(const auto& entry:entries) {if(table==0) {canonical.WriteString(entry.first);}canonical.WriteString(entry.second);}
    }
    // Preserve all Base bodies and the next ID without reinterpreting them.
    types::Buffer framed;framed.WriteString(canonical.ToString()+bases.ToString().substr(bases.lenr));
    return prefix+framed.ToString()+bytes.substr(world.lenr);
}

// Independent typed fixture: deliberately retained history, not a query of any
// current world. Marking order is unrelated to numeric Unit identity.
struct UpkeepCacheWire {
    int64_t version = 3;
    std::optional<int64_t> ecology;
    std::optional<std::array<int64_t,28>> citizens;
    bool present = true;
    std::array<int64_t,13> calculation{4,3,2, 4,2,2,19,19, 2,3,2,1,1};
    std::array<int64_t,5> support{1,2,2,1,2};
    int64_t count = 4;
    std::vector<std::pair<int64_t,int64_t>> units{{93,0},{7,3},{51,1},{4,2}};
    types::Buffer Encode() const {
        types::Buffer out; out.WriteInt(version); out.WriteBool(present);
        if (present) {
            for (const auto value : calculation) { out.WriteInt(value); }
            if(version>=2) {out.WriteBool(ecology.has_value());if(ecology) {out.WriteInt(*ecology);}}
            if(version>=3) {out.WriteBool(citizens.has_value());if(citizens) {for(const auto value:*citizens) {out.WriteInt(value);}}}
            for (const auto value : support) { out.WriteInt(value); }
            out.WriteInt(count);
            for (const auto& unit : units) { out.WriteInt(unit.first); out.WriteInt(unit.second); }
        }
        return out;
    }
};
bool CacheError(types::Buffer bytes, const std::string& message) {
    try { (void)DeserializeSmacBaseUpkeepCache(bytes); }
    catch (const std::runtime_error& error) { return std::string(error.what()).find(message) != std::string::npos; }
    return false; // bad_alloc is not accepted as a count/bounds rejection
}

Value* Call( GSE_CALLABLE, value::Object* object, const std::string& name, const value::function_arguments_t& args = {} ) {
	return static_cast< value::Callable* >( object->Get( name ) )->Run( GSE_CALL, args );
}
std::string Source( const std::string& filename ) {
	auto source = smac_startup_fixture::OrdinarySource( filename );
	if ( filename != "alpha.txt" ) { return source; }
	const auto replace = [ & ]( const std::string& name, const std::string& text ) {
		const auto begin = source.find( name ) + name.size(); const auto end = source.find( '#', begin ); source.replace( begin, end - begin, text );
	};
	replace( "#RESOURCEINFO\n", "Ocean,1,0,0\nCenter,2,1,1\nBonus,2,2,2\nForest,1,2,1\nRecycling,1,1,1\nLand,1,0,0\nSea,1,0,0\nMonolith,2,2,2\nBorehole,0,6,6\n" );
	std::string citizens;
	for ( int i = 0; i < 7; ++i ) { citizens += "Same name,Same plurals," + std::string( i < 2 ? "None" : "Disable" ) + ",Disable,"
		+ ( i == 0 ? "3,0,0" : i == 1 ? "0,2,0" : "0,0,0" ) + ",opaque\n"; }
	replace( "#CITIZENS\n", citizens + "A,As,\nB,Bs,\nC,Cs,\n" );
	std::istringstream input( source ); std::string result, line, section; size_t row = 0;
	while ( std::getline( input, line ) ) {
		if ( !line.empty() && line.front() == '#' ) { section = line; row = 0; }
		else {
			if ( section == "#RULES" && ( row == 1 || row == 10 ) ) { line.replace( 0, line.find( ',' ), row == 1 ? "2" : "5" ); }
			++row;
		}
		result += line + '\n';
	}
	return result;
}
std::string UpkeepSource( const std::string& filename ) {
	auto source = Source( filename ); if ( filename != "alpha.txt" ) { return source; }
	std::istringstream input( source ); std::string line, section, output; size_t row = 0;
	while ( std::getline( input, line ) ) {
		if ( !line.empty() && line[ 0 ] == '#' ) { section = line; row = 0; }
		else { ++row; if ( section == "#RULES" && ( row == 6 || row == 13 || row == 14 ) ) {
			line.replace( 0, line.find( ',' ), row == 6 ? "10" : row == 13 ? "7" : "14" );
		} }
		output += line + '\n';
	}
	return output;
}
std::string ColonyChoiceSource( const std::string& filename ) {
	auto source = UpkeepSource( filename ); if ( filename != "alpha.txt" ) { return source; }
	// The generic source deliberately uses multiplier 123 and exemption INT_MIN.
	// Literal Colony receipts require the installed multiplier 10 and exemption 10;
	// the existing owned Infantry/Colonist/Scout-armor design then costs 3 * 10.
	std::istringstream input( source ); std::string line, section, output; size_t row = 0;
	while ( std::getline( input, line ) ) {
		if ( !line.empty() && line[ 0 ] == '#' ) { section = line; row = 0; }
		else { ++row; if ( section == "#RULES" && ( row == 7 || row == 45 ) ) { line.replace( 0, line.find( ',' ), "10" ); } }
		output += line + '\n';
	}
	return output;
}
Base::SmacAllocationRoles Roles() {
	Base::SmacAllocationRoles result{ "live-worker", {} };
	for ( size_t i = 0; i < 7; ++i ) { result.specialists[ i ] = "live-role-" + std::to_string( i ); } return result;
}
std::string RestartSource(const std::string& filename) {
    auto source=ColonyChoiceSource(filename);
    if(filename=="alpha.txt") {
        auto at=source.find("#RULES\n")+7;
        for(int row=1;row<21;++row) {at=source.find('\n',at)+1;}
        source.replace(at,source.find(',',at)-at,"10");
    }
    return source;
}
std::string TanksSource(const std::string& filename) {
    auto source=RestartSource(filename);if(filename!="alpha.txt") {return source;}
    std::istringstream input(source);std::string line,section,output;size_t row=0;
    while(std::getline(input,line)) {
        if(!line.empty() && line.front()=='#') {section=line;row=0;}
        else {++row;
            if(section=="#RULES" && (row==43 || row==44)) {line.replace(0,line.find(','),row==43 ? "50" : "2");}
            if(section=="#FACILITIES" && row==1) {line="HQ,5,0,None,Disable,Efficiency";}
            if(section=="#FACILITIES" && row==3) {line="Tanks,4,0,Tech6,Disable,Resources";}
            // Controlled rule inputs yield the receipt's first N/M intake4/4;
            // completion still uses the actual native allocator and processors.
            if(section=="#RESOURCEINFO" && row==2) {line="Center,2,2,1";}
            if(section=="#RESOURCEINFO" && row==4) {line="Forest,2,2,2";}
        }
        output+=line+'\n';
    }
    return output;
}
std::string HabitationSource(const std::string& filename) {
    auto source=TanksSource(filename);if(filename!="alpha.txt") {return source;}
    std::istringstream input(source);std::string line,section,output;size_t row=0;
    while(std::getline(input,line)) {
        if(!line.empty() && line.front()=='#') {section=line;row=0;}
        else {++row;if(section=="#FACILITIES") {
            if(row==24) {line="Hab Complex,8,2,Tech6,Disable,Population";}
            if(row==25) {line="Habitation Dome,16,4,Tech8,Disable,Population";}
        }}
        output+=line+'\n';
    }return output;
}
std::string HabitationGrowthSource(const std::string& filename,int32_t population) {
    auto source=HabitationSource(filename);if(filename!="alpha.txt") {return source;}
    const auto at=source.find("Center,2,2,1");
    source.replace(at,std::string("Center,2,2,1").size(),"Center,"+std::to_string(2*population+4)+",2,1");return source;
}
std::string EnergyFacilitySource(const std::string& filename) {
    auto source=TanksSource(filename);if(filename!="alpha.txt") {return source;}
    std::istringstream input(source);std::string line,section,output;size_t row=0;
    while(std::getline(input,line)) {
        if(!line.empty() && line.front()=='#') {section=line;row=0;}
        else {++row;if(section=="#FACILITIES") {
            if(row==7) {line="Energy Bank,8,1,Tech6,Disable,Economy";}
            if(row==8) {line="Network Node,8,1,Tech6,Disable,Labs";}
        }}
        output+=line+'\n';
    }return output;
}
ProductionState Production( ProductionItemKind kind = ProductionItemKind::Unit ) {
	const ProductionItem item{ kind, kind == ProductionItemKind::Stockpile ? "" : "opaque-target" };
	return { item, 17, { ProductionItemKind::Stockpile, "" }, 5 };
}
std::string ManualSpecialistSource(const std::string& filename) {
    auto source=TanksSource(filename);if(filename!="alpha.txt") {return source;}
    const auto begin=source.find("#CITIZENS\n")+std::string("#CITIZENS\n").size();const auto end=source.find('#',begin);
    source.replace(begin,end-begin,"Technician,Technicians,Disable,Disable,3,0,0,opaque\nDoctor,Doctors,None,Disable,0,2,0,opaque\nLibrarian,Librarians,Disable,Disable,0,0,3,opaque\nEngineer,Engineers,None,Disable,3,0,2,opaque\nEmpath,Empaths,Disable,Disable,0,2,2,opaque\nThinker,Thinkers,Disable,Disable,0,1,3,opaque\nTranscend,Transcends,Disable,Disable,2,2,4,opaque\nA,As,\nB,Bs,\nC,Cs,\n");
    std::istringstream input(source);std::string line,section,output;size_t row=0;
    while(std::getline(input,line)) {if(!line.empty() && line.front()=='#') {section=line;row=0;}else {++row;if(section=="#RULES" && row==11) {line.replace(0,line.find(','),"5");}}output+=line+'\n';}return output;
}
std::string MaintenanceSource(const std::string& filename) {
    auto source=EnergyFacilitySource(filename);if(filename!="alpha.txt") {return source;}
    const auto at=source.find("Forest,2,2,2");source.replace(at,std::string("Forest,2,2,2").size(),"Forest,2,2,1");return source;
}
std::string BattleSource(const std::string& filename) {
    auto source=TanksSource(filename);
    if(filename=="gaians.txt") {const std::string bonus=",SOCIAL,-MORALE";const auto at=source.find(bonus);source.erase(at,bonus.size());}
    return source;
}
std::string FanaticBattleSource(const std::string& filename) {
    auto source=BattleSource(filename);
    if(filename=="gaians.txt") {const auto at=source.find("\n#END");source.insert(at,std::string(source.at(at-1)==',' ? "" : ",")+"FANATIC,1");}
    return source;
}
Base* Populate( GSE_CALLABLE, ConventionalUnitFixture& fixture, bool doctor, bool homed = false, size_t owner_slot = 0, int32_t simulation_id = 1, size_t population = 1, size_t center_x=2,size_t center_y=2 ) {
	auto* game = fixture.Game(); auto* bm = game->GetBM(); const auto roles = Roles();
	for ( const auto* id : { "NUTRIENTS", "MINERALS", "ENERGY" } ) { game->GetRM()->DefineResource( new game::backend::resource::Resource( id, id, {} ) ); }
	bm->DefinePop( new PopDef( roles.worker, "Display only", {}, {}, PopDef::PF_TILE_WORKER ) );
	for ( uint8_t i = 0; i < 7; ++i ) { bm->DefinePop( new PopDef( roles.specialists[ i ], "Display only", {}, {}, PopDef::PF_NONE, i ) ); }
	for ( size_t y = 0; y < game->GetMap()->GetHeight(); ++y ) { for ( size_t x = y & 1; x < game->GetMap()->GetWidth(); x += 2 ) {
		auto* tile = game->GetMap()->GetTile( x, y );
		for ( auto* corner : tile->elevation.corners ) { *corner = 0; }
		tile->Update(); tile->moisture = MOISTURE_ARID; tile->rockiness = ROCKINESS_FLAT; tile->features = 0; tile->terraforming = 0; tile->bonus = BONUS_NONE;
	} }
	game->GetMap()->GetTile( 4, 2 )->terraforming = TERRAFORMING_FOREST;
	auto* owner = &game->GetState()->m_slots->GetSlot( owner_slot );
	Base::pops_t pops;
	for ( size_t i = 0; i < population; ++i ) { pops.emplace( std::piecewise_construct, std::forward_as_tuple( 19 + i ), std::forward_as_tuple( nullptr, 19 + i, bm->GetPopDef( roles.specialists[ 1 ] ), 1 ) ); }
	auto* base = new Base( game, 701, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( center_x, center_y ), "Live allocation", {},
		pops, 41, 13, Production(), SmacBaseResourceContext{ simulation_id, false, 0, 0, 0x40 } );
	bm->SpawnBase( GSE_CALL, base );
	SmacBaseAllocationState allocation{ doctor ? 1u : population == 2 ? 7u : 3u, { {}, doctor ? static_cast< int32_t >( population ) : 0 } };
	for ( size_t i = 0; i < 16; ++i ) { allocation.specialists.types[ i ] = i ? ( i + 2 ) % 7 : 1; }
	base->PublishSmacAllocation( GSE_CALL, allocation, roles );
	// An actual conventional Unit and optional saved home, not an injected support total.
	using namespace game::backend::unit;
	MoraleSet::morale_values_t names; for ( int i = 0; i < 7; ++i ) { names.emplace_back( "level" + std::to_string( i ) ); }
	auto* um = game->GetUM(); um->DefineMoraleSet( new MoraleSet( "live-morale", names ) );
	um->DefineUnit( new StaticDef( "live-scout", um->GetMoraleSet( "live-morale" ), "Scout", MT_LAND, 999,
		new CvrScoutRender( 2, 0, 100, 100, { 1, 1 }, { 128, 128 } ) ) );
	ConventionalUnitState record{ 2, simulation_id, 1, 0, 0, false, ConventionalUnitState::Home::None, SmacMoraleMode::Mode0 };
	if ( homed ) { record.home = ConventionalUnitState::Home::Base; record.home_base_id = base->m_id; }
	um->SpawnConventionalUnit( GSE_CALL, 91, um->GetUnitDef( "live-scout" ), owner, game->GetMap()->GetTile( 6, 6 ), record );
	fixture.PrepareBaseCallbacks();
	auto* setup = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_base_growth" ) );
	setup->Run( GSE_CALL, { game->Wrap( GSE_CALL ) } );
	return base;
}
std::string FloodSource(const std::string& filename) {
    const auto source=UpkeepSource(filename);if(filename!="alpha.txt") {return source;}
    std::istringstream input(source);std::string line,section,output;size_t row=0;
    while(std::getline(input,line)) {
        if(!line.empty() && line.front()=='#') {section=line;row=0;}
        else if(++row==26 && section=="#FACILITIES") {
            const auto first=line.find(','),second=line.find(',',first+1),third=line.find(',',second+1);
            line.replace(second+1,third-second-1,"0");
        }
        output+=line+'\n';
    }return output;
}
Knowledge KnowledgeValue( bool old_available = false, bool candidate_available = true, int32_t width = 8, int32_t height = 8 ) {
	Knowledge::Data data{ width, height, true, true, 0, {}, {}, { { 91, 0, false } } };
	data.observers[ 1 ] = Knowledge::Observer{ false, {} }; data.observers[ 1 ]->pact_with = 0; data.tiles.resize( width * height / 2, { 0, {} } );
	for ( const auto xy : { std::array< int, 2 >{ 2, 2 }, { 3, 1 }, { 4, 2 } } ) {
		auto& tile = data.tiles[ xy[ 1 ] * ( width / 2 ) + xy[ 0 ] / 2 ]; tile.claimant = 1;
		tile.explored_by = xy[ 0 ] == 3 ? old_available ? 2 : 0 : xy[ 0 ] == 4 ? candidate_available ? 2 : 0 : 2;
	}
	return Knowledge::Create( data );
}
void RecalculateViaWrapper( GSE_CALLABLE, Base* base ) {
	value::array_elements_t specialists; for ( const auto& role : Roles().specialists ) { specialists.push_back( VALUE( value::String,, role ) ); }
	Call( GSE_CALL, static_cast< value::Object* >( base->Wrap( GSE_CALL ) ), "recalculate_smac_workers",
		{ VALUE( value::String,, Roles().worker ), VALUE( value::Array,, specialists ) } );
}
std::string SelectionSource( const std::string& filename ) {
	auto source = UpkeepSource( filename ); if ( filename != "alpha.txt" ) { return source; }
	std::istringstream input( source ); std::string line, section, output; size_t row = 0;
	while ( std::getline( input, line ) ) {
		if ( !line.empty() && line[ 0 ] == '#' ) { section = line; row = 0; }
		else { ++row; if ( section == "#RULES" && row >= 43 && row <= 45 ) {
			line.replace( 0, line.find( ',' ), row == 43 ? "50" : row == 44 ? "2" : "10" );
		} }
		output += line + '\n';
	} return output;
}
void SelectionPresentations( ConventionalUnitFixture& fixture ) {
	using namespace game::backend::unit;
	auto* um = fixture.Game()->GetUM(); auto* morale = um->GetMoraleSet( "live-morale" );
	um->DefineUnit( new StaticDef( "choice-scout", morale, "Scout", MT_LAND, 0,
		new CvrScoutRender( 2, 0, 100, 100, { 1, 1 }, { 128, 128 }, CvrScoutRender::Composition::InitializedScout ) ) );
	um->DefineUnit( new StaticDef( "choice-rover", morale, "Rover", MT_LAND, 0,
		new CvrScoutRender( 2, 0, 100, 100, { 1, 1 }, { 128, 128 }, CvrScoutRender::Composition::SpartanRover ) ) );
	um->DefineUnit( new StaticDef( "choice-former", morale, "Former", MT_LAND, 0, new FormerMarkerRender() ) );
	um->DefineUnit( new StaticDef( "choice-colony", morale, "Colony", MT_LAND, 0, new ColonyMarkerRender() ) );
	fixture.ProductionPresentations( { "choice-scout", "choice-rover", "choice-former", "choice-colony" } );
}
}

struct NativeCoexistenceCanary {
    size_t id;std::string wire,factory;
    bool Matches(ConventionalUnitFixture& fixture) const {
        const auto* body=fixture.Game()->GetUM()->GetUnit(id);std::string current;
        fixture.ResourceState([&](const auto& state){current=state.native_factory->Serialize().ToString();});
        return body && unit::Unit::Serialize(body).ToString()==wire && current==factory;
    }
};
NativeCoexistenceCanary PublishUnrelatedNative(GSE_CALLABLE,ConventionalUnitFixture& fixture) {
    using namespace game::backend::unit;
    auto* game=fixture.Game();auto* um=game->GetUM();
    um->DefineUnit(new StaticDef("MindWorms",um->GetMoraleSet("live-morale"),"Mind Worms",MT_LAND,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
    fixture.ResourceState([](auto& state){if(!state.native_factory) {state.native_factory=faction::SmacNativeFactoryState::Initialize(0,state.gameplay_random);}});
    const auto plan=PrepareSmacNativeSpawn({{{20,2},128,64,true},1,7,9,1},{{{20,2},1}},[](int32_t,int32_t){return SmacNativeSpawnTile{3,false,1,1,false,15};});
    auto birth=um->PrepareEcologyNativeBirths(GSE_CALL,plan);if(birth.Size()!=1) {throw std::runtime_error("Coexistence fixture did not create native");}
    const auto id=birth.Candidate(0).m_id;birth.Commit();birth.Notify(GSE_CALL);
    NativeCoexistenceCanary result{id,Unit::Serialize(um->GetUnit(id)).ToString(),{}};
    fixture.ResourceState([&](const auto& state){result.factory=state.native_factory->Serialize().ToString();});return result;
}

void AddSmacLiveWorkerAllocationTests( task::gsetests::GSETests* task ) {
    task->AddTest("SMAC inspector opening queues fresh resources before selection and keeps the final requested Base",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(UpkeepSource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* bm=game->GetBM();auto* base=Populate(GSE_CALL,fixture,true,false,0,1,2);
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(names);
            const auto stock=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
            const ProductionState production{stock,17,stock,5,std::vector<ProductionItem>{}};base->SetProductionState(GSE_CALL,production);
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            Base::pops_t peer_pops;peer_pops.emplace(1,Pop(nullptr,1,bm->GetPopDef(Roles().specialists[1]),0));
            SmacBaseAllocationState peer_allocation{1,{{},1}};peer_allocation.specialists.types[0]=1;
            auto* peer=new Base(game,702,base->m_owner,base->m_faction,game->GetMap()->GetTile(40,40),"Other Base",{},peer_pops,2,13,production,base->GetSmacResourceContext(),peer_allocation);
            bm->SpawnBase(GSE_CALL,peer);fixture.Updates();const auto before=fixture.Snapshot();
            GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"open_smac_base_inspector",{{"base",base->Wrap(GSE_CALL)},{"extra",VALUE(value::Int,,1)}});})
                && fixture.Snapshot()==before && fixture.Updates().empty(),"Malformed inspector request changed state or selection");
            const auto turn=game->GetTurnId();const auto random=fixture.GameplayRandom();const auto credits=game->GetCanonicalEconomy(base->m_owner).credits;
            const auto select=[&](Base* target){Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"select_base",{target->Wrap(GSE_CALL)});};
            select(base);select(peer);
            GT_ASSERT(fixture.Snapshot()==before && fixture.Updates().empty() && !base->GetSmacUpkeepCache() && !peer->GetSmacUpkeepCache(),
                "Inspector opening mutated or selected a Base before its queued owner callback");
            fixture.DrainEvents();std::set<size_t> updated;size_t selected_id=0,select_count=0;
            for(const auto& fr:fixture.Updates()) {
                if(fr.type==game::FrontendRequest::FR_BASE_UPDATE) {updated.insert(fr.data.base_update.base_id);}
                if(fr.type==game::FrontendRequest::FR_BASE_SELECT) {
                    selected_id=fr.data.base_select.base_id;++select_count;GT_ASSERT(updated.count(selected_id),"Inspector selection preceded current Base data");
                }
            }
            GT_ASSERT(select_count==1 && selected_id==702,"Queued inspector opens selected by Base hash order instead of the final request");
            GT_ASSERT(base->GetSmacUpkeepCache() && peer->GetSmacUpkeepCache() && fixture.GameplayRandom()==random && game->GetTurnId()==turn
                && base->GetAccumulatedNutrients()==13 && peer->GetAccumulatedNutrients()==13 && base->GetProductionState()==std::optional<ProductionState>{production}
                && peer->GetProductionState()==std::optional<ProductionState>{production} && game->GetCanonicalEconomy(base->m_owner).credits==credits,
                "Opening inspector omitted fresh cache or posted food, production, credits, RNG or a turn");GT_OK();
        });
    });

    task->AddTest("SMAC flood rejects incomplete native Base authority before RNG or retained population",[](GSE*,context::Context*)->std::string {
        for(const bool allocation:{false,true}) {for(const bool production:{false,true}) {
            if(allocation && production) {continue;}
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(FloodSource,"GAIANS");
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* bm=game->GetBM();auto* control=Populate(GSE_CALL,fixture,true);
                game->GetState()->m_settings.global.SelectSmacCampaignRules(false);
                auto context=control->GetSmacResourceContext();context.retained_status_word=0;
                bm->DefineFacility(new FacilityDef("flood-dome","Pressure Dome","",80,{},26));
                Base::pops_t pops;for(size_t i=0;i<12;++i) {pops.emplace(std::piecewise_construct,std::forward_as_tuple(100+i),std::forward_as_tuple(nullptr,100+i,bm->GetPopDef(Roles().specialists[1]),1));}
                SmacBaseAllocationState workers{1,{{},12}};workers.specialists.types.fill(1);
                auto* base=new Base(game,702,control->m_owner,control->m_faction,game->GetMap()->GetTile(20,16),"Incomplete flood",{},pops,112,7,
                    production ? std::optional<ProductionState>(Production()):std::nullopt,context,
                    allocation ? std::optional<SmacBaseAllocationState>(workers):std::nullopt);
                bm->SpawnBase(GSE_CALL,base);
                fixture.ResourceState([](auto& state){state.retained_base_totals=faction::SmacBaseTotals{};state.retained_base_totals->at(1)=9;state.faction_status.values[1]=0x1000u;});
                const auto before=fixture.Snapshot();const auto original=fixture.GameplayRandom();auto random=original;map::smac_climate::Streams streams{random};
                auto plan=bm->PrepareSmacFloods(GSE_CALL);
                GT_ASSERT(Rejects([&]{plan.Flood(GSE_CALL,*base,3,2,streams);}) && random==original && streams.crt_draws==0 && streams.shared_draws==0,
                    "Incomplete flood consumed RNG or accepted an unsaveable retained population");
                plan.CommitSurvivors(GSE_CALL);
                GT_ASSERT(fixture.Snapshot()==before && base->GetPopulation()==12 && !base->m_facilities.count("flood-dome"),
                    "Rejected flood left staged Base or owner changes");GT_OK();
            });if(!error.empty()) {return error;}
        }}GT_OK();
    });
    task->AddTest("SMAC owning flood preparation preserves allocation and stages Dome damage before publication",[](GSE*,context::Context*)->std::string {
        using Floods=BaseManager::PreparedSmacFloods;
        std::string flooded_save;
        // Damage, deletion, last-Base/objective protection, existing Dome,
        // sea-to-land, discarded candidate and changed-preimage rejection.
        for(int scenario=0;scenario<8;++scenario) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(FloodSource,"GAIANS");
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* bm=game->GetBM();auto* base=Populate(GSE_CALL,fixture,true,false,0,1,scenario==0 || scenario>=6 ? 12:1);
                game->GetState()->m_settings.global.SelectSmacCampaignRules(false);
                auto context=base->GetSmacResourceContext();context.retained_status_word=scenario==3 ? 0x8000u:0u;base->SetSmacResourceContext(GSE_CALL,context);
                bm->DefineFacility(new FacilityDef("flood-dome","Pressure Dome","",80,{},26));
                if(scenario==4) {base->m_facilities.emplace("flood-dome",bm->GetFacilityDef("flood-dome"));}
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));base->RecalculateSmacWorkers(GSE_CALL,Roles());
                if(base->GetPopulation()==12) {auto allocation=*base->GetSmacAllocation();allocation.selected=511;allocation.specialists.total_count=4;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());}
                fixture.ResourceState([&](auto& state){state.retained_base_totals=faction::SmacBaseTotals{};state.retained_base_totals->at(1)=scenario==2 ? 1:9;state.faction_status.values[1]=0x1000u;});
                Base* peer=nullptr;
                if(scenario==7) {
                    Base::pops_t pops;for(size_t i=0;i<12;++i) {pops.emplace(std::piecewise_construct,std::forward_as_tuple(100+i),std::forward_as_tuple(nullptr,100+i,bm->GetPopDef(Roles().specialists[1]),1));}
                    SmacBaseAllocationState peer_allocation{1,{{},12}};peer_allocation.specialists.types.fill(1);
                    peer=new Base(game,702,base->m_owner,base->m_faction,game->GetMap()->GetTile(20,16),"Flood peer",{},pops,112,7,Production(),context,peer_allocation);bm->SpawnBase(GSE_CALL,peer);
                }
                const auto before=fixture.Snapshot();const auto record=Base::Serialize(base).ToString();const auto allocation=base->GetSmacAllocation();
                const auto cache=SerializeSmacBaseUpkeepCache(base->GetSmacUpkeepCache()).ToString();const auto production=base->GetProductionState();
                const auto live_random=fixture.GameplayRandom();auto random=live_random;map::smac_climate::Streams streams{random};
                auto plan=bm->PrepareSmacFloods(GSE_CALL);
                const auto result=plan.Flood(GSE_CALL,*base,scenario==5 ? 2:3,scenario==5 ? 3:2,streams);
                const bool unchanged=scenario==4 || scenario==5;
                GT_ASSERT(streams.crt_draws==(unchanged ? 0u:1u) && streams.shared_draws==0,"Flood consumed the wrong random stream or draw count");
                GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==live_random,"Preparing flood published Base, owner or RNG changes");
                if(scenario==1) {
                    GT_ASSERT(result==Floods::Outcome::Remove && Rejects([&]{plan.CommitSurvivors(GSE_CALL);}) && fixture.Snapshot()==before,
                        "Flood deletion was normalized to survival or partially committed without ordinary removal");GT_OK();
                }
                GT_ASSERT(result==(unchanged ? Floods::Outcome::Unchanged:Floods::Outcome::Survived),"Wrong flood survival result");
                if(!unchanged) {const auto draws=streams.crt_draws;GT_ASSERT(plan.Flood(GSE_CALL,*base,3,2,streams)==Floods::Outcome::Unchanged && streams.crt_draws==draws,"Candidate Dome failed to suppress a repeated flood draw");}
                if(scenario==6) {GT_ASSERT(fixture.Snapshot()==before,"Discarded flood changed live state");GT_OK();}
                if(scenario==7) {
                    GT_ASSERT(plan.Flood(GSE_CALL,*peer,3,2,streams)==Floods::Outcome::Survived,"Peer flood was not staged");
                    peer->m_name="Changed peer";const auto edited=fixture.Snapshot();
                    GT_ASSERT(Rejects([&]{plan.CommitSurvivors(GSE_CALL);}) && fixture.Snapshot()==edited,"Changed flood preimage partially published");GT_OK();
                }
                plan.CommitSurvivors(GSE_CALL);
                GT_ASSERT(base->GetPopulation()==(scenario==0 ? 6u:1u) && base->m_pops.size()==(scenario==0 ? 12u:1u)
                    && base->GetSmacAllocation()==allocation && SerializeSmacBaseUpkeepCache(base->GetSmacUpkeepCache()).ToString()==cache
                    && base->GetAccumulatedNutrients()==13 && base->GetProductionState()==production && fixture.GameplayRandom()==live_random,
                    "Flood repaired allocation, posted stores/cache or published detached RNG");
                fixture.ResourceState([&](const auto& state){if(state.faction_status.values[1]!=(unchanged ? 0x1000u:0x81000u)) {throw std::runtime_error("Flood status did not retain unrelated bits");}});
                GT_ASSERT(base->m_facilities.count("flood-dome")==size_t(scenario!=5),"Flood Dome installation differs");
                if(unchanged) {GT_ASSERT(Base::Serialize(base).ToString()==record,"Bypassed flood changed Base bytes");}
                if(scenario==0) {flooded_save=fixture.Snapshot();}
                GT_ASSERT(Rejects([&]{plan.CommitSurvivors(GSE_CALL);}),"Flood plan published twice");GT_OK();
            });if(!error.empty()) {return "Flood scenario "+std::to_string(scenario)+": "+error;}
        }
        ConventionalUnitFixture restored;restored.ConfigureProduction(FloodSource,"GAIANS");
        std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}restored.BasePresentationRoles(names);
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.Game()->GetState()->m_settings.global.SelectSmacCampaignRules(false);
            restored.RestoreThroughStart(GSE_CALL,flooded_save,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
            GT_ASSERT(base && base->GetPopulation()==6 && base->m_pops.size()==12 && base->m_facilities.count("flood-dome"),"Actual Start lost produced flood state");
            Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"select_base",{base->Wrap(GSE_CALL)});restored.DrainEvents();
            GT_ASSERT(base->GetPopulation()==6 && base->m_pops.size()==6 && base->GetSmacUpkeepCache()->calculation.citizens
                && base->GetSmacUpkeepCache()->calculation.citizens->population==6 && base->GetAccumulatedNutrients()==13,
                "Produced flood state did not survive Start until actual inspector worker repair");GT_OK();
        });
    });

    task->AddTest("SMAC flooded retained population survives actual Start until queued inspector opening",[](GSE*,context::Context*)->std::string {
        const auto source_rules=[](const std::string& filename) {
            const auto source=UpkeepSource(filename);if(filename!="alpha.txt") {return source;}
            std::istringstream input(source);std::string line,section,output;size_t row=0;
            while(std::getline(input,line)) {
                if(!line.empty() && line.front()=='#') {section=line;row=0;}
                else if(++row==26 && section=="#FACILITIES") {
                    const auto first=line.find(','),second=line.find(',',first+1),third=line.find(',',second+1);
                    line.replace(second+1,third-second-1,"0"); // admitted ordinary Dome fee, not fixture's negative sentinel
                }
                output+=line+'\n';
            }return output;
        };
        const auto replace_frame=[](std::string whole,const std::string& before,const std::string& after) {
            types::Buffer old_frame,new_frame;old_frame.WriteString(before);new_frame.WriteString(after);
            const auto old=old_frame.ToString();const auto at=whole.find(old);
            if(at==std::string::npos || whole.find(old,at+1)!=std::string::npos) {throw std::runtime_error("Fixture expected one owned snapshot frame");}
            whole.replace(at,old.size(),new_frame.ToString());return whole;
        };
        ConventionalUnitFixture source;source.ConfigureProduction(source_rules,"GAIANS");std::string seeded,expected,repaired,old_cache;
        SmacBaseAllocationState allocation{};std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}
        source.BasePresentationRoles(names);
        const auto setup=source.Run([&](GSE_CALLABLE)->std::string {
            auto* game=source.Game();auto* bm=game->GetBM();auto* base=Populate(GSE_CALL,source,true,false,0,1,12,16,16);
            const auto stock=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
            base->SetProductionState(GSE_CALL,ProductionState{stock,17,stock,5,std::vector<ProductionItem>{}});
            bm->DefineFacility(new FacilityDef("flood-dome","Pressure Dome","",80,{},26));base->m_facilities.emplace("flood-dome",bm->GetFacilityDef("flood-dome"));
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            base->RecalculateSmacWorkers(GSE_CALL,Roles());
            allocation=*base->GetSmacAllocation();allocation.selected=511;allocation.specialists.total_count=4;
            base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
            old_cache=SerializeSmacBaseUpkeepCache(base->GetSmacUpkeepCache()).ToString();
            const auto old_record=Base::Serialize(base).ToString();types::Buffer record(old_record);
            const auto retained_prefix=record.ToString();record.WriteString("SMAC_RETAINED_POPULATION_V1");record.WriteInt(6);expected=record.ToString();
            const auto before=source.Snapshot();const auto next=Base::GetNextId();
            for(const int64_t invalid:{int64_t(-1),int64_t(0),int64_t(12),int64_t(128),INT64_MAX}) {
                types::Buffer bad(retained_prefix);bad.WriteString("SMAC_RETAINED_POPULATION_V1");bad.WriteInt(invalid);std::string error;
                try {std::unique_ptr<Base> decoded(Base::Deserialize(bad,game));}catch(const std::runtime_error& e){error=e.what();}
                GT_ASSERT(error.find("Invalid saved retained Base population")!=std::string::npos && source.Snapshot()==before && Base::GetNextId()==next,
                    "Malformed retained count reached construction or changed live world");
            }
            for(int malformed=0;malformed<3;++malformed) {
                types::Buffer bad(retained_prefix);bad.WriteString(malformed==0 ? "SMAC_RETAINED_POPULATION_V9":"SMAC_RETAINED_POPULATION_V1");
                if(malformed!=1) {bad.WriteInt(6);}if(malformed==2) {bad.WriteBool(false);}
                GT_ASSERT(Rejects([&]{std::unique_ptr<Base> decoded(Base::Deserialize(bad,game));}) && source.Snapshot()==before && Base::GetNextId()==next,
                    "Malformed retained extension changed the world or was admitted");
            }
            types::Buffer legacy(expected);std::string error;try {std::unique_ptr<Base> decoded(Base::Deserialize(legacy,game,false));}catch(const std::runtime_error& e){error=e.what();}
            GT_ASSERT(error.find("Unsupported retained Base population")!=std::string::npos && source.Snapshot()==before,"V11 reader admitted retained population extension");
            types::Buffer manager;bm->Serialize(manager);auto changed=replace_frame(manager.ToString(),old_record,expected);
            types::Buffer v11,v12;v11.WriteString("GLSMAC_BASES_V11");v12.WriteString("GLSMAC_BASES_V12");
            GT_ASSERT(changed.substr(0,v11.lenw)==v11.ToString(),"Ordinary saved Base manager did not preserve V11");
            changed.replace(0,v11.lenw,v12.ToString());seeded=replace_frame(before,manager.ToString(),changed);GT_OK();
        });if(!setup.empty()) {return setup;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(source_rules,"GAIANS");restored.BasePresentationRoles(names);
        const auto loaded=restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,seeded,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
            GT_ASSERT(base && base->GetPopulation()==6 && base->m_pops.size()==12 && Base::Serialize(base).ToString()==expected
                && base->GetSmacAllocation()==std::optional<SmacBaseAllocationState>{allocation}
                && SerializeSmacBaseUpkeepCache(base->GetSmacUpkeepCache()).ToString()==old_cache,
                "Actual Start normalized retained allocation/cache or lost reduced population");
            bool published=false;for(const auto& fr:restored.Updates()) {if(fr.type==game::FrontendRequest::FR_BASE_UPDATE && fr.data.base_update.base_id==701) {
                published=fr.data.base_update.has_population && fr.data.base_update.population==6 && fr.data.base_update.pops->size()==12;
            }}GT_ASSERT(published,"Frontend population was inferred from retained worker projection");
            const auto random=restored.GameplayRandom();const auto turn=game->GetTurnId();const auto credits=game->GetCanonicalEconomy(base->m_owner).credits;
            const auto food=base->GetAccumulatedNutrients();const auto production=base->GetProductionState();
            const auto preview=base->PrepareSmacResourceCalculation();
            const auto energy=game->PreviewCanonicalBaseEnergy(*base,game->GetCanonicalEconomy(base->m_owner).rates);
            GT_ASSERT(Base::Serialize(base).ToString()==expected && restored.GameplayRandom()==random,"Fresh flood preview mutated the retained Base or RNG");
            const auto tail=base->m_pops.rbegin()->first;
            GT_ASSERT(Rejects([&]{(void)base->GetSmacSpecialistMenu(tail);})
                && Rejects([&]{base->OpenSmacSpecialistSelection(GSE_CALL,tail,true);})
                && Base::Serialize(base).ToString()==expected && restored.Updates().empty(),"Retired-tail specialist request published repair or a menu");

            Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"select_base",{base->Wrap(GSE_CALL)});
            GT_ASSERT(Base::Serialize(base).ToString()==expected && restored.Updates().empty(),"Inspector selection mutated the read-only caller or opened before its owning calculation");
            restored.DrainEvents();
            bool updated=false,selected=false;for(const auto& fr:restored.Updates()) {
                if(fr.type==game::FrontendRequest::FR_BASE_UPDATE && fr.data.base_update.base_id==701) {
                    updated=fr.data.base_update.population==6 && fr.data.base_update.pops->size()==6;
                }
                if(fr.type==game::FrontendRequest::FR_BASE_SELECT && fr.data.base_select.base_id==701) {selected=updated;}
            }
            GT_ASSERT(selected && restored.GameplayRandom()==random && game->GetTurnId()==turn && base->GetAccumulatedNutrients()==food
                && base->GetProductionState()==production && game->GetCanonicalEconomy(base->m_owner).credits==credits,
                "Inspector opened before data publication or posted stores, credits, RNG or a turn");
            GT_ASSERT(preview.raw_intake==base->GetSmacUpkeepCache()->calculation.raw_intake
                && preview.nutrients.demand==base->GetSmacUpkeepCache()->calculation.nutrients.demand
                && preview.minerals.demand==base->GetSmacUpkeepCache()->calculation.minerals.demand,
                "Fresh preview used ghost worker intake or obsolete demand before actual repair");
            const auto repaired_energy=game->PreviewCanonicalBaseEnergy(*base,game->GetCanonicalEconomy(base->m_owner).rates);
            GT_ASSERT(energy.tail.final.economy==repaired_energy.tail.final.economy && energy.tail.final.psych==repaired_energy.tail.final.psych
                && energy.tail.final.labs==repaired_energy.tail.final.labs,"Energy preview before flood repair differs from the owning result");
            GT_ASSERT(base->GetSmacUpkeepCache() && base->GetSmacUpkeepCache()->calculation.citizens && base->GetSmacUpkeepCache()->calculation.citizens->population==6,"Inspector did not store the fresh resource calculation");
            GT_ASSERT(base->GetPopulation()==6 && base->m_pops.size()==6 && base->m_pops.begin()->first==19 && base->m_pops.rbegin()->first==24,
                "Later owning worker producer repaired to the obsolete population or changed survivor identities");
            types::Buffer manager;game->GetBM()->Serialize(manager);GT_ASSERT(manager.ReadString()=="GLSMAC_BASES_V11","Repaired coherent state retained the optional extension");
            repaired=restored.Snapshot();GT_OK();
        });if(!loaded.empty()) {return loaded;}
        ConventionalUnitFixture final;final.ConfigureProduction(source_rules,"GAIANS");final.BasePresentationRoles(names);
        return final.Run([&](GSE_CALLABLE)->std::string {
            final.RestoreThroughStart(GSE_CALL,repaired,[](GSE_CALLABLE){});const auto* base=final.Game()->GetBM()->GetBase(701);
            GT_ASSERT(base && base->GetPopulation()==6 && base->m_pops.size()==6,"Second actual Start lost the repaired population");GT_OK();
        });
    });

    task->AddTest("SMAC energy modal keeps live changes cancels by recalculation and fences retired requests",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");int64_t request=0;int closed=0;
        std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i){roles[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(roles);
        std::string seeded;const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);
            auto context=base->GetSmacResourceContext();context.governor_options=0;base->SetSmacResourceContext(GSE_CALL,context);
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            base->SetProductionState(GSE_CALL,ProductionState{scout,2,scout,2,std::vector<ProductionItem>{}});
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            seeded=fixture.Snapshot();GT_OK();
        });GT_ASSERT(setup.empty(),"Energy modal seed: "+setup);
        ConventionalUnitFixture restored;restored.ConfigureProduction(TanksSource,"GAIANS");restored.BasePresentationRoles(roles);
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,seeded,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);restored.BasePresentationRoles(roles);
            game->On(GSE_CALL,"smac_energy_rates_menu",NATIVE_CALL(&request,&closed) {
                N_EXPECT_ARGS(1);N_GET(view,0,Object);
                if(static_cast<value::Bool*>(view->Get("closed"))->value) {++closed;}
                else {request=static_cast<value::Int*>(view->Get("request_id"))->value;}
                return VALUE(value::Undefined);
            }));
            const auto open=[&] {
                auto* preview=static_cast<value::Object*>(game->CopySmacEnergyRatePreview(GSE_CALL,*base,{}));
                game->Event(GSE_CALL,"open_smac_energy_rates",{{"base",base->Wrap(GSE_CALL)},{"expected_state",preview->Get("expected_state")}});
            };
            const auto adjust=[&](int button){game->Event(GSE_CALL,"adjust_smac_energy_rates",{{"request_id",VALUE(value::Int,,request)},{"button",VALUE(value::Int,,button)},{"locks",VALUE(value::Int,,0)}});};
            const auto close=[&](int64_t id,bool cancel){game->Event(GSE_CALL,"close_smac_energy_rates",{{"request_id",VALUE(value::Int,,id)},{"cancel",VALUE(value::Bool,,cancel)}});};
            const auto money=game->GetCanonicalEconomy(base->m_owner);const auto random=restored.GameplayRandom();const auto production=base->GetProductionState();
            base->SetAccumulatedNutrients(-3);
            open();game->Event(GSE_CALL,"complete_turn",{});restored.DrainEvents();
            GT_ASSERT(request>0 && game->GetTurnId()==1 && base->GetAccumulatedNutrients()==-3,"Opening posted a pass or allowed already queued turn completion");
            adjust(34);adjust(34);restored.DrainEvents();auto live=game->GetCanonicalEconomy(base->m_owner);
            auto expected=money;expected.rates={2,3};
            GT_ASSERT(live==expected && base->GetAccumulatedNutrients()==0 && base->GetProductionState()==production && restored.GameplayRandom()==random,
                "Queued live buttons were lost, failed forced food normalization or posted stores/income/RNG");
            bool save_fenced=false;
            try {restored.SaveCampaign("/tmp/glsmac-energy-modal-fenced.glsmac");}
            catch(const std::runtime_error& error) {save_fenced=std::string(error.what()).find("Close the energy allocation window before saving")!=std::string::npos;}
            GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"complete_turn",{});}) && save_fenced,"Energy modal admitted turn/save or failed outside its actual owner save checkpoint");
            close(request+1,true);restored.DrainEvents();GT_ASSERT(game->GetCanonicalEconomy(base->m_owner)==expected && closed==0,"Stale close rolled back the live request");
            const auto old=request;close(request,true);restored.DrainEvents();
            GT_ASSERT(game->GetCanonicalEconomy(base->m_owner)==money && closed==1 && base->GetAccumulatedNutrients()==0,"Escape did not restore numeric rates by compensating recalculation");
            open();restored.DrainEvents();GT_ASSERT(request>old,"Energy modal reused a retired identity");
            adjust(30);restored.DrainEvents();expected=money;expected.rates={0,4};
            close(old,true);restored.DrainEvents();GT_ASSERT(game->GetCanonicalEconomy(base->m_owner)==expected && closed==1,"Retired modal rolled back a newer edit");
            close(request,false);restored.DrainEvents();GT_ASSERT(game->GetCanonicalEconomy(base->m_owner)==expected && closed==2 && restored.GameplayRandom()==random,"Enter failed to keep live rates without RNG");
            GT_OK();
        });
    });

    task->AddTest("SMAC retained global64 Rover survives recipient ownership and inactive source",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(SelectionSource,true,128,64,"SPARTANS");std::string saved;
        const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);auto* um=game->GetUM();
            auto* source=base->m_owner;auto* recipient=&game->GetState()->m_slots->GetSlot(6);
            // Retire fixture-only entities before publishing the integrated sight authority.
            um->DespawnUnit(GSE_CALL,91);game->GetBM()->DespawnBase(GSE_CALL,base->m_id);
            auto known=KnowledgeValue(true,true,128,64).CopyData();known.units.clear();
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(known));
            unit::ConventionalUnitState state{64,1,1,0,0,false,unit::ConventionalUnitState::Home::None,unit::SmacMoraleMode::Mode0};state.retained_state_word=0;
            auto* rover=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("choice-rover"),source,game->GetMap()->GetTile(8,6),state);
            auto* wrapper=rover->Wrap(GSE_CALL);const auto random=fixture.GameplayRandom();const auto before=rover->QueryStats(unit::SmacMoraleMode::Mode1);
            GT_ASSERT(before.full_allowance==6,"Fixture is not original selected global64 Rover");
            state.owner_simulation_id=2;auto transfer=um->PrepareDefectionUnits(GSE_CALL,recipient,{{92,state}});transfer.Commit();
            GT_ASSERT(rover->Conventional().design_id==64 && rover->m_owner==recipient && unit::Unit::Unwrap(wrapper)==rover
                && rover->QueryStats(unit::SmacMoraleMode::Mode1).full_allowance==6 && fixture.GameplayRandom()==random,
                "Retained global64 design followed the new controller's custom table or changed identity");
            bool ranked=false;fixture.ResourceState([&](auto& value){
                value.roster.front().active=false; // supplied consistent retired membership; no design reinitialization
                const auto& global=game->GetState()->m_settings.global;const auto& owned=*value.technology.Find(2);
                const auto counts=*value.economy.at(2).design_counts;faction::SmacDesignCounts empty{};
                ranked=counts[64]==1 && faction::CalculateSmacOrdinaryRankingScore(owned,0,counts,*global.GetSmacTechnologyRules(),global.GetSmacPredefinedUnitRules()->catalog,value.designs)
                    > faction::CalculateSmacOrdinaryRankingScore(owned,0,empty,*global.GetSmacTechnologyRules(),global.GetSmacPredefinedUnitRules()->catalog,value.designs);
            });
            GT_ASSERT(ranked && rover->QueryStats(unit::SmacMoraleMode::Mode1).full_allowance==6,
                "Inactive source lost retained material or recipient ranking counted wrong namespace");
            saved=fixture.Snapshot();GT_OK();
        });if(!first.empty()) {return first;}
        ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(SelectionSource,true,128,64,"SPARTANS");
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});const auto* rover=restored.Game()->GetUM()->GetUnit(92);
            GT_ASSERT(rover && rover->Conventional().owner_simulation_id==2 && rover->Conventional().design_id==64
                && rover->QueryStats(unit::SmacMoraleMode::Mode1).full_allowance==6,"Actual Start rebound global64 to recipient or discarded retired source");GT_OK();
        });
    });

    task->AddTest("SMAC defection Unit publication preserves identity counts and stale selection",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(SelectionSource,true);std::string saved;
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();Populate(GSE_CALL,fixture,false);auto* um=game->GetUM();auto* actor=um->GetUnit(91);
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            auto* previous=actor->m_owner;auto* recipient=&game->GetState()->m_slots->GetSlot(6);
            auto* wrapper=actor->Wrap(GSE_CALL);const auto record_order=um->GetUnitRecordOrder();const auto random=fixture.GameplayRandom();
            fixture.Updates(); // The original frontend view already exists.
            const auto before=fixture.Snapshot();auto next=actor->Conventional();next.owner_simulation_id=2;
            {auto dropped=um->PrepareDefectionUnits(GSE_CALL,recipient,{{91,next}});}
            GT_ASSERT(fixture.Snapshot()==before,"abandoned Unit transfer published authority");
            GT_ASSERT(Rejects([&]{auto invalid=um->PrepareDefectionUnits(GSE_CALL,recipient,{{91,next},{91,next}});})
                && fixture.Snapshot()==before,"late duplicate Unit transfer partially published");
            GT_ASSERT(game->SelectLocalConventionalUnit(game->GetTurnId(),{},{},actor->GetTile()->coord,false)==91,
                "fixture Unit was not selectable before ownership changed");
            auto plan=um->PrepareDefectionUnits(GSE_CALL,recipient,{{91,next}});plan.Commit();
            GT_ASSERT(um->GetUnit(91)==actor && unit::Unit::Unwrap(wrapper)==actor && actor->m_owner==recipient
                && actor->Conventional()==next && um->GetUnitRecordOrder()==record_order && fixture.GameplayRandom()==random,
                "Unit transfer replaced identity/wrapper/order or consumed RNG");
            GT_ASSERT(!game->SelectLocalConventionalUnit(game->GetTurnId(),91,91,actor->GetTile()->coord,false),
                "retained selection kept a transferred foreign Unit actionable");
            std::vector<game::FrontendRequest::request_type_t> published;
            for(const auto& update:fixture.Updates()) {
                if(update.type==game::FrontendRequest::FR_UNIT_DESPAWN && update.data.unit_despawn.unit_id==91) {published.push_back(update.type);}
                if(update.type==game::FrontendRequest::FR_UNIT_SPAWN && update.data.unit_spawn.unit_id==91) {
                    published.push_back(update.type);GT_ASSERT(update.data.unit_spawn.slot_index==6,"replacement view kept old faction");
                }
            }
            GT_ASSERT(published==std::vector<game::FrontendRequest::request_type_t>({game::FrontendRequest::FR_UNIT_DESPAWN,game::FrontendRequest::FR_UNIT_SPAWN}),
                "existing view was not replaced in removal/creation order");
            bool counts=false;fixture.ResourceState([&](auto& state){counts=state.economy.at(1).design_counts->at(2)==0 && state.economy.at(2).design_counts->at(2)==1;});
            GT_ASSERT(counts,"Unit transfer did not move saved active design counts");
            auto back=next;back.owner_simulation_id=1;auto returning=um->PrepareDefectionUnits(GSE_CALL,previous,{{91,back}});returning.Commit();fixture.Updates();
            GT_ASSERT(game->SelectLocalConventionalUnit(game->GetTurnId(),{},{},actor->GetTile()->coord,false)==91 && unit::Unit::Unwrap(wrapper)==actor,
                "foreign-to-local transfer lost stable selection identity");
            auto cleanup=back;cleanup.retained_state_word=0xffffffffu;cleanup.SetAccountingMarkers(6);
            auto marker_plan=um->PrepareDefectionUnits(GSE_CALL,previous,{{91,cleanup}});
            auto born=back;auto* pending=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("live-scout"),previous,game->GetMap()->GetTile(8,6),born);
            marker_plan.Commit();
            GT_ASSERT(actor->Conventional()==cleanup && actor->m_owner==previous && unit::Unit::Unwrap(wrapper)==actor,
                "Same-owner accounting cleanup changed identity or dropped opaque state");
            bool same_counts=false;fixture.ResourceState([&](auto& state){same_counts=state.economy.at(1).design_counts->at(2)==2 && state.economy.at(2).design_counts->at(2)==0;});
            GT_ASSERT(same_counts,"State-only cleanup changed active design counts");
            born.owner_simulation_id=2;auto newborn=um->PrepareDefectionUnits(GSE_CALL,recipient,{{92,born}});newborn.Commit();
            bool spawn=false;for(const auto& update:fixture.Updates()) {
                GT_ASSERT(update.type!=game::FrontendRequest::FR_UNIT_DESPAWN || update.data.unit_despawn.unit_id!=92,"unpublished newborn view was removed");
                if(update.type==game::FrontendRequest::FR_UNIT_SPAWN && update.data.unit_spawn.unit_id==92) {spawn=update.data.unit_spawn.slot_index==6;}
            }
            GT_ASSERT(spawn && um->GetUnit(92)==pending,"pending factory view did not publish its final recipient");
            saved=fixture.Snapshot();GT_OK();
        });GT_ASSERT(result.empty(),"Unit transfer: "+result);
        ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(SelectionSource,true);
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
            const auto* one=restored.Game()->GetUM()->GetUnit(91);const auto* two=restored.Game()->GetUM()->GetUnit(92);
            GT_ASSERT(one && two && one->Conventional().owner_simulation_id==1 && two->Conventional().owner_simulation_id==2,
                "actual Start lost transferred Unit ownership");
            GT_ASSERT(one->Conventional().retained_state_word==0xffffffefu && one->Conventional().accounting_markers==6,
                "Actual Start lost known full-word and accounting marker history");GT_OK();
        });
    });

    task->AddTest("SMAC riot legacy former faction pauses atomically and survives actual Start",[](GSE*,context::Context*)->std::string {
        const auto source=[](const std::string& name){auto text=HabitationGrowthSource(name,7);
            if(name=="gaians.txt") {const auto at=text.find("SOCIAL,-POLICE");text.replace(at,14,"SOCIAL,---POLICE");}return text;};
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(source,"GAIANS");std::string saved,repaired;
        const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,true,false,0,1,7);SelectionPresentations(fixture);auto* bm=game->GetBM();
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(names);
            auto context=base->GetSmacResourceContext();context.governor_options=0;context.drone_riots=true;base->SetSmacResourceContext(GSE_CALL,context);
            const auto stockpile=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
            base->SetProductionState(GSE_CALL,ProductionState{stockpile,7,stockpile,7,std::vector<ProductionItem>{}});
            Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),0));
            context.drone_riots=false;
            auto* peer=new Base(game,702,base->m_owner,base->m_faction,game->GetMap()->GetTile(16,16),"Unprocessed peer",{},pops,41,3,
                ProductionState{stockpile,0,stockpile,0,std::vector<ProductionItem>{}},context);
            bm->SpawnBase(GSE_CALL,peer);peer->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{1,{{},1}},Roles());
            auto known=KnowledgeValue(true,true,128,64).CopyData();for(auto& tile:known.tiles) {tile.explored_by|=2;tile.claimant=1;}
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(known));
            base->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{255,{{},0}},Roles());
            fixture.ResourceState([](auto& state){state.rank_order=faction::SmacRankOrder::Update({1,0,0,0,0,0,0},2);state.technology.factions[0].target=5;state.technology.factions[0].cached_threshold=100000;});
            auto random=fixture.GameplayRandom();random.SetCrt(8);fixture.GameplayRandom(random);
            const auto before=Base::Serialize(base).ToString(),before_peer=Base::Serialize(peer).ToString();
            game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
            auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));auto* value=Call(GSE_CALL,wrapper,"get_pending_smac_colony");
            GT_ASSERT(value->type==VT_OBJECT,"Actual ongoing riot did not request missing former faction");
            auto* pending=static_cast<gse::value::Object*>(value);const auto id=static_cast<gse::value::Int*>(pending->Get("request_id"))->value;
            GT_ASSERT(static_cast<gse::value::String*>(pending->Get("kind"))->value=="former_owner" && !base->GetSmacResourceContext().FormerOwner()
                && Base::Serialize(base).ToString()==before && Base::Serialize(peer).ToString()==before_peer && fixture.GameplayRandom()==random,
                "Question posted food/minerals/CRT/history or replayed another Base");
            game->Event(GSE_CALL,"choose_former_faction",{{"request_id",VALUE(gse::value::Int,,id)},{"former_owner",VALUE(gse::value::Int,,-1)}});fixture.DrainEvents();
            GT_ASSERT(!base->GetSmacResourceContext().FormerOwner() && Base::Serialize(base).ToString()==before && fixture.GameplayRandom()==random,
                "Cancel inferred history or changed the checkpoint");
            saved=fixture.Snapshot();GT_OK();
        });if(!first.empty()) {return first;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(source,"GAIANS");
        const auto resumed=restored.Run([&](GSE_CALLABLE)->std::string {
            const auto random=restored.GameplayRandom();(void)random;
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}restored.BasePresentationRoles(names);
            auto* pending=Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony");
            GT_ASSERT(pending->type==VT_OBJECT && static_cast<value::String*>(static_cast<value::Object*>(pending)->Get("kind"))->value=="former_owner"
                && !base->GetSmacResourceContext().FormerOwner() && base->GetAccumulatedNutrients()==13
                && game->GetBM()->GetBase(702)->GetAccumulatedNutrients()==3,"Actual Start replayed the pending Base or inferred an answer");
            const auto id=static_cast<value::Int*>(static_cast<value::Object*>(pending)->Get("request_id"))->value;
            const auto before=Base::Serialize(base).ToString();auto expected=restored.GameplayRandom();
            game->Event(GSE_CALL,"choose_former_faction",{{"request_id",VALUE(value::Int,,id+1)},{"former_owner",VALUE(value::Int,,0)}});restored.DrainEvents();
            GT_ASSERT(Base::Serialize(base).ToString()==before && !base->GetSmacResourceContext().FormerOwner(),"Stale answer repaired a newer request");
            game->Event(GSE_CALL,"choose_former_faction",{{"request_id",VALUE(value::Int,,id)},{"former_owner",VALUE(value::Int,,0)}});restored.DrainEvents();
            for(int i=0;i<22;++i) {expected.NextCrtRaw15();}
            GT_ASSERT(base->GetSmacResourceContext().FormerOwner()==0 && base->GetSmacResourceContext().former_owner_migration
                && base->GetSmacResourceContext().drone_riots && base->GetAccumulatedNutrients()>13 && restored.GameplayRandom()==expected,
                "Accepted former0 result: history="+std::to_string(base->GetSmacResourceContext().FormerOwner().value_or(-1))+" migration="+std::to_string(base->GetSmacResourceContext().former_owner_migration)+" riots="+std::to_string(base->GetSmacResourceContext().drone_riots)+" food="+std::to_string(base->GetAccumulatedNutrients())+" CRT="+std::to_string(restored.GameplayRandom().Crt())+" expected="+std::to_string(expected.Crt()));
            repaired=restored.Snapshot();GT_OK();
        });if(!resumed.empty()) {return resumed;}
        ConventionalUnitFixture final_restore;final_restore.ConfigureProduction(source,"GAIANS");
        return final_restore.Run([&](GSE_CALLABLE)->std::string {
            final_restore.RestoreThroughStart(GSE_CALL,repaired,[](GSE_CALLABLE){});
            const auto context=final_restore.Game()->GetBM()->GetBase(701)->GetSmacResourceContext();
            GT_ASSERT(context.FormerOwner()==0 && context.former_owner_migration && context.drone_riots,
                "Accepted migration provenance or explicit former0 was lost on actual Start");GT_OK();
        });
    });

    task->AddTest("SMAC riot live transfer follows later and earlier owner traversal",[](GSE*,context::Context*)->std::string {
        const auto source=[](const std::string& name){auto text=HabitationGrowthSource(name,7);
            if(name=="gaians.txt") {const auto at=text.find("SOCIAL,-POLICE");text.replace(at,14,"SOCIAL,---POLICE");}return text;};
        for(const int32_t scenario:{1,2,3,4,5,6,7,8}) {
            const bool inactive=scenario>=5;const int32_t old_owner=scenario>=3 ? 1:scenario;const bool screen=scenario==3 || scenario==8,suspended=scenario==4 || scenario==7;
            const int32_t recipient=3-old_owner,former=scenario==1 ? old_owner:recipient;std::string saved;int64_t food=0,final_food=0;
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(source,true);
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* bm=game->GetBM();
                auto* base=Populate(GSE_CALL,fixture,true,false,old_owner==1 ? 0:6,old_owner,screen ? 1:7);SelectionPresentations(fixture);
                if(screen) {base->SetAccumulatedNutrients(0);}
                if(screen || suspended) {for(const size_t slot:{0u,6u}) {game->GetState()->m_slots->GetSlot(slot).GetPlayer()->SetDifficultyLevel("Librarian");}}
                std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(names);
                auto context=base->GetSmacResourceContext();context.governor_options=0;context.drone_riots=true;
                context.uncaptured_former_owner=former;context.retained_status_word=2;base->SetSmacResourceContext(GSE_CALL,context);
                const auto stockpile=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
                base->SetProductionState(GSE_CALL,ProductionState{stockpile,7,stockpile,7,std::vector<ProductionItem>{},true});
                if(screen) {const auto colony=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
                    base->SetProductionState(GSE_CALL,ProductionState{colony,100,colony,100,std::vector<ProductionItem>{},true});}
                const auto make_peer=[&](size_t id,int32_t owner,size_t x,size_t y) {
                    auto* slot=&game->GetState()->m_slots->GetSlot(owner==1 ? 0:6);
                    Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),0));
                    auto copy=context;copy.owner_simulation_id=owner;copy.drone_riots=false;copy.uncaptured_former_owner=owner;copy.SyncKnownStatusBits();
                    const auto item=game->MakeSmacProductionItem(slot,ProductionItemKind::Stockpile,38);
                    auto* peer=new Base(game,id,slot,slot->GetPlayer()->GetFaction(),game->GetMap()->GetTile(x,y),"Transfer peer",{},pops,41,3,
                        ProductionState{item,0,item,0,std::vector<ProductionItem>{}},copy);
                    bm->SpawnBase(GSE_CALL,peer);peer->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{1,{{},1}},Roles());return peer;
                };
                auto* old_peer=make_peer(702,old_owner,16,16);if(!inactive) {make_peer(703,recipient,24,8);}
                if(suspended) {bm->DefineFacility(new FacilityDef("transfer-hq","HQ","",0,{},1));old_peer->m_facilities.emplace("transfer-hq",bm->GetFacilityDef("transfer-hq"));}
                auto known=KnowledgeValue(true,true,128,64).CopyData();known.observers[2]=Knowledge::Observer{false,{}};known.observers[2]->pact_with=0;
                for(auto& tile:known.tiles) {tile.explored_by=6;tile.claimant=old_owner;}
                known.occupancy=map::SmacOccupancyCache::Blank(128,64,true);
                for(const auto& row:bm->GetBases()) {const auto* tile=row.second->GetTile();known.occupancy->FoundBase({static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)},row.second->GetSmacResourceContext().owner_simulation_id);}
                for(const auto& row:game->GetUM()->GetUnits()) {const auto* tile=row.second->GetTile();known.occupancy->PlaceUnit({static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)},row.second->SimulationOwner(),true);}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(known));
                using UnitState=unit::ConventionalUnitState;auto* um=game->GetUM();
                const auto spawn=[&](size_t id,int32_t owner,int32_t design,size_t home,size_t x,size_t y,const std::string& definition) {
                    UnitState record{design,owner,1,0,0,false,UnitState::Home::Base,unit::SmacMoraleMode::Mode0};record.home_base_id=home;
                    record.retained_state_word=0xffffffffu;record.SetAccountingMarkers(7);
                    return um->SpawnConventionalUnit(GSE_CALL,id,um->GetUnitDef(definition),&game->GetState()->m_slots->GetSlot(owner==1 ? 0:6),game->GetMap()->GetTile(x,y),record);
                };
                spawn(92,old_owner,2,701,screen ? 22:2,screen ? 20:2,"choice-scout");
                spawn(93,old_owner,screen ? 2:1,701,20,20,screen ? "choice-scout":"choice-former");
                if(!inactive) {spawn(94,recipient,2,703,5,3,"choice-scout");}
                if(screen) {auto terrain=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                    for(const auto x:{20u,22u}) {terrain.tiles.at(20*64+x/2).claimant=recipient;}
                    game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(terrain));}
                base->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{screen ? 5u:255u,{{},0}},Roles());
                fixture.ResourceState([&](auto& state){std::array<int32_t,7> scores{};scores[old_owner-1]=10;scores[recipient-1]=1;
                    state.rank_order=faction::SmacRankOrder::Update(scores,6);
                    if(inactive) {for(auto& row:state.roster) {if(row.simulation_id==recipient) {row.active=false;}}if(scenario==6) {state.human_membership&=~(1u<<recipient);}state.retained_base_totals->at(recipient)=0;state.economy.at(recipient).credits=217;state.economy.at(recipient).pending_debit=11;state.economy.at(recipient).research_fraction=37;}
                    state.battle_plan_target[0]=701;state.battle_plan_target[1]=701;state.battle_plan_target[2]=702;state.battle_plan_target[3]=701;
                    for(size_t i=0;i<2;++i) {state.technology.factions[i].target=5;state.technology.factions[i].cached_threshold=100000;}
                    if(suspended) {state.economy.at(old_owner).rates={0,10};state.technology.factions[old_owner-1].target.reset();}
                });
                auto random=fixture.GameplayRandom();random.SetCrt(8);fixture.GameplayRandom(random);
                const auto surplus=base->PrepareSmacResourceCalculation().nutrients.surplus;
                const auto credits=game->GetCanonicalEconomy(base->m_owner).credits;
                int64_t labs=0;fixture.ResourceState([&](auto& state){labs=state.technology.factions[old_owner-1].labs;});
                game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
                if(screen) {
                    auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));auto* question=Call(GSE_CALL,wrapper,"get_pending_smac_colony");
                    GT_ASSERT(question->type==VT_OBJECT,"Funded size-one Colony did not suspend before the riot tail");
                    game->Event(GSE_CALL,"choose_colony",{{"request_id",static_cast<value::Object*>(question)->Get("request_id")},{"choice",VALUE(value::String,,"screen")}});fixture.DrainEvents();
                    auto* pending=Call(GSE_CALL,wrapper,"get_pending_smac_colony_screen");
                    GT_ASSERT(pending->type==VT_OBJECT && base->GetSmacResourceContext().owner_simulation_id==recipient
                        && static_cast<value::Bool*>(static_cast<value::Object*>(pending)->Get("read_only"))->value,
                        "Accepted Colony Screen did not retain the transferred stable Base as read-only");
                    if(inactive) {auto* calculation=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_resource_calculation"));
                        GT_ASSERT(static_cast<value::Bool*>(calculation->Get("retained_only"))->value,"Inactive Screen did not distinguish its retained resource display");}
                    bool research_untouched=false;fixture.ResourceState([&](auto& state){research_untouched=state.technology.factions[old_owner-1].labs==labs;});
                    GT_ASSERT(base->GetAccumulatedNutrients()==surplus && research_untouched
                        && game->GetCanonicalEconomy(&game->GetState()->m_slots->GetSlot(0)).credits==credits,
                        "Transferred Screen replayed food or posted old-owner research/income before closing");
                    game->Event(GSE_CALL,"close_colony_screen",{{"request_id",static_cast<value::Object*>(pending)->Get("request_id")}});fixture.DrainEvents();
                    GT_ASSERT(Call(GSE_CALL,wrapper,"get_pending_smac_colony_screen")->type==VT_NULL,"Transferred Screen did not close");
                }
                GT_ASSERT(game->GetTurnId()==2 && base->GetSmacResourceContext().owner_simulation_id==recipient,
                    "Actual owner pass did not transfer the rioting Base");
                const auto occupancy=game->GetMap()->GetSmacWorkerKnowledge()->CopyData().occupancy;const auto* center=base->GetTile();
                GT_ASSERT(occupancy->At({static_cast<int32_t>(center->coord.x),static_cast<int32_t>(center->coord.y)}).affiliation==recipient,"Riot transfer omitted independent Base-center cache write");
                if(!screen) {GT_ASSERT(occupancy->At({2,2}).affiliation==recipient && (occupancy->At({2,2}).presence&2u),"Riot Unit conversion omitted its separate cache write");}
                GT_ASSERT(!(occupancy->At({20,20}).presence&2u),"Riot casualty retained Unit presence");
                food=(screen ? 0:13)+surplus*(old_owner==1 && !suspended && !inactive ? 2:1);final_food=13+2*surplus;
                GT_ASSERT(base->GetAccumulatedNutrients()==food && base->GetSmacResourceContext().FormerOwner()==former
                    && base->GetSmacResourceContext().conquest->assimilation==(former==recipient ? 0:49),
                    "Transferred Base replayed or skipped its normal owner pass: food="+std::to_string(base->GetAccumulatedNutrients())+" expected="+std::to_string(food));
                GT_ASSERT(!um->GetUnit(93) && (screen ? !um->GetUnit(92):(um->GetUnit(92)->Conventional().owner_simulation_id==recipient
                    && um->GetUnit(92)->Conventional().home_base_id==701)) && um->GetUnit(91)->Conventional().owner_simulation_id==old_owner,
                    "Riot transfer did not convert inner Units, lose remote target homes, or preserve homeless Units");
                GT_ASSERT((screen || (*um->GetUnit(92)->Conventional().retained_state_word&0x03004200u)==0)
                    && (inactive || (*um->GetUnit(94)->Conventional().retained_state_word&0x40000u)==0),
                    "Riot Unit cleanup lost the literal state masks");
                GT_ASSERT(base->GetProductionState()->hurry_partial_paid==(old_owner==2 || suspended || inactive)
                    && bool(*base->GetSmacResourceContext().retained_status_word&0x40000000u)==(old_owner==2 || suspended || inactive),
                    "Partial Hurry marker did not survive transfer until its recipient owner reset");
                bool retained=false;fixture.ResourceState([&](auto& state){retained=state.battle_plan_target[0]==701 && state.battle_plan_target[1]==-1
                    && state.battle_plan_target[2]==702 && state.battle_plan_target[3]==-1;});
                GT_ASSERT(retained,"Transfer cleared slot0/unrelated battle targets or retained a matching positive target");
                if(inactive) {bool still_inactive=false;fixture.ResourceState([&](auto& state){for(const auto& row:state.roster) {if(row.simulation_id==recipient) {still_inactive=!row.active && state.economy.at(recipient).credits==217 && state.economy.at(recipient).pending_debit==11 && state.economy.at(recipient).research_fraction==37 && state.IsHuman(recipient)==(scenario!=6);}}});
                    GT_ASSERT(still_inactive,"Riot transfer activated its eliminated recipient");
                    const auto before=fixture.Snapshot();bool rejected=false;
                    try {base->SetProductionState(GSE_CALL,base->GetProductionState());}catch(const std::runtime_error&) {rejected=true;}
                    GT_ASSERT(rejected && fixture.Snapshot()==before,"Inactive production command bypassed its active-owner fence or retained private calculation permission");
                    if(!screen) {auto* unit=um->GetUnit(92);
                        GT_ASSERT(Rejects([&]{um->UpdateConventionalUnit(GSE_CALL,unit,unit->Conventional());})
                            && Rejects([&]{um->PrepareConventionalRemoval(GSE_CALL,{92});})
                            && Rejects([&]{um->ResetConventionalSpent(GSE_CALL,unit);}) && fixture.Snapshot()==before,
                            "Inactive Unit command bypassed the active-owner boundary");}
                    if(scenario==6) {GT_ASSERT(base->GetSmacUpkeepCache()->calculation.citizens->prefix.content_allowance==3,"Inactive AI recipient used human citizen difficulty");}}
                if(suspended) {GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_research")->type==VT_OBJECT,
                    "Old peer did not suspend at actual research after transfer");}
                saved=fixture.Snapshot();GT_OK();
            });if(!result.empty()) {return std::to_string(scenario)+" transfer "+std::to_string(old_owner)+" to "+std::to_string(recipient)+": "+result;}
            ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(source,true);
            const auto restore=restored.Run([&](GSE_CALLABLE)->std::string {
                if(screen || suspended) {for(const size_t slot:{0u,6u}) {restored.Game()->GetState()->m_slots->GetSlot(slot).GetPlayer()->SetDifficultyLevel("Librarian");}}
                restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* base=restored.Game()->GetBM()->GetBase(701);
                GT_ASSERT(base->GetSmacResourceContext().owner_simulation_id==recipient && base->GetAccumulatedNutrients()==food,
                    "Actual Start lost completed ownership traversal");
                if(inactive) {bool repaired=false;restored.ResourceState([&](auto& state){for(const auto& row:state.roster) {if(row.simulation_id==recipient) {repaired=row.active;}}});
                    GT_ASSERT(repaired,"Actual Start did not repair inactive Base ownership");}
                if(suspended) {
                    std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}restored.BasePresentationRoles(names);
                    auto* game=restored.Game();auto* pending=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_research"));
                    auto* choice=static_cast<value::Object*>(static_cast<value::Array*>(pending->Get("choices"))->value.front());
                    game->Event(GSE_CALL,"choose_research",{{"request_id",pending->Get("request_id")},{"technology_id",choice->Get("id")}});restored.DrainEvents();
                    GT_ASSERT(base->GetAccumulatedNutrients()==final_food && !base->GetProductionState()->hurry_partial_paid,
                        "Restored old-peer research replayed transfer or skipped the recipient's later pass");
                }GT_OK();
            });if(!restore.empty()) {return restore;}
        }GT_OK();
    });

    task->AddTest("SMAC load roster repair uses Bases or Colony role and preserves retained state",[](GSE*,context::Context*)->std::string {
        const auto source=[](const std::string& name){return HabitationGrowthSource(name,7);};
        for(int scenario=0;scenario<7;++scenario) {
            const bool base_owned=scenario==0 || scenario==6,pod=scenario==1 || scenario==5;
            const bool prior_active=scenario==2 || scenario==3;
            const bool ending=scenario==6;std::string saved;SmacGameplayRandomState random{0,0};
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(source,true);
            const auto prepared=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();Populate(GSE_CALL,fixture,true,false,base_owned ? 6:0,base_owned ? 2:1,7);SelectionPresentations(fixture);
                if(!base_owned && scenario!=3) {
                    using namespace unit;ConventionalUnitState record{pod ? 0:2,2,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};
                    game->GetUM()->SpawnConventionalUnit(GSE_CALL,92,game->GetUM()->GetUnitDef(pod ? "choice-colony":"choice-scout"),
                        &game->GetState()->m_slots->GetSlot(6),game->GetMap()->GetTile(6,2),record);
                }
                fixture.ResourceState([&](auto& state){state.roster[1].active=prior_active;
                    if(scenario==5) {state.human_membership&=~4u;}
                    state.economy.at(2).credits=217;state.economy.at(2).research_fraction=37;
                    state.technology.factions[1].labs=123;state.technology.factions[1].target=5;
                    if(ending) {state.human_defeat_request=2;}
                });random=fixture.GameplayRandom();saved=fixture.Snapshot();GT_OK();
            });if(!prepared.empty()) {return prepared;}
            ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(source,true);
            const auto result=restored.Run([&](GSE_CALLABLE)->std::string {
                if(ending) {
                    bool rejected=false;try {restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});}
                    catch(const std::runtime_error& error) {rejected=std::string(error.what()).find("ending conflicts")!=std::string::npos;}
                    GT_ASSERT(rejected,"Contradictory saved ending silently changed retained ending/roster authority");
                    bool unavailable=false;try {(void)restored.Snapshot();}catch(const std::exception&) {unavailable=true;}
                    GT_ASSERT(unavailable,"Rejected ending/repair world remained saveable");GT_OK();
                }
                restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});bool exact=false;
                restored.ResourceState([&](auto& state){exact=state.roster[1].active==(base_owned || pod)
                    && state.IsHuman(2)==(scenario!=5) && state.economy.at(2).credits==217 && state.economy.at(2).research_fraction==37
                    && state.technology.factions[1].labs==123 && state.technology.factions[1].target==5;});
                GT_ASSERT(exact && restored.GameplayRandom()==random,"Load roster repair changed retained fields/control/RNG or used an ordinary Unit as a Pod");
                (void)restored.Snapshot();GT_OK();
            });if(!result.empty()) {return "roster repair "+std::to_string(scenario)+": "+result;}
        }GT_OK();
    });

    task->AddTest("SMAC citizen live allocation status and retained Save Start",[](GSE*,context::Context*)->std::string {
        const auto source=[](const std::string& name){return HabitationGrowthSource(name,4);};
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(source,"GAIANS");std::string saved;
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,true,false,0,1,4);SelectionPresentations(fixture);
            auto known=KnowledgeValue(true,true,128,64).CopyData();
            for(auto& tile:known.tiles) {tile.explored_by|=2;tile.claimant=1;}
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(known));
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(names);
            base->m_owner->GetPlayer()->SetDifficultyLevel("Transcend");
            auto context=base->GetSmacResourceContext();context.governor_options=0;base->SetSmacResourceContext(GSE_CALL,context);
            fixture.ResourceState([](auto& state){state.social.factions[0].current[3]=0;state.social.factions[0].current[5]=0;
                state.economy.at(1).credits=100;state.technology.factions[0].owned[5]=false;
                state.technology.factions[0].target=5;state.technology.factions[0].cached_threshold=100000;});
            SmacBaseAllocationState workers{31,{{},0}};base->PublishSmacAllocation(GSE_CALL,workers,Roles());
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            base->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
            const auto random=fixture.GameplayRandom();const auto first=base->PrepareSmacResourceCalculation();
            GT_ASSERT(first.citizens && first.citizens->population==4 && first.citizens->excluded==0
                && first.citizens->early.initial.drones==3 && first.citizens->final.drones>first.citizens->final.talents,
                "actual provider did not consume current population/allocation and difficulty: "+(first.citizens ? std::to_string(first.citizens->population)+"/"+std::to_string(first.citizens->excluded)+" initialD="+std::to_string(first.citizens->early.initial.drones)+" finalD="+std::to_string(first.citizens->final.drones)+" T="+std::to_string(first.citizens->final.talents):"absent"));
            GT_ASSERT(base->PrepareSmacResourceCalculation().citizens==first.citizens && fixture.GameplayRandom()==random
                && !base->GetSmacResourceContext().drone_riots,"read-only citizen query made draws or admitted status");
            game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
            GT_ASSERT(game->GetTurnId()==2 && base->GetSmacResourceContext().drone_riots && base->GetSmacUpkeepCache()->calculation.citizens,
                "actual owner upkeep did not admit riots after its second pass");
            const auto minerals=base->GetProductionState()->minerals;
            SmacBaseAllocationState doctors{1,{{},4}};doctors.specialists.types.fill(1);base->PublishSmacAllocation(GSE_CALL,doctors,Roles());
            const auto calm=base->PrepareSmacResourceCalculation();
            GT_ASSERT(calm.citizens && calm.citizens->excluded==4 && calm.citizens->final.drones==0
                && base->GetSmacResourceContext().drone_riots,"Doctor query changed status before actual upkeep");
            game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
            GT_ASSERT(game->GetTurnId()==3 && !base->GetSmacResourceContext().drone_riots
                && base->GetProductionState()->minerals==minerals
                && base->GetSmacUpkeepCache()->calculation.citizens->excluded==4,
                "recovery used new calm status for incoming mineral posting or lost second-pass history");
            saved=fixture.Snapshot();GT_OK();
        });GT_ASSERT(result.empty(),"live citizen caller: "+result);
        ConventionalUnitFixture restored;restored.ConfigureProduction(source,"GAIANS");
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
            const auto* base=restored.Game()->GetBM()->GetBase(701);
            GT_ASSERT(base && !base->GetSmacResourceContext().drone_riots && base->GetSmacUpkeepCache()
                && base->GetSmacUpkeepCache()->calculation.citizens
                && base->GetSmacUpkeepCache()->calculation.citizens->excluded==4
                && base->PrepareSmacResourceCalculation().citizens->final.drones==0,
                "actual Save Start lost citizen history or status");GT_OK();
        });
    });

    task->AddTest("SMAC faction energy rates preview and atomically refresh two owned Bases",[](GSE*,context::Context*)->std::string {
        const auto source=[](const std::string& name){auto data=TanksSource(name);if(name=="alpha.txt") {
            for(const auto& row:std::array<std::pair<std::string,std::string>,2>{{{"Center,2,2,1","Center,2,2,20"},{"Forest,2,2,2","Forest,2,2,10"}}}) {
                const auto at=data.find(row.first);if(at==std::string::npos) {throw std::runtime_error("Missing controlled energy source row");}data.replace(at,row.first.size(),row.second);
            }}return data;};
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(source,"GAIANS");std::string saved;
        std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i){roles[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(roles);
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* first=Populate(GSE_CALL,fixture,true);SelectionPresentations(fixture);auto* bm=game->GetBM();
            auto context=first->GetSmacResourceContext();context.governor_options=0;first->SetSmacResourceContext(GSE_CALL,context);
            const auto scout=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,2);
            first->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
            Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),0));
            auto* second=new Base(game,702,first->m_owner,first->m_faction,game->GetMap()->GetTile(8,8),"Second",{},pops,41,0,
                ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}},context);
            bm->SpawnBase(GSE_CALL,second);game->GetMap()->GetTile(10,8)->terraforming=TERRAFORMING_FOREST;
            second->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{3,{{},0}},Roles());
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            first->RecalculateSmacWorkers(GSE_CALL,Roles());second->RecalculateSmacWorkers(GSE_CALL,Roles());
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();const auto money=game->GetCanonicalEconomy(first->m_owner);
            const auto old=game->GetCanonicalBaseEnergy(*first).tail.final;
            const auto candidate_first=game->PreviewCanonicalBaseEnergy(*first,{2,7}),candidate_second=game->PreviewCanonicalBaseEnergy(*second,{2,7});
            auto* preview=static_cast<value::Object*>(game->CopySmacEnergyRatePreview(GSE_CALL,*first,SmacEnergyRates{2,7}));
            GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random && !(candidate_first.tail.final==old)
                && static_cast<value::Array*>(preview->Get("bases"))->value.size()==2,"Candidate preview mutated authority or omitted a Base");
            auto expected=static_cast<value::String*>(preview->Get("expected_state"))->value;
            const auto command=[&](int64_t psych,int64_t labs){game->Event(GSE_CALL,"set_smac_energy_rates",{{"base",first->Wrap(GSE_CALL)},
                {"expected_state",VALUE(value::String,,expected)},{"psych",VALUE(value::Int,,psych)},{"labs",VALUE(value::Int,,labs)}});};
            fixture.Updates();command(2,7);GT_ASSERT(fixture.Snapshot()==before,"Rate edit applied before native dispatch");fixture.DrainEvents();
            auto after_money=money;after_money.rates={2,7};
            GT_ASSERT(game->GetCanonicalEconomy(first->m_owner)==after_money && fixture.GameplayRandom()==random
                && game->GetCanonicalBaseEnergy(*first).tail.final==candidate_first.tail.final && game->GetCanonicalBaseEnergy(*second).tail.final==candidate_second.tail.final,
                "Rate publication posted income/RNG or failed current outputs");
            std::set<size_t> refreshed;for(const auto& update:fixture.Updates()) {if(update.type==game::FrontendRequest::FR_BASE_UPDATE) {refreshed.insert(update.data.base_update.base_id);}}
            GT_ASSERT(refreshed==std::set<size_t>({701,702}),"Rate edit did not refresh both owned Bases");
            const auto accepted=fixture.Snapshot();command(0,5);fixture.DrainEvents();GT_ASSERT(fixture.Snapshot()==accepted,"Stale faction dialog changed rates");
            expected=static_cast<value::String*>(static_cast<value::Object*>(game->CopySmacEnergyRatePreview(GSE_CALL,*first,{}))->Get("expected_state"))->value;
            for(const auto rates:std::vector<std::array<int64_t,2>>{{-1,5},{2,9},{11,0},{0,INT64_MAX}}) {
                GT_ASSERT(Rejects([&]{command(rates[0],rates[1]);}) && fixture.Snapshot()==accepted,"Invalid rate edit published state");
            }
            bm->DefineFacility(new FacilityDef("unresolved-rate","Unknown","",0,{}));second->m_facilities.emplace("unresolved-rate",bm->GetFacilityDef("unresolved-rate"));
            const auto late=fixture.Snapshot();GT_ASSERT(Rejects([&]{command(0,5);}) && fixture.Snapshot()==late,"Late second-Base energy failure partially published rates");
            second->m_facilities.erase("unresolved-rate");
            saved=fixture.Snapshot();GT_OK();
        });GT_ASSERT(result.empty(),"Faction energy rates: "+result);
        ConventionalUnitFixture restored;restored.ConfigureProduction(source,"GAIANS");
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
            const auto rates=game->GetCanonicalEconomy(base->m_owner).rates;
            GT_ASSERT(rates.psych==2 && rates.labs==7 && game->GetBM()->GetBase(702),"Actual Start lost faction rates or peer Base");
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}restored.BasePresentationRoles(names);
            base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
            restored.ResourceState([](auto& state){auto& economy=state.economy.at(1);economy.credits=100;economy.pending_debit=0;economy.research_fraction=37;
                auto& technology=state.technology.factions[0];technology.owned[5]=false;technology.target=5;technology.cached_threshold=100000;technology.labs=0;
                state.social.factions[0].current[10]=0;});
            const auto one=game->GetCanonicalBaseEnergy(*base),two=game->GetCanonicalBaseEnergy(*game->GetBM()->GetBase(702));
            const auto expected_credits=100+one.tail.final.economy+two.tail.final.economy-one.maintenance-two.maintenance;
            const auto expected_labs=one.tail.final.labs+two.tail.final.labs;
            GT_ASSERT(expected_labs>0,"Controlled rate fixture has no positive research to submit");
            game->Event(GSE_CALL,"complete_turn",{});restored.DrainEvents();
            bool submitted=false;restored.ResourceState([&](auto& state){submitted=state.technology.factions[0].labs==expected_labs && state.economy.at(1).research_fraction==37;});
            GT_ASSERT(game->GetTurnId()==2 && submitted && game->GetCanonicalEconomy(base->m_owner).credits==expected_credits,
                "Next actual owner upkeep did not credit both Bases and submit labs from saved faction rates");GT_OK();
        });
    });

    task->AddTest("SMAC Governor local Apply commits independent remembered state and actual restore",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(SelectionSource,"GAIANS");std::string saved;
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            base->SetProductionState(GSE_CALL,ProductionState{scout,7,scout,11,std::vector<ProductionItem>{scout}});
            auto context=base->GetSmacResourceContext();context.governor_options=0x42000040u;base->SetSmacResourceContext(GSE_CALL,context);
            const auto production=base->GetProductionState();const auto allocation=base->GetSmacAllocation();const auto random=fixture.GameplayRandom();
            fixture.ResourceState([](auto& state){state.remembered_governor[1].reset();state.remembered_governor[0]=0xffffffffu;state.remembered_governor[7]=0x12345678u;});
            auto expected=base->SmacGovernorSelectionState();const auto before=fixture.Snapshot();
            const auto command=[&](int64_t controls){game->Event(GSE_CALL,"apply_smac_governor",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,expected)},{"controls",VALUE(value::Int,,controls)}});};
            for(const int64_t controls:{int64_t(-1),int64_t(0x80000),int64_t(1|4),int64_t(1|8),int64_t(1|16)}) {
                GT_ASSERT(Rejects([&]{command(controls);}) && fixture.Snapshot()==before,"Unavailable Governor branch or malformed selection changed state");
            }
            command(0);GT_ASSERT(fixture.Snapshot()==before,"Governor applied before dispatch");fixture.DrainEvents();
            bool remembered=false;fixture.ResourceState([&](auto& state){remembered=state.remembered_governor[1]==0x02000000u && state.remembered_governor[0]==0xffffffffu && state.remembered_governor[7]==0x12345678u;});
            GT_ASSERT(remembered && base->GetSmacResourceContext().governor_options==0x02000000u
                && base->GetProductionState()==production && base->GetSmacAllocation()==allocation && fixture.GameplayRandom()==random,
                "Local inactive Apply changed allocation/production/RNG or lost independent templates");
            const auto accepted=fixture.Snapshot();command(2);fixture.DrainEvents();GT_ASSERT(fixture.Snapshot()==accepted,"Stale Governor command changed state");
            expected=base->SmacGovernorSelectionState();fixture.ResourceState([](auto& state){state.remembered_governor[1]=7u;});
            const auto changed=fixture.Snapshot();command(2);fixture.DrainEvents();GT_ASSERT(fixture.Snapshot()==changed,"Changed remembered authority did not stale the command");
            expected=base->SmacGovernorSelectionState();command(1);fixture.DrainEvents();
            GT_ASSERT(base->GetSmacResourceContext().governor_options==0x82000000u,"Supported active no-management Apply failed");
            saved=fixture.Snapshot();GT_OK();
        });GT_ASSERT(result.empty(),"Governor Apply: "+result);
        ConventionalUnitFixture restored;restored.ConfigureProduction(SelectionSource,"GAIANS");
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.Restore(GSE_CALL,saved,true);auto* base=restored.Game()->GetBM()->GetBase(701);bool remembered=false;
            restored.ResourceState([&](auto& state){remembered=state.remembered_governor[1]==0x82000000u && state.remembered_governor[0]==0xffffffffu && state.remembered_governor[7]==0x12345678u;});
            GT_ASSERT(remembered && base->GetSmacResourceContext().governor_options==0x82000000u,"Actual Start restore lost Governor settings");GT_OK();
        });
    });

    task->AddTest("SMAC queued Hurry discards a Base retired by actual earlier upkeep",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,true);SelectionPresentations(fixture);auto* bm=game->GetBM();
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            const auto stockpile=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
            base->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
            auto context=base->GetSmacResourceContext();context.resource_events=SmacBaseResourceContext::Famine;context.governor_options=0;base->SetSmacResourceContext(GSE_CALL,context);base->SetAccumulatedNutrients(-1);
            Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),0));
            auto survivor_context=context;survivor_context.resource_events=0;
            auto* survivor=new Base(game,702,base->m_owner,base->m_faction,game->GetMap()->GetTile(8,8),"Survivor",{},pops,41,0,
                ProductionState{stockpile,0,stockpile,0,std::vector<ProductionItem>{}},survivor_context);
            bm->SpawnBase(GSE_CALL,survivor);survivor->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{1,{{},1}},Roles());
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(names);
            auto* wrapper=static_cast<value::Object*>(base->Wrap(GSE_CALL));const auto offer=base->GetSmacHurryOffer();
            const auto governor_expected=base->SmacGovernorSelectionState();
            auto* rate_preview=static_cast<value::Object*>(game->CopySmacEnergyRatePreview(GSE_CALL,*base,{}));
            const auto rate_expected=static_cast<value::String*>(rate_preview->Get("expected_state"))->value;
            game->Event(GSE_CALL,"complete_turn",{});
            game->Event(GSE_CALL,"hurry_smac_production",{{"base",wrapper},{"expected_state",VALUE(value::String,,offer.expected)},{"full",VALUE(value::Bool,,false)},{"amount",VALUE(value::Int,,0)}});
            game->Event(GSE_CALL,"apply_smac_governor",{{"base",wrapper},{"expected_state",VALUE(value::String,,governor_expected)},{"controls",VALUE(value::Int,,2)}});
            game->Event(GSE_CALL,"set_smac_energy_rates",{{"base",wrapper},{"expected_state",VALUE(value::String,,rate_expected)},{"psych",VALUE(value::Int,,0)},{"labs",VALUE(value::Int,,5)}});
            fixture.DrainEvents();
            GT_ASSERT(!bm->GetBase(701) && bm->GetBase(702)==survivor && !wrapper->wrapobj && wrapper->type==VT_UNDEFINED,
                "Earlier turn did not retire queued Hurry target through actual upkeep");
            GT_ASSERT(!survivor->GetProductionState()->hurry_partial_paid,"Stale Hurry mutated replacement Base");GT_OK();
        });
    });
    task->AddTest("SMAC live Hurry full purchase stages actual credits minerals baselines and stale guards",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        fixture.ResourceState([](auto& state){state.economy.at(1).credits=1000;state.economy.at(1).pending_debit=11;});
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            const auto former=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,1);
            const auto cost=game->GetSmacProductionCost(base->m_owner,scout);
            base->SetProductionState(GSE_CALL,ProductionState{scout,cost/2,former,7,std::vector<ProductionItem>{former}});
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();const auto food=base->GetAccumulatedNutrients();
            auto* preview=static_cast<value::Object*>(base->CopySmacHurryPreview(GSE_CALL,0));
            const auto quote=static_cast<value::Int*>(preview->Get("quote"))->value;
            GT_ASSERT(static_cast<value::Bool*>(preview->Get("available"))->value && static_cast<value::Bool*>(preview->Get("can_full"))->value && fixture.Snapshot()==before,"Hurry preview mutated or lost actual budget");
            value::object_properties_t command{{"base",base->Wrap(GSE_CALL)},{"expected_state",preview->Get("expected_state")},{"full",VALUE(value::Bool,,true)},{"amount",VALUE(value::Int,,0)}};
            for(const auto amount:{int64_t(-1),int64_t(1),int64_t(INT32_MAX)+1}) {auto bad=command;bad["amount"]=VALUE(value::Int,,amount);GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"hurry_smac_production",bad);}) && fixture.Snapshot()==before,"invalid full credit amount was admitted");}
            game->Event(GSE_CALL,"hurry_smac_production",command);game->Event(GSE_CALL,"hurry_smac_production",command);
            GT_ASSERT(fixture.Snapshot()==before,"Hurry applied before dispatch");fixture.DrainEvents();
            const auto& paid=*base->GetProductionState();const auto money=game->GetCanonicalEconomy(base->m_owner);
            GT_ASSERT(paid.current_item==scout && paid.minerals==cost && paid.baseline_item==scout && paid.baseline_minerals==cost && !paid.hurry_partial_paid
                && paid.queue==std::optional<std::vector<ProductionItem>>({former}) && money.credits==1000-quote && money.pending_debit==11
                && base->GetAccumulatedNutrients()==food && fixture.GameplayRandom()==random,"full Hurry changed queue/food/RNG, set partial marker or debited twice");
            const auto unchanged=fixture.Snapshot();auto current=base->GetSmacHurryOffer();command["expected_state"]=VALUE(value::String,,current.expected);
            GT_ASSERT(!current.reason.empty() && Rejects([&]{game->Event(GSE_CALL,"hurry_smac_production",command);}) && fixture.Snapshot()==unchanged,"funded production was offered another ordinary purchase");GT_OK();
        });
    });
    task->AddTest("SMAC live Hurry partial zero tiny clamped and manual-switch marker survive real Start and owner reset",[](GSE*,context::Context*)->std::string {
        for(const int entered:{0,1,5,100}) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
            const auto roles=Roles();std::array<std::string,8> names;names[0]=roles.worker;for(size_t i=0;i<7;++i) {names[i+1]=roles.specialists[i];}fixture.BasePresentationRoles(names);
            fixture.ResourceState([&](auto& state){state.economy.at(1).credits=entered==100 ? 12 : 1000;state.economy.at(1).pending_debit=10;auto& tech=state.technology.factions[0];tech.target=5;tech.cached_threshold=100000;tech.labs=0;});
            std::string saved;int32_t expected_credits=0;
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
                const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);const auto cost=game->GetSmacProductionCost(base->m_owner,scout);
                base->SetProductionState(GSE_CALL,ProductionState{scout,cost/2,scout,7,std::vector<ProductionItem>{}});
                const auto offer=base->GetSmacHurryOffer();const auto quote=CalculateSmacHurryQuote(*base->GetProductionState(),*offer.context);
                const auto amount=std::min(entered,quote);const auto paid=std::min(amount,offer.credits-offer.reserved);const auto expected=cost/2+(cost-cost/2)*paid/quote;
                const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
                auto* preview=static_cast<value::Object*>(base->CopySmacHurryPreview(GSE_CALL,amount));
                GT_ASSERT(static_cast<value::Int*>(preview->Get("paid"))->value==paid && static_cast<value::Int*>(preview->Get("minerals_after"))->value==expected && fixture.Snapshot()==before,"partial private preview posted stores or used a different cost fraction");
                value::object_properties_t command{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,offer.expected)},{"full",VALUE(value::Bool,,false)},{"amount",VALUE(value::Int,,amount)}};
                auto bad=command;bad["amount"]=VALUE(value::Int,,int64_t(quote)+1);GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"hurry_smac_production",bad);}) && fixture.Snapshot()==before,"over-quote port input admission mutated state");
                game->Event(GSE_CALL,"hurry_smac_production",command);game->Event(GSE_CALL,"hurry_smac_production",command);fixture.DrainEvents();
                expected_credits=offer.credits-paid;
                GT_ASSERT(base->GetProductionState()->hurry_partial_paid && base->GetProductionState()->minerals==expected && base->GetProductionState()->baseline_minerals==expected
                    && game->GetCanonicalEconomy(base->m_owner).credits==expected_credits && game->GetCanonicalEconomy(base->m_owner).pending_debit==10 && fixture.GameplayRandom()==random,"partial Hurry lost zero/tiny marker, baseline or payment mode");
                auto* choices=static_cast<value::Object*>(base->CopySmacProductionChoices(GSE_CALL));value::Object* selected=nullptr;
                for(auto* row:static_cast<value::Array*>(choices->Get("choices"))->value) {auto* choice=static_cast<value::Object*>(row);if(static_cast<value::Int*>(choice->Get("source_id"))->value==1) {selected=choice;}}
                GT_ASSERT(selected,"Former is not eligible in partial marker switch fixture");
                game->Event(GSE_CALL,"select_smac_production",{{"base",base->Wrap(GSE_CALL)},{"source_id",VALUE(value::Int,,1)},{"expected_state",choices->Get("expected_state")},{"expected_minerals",selected->Get("minerals")},{"confirm",VALUE(value::Bool,,true)}});fixture.DrainEvents();
                auto* wrapper=static_cast<value::Object*>(base->Wrap(GSE_CALL));
                Call(GSE_CALL,wrapper,"set_production_state",{Call(GSE_CALL,wrapper,"get_production_state")});
                GT_ASSERT(base->GetProductionState()->hurry_partial_paid,"legacy state DTO roundtrip cleared the independent paid marker");
                const auto blocked=base->GetSmacHurryOffer();command["expected_state"]=VALUE(value::String,,blocked.expected);command["amount"]=VALUE(value::Int,,0);command["full"]=VALUE(value::Bool,,true);
                const auto unchanged=fixture.Snapshot();GT_ASSERT(base->GetProductionState()->hurry_partial_paid && base->GetProductionState()->current_item.smac_source_id==1
                    && !blocked.reason.empty() && Rejects([&]{game->Event(GSE_CALL,"hurry_smac_production",command);}) && fixture.Snapshot()==unchanged,"manual target change cleared partial lock or admitted repeat payment");
                saved=fixture.Snapshot();GT_OK();
            });GT_ASSERT(result.empty(),"partial live Hurry: "+result);
            ConventionalUnitFixture restored;restored.ConfigureProduction(TanksSource,"GAIANS");restored.BasePresentationRoles(names);restored.ProductionPresentations({"choice-scout","choice-rover","choice-former","choice-colony"});
            const auto loaded=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.Game()->GetState()->m_slots->GetSlot(0).GetPlayer()->SetDifficultyLevel("Librarian");restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
                auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
                GT_ASSERT(base->GetProductionState()->hurry_partial_paid && game->GetCanonicalEconomy(base->m_owner).credits==expected_credits
                    && !base->GetSmacHurryOffer().reason.empty(),"real Start replayed payment or lost paid marker");
                game->Event(GSE_CALL,"complete_turn",{});GT_OK();
            });GT_ASSERT(loaded.empty(),"partial saved Start: "+loaded);restored.DrainEvents();
            const auto tail=restored.Run([&](GSE_CALLABLE)->std::string {auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
                GT_ASSERT(game->GetTurnId()==2 && !base->GetProductionState()->hurry_partial_paid && game->GetCanonicalEconomy(base->m_owner).pending_debit==0,"owner tail omitted marker reset or reservation settlement");GT_OK();
            });GT_ASSERT(tail.empty(),"partial owner reset: "+tail);
        }GT_OK();
    });
    task->AddTest("SMAC live Hurry fences changed treasury without touching Base",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        fixture.ResourceState([](auto& state){state.economy.at(1).credits=1000;});
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);base->SetProductionState(GSE_CALL,ProductionState{scout,1,scout,1,std::vector<ProductionItem>{}});
            const auto offer=base->GetSmacHurryOffer();fixture.ResourceState([](auto& state){--state.economy.at(1).credits;});const auto before=fixture.Snapshot();
            game->Event(GSE_CALL,"hurry_smac_production",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,offer.expected)},{"full",VALUE(value::Bool,,true)},{"amount",VALUE(value::Int,,0)}});fixture.DrainEvents();
            GT_ASSERT(fixture.Snapshot()==before,"old quote overwrote changed treasury");GT_OK();
        });
    });

    task->AddTest("SMAC live Hurry withholds foreign authority",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.Configure(true,3,false,true);fixture.ResourcePolicy(true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,false,2,2);const auto before=fixture.Snapshot();
            GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_hurry_preview")->type==VT_UNDEFINED
                && Rejects([&]{game->Event(GSE_CALL,"hurry_smac_production",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,"")},{"full",VALUE(value::Bool,,true)},{"amount",VALUE(value::Int,,0)}});})
                && fixture.Snapshot()==before,"foreign Hurry exposed a command or changed state");GT_OK();
        });
    });

    task->AddTest("SMAC queue editor authenticates atomic ordered duplicates stale commands and actual saved upkeep",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(SelectionSource,"GAIANS");std::string saved;
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,false,0,1,2);SelectionPresentations(fixture);
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            const auto former=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,1);
            base->SetProductionState(GSE_CALL,ProductionState{scout,game->GetSmacProductionCost(base->m_owner,scout)-1,scout,7,std::vector<ProductionItem>{}});
            const auto original=*base->GetProductionState();const auto random=fixture.GameplayRandom();const auto before=fixture.Snapshot();
            auto* view=static_cast<value::Object*>(base->CopySmacQueueEditor(GSE_CALL));
            GT_ASSERT(fixture.Snapshot()==before && static_cast<value::Int*>(view->Get("limit"))->value==9,"queue query mutated or changed port budget");
            const auto entry=[&](const char* key,int64_t number)->Value* {return VALUEEXT(value::Object,GSE_CALL,(value::object_properties_t{{key,VALUE(value::Int,,number)}}));};
            auto expected=base->SmacSpecialistSelectionState();
            const auto command=[&](value::array_elements_t entries) {game->Event(GSE_CALL,"edit_smac_queue",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,expected)},{"entries",VALUE(value::Array,,entries)}});};
            command({entry("source_id",2),entry("source_id",1),entry("source_id",2)});
            GT_ASSERT(*base->GetProductionState()==original,"queue edit applied before dispatch");fixture.DrainEvents();
            GT_ASSERT(base->GetProductionState()->queue==std::optional<std::vector<ProductionItem>>({scout,former,scout})
                && base->GetProductionState()->current_item==scout && base->GetProductionState()->minerals==original.minerals
                && base->GetProductionState()->baseline_minerals==7 && fixture.GameplayRandom()==random,"queue edit lost duplicates or changed production/RNG");
            auto unchanged=fixture.Snapshot();command({});fixture.DrainEvents();GT_ASSERT(fixture.Snapshot()==unchanged,"stale queue command changed saved state");
            expected=base->SmacSpecialistSelectionState();
            for(const auto entries:std::vector<value::array_elements_t>{{entry("existing",0),entry("existing",0)},{entry("existing",3)},{entry("source_id",511)},{entry("bad",0)}}) {
                GT_ASSERT(Rejects([&]{command(entries);}) && fixture.Snapshot()==unchanged,"invalid queue token published state");
            }
            value::array_elements_t too_many;for(size_t i=0;i<10;++i) {too_many.push_back(entry("source_id",2));}
            GT_ASSERT(Rejects([&]{command(too_many);}) && fixture.Snapshot()==unchanged,"oversized queue was admitted");
            command({entry("existing",2),entry("existing",0),entry("existing",1)});fixture.DrainEvents();
            GT_ASSERT(*base->GetProductionState()->queue==std::vector<ProductionItem>({scout,scout,former}),"queue reorder changed retained identities");
            saved=fixture.Snapshot();GT_OK();
        });GT_ASSERT(result.empty(),"queue edit: "+result);
        ConventionalUnitFixture restored;restored.ConfigureProduction(SelectionSource,"GAIANS");restored.ProductionPresentations({"choice-scout","choice-rover","choice-former","choice-colony"});
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.Restore(GSE_CALL,saved,true);auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
            GT_ASSERT(base->GetProductionState()->queue->size()==3,"actual save lost pending queue");
            const auto result=base->ProcessSmacUpkeep(GSE_CALL,Roles(),[&](int32_t id){return game->ResolveSmacProductionPresentation(base->m_owner,id);},unit::SmacMoraleMode::Mode0);
            GT_ASSERT(result.completed_unit && base->GetProductionState()->current_item.smac_source_id==2
                && base->GetProductionState()->queue->size()==2 && base->GetProductionState()->queue->back().smac_source_id==1,"saved queue did not advance once through real completion");GT_OK();
        });
    });

    task->AddTest("SMAC queue editor preserves unavailable retained items without copying them",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);
            auto* bm=game->GetBM();bm->DefineFacility(new FacilityDef("tanks","Tanks","",40,{},3));base->m_facilities.emplace("tanks",bm->GetFacilityDef("tanks"));
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            const auto tanks=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Facility,3);
            base->SetProductionState(GSE_CALL,ProductionState{scout,2,scout,7,std::vector<ProductionItem>{tanks,scout}});
            auto* view=static_cast<value::Object*>(base->CopySmacQueueEditor(GSE_CALL));
            GT_ASSERT(static_cast<value::Array*>(view->Get("items"))->value.size()==2,"unavailable retained facility disappeared from editor");
            const auto entry=[&](const char* key,int64_t id)->Value* {return VALUEEXT(value::Object,GSE_CALL,(value::object_properties_t{{key,VALUE(value::Int,,id)}}));};
            const auto command=[&](value::array_elements_t entries) {game->Event(GSE_CALL,"edit_smac_queue",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())},{"entries",VALUE(value::Array,,entries)}});};
            const auto before=fixture.Snapshot();GT_ASSERT(Rejects([&]{command({entry("source_id",-3)});}) && fixture.Snapshot()==before,"existing facility was admitted as a new pending choice");
            command({entry("existing",1),entry("existing",0)});fixture.DrainEvents();
            GT_ASSERT(*base->GetProductionState()->queue==std::vector<ProductionItem>({scout,tanks}),"unavailable retained entry could not move");
            command({});fixture.DrainEvents();GT_ASSERT(base->GetProductionState()->queue->empty() && base->GetProductionState()->minerals==2 && base->GetProductionState()->baseline_minerals==7,"queue clear changed mineral authority");GT_OK();
        });
    });
    task->AddTest("SMAC queue editor rejects foreign Base control",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.Configure(true,3,false,true);fixture.ResourcePolicy(true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,false,2,2);const auto before=fixture.Snapshot();
            GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_queue_editor")->type==VT_UNDEFINED
                && Rejects([&]{game->Event(GSE_CALL,"edit_smac_queue",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())},{"entries",VALUE(value::Array,,value::array_elements_t{})}});})
                && fixture.Snapshot()==before,"foreign queue admitted authority or mutation");GT_OK();
        });
    });

    task->AddTest("SMAC Base Rename accepts empty duplicate spaced and byte bounded names through live requests",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i) {roles[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(roles);
        int64_t request=0;std::string expected;
        const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);base->SetProductionState(GSE_CALL,ProductionState{scout,2,scout,2,std::vector<ProductionItem>{}});
            Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),0));
            auto* second=new Base(game,702,base->m_owner,base->m_faction,game->GetMap()->GetTile(12,2),"Duplicate",{},pops,20,0,ProductionState{scout,2,scout,2,std::vector<ProductionItem>{}},SmacBaseResourceContext{1,false,0,0,0,false});
            bm->SpawnBase(GSE_CALL,second);SmacBaseAllocationState allocation{1,{{},1}};allocation.specialists.types[0]=1;second->PublishSmacAllocation(GSE_CALL,allocation,Roles());
            auto knowledge=KnowledgeValue(true,true,128,64).CopyData();for(auto& tile:knowledge.tiles) {tile.explored_by=2;tile.claimant=1;}game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
            base->SetAccumulatedNutrients(-3);GT_OK();
        });GT_ASSERT(setup.empty(),"Rename setup: "+setup);
        const auto open=[&]()->std::string {
            auto result=fixture.Run([&](GSE_CALLABLE)->std::string {auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);
                game->Event(GSE_CALL,"open_smac_rename",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())}});GT_OK();
            });if(!result.empty()) {return result;}fixture.DrainEvents();
            return fixture.Run([&](GSE_CALLABLE)->std::string {auto* base=fixture.Game()->GetBM()->GetBase(701);auto* menu=static_cast<value::Object*>(base->CopySmacRenameChoices(GSE_CALL));
                request=static_cast<value::Int*>(menu->Get("request_id"))->value;expected=static_cast<value::String*>(menu->Get("expected_state"))->value;
                GT_ASSERT(base->GetAccumulatedNutrients()>=0 && static_cast<value::String*>(menu->Get("name"))->value==base->m_name,"Rename did not retain prelude/default text");GT_OK();
            });
        };
        const auto answer=[&](const std::string& event,const std::string& text)->std::string {
            auto result=fixture.Run([&](GSE_CALLABLE)->std::string {auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);
                value::object_properties_t args{{"base",base->Wrap(GSE_CALL)},{"request_id",VALUE(value::Int,,request)},{"expected_state",VALUE(value::String,,expected)}};
                if(event=="commit_smac_rename") {args.emplace("name",VALUE(value::String,,text));}game->Event(GSE_CALL,event,args);game->Event(GSE_CALL,event,args);GT_OK();
            });if(!result.empty()) {return result;}fixture.DrainEvents();return {};
        };
        std::vector<std::pair<std::string,std::string>> roundtrips;
        const auto random=fixture.GameplayRandom();
        for(const std::string text:{std::string(""),std::string("   "),std::string("Duplicate"),std::string(" duplicate "),std::string("12345678901234567890123456789"),std::string("NUL\0ignored",11)}) {
            const auto opened=open();GT_ASSERT(opened.empty(),"Rename open: "+opened);const auto committed=answer("commit_smac_rename",text);GT_ASSERT(committed.empty(),"Rename commit: "+committed);
            const auto checked=fixture.Run([&](GSE_CALLABLE)->std::string {auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);const auto accepted=text.substr(0,std::min(size_t(24),text.find('\0')));
                GT_ASSERT(base->m_name==accepted && game->GetBM()->GetBase(702)->m_name=="Duplicate" && game->GetBM()->GetBaseRecordOrder()==std::vector<size_t>({701,702}) && fixture.GameplayRandom()==random,"Rename trimmed/coerced text, changed neighbor/order or drew RNG");roundtrips.emplace_back(accepted,fixture.Snapshot());GT_OK();
            });GT_ASSERT(checked.empty(),"Rename check: "+checked);
        }
        const auto invalid_open=open();GT_ASSERT(invalid_open.empty(),"Rename invalid open: "+invalid_open);
        const auto invalid=fixture.Run([&](GSE_CALLABLE)->std::string {auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);const auto before=fixture.Snapshot();
            for(const auto text:{std::string("high")+char(0x80),std::string("control\t")}) {
                GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"commit_smac_rename",{{"base",base->Wrap(GSE_CALL)},{"request_id",VALUE(value::Int,,request)},{"expected_state",VALUE(value::String,,expected)},{"name",VALUE(value::String,,text)}});}) && fixture.Snapshot()==before,"Unsupported display bytes published or changed state");
            }GT_OK();
        });GT_ASSERT(invalid.empty(),"Rename unsupported text: "+invalid);
        const auto opened=open();GT_ASSERT(opened.empty(),"Rename cancel open: "+opened);auto unchanged=fixture.Snapshot();
        const auto cancelled=answer("cancel_smac_rename","");GT_ASSERT(cancelled.empty(),"Rename cancel: "+cancelled);
        const auto replay=answer("commit_smac_rename","replay");GT_ASSERT(replay.empty(),"Rename replay: "+replay);GT_ASSERT(fixture.Snapshot()==unchanged,"Cancel or old request replay changed saved state");
        const auto stale_open=open();GT_ASSERT(stale_open.empty(),"Rename stale open: "+stale_open);
        const auto mutate=fixture.Run([&](GSE_CALLABLE)->std::string {fixture.Game()->GetBM()->GetBase(701)->SetAccumulatedNutrients(9);GT_OK();});GT_ASSERT(mutate.empty(),"Rename intervening mutation: "+mutate);unchanged=fixture.Snapshot();
        const auto stale=answer("commit_smac_rename","stale");GT_ASSERT(stale.empty(),"Rename stale answer: "+stale);GT_ASSERT(fixture.Snapshot()==unchanged,"Stale Rename overwrote changed Base");
        for(const auto& saved:roundtrips) {
            ConventionalUnitFixture restored;restored.ConfigureProduction(TanksSource,"GAIANS");restored.BasePresentationRoles(roles);
            const auto result=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.Restore(GSE_CALL,saved.second,true);auto* bm=restored.Game()->GetBM();
                GT_ASSERT(bm->GetBase(701)->m_name==saved.first && bm->GetBase(702)->m_name=="Duplicate" && bm->GetBaseRecordOrder()==std::vector<size_t>({701,702}),"Actual Base codec coerced duplicate/empty/byte-bounded names");GT_OK();
            });GT_ASSERT(result.empty(),"Rename codec restore: "+result);
        }GT_OK();
    });

    task->AddTest("SMAC Base Rename withholds foreign authority",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.Configure(true,3,false,true);fixture.ResourcePolicy(true);
        std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i) {roles[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(roles);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,false,2,2);const auto before=fixture.Snapshot();
            GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_rename_state")->type==VT_UNDEFINED && Rejects([&]{game->Event(GSE_CALL,"open_smac_rename",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())}});}) && fixture.Snapshot()==before,"Foreign Rename exposed authority or changed state");GT_OK();
        });
    });

    task->AddTest("SMAC Base Rename withholds captured AI consequences until saved counter authority exists",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.Configure(true,3,false,true);fixture.ResourcePolicy(true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* own=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();auto context=own->GetSmacResourceContext();context.conquest=SmacConquestState{2,50};
            Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),0));
            auto* base=new Base(game,702,own->m_owner,own->m_faction,game->GetMap()->GetTile(6,4),"Captured saved history",{},pops,20,0,Production(),context);
            bm->SpawnBase(GSE_CALL,base);SmacBaseAllocationState allocation{1,{{},1}};allocation.specialists.types[0]=1;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
            fixture.ResourceState([](auto& state){state.human_membership=2;});const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            GT_ASSERT(!base->CanSmacRename() && Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_rename_state")->type==VT_UNDEFINED && Rejects([&]{game->Event(GSE_CALL,"open_smac_rename",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())}});}) && fixture.Snapshot()==before && fixture.GameplayRandom()==random,"Captured AI Rename fabricated diplomacy, changed RNG or exposed command");
            fixture.ResourceState([](auto& state){state.human_membership=6;});
            GT_ASSERT(base->CanSmacRename(),"Known human former owner was treated as captured AI");GT_OK();
        });
    });

    task->AddTest("SMAC manual specialist foreign Base cannot expose or accept commands",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.Configure(true,3,false,true);fixture.ResourcePolicy(true);
        std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i) {roles[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(roles);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,false,2,2);
            const auto before=fixture.Snapshot();
            GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_specialist_selection_state")->type==VT_UNDEFINED,"Foreign Base exposed manual specialist authority");
            for(const auto* event:{"open_smac_specialist","select_smac_specialist","cancel_smac_specialist"}) {
                value::object_properties_t args{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,19)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())}};
                if(std::string(event)!="open_smac_specialist") {args.emplace("request_id",VALUE(value::Int,,1));}
                if(std::string(event)=="select_smac_specialist") {args.emplace("type",VALUE(value::Int,,1));}
                GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,event,args);}) && fixture.Snapshot()==before,"Foreign specialist command changed gameplay");
            }GT_OK();
        });
    });

    task->AddTest("SMAC manual specialist Worker drops last slot without shifting and recalculates real output",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(ManualSpecialistSource,"GAIANS");
        std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i) {roles[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(roles);
        const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,true,false,0,1,5);
            for(size_t y=0;y<8;++y) {for(size_t x=y&1;x<10;x+=2) {game->GetMap()->GetTile(x,y)->terraforming=TERRAFORMING_FOREST;}}
            auto knowledge=KnowledgeValue(true,true,128,64).CopyData();for(auto& tile:knowledge.tiles) {tile.explored_by=2;tile.claimant=1;}game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
            SmacBaseAllocationState allocation{15,{{},2}};allocation.specialists.types[0]=3;allocation.specialists.types[1]=1;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);base->SetProductionState(GSE_CALL,ProductionState{scout,2,scout,2,std::vector<ProductionItem>{}});
            auto context=base->GetSmacResourceContext();context.governor_options=0;base->SetSmacResourceContext(GSE_CALL,context);
            game->Event(GSE_CALL,"open_smac_specialist",{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,22)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())}});GT_OK();
        });GT_ASSERT(setup.empty(),"setup: "+setup);fixture.DrainEvents();
        const auto choose=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);auto* menu=static_cast<value::Object*>(base->CopySmacSpecialistChoices(GSE_CALL,22));
            GT_ASSERT(static_cast<value::Int*>(menu->Get("current_type"))->value==3,"Original Engineer slot lost before menu");
            game->Event(GSE_CALL,"select_smac_specialist",{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,22)},{"request_id",menu->Get("request_id")},{"expected_state",menu->Get("expected_state")},{"type",VALUE(value::Int,,-1)}});GT_OK();
        });GT_ASSERT(choose.empty(),"choose: "+choose);fixture.DrainEvents();
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);const auto allocation=*base->GetSmacAllocation();
            GT_ASSERT(allocation.specialists.total_count==1 && allocation.specialists.types[0]==3 && allocation.specialists.types[1]==1 && base->m_pops.size()==5,"Worker result count/types/size: "+std::to_string(allocation.specialists.total_count)+"/"+std::to_string(allocation.specialists.types[0])+"/"+std::to_string(allocation.specialists.types[1])+"/"+std::to_string(base->m_pops.size()));
            const auto energy=game->GetCanonicalBaseEnergy(*base);const auto specialist=energy.allocation.after_specialists;const auto ordinary=energy.allocation.allocation;
            GT_ASSERT(specialist.economy-ordinary.economy==3 && specialist.psych-ordinary.psych==0 && specialist.labs-ordinary.labs==2,"Remaining Engineer did not feed actual E/P/L calculation");GT_OK();
        });
    });

    task->AddTest("SMAC manual specialist creation with sixteen stored types is explicitly unavailable",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(ManualSpecialistSource,"GAIANS");
        std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i) {roles[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(roles);
        const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* seed=Populate(GSE_CALL,fixture,true);auto* bm=game->GetBM();

            Base::pops_t pops;for(size_t i=0;i<17;++i) {pops.emplace(std::piecewise_construct,std::forward_as_tuple(19+i),std::forward_as_tuple(nullptr,19+i,bm->GetPopDef(Roles().specialists[1]),0));}
            const auto scout=game->MakeSmacProductionItem(seed->m_owner,ProductionItemKind::Unit,2);
            auto* base=new Base(game,702,seed->m_owner,seed->m_faction,game->GetMap()->GetTile(12,2),"Sixteen stored",{},pops,36,13,ProductionState{scout,2,scout,2,std::vector<ProductionItem>{}},SmacBaseResourceContext{1,false,0,0,0,false});
            bm->SpawnBase(GSE_CALL,base);SmacBaseAllocationState allocation{3,{{},16}};allocation.specialists.types.fill(1);base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
            auto knowledge=KnowledgeValue(true,true,128,64).CopyData();for(auto& tile:knowledge.tiles) {tile.explored_by=2;tile.claimant=1;}game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
            game->Event(GSE_CALL,"open_smac_specialist",{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,19)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())}});GT_OK();
        });GT_ASSERT(setup.empty(),"setup: "+setup);fixture.DrainEvents();
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(702);auto* menu=static_cast<value::Object*>(base->CopySmacSpecialistChoices(GSE_CALL,19));const auto before=fixture.Snapshot();
            GT_ASSERT(static_cast<value::Bool*>(menu->Get("creation_limited"))->value && static_cast<value::Array*>(menu->Get("choices"))->value.size()==1,"Unimplemented live overflow was offered as a real choice");
            GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"select_smac_specialist",{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,19)},{"request_id",menu->Get("request_id")},{"expected_state",menu->Get("expected_state")},{"type",VALUE(value::Int,,1)}});}) && fixture.Snapshot()==before,"Unadmitted overflow mutated the Base");GT_OK();
        });
    });

    task->AddTest("SMAC manual specialist live commands preserve prelude cancel same stale and stable Pop identity",[](GSE*,context::Context*)->std::string {
        int menus=0;ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i) {roles[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(roles);
        int64_t request=0;std::string expected;Pop* identity=nullptr;std::string unchanged;
        const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);
            game->On(GSE_CALL,"smac_specialist_menu",NATIVE_CALL(&menus) {++menus;return VALUE(value::Undefined); }));
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,true,128,64));
            auto allocation=*base->GetSmacAllocation();allocation.selected=5;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            base->SetProductionState(GSE_CALL,ProductionState{scout,2,scout,2,std::vector<ProductionItem>{}});
            auto context=base->GetSmacResourceContext();context.governor_options=SmacBaseResourceContext::GOVERNOR_ACTIVE|SmacBaseResourceContext::MANAGE_CITIZENS|8;base->SetSmacResourceContext(GSE_CALL,context);
            base->SetAccumulatedNutrients(-3);identity=&base->m_pops.at(19);
            GT_OK();
        });GT_ASSERT(setup.empty(),"specialist stage: "+setup);
        const auto open=[&]()->std::string {
            auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);
                auto* state=Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_specialist_selection_state");
                game->Event(GSE_CALL,"open_smac_specialist",{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,19)},{"expected_state",state}});GT_OK();
            });if(!result.empty()) {return result;}fixture.DrainEvents();
            return fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* base=fixture.Game()->GetBM()->GetBase(701);auto* choices=static_cast<value::Object*>(base->CopySmacSpecialistChoices(GSE_CALL,19));
                request=static_cast<value::Int*>(choices->Get("request_id"))->value;expected=static_cast<value::String*>(choices->Get("expected_state"))->value;
                GT_ASSERT(menus>0 && request>0 && expected==base->SmacSpecialistSelectionState(),"Menu did not own its post-prelude snapshot/request");GT_OK();
            });
        };
        const auto answer=[&](const std::string& event,int32_t type,bool duplicate)->std::string {
            auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);
                value::object_properties_t args{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,19)},{"request_id",VALUE(value::Int,,request)},{"expected_state",VALUE(value::String,,expected)}};
                if(event=="select_smac_specialist") {args.emplace("type",VALUE(value::Int,,type));}
                game->Event(GSE_CALL,event,args);if(duplicate) {game->Event(GSE_CALL,event,args);}GT_OK();
            });if(!result.empty()) {return result;}fixture.DrainEvents();return {};
        };
        const auto first=open();GT_ASSERT(first.empty(),"specialist stage: "+first);
        const auto prelude=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* base=fixture.Game()->GetBM()->GetBase(701);
            GT_ASSERT(base->GetAccumulatedNutrients()==0 && base->m_pops.size()==1 && base->GetSmacResourceContext().ManageCitizens(),"Opening skipped normalization, posted growth or cleared management");
            unchanged=fixture.Snapshot();
            value::object_properties_t args{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,19)},{"request_id",VALUE(value::Int,,request)},{"expected_state",VALUE(value::String,,expected)},{"type",VALUE(value::Int,,3)}};
            GT_ASSERT(Rejects([&]{fixture.Game()->Event(GSE_CALL,"select_smac_specialist",args);}),"Ineligible specialist entered actual event queue");
            args.emplace("caller",VALUE(value::Int,,1));
            GT_ASSERT(Rejects([&]{fixture.Game()->Event(GSE_CALL,"select_smac_specialist",args);}),"Caller-supplied authority entered actual event queue");
            GT_ASSERT(fixture.Snapshot()==unchanged,"Rejected command changed world state");GT_OK();
        });GT_ASSERT(prelude.empty(),"specialist stage: "+prelude);
        const auto cancelled=answer("cancel_smac_specialist",0,true);GT_ASSERT(cancelled.empty(),"specialist stage: "+cancelled);
        GT_ASSERT(fixture.Snapshot()==unchanged,"Cancel changed gameplay after its completed prelude");
        // A cancelled request cannot be replayed even while its Base snapshot matches.
        const auto replay=answer("select_smac_specialist",1,true);GT_ASSERT(replay.empty(),"specialist stage: "+replay);GT_ASSERT(fixture.Snapshot()==unchanged,"Cancelled request replayed");
        const auto opened=open();GT_ASSERT(opened.empty(),"specialist stage: "+opened);
        const auto random=fixture.GameplayRandom();
        const auto selected=answer("select_smac_specialist",1,true);GT_ASSERT(selected.empty(),"specialist stage: "+selected);
        const auto doctor=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* base=fixture.Game()->GetBM()->GetBase(701);const auto allocation=*base->GetSmacAllocation();
            GT_ASSERT(allocation.selected==1 && allocation.specialists.total_count==1 && allocation.specialists.types[0]==1
                && &base->m_pops.at(19)==identity && identity->m_def->m_smac_specialist_id==1 && !identity->GetWorkedTileCoords(),"Worker-to-Doctor lost live allocation/Pop identity");
            GT_ASSERT(base->GetSmacResourceContext().governor_options==(SmacBaseResourceContext::GOVERNOR_ACTIVE|8)
                && base->GetAccumulatedNutrients()==0 && fixture.GameplayRandom()==random,"Choice cleared unrelated governor bits, posted food or changed RNG");GT_OK();
        });GT_ASSERT(doctor.empty(),"specialist stage: "+doctor);
        const auto same_open=open();GT_ASSERT(same_open.empty(),"specialist stage: "+same_open);unchanged=fixture.Snapshot();
        const auto same=answer("select_smac_specialist",1,true);GT_ASSERT(same.empty(),"specialist stage: "+same);GT_ASSERT(fixture.Snapshot()==unchanged,"Same type posted a second recalculation or mutated state");
        const auto worker_open=open();GT_ASSERT(worker_open.empty(),"specialist stage: "+worker_open);
        const auto worker=answer("select_smac_specialist",-1,true);GT_ASSERT(worker.empty(),"specialist stage: "+worker);
        const auto restored=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* base=fixture.Game()->GetBM()->GetBase(701);const auto allocation=*base->GetSmacAllocation();
            GT_ASSERT(allocation.specialists.total_count==0 && allocation.selected!=1 && &base->m_pops.at(19)==identity && identity->GetWorkedTileCoords(),"Worker choice did not refill an actual square with stable Pop");GT_OK();
        });GT_ASSERT(restored.empty(),"specialist stage: "+restored);
        const auto stale_open=open();GT_ASSERT(stale_open.empty(),"specialist stage: "+stale_open);
        const auto edited=fixture.Run([&](GSE_CALLABLE)->std::string {fixture.Game()->GetBM()->GetBase(701)->SetAccumulatedNutrients(7);GT_OK();});GT_ASSERT(edited.empty(),"specialist stage: "+edited);unchanged=fixture.Snapshot();
        const auto stale=answer("select_smac_specialist",1,true);GT_ASSERT(stale.empty(),"specialist stage: "+stale);GT_ASSERT(fixture.Snapshot()==unchanged,"Stale menu overwrote changed Base state");
        GT_OK();
    });

    task->AddTest("SMAC Base navigation follows saved owner order and rereads removal without world mutation",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.Configure(true,3,false,true);fixture.ResourcePolicy(true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* first=Populate(GSE_CALL,fixture,true);auto* bm=game->GetBM();
            const auto query=[&](Base* base){return Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_base_navigation");};
            GT_ASSERT(query(first)->type==VT_UNDEFINED,"Singleton Base invented a neighbor");
            const auto add=[&](size_t id,size_t slot,int32_t simulation,size_t x,size_t y)->Base* {
                auto* owner=&game->GetState()->m_slots->GetSlot(slot);
                Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),0));
                auto* base=new Base(game,id,owner,owner->GetPlayer()->GetFaction(),game->GetMap()->GetTile(x,y),"Navigation "+std::to_string(id),{},pops,20,0,Production(),SmacBaseResourceContext{simulation,false,0,0,0,false});
                bm->SpawnBase(GSE_CALL,base);SmacBaseAllocationState allocation{1,{{},1}};allocation.specialists.types[0]=1;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());return base;
            };
            auto* second=add(902,0,1,6,2);auto* foreign=add(65,2,2,2,6);auto* third=add(13,0,1,6,6);
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            const auto matches=[&](Base* base,Base* previous,Base* next) {
                auto* view=static_cast<value::Object*>(query(base));
                return Base::Unwrap(view->Get("previous"))==previous && Base::Unwrap(view->Get("next"))==next;
            };
            GT_ASSERT(matches(first,third,second) && matches(second,first,third) && matches(third,second,first),"Navigation used numeric IDs or included a foreign Base");
            GT_ASSERT(query(foreign)->type==VT_UNDEFINED && fixture.Snapshot()==before && fixture.GameplayRandom()==random,"Navigation leaked foreign roster or changed world/RNG");
            bm->DespawnBase(GSE_CALL,second->m_id);
            GT_ASSERT(matches(first,third,third) && matches(third,first,first),"Navigation retained removed Base identity");
            GT_OK();
        });
    });

    task->AddTest("SMAC nutrient grid helper matches original filled and prospective overlay controls",[](GSE*,context::Context*)->std::string {
        struct Case {int64_t food;int32_t capacity,surplus;std::vector<int32_t> filled,overlay;};
        const Case cases[]{
            {0,20,2,{},{0,1}},
            {19,20,1,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18},{19}},
            {18,20,2,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17},{18,19}},
            {20,20,2,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},{}},
            {21,20,2,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},{}},
            {15,20,2,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14},{15,16}},
            {5,20,0,{0,1,2,3,4},{}},
            {20,20,0,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},{}},
            {5,20,-2,{0,1,2,3,4},{3,4}},
            {0,20,-2,{},{}},
            {20,20,-2,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19},{18,19}},
            {-1,20,-2,{},{}},
            {78,80,2,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77},{78,79}},
            {80,80,2,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79},{}},
            {80,80,2,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23,24,25,26,27,28,29,30,31,32,33,34,35,36,37,38,39,40,41,42,43,44,45,46,47,48,49,50,51,52,53,54,55,56,57,58,59,60,61,62,63,64,65,66,67,68,69,70,71,72,73,74,75,76,77,78,79},{}},
            {0,24,2,{},{0,1}},
            {24,24,2,{0,1,2,3,4,5,6,7,8,9,10,11,12,13,14,15,16,17,18,19,20,21,22,23},{}},
            {0,10,2,{},{0,1}},
            {0,24,2,{},{0,1}},
            {0,10,2,{},{0,1}},
            {0,10,0,{},{}},
            {0,10,2,{},{0,1}},
        };
        for(const auto& c:cases) {
            const auto result=::ui::ResourceProgress::Format(c.surplus,0,c.food,c.capacity);
            std::vector<int32_t> filled,overlay;
            for(int32_t i=0;i<c.capacity;++i) {
                if(i<result.filled_cells) {filled.push_back(i);if(result.direction<0 && i>=result.end_cells) {overlay.push_back(i);}}
                else if(i<result.end_cells) {overlay.push_back(i);}
            }
            GT_ASSERT(filled==c.filled && overlay==c.overlay,"Original nutrient filled/overlay cells differ");
        }
        GT_OK();
    });

    task->AddTest("SMAC nutrient display matches original label controls independently of population outcomes",[](GSE*,context::Context*)->std::string {
        struct Case {int64_t food;int32_t capacity,surplus;const char* label;};
        const Case cases[]{
            {0,20,2,"Growth: 11 turns"},
            {19,20,1,"Growth: 2 turns"},
            {18,20,2,"Growth: 2 turns"},
            {20,20,2,"Growth: 1 turn"},
            {21,20,2,"Growth: 1 turn"},
            {15,20,2,"Growth: 4 turns"},
            {5,20,0,"Growth: -stagnant-"},
            {20,20,0,"Growth: 1 turn"},
            {5,20,-2,"Hunger!"},
            {0,20,-2,"Hunger!"},
            {20,20,-2,"Hunger: 1 turn"},
            {-1,20,-2,"Hunger!"},
            {78,80,2,"Growth: 2 turns"},
            {80,80,2,"Growth: 1 turn"},
            {80,80,2,"Growth: 1 turn"},
            {0,24,2,"Growth: 13 turns"},
            {24,24,2,"Growth: 1 turn"},
            {0,10,2,"Growth: 6 turns"},
            {0,24,2,"Growth: 13 turns"},
            {0,10,2,"Growth: 6 turns"},
            {0,10,0,"Growth: -stagnant-"},
            {0,20000,1,"Growth: 9999 turns"},
            {0,0,2,"Growth: 1 turn"},
            {0,10,2,"Growth: 6 turns"},
            {INT64_MIN,20,1,"Growth: 9999 turns"},
            {INT64_MAX,20,-1,"Hunger: 1 turn"},
        };
        for(const auto& c:cases) {GT_ASSERT(FormatSmacNutrientGrowthLabel(c.food,c.capacity,c.surplus)==c.label,"Original nutrient label differs");}
        GT_OK();
    });

    task->AddTest("SMAC Base nutrient presentation binds current box and native step without posting upkeep",[](GSE*,context::Context*)->std::string {
        struct Case {size_t population;bool doctor;int32_t growth,food;bool famine;int32_t threshold;const char* branch;int32_t result_population,surplus;bool recalculation;};
        const Case cases[]{
            {1,false,0,13,false,20,"collection",1,2,false},
            {1,false,0,20,false,20,"growth",2,2,true},
            {1,false,-3,24,false,24,"stagnation",1,2,false},
            {1,false,6,0,false,10,"boom",2,2,true},
            {7,true,0,80,false,80,"housing",7,-12,false},
            {2,true,0,0,false,30,"starvation",1,-2,false},
            {1,true,0,0,true,20,"removal",0,-1,false},
        };
        for(const auto& c:cases) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,c.doctor,false,0,1,c.population);
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,true,128,64));
                if(!c.doctor) {auto allocation=*base->GetSmacAllocation();allocation.selected=5;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());}
                fixture.ResourceState([&](auto& state){state.social.factions.front().current[6]=c.growth;});
                const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
                base->SetProductionState(GSE_CALL,ProductionState{scout,2,scout,2,std::vector<ProductionItem>{}});
                base->SetAccumulatedNutrients(c.food);
                if(c.famine) {auto context=base->GetSmacResourceContext();context.resource_events=SmacBaseResourceContext::Famine;base->SetSmacResourceContext(GSE_CALL,context);}
                const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
                for(int repeat=0;repeat<3;++repeat) {
                    auto* view=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_nutrient_presentation"));
                    const auto integer=[](value::Object* o,const std::string& key){return static_cast<value::Int*>(o->Get(key))->value;};
                    GT_ASSERT(integer(view,"stored")==c.food && integer(view,"rows")==int64_t(c.population+1)
                        && integer(view,"threshold")==c.threshold && integer(view,"surplus")==c.surplus,"Current box values: stored="+std::to_string(integer(view,"stored"))+" rows="+std::to_string(integer(view,"rows"))+" threshold="+std::to_string(integer(view,"threshold"))+" surplus="+std::to_string(integer(view,"surplus")));
                    auto* step=static_cast<value::Object*>(view->Get("next_step"));
                    GT_ASSERT(static_cast<value::String*>(step->Get("basis"))->value=="current_population_and_allocation"
                        && static_cast<value::String*>(step->Get("branch"))->value==c.branch
                        && integer(step,"population")==c.result_population
                        && static_cast<value::Bool*>(step->Get("requires_recalculation"))->value==c.recalculation,"Native nutrient step or its limited basis changed");
                }
                GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random && !base->GetSmacUpkeepCache(),"Nutrient presentation posted food/population, invented history or changed RNG");
                GT_OK();
            });if(!result.empty()) {return std::string(c.branch)+": "+result;}
        }
        GT_OK();
    });

    task->AddTest("SMAC Base support presentation withholds remote foreign roster before calculation",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.Configure(true,3,false,true);fixture.ResourcePolicy(true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* base=Populate(GSE_CALL,fixture,false,true,2,2);
            GT_ASSERT(base->m_owner->GetPlayer()!=fixture.Game()->GetPlayer(),"Fixture does not inspect a foreign Base");
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            auto* view=Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_support_presentation");
            GT_ASSERT(view->type==VT_UNDEFINED && fixture.Snapshot()==before && fixture.GameplayRandom()==random,
                "Foreign inspector exposed homed Unit identities/locations, ran the support provider or changed world/RNG");
            GT_OK();
        });
    });

    task->AddTest("SMAC Base support presentation preserves live home order and retained pass without mutation", [](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,true);
            fixture.ResourceState([](auto& state){state.social.factions.front().current[2]=-2;});
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,true,128,64));
            auto* wrap=static_cast<value::Object*>(base->Wrap(GSE_CALL));
            const auto query=[&]() {return static_cast<value::Object*>(Call(GSE_CALL,wrap,"get_smac_support_presentation"));};
            GT_ASSERT(query()->Get("retained")->type==VT_UNDEFINED,"Read invented a retained resource pass");
            base->RecalculateSmacWorkers(GSE_CALL,Roles(),SmacBaseResourcePhase::Upkeep);
            auto* um=game->GetUM();
            auto state=um->GetUnit(91)->Conventional();
            for(const size_t id:{33u,12u,9u}) {
                if(id==9) {state.home=game::backend::unit::ConventionalUnitState::Home::None;state.home_base_id.reset();}
                um->SpawnConventionalUnit(GSE_CALL,id,um->GetUnitDef("live-scout"),base->m_owner,game->GetMap()->GetTile(6,6),state);
            }
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            for(int repeat=0;repeat<3;++repeat) {
                auto* view=query();auto* current=static_cast<value::Object*>(view->Get("current"));
                auto* retained=static_cast<value::Object*>(view->Get("retained"));
                const auto integer=[](value::Object* o,const std::string& key){return static_cast<value::Int*>(o->Get(key))->value;};
                GT_ASSERT(integer(current,"eligible")==3 && integer(current,"paid")==2 && integer(current,"minerals")==2
                    && integer(current,"free_allowance")==1,"Preview totals do not use actual support provider");
                GT_ASSERT(integer(retained,"eligible")==1 && integer(retained,"minerals")==0,"Current query replaced historical support");
                const auto& rows=static_cast<value::Array*>(current->Get("units"))->value;
                const std::array<int64_t,3> expected_ids{91,33,12};
                GT_ASSERT(rows.size()==3,"No-home Unit leaked into this Base roster");
                for(size_t i=0;i<rows.size();++i) {
                    auto* row=static_cast<value::Object*>(rows[i]);
                    GT_ASSERT(integer(row,"id")==expected_ids[i] && integer(row,"home")==701
                        && integer(row,"x")==6 && integer(row,"y")==6,"Roster lost saved order, actual home or location");
                    GT_ASSERT(static_cast<value::String*>(row->Get("status"))->value==(i==0?"Free":"Paid")
                        && integer(row,"minerals")==int64_t(i==0?0:1),"Free/paid Unit association changed");
                }
            }
            GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random,"Support drawing changed world, history or RNG");
            auto removal=um->PrepareConventionalRemoval(GSE_CALL,{91});removal.Commit();removal.Notify(GSE_CALL);
            auto* after=query();
            auto* now=static_cast<value::Object*>(after->Get("current"));
            auto* history=static_cast<value::Object*>(after->Get("retained"));
            GT_ASSERT(!um->GetUnit(91) && static_cast<value::Array*>(now->Get("units"))->value.size()==2
                && static_cast<value::Int*>(history->Get("eligible"))->value==1,
                "Historical support resolved retired Unit IDs or lost its saved pass");
            GT_OK();
        });
    });


    task->AddTest("SMAC Base production presentation leaves unresolved legacy records unavailable", [](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* base=Populate(GSE_CALL,fixture,false);
            const auto before=fixture.Snapshot();
            auto* view=Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation");
            GT_ASSERT(view->type==VT_UNDEFINED && fixture.Snapshot()==before,"Unresolved production threw, invented a source identity or mutated the world");
            GT_OK();
        });
    });
    task->AddTest("SMAC Base production presentation does not require local control of foreign custom designs", [](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.Configure(true,3,false,true);fixture.ResourcePolicy(true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,false,2,2);
            const auto rover=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,128);
            base->SetProductionState(GSE_CALL,ProductionState{rover,7,rover,7,std::vector<ProductionItem>{}});
            const auto before=fixture.Snapshot();
            auto* view=Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation");
            GT_ASSERT(view->type==VT_UNDEFINED && fixture.Snapshot()==before,"Foreign custom presentation invoked local commands or changed the world");
            GT_OK();
        });
    });
    task->AddTest("SMAC Base production presentation reads current and complete saved queue without mutation", [](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            const auto tanks=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Facility,3);
            const auto stockpile=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
            const std::vector<ProductionItem> queue{tanks,scout,stockpile,tanks,scout,stockpile,tanks,scout,stockpile};
            base->SetProductionState(GSE_CALL,ProductionState{scout,17,scout,11,queue});
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            auto* view=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation"));
            const auto& rules=game->GetState()->m_settings.global;
            const auto& rows=static_cast<value::Array*>(view->Get("queue"))->value;
            GT_ASSERT(static_cast<value::String*>(view->Get("name"))->value==rules.GetSmacPredefinedUnitRules()->catalog.source.units.at(2).name
                && rows.size()==9 && static_cast<value::Int*>(view->Get("cost"))->value==game->GetSmacProductionCost(base->m_owner,scout),"Current identity, cost or full queue missing");
            for(size_t i=0;i<rows.size();++i) {
                const auto expected=i%3==0 ? rules.GetSmacProductionRules()->facilities.at(2).name
                    : i%3==1 ? rules.GetSmacPredefinedUnitRules()->catalog.source.units.at(2).name
                    : rules.GetSmacProductionRules()->facilities.at(37).name;
                GT_ASSERT(static_cast<value::String*>(rows[i])->value==expected,"Saved queue order or source names changed");
            }
            GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==random,"Read-only projection mutated stores, queue, world or gameplay RNG");
            base->SetProductionState(GSE_CALL,ProductionState{tanks,7,scout,888,queue});
            const auto prepared=fixture.Snapshot();
            auto* formatted=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation",{VALUE(value::Int,,4)}));
            GT_ASSERT(static_cast<value::Int*>(formatted->Get("cost"))->value==40
                && static_cast<value::String*>(formatted->Get("forecast"))->value=="TURNS:  9"
                && fixture.Snapshot()==prepared && fixture.GameplayRandom()==random,"forecast used switching baseline, wrong cost or mutated authority");
            auto* no_surplus=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation"));
            GT_ASSERT(no_surplus->value.count("forecast")==0,"missing current surplus invented a forecast");
            GT_ASSERT(Rejects([&]{Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation",{VALUE(value::Int,,int64_t(INT32_MAX)+1)});})
                && fixture.Snapshot()==prepared,"wide formatting surplus narrowed or changed authority");
            base->SetProductionState(GSE_CALL,ProductionState{tanks,INT64_MAX,scout,888,queue});
            const auto wide=fixture.Snapshot();
            auto* unavailable=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation",{VALUE(value::Int,,4)}));
            GT_ASSERT(unavailable->value.count("cost")==1 && unavailable->value.count("forecast")==0 && fixture.Snapshot()==wide,
                "wide port store crashed or fabricated an original forecast");
            base->SetProductionState(GSE_CALL,ProductionState{tanks,7,scout,888,queue});
            auto* bm=game->GetBM();bm->DefineFacility(new FacilityDef("progress-tanks","Tanks","",40,{},3));
            base->m_facilities.emplace("progress-tanks",bm->GetFacilityDef("progress-tanks"));
            const auto installed=fixture.Snapshot();
            auto* built=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation",{VALUE(value::Int,,4)}));
            GT_ASSERT(built->value.count("cost")==1 && built->value.count("forecast")==0 && fixture.Snapshot()==installed,
                "already installed facility entered the ordinary-unbuilt forecast branch");
            base->m_facilities.clear();bm->DefineFacility(new FacilityDef("progress-dome","Pressure Dome","",80,{},26));
            base->m_facilities.emplace("progress-dome",bm->GetFacilityDef("progress-dome"));
            const auto equivalent=fixture.Snapshot();
            auto* dome_blocked=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation",{VALUE(value::Int,,4)}));
            GT_ASSERT(dome_blocked->value.count("cost")==1 && dome_blocked->value.count("forecast")==0
                && fixture.Snapshot()==equivalent && fixture.GameplayRandom()==random,"Pressure Dome did not withhold the unbuildable Tanks forecast");
            const auto& designs=rules.GetSmacPredefinedUnitRules()->catalog.units;
            const auto native=std::find_if(designs.begin(),designs.end(),[](const auto& row){return row.source_id==2;});
            GT_ASSERT(native!=designs.end(),"missing actual Scout design");
            fixture.ResourceState([&](auto& state){state.designs.owners.front().designs.push_back({64,native->chassis_id,native->weapon_id,native->defense_id,native->reactor,
                native->cost,native->plan,native->carry,native->abilities,native->faction_exclusion,true,false});});
            const auto custom=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,64);
            base->SetProductionState(GSE_CALL,ProductionState{custom,7,custom,7,std::vector<ProductionItem>{custom,scout}});
            const auto unprototyped=fixture.Snapshot();
            auto* unsupported=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation",{VALUE(value::Int,,4)}));
            GT_ASSERT(unsupported->value.count("cost")==0 && unsupported->value.count("forecast")==0
                && static_cast<value::Array*>(unsupported->Get("queue"))->value.size()==2
                && !static_cast<value::String*>(unsupported->Get("name"))->value.empty()
                && fixture.Snapshot()==unprototyped && fixture.GameplayRandom()==random,"unavailable custom cost hid a retained target/queue or changed authority");GT_OK();
        });
    });

    task->AddTest("SMAC production cost display withholds foreign authority without hiding identity",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(TanksSource,true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,false,6,2);
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            base->SetProductionState(GSE_CALL,ProductionState{scout,7,scout,7,std::vector<ProductionItem>{}});
            const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();
            auto* view=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_presentation",{VALUE(value::Int,,4)}));
            GT_ASSERT(view->value.count("cost")==0 && view->value.count("forecast")==0 && view->value.count("name")==1 && fixture.Snapshot()==before
                && fixture.GameplayRandom()==random,"foreign display invented local cost authority or mutated state");GT_OK();
        });
    });
    task->AddTest("SMAC live human membership survives Start while empty-owner activity is repaired",[](GSE*,context::Context*)->std::string {
        const std::array<std::string,8> names{"live-worker","live-role-0","live-role-1","live-role-2","live-role-3","live-role-4","live-role-5","live-role-6"};
        for(const uint8_t membership:{2,6}) {
            std::string saved;ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(TanksSource,true);fixture.BasePresentationRoles(names);
            const auto source=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);game->AdvanceTurn(1);
                for(const size_t slot:{0u,6u}) {game->GetState()->m_slots->GetSlot(slot).GetPlayer()->SetDifficultyLevel("Librarian");}
                fixture.ResourceState([&](auto& state){
                    if(state.human_membership!=6) {throw std::runtime_error("new-world accepted human controllers were not bound explicitly");}
                    state.human_membership=membership;state.roster[1].active=membership==2;
                });
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));saved=fixture.Snapshot();GT_OK();
            });if(!source.empty()) {return "human membership source: "+source;}
            ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(TanksSource,true);restored.BasePresentationRoles(names);
            restored.ProductionPresentations({"choice-scout","choice-rover","choice-former","choice-colony"});
            const auto result=restored.Run([&](GSE_CALLABLE)->std::string {
                auto* game=restored.Game();for(const size_t slot:{0u,6u}) {game->GetState()->m_slots->GetSlot(slot).GetPlayer()->SetDifficultyLevel("Librarian");}
                restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
                bool exact=false;restored.ResourceState([&](auto& state){exact=state.human_membership==membership && !state.roster[1].active
                    && state.roster[1].role==roster::Role::Player && !state.human_defeat_request;});
                GT_ASSERT(exact,"Start inferred human membership or a loss from transport/activity");
                const auto before=restored.Snapshot();const auto random=restored.GameplayRandom();
                (void)game->GetCanonicalEconomy(&game->GetState()->m_slots->GetSlot(0));
                GT_ASSERT(Rejects([&]{game->GetCanonicalEconomy(&game->GetState()->m_slots->GetSlot(6));})
                    && restored.Snapshot()==before && restored.GameplayRandom()==random,"nonhuman/inactive owner was treated as active human or query mutated state");
                GT_OK();
            });if(!result.empty()) {return "human membership restore: "+result;}
        }GT_OK();
    });

    task->AddTest("SMAC live ranking uses saved caches before owner summary and preserves them through actual Start",[](GSE*,context::Context*)->std::string {
        const std::array<std::string,8> names{"live-worker","live-role-0","live-role-1","live-role-2","live-role-3","live-role-4","live-role-5","live-role-6"};
        std::string saved;
        ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(TanksSource,true);fixture.BasePresentationRoles(names);
        const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);game->AdvanceTurn(1);
            for(const size_t slot:{0u,6u}) {game->GetState()->m_slots->GetSlot(slot).GetPlayer()->SetDifficultyLevel("Librarian");}
            const auto stock=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
            base->SetProductionState(GSE_CALL,ProductionState{stock,0,stock,0,std::vector<ProductionItem>{}});base->SetAccumulatedNutrients(0);
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            GT_ASSERT(game->GetCanonicalEconomy(base->m_owner).design_counts->at(2)==1,"actual Scout factory omitted count increment");
            fixture.ResourceState([](auto& state){state.economy.at(1).design_counts->at(2)=255;});
            auto* um=game->GetUM();const auto* existing=um->GetUnits().begin()->second;
            auto* extra=um->SpawnConventionalUnit(GSE_CALL,unit::Unit::GetNextId(),existing->m_def,existing->m_owner,
                game->GetMap()->GetTile(8,2),existing->Conventional());
            GT_ASSERT(game->GetCanonicalEconomy(base->m_owner).design_counts->at(2)==0,"factory byte count did not wrap 255 to zero");
            auto removal=um->PrepareConventionalRemoval(GSE_CALL,{extra->m_id});removal.Commit();removal.Notify(GSE_CALL);
            GT_ASSERT(game->GetCanonicalEconomy(base->m_owner).design_counts->at(2)==255,"removal byte count did not wrap zero to 255");
            auto admitted=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();admitted.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(admitted));
            // Keep the second controller admitted after native load repair,
            // while its retained population cache still differs from live zero.
            auto* pod_tile=game->GetMap()->GetTile(16,16);pod_tile->is_water_tile=false;pod_tile->rockiness=game::backend::map::tile::ROCKINESS_FLAT;
            pod_tile->features=game::backend::map::tile::FEATURE_NONE;pod_tile->terraforming=game::backend::map::tile::TERRAFORMING_NONE;
            um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("choice-colony"),&game->GetState()->m_slots->GetSlot(6),
                pod_tile,{0,2,1,0,0,false,unit::ConventionalUnitState::Home::None,unit::SmacMoraleMode::Mode0});
            fixture.ResourceState([](auto& state){
                for(auto& technology:state.technology.factions) {technology.owned.fill(false);}
                for(auto& row:state.economy) {row.second.rates={0,0};row.second.population_aggregate=row.first==1 ? 0 : 3;}
                state.economy.at(1).design_counts->at(2)=251;
                state.ranking_scores=std::array<int32_t,8>{{17,31,47,61,79,97,113,127}};
            });
            saved=fixture.Snapshot();GT_OK();
        });if(!setup.empty()) {return "ranking setup: "+setup;}
        ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(TanksSource,true);restored.BasePresentationRoles(names);
        restored.ProductionPresentations({"choice-scout","choice-rover","choice-former","choice-colony"});
        return restored.Run([&](GSE_CALLABLE)->std::string {
            auto* game=restored.Game();for(const size_t slot:{0u,6u}) {game->GetState()->m_slots->GetSlot(slot).GetPlayer()->SetDifficultyLevel("Librarian");}
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
            restored.ResourceState([](auto& state){
                if(state.rank_order || state.economy.at(1).design_counts->at(2)!=251 || *state.economy.at(2).population_aggregate!=3
                    || state.ranking_scores!=std::optional<std::array<int32_t,8>>{{{17,31,47,61,79,97,113,127}}}) {
                    throw std::runtime_error("Start rebuilt retained ranking inputs from entities");
                }
            });
            game->Event(GSE_CALL,"complete_turn",{});restored.DrainEvents();
            int32_t leader=0;restored.ResourceState([&](auto& state){leader=state.rank_order ? state.rank_order->Leader() : 0;});
            GT_ASSERT(leader==2,"ranking used post-summary population or ignored the above-250 count guard");
            bool consumed=false;restored.ResourceState([&](auto& state){consumed=state.ranking_scores==std::optional<std::array<int32_t,8>>{{{17,-1,-1,-1,-1,-1,-1,-1}}};});
            GT_ASSERT(consumed,"Full global ranking published fresh scores instead of consumed sentinels or overwrote slot0");
            const auto economy=game->GetCanonicalEconomy(&game->GetState()->m_slots->GetSlot(0));
            GT_ASSERT(economy.design_counts->at(2)==1 && *economy.population_aggregate==1,"owner summary did not rebuild actual Unit and population caches");
            game->Event(GSE_CALL,"complete_turn",{});restored.DrainEvents();
            restored.ResourceState([&](auto& state){leader=state.rank_order ? state.rank_order->Leader() : 0;});
            GT_ASSERT(leader==1,"next global ranking did not consume the prior owner summaries");GT_OK();
        });
    });

    task->AddTest("SMAC human defeat request finishes its owner tail blocks gameplay and survives actual Start",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        const std::array<std::string,8> names{"live-worker","live-role-0","live-role-1","live-role-2","live-role-3","live-role-4","live-role-5","live-role-6"};
        for(const int victim:{1,2}) {
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(RestartSource,true);fixture.BasePresentationRoles(names);std::string saved;
            // Retain explicit population history before the ending; the
            // victim's actual owner tail must still refresh it to zero.
            fixture.ResourceState([](auto& state){for(auto& row:state.economy) {row.second.population_aggregate=10;}});

            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();game->AdvanceTurn(3);auto* lost=Populate(GSE_CALL,fixture,true,false,victim==1 ? 0 : 6,victim);
                SelectionPresentations(fixture);auto* bm=game->GetBM();auto* um=game->GetUM();
                for(const size_t slot:{0u,6u}) {game->GetState()->m_slots->GetSlot(slot).GetPlayer()->SetDifficultyLevel("Librarian");}
                const int other=victim==1 ? 2 : 1;auto* owner=&game->GetState()->m_slots->GetSlot(other==1 ? 0 : 6);
                const auto stock=game->MakeSmacProductionItem(owner,ProductionItemKind::Stockpile,38);
                const auto scout=game->MakeSmacProductionItem(lost->m_owner,ProductionItemKind::Unit,2);
                lost->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});lost->SetAccumulatedNutrients(-1);
                auto context=lost->GetSmacResourceContext();context.resource_events=SmacBaseResourceContext::Famine;lost->SetSmacResourceContext(GSE_CALL,context);
                Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),0));
                auto* control=new Base(game,702,owner,owner->GetPlayer()->GetFaction(),game->GetMap()->GetTile(16,16),"Ending control",{},pops,41,5,
                    ProductionState{stock,0,stock,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{other,false,0,0,0x40u,false});
                bm->SpawnBase(GSE_CALL,control);control->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{1,{{},1}},Roles());
                auto knowledge=KnowledgeValue(true,true,128,64).CopyData();knowledge.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
                if(victim==2) {um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("live-scout"),owner,game->GetMap()->GetTile(10,10),
                    {2,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});knowledge.units.push_back({92,0,false});}
                for(auto& tile:knowledge.tiles) {tile.explored_by=6;}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
                fixture.ResourceState([](auto& state){state.do_or_die=true;for(auto& row:state.economy) {row.second.credits=100;row.second.pending_debit=7;}
                    for(auto& tech:state.technology.factions) {tech.target=5;tech.cached_threshold=10000;}});
                auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));
                GT_ASSERT(Call(GSE_CALL,wrapper,"get_smac_ending")->type==VT_NULL,"fresh campaign invented an ending request");
                const auto control_before=Base::Serialize(control).ToString();size_t notifications=0;bool manager_writes_blocked=false;
                game->On(GSE_CALL,"smac_ending",NATIVE_CALL(&notifications,&manager_writes_blocked,bm) {
                    ++notifications;auto* manager=static_cast<value::Object*>(bm->Wrap(GSE_CALL));
                    manager_writes_blocked=Rejects([&]{Call(GSE_CALL,manager,"despawn_base",{VALUE(value::Int,,702)});})
                        && Rejects([&]{bm->DespawnBase(GSE_CALL,702);});
                    return VALUE(value::Undefined);
                }));
                game->Event(GSE_CALL,"complete_turn",{});
                auto* moving=um->GetUnit(victim==1 ? 91 : 92);auto* destination=game->GetMap()->GetTile(victim==1 ? 8 : 11,victim==1 ? 6 : 11);
                game->Event(GSE_CALL,"move_unit",{{"unit",moving->Wrap(GSE_CALL)},{"tile",destination->Wrap(GSE_CALL)}});fixture.DrainEvents();
                auto* ending=static_cast<value::Object*>(Call(GSE_CALL,wrapper,"get_smac_ending"));
                GT_ASSERT(notifications==1 && manager_writes_blocked && bm->GetBase(702)==control && !bm->GetBase(701) && !um->GetUnit(91) && ending->type==VT_OBJECT
                    && static_cast<value::Int*>(ending->Get("owner_simulation_id"))->value==victim
                    && static_cast<value::Bool*>(ending->Get("local"))->value==(victim==1)
                    && static_cast<value::Bool*>(Call(GSE_CALL,wrapper,"get_smac_eliminated"))->value==(victim==1),"ending controller conflated local status, victim or request notification");
                if(victim==1) {GT_ASSERT(Base::Serialize(control).ToString()==control_before,"ending processed a later owner's Base");}
                else {GT_ASSERT(um->GetUnit(92)->GetTile()==game->GetMap()->GetTile(10,10),"prequeued gameplay ran after the human ending request");}
                bool exact=false;fixture.ResourceState([&](auto& state){exact=state.human_defeat_request==victim && state.economy.at(victim).credits==120
                    && state.economy.at(victim).population_aggregate==0;});GT_ASSERT(exact,"ending skipped the original empty-owner tail or lost persistent request");
                saved=fixture.Snapshot();GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"complete_turn",{});}) && fixture.Snapshot()==saved,"ending allowed another turn");
                if(victim==2) {
                    auto* unit=um->GetUnit(92);auto* unit_wrapper=static_cast<value::Object*>(unit->Wrap(GSE_CALL));
                    auto* manager=static_cast<value::Object*>(um->Wrap(GSE_CALL));
                    GT_ASSERT(Rejects([&]{Call(GSE_CALL,manager,"despawn_unit",{VALUE(value::Int,,92)});})
                        && Rejects([&]{um->UpdateConventionalUnit(GSE_CALL,unit,unit->Conventional());})
                        && Rejects([&]{um->PrepareConventionalRemoval(GSE_CALL,{92});}) && fixture.Snapshot()==saved,"direct manager mutation bypassed the ending request");
                    GT_ASSERT(Rejects([&]{Call(GSE_CALL,unit_wrapper,"move_conventional",{VALUE(value::Int,,0),destination->Wrap(GSE_CALL)});})
                        && Rejects([&]{um->ValidateConventionalMove(unit,destination,0);}) && fixture.Snapshot()==saved,"direct movement wrapper bypassed the ending request");
                    GT_ASSERT(Rejects([&]{control->SetAccumulatedNutrients(999);}) && Rejects([&]{control->SetProductionState(GSE_CALL,control->GetProductionState());})
                        && Rejects([&]{control->SetSmacResourceContext(GSE_CALL,control->GetSmacResourceContext());})
                        && Rejects([&]{control->RecalculateSmacWorkers(GSE_CALL,Roles());}) && fixture.Snapshot()==saved,"direct Base mutation bypassed the ending request");
                    GT_ASSERT(Call(GSE_CALL,unit_wrapper,"get_conventional_state")->type==VT_OBJECT && unit->QueryStats(SmacMoraleMode::Mode0).hit_points>0,
                        "terminal request blocked read-only Unit inspection");
                }
                GT_ASSERT(Rejects([&]{Call(GSE_CALL,wrapper,"event",{VALUE(value::String,,"complete_turn"),VALUE(value::Object,,GSE_CALL_NOGC,{})});}) && fixture.Snapshot()==saved,
                    "script event bypassed the terminal request");GT_OK();
            });if(!result.empty()) {return "ending victim "+std::to_string(victim)+": "+result;}
            ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(RestartSource,true);restored.BasePresentationRoles(names);
            const auto result2=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));
                auto* ending=static_cast<value::Object*>(Call(GSE_CALL,wrapper,"get_smac_ending"));
                GT_ASSERT(ending->type==VT_OBJECT && static_cast<value::Int*>(ending->Get("owner_simulation_id"))->value==victim
                    && static_cast<value::Bool*>(ending->Get("local"))->value==(victim==1) && Rejects([&]{game->Event(GSE_CALL,"complete_turn",{});}),"actual Start dropped or replayed the ending request");GT_OK();
            });if(!result2.empty()) {return result2;}
        }GT_OK();
    });

    task->AddTest("SMAC conquered Base upkeep ages incoming history once after resource passes",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        for(const int former:{1,2}) {for(const int timer:{0,1,2,50}) {
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(BattleSource,true);std::string saved;
            const auto expected=timer==0 ? 0 : std::max(0,timer-(former==1 ? 2 : 1));
            const SmacConquestState next{timer && !expected ? 1 : former,static_cast<uint8_t>(expected)};
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* own=Populate(GSE_CALL,fixture,true);auto* bm=game->GetBM();
                own->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
                const auto stockpile=game->MakeSmacProductionItem(own->m_owner,ProductionItemKind::Stockpile,38);
                auto context=own->GetSmacResourceContext();context.conquest=SmacConquestState{former,static_cast<uint8_t>(timer)};
                Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),0));
                auto* base=new Base(game,702,own->m_owner,own->m_faction,game->GetMap()->GetTile(8,8),"Assimilation",{},pops,41,5,
                    ProductionState{stockpile,0,stockpile,0,std::vector<ProductionItem>{}},context);
                bm->SpawnBase(GSE_CALL,base);base->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{1,{{},1}},Roles());
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
                base->RecalculateSmacWorkers(GSE_CALL,Roles());
                GT_ASSERT(base->GetSmacResourceContext()==context,"ordinary recalculation aged conquest history");
                auto changed=context;changed.conquest->assimilation^=1;
                GT_ASSERT(Rejects([&]{base->SetSmacResourceContext(GSE_CALL,changed);}) && base->GetSmacResourceContext()==context,"generic setter rewrote conquest history");
                fixture.Updates();const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();auto bad=Roles();bad.specialists[6]="missing-role";
                GT_ASSERT(Rejects([&]{base->ProcessSmacUpkeep(GSE_CALL,bad,{},SmacMoraleMode::Mode0);}) && fixture.Snapshot()==before && fixture.Updates().empty(),
                    "failed upkeep published timer decay before Base transaction");
                const auto result=base->ProcessSmacUpkeep(GSE_CALL,Roles(),{},SmacMoraleMode::Mode0);
                GT_ASSERT(base->GetSmacResourceContext().conquest==next && base->m_pops.size()==1 && result.final_calculation
                    && fixture.GameplayRandom()==random,"finished Base upkeep missed tail decay or changed unrelated streams/population");
                base->RecalculateSmacWorkers(GSE_CALL,Roles());GT_ASSERT(base->GetSmacResourceContext().conquest==next,"subsequent query aged timer twice");
                saved=fixture.Snapshot();GT_OK();
            });if(!result.empty()) {return "assimilation timer="+std::to_string(timer)+" former="+std::to_string(former)+": "+result;}
            ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(BattleSource,true);
            const auto restored_result=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
                GT_ASSERT(restored.Game()->GetBM()->GetBase(702)->GetSmacResourceContext().conquest==next,"actual Start replayed assimilation tail or normalized stale zero history");GT_OK();
            });if(!restored_result.empty()) {return restored_result;}
        }}GT_OK();
    });

    task->AddTest("SMAC last Base capture stages successful replacement queries on another land region",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        const std::array<std::string,8> names{"live-worker","live-role-0","live-role-1","live-role-2","live-role-3","live-role-4","live-role-5","live-role-6"};
        // Turn6 is the native caller. Turn100 checks the statically established
        // fresh-stack rule with the existing three-Scout reinforcement package.
        for(const size_t turn:{6u,100u}) {
        const size_t count=turn==6 ? 4 : 7;
        ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(BattleSource,true,64,32);fixture.BasePresentationRoles(names);std::string saved,remembered;
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* own=Populate(GSE_CALL,fixture,true,false,0,1,1,16,16);SelectionPresentations(fixture);
            auto* bm=game->GetBM();auto* um=game->GetUM();auto* actor=um->GetUnit(91);auto* foreign=&game->GetState()->m_slots->GetSlot(6);
            own->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");foreign->GetPlayer()->SetDifficultyLevel("Librarian");game->AdvanceTurn(turn);
            const auto scout=game->MakeSmacProductionItem(own->m_owner,ProductionItemKind::Unit,2);
            own->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
            Base::pops_t pops;for(size_t p=0;p<2;++p) {pops.emplace(std::piecewise_construct,std::forward_as_tuple(19+p),std::forward_as_tuple(nullptr,19+p,bm->GetPopDef(Roles().specialists[1]),0));}
            const auto former=game->MakeSmacProductionItem(foreign,ProductionItemKind::Unit,1);
            auto* target=new Base(game,702,foreign,foreign->GetPlayer()->GetFaction(),game->GetMap()->GetTile(20,16),"Restart capture",{},pops,41,5,
                ProductionState{former,7,former,7,std::vector<ProductionItem>{former}},SmacBaseResourceContext{2,false,0,0,0x1000040u,false});
            bm->SpawnBase(GSE_CALL,target);target->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{1,{{},2}},Roles());actor->SetTile(GSE_CALL,game->GetMap()->GetTile(19,15));
            ConventionalUnitState remote{1,2,1,0,0,false,ConventionalUnitState::Home::Base,SmacMoraleMode::Mode0};remote.home_base_id=702;
            um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("choice-former"),foreign,game->GetMap()->GetTile(24,16),remote);
            // Two displayed land regions, separated even across periodic X.
            // Set shared vertices first, then update every cached classification.
            for(size_t y=0;y<32;++y) {for(size_t x=y&1;x<64;x+=2) {for(auto* corner:game->GetMap()->GetTile(x,y)->elevation.corners) {*corner=100;}}}
            for(size_t y=0;y<32;++y) {for(size_t x=y&1;x<64;x+=2) {if(x<4 || x>60 || (x>=29 && x<=35)) {
                for(auto* corner:game->GetMap()->GetTile(x,y)->elevation.corners) {*corner=-1000;}
            }}}
            for(size_t y=0;y<32;++y) {for(size_t x=y&1;x<64;x+=2) {auto* tile=game->GetMap()->GetTile(x,y);tile->Update();tile->moisture=MOISTURE_RAINY;}}
            auto knowledge=KnowledgeValue(true,true,64,32).CopyData();knowledge.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
            knowledge.units={{91,6,false},{92,6,false}};for(auto& tile:knowledge.tiles) {tile.explored_by=6;}
            knowledge.remembered[1].resize(1024);
            for(size_t y=0;y<32;++y) {for(size_t x=y&1;x<64;x+=2) {if(x>=36 && x<60) {
                const auto at=y*32+x/2;knowledge.tiles[at].explored_by=4;
                knowledge.remembered[1][at]=Knowledge::Improvements{FEATURE_RIVER,TERRAFORMING_ROAD,false,false};
            }}}
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
            fixture.ResourceState([](auto& state) {state.hostility.owners.at(1).directions[2].primary=16;state.hostility.owners.at(2).directions[1].primary=16;
                state.economy.at(1).credits=100;state.economy.at(2).credits=200;state.economy.at(2).population_aggregate=2;});
			fixture.ResourceState([&](auto& state){state.retained_land_region_concentration=(turn==100);state.retained_base_totals->at(1)=10;state.retained_base_totals->at(2)=20;state.victory_history->last_base_expansion_or_advice_turn.fill(777);
				state.victory_history->economic_deadlines.fill(777);state.victory_history->economic_paid_costs.fill(777);});
            fixture.GameplayRandom({3,3});fixture.Updates();const auto before=fixture.Snapshot();const auto first=Unit::GetNextId();
            um->ValidateConventionalMove(actor,target->GetTile(),0);
            GT_ASSERT(fixture.Snapshot()==before && Unit::GetNextId()==first && um->GetUnit(92),"successful replacement query published candidate placements or retired old Units");
            fixture.MoveEvent(GSE_CALL,actor,target->GetTile());GT_ASSERT(um->GetUnits().size()==count+1 && !um->GetUnit(92),"successful capture restart did not replace old faction Units");
			bool stamped=false;fixture.ResourceState([&](auto& state){const auto& history=*state.victory_history;
				stamped=history.last_base_expansion_or_advice_turn[1]==static_cast<int32_t>(turn) && history.last_base_expansion_or_advice_turn[2]==static_cast<int32_t>(turn)
					&& history.last_base_expansion_or_advice_turn[7]==777 && history.economic_deadlines[2]==777 && history.economic_paid_costs[2]==777
					&& state.retained_land_region_concentration==std::optional<bool>{turn==100}
					&& state.retained_base_totals->at(1)==11 && state.retained_base_totals->at(2)==0;});
			GT_ASSERT(stamped,"Capture/restart failed owner history stamps or reset the retained economic attempt");
            const std::array<int32_t,4> designs{{0,1,0,2}};const auto& topology=*game->GetMap()->GetTerritoryTopology();
            const auto captured_region=topology.Regions().at(16*32+10);map::tile::Tile* site=nullptr;
            for(size_t i=0;i<count;++i) {const auto* unit=um->GetUnit(first+i);GT_ASSERT(unit && unit->Conventional().design_id==(i<4 ? designs[i] : 2) && !unit->Conventional().home_base_id,"successful capture lost native package order or unhomed state");
                if(!site) {site=unit->GetTile();}GT_ASSERT(unit->GetTile()==site && topology.Regions().at(site->coord.y*32+site->coord.x/2)!=captured_region,"captor-region rejection failed or package split across sites");}
            GT_ASSERT(site->coord.x==size_t(smac_capture_restart_fixture::site[0]) && site->coord.y==size_t(smac_capture_restart_fixture::site[1])
                && fixture.GameplayRandom()==SmacGameplayRandomState(3,smac_capture_restart_fixture::final_crt),"native two-attempt restart or extra tracking CRT draw differs");
            const auto revealed=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();size_t explored=0;
            for(size_t y=0;y<32;++y) {for(size_t x=y&1;x<64;x+=2) {if(x>=36 && x<60 && (revealed.tiles[y*32+x/2].explored_by&2)) {++explored;}}}
            GT_ASSERT(explored==9,"tracking did not reveal exactly the native nine squares to the captor");
            for(const auto point:smac_capture_restart_fixture::reveal) {
                const auto* tile=game->GetMap()->GetTile(point[0],point[1]);const auto& memory=revealed.remembered[1][point[1]*32+point[0]/2];
                GT_ASSERT(memory && memory->features==tile->features && memory->terraforming==tile->terraforming && memory->unit_present==(tile==site),"tracking failed to refresh the captor's remembered terrain");
            }
            for(size_t i=0;i<count;++i) {const auto found=std::find_if(revealed.units.begin(),revealed.units.end(),[&](const auto& row){return row.id==first+i;});
                GT_ASSERT(found!=revealed.units.end() && found->visible_to==(i<4 ? smac_capture_restart_fixture::visibility[i] : 0),"tracking used numeric/reverse stack order or exposed every replacement");}
            size_t centers=0;for(const auto& request:fixture.QueuedFrontendRequests()) {if(request.type==game::FrontendRequest::FR_MAP_CENTER) {
                GT_ASSERT(request.data.map_center.x==44 && request.data.map_center.y==6,"native tracking recenter differs");++centers;
            }}GT_ASSERT(centers==1,"tracking omitted or duplicated camera request");
            bool active=false;fixture.ResourceState([&](auto& state){active=state.roster[1].active && !state.human_defeat_request;});GT_ASSERT(active,"successful capture restart requested defeat");
            remembered=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();saved=fixture.Snapshot();GT_OK();
        });if(!result.empty()) {return result;}
        ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(BattleSource,true,64,32);restored.BasePresentationRoles(names);
        const auto loaded=restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
			bool stamped=false;restored.ResourceState([&](auto& state){const auto& history=*state.victory_history;
				stamped=history.last_base_expansion_or_advice_turn[1]==static_cast<int32_t>(turn) && history.last_base_expansion_or_advice_turn[2]==static_cast<int32_t>(turn)
					&& history.economic_deadlines[2]==777 && history.economic_paid_costs[2]==777
					&& state.retained_land_region_concentration==std::optional<bool>{turn==100}
					&& state.retained_base_totals->at(1)==11 && state.retained_base_totals->at(2)==0;});
			GT_ASSERT(stamped,"Actual Start reconstructed capture/restart or economic history");
            GT_ASSERT(restored.Game()->GetUM()->GetUnits().size()==count+1 && restored.Game()->GetBM()->GetBases().size()==2
                && restored.GameplayRandom()==SmacGameplayRandomState(3,smac_capture_restart_fixture::final_crt)
                && restored.Game()->GetMap()->GetSmacWorkerKnowledge()->Serialize()==remembered,"actual Start replayed tracking or lost its visibility/exploration history");GT_OK();
        });
        if(!loaded.empty()) {return loaded;}
        }GT_OK();
    });
    task->AddTest("SMAC last Base capture joins native failed restart cleanup ending and actual Start",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        const std::array<std::string,8> names{"live-worker","live-role-0","live-role-1","live-role-2","live-role-3","live-role-4","live-role-5","live-role-6"};
        for(const int design:{1,0}) {for(const bool rollback:{false,true}) {
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(BattleSource,true,64,32);fixture.BasePresentationRoles(names);std::string saved;
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* own=Populate(GSE_CALL,fixture,true);SelectionPresentations(fixture);
                auto* bm=game->GetBM();auto* um=game->GetUM();auto* actor=um->GetUnit(91);auto* foreign=&game->GetState()->m_slots->GetSlot(6);
                own->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");foreign->GetPlayer()->SetDifficultyLevel("Librarian");game->AdvanceTurn(6);
                const auto scout=game->MakeSmacProductionItem(own->m_owner,ProductionItemKind::Unit,2);
                own->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
                Base::pops_t pops;for(size_t p=0;p<2;++p) {pops.emplace(std::piecewise_construct,std::forward_as_tuple(19+p),std::forward_as_tuple(nullptr,19+p,bm->GetPopDef(Roles().specialists[1]),0));}
                const auto former=game->MakeSmacProductionItem(foreign,ProductionItemKind::Unit,1);
                auto* target=new Base(game,702,foreign,foreign->GetPlayer()->GetFaction(),game->GetMap()->GetTile(20,16),"Last capture",{},pops,41,5,
                    ProductionState{former,7,former,7,std::vector<ProductionItem>{former}},SmacBaseResourceContext{2,false,0,0,0x1000040u,false});
                bm->SpawnBase(GSE_CALL,target);target->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{1,{{},2}},Roles());
                actor->SetTile(GSE_CALL,game->GetMap()->GetTile(19,15));
                ConventionalUnitState remote{design,2,1,0,0,false,ConventionalUnitState::Home::Base,SmacMoraleMode::Mode0};remote.home_base_id=702;
                auto* homed=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef(design ? "choice-former" : "choice-colony"),foreign,game->GetMap()->GetTile(30,16),remote);
                // Also exercise disposition's remaining-unit cleanup.
                um->SpawnConventionalUnit(GSE_CALL,93,um->GetUnitDef("choice-scout"),foreign,game->GetMap()->GetTile(40,16),
                    {2,2,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
                auto knowledge=KnowledgeValue(true,true,64,32).CopyData();knowledge.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
                knowledge.units={{91,6,false},{92,6,false},{93,6,false}};for(auto& tile:knowledge.tiles) {tile.explored_by=6;}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
                for(size_t y=0;y<32;++y) {for(size_t x=y&1;x<64;x+=2) {game->GetMap()->GetTile(x,y)->moisture=MOISTURE_RAINY;}}
                fixture.ResourceState([](auto& state) {
                    state.hostility.owners.at(1).directions[2].primary=16;state.hostility.owners.at(2).directions[1].primary=16;
                    state.economy.at(1).credits=100;state.economy.at(2).credits=200;state.economy.at(2).population_aggregate=2;
                    state.economy.at(1).regional_base_counts->at(1)=3;state.economy.at(2).regional_base_counts->at(1)=2;
                });fixture.GameplayRandom({3,3});fixture.Updates();
                auto* retained=homed->Wrap(GSE_CALL);auto* citizen=target->m_pops.at(19).Wrap(GSE_CALL);
                const auto before=fixture.Snapshot();const auto id=Unit::GetNextId();const auto jsf=game->GetRandom()->GetStateString();
                um->ValidateConventionalMove(actor,target->GetTile(),0);
                GT_ASSERT(fixture.Snapshot()==before && Unit::GetNextId()==id && Unit::Unwrap(retained)==homed,"capture query view published removal RNG IDs or ownership");
                if(rollback) {
                    auto missing=names;missing[7]="missing-role";fixture.BasePresentationRoles(missing);
                    GT_ASSERT(Rejects([&]{fixture.MoveEvent(GSE_CALL,actor,target->GetTile());}) && fixture.Snapshot()==before && Unit::GetNextId()==id
                        && Unit::Unwrap(retained)==homed && Pop::Unwrap(citizen)==&target->m_pops.at(19),"late last-capture failure published prepared retirement or faction authority");GT_OK();
                }
                fixture.MoveEvent(GSE_CALL,actor,target->GetTile());
                const auto& native=smac_restart_fixture::cases.back();
                GT_ASSERT(um->GetUnits().size()==1 && um->GetUnit(91)==actor && !um->GetUnit(92) && !um->GetUnit(93) && retained->type==VT_UNDEFINED
                    && Unit::GetNextId()==id && actor->GetTile()==target->GetTile() && actor->Conventional().spent==3,"capture kept old homed/remaining Units or made replacements after failed search");
                GT_ASSERT(target->m_owner==own->m_owner && target->m_pops.size()==1 && target->GetSmacResourceContext().conquest==std::optional<SmacConquestState>({2,50})
                    && target->GetAccumulatedNutrients()==5 && target->GetProductionState()->minerals==0 && Pop::Unwrap(citizen)==&target->m_pops.at(19),"last capture lost retained Base/citizen state");
                GT_ASSERT(fixture.GameplayRandom()==SmacGameplayRandomState(3,native.final_crt) && game->GetRandom()->GetStateString()==jsf,
                    "capture and failed restart did not consume original37+2050 CRT draws");
                bool exact=false;fixture.ResourceState([&](auto& state) {exact=state.economy.at(1).credits==233 && state.economy.at(2).credits==77
                    && !state.roster[1].active && state.human_defeat_request==std::optional<int32_t>(2) && state.roster[0].active
                    && state.economy.at(1).regional_base_counts->at(1)==3 && state.economy.at(2).regional_base_counts->at(1)==2;});
                GT_ASSERT(exact && game->GetSlotNum()==0,"native last-capture loot initialization stale counts activity ending or controller differs");
                size_t selections=0;bool refreshed=false;
                for(const auto& request:fixture.Updates()) {
                    if(request.type==game::FrontendRequest::FR_BASE_UPDATE && request.data.base_update.base_id==702 && request.data.base_update.slot_index==0) {refreshed=true;}
                    if(request.type==game::FrontendRequest::FR_BASE_SELECT && request.data.base_select.base_id==702) {GT_ASSERT(refreshed,"last-capture screen preceded ownership refresh");++selections;}
                }
                GT_ASSERT(selections==1,"ending request omitted or duplicated captured-Base screen entry");
                saved=fixture.Snapshot();GT_OK();
            });if(!result.empty()) {return "last capture "+std::to_string(design)+(rollback ? " rollback: " : ": ")+result;}
            if(!rollback) {
                ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(BattleSource,true,64,32);restored.BasePresentationRoles(names);
                const auto result=restored.Run([&](GSE_CALLABLE)->std::string {
                    restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();
                    GT_ASSERT(game->GetBM()->GetBases().size()==2 && game->GetUM()->GetUnits().size()==1 && game->GetBM()->GetBase(702)->m_owner->GetIndex()==0
                        && restored.GameplayRandom()==SmacGameplayRandomState(3,smac_restart_fixture::cases.back().final_crt),"actual Start replayed last-capture disposition");GT_OK();
                });if(!result.empty()) {return result;}
            }
        }}GT_OK();
    });
    task->AddTest("SMAC live undefended Base capture commits ownership loot homes history and actual Start",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        const std::array<std::string,8> names{"live-worker","live-role-0","live-role-1","live-role-2","live-role-3","live-role-4","live-role-5","live-role-6"};
        // The final extension distinguishes relocation scoring from nearest
        // home repair: the larger farther Base receives HQ, not the Former.
        for(const int mode:{2,1,0,3,4,5,6,7,8,9}) {
            const bool with_native=mode>=8;const int scenario=with_native?mode-8:mode;
            const size_t population=scenario>=3 ? 3 : 2;
            const bool paid=scenario>=5;
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(BattleSource,true);fixture.BasePresentationRoles(names);
            std::string saved,own_record,foreign_record,native_wire,native_factory;size_t native_id=0;std::vector<uint8_t> shared_details;
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* own=Populate(GSE_CALL,fixture,true);SelectionPresentations(fixture);
                auto* bm=game->GetBM();auto* um=game->GetUM();auto* actor=um->GetUnit(91);auto* foreign=&game->GetState()->m_slots->GetSlot(6);
                own->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");foreign->GetPlayer()->SetDifficultyLevel("Librarian");
                const auto scout=game->MakeSmacProductionItem(own->m_owner,ProductionItemKind::Unit,2);
                own->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
                bm->DefineFacility(new FacilityDef("tanks","Tanks","",40,{},3));
                bm->DefineFacility(new FacilityDef("capture-hq","HQ","",50,{},1));
                std::vector<Base*> bases;
                for(size_t i=0;i<(scenario==7 ? 3u : 2u);++i) {
                    Base::pops_t pops;
                    for(size_t p=0;p<(i==2 ? 4u : i ? 1u : population);++p) {pops.emplace(std::piecewise_construct,std::forward_as_tuple(19+p),std::forward_as_tuple(nullptr,19+p,bm->GetPopDef(Roles().specialists[1]),0));}
                    const auto former=game->MakeSmacProductionItem(foreign,ProductionItemKind::Unit,1);
                    auto* base=new Base(game,702+i,foreign,foreign->GetPlayer()->GetFaction(),game->GetMap()->GetTile(20+6*i,16),"Capture "+std::to_string(i),{},pops,41,5,
                        ProductionState{former,7,former,7,std::vector<ProductionItem>{former}},SmacBaseResourceContext{2,false,0,0,0x1000040u,false});
                    bm->SpawnBase(GSE_CALL,base);base->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{1,{{},static_cast<int32_t>(pops.size())}},Roles());bases.push_back(base);
                }
                for(size_t i=0;i<bases.size();++i) {auto production=*bases[i]->GetProductionState();production.hurry_partial_paid=i>0 || scenario%2==1;bases[i]->SetProductionState(GSE_CALL,production);}
                auto* target=bases[0];target->m_facilities.emplace("tanks",bm->GetFacilityDef("tanks"));
                if(scenario>=3) {target->m_facilities.emplace("capture-hq",bm->GetFacilityDef("capture-hq"));}
                actor->SetTile(GSE_CALL,game->GetMap()->GetTile(19,15));
                ConventionalUnitState former{1,2,1,0,0,false,ConventionalUnitState::Home::Base,SmacMoraleMode::Mode0};former.home_base_id=702;
                auto* remote=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("choice-former"),foreign,game->GetMap()->GetTile(30,16),former);
                fixture.ResourceState([&](auto& state) {
                    state.hostility.owners.at(1).directions[2].primary=16;state.hostility.owners.at(2).directions[1].primary=16;
                    state.economy.at(1).credits=100;state.economy.at(2).credits=paid ? 1200 : 200;
                    state.economy.at(1).population_aggregate=111;state.economy.at(2).population_aggregate=static_cast<int32_t>(population+1);
					state.victory_history->last_base_expansion_or_advice_turn[1]=777;state.victory_history->last_base_expansion_or_advice_turn[2]=888;
                    state.retained_base_totals->at(1)=10;state.retained_base_totals->at(2)=20;
                    // HQ reference retains3 here despite only2 live defender
                    // Bases. Relocation must not derive a fresh count.
                    state.economy.at(1).regional_base_counts->at(1)=1;state.economy.at(2).regional_base_counts->at(1)=scenario>=3 ? 3 : 2;
                });
                auto knowledge=KnowledgeValue(true,true,128,64).CopyData();knowledge.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
                knowledge.units={{91,6,false},{92,6,false}};for(auto& tile:knowledge.tiles) {tile.explored_by=6;}
                for(int observer=1;observer<=2;++observer) {
                    knowledge.remembered[observer].resize(knowledge.tiles.size());
                    for(int y=0;y<64;++y) {for(int x=y&1;x<128;x+=2) {
                        const auto* tile=game->GetMap()->GetTile(x,y);
                        knowledge.remembered[observer][y*64+x/2]=Knowledge::Improvements{tile->features,tile->terraforming,tile->base!=nullptr,!tile->units.empty()};
                    }}
                }
                knowledge.occupancy=map::SmacOccupancyCache::Blank(128,64,true);
                for(const auto& row:bm->GetBases()) {const auto* tile=row.second->GetTile();knowledge.occupancy->FoundBase({static_cast<int32_t>(tile->coord.x),static_cast<int32_t>(tile->coord.y)},row.second->GetSmacResourceContext().owner_simulation_id);}
                knowledge.occupancy->PlaceUnit({19,15},1,true);knowledge.occupancy->PlaceUnit({30,16},2,true);
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
                if(with_native) {
                    MoraleSet::morale_values_t native_names;for(int level=0;level<7;++level) {native_names.emplace_back("Native"+std::to_string(level));}
                    um->DefineMoraleSet(new MoraleSet("capture-native",native_names));
                    um->DefineUnit(new StaticDef("MindWorms",um->GetMoraleSet("capture-native"),"Mind Worms",MT_LAND,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
                    fixture.ResourceState([](auto& state){if(!state.native_factory) {state.native_factory=faction::SmacNativeFactoryState::Initialize(0,state.gameplay_random);}});
                    const auto plan=PrepareSmacNativeSpawn({{{80,40},128,64,true},1,7,9,1},{{{16,16},1}},[](int32_t,int32_t){return SmacNativeSpawnTile{3,false,1,1,false,15};});
                    auto birth=um->PrepareEcologyNativeBirths(GSE_CALL,plan);GT_ASSERT(birth.Size()==1,"Capture fixture did not create one native body");
                    native_id=birth.Candidate(0).m_id;birth.Commit();birth.Notify(GSE_CALL);native_wire=Unit::Serialize(um->GetUnit(native_id)).ToString();
                    fixture.ResourceState([&](auto& state){native_factory=state.native_factory->Serialize().ToString();});
                }
                fixture.GameplayRandom({3,3});
                shared_details.resize(4096);const std::array<uint8_t,5> detail_pattern{{0,1,127,128,255}};
                for(size_t i=0;i<shared_details.size();++i) {shared_details[i]=detail_pattern[i%detail_pattern.size()];}
                fixture.TerritoryTopology(game->GetMap()->GetTerritoryTopology()->WithLandmarkDetails(shared_details).Serialize());
                fixture.Updates();const auto before=fixture.Snapshot();const auto next=Unit::GetNextId();const auto jsf=game->GetRandom()->GetStateString();
                auto* survivor=&target->m_pops.at(19);auto* retained=survivor->Wrap(GSE_CALL);auto* retired=target->m_pops.at(18+population).Wrap(GSE_CALL);
                if(scenario==2) {
                    game->GetMap()->GetTile(18,16)->terraforming=TERRAFORMING_MIRROR;
                    const auto rejected=fixture.Snapshot();
                    GT_ASSERT(Rejects([&]{um->ValidateConventionalMove(actor,target->GetTile(),0);}) && fixture.Snapshot()==rejected
                        && fixture.Updates().empty() && Pop::Unwrap(retained)==survivor && retired->type!=VT_UNDEFINED,
                        "future resource query failure did not restore capture preview authority");GT_OK();
                }
                um->ValidateConventionalMove(actor,target->GetTile(),0);
                GT_ASSERT(*game->GetMap()->GetTerritoryTopology()->LandmarkDetails()==shared_details,"capture preview rewrote raw shared detail");
                GT_ASSERT(fixture.Snapshot()==before && fixture.Updates().empty(),"capture validation published authoritative state or refreshes");
                if(scenario==1 || scenario==4 || scenario==6) {
                    auto roles=names;roles[7]="missing-role";fixture.BasePresentationRoles(roles);
                    const auto rejected=fixture.Snapshot();
                    GT_ASSERT(Rejects([&]{fixture.MoveEvent(GSE_CALL,actor,target->GetTile());}) && fixture.Snapshot()==rejected
                        && fixture.Updates().empty() && target->m_owner==foreign && Pop::Unwrap(retained)==survivor && retired->type!=VT_UNDEFINED,
                        "late capture publication failure changed Base, Units, wrappers, streams or queues");GT_OK();
                }
                own_record=Base::Serialize(own).ToString();
                // Paid relocation changes only this control Base's HQ bit.
                if(paid && scenario!=7) {bases[1]->m_facilities.emplace("capture-hq",bm->GetFacilityDef("capture-hq"));}
                foreign_record=Base::Serialize(bases[1]).ToString();
                if(paid) {bases[1]->m_facilities.erase("capture-hq");}
                fixture.MoveEvent(GSE_CALL,actor,target->GetTile());
                const auto occupancy=game->GetMap()->GetSmacWorkerKnowledge()->CopyData().occupancy;
                GT_ASSERT((occupancy->At({19,15})==map::SmacOccupancyCache::Cell{1,0}
                    && occupancy->At({20,16})==map::SmacOccupancyCache::Cell{1,3}
                    && occupancy->At({30,16})==map::SmacOccupancyCache::Cell{2,2}),
                    "Capture omitted source departure, Base-center presence/affiliation or changed an unrelated Unit cache");
                GT_ASSERT(*game->GetMap()->GetTerritoryTopology()->LandmarkDetails()==shared_details,"capture commit rewrote raw shared detail");
                const auto context=target->GetSmacResourceContext();const auto production=target->GetProductionState();
                if(with_native) {std::string factory;fixture.ResourceState([&](auto& state){factory=state.native_factory->Serialize().ToString();});
                    GT_ASSERT(Unit::Serialize(um->GetUnit(native_id)).ToString()==native_wire && factory==native_factory,"Capture changed the unrelated native record or native factory counts");}
				bool capture_history=false;fixture.ResourceState([&](auto& state){capture_history=state.victory_history->last_base_expansion_or_advice_turn[1]==static_cast<int32_t>(game->GetTurnId())
					&& state.victory_history->last_base_expansion_or_advice_turn[2]==888 && state.retained_base_totals->at(1)==11 && state.retained_base_totals->at(2)==19;});
				GT_ASSERT(capture_history,"Capture did not stamp only the gaining owner");
                GT_ASSERT(target->m_owner==own->m_owner && target->m_pops.size()==population-1 && &target->m_pops.at(19)==survivor && Pop::Unwrap(retained)==survivor
                    && retired->type==VT_UNDEFINED && context.conquest==std::optional<SmacConquestState>({2,50}) && context.owner_simulation_id==1
                    && context.governor_options==own->GetSmacResourceContext().governor_options && !target->m_facilities.count("tanks") && !target->m_facilities.count("capture-hq"),"capture lost ownership, citizen lifetime, governor or facility rules");
                GT_ASSERT(actor->GetTile()==target->GetTile() && actor->Conventional().spent==3 && remote->Conventional().home_base_id==703
                    && remote->GetTile()==game->GetMap()->GetTile(30,16) && remote->m_owner==foreign && um->GetUnits().size()==(with_native?3u:2u)
                    && Unit::GetNextId()==next && game->GetRandom()->GetStateString()==jsf && !fixture.BattlePalette()
                    && fixture.GameplayRandom()==SmacGameplayRandomState(3,1951609278u),"capture changed movement, remote unit, identities or measured random streams");
                GT_ASSERT(target->GetAccumulatedNutrients()==5 && production && production->minerals==0 && production->current_item==scout
                    && production->baseline_item.kind==ProductionItemKind::Stockpile && production->baseline_minerals==0 && production->queue && production->queue->empty()
                    && Base::Serialize(own).ToString()==own_record && Base::Serialize(bases[1]).ToString()==foreign_record,"capture changed food/control Bases or selected wrong production");
                GT_ASSERT(target->GetSmacUpkeepCache() && target->GetSmacUpkeepCache()->calculation.nutrients.demand==(population==3 ? 4 : 2),
                    "capture resource pass did not use the reduced population");
                if(paid) {
                    const auto* relocated=bases[scenario==7 ? 2 : 1];
                    GT_ASSERT(relocated->m_facilities.count("capture-hq") && relocated->GetProductionState()->minerals==7,
                        "paid capture did not relocate HQ independently of home repair or changed its production");
                }
                bool exact=false;fixture.ResourceState([&](auto& state) {
                    const auto& a=state.hostility.owners.at(1);const auto& d=state.hostility.owners.at(2);
                    exact=state.economy.at(1).credits==(paid ? 820 : population==3 ? 220 : 200) && state.economy.at(2).credits==(paid ? 0 : population==3 ? 80 : 100) && state.economy.at(1).population_aggregate==111
                        && state.economy.at(2).population_aggregate==static_cast<int32_t>(population+1) && state.economy.at(1).regional_base_counts->at(1)==1
                        && state.economy.at(2).regional_base_counts->at(1)==(scenario>=3 ? 3 : 2) && a.directions[2].primary==0x8000a118u && d.directions[1].primary==0x8000a118u
                        && a.directions[2].paired_a==population*2 && a.directions[2].paired_b==population*2 && d.directions[1].paired_a==0 && d.losses[2]==0;
                });GT_ASSERT(exact,"capture differs from original loot and nondeduced history");
                size_t captured_refresh=SIZE_MAX,captured_select=SIZE_MAX,selections=0;
                const auto& requests=fixture.Updates();
                for(size_t i=0;i<requests.size();++i) {
                    const auto& request=requests[i];
                    if(request.type==game::FrontendRequest::FR_BASE_UPDATE && request.data.base_update.base_id==702
                        && request.data.base_update.slot_index==0 && request.data.base_update.pops->size()==population-1) {captured_refresh=i;}
                    if(request.type==game::FrontendRequest::FR_BASE_SELECT) {++selections;if(request.data.base_select.base_id==702) {captured_select=i;}}
                }
                GT_ASSERT(selections==1 && captured_refresh<captured_select,"captured Base screen opened before owner/population refresh or more than once");
                for(const auto& request:fixture.Updates()) {GT_ASSERT(request.type!=game::FrontendRequest::FR_BASE_SELECT,"capture screen selection replayed during later update flush");}
                GT_ASSERT(target->GetProductionState()->hurry_partial_paid==(scenario%2==1)
                    && game->GetBM()->GetBase(703)->GetProductionState()->hurry_partial_paid,
                    "capture cleared the independent partial marker or foreign control");
                saved=fixture.Snapshot();GT_OK();
            });if(!result.empty()) {return "capture: "+result;}
            if(scenario==0 || scenario==3 || scenario==5 || scenario==7) {
                ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(BattleSource,true);restored.BasePresentationRoles(names);
                const auto result=restored.Run([&](GSE_CALLABLE)->std::string {
                    restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(702);
                    if(with_native) {std::string factory;restored.ResourceState([&](auto& state){factory=state.native_factory->Serialize().ToString();});
                        GT_ASSERT(game->GetUM()->GetUnits().size()==3 && Unit::Serialize(game->GetUM()->GetUnit(native_id)).ToString()==native_wire && factory==native_factory,"Captured world Start lost native identity or factory authority");}
                    GT_ASSERT(*game->GetMap()->GetTerritoryTopology()->LandmarkDetails()==shared_details,"actual captured world Start lost shared bytes");
                    GT_ASSERT(base && base->GetSmacResourceContext().conquest==std::optional<SmacConquestState>({2,50}) && base->m_owner->GetIndex()==0
                        && base->m_pops.size()==population-1 && !base->m_facilities.count("capture-hq") && game->GetUM()->GetUnit(92)->Conventional().home_base_id==703
                        && restored.GameplayRandom()==SmacGameplayRandomState(3,1951609278u) && Base::Serialize(game->GetBM()->GetBase(701)).ToString()==own_record
                        && Base::Serialize(game->GetBM()->GetBase(703)).ToString()==foreign_record,"actual Start replayed capture or lost saved ownership/history/home/control Bases");
                    GT_ASSERT(base->GetProductionState()->hurry_partial_paid==(scenario%2==1)
                        && game->GetBM()->GetBase(703)->GetProductionState()->hurry_partial_paid,"captured paid marker lost through actual Start");
                    GT_ASSERT(base->GetSmacUpkeepCache() && base->GetSmacUpkeepCache()->calculation.nutrients.demand==(population==3 ? 4 : 2),"actual Start lost the captured population's resource cache");
                    if(paid) {GT_ASSERT(game->GetBM()->GetBase(scenario==7 ? 704 : 703)->m_facilities.count("capture-hq"),"actual Start lost relocated Headquarters");}
                    base->ProcessSmacUpkeep(GSE_CALL,Roles(),{},SmacMoraleMode::Mode0);
                    GT_ASSERT(base->GetSmacResourceContext().conquest==std::optional<SmacConquestState>({2,49}),"captured and reloaded Base did not reach the next assimilation tail");
                    if(with_native) {
                        restored.ResourceState([](auto& state){for(auto& technology:state.technology.factions) {
                            technology.target=5;technology.cached_threshold=100000;technology.labs=0;
                        }});
                        // Owner2 retains its Base but no Units. Its existence scan
                        // must reach owner0 rather than unpacking it as conventional.
                        auto removal=game->GetUM()->PrepareConventionalRemoval(GSE_CALL,{92});removal.Commit();removal.Notify(GSE_CALL);
                        const auto turn=game->GetTurnId();const auto* body=game->GetUM()->GetUnit(native_id);
                        const auto turn_wire=Unit::Serialize(body).ToString();
                        game->Event(GSE_CALL,"complete_turn",{});restored.DrainEvents();
                        std::string factory;restored.ResourceState([&](auto& state){factory=state.native_factory->Serialize().ToString();});
                        GT_ASSERT(game->GetTurnId()==turn+1 && game->GetUM()->GetUnit(native_id)==body
                            && Unit::Serialize(body).ToString()==turn_wire && factory==native_factory,
                            "Actual human turn rejected or rewrote the independently owned native body/factory");
                        GT_ASSERT(game->GetUM()->GetUnit(91)->Conventional().spent==0,
                            "Native coexistence bypassed human movement upkeep");
                    }GT_OK();
                });if(!result.empty()) {return "capture restore: "+result;}
            }
        }GT_OK();
    });

    task->AddTest("SMAC nonzero initialization clears mutual relation mask and retains battle history",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(TanksSource,true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            fixture.ResourceState([](auto& state){auto& a=state.hostility.owners.at(1);auto& d=state.hostility.owners.at(2);
                a.directions[2]={UINT32_MAX,77,-9,17,23};d.directions[1]={0x80008118u,99,42,31,41};d.directions[3].primary=UINT32_MAX;
                a.losses[2]=9;a.losses[511]=UINT16_MAX;a.integrity=4;a.bookkeeping=7;
                state.economy.at(1).population_aggregate=137;state.economy.at(2).population_aggregate=222;state.retained_base_totals->at(1)=777;state.retained_base_totals->at(2)=888;});
			fixture.ResourceState([](auto& state){state.victory_history->last_base_expansion_or_advice_turn.fill(777);
				state.victory_history->economic_deadlines.fill(777);state.victory_history->economic_paid_costs.fill(777);});
            fixture.SettleEconomy(1);bool exact=false;
            fixture.ResourceState([&](auto& state){const auto& a=state.hostility.owners.at(1);const auto& d=state.hostility.owners.at(2);
                exact=a.directions[2]==faction::SmacHostileDirection{0x6fff00b7u,77,-9,17,23}
                    && d.directions[1]==faction::SmacHostileDirection{0x10u,99,42,31,41} && d.directions[3].primary==UINT32_MAX
                    && a.losses[2]==9 && a.losses[511]==UINT16_MAX && a.integrity==4 && a.bookkeeping==7
                    && state.economy.at(1).population_aggregate==137 && state.economy.at(2).population_aggregate==222 && state.retained_base_totals->at(1)==0 && state.retained_base_totals->at(2)==888;});
			bool history=false;fixture.ResourceState([&](auto& state){const auto& saved=*state.victory_history;
				history=saved.last_base_expansion_or_advice_turn[1]==static_cast<int32_t>(fixture.Game()->GetTurnId())
					&& saved.last_base_expansion_or_advice_turn[2]==777 && saved.economic_deadlines[1]==777 && saved.economic_paid_costs[1]==777;});
            GT_ASSERT(exact && history,"actual faction-tail initialization reset economic history or touched an unrelated pair");GT_OK();
        });
    });

    task->AddTest("SMAC live Blind research priorities and discovery tails resume once through actual Start",[](GSE*,context::Context*)->std::string {
        const auto provider=[](const std::string& filename) {
            auto source=UpkeepSource(filename);size_t at=0;
            while((at=source.find("UnknownTech",at))!=std::string::npos) {source.replace(at,11,"Tech");at+=4;}
            return source;
        };
        const std::array<std::string,8> names{"live-worker","live-role-0","live-role-1","live-role-2","live-role-3","live-role-4","live-role-5","live-role-6"};
        for(int scenario=0;scenario<7;++scenario) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(provider,"GAIANS");fixture.BasePresentationRoles(names);
            std::vector<std::string> paused;std::string completed,completed_base,completed_unit,completed_jsf;std::optional<game::backend::faction::SmacSimulationState> final_state;
            const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
                // Turn 14 exercises the late research gate without the separate
                // four-turn territory producer, which this fixture does not own.
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,true);game->AdvanceTurn(scenario>=5 ? 13:6);
                game->GetState()->m_settings.global.SelectSmacCampaignRules(true);
                // Late-turn controls are beyond the original starvation grace.
                base->SetAccumulatedNutrients(scenario>=5 ? 20:0);
                const auto stock=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
                base->SetProductionState(GSE_CALL,ProductionState{stock,0,stock,0,std::vector<ProductionItem>{}});
                fixture.ResourceState([&](auto& state){
                    state.ecology_world=game::backend::faction::SmacEcologyWorld{1,1,0};
                    state.strategy->at(1).priorities={2,-1,0,3};
                    auto& tech=state.technology.factions.front();tech.labs=scenario>=2 && scenario<=4 ? 99:0;tech.score=scenario==6 ? 2:0;
                    tech.target=scenario>=2 && scenario<=4 ? std::optional<int32_t>(scenario==2 ? 0:1):std::nullopt;
                    tech.cached_threshold=scenario==3 ? 10000:1;tech.pending_secrets=scenario==3;
                    state.economy.at(1).rates={0,5};state.economy.at(1).credits=100;
                });
                std::vector<bool> sea(128*64/2,false);
                auto topology=game::backend::map::GlsmacTerritoryTopology::Build(128,64,true,sea);
                fixture.TerritoryTopology(topology.WithTerritorySummary({}).Serialize());
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
                fixture.GameplayRandom({1234,5678});game->Event(GSE_CALL,"complete_turn",{});GT_OK();
            });if(!setup.empty()) {return setup;}fixture.DrainEvents();
            const auto finish=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));
                int questions=0;
                while(true) {
                    auto* pending=Call(GSE_CALL,wrapper,"get_pending_smac_research");if(pending->type==VT_NULL) {break;}
                    GT_ASSERT(pending->type==VT_OBJECT && questions<3,"Blind research repeated a question or exposed invalid data");
                    auto* request=static_cast<value::Object*>(pending);
                    GT_ASSERT(static_cast<value::Bool*>(request->Get("blind"))->value && static_cast<value::Array*>(request->Get("choices"))->value.empty(),
                        "Standard Rules still exposed the manual technology pool");
                    if(questions==0 && scenario==2) {
                        bool exact=false;fixture.ResourceState([&](const auto& state){const auto& tech=state.technology.factions.front();exact=tech.owned[0] && tech.score==2 && tech.labs>=100;});
                        GT_ASSERT(exact,"Post-discovery question reset labs early or omitted ownership");
                    }
                    if(scenario==6) {
                        const auto random=fixture.GameplayRandom();
                        fixture.ResourceState([](auto& state){state.technology.factions.front().score=0;});
                        GT_ASSERT(Rejects([&]{(void)fixture.Snapshot();}) && fixture.GameplayRandom()==random,
                            "Saving an invalid late Blind question drew RNG or accepted an automatic-selection boundary");
                        fixture.ResourceState([](auto& state){state.technology.factions.front().score=2;});
                    }
                    paused.push_back(fixture.Snapshot());
                    const auto id=request->Get("request_id");
                    const value::object_properties_t answer{{"request_id",id},{"mask",scenario==1 ? static_cast<Value*>(VALUE(value::Null)):VALUE(value::Int,,0)}};
                    game->Event(GSE_CALL,"set_research_priorities",answer);game->Event(GSE_CALL,"set_research_priorities",answer);
                    fixture.DrainEvents();++questions;
                }
                if(questions==0 && scenario!=5) {
                    for(const auto& fr:fixture.QueuedFrontendRequests()) {if(fr.type==game::FrontendRequest::FR_ERROR) {return *fr.data.error.what;}}
                    std::string diagnostic;fixture.ResourceState([&](const auto& state){const auto& tech=state.technology.factions.front();diagnostic="labs="+std::to_string(tech.labs)+" target="+std::to_string(tech.target.value_or(-1))+" turn="+std::to_string(game->GetTurnId());});
                    return "No Blind question: "+diagnostic;
                }
                GT_ASSERT(questions==(scenario==5 ? 0:scenario==4 ? 2:1),"Blind discovery/Secrets selected an incorrect number of times at scenario "+std::to_string(scenario));
                bool exact=false;fixture.ResourceState([&](const auto& state){const auto& tech=state.technology.factions.front();
                    exact=tech.target.has_value() && tech.score==(scenario==4 ? 4:(scenario>=2 && scenario<=3) || scenario==6 ? 2:0)
                        && (scenario==1 || scenario==5 ? state.strategy->at(1).priorities==std::array<int32_t,4>{2,-1,0,3}:state.strategy->at(1).priorities==std::array<int32_t,4>{0,0,0,0})
                        && (scenario==2 || scenario==4 ? tech.labs==0:tech.labs>0) && (scenario!=3 || tech.pending_secrets);
                });GT_ASSERT(exact,"Blind selection lost cancellation, labs, bounded Secrets or score at scenario "+std::to_string(scenario));
                completed=fixture.Snapshot();completed_base=Base::Serialize(game->GetBM()->GetBase(701)).ToString();completed_unit=game::backend::unit::Unit::Serialize(game->GetUM()->GetUnit(91)).ToString();completed_jsf=game->GetRandom()->GetStateString();fixture.ResourceState([&](const auto& state){final_state=state;});GT_OK();
            });if(!finish.empty()) {return finish;}
            for(size_t checkpoint=0;checkpoint<paused.size();++checkpoint) {
                ConventionalUnitFixture restored;restored.ConfigureProduction(provider,"GAIANS");restored.BasePresentationRoles(names);
                const auto resumed=restored.Run([&](GSE_CALLABLE)->std::string {
                    auto* game=restored.Game();game->GetState()->m_settings.global.SelectSmacCampaignRules(true);
                    restored.RestoreThroughStart(GSE_CALL,paused[checkpoint],[](GSE_CALLABLE){});
                    auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));size_t count=0;
                    while(true) {
                        auto* pending=Call(GSE_CALL,wrapper,"get_pending_smac_research");if(pending->type==VT_NULL) {break;}
                        GT_ASSERT(pending->type==VT_OBJECT && ++count<4,"Restored Blind choice did not finish");auto* request=static_cast<value::Object*>(pending);
                        game->Event(GSE_CALL,"set_research_priorities",{{"request_id",request->Get("request_id")},{"mask",scenario==1 ? static_cast<Value*>(VALUE(value::Null)):VALUE(value::Int,,0)}});
                        restored.DrainEvents();
                    }
                    const auto actual=restored.Snapshot();size_t difference=0;while(difference<actual.size() && difference<completed.size() && actual[difference]==completed[difference]) {++difference;}
                    bool same=false;restored.ResourceState([&](const auto& state){same=state==*final_state;});
                    GT_ASSERT(ResearchSnapshot(game,actual)==ResearchSnapshot(game,completed),"Blind Save/Start replayed a stage or changed target, labs, priorities or RNG at scenario "+std::to_string(scenario)+"/"+std::to_string(checkpoint)+" first="+std::to_string(difference)+" lengths="+std::to_string(actual.size())+"/"+std::to_string(completed.size())+" simulation_equal="+std::to_string(same)+" base="+std::to_string(Base::Serialize(game->GetBM()->GetBase(701)).ToString()==completed_base)+" unit="+std::to_string(game::backend::unit::Unit::Serialize(game->GetUM()->GetUnit(91)).ToString()==completed_unit)+" jsf="+std::to_string(game->GetRandom()->GetStateString()==completed_jsf));GT_OK();
                });if(!resumed.empty()) {return resumed;}
            }
        }GT_OK();
    });

    task->AddTest("SMAC live human research binds retained histories without refreshing or drawing",[](GSE*,context::Context*)->std::string {
        const auto provider=[](const std::string& filename) {
            auto source=UpkeepSource(filename);
            // The parser fixture intentionally has unresolved facility tokens;
            // this consumer requires a complete valid catalog instead.
            size_t at=0;while((at=source.find("UnknownTech",at))!=std::string::npos) {source.replace(at,11,"Tech");at+=4;}
            return source;
        };
        ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(provider,true,128,64,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            Populate(GSE_CALL,fixture,false);
            std::vector<bool> sea(128*64/2,false);
            auto topology=game::backend::map::GlsmacTerritoryTopology::Build(128,64,true,sea);
            game::backend::map::smac_territory::TerritorySummary summary{};summary.owners[1].known[1]=77;
            fixture.TerritoryTopology(topology.WithTerritorySummary(summary).Serialize());
            fixture.ResourceState([](auto& state){
                state.strategy->at(1).priorities={2,-1,0,3};state.retained_base_totals->at(1)=13;
                state.capabilities->factions[0].available[0]=7;state.capabilities->factions[0].foreign[0]=19;
                state.capabilities->factions[0].available[1]=99;
                state.ecology_history->research_aggregate[1]=31;state.ecology_world=game::backend::faction::SmacEcologyWorld{1,0,0};
                state.regional_strategy->at(1)[1]=2;state.economy.at(1).regional_base_counts->at(1)=4;
                state.economy.at(2).regional_base_counts->at(1)=9;
                state.hostility.owners.at(1).directions[2].primary=0x18;
                state.victory_history->economic_deadlines[1]=500;
            });
            const auto before=fixture.Snapshot();const auto rng=fixture.GameplayRandom();
            const auto input=fixture.HumanResearchInputs(1,true);
            GT_ASSERT((input.priorities==std::array<int32_t,4>{2,-1,0,3}) && input.base_count==13 && input.maximum_attack==7
                && input.maximum_defense==19 && input.ecology_aggregate==31 && input.cloud_cover==0 && input.endgame_pressure
                && input.regional_known_tiles[1]==77 && input.regional_strategy[1]==2 && input.regional_bases[1][1]==4
                && input.regional_bases[2][1]==9 && input.relations[2]==0x18 && input.blind
                && input.intrinsic_planet==1 && input.intrinsic_probe==0 && input.former_prerequisite==6
                && input.foil_prerequisite==-2 && input.reactor_prerequisites[0]==-2,
                "Live research binder recounted or confused retained input identities");
            GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==rng,"Research input query changed authority or RNG");
            fixture.ResourceState([](auto& state){state.capabilities.reset();});
            GT_ASSERT(Rejects([&]{fixture.HumanResearchInputs(1,true);}),"Missing retained research capability was invented");GT_OK();
        });
    });

    task->AddTest("SMAC live hostile Scout movement joins battle history casualties palette and actual Start",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        for(int scenario=0;scenario<7;++scenario) {
            const auto provider=scenario==5 ? FanaticBattleSource : BattleSource;
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(provider,true);
            std::string saved;uint32_t saved_crt=0;size_t survivor_id=0;int32_t winner=0,loser=0;
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,scenario==4);auto* um=game->GetUM();auto* actor=um->GetUnit(91);
                const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
                base->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
                auto* foreign=&game->GetState()->m_slots->GetSlot(6);
                base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");foreign->GetPlayer()->SetDifficultyLevel("Librarian");
                auto record=actor->Conventional();if(scenario==2) {record.stored_morale=6;}if(scenario==3) {record.damage=9;}
                um->UpdateConventionalUnit(GSE_CALL,actor,record);actor->SetTile(GSE_CALL,game->GetMap()->GetTile(8,8));
                const ConventionalUnitState enemy{2,2,1,static_cast<uint8_t>(scenario==3 ? 9 : 0),0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0};
                auto* defender=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("live-scout"),foreign,game->GetMap()->GetTile(9,9),enemy);
                if(scenario==0) {
                    auto own=record;um->SpawnConventionalUnit(GSE_CALL,93,um->GetUnitDef("live-scout"),base->m_owner,game->GetMap()->GetTile(10,10),own);
                    um->SpawnConventionalUnit(GSE_CALL,94,um->GetUnitDef("live-scout"),foreign,game->GetMap()->GetTile(11,11),enemy);
                }
                fixture.ResourceState([&](auto& state){
                    auto& a=state.hostility.owners.at(1);auto& d=state.hostility.owners.at(2);
                    a.directions[2].primary=d.directions[1].primary=scenario==6 ? 0x8000a118u : scenario==1 ? 16 : 0;
                    a.directions[2].transition_turn=2;d.directions[1].transition_turn=3;
                    a.directions[2].paired_b=5;d.directions[1].paired_b=7;a.bookkeeping=4;d.bookkeeping=7;
                    if(scenario==4) {state.social.factions[0].retained->at(4)=-1;}
                });
                auto data=KnowledgeValue(true,true,128,64).CopyData();data.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
                data.units={{91,4,false},{92,2,false}};if(scenario==0) {data.units.push_back({93,4,false});data.units.push_back({94,2,false});}
                for(auto& tile:data.tiles) {tile.explored_by=6;}
                for(int observer=1;observer<=2;++observer) {
                    data.remembered[observer].resize(data.tiles.size());
                    for(const auto xy:{std::pair<int,int>{8,8},{9,9},{10,10},{11,11}}) {data.remembered[observer][xy.second*64+xy.first/2]=Knowledge::Improvements{0,0,false,true};}
                }
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                fixture.GameplayRandom({3,scenario==2 ? 1u : scenario==3 ? 308u : 3u});
                const auto next=Unit::GetNextId();const auto jsf=game->GetRandom()->GetStateString();fixture.Updates();const auto before=fixture.Snapshot();
                um->ValidateConventionalMove(actor,defender->GetTile(),0);
                GT_ASSERT(fixture.Snapshot()==before && !fixture.BattlePalette(),"attack validation published random, palette, war or Unit state");
                size_t hooks=0;bool coherent=true;
                um->On(GSE_CALL,"unit_despawn",NATIVE_CALL(&fixture,game,scenario,&hooks,&coherent) {
                    ++hooks;const auto word=scenario==6 ? 0x8000a118u : 0x80008118u;bool history=false;
                    fixture.ResourceState([&](auto& state){history=state.hostility.owners.at(1).directions[2].primary==word
                        && state.hostility.owners.at(2).directions[1].primary==word;});
                    coherent=coherent && history && fixture.BattlePalette()==9605 && game->GetUM()->GetUnits().size()==(scenario==0 ? 4-hooks : 1);
                    return VALUE(value::Undefined);
                }));
                auto* retained_actor=actor->Wrap(GSE_CALL);auto* retained_defender=defender->Wrap(GSE_CALL);
                if(scenario==3) {
                    const value::object_properties_t args{{"unit",retained_actor},{"tile",defender->GetTile()->Wrap(GSE_CALL)}};
                    try {game->Event(GSE_CALL,"move_unit",args);} catch(const std::exception& error) {return std::string("first battle enqueue: ")+error.what();}
                    try {game->Event(GSE_CALL,"move_unit",args);} catch(const std::exception& error) {return std::string("second battle enqueue: ")+error.what();}
                    try {fixture.DrainEvents();} catch(const std::exception& error) {return std::string("battle dispatch: ")+error.what();}
                } else {fixture.MoveEvent(GSE_CALL,actor,defender->GetTile());}
                const auto& requests=fixture.QueuedFrontendRequests();
                const auto playback=std::find_if(requests.begin(),requests.end(),[](const auto& request){return request.type==game::FrontendRequest::FR_SCOUT_BATTLE;});
                GT_ASSERT(playback!=requests.end() && playback->data.scout_battle.plan->units[0]==91 && playback->data.scout_battle.plan->units[1]==92
                    && !playback->data.scout_battle.plan->steps.empty() && playback->data.scout_battle.plan->steps.back().effect==game::scout_presentation::Effect::Death,
                    "Actual Scout command omitted its owned presentation before retiring the casualty");
                const bool defender_wins=scenario==3 || scenario==4 || scenario==5;
                GT_ASSERT((defender_wins ? retained_actor : retained_defender)->type==VT_UNDEFINED,"casualty left a callable native Unit wrapper linked");
                survivor_id=defender_wins ? 92 : 91;winner=defender_wins ? 2 : 1;loser=defender_wins ? 1 : 2;
                auto* survivor=um->GetUnit(survivor_id);
                const auto expected_damage=scenario==2 ? 1 : scenario==4 || scenario==5 ? 7 : 9;
                saved_crt=scenario==2 ? 255755822u : scenario==3 ? 2715072846u : scenario==4 || scenario==5 ? 4127244341u : 1369276809u;
                GT_ASSERT(survivor && !um->GetUnit(defender_wins ? 91 : 92) && survivor->GetTile()==game->GetMap()->GetTile(defender_wins ? 9 : 8,defender_wins ? 9 : 8)
                    && survivor->Conventional().damage==expected_damage && survivor->Conventional().stored_morale==(scenario==2 ? 6 : 2)
                    && survivor->Conventional().spent==(defender_wins ? 0 : 3) && fixture.GameplayRandom()==SmacGameplayRandomState(1018897798u,saved_crt)
                    && fixture.BattlePalette()==9605 && hooks==1 && coherent,"live attack differs at scenario "+std::to_string(scenario)
                    +" units="+std::to_string(um->GetUnits().size())+" damage="+std::to_string(survivor ? survivor->Conventional().damage : -1)
                    +" morale="+std::to_string(survivor ? survivor->Conventional().stored_morale : -1)+" spent="+std::to_string(survivor ? survivor->Conventional().spent : -1)
                    +" shared="+std::to_string(fixture.GameplayRandom().Shared())+" crt="+std::to_string(fixture.GameplayRandom().Crt())
                    +" palette="+std::to_string(fixture.BattlePalette().value_or(0))+" hooks="+std::to_string(hooks)+" coherent="+std::to_string(coherent));
                if(scenario==0) {
                    auto* second=um->GetUnit(93);fixture.MoveEvent(GSE_CALL,second,game->GetMap()->GetTile(11,11));saved_crt=928401183u;
                    GT_ASSERT(second->Conventional().damage==9 && second->Conventional().stored_morale==2 && second->Conventional().spent==3
                        && !um->GetUnit(94) && hooks==2 && coherent && fixture.GameplayRandom()==SmacGameplayRandomState(1018897798u,saved_crt),
                        "second actual attack recreated palette, reseeded streams or lost casualty compaction");
                }
                if(scenario==2) {
                    fixture.MoveEvent(GSE_CALL,survivor,game->GetMap()->GetTile(9,9));
                    GT_ASSERT(survivor->GetTile()==game->GetMap()->GetTile(9,9) && survivor->Conventional().spent==6 && hooks==1
                        && fixture.GameplayRandom()==SmacGameplayRandomState(1018897798u,saved_crt),"Elite survivor could not take the real next movement without reseeding or repeating combat");
                }
                bool history=false;fixture.ResourceState([&](auto& state){const auto& a=state.hostility.owners.at(1);const auto& d=state.hostility.owners.at(2);
                    const auto& victorious=state.hostility.owners.at(winner).directions[loser];
                    history=a.directions[2].transition_turn==2 && d.directions[1].transition_turn==3 && a.bookkeeping==4 && d.bookkeeping==7
                        && a.integrity==0 && d.integrity==0 && victorious.paired_a==(scenario==0 ? 2u : 1u)
                        && victorious.paired_b==(defender_wins ? 8u : scenario==0 ? 7u : 6u) && state.hostility.owners.at(loser).losses[2]==(scenario==0 ? 2u : 1u);});
                GT_ASSERT(history && Unit::GetNextId()==next && game->GetRandom()->GetStateString()==jsf,"battle dropped history, changed war-break fields or used JSF/Unit IDs");
                saved=fixture.Snapshot();GT_OK();
            });if(!result.empty()) {return "hostile scenario "+std::to_string(scenario)+": "+result;}
            ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(provider,true);restored.BattlePalette(12345);
            const auto restore=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* survivor=game->GetUM()->GetUnit(survivor_id);
                bool history=false;restored.ResourceState([&](auto& state){history=state.hostility.owners.at(loser).losses[2]==(scenario==0 ? 2u : 1u)
                    && state.hostility.owners.at(winner).directions[loser].paired_a==(scenario==0 ? 2u : 1u);});
                GT_ASSERT(survivor && history && restored.BattlePalette()==12345 && restored.GameplayRandom()==SmacGameplayRandomState(1018897798u,saved_crt),
                    "actual Start replayed battle, lost nonderived history, reseeded streams or overwrote process presentation state");GT_OK();
            });if(!restore.empty()) {return restore;}
        }GT_OK();
    });

    task->AddTest("SMAC defended Base battle preserves ownership and saved aftermath before separate capture",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        const std::array<std::string,8> names{"live-worker","live-role-0","live-role-1","live-role-2","live-role-3","live-role-4","live-role-5","live-role-6"};
        for(const bool attacker_wins:{true,false}) {
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(BattleSource,true);fixture.BasePresentationRoles(names);std::string saved;
            auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* own=Populate(GSE_CALL,fixture,true);SelectionPresentations(fixture);
                auto* bm=game->GetBM();auto* um=game->GetUM();auto* actor=um->GetUnit(91);auto* foreign=&game->GetState()->m_slots->GetSlot(6);
                own->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");foreign->GetPlayer()->SetDifficultyLevel("Librarian");game->AdvanceTurn(6);
                const auto scout=game->MakeSmacProductionItem(own->m_owner,ProductionItemKind::Unit,2);
                own->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
                auto* target_tile=game->GetMap()->GetTile(9,9);actor->SetTile(GSE_CALL,game->GetMap()->GetTile(8,8));
                Base* target=nullptr;
                for(size_t id:{702u,703u}) {
                    const size_t population=id==702 ? 2 : 1;Base::pops_t pops;
                    for(size_t p=0;p<population;++p) {pops.emplace(std::piecewise_construct,std::forward_as_tuple(19+p),std::forward_as_tuple(nullptr,19+p,bm->GetPopDef(Roles().specialists[1]),0));}
                    const auto item=game->MakeSmacProductionItem(foreign,ProductionItemKind::Unit,2);
                    auto* base=new Base(game,id,foreign,foreign->GetPlayer()->GetFaction(),id==702 ? target_tile : game->GetMap()->GetTile(20,16),"Garrison",{},pops,41,5,
                        ProductionState{item,0,item,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{2,false,0,0,0,false});
                    bm->SpawnBase(GSE_CALL,base);base->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{1,{{},static_cast<int32_t>(population)}},Roles());
                    if(id==702) {target=base;}
                }
                ConventionalUnitState record{2,2,1,0,0,false,ConventionalUnitState::Home::Base,SmacMoraleMode::Mode0};record.home_base_id=702;
                auto* defender=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("live-scout"),foreign,target_tile,record);
                auto data=KnowledgeValue(true,true,128,64).CopyData();data.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
                data.units={{91,6,false},{92,6,false}};for(auto& tile:data.tiles) {tile.explored_by=6;}
                const auto index=9*64+9/2;data.tiles[index].explored_by=4;data.tiles[index].claimant=2;
                data.bases={{702,4,{}}};data.remembered[1].resize(data.tiles.size());data.remembered[1][index]=Knowledge::Improvements{0,0,true,true};
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                fixture.ResourceState([](auto& state){state.hostility.owners.at(1).directions[2].primary=16;state.hostility.owners.at(2).directions[1].primary=16;
                    state.economy.at(1).credits=100;state.economy.at(2).credits=200;state.economy.at(2).population_aggregate=3;
                    state.economy.at(1).regional_base_counts->at(1)=1;state.economy.at(2).regional_base_counts->at(1)=2;});
                fixture.GameplayRandom({3,attacker_wins ? 4u : 3u});fixture.Updates();
                const auto before=fixture.Snapshot();um->ValidateConventionalMove(actor,target_tile,0);
                GT_ASSERT(fixture.Snapshot()==before,"Base attack validation published casualty, status or RNG");
                size_t hooks=0;bool coherent=true;
                um->On(GSE_CALL,"unit_despawn",NATIVE_CALL(target,attacker_wins,&hooks,&coherent){
                    ++hooks;coherent=coherent && target->m_owner->GetIndex()==6 && target->m_pops.size()==2
                        && target->GetSmacResourceContext().recent_defender_loss==attacker_wins;
                    return VALUE(value::Undefined);
                }));
                fixture.MoveEvent(GSE_CALL,actor,target_tile);
                const auto knowledge=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                GT_ASSERT(hooks==1 && coherent && target->m_owner==foreign && target->m_pops.size()==2
                    && !target->GetSmacResourceContext().conquest && target->GetSmacResourceContext().recent_defender_loss==attacker_wins,
                    "Base battle captured ownership/population or published status after casualty callbacks");
                if(attacker_wins) {
                    GT_ASSERT(um->GetUnit(91)==actor && !um->GetUnit(92) && actor->GetTile()==game->GetMap()->GetTile(8,8)
                        && actor->Conventional().damage==7 && actor->Conventional().stored_morale==2 && actor->Conventional().spent==3
                        && fixture.GameplayRandom()==SmacGameplayRandomState(1018897798u,3665287870u) && target_tile->units.empty(),"original garrison victory differs");
                    GT_ASSERT(knowledge.bases.front().known_by==6 && knowledge.observers[1]->relations[2].territory_recognition
                        && knowledge.tiles[index].explored_by==6 && !knowledge.remembered[1][index]->unit_present,"victory lost Base discovery or retained garrison occupancy");
                    const auto exhausted=fixture.Snapshot();
                    GT_ASSERT(Rejects([&]{fixture.MoveEvent(GSE_CALL,actor,target_tile);}) && fixture.Snapshot()==exhausted,"exhausted Scout captured again or changed authority");
                    saved=exhausted;
                } else {
                    GT_ASSERT(!um->GetUnit(91) && um->GetUnit(92)==defender && defender->GetTile()==target_tile
                        && knowledge.bases.front().known_by==4 && !knowledge.observers[1]->relations[2].territory_recognition,
                        "attacker loss published victory-only Base discovery");
                }
                GT_OK();
            });if(!result.empty()) {return result;}
            if(!attacker_wins) {continue;}
            for(const bool capture:{true,false}) {
                ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(BattleSource,true);restored.BasePresentationRoles(names);
                result=restored.Run([&](GSE_CALLABLE)->std::string {
                    // A world download carries the simulation, while its already
                    // admitted lobby supplies Player metadata. Recreate that
                    // same roster; full campaign files store the roster too.
                    restored.Game()->GetState()->m_slots->GetSlot(0).GetPlayer()->SetDifficultyLevel("Librarian");
                    restored.Game()->GetState()->m_slots->GetSlot(6).GetPlayer()->SetDifficultyLevel("Librarian");
                    restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* target=game->GetBM()->GetBase(702);
                    GT_ASSERT(target->GetSmacResourceContext().recent_defender_loss && target->m_pops.size()==2
                        && restored.GameplayRandom()==SmacGameplayRandomState(1018897798u,3665287870u),"Start cleared aftermath or replayed combat");
                    if(capture) {
                        auto* actor=game->GetUM()->GetUnit(91);game->GetUM()->ResetConventionalSpent(GSE_CALL,actor);
                        const auto stats=actor->QueryStats(SmacMoraleMode::Mode1);
                        GT_ASSERT(stats.full_allowance==3 && stats.remaining_allowance==3 && actor->m_owner->GetPlayer()->GetDifficultyLevel()=="Librarian"
                            && target->m_owner->GetPlayer()->GetDifficultyLevel()=="Librarian","restored capture input: allowance="+std::to_string(stats.full_allowance)+" remaining="+std::to_string(stats.remaining_allowance)
                            +" attacker="+actor->m_owner->GetPlayer()->GetDifficultyLevel()+" defender="+target->m_owner->GetPlayer()->GetDifficultyLevel());
                        restored.MoveEvent(GSE_CALL,actor,target->GetTile());
                        GT_ASSERT(target->m_owner==actor->m_owner && target->m_pops.size()==1 && !target->GetSmacResourceContext().recent_defender_loss
                            && target->GetSmacResourceContext().conquest && actor->GetTile()==target->GetTile(),"separate admitted capture did not clear combat aftermath");
                    } else {
                        target->ProcessSmacUpkeep(GSE_CALL,Roles(),[&](int32_t){return game->GetUM()->GetUnitDef("live-scout");},SmacMoraleMode::Mode0);
                        GT_ASSERT(!target->GetSmacResourceContext().recent_defender_loss,"ordinary Base upkeep did not clear aftermath at its tail");
                    }GT_OK();
                });if(!result.empty()) {return result;}
            }
        }GT_OK();
    });

    task->AddTest("SMAC live friendly stack movement retains Base occupancy and saved sight",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");std::string saved;
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* um=game->GetUM();auto* actor=um->GetUnit(91);
            const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            base->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
            auto* outside=game->GetMap()->GetTile(3,3);actor->SetTile(GSE_CALL,outside);
            const ConventionalUnitState home{2,1,1,0,0,false,ConventionalUnitState::Home::Base,SmacMoraleMode::Mode0,base->m_id};
            auto* resident=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("live-scout"),base->m_owner,base->GetTile(),home);
            auto data=KnowledgeValue(true,true,128,64).CopyData();data.units.push_back({92,0,false});for(auto& tile:data.tiles) {tile.explored_by=2;}
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
            const auto resident_wire=Unit::Serialize(resident).ToString();const auto random=fixture.GameplayRandom();fixture.Updates();
            fixture.MoveEvent(GSE_CALL,actor,base->GetTile());
            auto observed=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
            GT_ASSERT(base->GetTile()->units.size()==2 && outside->units.empty() && observed.units.size()==2
                && observed.remembered[1].at(2*64+1)->unit_present && !observed.remembered[1].at(3*64+1)->unit_present,
                "live occupied Base entry lost stack members or retained the wrong source/destination sight markers");
            um->ResetConventionalSpent(GSE_CALL,actor);fixture.MoveEvent(GSE_CALL,actor,outside);
            observed=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
            GT_ASSERT(base->GetTile()->units.size()==1 && base->GetTile()->units.at(92)==resident && actor->GetTile()==outside
                && observed.remembered[1].at(2*64+1)->unit_present && observed.remembered[1].at(3*64+1)->unit_present
                && Unit::Serialize(resident).ToString()==resident_wire && fixture.GameplayRandom()==random,
                "live stack departure cleared a resident's occupancy, changed it or consumed randomness");
            um->ResetConventionalSpent(GSE_CALL,actor);fixture.MoveEvent(GSE_CALL,actor,base->GetTile());saved=fixture.Snapshot();GT_OK();
        });if(!result.empty()) {return result;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(TanksSource,"GAIANS");
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
            GT_ASSERT(base->GetTile()->units.size()==2 && base->GetTile()->units.at(91)==game->GetUM()->GetUnit(91)
                && base->GetTile()->units.at(92)==game->GetUM()->GetUnit(92)
                && game->GetMap()->GetSmacWorkerKnowledge()->CopyData().remembered[1].at(2*64+1)->unit_present,
                "actual Start changed the live friendly stack or its remembered occupancy");GT_OK();
        });
    });

    task->AddTest("SMAC maintenance insolvency follows live Base order and retains already earned research",[](GSE*,context::Context*)->std::string {
        for(int scenario=0;scenario<5;++scenario) {
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(scenario==4 ? EnergyFacilitySource : MaintenanceSource,true);
            fixture.ResourceState([&](auto& state){for(auto& e:state.economy) {e.second.rates={0,5};e.second.credits=e.first==1 ? (scenario==0 ? 100 : 0) : 321;}
                for(auto& t:state.technology.factions) {t.target=5;t.cached_threshold=1000;t.labs=0;}});
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(names);
            std::string saved;int32_t final_credits=0;
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* first=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();SelectionPresentations(fixture);
                first->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
                bm->DefineFacility(new FacilityDef("hq","HQ","",50,{},1));
                // Presentation keys deliberately reverse the source ordering.
                bm->DefineFacility(new FacilityDef("z-bank","Bank","",80,{},7));bm->DefineFacility(new FacilityDef("a-node","Node","",80,{},8));
                const auto scout=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,2);
                first->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});first->SetAccumulatedNutrients(0);
                std::vector<Base*> bases{first};
                for(int i=1;i<=(scenario==3 ? 2 : scenario==4 ? 0 : 1);++i) {
                    const bool foreign=scenario==3 && i==1;auto* owner=foreign ? &game->GetState()->m_slots->GetSlot(6) : first->m_owner;
                    owner->GetPlayer()->SetDifficultyLevel("Librarian");auto* worked=game->GetMap()->GetTile(4+4*i,2);worked->terraforming=TERRAFORMING_FOREST;
                    Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().worker),0,worked));
                    const auto target=game->MakeSmacProductionItem(owner,ProductionItemKind::Unit,2);
                    auto* base=new Base(game,701+i,owner,owner->GetPlayer()->GetFaction(),game->GetMap()->GetTile(2+4*i,2),"Maintenance control "+std::to_string(i),{},pops,20,0,
                        ProductionState{target,0,target,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{foreign ? 2 : 1,false,0,0,0,false});
                    bm->SpawnBase(GSE_CALL,base);base->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{5,{{},0}},Roles());bases.push_back(base);
                }
                auto* poor=scenario==2 ? bases.back() : first;auto* rich=scenario==2 ? first : bases.back();
                auto poor_allocation=*poor->GetSmacAllocation();poor_allocation.selected=5;poor->PublishSmacAllocation(GSE_CALL,poor_allocation,Roles());
                poor->m_facilities.emplace("z-bank",bm->GetFacilityDef("z-bank"));poor->m_facilities.emplace("a-node",bm->GetFacilityDef("a-node"));
                // Cloud Cover on both squares makes actual poor-Base economy0.
                auto context=poor->GetSmacResourceContext();context.governor_options=0;context.resource_events=SmacBaseResourceContext::CloudCover;context.resource_event_timer=5;
                poor->SetSmacResourceContext(GSE_CALL,context);
                if(scenario!=4) {
                    rich->m_facilities.emplace("hq",bm->GetFacilityDef("hq"));
                    auto rich_context=rich->GetSmacResourceContext();rich_context.governor_options=0;rich->SetSmacResourceContext(GSE_CALL,rich_context);
                    // An explicit Doctor makes the rich Base work no noncenter
                    // tiles. Its actual HQ center supplies E2 -> economy1/labs1.
                    auto allocation=*rich->GetSmacAllocation();allocation.selected=1;allocation.specialists.total_count=1;allocation.specialists.types[0]=1;
                    rich->PublishSmacAllocation(GSE_CALL,allocation,Roles());
                } else {
                    context.resource_events=0;poor->SetSmacResourceContext(GSE_CALL,context);poor->m_facilities.emplace("hq",bm->GetFacilityDef("hq"));
                    poor->m_facilities.erase("z-bank");
                    fixture.ResourceState([](auto& state){state.economy.at(1).rates={5,5};});
                }
                auto knowledge=KnowledgeValue(true,true,128,64).CopyData();knowledge.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
                for(auto& tile:knowledge.tiles) {tile.explored_by=6;}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
                const auto foreign=scenario==3 ? Base::Serialize(bases[1]).ToString() : std::string{};
                const auto pair=fixture.GameplayRandom();const auto next_id=game::backend::unit::Unit::GetNextId();
                if(scenario==3) {
                    // Current-faction return, matching the original foreign
                    // control. A full local turn also processes owner2 later.
                    for(auto* base:{poor,rich}) {base->ProcessSmacUpkeep(GSE_CALL,Roles(),{},game::backend::unit::SmacMoraleMode::Mode0);fixture.PostEconomy(*base);}
                    fixture.SettleEconomy(1);
                } else {game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();}
                const bool bank_kept=poor->m_facilities.count("z-bank"),node_kept=poor->m_facilities.count("a-node");
                final_credits=game->GetCanonicalEconomy(poor->m_owner).credits;
                const auto expected=scenario==0 ? 99 : 80;
                GT_ASSERT(final_credits==expected && bank_kept==(scenario==0 || scenario==2) && node_kept==(scenario!=2 && scenario!=4),
                    "maintenance order, exact-zero payment or assignment of sale proceeds differs: scenario="+std::to_string(scenario)+" credits="+std::to_string(final_credits));
                GT_ASSERT(fixture.GameplayRandom()==pair && game::backend::unit::Unit::GetNextId()==next_id,"maintenance changed gameplay RNG or Unit identity");
                if(scenario==3) {GT_ASSERT(Base::Serialize(bases[1]).ToString()==foreign && game->GetCanonicalEconomy(bases[1]->m_owner).credits==321,"current-faction maintenance touched the foreign control");}
                if(scenario==4) {
                    int32_t earned=0;fixture.ResourceState([&](auto& state){earned=state.technology.factions[0].labs;});
                    const auto after=game->GetCanonicalBaseEnergy(*poor);
                    GT_ASSERT(earned==3 && after.tail.final.labs==2,"maintenance reposted research or lost the pre-sale Node output: earned="+std::to_string(earned)+" after="+std::to_string(after.tail.final.labs)+" raw="+std::to_string(after.raw_intake));
                }
                saved=fixture.Snapshot();GT_OK();
            });if(!result.empty()) {return result;}
            if(scenario==1 || scenario==2) {
                ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(MaintenanceSource,true);
                const auto result=restored.Run([&](GSE_CALLABLE)->std::string {
                    restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* base=restored.Game()->GetBM()->GetBase(scenario==2 ? 702 : 701);
                    GT_ASSERT(restored.Game()->GetCanonicalEconomy(base->m_owner).credits==final_credits && base->m_facilities.count("z-bank")==size_t(scenario==2)
                        && base->m_facilities.count("a-node")==size_t(scenario!=2),"actual Start replayed a sale or restored the removed facility");GT_OK();
                });if(!result.empty()) {return result;}
            }
        }GT_OK();
    });

    task->AddTest("SMAC HQ construction joins relocation resource history and all affected Base refreshes",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        for(const bool late_failure:{false,true}) {
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(EnergyFacilitySource,true);
            std::string saved,retained_history,foreign_record;
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* old=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();auto* um=game->GetUM();SelectionPresentations(fixture);
                bm->DefineFacility(new FacilityDef("hq","HQ","",50,{},1));bm->DefineFacility(new FacilityDef("tanks","Tanks","",40,{},3));
                old->m_facilities.emplace("hq",bm->GetFacilityDef("hq"));
                const auto scout=game->MakeSmacProductionItem(old->m_owner,ProductionItemKind::Unit,2);
                old->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
                std::vector<Base*> bases{old};
                for(size_t i=1;i<=3;++i) {
                    auto* owner=i==3 ? &game->GetState()->m_slots->GetSlot(6) : old->m_owner;
                    const int32_t simulation=i==3 ? 2 : 1;auto* worked=game->GetMap()->GetTile(4+4*i,2);worked->terraforming=TERRAFORMING_FOREST;
                    Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().worker),0,worked));
                    const auto target=game->MakeSmacProductionItem(owner,ProductionItemKind::Unit,2);
                    auto* base=new Base(game,701+i,owner,owner->GetPlayer()->GetFaction(),game->GetMap()->GetTile(2+4*i,2),"HQ control "+std::to_string(i),{},pops,20,0,
                        ProductionState{target,0,target,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{simulation,false,0,0,0,false});
                    bm->SpawnBase(GSE_CALL,base);base->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{5,{{},0}},Roles());bases.push_back(base);
                    if(i>=2) {base->m_facilities.emplace("hq",bm->GetFacilityDef("hq"));base->m_facilities.emplace("tanks",bm->GetFacilityDef("tanks"));}
                }
                auto knowledge=KnowledgeValue(true,true,128,64).CopyData();knowledge.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
                for(auto& tile:knowledge.tiles) {tile.explored_by=6;}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));old->RecalculateSmacWorkers(GSE_CALL,Roles());
                const auto old_history=SerializeSmacBaseUpkeepCache(old->GetSmacUpkeepCache()).ToString();
                auto* current=bases[1];const auto hq=game->MakeSmacProductionItem(current->m_owner,ProductionItemKind::Facility,1);
                current->SetProductionState(GSE_CALL,ProductionState{hq,46,scout,17,std::vector<ProductionItem>{hq,scout}});current->SetAccumulatedNutrients(0);
                const auto foreign=Base::Serialize(bases[3]).ToString();fixture.Updates();const auto before=fixture.Snapshot();
                const auto pair=fixture.GameplayRandom();const auto next=Unit::GetNextId();auto roles=Roles();if(late_failure) {roles.specialists[6]="missing-role";}
                if(late_failure) {
                    GT_ASSERT(Rejects([&]{current->ProcessSmacUpkeep(GSE_CALL,roles,{},SmacMoraleMode::Mode0);}) && fixture.Snapshot()==before && fixture.Updates().empty(),
                        "late HQ failure published relocation, resources, Pop state or refresh queues");GT_OK();
                }
                const auto completed=current->ProcessSmacUpkeep(GSE_CALL,roles,{},SmacMoraleMode::Mode0);
                GT_ASSERT(!old->m_facilities.count("hq") && current->m_facilities.count("hq")==1 && !bases[2]->m_facilities.count("hq")
                    && bases[2]->m_facilities.count("tanks")==1 && Base::Serialize(bases[3]).ToString()==foreign,"HQ relocation missed an owned HQ, removed another facility or changed the foreign control");
                GT_ASSERT(completed.first_calculation.raw_intake[2]==3 && completed.final_calculation && completed.final_calculation->raw_intake[2]==4
                    && current->GetProductionState()->minerals==0 && current->GetProductionState()->current_item==scout
                    && SerializeSmacBaseUpkeepCache(old->GetSmacUpkeepCache()).ToString()==old_history,"HQ grant was not visible in the later pass, queue or retained old-Base history");
                GT_ASSERT(game->GetCanonicalBaseEnergy(*current).headquarters_distance==0 && game->GetCanonicalBaseEnergy(*old).headquarters_distance>0,
                    "live energy distance did not use the relocated HQ");
                std::set<size_t> refreshed;for(const auto& update:fixture.Updates()) {if(update.type==game::FrontendRequest::FR_BASE_UPDATE) {refreshed.insert(update.data.base_update.base_id);}}
                GT_ASSERT(refreshed==std::set<size_t>({701,702,703}) && Unit::GetNextId()==next && fixture.GameplayRandom()==pair,
                    "HQ relocation omitted another Base's refresh or changed Unit identities/RNG");
                saved=fixture.Snapshot();retained_history=old_history;foreign_record=foreign;GT_OK();
            });if(!result.empty()) {return "HQ relocation: "+result;}
            if(!late_failure) {
                ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(EnergyFacilitySource,true);
                const auto restore=restored.Run([&](GSE_CALLABLE)->std::string {
                    restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* bm=restored.Game()->GetBM();
                    GT_ASSERT(!bm->GetBase(701)->m_facilities.count("hq") && bm->GetBase(702)->m_facilities.count("hq")==1
                        && !bm->GetBase(703)->m_facilities.count("hq") && Base::Serialize(bm->GetBase(704)).ToString()==foreign_record
                        && SerializeSmacBaseUpkeepCache(bm->GetBase(701)->GetSmacUpkeepCache()).ToString()==retained_history,
                        "actual Start replayed HQ relocation, changed old resource history or touched the foreign Base");GT_OK();
                });if(!restore.empty()) {return "HQ restore: "+restore;}
            }
        }GT_OK();
    });
    task->AddTest("SMAC installed facility details use frozen rules current free fee and real view callbacks",[](GSE*,context::Context*)->std::string {
        for(const bool free:{false,true}) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction([&](const std::string& name){
                auto source=HabitationSource(name);
                if(free && name=="gaians.txt") {const auto at=source.find("\n#END");source.insert(at,std::string(source.at(at-1)==',' ? "" : ",")+"FACILITY,24");}
                return source;
            },"GAIANS");
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();
                bm->DefineFacility(new FacilityDef("detail","Presentation name","A registered description with enough words to wrap across several lines safely.",9999,{"facs/fac024.pcx",2,3,20,30},24));
                base->m_facilities.emplace("detail",bm->GetFacilityDef("detail"));
                const auto stockpile=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
                base->SetProductionState(GSE_CALL,ProductionState{stockpile,0,stockpile,0,std::vector<ProductionItem>{}});
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));fixture.Updates();
                const auto before=fixture.Snapshot();const auto rng=fixture.GameplayRandom();
                auto* wrapped=static_cast<value::Object*>(base->Wrap(GSE_CALL));
                auto* detail=static_cast<value::Object*>(Call(GSE_CALL,wrapped,"get_smac_facility_details",{VALUE(value::String,,"detail")}));
                GT_ASSERT(static_cast<value::String*>(detail->Get("name"))->value=="Hab Complex"
                    && static_cast<value::Int*>(detail->Get("cost"))->value==80
                    && static_cast<value::Int*>(detail->Get("listed_maintenance"))->value==2
                    && static_cast<value::Int*>(detail->Get("current_maintenance"))->value==(free ? 0 : 2)
                    && static_cast<value::String*>(detail->Get("prerequisite"))->value==game->GetState()->m_settings.global.GetSmacTechnologyRules()->technologies[6].name
                    && static_cast<value::String*>(detail->Get("art"))->value=="facs/fac024.pcx:crop(2,3,21,32)","details used presentation cost/name, forgot free maintenance or lost frozen prerequisite/art bounds");
                auto* setup=static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_base_facility_details"));
                auto* control=static_cast<value::Callable*>(setup->Run(GSE_CALL,{game->Wrap(GSE_CALL),base->Wrap(GSE_CALL)}));
                for(const auto* action:{"row","close","open","escape","open","turn","open","refresh"}) {
                    control->Run(GSE_CALL,{VALUE(value::String,,"detail"),VALUE(value::String,,action)});
                }
                control->Run(GSE_CALL,{VALUE(value::String,,"missing"),VALUE(value::String,,"missing")});
                GT_ASSERT(fixture.Snapshot()==before && fixture.GameplayRandom()==rng && fixture.Updates().empty(),"view/cancel/reopen or optional private providers mutated saved authority or requests");
                bm->DefineFacility(new FacilityDef("unresolved","Unresolved facility","",10,{}));base->m_facilities.emplace("unresolved",bm->GetFacilityDef("unresolved"));
                auto* missing=static_cast<value::Object*>(Call(GSE_CALL,wrapped,"get_smac_facility_details",{VALUE(value::String,,"unresolved")}));
                GT_ASSERT(missing->value.count("cost")==0 && missing->value.count("current_maintenance")==0 && missing->value.count("prerequisite")==0
                    && missing->value.count("art")==0 && static_cast<value::String*>(missing->Get("name"))->value=="Unresolved facility","absent metadata invented rules or artwork");GT_OK();
            });if(!result.empty()) {return result;}
        }GT_OK();
    });
    task->AddTest("SMAC installed facility details withhold foreign private cost and fee",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(HabitationSource,true);
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false,false,6,2);auto* bm=game->GetBM();
            bm->DefineFacility(new FacilityDef("detail","Displayed","",80,{},24));base->m_facilities.emplace("detail",bm->GetFacilityDef("detail"));
            const auto before=fixture.Snapshot();auto* detail=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_facility_details",{VALUE(value::String,,"detail")}));
            GT_ASSERT(detail->value.count("cost")==0 && detail->value.count("current_maintenance")==0
                && static_cast<value::String*>(detail->Get("name"))->value=="Hab Complex" && fixture.Snapshot()==before,"foreign information granted private pricing or hid public identity");GT_OK();
        });
    });
    task->AddTest("SMAC habitation picker completion queue maintenance and actual Start",[](GSE*,context::Context*)->std::string {
        std::string saved;std::array<std::string,8> names;names[0]=Roles().worker;
        for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(HabitationSource,"GAIANS");fixture.BasePresentationRoles(names);
        fixture.ResourceState([](auto& state){state.technology.factions[0].owned[8]=true;});
        const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();SelectionPresentations(fixture);
            bm->DefineFacility(new FacilityDef("complex","Hab Complex","",80,{},24));
            bm->DefineFacility(new FacilityDef("dome","Habitation Dome","",160,{},25));
            const auto complex=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Facility,24);
            const auto dome=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Facility,25);
            const auto stockpile=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
            base->SetProductionState(GSE_CALL,ProductionState{stockpile,0,stockpile,0,std::vector<ProductionItem>{}});
            auto offered=[&](int32_t id){auto* menu=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_choices"));const auto& choices=static_cast<value::Array*>(menu->Get("choices"))->value;return std::any_of(choices.begin(),choices.end(),[&](auto* row){return static_cast<value::Int*>(static_cast<value::Object*>(row)->Get("source_id"))->value==-id;});};
            GT_ASSERT(offered(24) && !offered(25),"Dome ignored its installed Complex dependency");
            fixture.ResourceState([](auto& state){state.technology.factions[0].owned[6]=false;});
            GT_ASSERT(!offered(24),"Complex ignored its actual technology prerequisite");
            fixture.ResourceState([](auto& state){state.technology.factions[0].owned[6]=true;});
            GT_ASSERT(game->IsSmacFacilityProductionEligible(*base,dome,bm->GetFacilityDef("complex")),"queue admission did not see the staged Complex");
            auto* setup=static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_base_production_selection"));
            auto* picker=static_cast<value::Callable*>(setup->Run(GSE_CALL,{game->Wrap(GSE_CALL),base->Wrap(GSE_CALL)}));
            picker->Run(GSE_CALL,{VALUE(value::Int,,-24),VALUE(value::String,,"open")});
            picker->Run(GSE_CALL,{VALUE(value::Int,,-24),VALUE(value::String,,"ok")});fixture.DrainEvents();
            GT_ASSERT(base->GetProductionState()->current_item==complex,"actual picker did not select Complex");
            base->SetProductionState(GSE_CALL,ProductionState{complex,80,complex,80,std::vector<ProductionItem>{dome,stockpile}});
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));base->SetAccumulatedNutrients(0);
            base->ProcessSmacUpkeep(GSE_CALL,Roles(),{},game::backend::unit::SmacMoraleMode::Mode0);
            GT_ASSERT(base->m_facilities.count("complex")==1 && base->GetProductionState()->current_item==dome
                && !offered(24) && offered(25) && game->GetCanonicalBaseEnergy(*base).maintenance==2,
                "Complex completion missed its fee, duplicate guard or dependent queue target");
            fixture.ResourceState([](auto& state){state.technology.factions[0].owned[8]=false;});
            GT_ASSERT(!offered(25),"Dome ignored its actual technology prerequisite");
            fixture.ResourceState([](auto& state){state.technology.factions[0].owned[8]=true;});
            base->SetProductionState(GSE_CALL,ProductionState{dome,160,dome,160,std::vector<ProductionItem>{stockpile}});
            base->ProcessSmacUpkeep(GSE_CALL,Roles(),{},game::backend::unit::SmacMoraleMode::Mode0);
            GT_ASSERT(base->m_facilities.count("dome")==1 && base->GetProductionState()->current_item==stockpile
                && !offered(25) && game->GetCanonicalBaseEnergy(*base).maintenance==6,"Dome grant lost Complex, maintenance or queue advancement");
            saved=fixture.Snapshot();GT_OK();
        });if(!first.empty()) {return first;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(HabitationSource,"GAIANS");restored.BasePresentationRoles(names);
        restored.ProductionPresentations({"choice-scout","choice-rover","choice-former","choice-colony"});
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* base=restored.Game()->GetBM()->GetBase(701);
            GT_ASSERT(base && base->m_facilities.count("complex")==1 && base->m_facilities.count("dome")==1
                && base->GetProductionState()->current_item.kind==ProductionItemKind::Stockpile
                && restored.Game()->GetCanonicalBaseEnergy(*base).maintenance==6,"actual Start lost habitation grants or replayed production");GT_OK();
        });
    });
    task->AddTest("SMAC habitation completion lifts same-pass housing with private rollback",[](GSE*,context::Context*)->std::string {
        for(const int32_t population:{7,14}) {for(int scenario=0;scenario<3;++scenario) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction([&](const std::string& name){return HabitationGrowthSource(name,population);},"GAIANS");
            fixture.ResourceState([](auto& state){state.technology.factions[0].owned[8]=true;});
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,true,false,0,1,population);auto* bm=game->GetBM();
                bm->DefineFacility(new FacilityDef("complex","Hab Complex","",80,{},24));bm->DefineFacility(new FacilityDef("dome","Habitation Dome","",160,{},25));
                if(population==14) {base->m_facilities.emplace("complex",bm->GetFacilityDef("complex"));}
                const int32_t source=population==7 ? 24 : 25;
                const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Facility,source);
                const auto stockpile=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
                base->SetAccumulatedNutrients(10*(population+1));
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,false,128,64));
                const auto intake=base->PrepareSmacResourceCalculation();
                GT_ASSERT(intake.nutrients.surplus==4,"controlled center input did not prepare actual surplus4");
                const auto cost=game->GetSmacProductionCost(base->m_owner,target);
                const auto stored=scenario==0 ? cost-intake.minerals.surplus-1 : cost;
                base->SetProductionState(GSE_CALL,ProductionState{target,stored,target,stored,std::vector<ProductionItem>{stockpile}});
                const auto before=fixture.Snapshot();const auto random=fixture.GameplayRandom();fixture.Updates();
                auto roles=Roles();if(scenario==2) {roles.specialists[6]="missing-role";}
                if(scenario==2) {
                    GT_ASSERT(Rejects([&]{base->ProcessSmacUpkeep(GSE_CALL,roles,{},game::backend::unit::SmacMoraleMode::Mode0);})
                        && fixture.Snapshot()==before && fixture.GameplayRandom()==random && fixture.Updates().empty(),"late habitation failure leaked grant, growth or stores");GT_OK();
                }
                const auto completed=base->ProcessSmacUpkeep(GSE_CALL,roles,{},game::backend::unit::SmacMoraleMode::Mode0);
                GT_ASSERT(base->m_pops.size()==size_t(population+(scenario==1))
                    && base->m_facilities.count(source==24 ? "complex" : "dome")==size_t(scenario==1),"new grant was not visible to same-pass housing or incomplete grant escaped");
                GT_ASSERT(completed.first_calculation.nutrients.surplus==4 && base->GetAccumulatedNutrients()==(scenario==1 ? 2 : 5*(population+1)+4),
                    "housing completion recollected old nutrients or used the wrong growth resource pass");GT_OK();
            });if(!result.empty()) {return "housing "+std::to_string(population)+" scenario "+std::to_string(scenario)+": "+result;}
        }}GT_OK();
    });
    task->AddTest("SMAC Energy Bank and Node construction reaches live credit and research consumers",[](GSE*,context::Context*)->std::string {
        for(const int32_t source:{7,8}) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(EnergyFacilitySource,"GAIANS");
            fixture.ResourceState([&](auto& state){state.economy.at(1).rates={0,5};state.economy.at(1).credits=100;
                auto& technology=state.technology.factions[0];technology.target=5;technology.cached_threshold=1000;technology.labs=0;});
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i){names[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(names);
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();SelectionPresentations(fixture);
                base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
                bm->DefineFacility(new FacilityDef("hq","HQ","",50,{},1));base->m_facilities.emplace("hq",bm->GetFacilityDef("hq"));
                bm->DefineFacility(new FacilityDef("built","Built facility","",80,{},source));
                auto allocation=*base->GetSmacAllocation();allocation.selected=5;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());base->SetAccumulatedNutrients(0);
                const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Facility,source);
                const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
                base->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
                const auto native=PublishUnrelatedNative(GSE_CALL,fixture);
                auto* setup=static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_base_production_selection"));
                auto* picker=static_cast<value::Callable*>(setup->Run(GSE_CALL,{game->Wrap(GSE_CALL),base->Wrap(GSE_CALL)}));
                picker->Run(GSE_CALL,{VALUE(value::Int,,-source),VALUE(value::String,,"open")});
                picker->Run(GSE_CALL,{VALUE(value::Int,,-source),VALUE(value::String,,"ok")});fixture.DrainEvents();
                GT_ASSERT(base->GetProductionState()->current_item==target,"basic facility picker did not install the actual typed target");
                auto posted=*base->GetProductionState();posted.minerals=76;posted.baseline_minerals=76;base->SetProductionState(GSE_CALL,posted);
                const auto before=game->GetCanonicalBaseEnergy(*base);GT_ASSERT(before.raw_intake==4 && before.maintenance==0,"energy facility fixture lost its actual pre-completion inputs");
                game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();const auto after=game->GetCanonicalBaseEnergy(*base);
                GT_ASSERT(base->m_facilities.count("built")==1 && base->GetProductionState()->minerals==0 && after.raw_intake==4 && after.maintenance==1,
                    "basic facility completion omitted its fee or changed raw resources/production");
                bool exact=false;fixture.ResourceState([&](auto& state){const auto& e=state.economy.at(1);const auto& t=state.technology.factions[0];
                    exact=e.credits==(source==7 ? 102 : 101) && t.labs==(source==7 ? 2 : 3);});
                GT_ASSERT(native.Matches(fixture),"Facility completion changed unrelated native body or factory");
                GT_ASSERT(exact && after.tail.final.economy==(source==7 ? 3 : 2) && after.tail.final.labs==(source==7 ? 2 : 3),
                    "completed Bank/Node did not affect the same native turn's economy or research");(void)fixture.Snapshot();GT_OK();
            });if(!result.empty()) {return "energy facility "+std::to_string(source)+": "+result;}
        }GT_OK();
    });

    task->AddTest("SMAC Tanks completion grants before queue and final resources without reposting",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        for(int scenario=0;scenario<3;++scenario) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();auto* um=game->GetUM();
                SelectionPresentations(fixture);bm->DefineFacility(new FacilityDef("tanks","Tanks","",40,{},3));
                bm->DefineFacility(new FacilityDef("hq","HQ","",50,{},1));base->m_facilities.emplace("hq",bm->GetFacilityDef("hq"));
                auto allocation=*base->GetSmacAllocation();allocation.selected=5;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
                const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Facility,3);
                const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
                const auto stockpile=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
                std::vector<ProductionItem> queue=scenario==0 ? std::vector<ProductionItem>{stockpile}
                    : scenario==1 ? std::vector<ProductionItem>{target,scout} : std::vector<ProductionItem>{};
                base->SetProductionState(GSE_CALL,ProductionState{target,36,scout,17,queue});base->SetAccumulatedNutrients(0);
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));fixture.Updates();
                const auto old_unit=Unit::Serialize(um->GetUnit(91)).ToString();const auto next=Unit::GetNextId();const auto pair=fixture.GameplayRandom();const auto random=game->GetRandom()->GetStateString();
                const auto completed=base->ProcessSmacUpkeep(GSE_CALL,Roles(),{},SmacMoraleMode::Mode0);
                const auto& state=*base->GetProductionState();
                GT_ASSERT(!completed.completed_unit && completed.first_calculation.nutrients.surplus==2
                    && completed.first_calculation.minerals.intake==4 && completed.final_calculation && completed.final_calculation->minerals.intake==5
                    && base->m_facilities.size()==2 && base->m_facilities.count("tanks")==1,"Tanks grant missed its native first/second resource boundary N="+std::to_string(completed.first_calculation.nutrients.surplus)
                    +" M="+std::to_string(completed.first_calculation.minerals.intake)+" finalM="+std::to_string(completed.final_calculation ? completed.final_calculation->minerals.intake : -1)
                    +" facilities="+std::to_string(base->m_facilities.size()));
                GT_ASSERT(state.minerals==0 && state.baseline_minerals==0 && state.current_item==state.baseline_item
                    && state.current_item==(scenario==0 ? stockpile : scenario==1 ? scout : target)
                    && state.queue->empty() && completed.queue_advanced==(scenario!=2)
                    && base->GetAccumulatedNutrients()==2,"Tanks completion reposted minerals, used new nutrient surplus early or ignored the granted duplicate in queue");
                GT_ASSERT(Unit::GetNextId()==next && Unit::Serialize(um->GetUnit(91)).ToString()==old_unit
                    && fixture.GameplayRandom()==pair && game->GetRandom()->GetStateString()==random,"facility construction created a Unit or changed unrelated RNG/state");
                if(scenario==2) {
                    base->ProcessSmacUpkeep(GSE_CALL,Roles(),{},SmacMoraleMode::Mode0);
                    GT_ASSERT(base->GetProductionState()->minerals==5 && base->m_facilities.size()==2
                        && base->GetSmacUpkeepCache()->calculation.minerals.intake==5,"already present Tanks completed or added its resource bonus twice");
                }
                (void)fixture.Snapshot();GT_OK();
            });if(!result.empty()) {return "Tanks queue "+std::to_string(scenario)+": "+result;}
        }GT_OK();
    });
    task->AddTest("SMAC Tanks picker follows owned prerequisite and Dome then completes across saved live turns",[](GSE*,context::Context*)->std::string {
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
        fixture.ResourceState([](auto& state){state.economy.at(1).rates={0,0};});
        std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i){names[i+1]=Roles().specialists[i];}
        fixture.BasePresentationRoles(names);std::string saved;
        const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();SelectionPresentations(fixture);
            base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
            bm->DefineFacility(new FacilityDef("tanks","Tanks","",40,{},3));bm->DefineFacility(new FacilityDef("hq","HQ","",50,{},1));
            bm->DefineFacility(new FacilityDef("dome","Dome","",80,{},26));base->m_facilities.emplace("hq",bm->GetFacilityDef("hq"));
            auto allocation=*base->GetSmacAllocation();allocation.selected=5;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
            base->SetAccumulatedNutrients(0);const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
            base->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));fixture.Updates();
            const auto offered=[&] {
                auto* menu=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_choices"));
                const auto& rows=static_cast<value::Array*>(menu->Get("choices"))->value;
                return std::any_of(rows.begin(),rows.end(),[](auto* row){return static_cast<value::Int*>(static_cast<value::Object*>(row)->Get("source_id"))->value==-3;});
            };
            const auto before=fixture.Snapshot();GT_ASSERT(offered() && fixture.Snapshot()==before,"eligible Tanks are absent or picker query mutated the world");
            fixture.ResourceState([](auto& state){state.technology.factions[0].owned[6]=false;});GT_ASSERT(!offered(),"unowned Tanks prerequisite was ignored");
            fixture.ResourceState([](auto& state){state.technology.factions[0].owned[6]=true;});
            base->m_facilities.emplace("dome",bm->GetFacilityDef("dome"));GT_ASSERT(!offered(),"Dome did not suppress redundant Tanks construction");base->m_facilities.erase("dome");
            auto* choices=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_choices"));
            const auto& rows=static_cast<value::Array*>(choices->Get("choices"))->value;value::Object* selected=nullptr;
            for(auto* row:rows) {auto* choice=static_cast<value::Object*>(row);if(static_cast<value::Int*>(choice->Get("source_id"))->value==-3) {selected=choice;}}
            GT_ASSERT(selected && static_cast<value::Int*>(selected->Get("cost"))->value==40,"native picker omitted the frozen Tanks cost");
            auto* setup=static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_base_production_selection"));
            auto* picker=static_cast<value::Callable*>(setup->Run(GSE_CALL,{game->Wrap(GSE_CALL),base->Wrap(GSE_CALL)}));
            picker->Run(GSE_CALL,{VALUE(value::Int,,-3),VALUE(value::String,,"open")});
            picker->Run(GSE_CALL,{VALUE(value::Int,,-3),VALUE(value::String,,"ok")});fixture.DrainEvents();
            GT_ASSERT(base->GetProductionState()->current_item.kind==ProductionItemKind::Facility && base->GetProductionState()->current_item.smac_source_id==3,"actual picker event did not install the Tanks target");
            // A retained canary in an unused sea region distinguishes the
            // global turn5 refresh from owner upkeep/read-only queries.
            auto territory=*game->GetMap()->GetTerritoryTopology()->Summary();territory.site_signals[127]=12345;
            fixture.TerritoryTopology(game->GetMap()->GetTerritoryTopology()->WithTerritorySummary(territory).Serialize());
            for(int turn=0;turn<5;++turn) {
                game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();
                GT_ASSERT(game->GetMap()->GetTerritoryTopology()->Summary()->site_signals[127]==(game->GetTurnId()<5 ? 12345 : 0),
                    "actual global territory refresh ran outside turn modulo4 equals1 or failed to run at turn5");
            }
            GT_ASSERT(game->GetTurnId()==6 && base->GetProductionState()->minerals==20 && !base->m_facilities.count("tanks"),"live turns did not accumulate Tanks minerals exactly once");
            saved=fixture.Snapshot();GT_OK();
        });if(!first.empty()) {return "Tanks picker/first turns: "+first;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(TanksSource,"GAIANS");restored.BasePresentationRoles(names);
        restored.ProductionPresentations({"choice-scout","choice-rover","choice-former","choice-colony"});
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.Game()->GetState()->m_slots->GetSlot(0).GetPlayer()->SetDifficultyLevel("Librarian");
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
            GT_ASSERT(base && base->GetProductionState()->minerals==20 && !base->m_facilities.count("tanks"),"Start replayed the in-progress facility build");
            for(int turn=0;turn<5;++turn) {game->Event(GSE_CALL,"complete_turn",{});restored.DrainEvents();
                GT_ASSERT(base->m_facilities.count("tanks")==size_t(turn==4),"Tanks completed before or after its tenth native production posting");}
            GT_ASSERT(game->GetTurnId()==11 && base->GetProductionState()->minerals==0 && base->GetSmacUpkeepCache()->calculation.minerals.intake==5,
                "saved production omitted its grant or retroactively posted the completed facility's mineral");
            auto* menu=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_choices"));
            const auto& rows=static_cast<value::Array*>(menu->Get("choices"))->value;
            GT_ASSERT(std::none_of(rows.begin(),rows.end(),[](auto* row){return static_cast<value::Int*>(static_cast<value::Object*>(row)->Get("source_id"))->value==-3;}),"completed Tanks remain buildable");GT_OK();
        });
    });
    task->AddTest("SMAC Tanks late completion failure leaves facilities stores workers and queues unchanged",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        for(int scenario=0;scenario<3;++scenario) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(TanksSource,"GAIANS");
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();auto* um=game->GetUM();
                if(scenario!=0) {bm->DefineFacility(new FacilityDef("tanks","Tanks","",40,{},3));}
                const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Facility,3);
                std::vector<ProductionItem> queue;if(scenario==1) {queue.push_back(game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Project,39));}
                base->SetProductionState(GSE_CALL,ProductionState{target,40,target,40,queue});base->SetAccumulatedNutrients(0);
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));fixture.Updates();
                const auto before=fixture.Snapshot();const auto next=Unit::GetNextId();const auto pair=fixture.GameplayRandom();
                auto roles=Roles();if(scenario==2) {roles.specialists[6]="missing-role";}
                GT_ASSERT(Rejects([&]{base->ProcessSmacUpkeep(GSE_CALL,roles,{},SmacMoraleMode::Mode0);}) && fixture.Snapshot()==before
                    && base->m_facilities.empty() && Unit::GetNextId()==next && fixture.GameplayRandom()==pair && fixture.Updates().empty(),
                    "rejected facility completion published a private grant/resource pass or production debit");GT_OK();
            });if(!result.empty()) {return "Tanks rejection "+std::to_string(scenario)+": "+result;}
        }GT_OK();
    });

    task->AddTest("SMAC owner ecology aggregate posts after research once and clears on next pass",[](GSE*,context::Context*)->std::string {
        const auto source=[](const std::string& filename) {
            auto value=Source(filename);
            if(filename=="alpha.txt") {value+="\n#WORLDBUILDER\n";for(int i=0;i<15;++i) {value+="5\n";}}
            return value;
        };
        for(const int32_t score:{1,100}) {
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(source,true,32,16);
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}
            fixture.BasePresentationRoles(names);
            const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();game->AdvanceTurn(5);
                auto* first=Populate(GSE_CALL,fixture,false,false,0,1,1,8,8);SelectionPresentations(fixture);
                Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(29),
                    std::forward_as_tuple(nullptr,29,game->GetBM()->GetPopDef(Roles().specialists[1]),1));
                auto* second=new Base(game,702,first->m_owner,first->m_faction,game->GetMap()->GetTile(20,8),"Second ecology",{},
                    pops,30,0,Production(),SmacBaseResourceContext{1,false,0,0,0x40});
                game->GetBM()->SpawnBase(GSE_CALL,second);second->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{3,{{},0}},Roles());
                for(size_t y=0;y<16;++y) {for(size_t x=y&1;x<32;x+=2) {game->GetMap()->GetTile(x,y)->terraforming=TERRAFORMING_BOREHOLE;}}
                const auto stock=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Stockpile,38);
                for(auto* base:{first,second}) {base->SetAccumulatedNutrients(0);base->SetProductionState(GSE_CALL,ProductionState{stock,0,stock,0,std::vector<ProductionItem>{}});}
                fixture.RetainedPlanet(0,0);
                fixture.ResourceState([&](auto& state) {
                    state.ecology_world=faction::SmacEcologyWorld{1,2,0};state.perihelion=false;state.ecology_history=faction::SmacEcologyHistory{};
                    state.ecology_history->research_aggregate[0]=888;state.ecology_history->research_aggregate[1]=777;state.ecology_history->research_aggregate[2]=222;state.ecology_history->research_aggregate[7]=999;
                    for(auto& tech:state.technology.factions) {tech.score=score;tech.future_count=0;tech.labs=0;tech.target.reset();tech.cached_threshold=1;}
                });
                Knowledge::Data known{32,16,true,true,0,{},{},{{91,0,false}}};known.observers[1]=Knowledge::Observer{false,{}};known.tiles.resize(32*16/2);
                for(auto& tile:known.tiles) {tile.claimant=1;tile.explored_by=2;}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(known));game->Event(GSE_CALL,"complete_turn",{});GT_OK();
            });if(!setup.empty()) {return setup;}fixture.DrainEvents();
            for(int question=0;question<2;++question) {
                const auto step=fixture.Run([&](GSE_CALLABLE)->std::string {
                    auto* game=fixture.Game();auto* pending=Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_research");
                    GT_ASSERT(pending->type==VT_OBJECT,"actual research stage did not suspend before ecology contribution");
                    bool exact=false;fixture.ResourceState([&](auto& state) {exact=state.ecology_history->research_aggregate[1]==(score==100 ? question*20 : 0)
                        && state.ecology_history->research_aggregate[2]==222 && state.ecology_history->research_aggregate[0]==888 && state.ecology_history->research_aggregate[7]==999;});
                    GT_ASSERT(exact,"research saw current Base contribution, stale prior owner total or changed peer words");
                    const auto saved=fixture.Snapshot();GT_ASSERT(!saved.empty(),"research pause could not save retained aggregate");
                    auto* request=static_cast<value::Object*>(pending);
                    const auto* choices=static_cast<value::Array*>(request->Get("choices"));
                    auto* choice=static_cast<value::Object*>(choices->value.front());
                    game->Event(GSE_CALL,"choose_research",{{"request_id",request->Get("request_id")},{"technology_id",choice->Get("id")}});
                    GT_OK();
                });if(!step.empty()) {return step;}fixture.DrainEvents();
            }
            const auto finished=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();const auto* second=game->GetBM()->GetBase(702);
                bool exact=false;fixture.ResourceState([&](auto& state) {exact=state.ecology_history->research_aggregate[1]==(score==100 ? 40 : 0) && state.ecology_history->research_aggregate[2]==0
                    && state.technology.factions.front().score==score+2;});
                GT_ASSERT(exact && second->GetSmacUpkeepCache()->calculation.eco_damage==(score==100 ? 20 : 0),"post-research contribution replaced retained damage or repeated a Base");
                if(score==1) {GT_ASSERT(second->PrepareSmacResourceCalculation().eco_damage==1,"award control did not distinguish cached zero from forced fresh damage");}
                fixture.ResourceState([](auto& state) {state.ecology_history->research_aggregate[1]=777;
                    for(auto& tech:state.technology.factions) {tech.score=0;tech.cached_threshold=1000000;}});
                game->Event(GSE_CALL,"complete_turn",{});GT_OK();
            });if(!finished.empty()) {return finished;}fixture.DrainEvents();
            const auto cleared=fixture.Run([&](GSE_CALLABLE)->std::string {
                bool exact=false;fixture.ResourceState([&](auto& state) {exact=state.ecology_history->research_aggregate[1]==0
                    && state.ecology_history->research_aggregate[0]==888 && state.ecology_history->research_aggregate[7]==999;});
                GT_ASSERT(exact,"next owner pass appended to prior aggregate or cleared peers");GT_OK();
            });if(!cleared.empty()) {return cleared;}
        }GT_OK();
    });
    task->AddTest("SMAC owner ecology aggregate preserves unavailable narrow map history through actual turn",[](GSE*,context::Context*)->std::string {
        for(const int32_t width:{4,6}) {
            const auto source=[width](const std::string& filename) {
                auto value=Source(filename);if(filename=="alpha.txt") {
                    const auto row=std::string("Fixture tiny, 4, 8");const auto at=value.find(row);
                    value.replace(at,row.size(),"Fixture tiny, "+std::to_string(width/2)+", 16");
                    value+="\n#WORLDBUILDER\n";for(int i=0;i<15;++i) {value+="5\n";}
                }return value;
            };
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(source,"GAIANS",width,16);
            std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i) {names[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(names);
            const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* bm=game->GetBM();
                for(const auto* id:{"NUTRIENTS","MINERALS","ENERGY"}) {game->GetRM()->DefineResource(new game::backend::resource::Resource(id,id,{}));}
                bm->DefinePop(new PopDef(Roles().worker,"Worker",{},{},PopDef::PF_TILE_WORKER));
                for(uint8_t i=0;i<7;++i) {bm->DefinePop(new PopDef(Roles().specialists[i],"Specialist",{},{},PopDef::PF_NONE,i));}
                for(int32_t y=0;y<16;++y) {for(int32_t x=y&1;x<width;x+=2) {
                    auto* tile=game->GetMap()->GetTile(x,y);for(auto* corner:tile->elevation.corners) {*corner=0;}tile->Update();
                    tile->moisture=MOISTURE_ARID;tile->rockiness=ROCKINESS_FLAT;tile->features=0;tile->terraforming=0;tile->bonus=BONUS_NONE;
                }}
                auto* owner=&game->GetState()->m_slots->GetSlot(0);
                Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().specialists[1]),1));
                auto* base=new Base(game,701,owner,owner->GetPlayer()->GetFaction(),game->GetMap()->GetTile(2,2),"Narrow ecology",{},pops,20,0,Production(),SmacBaseResourceContext{1,false,0,0,0x40});
                bm->SpawnBase(GSE_CALL,base);base->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{3,{{},0}},Roles());
                const auto stock=game->MakeSmacProductionItem(owner,ProductionItemKind::Stockpile,38);
                base->SetProductionState(GSE_CALL,ProductionState{stock,0,stock,0,std::vector<ProductionItem>{}});
                fixture.PrepareBaseCallbacks();static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_base_growth"))->Run(GSE_CALL,{game->Wrap(GSE_CALL)});
                fixture.RetainedPlanet(0,0);fixture.ResourceState([](auto& state) {
                    state.ecology_world=faction::SmacEcologyWorld{1,2,0};state.perihelion=false;state.ecology_history=faction::SmacEcologyHistory{};
                    state.ecology_history->research_aggregate[1]=777;state.ecology_history->research_aggregate[7]=999;
                });
                GT_ASSERT(!base->PrepareSmacResourceCalculation().eco_damage,"Narrow map fabricated calculable ecology");
                Knowledge::Data known{width,16,true,true,0,{},{},{}};known.observers[1]=Knowledge::Observer{false,{}};known.tiles.resize(width*16/2);
                for(auto& tile:known.tiles) {tile.claimant=1;tile.explored_by=2;}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(known));
                game->Event(GSE_CALL,"complete_turn",{});GT_OK();
            });if(!setup.empty()) {return setup;}fixture.DrainEvents();
            const auto finished=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();bool retained=false;fixture.ResourceState([&](auto& state) {retained=state.ecology_history->research_aggregate[1]==777 && state.ecology_history->research_aggregate[7]==999;});
                GT_ASSERT(game->GetTurnId()==2,"Narrow map failed to advance: turn="+std::to_string(game->GetTurnId()));
                GT_ASSERT(retained,"Narrow map cleared unavailable ecology history");
                GT_ASSERT(game->GetBM()->GetBase(701)->GetSmacUpkeepCache(),"Narrow Base did not commit its resource pass");
                GT_ASSERT(!game->GetBM()->GetBase(701)->GetSmacUpkeepCache()->calculation.eco_damage,"Narrow Base invented ecology damage");GT_OK();
            });if(!finished.empty()) {return finished;}
        }GT_OK();
    });
    task->AddTest("SMAC live ecology uses full radius current inputs and preserves cached history", [](GSE*,context::Context*) -> std::string {
        const auto source=[](const std::string& filename) {
            auto value=Source(filename);
            if(filename=="alpha.txt") {value+="\n#WORLDBUILDER\n";for(int i=0;i<15;++i) {value+="5\n";}}
            return value;
        };
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(source,"GAIANS",32,16);
        fixture.ResourceState([](auto& state) {
            state.ecology_world=faction::SmacEcologyWorld{1,2,0};state.perihelion=false;
            state.ecology_history=faction::SmacEcologyHistory{};
            for(auto& tech:state.technology.factions) {tech.score=100;tech.future_count=0;}
        });
        std::string saved;
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* base=Populate(GSE_CALL,fixture,false,false,0,1,1,4,4);auto* game=fixture.Game();
            fixture.RetainedPlanet(0,0);
            // Every radius position has a Borehole, including unworked and
            // unowned squares. Center selection is an independent retained bit.
            for(size_t y=0;y<8;++y) {for(size_t x=y&1;x<8;x+=2) {game->GetMap()->GetTile(x,y)->terraforming=TERRAFORMING_BOREHOLE;}}
            auto allocation=*base->GetSmacAllocation();
            const auto before=fixture.Snapshot();const auto rng=fixture.GameplayRandom();
            const auto first=base->PrepareSmacResourceCalculation();
            // 21*9 + two explicit selected bits =191 -> floor/8=23.
            // Gross minerals=3 (center1 plus capped worker2); clean allowance16 is consumed by terrain,
            // giving residual7 + mineral3 = burden10. PLANET0, L1,
            // technology100 and Citizen factor2 yield damage20.
            GT_ASSERT(first.minerals.intake==3 && first.eco_damage==20,"live ecology did not consume measured full-radius and gross mineral inputs: minerals="+std::to_string(first.minerals.intake)+", damage="+(first.eco_damage ? std::to_string(*first.eco_damage) : "unavailable"));
            const auto wrapped=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_resource_calculation"));
            GT_ASSERT(static_cast<value::Int*>(wrapped->Get("eco_damage"))->value==20
                && fixture.Snapshot()==before && fixture.GameplayRandom()==rng,"query mutated saved state/RNG or omitted GSE damage");
            auto* remote=game->GetMap()->GetTile(1,3);remote->terraforming|=TERRAFORMING_ROAD;
            GT_ASSERT(base->PrepareSmacResourceCalculation().eco_damage==22,"unworked road was omitted from the full radius");
            allocation.selected&=~1u;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
            GT_ASSERT(base->PrepareSmacResourceCalculation().eco_damage==20,"center worked bit was inferred from automatic center intake");
            remote->terraforming&=~TERRAFORMING_ROAD;allocation.selected|=1u;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
            fixture.ResourceState([](auto& state){state.perihelion=true;});
            GT_ASSERT(base->PrepareSmacResourceCalculation().eco_damage==40,"live perihelion did not double damage");
            fixture.ResourceState([](auto& state){state.perihelion=false;state.ecology_history->clean_minerals[1]=20;});
            GT_ASSERT(base->PrepareSmacResourceCalculation().eco_damage==0,"retained clean history was inferred from installed facilities");
            fixture.ResourceState([](auto& state){state.ecology_history->clean_minerals[1]=0;});
            const auto stockpile=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Stockpile,38);
            base->SetProductionState(GSE_CALL,ProductionState{stockpile,0,stockpile,0,std::vector<ProductionItem>{}});
            Knowledge::Data known{32,16,true,true,0,{},{},{{91,0,false}}};
            known.observers[1]=Knowledge::Observer{false,{}};known.tiles.resize(32*16/2);
            for(auto& tile:known.tiles) {tile.claimant=1;tile.explored_by=2;}
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(known));
            base->SetAccumulatedNutrients(0);
            base->ProcessSmacUpkeep(GSE_CALL,Roles(),{},unit::SmacMoraleMode::Mode0);
            GT_ASSERT(base->GetSmacUpkeepCache() && base->GetSmacUpkeepCache()->calculation.eco_damage==20,"upkeep lost live final resource damage");
            fixture.ResourceState([](auto& state){state.ecology_warming_count=17;state.faction_status.values[1]=0x80004000u;state.faction_status.values[7]=UINT32_MAX;});
            saved=fixture.Snapshot();
            fixture.ResourceState([](auto& state){state.ecology_world.reset();state.ecology_warming_count=777;state.faction_status.values[1]=0;});
            GT_ASSERT(!base->PrepareSmacResourceCalculation().eco_damage
                && base->GetSmacUpkeepCache()->calculation.eco_damage==20,"unknown world inputs fabricated a zero or rewrote retained damage");
            GT_OK();
        });
        if(!result.empty()) {return result;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(source,"GAIANS",32,16);
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
            const auto* base=restored.Game()->GetBM()->GetBase(701);
            GT_ASSERT(base && base->GetSmacUpkeepCache() && base->GetSmacUpkeepCache()->calculation.eco_damage==20,
                "snapshot restore lost historical damage");
            bool retained=false;restored.ResourceState([&](auto& state){retained=state.ecology_warming_count==17 && state.faction_status.values[1]==0x80004000u && state.faction_status.values[7]==UINT32_MAX;});
            GT_ASSERT(retained,"Saved Start reset restored faction status or warming words");
            GT_OK();
        });
    });
    task->AddTest("SMAC ecology cache retains signed damage and migrates legacy absence", [](GSE*,context::Context*) -> std::string {
        UpkeepCacheWire wire;wire.version=1;
        const auto old=DeserializeSmacBaseUpkeepCache(wire.Encode());
        GT_ASSERT(old && !old->calculation.eco_damage,"legacy cache invented eco-damage");
        GT_ASSERT(!DeserializeSmacBaseUpkeepCache(SerializeSmacBaseUpkeepCache(old))->calculation.eco_damage,"migration filled missing history");
        wire.version=3;
        for(const auto damage:{INT32_MIN,-1,0,137,INT32_MAX}) {
            wire.ecology=damage;const auto decoded=DeserializeSmacBaseUpkeepCache(wire.Encode());
            GT_ASSERT(decoded && decoded->calculation.eco_damage==damage
                && SerializeSmacBaseUpkeepCache(decoded).ToString()==wire.Encode().ToString(),"retained eco-damage changed or was capped");
        }
        for(const auto damage:{int64_t(INT32_MIN)-1,int64_t(INT32_MAX)+1}) {
            wire.ecology=damage;GT_ASSERT(CacheError(wire.Encode(),"signed32"),"eco-damage narrowed outside signed32");
        }
        wire.ecology=23;const auto valid=wire.Encode().ToString();
        GT_ASSERT(Rejects([&]{DeserializeSmacBaseUpkeepCache(types::Buffer(valid.substr(0,14+6+13*14+6+13)));}),"truncated eco-damage admitted");
        GT_OK();
    });
    task->AddTest("SMAC citizen cache retains ordered stages and legacy absence", [](GSE*,context::Context*) -> std::string {
        UpkeepCacheWire wire;
        wire.citizens=std::array<int64_t,28>{7,1,0,0,4,0,-2,0, 1,6,0,6,0,
            0,6,0, 1,5,0, 1,3,0, 1,2,0, 1,2,0};
        const auto decoded=DeserializeSmacBaseUpkeepCache(wire.Encode());
        GT_ASSERT(decoded && decoded->calculation.citizens
            && decoded->calculation.citizens->population==7 && decoded->calculation.citizens->excluded==1
            && decoded->calculation.citizens->early.initial.drones==6
            && decoded->calculation.citizens->early.psych.talents==1
            && decoded->calculation.citizens->early.facilities.drones==3
            && decoded->calculation.citizens->after_police.drones==2
            && decoded->calculation.citizens->final.drones==2
            && SerializeSmacBaseUpkeepCache(decoded).ToString()==wire.Encode().ToString(),
            "citizen snapshot changed ordered historical stages");
        auto copy=DeserializeSmacBaseUpkeepCache(SerializeSmacBaseUpkeepCache(decoded));
        copy->calculation.citizens->final.drones=0;
        GT_ASSERT(decoded->calculation.citizens->final.drones==2,"citizen snapshots share storage");
        for(const auto version:{1,2}) {
            wire.version=version;const auto old=DeserializeSmacBaseUpkeepCache(wire.Encode());
            GT_ASSERT(old && !old->calculation.citizens
                && !DeserializeSmacBaseUpkeepCache(SerializeSmacBaseUpkeepCache(old))->calculation.citizens,
                "legacy citizen history was invented on migration");
        }
        wire.version=3;
        for(const auto field:{0u,1u,13u,27u}) {
            auto invalid=wire;(*invalid.citizens)[field]=field==0 ? 128 : -1;
            GT_ASSERT(CacheError(invalid.Encode(),"citizen"),"invalid citizen history admitted");
        }
        (*wire.citizens)[0]=int64_t(INT32_MAX)+1;
        GT_ASSERT(CacheError(wire.Encode(),"signed32"),"citizen population narrowed");
        GT_OK();
    });
    task->AddTest("SMAC Base upkeep cache preserves historical values order and absent state", [](GSE*,context::Context*) -> std::string {
        UpkeepCacheWire wire;
        auto decoded = DeserializeSmacBaseUpkeepCache(wire.Encode());
        GT_ASSERT(decoded && decoded->calculation.raw_intake == (std::array<int64_t,3>{4,3,2})
            && decoded->calculation.nutrients.intake == 4 && decoded->calculation.nutrients.demand == 2
            && decoded->calculation.nutrients.surplus == 2 && decoded->calculation.nutrients.stored_before == 19
            && decoded->calculation.nutrients.normalized_stored == 19 && decoded->calculation.minerals.multiplier_steps == 2
            && decoded->calculation.minerals.intake == 3 && decoded->calculation.minerals.demand == 2
            && decoded->calculation.minerals.surplus == 1 && decoded->calculation.minerals.pre_orbital_surplus == 1,
            "cache lost or recomputed a retained resource field");
        GT_ASSERT(decoded->support.free_allowance == 1 && decoded->support.minerals_per_paid_unit == 2
            && decoded->support.eligible_count == 2 && decoded->support.paid_count == 1 && decoded->support.mineral_support == 2
            && decoded->support.units.size() == 4, "cache changed retained support totals");
        for (size_t i=0; i<wire.units.size(); ++i) {
            GT_ASSERT(decoded->support.units[i].id == static_cast<uint64_t>(wire.units[i].first)
                && static_cast<int64_t>(decoded->support.units[i].disposition) == wire.units[i].second,
                "cache reordered paid/support history by Unit ID");
        }
        GT_ASSERT(SerializeSmacBaseUpkeepCache(decoded).ToString() == wire.Encode().ToString(), "cache writer changed independent typed fixture");
        const auto copy = DeserializeSmacBaseUpkeepCache(SerializeSmacBaseUpkeepCache(decoded));
        decoded->support.units.front().id = 99;
        GT_ASSERT(copy->support.units.front().id == 93, "decoded caches share record storage");
        wire.present = false;
        GT_ASSERT(!DeserializeSmacBaseUpkeepCache(wire.Encode())
            && SerializeSmacBaseUpkeepCache(std::nullopt).ToString() == wire.Encode().ToString(),
            "absent cache became a zero/current calculation"); GT_OK();
    });
    task->AddTest("SMAC Base upkeep cache preserves signed boundaries and rejects narrowing", [](GSE*,context::Context*) -> std::string {
        UpkeepCacheWire wire;
        wire.calculation = {INT64_MIN,INT64_MAX,-7, INT32_MIN,INT32_MAX,-2,INT64_MIN,INT64_MAX,
            INT32_MIN,INT32_MAX,-3,INT32_MIN,INT32_MAX};
        const auto decoded = DeserializeSmacBaseUpkeepCache(wire.Encode());
        GT_ASSERT(decoded && decoded->calculation.raw_intake == (std::array<int64_t,3>{INT64_MIN,INT64_MAX,-7})
            && decoded->calculation.nutrients.intake == INT32_MIN && decoded->calculation.nutrients.demand == INT32_MAX
            && decoded->calculation.nutrients.surplus == -2 && decoded->calculation.nutrients.stored_before == INT64_MIN
            && decoded->calculation.nutrients.normalized_stored == INT64_MAX
            && decoded->calculation.minerals.multiplier_steps == INT32_MIN && decoded->calculation.minerals.intake == INT32_MAX
            && decoded->calculation.minerals.demand == -3 && decoded->calculation.minerals.surplus == INT32_MIN
            && decoded->calculation.minerals.pre_orbital_surplus == INT32_MAX
            && SerializeSmacBaseUpkeepCache(decoded).ToString() == wire.Encode().ToString(),
            "cache narrowed historical signed fields or imposed current-world arithmetic");
        wire.calculation[3] = int64_t(INT32_MAX)+1;
        GT_ASSERT(CacheError(wire.Encode(),"signed32"), "nutrient overflow was narrowed");
        wire = UpkeepCacheWire{}; wire.calculation[12] = int64_t(INT32_MIN)-1;
        GT_ASSERT(CacheError(wire.Encode(),"signed32"), "mineral underflow was narrowed");
        wire = UpkeepCacheWire{}; wire.support[2] = int64_t(INT32_MAX)+1;
        GT_ASSERT(CacheError(wire.Encode(),"signed32"), "support count overflow was narrowed"); GT_OK();
    });
    task->AddTest("SMAC Base upkeep cache rejects malformed framing counts and markings", [](GSE*,context::Context*) -> std::string {
        UpkeepCacheWire wire;
        const auto valid = wire.Encode().ToString();
        GT_ASSERT(Rejects([&]{DeserializeSmacBaseUpkeepCache(types::Buffer(valid.substr(0,valid.size()-1)));}),
            "truncated final marking admitted");
        auto trailing = wire.Encode(); trailing.WriteInt(0);
        GT_ASSERT(CacheError(trailing,"Trailing"), "trailing cache data admitted");
        wire.present = false; trailing = wire.Encode(); trailing.WriteInt(0);
        GT_ASSERT(CacheError(trailing,"Trailing"), "absent cache ignored trailing fields");
        wire.version = 4;
        GT_ASSERT(CacheError(wire.Encode(),"Unsupported"), "unknown cache version admitted");
        for (const auto count : {int64_t(-1), int64_t(SmacUnitSupport::MAX_ORDERED_UNITS)+1,
                int64_t(SmacUnitSupport::MAX_ORDERED_UNITS), int64_t(5)}) {
            wire = UpkeepCacheWire{}; wire.count = count;
            GT_ASSERT(CacheError(wire.Encode(),"cache count"), "wire count reached allocation/records before extent validation");
        }
        wire = UpkeepCacheWire{}; wire.units[1].first = 93;
        GT_ASSERT(CacheError(wire.Encode(),"identity"), "duplicate cached Unit identity admitted");
        wire = UpkeepCacheWire{}; wire.units[0].first = -1;
        GT_ASSERT(CacheError(wire.Encode(),"record"), "negative cached Unit identity admitted");
        wire = UpkeepCacheWire{}; wire.units[0].second = 4;
        GT_ASSERT(CacheError(wire.Encode(),"record"), "unknown support disposition admitted");
        for (const size_t field : {2u,3u,4u}) {
            wire = UpkeepCacheWire{}; ++wire.support[field];
            GT_ASSERT(CacheError(wire.Encode(),"totals"), "support totals detached from saved markings");
        }
        wire = UpkeepCacheWire{}; wire.support[1] = 3;
        GT_ASSERT(CacheError(wire.Encode(),"support cache"), "unsupported paid cost admitted");
        auto invalid = DeserializeSmacBaseUpkeepCache(UpkeepCacheWire{}.Encode());
        invalid->support.units[0].id = uint64_t(INT64_MAX)+1;
        GT_ASSERT(Rejects([&]{SerializeSmacBaseUpkeepCache(invalid);}), "writer narrowed unsigned Unit ID");
        invalid = DeserializeSmacBaseUpkeepCache(UpkeepCacheWire{}.Encode());
        invalid->support.units[0].disposition = static_cast<SmacSupportDisposition>(4);
        GT_ASSERT(Rejects([&]{SerializeSmacBaseUpkeepCache(invalid);}), "writer admitted unknown disposition");
        invalid = DeserializeSmacBaseUpkeepCache(UpkeepCacheWire{}.Encode()); ++invalid->support.paid_count;
        GT_ASSERT(Rejects([&]{SerializeSmacBaseUpkeepCache(invalid);}), "writer admitted inconsistent paid count"); GT_OK();
    });


    task->AddTest("SMAC running founding consumes the manufactured moved Pod and preserves knowledge",[](GSE*,context::Context*) -> std::string {
        for(const bool with_native:{false,true}) {
        std::string saved, new_record, old_record, native_factory; size_t founded_id=0, pod_id=0,native_id=0;
        game::backend::unit::SmacNativeUnitRecord native_record;
        ConventionalUnitFixture fixture; fixture.ConfigureProduction(UpkeepSource,"GAIANS");
        const auto error=fixture.Run([&](GSE_CALLABLE) -> std::string {
            using namespace game::backend::unit;
            auto* game=fixture.Game(); auto* base=Populate(GSE_CALL,fixture,false,false,0,1,2); auto* um=game->GetUM();
            std::array<std::string,8> roles; roles[0]=Roles().worker; for(size_t i=0;i<7;++i) {roles[i+1]=Roles().specialists[i];}
            fixture.BasePresentationRoles(roles);
            game->GetMap()->GetTile(3,1)->terraforming=TERRAFORMING_FOREST;
            game->GetMap()->GetTile(7,3)->terraforming=TERRAFORMING_FOREST;
            um->DefineUnit(new StaticDef("live-colony",um->GetMoraleSet("live-morale"),"Colony",MT_LAND,0,new ColonyMarkerRender()));
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            fixture.ResourceState([](auto& state){state.economy.at(1).regional_base_counts->at(1)=77;});
            const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
            base->SetProductionState(GSE_CALL,ProductionState{target,game->GetSmacProductionCost(base->m_owner,target)-5,target,0,std::vector<ProductionItem>{}});
            base->SetAccumulatedNutrients(0);
            pod_id=*base->ProcessSmacUpkeep(GSE_CALL,Roles(),[&](int32_t){return um->GetUnitDef("live-colony");},SmacMoraleMode::Mode0).completed_unit;
            auto* pod=um->GetUnit(pod_id); fixture.MoveEvent(GSE_CALL,pod,game->GetMap()->GetTile(4,2));
            um->ResetConventionalSpent(GSE_CALL,pod); fixture.MoveEvent(GSE_CALL,pod,game->GetMap()->GetTile(6,2)); um->ResetConventionalSpent(GSE_CALL,pod);
            if(with_native) {
                um->DefineUnit(new StaticDef("MindWorms",um->GetMoraleSet("live-morale"),"Mind Worms",MT_LAND,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
                fixture.ResourceState([](auto& state){if(!state.native_factory) {state.native_factory=faction::SmacNativeFactoryState::Initialize(0,state.gameplay_random);}});
                const auto plan=PrepareSmacNativeSpawn({{{80,40},128,64,true},1,7,9,1},{{{2,2},1}},[](int32_t,int32_t){return SmacNativeSpawnTile{3,false,1,1,false,15};});
                auto birth=um->PrepareEcologyNativeBirths(GSE_CALL,plan);GT_ASSERT(birth.Size()==1,"Founding fixture did not create a native");
                native_id=birth.Candidate(0).m_id;birth.Commit();birth.Notify(GSE_CALL);native_record=um->GetUnit(native_id)->Native();
                fixture.ResourceState([&](auto& state){native_factory=state.native_factory->Serialize().ToString();});
            }
            (void)fixture.Updates();
            const auto knowledge=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
            const auto random=fixture.GameplayRandom(); const auto jsf=game->GetRandom()->GetStateString();
            auto* old_reserved=game->GetBM()->GetWorkingPop({4,2});
            old_record=Base::Serialize(base).ToString(); const auto scout=Unit::Serialize(um->GetUnit(91)).ToString();
            const auto before_query=fixture.Snapshot();
            GT_ASSERT(um->QueryConventionalCommands(pod).actions==std::vector<int32_t>{6},"available Pod lacks visible founding action");
            GT_ASSERT(fixture.Snapshot()==before_query&&fixture.Updates().empty(),"Pod command query changed world or queued updates");
            auto exhausted=pod->Conventional();exhausted.spent=3;um->UpdateConventionalUnit(GSE_CALL,pod,exhausted);
            GT_ASSERT(um->QueryConventionalCommands(pod).actions.empty(),"exhausted Pod exposes founding action");
            um->ResetConventionalSpent(GSE_CALL,pod);fixture.Updates();
            auto* initialize=static_cast<value::Callable*>(fixture.Script(GSE_CALL,"./GLSMAC_data/tests/_unit_commands"));
            auto* ui=static_cast<value::Callable*>(initialize->Run(GSE_CALL,{game->Wrap(GSE_CALL)}));
            auto* pod_wrapper=pod->Wrap(GSE_CALL);
            game->GetState()->TriggerObject(game,"unit_select",ARGS_F(&pod_wrapper){{"unit",pod_wrapper}};});
            ui->Run(GSE_CALL,{VALUE(value::String,,"check"),VALUE(value::String,,"Found Base [B]"),VALUE(value::String,,"Colony Pod — Found a Base on an available site")});
            ui->Run(GSE_CALL,{VALUE(value::String,,"click"),VALUE(value::String,,"Found Base [B]"),VALUE(value::String,,"")});fixture.DrainEvents();
            auto* founded=game->GetMap()->GetTile(6,2)->base;
            GT_ASSERT(game->GetCanonicalEconomy(base->m_owner).regional_base_counts->at(1)==78,"founding recounted a stale regional value instead of incrementing it");
            GT_ASSERT(founded && game->GetBM()->GetBases().size()==2 && !um->GetUnit(pod_id)
                && um->GetUnitRecordOrder()==(with_native ? std::vector<size_t>{91,native_id}:std::vector<size_t>{91}),"actual founding event did not consume Pod or retain survivor order");
            GT_ASSERT(Base::Serialize(base).ToString()==old_record && Unit::Serialize(um->GetUnit(91)).ToString()==scout,
                "founding changed old Base or survivor Unit authority");
            GT_ASSERT(founded->m_facilities.empty() && founded->m_pops.size()==1 && founded->GetProductionState()->current_item.smac_source_id==2
                && founded->GetProductionState()->minerals==game->GetState()->m_settings.global.GetSmacProductionRules()->retool_exemption
                && founded->GetProductionState()->baseline_item==founded->GetProductionState()->current_item
                && founded->GetProductionState()->baseline_minerals==founded->GetProductionState()->minerals
                && founded->GetProductionState()->queue->empty(),"later Base used startup HQ/target/credit or omitted baselines");
            const auto& cache=founded->GetSmacUpkeepCache();const auto fresh=founded->PrepareSmacResourceCalculation();
            GT_ASSERT(cache && cache->calculation.raw_intake==fresh.raw_intake
                && cache->calculation.nutrients.surplus==fresh.nutrients.surplus && cache->calculation.minerals.surplus==fresh.minerals.surplus
                && cache->calculation.nutrients.stored_before==0 && cache->calculation.nutrients.normalized_stored==founded->GetAccumulatedNutrients()
                && cache->support.mineral_support==0 && cache->support.units.size()==1 && cache->support.units.front().id==91
                && cache->support.units.front().disposition==SmacSupportDisposition::Ignored,
                "running founding omitted its actual initial calculation or retained the consumed Pod in support");
            if(with_native) {
                native_record.id=1;std::string factory;fixture.ResourceState([&](auto& state){factory=state.native_factory->Serialize().ToString();});
                GT_ASSERT(um->GetUnit(native_id)->Native()==native_record && factory==native_factory,
                    "Founding lost native fields, dense index repair or retained factory counts");
            }
            const auto after=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
            GT_ASSERT(after.remembered==knowledge.remembered && after.observers==knowledge.observers && after.units.size()+1==knowledge.units.size(),
                "founding replayed memory/observer state or retained Pod observation");
            for(size_t i=0;i<after.tiles.size();++i) {GT_ASSERT(after.tiles[i].explored_by==knowledge.tiles[i].explored_by,"founding revealed another 21 squares");}
            GT_ASSERT(random==fixture.GameplayRandom() && jsf==game->GetRandom()->GetStateString(),"founding spent gameplay or map RNG");
            auto* reserved=game->GetBM()->GetWorkingPop({4,2});
            GT_ASSERT(reserved==old_reserved,"second Base stole the old worked forest");
            bool removed=false, spawned=false;
            for(const auto& update:fixture.Updates()) {
                if(update.type==game::FrontendRequest::FR_UNIT_DESPAWN && update.data.unit_despawn.unit_id==pod_id) {removed=true;}
                if(update.type==game::FrontendRequest::FR_BASE_SPAWN && update.data.base_spawn.base_id==founded->m_id) {spawned=true;}
            }
            GT_ASSERT(removed && spawned,"founding did not publish actual Pod/Base frontend updates");
            founded_id=founded->m_id; new_record=Base::Serialize(founded).ToString(); saved=fixture.Snapshot(); GT_OK();
        }); GT_ASSERT(error.empty(),"running founding: "+error);
        ConventionalUnitFixture restored; restored.ConfigureProduction(UpkeepSource,"GAIANS");
        const auto restored_result=restored.Run([&](GSE_CALLABLE) -> std::string {
            restored.Restore(GSE_CALL,saved,true); auto* game=restored.Game(); auto* founded=game->GetBM()->GetBase(founded_id);
            GT_ASSERT(game->GetCanonicalEconomy(founded->m_owner).regional_base_counts->at(1)==78,"restore reconstructed regional counts from live Bases");
            GT_ASSERT(founded && Base::Serialize(founded).ToString()==new_record && Base::Serialize(game->GetBM()->GetBase(701)).ToString()==old_record
                && !game->GetUM()->GetUnit(pod_id) && game->GetUM()->GetUnitRecordOrder()==(with_native ? std::vector<size_t>{91,native_id}:std::vector<size_t>{91}),"restore replayed founding or resurrected Pod");
            if(with_native) {std::string factory;restored.ResourceState([&](auto& state){factory=state.native_factory->Serialize().ToString();});
                GT_ASSERT(game->GetUM()->GetUnit(native_id)->Native()==native_record && factory==native_factory,"Founding restore lost native state");}
            (void)game->GetSmacWorkerAvailability(*founded); GT_OK();
        });if(!restored_result.empty()) {return restored_result;}
        }GT_OK();
    });
    task->AddTest("SMAC running founding rejects terrain claims and late role failures atomically",[](GSE*,context::Context*) -> std::string {
        ConventionalUnitFixture fixture; fixture.ConfigureProduction(UpkeepSource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE) -> std::string {
            using namespace game::backend::unit;
            auto* game=fixture.Game(); auto* base=Populate(GSE_CALL,fixture,false); auto* um=game->GetUM(); auto* site=game->GetMap()->GetTile(6,2);
            um->DefineUnit(new StaticDef("live-colony",um->GetMoraleSet("live-morale"),"Colony",MT_LAND,0,new ColonyMarkerRender()));
            auto* pod=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("live-colony"),base->m_owner,site,{0,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
            auto knowledge=KnowledgeValue(false,true,128,64).CopyData(); knowledge.units.push_back({92,0,false}); game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
            std::array<std::string,8> roles; roles[0]=Roles().worker; for(size_t i=0;i<7;++i) {roles[i+1]=Roles().specialists[i];}
            fixture.BasePresentationRoles(roles); (void)fixture.Updates();
            for(const int failure:{0,1,2,3,5,4,6}) {
                if(failure==0) {site->features|=FEATURE_XENOFUNGUS;}
                if(failure==1) {site->rockiness=ROCKINESS_ROCKY;}
                if(failure==2) {roles[7]="absent-role";}
                if(failure==3) {auto data=game->GetMap()->GetSmacWorkerKnowledge()->CopyData(); data.tiles.at(2*64+3).claimant=2; game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));}
                if(failure==4) {auto value=pod->Conventional(); value.spent=3; um->UpdateConventionalUnit(GSE_CALL,pod,value);}
                if(failure==5) {fixture.ResourceState([](auto& state){state.retained_base_totals->at(1)=INT32_MAX;});}
                if(failure==6) {fixture.ResourceState([](auto& state){state.remembered_governor[1].reset();});}
                const auto before=fixture.Snapshot(); const auto id=Base::GetNextId();
                GT_ASSERT(Rejects([&]{game->GetBM()->FoundSmacBase(GSE_CALL,*pod,roles);}) && fixture.Snapshot()==before && Base::GetNextId()==id,
                    "rejected founding changed Base/Unit/knowledge or ID");
                site->features=0; site->rockiness=ROCKINESS_FLAT; roles[7]=Roles().specialists[6];
                if(failure==5) {fixture.ResourceState([](auto& state){state.retained_base_totals->at(1)=1;});}
                if(failure==6) {fixture.ResourceState([](auto& state){state.remembered_governor[1]=SmacBaseResourceContext::NewGameGovernorOptions(state.designs.global_difficulty);});}
                if(failure==3) {auto data=game->GetMap()->GetSmacWorkerKnowledge()->CopyData(); data.tiles.at(2*64+3).claimant.reset(); game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));}
            }
            GT_OK();
        });
    });
    task->AddTest("SMAC running founding scans saved designs and uses retained SUPPORT credit",[](GSE*,context::Context*) -> std::string {
        for(const int32_t support:{-2,-1}) {
            ConventionalUnitFixture fixture; fixture.ConfigureProduction(UpkeepSource,"GAIANS");
            const auto error=fixture.Run([&](GSE_CALLABLE) -> std::string {
                using namespace game::backend::unit;
                auto* game=fixture.Game(); auto* base=Populate(GSE_CALL,fixture,false); auto* um=game->GetUM();
                const auto& rows=game->GetState()->m_settings.global.GetSmacPredefinedUnitRules()->catalog.units;
                const auto row=std::find_if(rows.begin(),rows.end(),[](const auto& value){return value.source_id==2;});
                GT_ASSERT(row!=rows.end(),"fixture lacks actual Scout source");
                const int32_t difficulty=support==-2 ? 0 : 5;
                game->GetState()->m_settings.global.difficulty_level=difficulty-3;
                fixture.ResourceState([&](auto& state){
                    state.designs.global_difficulty=difficulty;
                    state.remembered_governor[1]=0x02000020u; // explicit accepted owner template, independent of startup difficulty
                    state.social.factions.front().retained->at(2)=support;
                    state.social.factions.front().current[2]=support==-2 ? 3 : -4;
                    // Explicit owner1 custom ID64, identical score to actual source2.
                    state.designs.owners.front().designs.push_back({64,row->chassis_id,row->weapon_id,row->defense_id,row->reactor,
                        row->cost,row->plan,row->carry,row->abilities,row->faction_exclusion,true,true});
                });
                um->DefineUnit(new StaticDef("live-colony",um->GetMoraleSet("live-morale"),"Colony",MT_LAND,0,new ColonyMarkerRender()));
                auto* site=game->GetMap()->GetTile(6,2);
                auto* pod=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("live-colony"),base->m_owner,site,{0,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
                auto knowledge=KnowledgeValue(false,true,128,64).CopyData(); knowledge.units.push_back({92,0,false});
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
                std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i){roles[i+1]=Roles().specialists[i];}
				fixture.ResourceState([](auto& state){state.victory_history->last_base_expansion_or_advice_turn[1]=777;state.retained_base_totals->at(1)=7;});
                fixture.BasePresentationRoles(roles); game->Event(GSE_CALL,"found_base",{{"unit",pod->Wrap(GSE_CALL)}}); fixture.DrainEvents();
                auto* founded=site->base;
				bool history=false;fixture.ResourceState([&](auto& state){history=state.victory_history->last_base_expansion_or_advice_turn[1]==static_cast<int32_t>(game->GetTurnId()) && state.retained_base_totals->at(1)==8;});
				GT_ASSERT(history,"Actual Colony founding retained old expansion history");
                GT_ASSERT(founded && founded->GetProductionState()->current_item.smac_source_id==64
                    && founded->GetProductionState()->minerals==(support==-2 ? 0 : game->GetState()->m_settings.global.GetSmacProductionRules()->retool_exemption),
                    "founding hardcoded Scout or substituted current SUPPORT for retained credit");
                GT_ASSERT(founded->GetSmacResourceContext().governor_options==0x02000020u
                    && base->GetSmacResourceContext().governor_options==0x40u,"later founding ignored the remembered owner template or copied another Base override"); GT_OK();
            });GT_ASSERT(error.empty(),"founding selection: "+error);
        } GT_OK();
    });
    task->AddTest("SMAC prepared removal composes ordered existing and newborn Unit retirement",[](GSE*,context::Context*) -> std::string {
        ConventionalUnitFixture fixture; fixture.ConfigureProduction(UpkeepSource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE) -> std::string {
            using namespace game::backend::unit;
            auto* game=fixture.Game(); auto* base=Populate(GSE_CALL,fixture,true); auto* um=game->GetUM();
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64)); (void)fixture.Updates();
            const auto before=fixture.Snapshot(); const auto next=Unit::GetNextId();
            {auto creation=um->PrepareManufacturedConventionalUnit(GSE_CALL,*base,2,um->GetUnitDef("live-scout"),SmacMoraleMode::Mode0);
             auto removal=um->PrepareConventionalRemoval(GSE_CALL,{91,creation.Candidate().m_id},&creation);}
            GT_ASSERT(fixture.Snapshot()==before && Unit::GetNextId()==next,"abandoned creation/removal leaked authority");
            auto creation=um->PrepareManufacturedConventionalUnit(GSE_CALL,*base,2,um->GetUnitDef("live-scout"),SmacMoraleMode::Mode0);
            const auto id=creation.Candidate().m_id; auto removal=um->PrepareConventionalRemoval(GSE_CALL,{91,id},&creation);
            creation.Commit(); removal.Commit(); removal.Notify(GSE_CALL);
            GT_ASSERT(removal.Removes(id) && um->GetUnits().empty() && um->GetUnitRecordOrder().empty()
                && base->GetTile()->units.empty() && game->GetMap()->GetSmacWorkerKnowledge()->CopyData().units.empty(),"batch retirement left membership/order/sight");
            size_t despawns=0;
            for(const auto& update:fixture.Updates()) {
                GT_ASSERT(update.type!=game::FrontendRequest::FR_UNIT_SPAWN,"removed newborn leaked a spawn update");
                if(update.type==game::FrontendRequest::FR_UNIT_DESPAWN) {GT_ASSERT(update.data.unit_despawn.unit_id==91,"newborn emitted redundant despawn");++despawns;}
            }
            GT_ASSERT(despawns==1,"existing Unit did not emit one despawn"); GT_OK();
        });
    });

	task->AddTest( "SMAC startup governor marker preserves same-item and unmarked production events", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( SelectionSource, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game=fixture.Game(); auto* base=Populate(GSE_CALL,fixture,false); SelectionPresentations(fixture);
			game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,true,128,64));
			const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
			const ProductionState original{scout,0,scout,0,std::vector<ProductionItem>{scout}};
			const auto allocation=*base->GetSmacAllocation();const auto pair=fixture.GameplayRandom();
			// Eight original prelude configurations, independent of the loss prompt.
			// Same-item and unmarked activation remain unchanged.
			for(uint32_t flags:{0xc0dfbe41u,0x00dfbe41u,0x80dfbe41u,0x40dfbe41u}) {
				for(bool change:{false,true}) {
					base->SetProductionState(GSE_CALL,original);
					auto context=base->GetSmacResourceContext();context.governor_options=flags;base->SetSmacResourceContext(GSE_CALL,context);
					const auto before=fixture.Snapshot();const auto target=change?1:2;
					std::optional<int64_t> shown;
					auto* choices=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(base->Wrap(GSE_CALL)),"get_smac_production_choices"));
					for(auto* entry:static_cast<value::Array*>(choices->Get("choices"))->value) {
						auto* choice=static_cast<value::Object*>(entry);
						if(static_cast<value::Int*>(choice->Get("source_id"))->value==target) {shown=static_cast<value::Int*>(choice->Get("minerals"))->value;}
					}
					GT_ASSERT(shown.has_value(),"supported production choice is absent");
					GT_ASSERT(fixture.Snapshot()==before,"production preview cleared startup automation");
					const auto expected=SerializeProductionState(base->GetProductionState()).ToString();
					game->Event(GSE_CALL,"select_smac_production",{{"base",base->Wrap(GSE_CALL)},
						{"source_id",VALUE(value::Int,,target)},{"expected_state",VALUE(value::String,,expected)},
						{"expected_minerals",VALUE(value::Int,,*shown)},{"confirm",VALUE(value::Bool,,true)}});
					fixture.DrainEvents();
					const auto after=base->GetSmacResourceContext();
					context.governor_options=change && (flags&0x40000000u)?flags&0x3fffffffu:flags;
					GT_ASSERT(after==context && base->GetSmacAllocation()==allocation && fixture.GameplayRandom()==pair,
						"production event changed permissions, allocation or RNG, or lost marker semantics");
					GT_ASSERT(base->GetProductionState()->current_item.smac_source_id==target
						&& base->GetProductionState()->queue==original.queue && base->GetProductionState()->baseline_item==scout,
						"governor override changed queue or baseline");
					GT_ASSERT(DeserializeSmacBaseResourceContext(SerializeSmacBaseResourceContext(after))==std::optional<SmacBaseResourceContext>(after),
						"restore recomputed saved governor state");
				}
			} GT_OK();
		});
	});
	task->AddTest( "SMAC production picker clears marked governor before loss decline", []( GSE*, context::Context* ) -> std::string {
		// RE-SMAC-GOVERNOR-MARKER-CANCEL-001: six literal original compositions.
		// Marked Talent/Librarian are controlled retained-marker states, not startup defaults.
		struct Case { const char* difficulty; uint32_t flags; int32_t minerals; bool prompt; uint32_t after; };
		const Case cases[]{
			{ "Citizen", 0xc0dfbe41u, 7, false, 0x00dfbe41u },
			{ "Specialist", 0x00dfbe41u, 7, false, 0x00dfbe41u },
			{ "Talent", 0xc0dfbe41u, 30, true, 0x00dfbe41u },
			{ "Librarian", 0xc0dfbe41u, 30, true, 0x00dfbe41u },
			{ "Talent", 0x80dfbe41u, 30, true, 0x80dfbe41u },
			{ "Talent", 0x00dfbe41u, 30, true, 0x00dfbe41u },
		};
		for ( const auto& row : cases ) {
			size_t refreshes = 0;
			std::string saved;
			std::optional< ProductionState > saved_production;
			std::optional< SmacBaseResourceContext > saved_context;
			ConventionalUnitFixture fixture; fixture.ConfigureProduction( SelectionSource, "GAIANS" );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false ); SelectionPresentations( fixture );
				game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( false, true, 128, 64 ) );
				base->m_owner->GetPlayer()->SetDifficultyLevel( row.difficulty );
				const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
				const ProductionState original{ scout, row.minerals, scout, 30, std::vector< ProductionItem >{ scout } };
				base->SetProductionState( GSE_CALL, original );
				auto context = base->GetSmacResourceContext(); context.governor_options = row.flags;
				base->SetSmacResourceContext( GSE_CALL, context );
				game->GetBM()->TriggerUpdates( GSE_CALL ); fixture.Updates();
				const auto listener = game->On( GSE_CALL, "update_base", NATIVE_CALL( &refreshes ) { ++refreshes; return VALUE( value::Undefined ); } ) );
				const auto allocation = *base->GetSmacAllocation(); const auto pair = fixture.GameplayRandom();
				const auto global = game->GetState()->m_settings.global.Serialize().ToString();
				const auto before = fixture.Snapshot();
				game->Event( GSE_CALL, "select_smac_production", { { "base", base->Wrap( GSE_CALL ) },
					{ "source_id", VALUE( value::Int,, 1 ) }, { "expected_state", VALUE( value::String,, "retired selection" ) },
					{ "expected_minerals", VALUE( value::Int,, row.prompt ? 20 : 30 ) }, { "confirm", VALUE( value::Bool,, false ) } } );
				fixture.DrainEvents();
				GT_ASSERT( fixture.Snapshot() == before, "stale prelude cleared marked governor state" );
				auto* setup = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_base_production_selection" ) );
				auto* picker = static_cast< value::Callable* >( setup->Run( GSE_CALL, { game->Wrap( GSE_CALL ), base->Wrap( GSE_CALL ) } ) );
				const auto click = [ & ]( const char* action ) { return static_cast< value::Bool* >( picker->Run( GSE_CALL,
					{ VALUE( value::Int,, 1 ), VALUE( value::String,, action ) } ) )->value; };
				GT_ASSERT( click( "open" ) && fixture.Snapshot() == before, "read-only picker changed governor or production" );
				GT_ASSERT( click( "ok" ) == row.prompt && fixture.Snapshot() == before, "picker applied before dispatch or used wrong loss gate" );
				fixture.DrainEvents();
				context.governor_options = row.after;
				GT_ASSERT( base->GetSmacResourceContext() == context, "admitted pre-confirmation did not preserve exact governor permissions" );
				if ( row.prompt ) {
					GT_ASSERT( *base->GetProductionState() == original, "pre-confirmation posted production loss" );
					game->GetBM()->TriggerUpdates( GSE_CALL );
					GT_ASSERT( refreshes == 0 && fixture.Updates().empty(), "governor-only prelude refreshed and closed the confirmation" );
					GT_ASSERT( !click( "cancel" ), "declined loss left the picker open" );
					fixture.DrainEvents();
					GT_ASSERT( *base->GetProductionState() == original && base->GetSmacResourceContext() == context,
						"declining loss restored marked governor or changed production/baseline/queue" );
				} else {
					GT_ASSERT( base->GetProductionState()->current_item.smac_source_id == 1 && base->GetProductionState()->minerals == 30
						&& base->GetProductionState()->baseline_item == scout && base->GetProductionState()->baseline_minerals == 30
						&& base->GetProductionState()->queue == original.queue, "easy no-prompt switch did not restore full baseline" );
				}
				GT_ASSERT( base->GetSmacAllocation() == allocation && fixture.GameplayRandom() == pair
					&& game->GetState()->m_settings.global.Serialize().ToString() == global, "Base override changed allocation, RNG or frozen founding defaults" );
				saved_production = base->GetProductionState(); saved_context = context; saved = fixture.Snapshot();
				game->GetBM()->TriggerUpdates( GSE_CALL );
				game->Off( GSE_CALL, "update_base", listener ); GT_OK();
			} ); GT_ASSERT( error.empty(), "governor loss composition: " + error );
			// A real body restore begins with a fresh destination Map. Its terrain
			// construction must not mutate the source's already-bound knowledge.
			ConventionalUnitFixture restored; restored.ConfigureProduction( SelectionSource, "GAIANS" );
			const auto restore_error = restored.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = restored.Game();
				game->GetState()->m_slots->GetSlot( 0 ).GetPlayer()->SetDifficultyLevel( row.difficulty );
				restored.Restore( GSE_CALL, saved, true ); const auto* base = game->GetBM()->GetBase( 701 );
				GT_ASSERT( base && base->GetSmacResourceContext() == *saved_context && base->GetProductionState() == saved_production,
					"world restore lost pre-confirmation governor state or changed production" ); GT_OK();
			} ); GT_ASSERT( restore_error.empty(), "governor loss restore: " + restore_error );
		} GT_OK();
	} );
	task->AddTest( "SMAC active governor uses saved management and current phase in live allocation", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture;fixture.ConfigureResources(Source,"GAIANS");
		return fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,true);
			game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true));
			auto context=base->GetSmacResourceContext();context.governor_options=0xc0dfbe41u;base->SetSmacResourceContext(GSE_CALL,context);
			const auto before=*base->GetSmacAllocation();const auto pair=fixture.GameplayRandom();auto* pop=&base->m_pops.at(19);
			base->RecalculateSmacWorkers(GSE_CALL,Roles());
			GT_ASSERT(base->GetSmacAllocation()==before,"ordinary active-governor query discarded a valid Doctor");
			base->RecalculateSmacWorkers(GSE_CALL,Roles(),SmacBaseResourcePhase::Upkeep);
			GT_ASSERT(base->GetSmacAllocation()==before && &base->m_pops.at(19)==pop && !pop->GetWorkedTile(),
				"managed upkeep discarded a retained Doctor");
			// Managed upkeep skips old worker collection, not the retained Doctor
			// budget. Use a complete worker allocation to distinguish the phases.
			auto worker=before;worker.selected=3;worker.specialists.total_count=0;
			base->PublishSmacAllocation(GSE_CALL,worker,Roles());
			base->RecalculateSmacWorkers(GSE_CALL,Roles());
			GT_ASSERT(base->GetSmacAllocation()==worker && &base->m_pops.at(19)==pop
				&& pop->GetWorkedTile()==game->GetMap()->GetTile(3,1),"ordinary managed allocation replaced its complete worker");
			base->RecalculateSmacWorkers(GSE_CALL,Roles(),SmacBaseResourcePhase::Upkeep);
			GT_ASSERT(base->GetSmacAllocation()->selected==5 && base->GetSmacAllocation()->specialists.total_count==0
				&& &base->m_pops.at(19)==pop && pop->GetWorkedTile()==game->GetMap()->GetTile(4,2),
				"managed upkeep did not choose the actual productive square");
			GT_ASSERT(base->GetSmacResourceContext()==context && fixture.GameplayRandom()==pair,"worker allocation cleared governor bits or spent RNG");
			GT_OK();
		});
	});
	task->AddTest( "SMAC production picker selects Former and Colony through real events and upkeep", []( GSE*, context::Context* ) -> std::string {
		for ( const int target_id : { 1, 0 } ) {
			ConventionalUnitFixture fixture; fixture.ConfigureProduction( SelectionSource, "GAIANS" );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false, false, 0, 1, 2 );
				game->GetMap()->GetTile( 3, 1 )->terraforming = TERRAFORMING_FOREST;
				SelectionPresentations( fixture ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
				const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
				const auto target = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, target_id );
				base->SetProductionState( GSE_CALL, ProductionState{ scout, game->GetSmacProductionCost( base->m_owner, target ) - 5,
					scout, 0, std::vector< ProductionItem >{ scout } } ); base->SetAccumulatedNutrients( 0 );
				const auto original = *base->GetProductionState(); const auto random = fixture.GameplayRandom(); const auto saved = fixture.Snapshot();
				auto* setup = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_base_production_selection" ) );
				auto* picker = static_cast< value::Callable* >( setup->Run( GSE_CALL, { game->Wrap( GSE_CALL ), base->Wrap( GSE_CALL ) } ) );
				const auto click = [ & ]( const char* action ) { return static_cast< value::Bool* >( picker->Run( GSE_CALL,
					{ VALUE( value::Int,, target_id ), VALUE( value::String,, action ) } ) )->value; };
				GT_ASSERT( click( "open" ) && fixture.Snapshot() == saved, "picker query changed authoritative state" );
				GT_ASSERT( !click( "cancel" ) && *base->GetProductionState() == original, "picker cancel changed production" );
				click( "open" ); GT_ASSERT( !click( "ok" ) && *base->GetProductionState() == original, "UI selection applied before event dispatch" );
				fixture.DrainEvents();
				const auto selected = *base->GetProductionState();
				GT_ASSERT( selected.current_item == target && selected.minerals == original.minerals && selected.baseline_item == original.baseline_item
					&& selected.baseline_minerals == original.baseline_minerals && selected.queue == original.queue
					&& fixture.GameplayRandom() == random, "selection changed baselines, queue, minerals or RNG" );
				const auto result = base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t id ) {
					return game->ResolveSmacProductionPresentation( base->m_owner, id ); }, unit::SmacMoraleMode::Mode0 );
				GT_ASSERT( result.completed_unit && game->GetUM()->GetUnit( *result.completed_unit )->Conventional().design_id == target_id
					&& game->GetUM()->GetUnit( *result.completed_unit )->Conventional().home_base_id == base->m_id
					&& base->GetProductionState()->current_item == scout && base->GetProductionState()->queue->empty(),
					"chosen unit did not manufacture through actual upkeep and saved queue" ); GT_OK();
			} ); GT_ASSERT( error.empty(), "production picker completion: " + error );
		} GT_OK();
	} );
	task->AddTest( "SMAC production picker confirms actual loss and rejects stale or invalid commands atomically", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( SelectionSource, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false ); SelectionPresentations( fixture );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( false, true, 128, 64 ) );
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
			base->SetProductionState( GSE_CALL, ProductionState{ scout, 30, scout, 30, std::vector< ProductionItem >{} } );
			// The generic participant fixture starts at Citizen. Set the actual
			// owner to Librarian while the global cost difficulty is easiest.
			// Manual retooling must still charge the owner's 30-to-20 penalty.
			base->m_owner->GetPlayer()->SetDifficultyLevel( "Librarian" );
			game->GetState()->m_settings.global.difficulty_level = -3; // port encoding of Citizen
			fixture.ResourceState( []( auto& state ) { state.designs.global_difficulty = 0; } );
			GT_ASSERT( unit::SmacStartupDesignState::OriginalDifficulty( base->m_owner->GetPlayer()->GetDifficultyLevel() ) == 3,
				"penalty fixture did not establish the owner's stored Librarian difficulty" );
			const auto original = *base->GetProductionState(); const auto random = fixture.GameplayRandom();
			auto* setup = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_base_production_selection" ) );
			auto* picker = static_cast< value::Callable* >( setup->Run( GSE_CALL, { game->Wrap( GSE_CALL ), base->Wrap( GSE_CALL ) } ) );
			const auto click = [ & ]( const char* action ) { return static_cast< value::Bool* >( picker->Run( GSE_CALL,
				{ VALUE( value::Int,, 1 ), VALUE( value::String,, action ) } ) )->value; };
			click( "open" ); GT_ASSERT( click( "ok" ), "penalty selection did not wait for confirmation" );
			click( "cancel" ); fixture.DrainEvents(); GT_ASSERT( *base->GetProductionState() == original, "penalty cancellation altered production" );
			const auto expected = SerializeProductionState( base->GetProductionState() ).ToString();
			value::object_properties_t args{ { "base", base->Wrap( GSE_CALL ) }, { "source_id", VALUE( value::Int,, 1 ) },
				{ "expected_state", VALUE( value::String,, expected ) }, { "expected_minerals", VALUE( value::Int,, 20 ) }, { "confirm", VALUE( value::Bool,, true ) } };
			click( "open" ); click( "ok" ); GT_ASSERT( !click( "ok" ), "confirmed selection did not enqueue" );
			game->Event( GSE_CALL, "select_smac_production", args ); // duplicate queued old confirmation
			fixture.DrainEvents();
			GT_ASSERT( base->GetProductionState()->current_item.smac_source_id == 1 && base->GetProductionState()->minerals == 20
				&& base->GetProductionState()->baseline_item == scout && base->GetProductionState()->baseline_minerals == 30
				&& fixture.GameplayRandom() == random, "penalty applied twice or baselines/RNG changed" );
			args[ "expected_state" ] = VALUE( value::String,, SerializeProductionState( base->GetProductionState() ).ToString() );
			args[ "source_id" ] = VALUE( value::Int,, 500 ); const auto saved = fixture.Snapshot();
			GT_ASSERT( Rejects( [ & ] { game->Event( GSE_CALL, "select_smac_production", args ); } ) && fixture.Snapshot() == saved,
				"unsupported target changed authority" );
			args[ "source_id" ] = VALUE( value::Int,, 2 ); args[ "expected_minerals" ] = VALUE( value::Int,, 30 );
			game->Event( GSE_CALL, "select_smac_production", args ); fixture.DrainEvents();
			GT_ASSERT( *base->GetProductionState() == original, "valid next command did not restore baseline via manual switch" );
			// Eligibility is read again at dispatch, rather than trusting the UI's
			// previous choice list. The owned technology change is fixture input.
			args[ "expected_state" ] = VALUE( value::String,, expected ); args[ "source_id" ] = VALUE( value::Int,, 1 );
			args[ "expected_minerals" ] = VALUE( value::Int,, 20 );
			game->Event( GSE_CALL, "select_smac_production", args );
			fixture.ResourceState( []( auto& state ) { state.technology.factions[ 0 ].owned[ 6 ] = false; } );
			GT_ASSERT( Rejects( [ & ] { fixture.DrainEvents(); } ) && *base->GetProductionState() == original,
				"queued selection bypassed current technology eligibility" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC production picker rejects another human Base owner", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureTwoResourceParticipants( SelectionSource );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false, false, 6, 2 ); SelectionPresentations( fixture );
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
			base->SetProductionState( GSE_CALL, ProductionState{ scout, 0, scout, 0, std::vector< ProductionItem >{} } );
			const auto before = fixture.Snapshot();
			const value::object_properties_t args{ { "base", base->Wrap( GSE_CALL ) }, { "source_id", VALUE( value::Int,, 2 ) },
				{ "expected_state", VALUE( value::String,, SerializeProductionState( base->GetProductionState() ).ToString() ) },
				{ "expected_minerals", VALUE( value::Int,, 0 ) }, { "confirm", VALUE( value::Bool,, true ) } };
			GT_ASSERT( Rejects( [ & ] { game->Event( GSE_CALL, "select_smac_production", args ); } ) && fixture.Snapshot() == before,
				"local command selected another human's Base production" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC Colony completion debits population before growth and repeats its queue", []( GSE*, context::Context* ) -> std::string {
		for ( const int food : { 0, 20 } ) {
			// Housing2 distinguishes the staged post-debit population from the live P2.
			const auto source = []( const std::string& filename ) { auto text = UpkeepSource( filename );
				if ( filename == "alpha.txt" ) { const auto at = text.find( "#RULES\n" ); size_t row = at + 7; for ( int i = 1; i < 13; ++i ) { row = text.find( '\n', row ) + 1; } text.replace( row, text.find( ',', row ) - row, "2" ); } return text; };
			ConventionalUnitFixture fixture; fixture.ConfigureProduction( source, "GAIANS" );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				using namespace game::backend::unit;
				auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false, false, 0, 1, 2 ); auto* um = game->GetUM();
				game->GetMap()->GetTile( 3, 1 )->terraforming = TERRAFORMING_FOREST;
				um->DefineUnit( new StaticDef( "live-colony", um->GetMoraleSet( "live-morale" ), "Colony port marker", MT_LAND, 0, new ColonyMarkerRender() ) );
				game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
				const auto target = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 0 );
				GT_ASSERT( game->IsSmacUnitProductionEligible( *base, target ), "ordinary generated Colony queue was not admitted" );
				base->SetProductionState( GSE_CALL, ProductionState{ target, game->GetSmacProductionCost( base->m_owner, target ) - 5, target, 77, std::vector< ProductionItem >{} } );
				base->SetAccumulatedNutrients( food );
				const auto result = base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t id ) { return id == 0 ? um->GetUnitDef( "live-colony" ) : nullptr; }, SmacMoraleMode::Mode0 );
				GT_ASSERT( result.completed_unit && result.first_calculation.nutrients.intake == 4 && result.first_calculation.nutrients.surplus == 0
					&& result.first_calculation.minerals.surplus == 5, "Colony did not complete after actual two-worker first pass" );
				auto* pod = um->GetUnit( *result.completed_unit ); const auto& record = pod->Conventional();
				GT_ASSERT( record.design_id == 0 && record.home_base_id == base->m_id && record.stored_morale == 1 && !record.damage && !record.spent
					&& pod->GetTile() == base->GetTile() && pod->QueryStats( SmacMoraleMode::Mode1 ).full_allowance == 3
					&& um->GetUnit( 91 )->Conventional().home == ConventionalUnitState::Home::None, "Pod identity/home/stats or existing Scout home differs" );
				GT_ASSERT( base->m_pops.size() == ( food ? 2u : 1u ) && base->GetAccumulatedNutrients() == 0
					&& result.final_calculation->minerals.surplus == ( food ? 5 : 3 ) && result.final_calculation->nutrients.surplus == ( food ? 0 : 1 )
					&& base->GetProductionState()->minerals == 0 && base->GetProductionState()->baseline_minerals == 0
					&& base->GetProductionState()->current_item == target && base->GetProductionState()->baseline_item == target
					&& base->GetProductionState()->queue->empty(), "Colony debit/growth/forced pass or baselines differ" ); GT_OK();
			} ); GT_ASSERT( error.empty(), "Colony completion: " + error );
		} GT_OK();
	} );
	task->AddTest( "SMAC Colony moves immediately and restores exact homed continuation", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source; source.ConfigureProduction( UpkeepSource, "GAIANS" ); std::string saved, record; size_t id = 0;
		const auto error = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = source.Game(); auto* base = Populate( GSE_CALL, source, false, false, 0, 1, 2 ); auto* um = game->GetUM();
			game->GetMap()->GetTile( 3, 1 )->terraforming = TERRAFORMING_FOREST;
			um->DefineUnit( new StaticDef( "live-colony", um->GetMoraleSet( "live-morale" ), "Colony port marker", MT_LAND, 0, new ColonyMarkerRender() ) );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
			const auto target = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 0 );
			base->SetProductionState( GSE_CALL, ProductionState{ target, game->GetSmacProductionCost( base->m_owner, target ) - 5, target, 0, std::vector< ProductionItem >{} } ); base->SetAccumulatedNutrients( 0 );
			id = *base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t ) { return um->GetUnitDef( "live-colony" ); }, SmacMoraleMode::Mode0 ).completed_unit;
			auto* pod = um->GetUnit( id ); source.MoveEvent( GSE_CALL, pod, game->GetMap()->GetTile( 4, 2 ) );
			GT_ASSERT( pod->GetTile() == game->GetMap()->GetTile( 4, 2 ) && pod->Conventional().spent == 6 && !pod->HasMovesLeft(), "Colony real forest event did not spend two movement points" );
			record = Unit::Serialize( pod ).ToString(); saved = source.Snapshot(); GT_OK();
		} ); GT_ASSERT( error.empty(), "Colony movement: " + error );
		ConventionalUnitFixture destination; destination.ConfigureProduction( UpkeepSource, "GAIANS" );
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			destination.Restore( GSE_CALL, saved, true ); auto* game = destination.Game(); auto* pod = game->GetUM()->GetUnit( id ); auto* base = game->GetBM()->GetBase( 701 );
			GT_ASSERT( Unit::Serialize( pod ).ToString() == record && static_cast< StaticDef* >( pod->m_def )->m_render->m_type == Render::RT_PORT_COLONY_MARKER, "Colony restore changed stored package/home/render/spent" );
			game->GetUM()->ResetConventionalSpent( GSE_CALL, pod ); destination.MoveEvent( GSE_CALL, pod, game->GetMap()->GetTile( 6, 2 ) );
			GT_ASSERT( pod->GetTile() == game->GetMap()->GetTile( 6, 2 ) && pod->Conventional().spent == 3, "restored Colony did not take ordinary flat step" );
			const auto next = base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t ) { return game->GetUM()->GetUnitDef( "live-colony" ); }, SmacMoraleMode::Mode0 );
			GT_ASSERT( !next.completed_unit && base->GetProductionState()->minerals == 3 && base->GetAccumulatedNutrients() == 1
				&& base->m_pops.size() == 1 && pod->Conventional().home_base_id == base->m_id, "restored following upkeep replayed Colony completion or lost home" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC Colony easy difficulty bypasses size-one governor deferral without population debit", []( GSE*, context::Context* ) -> std::string {
		for ( const auto difficulty : { "Citizen", "Specialist" } ) {
			for ( const size_t population : { 1u, 2u } ) {
				ConventionalUnitFixture fixture; fixture.ConfigureProduction( ColonyChoiceSource, "GAIANS" );
				const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
					using namespace game::backend::unit;
					auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false, false, 0, 1, population ); auto* um = game->GetUM();
					base->m_owner->GetPlayer()->SetDifficultyLevel( difficulty ); // global remains Librarian
					game->GetMap()->GetTile( 3, 1 )->terraforming = TERRAFORMING_FOREST;
					auto allocation = *base->GetSmacAllocation(); allocation.selected = population == 1 ? 5 : 7;
					base->PublishSmacAllocation( GSE_CALL, allocation, Roles() );
					auto context = base->GetSmacResourceContext(); context.governor_options = 0x80000001u;
					base->SetSmacResourceContext( GSE_CALL, context );
					um->DefineUnit( new StaticDef( "live-colony", um->GetMoraleSet( "live-morale" ), "Colony port marker", MT_LAND, 0, new ColonyMarkerRender() ) );
					game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
					const auto target = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 0 );
					GT_ASSERT( game->GetSmacProductionCost( base->m_owner, target ) == 30, "Colony literal fixture cost changed" );
					base->SetProductionState( GSE_CALL, ProductionState{ target, population == 1 ? 27 : 25, target, 77, std::vector< ProductionItem >{} } );
					base->SetAccumulatedNutrients( 0 );
					const auto result = base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t ) { return um->GetUnitDef( "live-colony" ); }, SmacMoraleMode::Mode0 );
					GT_ASSERT( result.completed_unit && um->GetUnits().size() == 2 && base->m_pops.size() == 1
						&& base->GetAccumulatedNutrients() == ( population == 1 ? 1 : 0 )
						&& base->GetProductionState()->minerals == 0 && base->GetProductionState()->baseline_minerals == 0,
						"stored easy difficulty did not precede governor deferral, or protected a larger population" );
					const auto* pod = um->GetUnit( *result.completed_unit );
					GT_ASSERT( pod->Conventional().home_base_id == base->m_id && pod->Conventional().spent == 0
						&& base->GetProductionState()->current_item == target && base->GetProductionState()->queue->empty()
						&& base->GetSmacResourceContext() == context, "easy completion changed home, target or governor policy" ); GT_OK();
				} ); GT_ASSERT( error.empty(), "difficulty " + std::string( difficulty ) + ": " + error );
			}
		} GT_OK();
	} );
	task->AddTest( "SMAC Colony governor deferral posts before growth and completes only on a later upkeep", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( ColonyChoiceSource, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false ); auto* um = game->GetUM();
			base->m_owner->GetPlayer()->SetDifficultyLevel( "Librarian" );
			game->GetMap()->GetTile( 3, 1 )->terraforming = TERRAFORMING_FOREST;
			auto allocation = *base->GetSmacAllocation(); allocation.selected = 5; base->PublishSmacAllocation( GSE_CALL, allocation, Roles() );
			auto context = base->GetSmacResourceContext(); context.governor_options = 0x80000001u; base->SetSmacResourceContext( GSE_CALL, context );
			um->DefineUnit( new StaticDef( "live-colony", um->GetMoraleSet( "live-morale" ), "Colony port marker", MT_LAND, 0, new ColonyMarkerRender() ) );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
			const auto target = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 0 );
			GT_ASSERT( game->GetSmacProductionCost( base->m_owner, target ) == 30, "Colony literal fixture cost changed" );
			base->SetProductionState( GSE_CALL, ProductionState{ target, 27, target, 27, std::vector< ProductionItem >{} } ); base->SetAccumulatedNutrients( 19 );
			const auto next_id = Unit::GetNextId(); size_t factory_calls = 0;
			const auto resolver = [ & ]( int32_t ) { ++factory_calls; return um->GetUnitDef( "live-colony" ); };
			const auto first = base->ProcessSmacUpkeep( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( !first.completed_unit && factory_calls == 0 && Unit::GetNextId() == next_id && base->m_pops.size() == 1
				&& base->GetAccumulatedNutrients() == 20 && base->GetProductionState()->minerals == 30
				&& base->GetProductionState()->baseline_minerals == 30, "governor deferral skipped posting or created a premature Pod" );
			const auto second = base->ProcessSmacUpkeep( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( !second.completed_unit && factory_calls == 0 && Unit::GetNextId() == next_id && base->m_pops.size() == 2
				&& base->GetAccumulatedNutrients() == 0 && base->GetProductionState()->minerals == 33
				&& base->GetProductionState()->baseline_minerals == 33, "same-upkeep growth retroactively completed the deferred Pod" );
			const auto third = base->ProcessSmacUpkeep( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( third.completed_unit && factory_calls == 1 && base->m_pops.size() == 1 && base->GetAccumulatedNutrients() == 0
				&& base->GetProductionState()->minerals == 5 && base->GetProductionState()->baseline_minerals == 5
				&& base->GetProductionState()->current_item == target && base->GetProductionState()->baseline_item == target
				&& base->GetSmacResourceContext() == context, "later Colony completion lost carry, population debit or target authority" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC Colony automatic deferral requires both governor bits and preserves the pending queue", []( GSE*, context::Context* ) -> std::string {
		for ( const uint32_t options : { 0u, 0x80000000u, 1u, 0x80000001u } ) {
			ConventionalUnitFixture fixture; fixture.ConfigureProduction( ColonyChoiceSource, "GAIANS" );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				using namespace game::backend::unit;
				auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false ); auto* um = game->GetUM();
				base->m_owner->GetPlayer()->SetDifficultyLevel( "Talent" ); // exact first non-easy boundary
				auto allocation = *base->GetSmacAllocation(); allocation.selected = 5; base->PublishSmacAllocation( GSE_CALL, allocation, Roles() );
				auto context = base->GetSmacResourceContext(); context.governor_options = options; base->SetSmacResourceContext( GSE_CALL, context );
				game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( false, true, 128, 64 ) );
				const auto target = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 0 );
				GT_ASSERT( game->GetSmacProductionCost( base->m_owner, target ) == 30, "Colony literal fixture cost changed" );
				const auto next = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
				const ProductionState production{ target, 27, target, 27, std::vector< ProductionItem >{ next } };
				base->SetProductionState( GSE_CALL, production ); base->SetAccumulatedNutrients( 0 );
				const auto before = fixture.Snapshot(); const auto id = Unit::GetNextId(); const auto requests = fixture.QueuedFrontendRequests().size();
				if ( options != 0x80000001u ) {
					GT_ASSERT( Rejects( [ & ] { base->ProcessSmacUpkeep( GSE_CALL, Roles(), {}, SmacMoraleMode::Mode0 ); } )
						&& fixture.Snapshot() == before && Unit::GetNextId() == id && fixture.QueuedFrontendRequests().size() == requests,
						"missing governor bit bypassed explicit choice or partially posted" );
				} else {
					const auto result = base->ProcessSmacUpkeep( GSE_CALL, Roles(), {}, SmacMoraleMode::Mode0 ); // no factory/presentation needed
					GT_ASSERT( !result.completed_unit && !result.queue_advanced && Unit::GetNextId() == id && um->GetUnits().size() == 1
						&& base->m_pops.size() == 1 && base->GetAccumulatedNutrients() == 1
						&& base->GetProductionState()->minerals == 30 && base->GetProductionState()->baseline_minerals == 30
						&& base->GetProductionState()->queue == production.queue && base->GetProductionState()->current_item == target,
						"automatic deferral advanced its queue, lost posting or attempted a factory" );
				} GT_OK();
			} ); GT_ASSERT( error.empty(), "governor options " + std::to_string( options ) + ": " + error );
		} GT_OK();
	} );
	task->AddTest( "SMAC Colony question posts minerals once and Delay resumes the actual event tail", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( ColonyChoiceSource, "GAIANS" );
		fixture.ResourceState([](auto& state){state.economy.at(1).rates={0,0};});
		int64_t request_id=0; size_t next_unit=0; bool obsolete_command=false;
		const auto prefix=fixture.Run([&](GSE_CALLABLE) -> std::string {
			auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);
			base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
			std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i){roles[i+1]=Roles().specialists[i];}
			fixture.BasePresentationRoles(roles);SelectionPresentations(fixture);
			auto allocation=*base->GetSmacAllocation();allocation.selected=5;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
			fixture.QueueProbeEvent(GSE_CALL,[&]{obsolete_command=true;});
			game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,true,128,64));
			const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
			base->SetProductionState(GSE_CALL,ProductionState{target,27,target,77,std::vector<ProductionItem>{}});base->SetAccumulatedNutrients(0);
			next_unit=game::backend::unit::Unit::GetNextId();
			fixture.BeginColonyChoice(GSE_CALL,base->m_id);
			GT_ASSERT(base->GetProductionState()->minerals==30 && base->GetProductionState()->baseline_minerals==77
				&& base->GetAccumulatedNutrients()==0 && base->m_pops.size()==1 && game::backend::unit::Unit::GetNextId()==next_unit,
				"question crossed the old baseline/food or manufactured before an answer");
			const auto& cache=base->GetSmacUpkeepCache();
			GT_ASSERT(cache && cache->calculation.nutrients.surplus==1 && cache->calculation.minerals.surplus==3,"question did not retain its actual first pass");
			auto* pending=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony"));
			request_id=static_cast<value::Int*>(pending->Get("request_id"))->value;
			GT_ASSERT(static_cast<value::Int*>(pending->Get("base_id"))->value==701
				&& static_cast<value::Int*>(pending->Get("owner_slot"))->value==0,"question did not expose authenticated Base/owner");
			GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"complete_turn",{});})
				&& Rejects([&]{game->Event(GSE_CALL,"choose_colony",{{"request_id",VALUE(value::Int,,request_id)},{"choice",VALUE(value::String,,"cancel")}});}),
				"pending question admitted unrelated mutation or implicit cancellation");
			for(int i=0;i<2;++i){game->Event(GSE_CALL,"choose_colony",{{"request_id",VALUE(value::Int,,request_id)},{"choice",VALUE(value::String,,"delay")}});}
			GT_OK();
		});GT_ASSERT(prefix.empty(),"Colony prefix: "+prefix);
		fixture.DrainEvents(); // outside the source accumulation, through actual event dispatch
		return fixture.Run([&](GSE_CALLABLE) -> std::string {
			auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);
			GT_ASSERT(!obsolete_command && base->GetProductionState()->minerals==30 && base->GetProductionState()->baseline_minerals==30
				&& base->GetAccumulatedNutrients()==1 && base->m_pops.size()==1 && game::backend::unit::Unit::GetNextId()==next_unit,
				"Delay or duplicate answer reposted minerals/food or executed an obsolete command");
			GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony")->type==VT_NULL,"Delay left its request live");
			GT_ASSERT(base->GetSmacUpkeepCache()->calculation.nutrients.stored_before==1,"final resource pass did not replace prefix cache");GT_OK();
		});
	} );
	task->AddTest( "SMAC Colony saved question restores without replay and Delay consumes the saved tail once", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source;source.ConfigureProduction(ColonyChoiceSource,"GAIANS");
		source.ResourceState([](auto& state){state.economy.at(1).rates={0,0};});
		std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i){roles[i+1]=Roles().specialists[i];}
		source.BasePresentationRoles(roles);
		std::string snapshot,base_record,random;
		const auto setup=source.Run([&](GSE_CALLABLE) -> std::string {
			auto* game=source.Game();auto* base=Populate(GSE_CALL,source,false);
			base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");SelectionPresentations(source);
			auto allocation=*base->GetSmacAllocation();allocation.selected=5;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
			game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,true,128,64));
			const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
			base->SetProductionState(GSE_CALL,ProductionState{target,27,target,77,std::vector<ProductionItem>{}});base->SetAccumulatedNutrients(0);
			source.BeginColonyChoice(GSE_CALL,base->m_id);
			snapshot=source.Snapshot();base_record=Base::Serialize(base).ToString();random=game->GetRandom()->GetStateString();GT_OK();
		});GT_ASSERT(setup.empty(),"save Colony question: "+setup);
		ConventionalUnitFixture restored;restored.ConfigureProduction(ColonyChoiceSource,"GAIANS");restored.BasePresentationRoles(roles);
		const auto load=restored.Run([&](GSE_CALLABLE) -> std::string {
			auto* game=restored.Game();game->GetState()->m_slots->GetSlot(0).GetPlayer()->SetDifficultyLevel("Librarian");
			restored.RestoreThroughStart(GSE_CALL,snapshot,[&](GSE_CALLABLE) {
				if(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony")->type!=VT_NULL) {
					throw std::runtime_error("restored question published before manager reconstruction");
				}
			});
			auto* base=game->GetBM()->GetBase(701);
			GT_ASSERT(Base::Serialize(base).ToString()==base_record && game->GetRandom()->GetStateString()==random,
				"restoration replayed the first pass or replaced saved cache/RNG");
			auto* pending=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony"));
			const auto request=static_cast<value::Int*>(pending->Get("request_id"))->value;
			GT_ASSERT(request>0 && base->GetProductionState()->minerals==30 && base->GetProductionState()->baseline_minerals==77
				&& base->GetAccumulatedNutrients()==0,"loaded question advanced its population or baseline");
			for(int i=0;i<2;++i) {game->Event(GSE_CALL,"choose_colony",{{"request_id",VALUE(value::Int,,request)},{"choice",VALUE(value::String,,"delay")}});}
			GT_OK();
		});GT_ASSERT(load.empty(),"restore Colony question: "+load);
		restored.DrainEvents();
		return restored.Run([&](GSE_CALLABLE) -> std::string {
			auto* game=restored.Game();const auto* base=game->GetBM()->GetBase(701);
			GT_ASSERT(base->GetProductionState()->minerals==30 && base->GetProductionState()->baseline_minerals==30
				&& base->GetAccumulatedNutrients()==1,"loaded Delay replayed mineral posting or population collection");
			GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony")->type==VT_NULL,"loaded answer did not retire");GT_OK();
		});
	} );
	task->AddTest("SMAC Colony deferred Screen survives research save and real Start without reposting",[](GSE*,context::Context*) -> std::string {
		ConventionalUnitFixture source;source.ConfigureProduction(ColonyChoiceSource,"GAIANS");
		std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i){roles[i+1]=Roles().specialists[i];}
		source.BasePresentationRoles(roles);
		source.ResourceState([](auto& state){
			state.economy.at(1).rates={0,5};state.social.factions[0].current[1]=0;
			state.technology.factions[0].target.reset();state.technology.factions[0].labs=0;
		});
		const auto setup=source.Run([&](GSE_CALLABLE)->std::string {
			auto* game=source.Game();auto* base=Populate(GSE_CALL,source,false);SelectionPresentations(source);
			base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
			auto* bm=game->GetBM();bm->DefineFacility(new FacilityDef("research-hq","HQ","",0,{},1));
			base->m_facilities.emplace("research-hq",bm->GetFacilityDef("research-hq"));
			auto allocation=*base->GetSmacAllocation();allocation.selected=5;base->PublishSmacAllocation(GSE_CALL,allocation,Roles());
			game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,true,128,64));
			const auto colony=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
			base->SetProductionState(GSE_CALL,ProductionState{colony,27,colony,77,std::vector<ProductionItem>{}});base->SetAccumulatedNutrients(0);
			source.BeginColonyChoice(GSE_CALL,base->m_id);
			auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));
			auto* question=static_cast<value::Object*>(Call(GSE_CALL,wrapper,"get_pending_smac_colony"));
			game->Event(GSE_CALL,"choose_colony",{{"request_id",question->Get("request_id")},{"choice",VALUE(value::String,,"screen")}});GT_OK();
		});GT_ASSERT(setup.empty(),"deferred research setup: "+setup);source.DrainEvents();
		std::string saved,base_record,random;int64_t credits=0;
		const auto paused=source.Run([&](GSE_CALLABLE)->std::string {
			auto* game=source.Game();auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));
			GT_ASSERT(Call(GSE_CALL,wrapper,"get_pending_smac_research")->type==VT_OBJECT &&
				Call(GSE_CALL,wrapper,"get_pending_smac_colony_screen")->type==VT_NULL,"Screen opened before the research answer");
			auto* base=game->GetBM()->GetBase(701);saved=source.Snapshot();base_record=Base::Serialize(base).ToString();
			random=game->GetRandom()->GetStateString();credits=game->GetCanonicalEconomy(base->m_owner).credits;GT_OK();
		});GT_ASSERT(paused.empty(),"save deferred Screen: "+paused);
		ConventionalUnitFixture restored;restored.ConfigureProduction(ColonyChoiceSource,"GAIANS");restored.BasePresentationRoles(roles);
		const auto loaded=restored.Run([&](GSE_CALLABLE)->std::string {
			auto* game=restored.Game();game->GetState()->m_slots->GetSlot(0).GetPlayer()->SetDifficultyLevel("Librarian");
			restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
			auto* base=game->GetBM()->GetBase(701);auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));
			GT_ASSERT(Base::Serialize(base).ToString()==base_record && game->GetRandom()->GetStateString()==random &&
				game->GetCanonicalEconomy(base->m_owner).credits==credits,"restoring research advanced the deferred Base");
			auto* pending=static_cast<value::Object*>(Call(GSE_CALL,wrapper,"get_pending_smac_research"));
			auto* choice=static_cast<value::Object*>(static_cast<value::Array*>(pending->Get("choices"))->value.front());
			game->Event(GSE_CALL,"choose_research",{{"request_id",pending->Get("request_id")},{"technology_id",choice->Get("id")}});GT_OK();
		});GT_ASSERT(loaded.empty(),"load deferred Screen: "+loaded);restored.DrainEvents();
		const auto screen=restored.Run([&](GSE_CALLABLE)->std::string {
			auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);auto* wrapper=static_cast<value::Object*>(game->Wrap(GSE_CALL));
			GT_ASSERT(Call(GSE_CALL,wrapper,"get_pending_smac_research")->type==VT_NULL &&
				base->GetProductionState()->minerals==30 && base->GetAccumulatedNutrients()==1 &&
				game->GetCanonicalEconomy(base->m_owner).credits==credits+game->GetCanonicalBaseEnergy(*base).tail.final.economy,
				"research answer reposted minerals, food or credits");
			auto* pending=static_cast<value::Object*>(Call(GSE_CALL,wrapper,"get_pending_smac_colony_screen"));
			for(int i=0;i<2;++i) {game->Event(GSE_CALL,"close_colony_screen",{{"request_id",pending->Get("request_id")}});}GT_OK();
		});GT_ASSERT(screen.empty(),"open deferred Screen: "+screen);restored.DrainEvents();
		return restored.Run([&](GSE_CALLABLE)->std::string {
			auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
			GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony_screen")->type==VT_NULL &&
				base->GetProductionState()->minerals==30 && base->GetAccumulatedNutrients()==1,"closing restored Screen replayed the Base");GT_OK();
		});
	});
	task->AddTest( "SMAC Colony retained calculation changes only at actual calculation publication", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture;fixture.ConfigureProduction(ColonyChoiceSource,"GAIANS");
		return fixture.Run([&](GSE_CALLABLE) -> std::string {
			auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);
			game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,true,128,64));
			GT_ASSERT(!base->GetSmacUpkeepCache(),"supplied allocation fabricated calculation history");
			(void)base->PrepareSmacResourceCalculation();
			GT_ASSERT(!base->GetSmacUpkeepCache(),"read-only resource query fabricated calculation history");
			base->RecalculateSmacWorkers(GSE_CALL,Roles(),SmacBaseResourcePhase::Upkeep);
			const auto before=*base->GetSmacUpkeepCache();
			GT_ASSERT(before.calculation.nutrients.surplus==1 && before.calculation.minerals.surplus==3,"actual allocation did not retain forest result");
			game->GetMap()->GetTile(4,2)->terraforming=0;
			const auto fresh=base->PrepareSmacResourceCalculation();
			GT_ASSERT(fresh.nutrients.surplus==0 && base->GetSmacUpkeepCache()->calculation.nutrients.surplus==1,
				"query or terrain edit replaced the retained successor surplus");
			const auto restored_cache=DeserializeSmacBaseUpkeepCache(SerializeSmacBaseUpkeepCache(base->GetSmacUpkeepCache()));
			GT_ASSERT(restored_cache && restored_cache->calculation.nutrients.surplus==1
				&& restored_cache->calculation.raw_intake==before.calculation.raw_intake
				&& restored_cache->support.mineral_support==before.support.mineral_support,
				"cache codec replaced stale history with fresh changed-terrain values");
			base->RecalculateSmacWorkers(GSE_CALL,Roles(),SmacBaseResourcePhase::Upkeep);
			GT_ASSERT(base->GetSmacUpkeepCache()->calculation.nutrients.surplus==0,"actual later calculation did not replace retained surplus");GT_OK();
		});
	} );

	task->AddTest("SMAC Colony Screen posts current income before owned edits and closes before next Base",[](GSE*,context::Context*) -> std::string {
		const auto source=[](const std::string& filename) {
			auto value=ColonyChoiceSource(filename);if(filename!="alpha.txt") {return value;}
			std::istringstream input(value);std::string line,section,result;size_t row=0;
			while(std::getline(input,line)) {
				if(!line.empty() && line[0]=='#') {section=line;row=0;}
				else {++row;
					if(section=="#RULES" && (row==43 || row==44)) {line.replace(0,line.find(','),row==43?"50":"2");}
					if(section=="#FACILITIES" && row==1) {const auto a=line.find(','),b=line.find(',',a+1),c=line.find(',',b+1);line.replace(b+1,c-b-1,"0");}
                    if(section=="#FACILITIES" && row==3) {line="Tanks,4,0,Tech6,Disable,Resources";}
				}result+=line+'\n';
			}return result;
		};
		ConventionalUnitFixture fixture;fixture.ConfigureProduction(source,"GAIANS");
		fixture.ResourceState([](auto& state){
			auto& saved=state.economy.at(1);saved.rates={0,5};saved.credits=100;saved.pending_debit=0;saved.research_fraction=0;
			state.social.factions[0].current[1]=0;state.social.factions[0].current[10]=0;
			auto& tech=state.technology.factions[0];tech.target=5;tech.cached_threshold=1000;tech.labs=0;
		});
		int64_t request=0;bool obsolete=false;
		const auto setup=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* first=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();
			first->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");SelectionPresentations(fixture);
			std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i){roles[i+1]=Roles().specialists[i];}fixture.BasePresentationRoles(roles);
			bm->DefineFacility(new FacilityDef("screen-tanks","Tanks","",40,{},3));bm->DefineFacility(new FacilityDef("screen-hq","HQ","",0,{},1));first->m_facilities.emplace("screen-hq",bm->GetFacilityDef("screen-hq"));
			auto allocation=*first->GetSmacAllocation();allocation.selected=5;first->PublishSmacAllocation(GSE_CALL,allocation,Roles());
			const auto former=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,1);
			game->GetMap()->GetTile(8,2)->terraforming=TERRAFORMING_FOREST;
			Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().worker),0,game->GetMap()->GetTile(8,2)));
			auto* second=new Base(game,702,first->m_owner,first->m_faction,game->GetMap()->GetTile(6,2),"Next Base",{},pops,20,0,
				ProductionState{former,0,former,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{1,false,0,0,0,false});
			bm->SpawnBase(GSE_CALL,second);second->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{5,{{},0}},Roles());
			auto knowledge=KnowledgeValue(true,true,128,64).CopyData();for(auto& tile:knowledge.tiles){tile.explored_by=2;tile.claimant=1;}
			fixture.QueueProbeEvent(GSE_CALL,[&]{obsolete=true;});
			game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
			const auto colony=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,0);
			first->SetProductionState(GSE_CALL,ProductionState{colony,27,colony,77,std::vector<ProductionItem>{}});first->SetAccumulatedNutrients(0);
			fixture.BeginColonyChoice(GSE_CALL,first->m_id);
			auto* pending=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony"));
			request=static_cast<value::Int*>(pending->Get("request_id"))->value;
			game->Event(GSE_CALL,"choose_colony",{{"request_id",VALUE(value::Int,,request)},{"choice",VALUE(value::String,,"screen")}});GT_OK();
		});GT_ASSERT(setup.empty(),"Screen setup: "+setup);fixture.DrainEvents();
        const auto queue_edit=fixture.Run([&](GSE_CALLABLE)->std::string {auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);auto* second=game->GetBM()->GetBase(702);
            const value::array_elements_t entries{VALUEEXT(value::Object,GSE_CALL,(value::object_properties_t{{"source_id",VALUE(value::Int,,2)}}))};
            GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"edit_smac_queue",{{"base",second->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,second->SmacSpecialistSelectionState())},{"entries",VALUE(value::Array,,entries)}});}),"Screen queue admitted another Base");
            game->Event(GSE_CALL,"edit_smac_queue",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())},{"entries",VALUE(value::Array,,entries)}});GT_OK();
        });GT_ASSERT(queue_edit.empty(),"Screen queue edit: "+queue_edit);fixture.DrainEvents();
        const auto queue_check=fixture.Run([&](GSE_CALLABLE)->std::string {auto* base=fixture.Game()->GetBM()->GetBase(701);
            GT_ASSERT(base->GetProductionState()->queue->size()==1 && base->GetProductionState()->queue->front().smac_source_id==2
                && base->GetProductionState()->minerals==30 && base->GetAccumulatedNutrients()==1,"Screen queue edit failed or reposted upkeep");GT_OK();
        });GT_ASSERT(queue_check.empty(),"Screen queue check: "+queue_check);
        const auto rename_open=fixture.Run([&](GSE_CALLABLE)->std::string {auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);
            game->Event(GSE_CALL,"open_smac_rename",{{"base",base->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())}});GT_OK();
        });GT_ASSERT(rename_open.empty(),"Screen Rename open: "+rename_open);fixture.DrainEvents();
        const auto rename_commit=fixture.Run([&](GSE_CALLABLE)->std::string {auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);auto* menu=static_cast<value::Object*>(base->CopySmacRenameChoices(GSE_CALL));
            game->Event(GSE_CALL,"commit_smac_rename",{{"base",base->Wrap(GSE_CALL)},{"request_id",menu->Get("request_id")},{"expected_state",menu->Get("expected_state")},{"name",VALUE(value::String,,"Screen renamed")}});GT_OK();
        });GT_ASSERT(rename_commit.empty(),"Screen Rename commit: "+rename_commit);fixture.DrainEvents();
        const auto renamed=fixture.Run([&](GSE_CALLABLE)->std::string {auto* base=fixture.Game()->GetBM()->GetBase(701);
            GT_ASSERT(base->m_name=="Screen renamed" && base->GetAccumulatedNutrients()==1 && base->GetProductionState()->minerals==30,"Screen Rename failed or reposted upkeep");GT_OK();
        });GT_ASSERT(renamed.empty(),"Screen Rename result: "+renamed);
        // Optional specialist menus must use the real suspended Screen command
        // admission, rather than bypassing the stamped continuation dispatcher.
        for(const int32_t type:{1,-1}) {
            const auto open_specialist=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);
                game->Event(GSE_CALL,"open_smac_specialist",{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,19)},{"expected_state",VALUE(value::String,,base->SmacSpecialistSelectionState())}});GT_OK();
            });GT_ASSERT(open_specialist.empty(),"Screen specialist open: "+open_specialist);fixture.DrainEvents();
            const auto choose_specialist=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=game->GetBM()->GetBase(701);auto* menu=static_cast<value::Object*>(base->CopySmacSpecialistChoices(GSE_CALL,19));
                game->Event(GSE_CALL,"select_smac_specialist",{{"base",base->Wrap(GSE_CALL)},{"pop_id",VALUE(value::Int,,19)},{"request_id",menu->Get("request_id")},{"expected_state",menu->Get("expected_state")},{"type",VALUE(value::Int,,type)}});GT_OK();
            });GT_ASSERT(choose_specialist.empty(),"Screen specialist choice: "+choose_specialist);fixture.DrainEvents();
            const auto selected=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* base=fixture.Game()->GetBM()->GetBase(701);
                GT_ASSERT(base->GetSmacAllocation()->specialists.total_count==(type==1 ? 1 : 0) && base->GetAccumulatedNutrients()==1,"Screen specialist edit failed or recollected food");GT_OK();
            });GT_ASSERT(selected.empty(),"Screen specialist result: "+selected);
        }
		const auto opened=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* first=game->GetBM()->GetBase(701);auto* second=game->GetBM()->GetBase(702);
			auto* pending=Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony_screen");
			GT_ASSERT(pending->type==VT_OBJECT && static_cast<value::Int*>(static_cast<value::Object*>(pending)->Get("request_id"))->value==request,"Screen did not retain its question identity");
			bool research=false;fixture.ResourceState([&](auto& state){research=state.technology.factions[0].labs==2;});
			GT_ASSERT(!obsolete && first->GetProductionState()->minerals==30 && first->GetProductionState()->baseline_minerals==30
				&& first->GetAccumulatedNutrients()==1 && game->GetCanonicalEconomy(first->m_owner).credits==101 && research
				&& second->GetProductionState()->minerals==0 && second->GetAccumulatedNutrients()==0,"Screen boundary ran before current credit or after next Base");
			GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"complete_turn",{});})
				&& Rejects([&]{game->Event(GSE_CALL,"unwork_base_tile",{{"base",second->Wrap(GSE_CALL)},{"tile",game->GetMap()->GetTile(8,2)->Wrap(GSE_CALL)}});}),"Screen admitted another Base or unrelated turn mutation");
			game->Event(GSE_CALL,"unwork_base_tile",{{"base",first->Wrap(GSE_CALL)},{"tile",game->GetMap()->GetTile(4,2)->Wrap(GSE_CALL)}});
			std::optional<int64_t> shown;bool confirmation=false;
			auto* choices=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(first->Wrap(GSE_CALL)),"get_smac_production_choices"));
			for(auto* entry:static_cast<value::Array*>(choices->Get("choices"))->value) {
				auto* choice=static_cast<value::Object*>(entry);
				if(static_cast<value::Int*>(choice->Get("source_id"))->value==1) {
					shown=static_cast<value::Int*>(choice->Get("minerals"))->value;
					confirmation=static_cast<value::Bool*>(choice->Get("confirmation"))->value;
				}
			}
			// Gaia owns Tech6, admitting the Former package and row. This fixture
			// deliberately uses strictness2: distinct Unit identities incur the
			// accepted loss, 10 + (baseline30 - exemption10) * 50 / 100 =20.
			// The Screen receipt supplies selection after its separate loss decision.
			GT_ASSERT(shown==20 && confirmation,"Screen Former quote must retain20 with the measured manual-loss confirmation");
			game->Event(GSE_CALL,"select_smac_production",{{"base",first->Wrap(GSE_CALL)},{"source_id",VALUE(value::Int,,1)},
				{"expected_state",VALUE(value::String,,SerializeProductionState(first->GetProductionState()).ToString())},
				{"expected_minerals",VALUE(value::Int,,*shown)},{"confirm",VALUE(value::Bool,,true)}});GT_OK();
		});GT_ASSERT(opened.empty(),"Screen entry: "+opened);fixture.DrainEvents();fixture.DrainEvents();
		const auto edited=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* first=game->GetBM()->GetBase(701);auto* second=game->GetBM()->GetBase(702);
			bool research=false;fixture.ResourceState([&](auto& state){research=state.technology.factions[0].labs==2;});
			GT_ASSERT(first->GetSmacAllocation()->specialists.total_count==1 && first->GetProductionState()->current_item.smac_source_id==1
				&& first->GetAccumulatedNutrients()==1 && first->GetProductionState()->minerals==20 && first->GetProductionState()->baseline_minerals==30
				&& game->GetCanonicalEconomy(first->m_owner).credits==101 && research && second->GetProductionState()->minerals==0,
				"Screen edits failed or retroactively reposted the current Base");
			GT_ASSERT((game->GetCanonicalBaseEnergy(*first).tail.final==SmacEnergyCategories{1,2,1}),"Screen worker click did not refresh actual E/P/L outputs");
            // The Former is fully funded. Use the actual stamped production
            // selector to choose the more expensive Tanks before zero Hurry.
            game->Event(GSE_CALL,"select_smac_production",{{"base",first->Wrap(GSE_CALL)},{"source_id",VALUE(value::Int,,-3)},
                {"expected_state",VALUE(value::String,,SerializeProductionState(first->GetProductionState()).ToString())},
                {"expected_minerals",VALUE(value::Int,,20)},{"confirm",VALUE(value::Bool,,true)}});GT_OK();
        });GT_ASSERT(edited.empty(),"Screen edits: "+edited);fixture.DrainEvents();
        const auto buy=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* first=game->GetBM()->GetBase(701);auto* second=game->GetBM()->GetBase(702);
            const auto offer=first->GetSmacHurryOffer();
            GT_ASSERT(offer.reason.empty(),"Screen Hurry lost actual local authority: "+offer.reason);
            GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"hurry_smac_production",{{"base",second->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,second->GetSmacHurryOffer().expected)},{"full",VALUE(value::Bool,,false)},{"amount",VALUE(value::Int,,0)}});}),"Screen Hurry admitted another Base");
            game->Event(GSE_CALL,"hurry_smac_production",{{"base",first->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,offer.expected)},{"full",VALUE(value::Bool,,false)},{"amount",VALUE(value::Int,,0)}});
            GT_OK();
        });GT_ASSERT(buy.empty(),"Screen Hurry dispatch: "+buy);fixture.DrainEvents();
        const auto hurry=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* first=game->GetBM()->GetBase(701);
            GT_ASSERT(first->GetProductionState()->hurry_partial_paid && first->GetProductionState()->minerals==20
                && first->GetProductionState()->baseline_minerals==20 && game->GetCanonicalEconomy(first->m_owner).credits==101,
                "Screen zero Hurry lost its marker/baseline or reposted current income");
			for(int i=0;i<2;++i) {game->Event(GSE_CALL,"close_colony_screen",{{"request_id",VALUE(value::Int,,request)}});}
			// Accepted while open, but ordered after close: its retired stamp must
			// prevent it from changing the Base after the suspended turn finishes.
			game->Event(GSE_CALL,"work_base_tile",{{"base",first->Wrap(GSE_CALL)},{"tile",game->GetMap()->GetTile(4,2)->Wrap(GSE_CALL)}});GT_OK();
		});GT_ASSERT(hurry.empty(),"Screen Hurry and close: "+hurry);fixture.DrainEvents();
		return fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* first=game->GetBM()->GetBase(701);auto* second=game->GetBM()->GetBase(702);
			bool research=false;fixture.ResourceState([&](auto& state){research=state.technology.factions[0].labs==3;});
			GT_ASSERT(first->GetAccumulatedNutrients()==1 && first->GetProductionState()->minerals==20 && first->GetProductionState()->baseline_minerals==20
                && !first->GetProductionState()->hurry_partial_paid
				&& first->GetSmacAllocation()->specialists.total_count==1
				&& second->GetAccumulatedNutrients()==1 && second->GetProductionState()->minerals==3
				&& game->GetCanonicalEconomy(first->m_owner).credits==102 && research,"Screen close replayed current posting, stale edit or next Base upkeep");
			GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony_screen")->type==VT_NULL,"closed Screen remained pending");GT_OK();
		});
	});
	task->AddTest( "SMAC Colony explicit choice and wrong presentation reject before publication", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( UpkeepSource, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false ); auto* um = game->GetUM();
			um->DefineUnit( new StaticDef( "live-colony", um->GetMoraleSet( "live-morale" ), "Colony port marker", MT_LAND, 0, new ColonyMarkerRender() ) );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( false, true, 128, 64 ) ); const auto target = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 0 );
			base->SetProductionState( GSE_CALL, ProductionState{ target, game->GetSmacProductionCost( base->m_owner, target ), target, 0, std::vector< ProductionItem >{} } );
			for ( const auto* presentation : { "live-scout", "live-colony" } ) {
				base->m_owner->GetPlayer()->SetDifficultyLevel( std::string( presentation ) == "live-colony" ? "Librarian" : "Specialist" );
				const auto before = fixture.Snapshot(); const auto id = Unit::GetNextId();
				GT_ASSERT( Rejects( [ & ] { base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t ) { return um->GetUnitDef( presentation ); }, SmacMoraleMode::Mode0 ); } )
					&& fixture.Snapshot() == before && Unit::GetNextId() == id, "unsupported Colony choice/presentation partially posted" );
			} GT_OK();
		} );
	} );
	task->AddTest("SMAC population aggregate waits for owner refresh and survives actual Start independently",[](GSE*,context::Context*)->std::string {
		using namespace game::backend::unit;
		const auto roles=Roles();const std::array<std::string,8> names{roles.worker,roles.specialists[0],roles.specialists[1],roles.specialists[2],roles.specialists[3],roles.specialists[4],roles.specialists[5],roles.specialists[6]};
		ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(UpkeepSource,true);fixture.BasePresentationRoles(names);std::string saved;
		const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,true);SelectionPresentations(fixture);
			game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
			const auto scout=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,2);
			base->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});base->SetAccumulatedNutrients(20);
			fixture.ResourceState([](auto& state){state.economy.at(1).population_aggregate=111;state.economy.at(2).population_aggregate=777;
				state.economy.at(1).regional_base_counts->fill(77);state.economy.at(2).regional_base_counts->fill(88);
				auto& tech=state.technology.factions[0];tech.target=5;tech.cached_threshold=10000;});
			const auto resolver=[&](int32_t){return game->GetUM()->GetUnitDef("live-scout");};
			base->ProcessSmacUpkeep(GSE_CALL,roles,resolver,SmacMoraleMode::Mode0);
			GT_ASSERT(base->m_pops.size()==2 && game->GetCanonicalEconomy(base->m_owner).population_aggregate==111,"Base growth prematurely refreshed owner population");
			fixture.RefreshPopulation(1);
			GT_ASSERT(game->GetCanonicalEconomy(base->m_owner).population_aggregate==2,"post-Base owner refresh omitted growth");
			const auto& counts=game->GetCanonicalEconomy(base->m_owner).regional_base_counts;
			GT_ASSERT(counts && counts->at(1)==1 && counts->at(63)==0 && counts->at(127)==0,"owner summary did not clear and rebuild regional counts");
			base->ProcessSmacUpkeep(GSE_CALL,roles,resolver,SmacMoraleMode::Mode0);
			GT_ASSERT(base->m_pops.size()==1 && game->GetCanonicalEconomy(base->m_owner).population_aggregate==2,"starvation changed the retained population aggregate");
			bool untouched=false;fixture.ResourceState([&](auto& state){untouched=state.economy.at(2).population_aggregate==777
				&& state.economy.at(2).regional_base_counts->at(1)==88 && state.economy.at(1).regional_base_counts->at(1)==1;});
			GT_ASSERT(untouched,"owner refresh touched the foreign aggregate");saved=fixture.Snapshot();GT_OK();
		});if(!first.empty()) {return first;}
		ConventionalUnitFixture restored;restored.ConfigureTwoResourceParticipants(UpkeepSource,true);restored.BasePresentationRoles(names);
		restored.ProductionPresentations({"choice-scout","choice-rover","choice-former","choice-colony"});
		return restored.Run([&](GSE_CALLABLE)->std::string {
			restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(701);
			GT_ASSERT(base && base->m_pops.size()==1 && game->GetCanonicalEconomy(base->m_owner).population_aggregate==2,"Start derived aggregate from current Base populations");
			GT_ASSERT(game->GetCanonicalEconomy(base->m_owner).regional_base_counts->at(1)==1,"Start lost the owned regional summary");
			bool untouched=false;restored.ResourceState([&](auto& state){untouched=state.economy.at(2).population_aggregate==777;});GT_ASSERT(untouched,"Start reset foreign population history");
			game->Event(GSE_CALL,"complete_turn",{});restored.DrainEvents();
			GT_ASSERT(game->GetCanonicalEconomy(base->m_owner).population_aggregate==static_cast<int32_t>(base->m_pops.size()),"actual turn controller omitted post-Base population refresh");GT_OK();
		});
	});
	task->AddTest( "SMAC live upkeep stages growth starvation and stable Pop lifetime", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( UpkeepSource, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, true );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
			base->SetProductionState( GSE_CALL, ProductionState{ scout, 0, scout, 0, std::vector< ProductionItem >{} } );
			base->SetAccumulatedNutrients( 20 ); auto* survivor = &base->m_pops.at( 19 );
			auto* wrapper = static_cast< value::Object* >( survivor->Wrap( GSE_CALL ) );
			const auto resolver = [ & ]( int32_t ) { return game->GetUM()->GetUnitDef( "live-scout" ); };
			const auto grown = base->ProcessSmacUpkeep( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( grown.final_calculation && base->m_pops.size() == 2 && &base->m_pops.at( 19 ) == survivor && Pop::Unwrap( wrapper ) == survivor,
				"growth replaced surviving Pop identity" );
			GT_ASSERT( base->GetAccumulatedNutrients() == -2 && grown.final_calculation->nutrients.surplus == -1,
				"growth did not normalize zero against actual new-population deficit before collection" );
			auto* retired = static_cast< value::Object* >( base->m_pops.at( 41 ).Wrap( GSE_CALL ) );
			const auto starved = base->ProcessSmacUpkeep( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( starved.final_calculation && base->m_pops.size() == 1 && &base->m_pops.at( 19 ) == survivor && !retired->wrapobj
				&& base->GetAccumulatedNutrients() == starved.final_calculation->nutrients.normalized_stored,
				"starvation failed to retire only the removed Pop or skipped forced resource pass" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live upkeep includes newborn support without reposting and restores continuation", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( UpkeepSource, "GAIANS" ); std::string saved;
		const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( false, true, 128, 64 ) );
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 ); const auto cost = game->GetSmacProductionCost( base->m_owner, scout );
			const auto resolver = [ & ]( int32_t ) { return game->GetUM()->GetUnitDef( "live-scout" ); };
			for ( int i = 0; i < 2; ++i ) {
				base->SetProductionState( GSE_CALL, ProductionState{ scout, cost - 3, scout, 0, std::vector< ProductionItem >{} } );
				GT_ASSERT( base->ProcessSmacProduction( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 ).completed_unit, "existing homed setup failed" );
			}
			base->SetProductionState( GSE_CALL, ProductionState{ scout, cost - 3, scout, 0, std::vector< ProductionItem >{} } ); base->SetAccumulatedNutrients( 0 );
			const auto result = base->ProcessSmacUpkeep( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( result.completed_unit && result.first_calculation.minerals.demand == 0 && result.final_calculation->minerals.demand == 1
				&& result.final_calculation->minerals.surplus == 2 && base->GetProductionState()->minerals == 0 && base->GetProductionState()->baseline_minerals == 0
				&& base->GetAccumulatedNutrients() == 1, "second pass missed newborn support or posted/collected twice" );
			saved = fixture.Snapshot(); GT_OK();
		} ); GT_ASSERT( error.empty(), "source upkeep: " + error );
		ConventionalUnitFixture destination; destination.ConfigureProduction( UpkeepSource, "GAIANS" );
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			destination.Restore( GSE_CALL, saved, true ); auto* game = destination.Game(); auto* base = game->GetBM()->GetBase( 701 );
			const auto pair = destination.GameplayRandom(); const auto jsf = game->GetRandom()->GetStateString();
			const auto next = base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t ) { return game->GetUM()->GetUnitDef( "live-scout" ); }, SmacMoraleMode::Mode0 );
			GT_ASSERT( !next.completed_unit && game->GetUM()->GetUnits().size() == 4 && base->GetProductionState()->minerals == 2
				&& base->GetProductionState()->baseline_minerals == 2 && base->GetAccumulatedNutrients() == 2
				&& destination.GameplayRandom() == pair && game->GetRandom()->GetStateString() == jsf, "restored next upkeep changed posting/home/food/RNG" ); GT_OK();
		} );
	} );
    task->AddTest("SMAC faction Unit replacement stages complete rosters before callbacks and late failure",[](GSE*,context::Context*) -> std::string {
        using namespace game::backend::unit;
        for(const bool transient_factory:{false,true}) {
            ConventionalUnitFixture fixture;fixture.ConfigureTwoResourceParticipants(ColonyChoiceSource,true);
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);auto* um=game->GetUM();
                const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
                base->SetProductionState(GSE_CALL,ProductionState{target,30,target,27,std::vector<ProductionItem>{}});
                um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("choice-former"),base->m_owner,game->GetMap()->GetTile(0,4),
                    {1,1,1,0,0,false,ConventionalUnitState::Home::Base,SmacMoraleMode::Mode0,base->m_id});
                auto* foreign_tile=game->GetMap()->GetTile(4,2);foreign_tile->terraforming=0;
                auto* foreign=um->SpawnConventionalUnit(GSE_CALL,93,um->GetUnitDef("live-scout"),&game->GetState()->m_slots->GetSlot(6),foreign_tile,
                    {2,2,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
                const auto foreign_bytes=Unit::Serialize(foreign).ToString();
                auto knowledge=KnowledgeValue(true,true,128,64).CopyData();knowledge.observers[2]=Knowledge::Observer{false,{},uint8_t(0)};
                knowledge.units.push_back({92,0,false});knowledge.units.push_back({93,0,false});
                for(auto& tile:knowledge.tiles){tile.explored_by=6;}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));fixture.Updates();
                const auto before=fixture.Snapshot();const auto pair=fixture.GameplayRandom();const auto random=game->GetRandom()->GetStateString();
                const auto next=Unit::GetNextId();const auto first=next+(transient_factory ? 1 : 0);
                std::vector<std::pair<Def*,ConventionalUnitState>> specs;
                for(const int32_t id:{0,1,0,2}) {
                    specs.push_back({um->GetUnitDef(id==0 ? "choice-colony" : id==1 ? "choice-former" : "choice-scout"),
                        {id,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0}});
                }
                std::optional<UnitManager::PreparedConventionalUnit> transient;
                if(transient_factory){transient.emplace(um->PrepareManufacturedConventionalUnit(GSE_CALL,*base,0,um->GetUnitDef("choice-colony"),SmacMoraleMode::Mode0));}
                auto* pending=transient ? &*transient : nullptr;auto* site=game->GetMap()->GetTile(4,4);
                const auto claims=*game->GetMap()->GetSmacWorkerKnowledge();
                {auto staged=um->PrepareFactionUnits(GSE_CALL,base->m_owner,site,specs,claims,pending);}
                GT_ASSERT(fixture.Snapshot()==before && Unit::GetNextId()==next && fixture.Updates().empty(),"discarded restart preparation published Units or IDs");
                auto invalid=specs;invalid.back().first=um->GetUnitDef("choice-former");
                GT_ASSERT(Rejects([&]{um->PrepareFactionUnits(GSE_CALL,base->m_owner,site,invalid,claims,pending);})
                    && fixture.Snapshot()==before && Unit::GetNextId()==next && fixture.Updates().empty(),"late replacement rejection partially removed or created Units");
                size_t removed=0,spawned=0;bool committed=false;
                const auto complete=[&] {
                    if(!committed || um->GetUnits().size()!=5 || um->GetUnit(91) || um->GetUnit(92) || um->GetUnit(93)!=foreign
                        || site->units.size()!=4 || Unit::GetNextId()!=first+4) {throw std::runtime_error("callback observed a partial replacement roster");}
                };
                um->On(GSE_CALL,"unit_despawn",NATIVE_CALL(&){complete();++removed;return VALUE(value::Undefined);}));
                um->On(GSE_CALL,"unit_spawn",NATIVE_CALL(&){complete();++spawned;return VALUE(value::Undefined);}));
                auto* old=static_cast<value::Object*>(um->GetUnit(91)->Wrap(GSE_CALL));
                auto staged=um->PrepareFactionUnits(GSE_CALL,base->m_owner,site,specs,claims,pending);
                staged.Commit();committed=true;GT_ASSERT(!removed && !spawned && old->wrapobj,"replacement notified or destroyed borrowed wrappers before publication");
                staged.Notify(GSE_CALL);GT_ASSERT(removed==(transient_factory ? 3u : 2u) && spawned==4 && !old->wrapobj,"replacement hooks or wrapper retirement differ");
                std::vector<size_t> order{93};
                for(size_t i=0;i<specs.size();++i) {
                    order.push_back(first+i);const auto* unit=um->GetUnit(first+i);
                    GT_ASSERT(unit && unit->Conventional()==specs[i].second && unit->GetTile()==site,"replacement lost factory order or fresh unhomed state");
                }
                GT_ASSERT(um->GetUnitRecordOrder()==order && Unit::Serialize(foreign).ToString()==foreign_bytes
                    && fixture.GameplayRandom()==pair && game->GetRandom()->GetStateString()==random,"replacement changed foreign Unit, order or random state");
                const auto frame=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                GT_ASSERT(frame.units.size()==5 && frame.remembered[1].size()==frame.tiles.size()
                    && frame.remembered[1][4*(frame.width/2)+2]->unit_present,"replacement failed to bind sparse remembered sight");GT_OK();
            });if(!error.empty()){return error;}
        }GT_OK();
    });
    task->AddTest("SMAC faction Unit refusal removes roster without reserving IDs",[](GSE*,context::Context*) -> std::string {
        using namespace game::backend::unit;
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(ColonyChoiceSource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* um=game->GetUM();
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));fixture.Updates();
            const auto before=fixture.Snapshot();const auto next=Unit::GetNextId();const auto pair=fixture.GameplayRandom();
            const auto claims=*game->GetMap()->GetSmacWorkerKnowledge();
            GT_ASSERT(Rejects([&]{um->PrepareFactionUnits(GSE_CALL,base->m_owner,base->GetTile(),{},claims);}) && fixture.Snapshot()==before,
                "empty disposition admitted a fabricated replacement site");
            auto staged=um->PrepareFactionUnits(GSE_CALL,base->m_owner,nullptr,{},claims);staged.Commit();staged.Notify(GSE_CALL);
            GT_ASSERT(um->GetUnits().empty() && um->GetUnitRecordOrder().empty() && game->GetMap()->GetTile(6,6)->units.empty()
                && game->GetMap()->GetSmacWorkerKnowledge()->CopyData().units.empty() && Unit::GetNextId()==next && fixture.GameplayRandom()==pair,
                "refused restart retained old Units, observations or consumed IDs/RNG");GT_OK();
        });
    });
    task->AddTest("SMAC Colony abandonment preserves newborn home and compacted successor cache",[](GSE*,context::Context*) -> std::string {
        // First/last, full-food and deliberately stale-N2 inputs project the
        // non-last COLONY-ABANDONMENT-001 boundary. The fifth case isolates the
        // independently measured later-record nearest-home tie from successor.
        for (int scenario=0;scenario<5;++scenario) {
            ConventionalUnitFixture fixture; fixture.ConfigureProduction(ColonyChoiceSource,"GAIANS");
            const auto error=fixture.Run([&](GSE_CALLABLE) -> std::string {
                using namespace game::backend::unit;
                auto* game=fixture.Game(); auto* bm=game->GetBM(); auto* um=game->GetUM();
                auto* first=Populate(GSE_CALL,fixture,false); SelectionPresentations(fixture); first->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
                const auto roles=Roles(); const auto target=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,0);
                const auto former=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,1);
                std::vector<Base*> bases{first};
                const size_t count=scenario==4 ? 3 : 2;
                for(size_t i=1;i<count;++i) {
                    Base::pops_t pops; pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(roles.worker),0,game->GetMap()->GetTile(4+4*i,2)));
                    auto* base=new Base(game,701+i,first->m_owner,first->m_faction,game->GetMap()->GetTile(2+4*i,2),"Abandonment survivor "+std::to_string(i),{},pops,20,0,
                        ProductionState{former,0,former,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{1,false,0,0,0,false});
                    bm->SpawnBase(GSE_CALL,base); bases.push_back(base);
                    SmacBaseAllocationState allocation{5,{{},0}}; base->PublishSmacAllocation(GSE_CALL,allocation,roles);
                    game->GetMap()->GetTile(4+4*i,2)->terraforming=TERRAFORMING_FOREST;
                }
                auto selected=*first->GetSmacAllocation(); selected.selected=5; first->PublishSmacAllocation(GSE_CALL,selected,roles);
                first->SetProductionState(GSE_CALL,ProductionState{former,0,former,0,std::vector<ProductionItem>{}});first->SetAccumulatedNutrients(0);
                const size_t lost_index=scenario==1 || scenario==4 ? 1 : 0;
                auto* lost=bases[lost_index]; const auto lost_id=lost->m_id; auto* lost_tile=lost->GetTile();
                const auto home_id=bases[scenario==4 ? 2 : 1-lost_index]->m_id;
                for(size_t i=0;i<2;++i) {
                    um->SpawnConventionalUnit(GSE_CALL,92+i,um->GetUnitDef("choice-former"),lost->m_owner,game->GetMap()->GetTile(i*2,8),
                        {1,1,1,0,0,false,ConventionalUnitState::Home::Base,SmacMoraleMode::Mode0,lost_id});
                }
                auto knowledge=KnowledgeValue(true,true,128,64).CopyData();
                for(auto& tile:knowledge.tiles) {tile.explored_by=2;tile.claimant=1;}
                knowledge.units.push_back({92,0,false});knowledge.units.push_back({93,0,false});
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
                const auto native=PublishUnrelatedNative(GSE_CALL,fixture);
                auto* cached_tile=game->GetMap()->GetTile(8,2);
                if(scenario==3) {cached_tile->terraforming=0;cached_tile->moisture=MOISTURE_RAINY;}
                for(auto* base:bases) {base->RecalculateSmacWorkers(GSE_CALL,roles);}
                if(scenario==3) {cached_tile->terraforming=TERRAFORMING_FOREST;}
                if(lost_index+1<count) {
                    const auto* successor=bases[lost_index+1];
                    GT_ASSERT(successor->GetSmacUpkeepCache() && successor->GetSmacUpkeepCache()->calculation.nutrients.surplus==(scenario==3 ? 2 : 1),"fixture did not retain the original successor surplus");
                    GT_ASSERT(successor->PrepareSmacResourceCalculation().nutrients.surplus==1,"fresh successor resource query differs from forest input");
                }
                // The accepted-choice caller has posted 27+3, but has not
                // committed the old baseline or collected its nutrient box.
                GT_ASSERT(game->GetSmacProductionCost(lost->m_owner,target)==30,"literal Colony cost differs");
                lost->SetProductionState(GSE_CALL,ProductionState{target,30,target,27,std::vector<ProductionItem>{}});
                lost->SetAccumulatedNutrients(scenario==2 ? 20 : 0);
                auto* base_wrapper=static_cast<value::Object*>(lost->Wrap(GSE_CALL));
                auto* pop_wrapper=static_cast<value::Object*>(lost->m_pops.begin()->second.Wrap(GSE_CALL));
                std::vector<std::pair<size_t,std::string>> survivors;
                for(const auto* base:bases) {if(base!=lost) {survivors.emplace_back(base->m_id,Base::Serialize(base).ToString());}}
                const auto pair=fixture.GameplayRandom();const auto jsf=game->GetRandom()->GetStateString();
                const auto scout=Unit::Serialize(um->GetUnit(91)).ToString();const auto next=Unit::GetNextId();fixture.Updates();
                fixture.ResourceState([&](auto& state){state.economy.at(1).regional_base_counts->at(1)=scenario==0 ? 0 : 77;});
                auto* lost_owner=lost->m_owner;
                const auto result=bm->AbandonSmacBase(GSE_CALL,*lost,um->GetUnitDef("choice-colony"),SmacMoraleMode::Mode0);
                GT_ASSERT(game->GetCanonicalEconomy(lost_owner).regional_base_counts->at(1)==(scenario==0 ? 255 : 76),"abandonment recounted or failed to decrement the stored byte");
                GT_ASSERT(result.newborn_id==next && !bm->GetBase(lost_id) && !lost_tile->base && !base_wrapper->wrapobj && !pop_wrapper->wrapobj,"abandonment failed to create/retire its actual records");
                const auto* newborn=um->GetUnit(result.newborn_id);
                GT_ASSERT(newborn && newborn->GetTile()==lost_tile && newborn->Conventional().home==ConventionalUnitState::Home::None
                    && !newborn->Conventional().home_base_id && newborn->Conventional().damage==0 && newborn->Conventional().spent==0
                    && newborn->Conventional().work_order==ConventionalUnitState::WorkOrder::None,"newborn was rehomed, damaged, ordered or moved");
                for(size_t id=92;id<94;++id) {GT_ASSERT(um->GetUnit(id)->Conventional().home_base_id==home_id,"older home repair differs from nearest/later-record rule");}
                GT_ASSERT(Unit::Serialize(um->GetUnit(91)).ToString()==scout && fixture.GameplayRandom()==pair && game->GetRandom()->GetStateString()==jsf,"abandonment changed unhomed Scout or RNG");
                std::vector<size_t> order;for(const auto& survivor:survivors) {
                    order.push_back(survivor.first);GT_ASSERT(Base::Serialize(bm->GetBase(survivor.first)).ToString()==survivor.second,"provider eagerly recalculated or posted successor stores");
                }
                GT_ASSERT(bm->GetBaseRecordOrder()==order,"record compaction changed stable survivor order");
                if(lost_index+1<count) {
                    GT_ASSERT(result.successor_id==701+lost_index+1 && result.successor_cache
                        && result.successor_cache->calculation.nutrients.surplus==(scenario==3 ? 2 : 1),"compacted successor lost its retained tail authority");
                } else {GT_ASSERT(!result.successor_id && !result.successor_cache,"last record revisited an earlier Base");}
                const auto after=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                GT_ASSERT(native.Matches(fixture),"Colony abandonment changed unrelated native body or factory");
                GT_ASSERT(after.units.size()==5 && after.units.back().id==result.newborn_id,"territory commit discarded newborn sight observation");
                size_t spawn=0,despawn=0;for(const auto& update:fixture.Updates()) {
                    if(update.type==game::FrontendRequest::FR_UNIT_SPAWN && update.data.unit_spawn.unit_id==result.newborn_id) {++spawn;}
                    if(update.type==game::FrontendRequest::FR_BASE_DESPAWN && update.data.base_despawn.base_id==lost_id) {++despawn;}
                }
                GT_ASSERT(spawn==1 && despawn==1,"abandonment duplicated or lost frontend publication");GT_OK();
            });
            if(!error.empty()) {return "abandonment scenario "+std::to_string(scenario)+": "+error;}
        }
        return {};
    });
    task->AddTest("SMAC Colony Accept consumes compacted cached surplus without duplicate posting",[](GSE*,context::Context*) -> std::string {
        // COLONY-ABANDONMENT-001: the successor receives the remainder of the
        // abandoned record's call, not a fresh mineral-production pass. The
        // stale control must consume N2 before the final forest pass returns N1.
        for(const bool stale:{false,true}) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(ColonyChoiceSource,"GAIANS");
            fixture.ResourceState([](auto& state){
                auto& economy=state.economy.at(1);economy.rates={0,5};economy.credits=100;economy.pending_debit=7;economy.research_fraction=0;
                state.social.factions[0].current[1]=2;state.social.factions[0].current[10]=0;
                auto& research=state.technology.factions[0];research.target=5;research.cached_threshold=1000;research.labs=0;
            });
            size_t newborn_id=0;std::string scout_before,random_before;
            auto pair=fixture.GameplayRandom();
            const auto prefix=fixture.Run([&](GSE_CALLABLE)->std::string {
                using namespace game::backend::unit;
                auto* game=fixture.Game();auto* bm=game->GetBM();auto* um=game->GetUM();
                auto* first=Populate(GSE_CALL,fixture,false);first->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");SelectionPresentations(fixture);
                const auto roles=Roles();std::array<std::string,8> names;names[0]=roles.worker;
                for(size_t i=0;i<7;++i){names[i+1]=roles.specialists[i];}fixture.BasePresentationRoles(names);
                auto allocation=*first->GetSmacAllocation();allocation.selected=5;first->PublishSmacAllocation(GSE_CALL,allocation,roles);
                const auto former=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,1);
                auto* forest=game->GetMap()->GetTile(8,2);forest->terraforming=TERRAFORMING_FOREST;
                Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(roles.worker),0,forest));
                auto* second=new Base(game,702,first->m_owner,first->m_faction,game->GetMap()->GetTile(6,2),"Compacted successor",{},pops,20,0,
                    ProductionState{former,0,former,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{1,false,0,0,0,false});
                bm->SpawnBase(GSE_CALL,second);second->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{5,{{},0}},roles);
                for(size_t i=0;i<2;++i){um->SpawnConventionalUnit(GSE_CALL,92+i,um->GetUnitDef("choice-former"),first->m_owner,game->GetMap()->GetTile(i*2,8),
                    {1,1,1,0,0,false,ConventionalUnitState::Home::Base,SmacMoraleMode::Mode0,first->m_id});}
                auto knowledge=KnowledgeValue(true,true,128,64).CopyData();for(auto& tile:knowledge.tiles){tile.explored_by=2;tile.claimant=1;}
                knowledge.units.push_back({92,0,false});knowledge.units.push_back({93,0,false});game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
                if(stale){forest->terraforming=0;forest->moisture=MOISTURE_RAINY;}
                second->RecalculateSmacWorkers(GSE_CALL,roles);
                if(stale){forest->terraforming=TERRAFORMING_FOREST;}
                GT_ASSERT(second->GetSmacUpkeepCache()->calculation.nutrients.surplus==(stale?2:1)
                    && second->PrepareSmacResourceCalculation().nutrients.surplus==1,"Accept fixture did not preserve its recorded versus fresh surplus");
                const auto colony=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,0);
                first->SetProductionState(GSE_CALL,ProductionState{colony,27,colony,27,std::vector<ProductionItem>{}});first->SetAccumulatedNutrients(0);
                newborn_id=Unit::GetNextId();scout_before=Unit::Serialize(um->GetUnit(91)).ToString();pair=fixture.GameplayRandom();random_before=game->GetRandom()->GetStateString();fixture.Updates();
                fixture.BeginColonyChoice(GSE_CALL,first->m_id);
                GT_ASSERT(first->GetProductionState()->minerals==30 && first->GetAccumulatedNutrients()==0
                    && second->GetProductionState()->minerals==0 && second->GetAccumulatedNutrients()==0,"question posted food or ran successor before Accept");
                auto* pending=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony"));
                const auto request=static_cast<value::Int*>(pending->Get("request_id"))->value;
                for(int i=0;i<2;++i){game->Event(GSE_CALL,"choose_colony",{{"request_id",VALUE(value::Int,,request)},{"choice",VALUE(value::String,,"accept")}});}
                GT_OK();
            });GT_ASSERT(prefix.empty(),"Accept prefix: "+prefix);
            fixture.DrainEvents(); // actual queued dispatch, outside owning source accumulation
            const auto tail=fixture.Run([&](GSE_CALLABLE)->std::string {
                using namespace game::backend::unit;
                auto* game=fixture.Game();auto* bm=game->GetBM();auto* um=game->GetUM();auto* survivor=bm->GetBase(702);
                GT_ASSERT(!fixture.Snapshot().empty() && !bm->GetBase(701) && survivor && bm->GetBaseRecordOrder()==std::vector<size_t>{702},"Accept did not finish with the actual compacted survivor");
                GT_ASSERT(survivor->GetAccumulatedNutrients()==(stale?2:1) && survivor->m_pops.size()==1
                    && survivor->GetProductionState()->minerals==0 && survivor->GetProductionState()->baseline_minerals==0
                    && survivor->GetProductionState()->current_item.smac_source_id==1 && survivor->GetProductionState()->baseline_item.smac_source_id==1,
                    "Accept recomputed cached food early or reposted successor mineral production");
                const auto& cache=survivor->GetSmacUpkeepCache();
                GT_ASSERT(cache && cache->calculation.nutrients.surplus==1 && cache->calculation.nutrients.stored_before==(stale?2:1),"Accept omitted the later resource pass or collected food twice");
                const auto* newborn=um->GetUnit(newborn_id);
                GT_ASSERT(newborn && Unit::GetNextId()==newborn_id+1 && newborn->GetTile()==game->GetMap()->GetTile(2,2)
                    && newborn->Conventional().home==ConventionalUnitState::Home::None && !newborn->Conventional().home_base_id
                    && newborn->Conventional().damage==0 && newborn->Conventional().spent==0,"Accept lost, rehomed or duplicated the actual Pod");
                GT_ASSERT(um->GetUnit(92)->Conventional().home_base_id==702 && um->GetUnit(93)->Conventional().home_base_id==702
                    && Unit::Serialize(um->GetUnit(91)).ToString()==scout_before,"Accept home repair changed newborn/unhomed semantics");
                bool research=false;fixture.ResourceState([&](auto& state){research=state.technology.factions[0].labs==1;});
                GT_ASSERT(game->GetCanonicalEconomy(survivor->m_owner).credits==94 && research,"Accept skipped or duplicated successor research/credit or faction debit");
                GT_ASSERT(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony")->type==VT_NULL
                    && fixture.GameplayRandom()==pair && game->GetRandom()->GetStateString()==random_before,"Accept retained a question or consumed unexpected RNG");
                size_t spawn=0,despawn=0;for(const auto& update:fixture.Updates()){
                    if(update.type==game::FrontendRequest::FR_UNIT_SPAWN && update.data.unit_spawn.unit_id==newborn_id){++spawn;}
                    if(update.type==game::FrontendRequest::FR_BASE_DESPAWN && update.data.base_despawn.base_id==701){++despawn;}
                }
                GT_ASSERT(spawn==1 && despawn==1,"duplicate Accept repeated a lifecycle publication");GT_OK();
            });GT_ASSERT(tail.empty(),"Accept tail: "+tail);
        }GT_OK();
    });
    task->AddTest("SMAC live restart matches original starvation site CRT and post-search veto consumption",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        const auto& native=smac_restart_fixture::cases.front();
        for(int scenario=0;scenario<6;++scenario) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(RestartSource,"GAIANS",64,32);
            fixture.ResourceState([&](auto& state){state.do_or_die=scenario==1;state.roster[0].restart_prohibited=scenario==2;});
            std::string saved;std::vector<size_t> created;
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,true);SelectionPresentations(fixture);
                game->AdvanceTurn(3);base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,64,32));
                // A real dry-map postpass produced F=0. Subsequent displayed
                // moisture changes must not repair that retained native input.
                const auto before=*game->GetMap()->GetTerritoryTopology();
                GT_ASSERT(before.Areas()->at(1)==native.area && before.Postpass()->fertility.at(1)==native.fertility,"native retained geography differs");
                for(size_t y=0;y<32;++y) {for(size_t x=y&1;x<64;x+=2) {
                    auto* tile=game->GetMap()->GetTile(x,y);tile->moisture=MOISTURE_RAINY;
                    if(scenario==3) {tile->features=FEATURE_XENOFUNGUS;}
                }}
                if(scenario==1 || scenario==2) {
                    const auto marked=before.SiteRatings()->FoundBase(*native.selected);fixture.SiteRatings(marked.Serialize());
                }
                if(scenario==5) {game->GetMap()->GetTile((*native.selected)[0],(*native.selected)[1])->features=FEATURE_MONOLITH;}
                const auto& reference=scenario==3 ? smac_restart_fixture::cases.back() : native;
                fixture.GameplayRandom({reference.shared,reference.crt});const auto jsf=game->GetRandom()->GetStateString();
                const auto first=Unit::GetNextId();const auto colony=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
                base->SetProductionState(GSE_CALL,ProductionState{colony,999,colony,0,std::vector<ProductionItem>{}});
                if(scenario==4) {
                    auto legacy=game->GetMap()->GetTerritoryTopology()->Serialize();legacy.resize(legacy.size()-1024-map::smac_territory::TerritorySummary::WIRE_BYTES);
                    legacy.replace(0,8,"GLSTERR3");legacy[15]=static_cast<char>(static_cast<uint8_t>(legacy[15])&1);fixture.TerritoryTopology(legacy);
                    fixture.Updates();const auto original=fixture.Snapshot();const auto queues=fixture.QueuedFrontendRequests().size();
                    auto* retained=static_cast<value::Object*>(base->Wrap(GSE_CALL));
                    GT_ASSERT(Rejects([&]{game->GetBM()->AbandonSmacBase(GSE_CALL,*base,game->GetUM()->GetUnitDef("choice-colony"),SmacMoraleMode::Mode0);})
                        && fixture.Snapshot()==original && Unit::GetNextId()==first && retained->wrapobj
                        && fixture.QueuedFrontendRequests().size()==queues,"missing-history preparation changed authority IDs wrappers RNG or queues");GT_OK();
                }
                game->GetBM()->AbandonSmacBase(GSE_CALL,*base,game->GetUM()->GetUnitDef("choice-colony"),SmacMoraleMode::Mode0);
                GT_ASSERT(fixture.GameplayRandom()==SmacGameplayRandomState(reference.shared,reference.final_crt)
                    && game->GetRandom()->GetStateString()==jsf,"live search omitted native CRT prefix or consumed shared/map RNG");
                GT_ASSERT(game->GetBM()->GetBases().empty() && !game->GetUM()->GetUnit(91),"future restart kept the lost Base or victim Unit");
                if(scenario==1 || scenario==2) {
                    const auto& sites=*game->GetMap()->GetTerritoryTopology()->SiteRatings();
                    GT_ASSERT(sites.Excluded(*native.selected) && sites.Rating(*native.selected)==1,"later restart veto discarded an actual excluded cache-miss write");
                }
                if(scenario==5) {GT_ASSERT(game->GetMap()->GetTerritoryTopology()->SiteRatings()->Rating(*native.selected)==0,"monolith zero score did not remain a miss");}
                if(scenario==0 || scenario==5) {
                    GT_ASSERT(game->GetUM()->GetUnits().size()==4 && native.selected,"original starvation package missing");
                    for(size_t i=0;i<4;++i) {
                        const auto* unit=game->GetUM()->GetUnit(first+1+i);
                        GT_ASSERT(unit && unit->GetTile()->coord.x==size_t((*native.selected)[0]) && unit->GetTile()->coord.y==size_t((*native.selected)[1]),
                            "live caller selected another site than the original starvation trace");created.push_back(unit->m_id);
                    }
                    saved=fixture.Snapshot();
                } else {GT_ASSERT(game->GetUM()->GetUnits().empty(),"failed/vetoed search manufactured replacement Units");}
                GT_OK();
            });if(!result.empty()) {return "native live restart "+std::to_string(scenario)+": "+result;}
            if(scenario==0) {
                ConventionalUnitFixture restored;restored.ConfigureProduction(RestartSource,"GAIANS",64,32);
                const auto result=restored.Run([&](GSE_CALLABLE)->std::string {
                    restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});
                    GT_ASSERT(restored.GameplayRandom()==SmacGameplayRandomState(native.shared,native.final_crt)
                        && restored.Game()->GetUM()->GetUnits().size()==4,"actual Start replayed the candidate loop");
                    for(const auto id:created) {GT_ASSERT(restored.Game()->GetUM()->GetUnit(id),"actual Start lost a native restart replacement");}GT_OK();
                });if(!result.empty()) {return result;}
            }
        }GT_OK();
    });
    task->AddTest("SMAC last Base loss restarts or eliminates using saved faction authority",[](GSE*,context::Context*) -> std::string {
        using namespace game::backend::unit;
        for(const bool accept:{false,true}) {for(int scenario=0;scenario<6;++scenario) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(RestartSource,"GAIANS");
            std::string saved,knowledge_wire,random_wire;
            std::optional<faction::SmacSimulationState> expected_state;
            std::map<size_t,std::string> expected_units;
            fixture.ResourceState([&](auto& state){
                state.do_or_die=scenario==3;state.roster[0].restart_prohibited=scenario==4;
                state.capabilities.emplace();
                state.capabilities->factions[0].available.fill(70000);
                state.capabilities->factions[0].foreign.fill(-73);
                state.capabilities->factions[1].available.fill(12345);
                auto& e=state.economy.at(1);e.credits=100;e.pending_debit=7;e.research_fraction=70;e.rates={2,3};
                auto& t=state.technology.factions[0];t.target=5;t.cached_threshold=1000;t.labs=40;
            });
            const auto error=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* bm=game->GetBM();auto* um=game->GetUM();
                game->AdvanceTurn(scenario==1 ? 100 : scenario==2 ? 101 : 3);
                auto* base=Populate(GSE_CALL,fixture,true);SelectionPresentations(fixture);
                base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
                if(scenario==5) {for(size_t y=0;y<64;++y) {for(size_t x=y&1;x<128;x+=2) {game->GetMap()->GetTile(x,y)->features=FEATURE_XENOFUNGUS;}}}
                const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,accept ? 0 : 2);
                base->SetProductionState(GSE_CALL,ProductionState{target,999,target,0,std::vector<ProductionItem>{}});
                base->SetAccumulatedNutrients(-1);
                if(!accept) {auto context=base->GetSmacResourceContext();context.resource_events=SmacBaseResourceContext::Famine;base->SetSmacResourceContext(GSE_CALL,context);}
                game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));fixture.Updates();
                const auto first=Unit::GetNextId();const auto pair=fixture.GameplayRandom();const auto random=game->GetRandom()->GetStateString();
                auto* wrapper=static_cast<value::Object*>(base->Wrap(GSE_CALL));
                if(accept) {bm->AbandonSmacBase(GSE_CALL,*base,um->GetUnitDef("choice-colony"),SmacMoraleMode::Mode0);}
                else {const auto result=base->ProcessSmacUpkeep(GSE_CALL,Roles(),[&](int32_t){return um->GetUnitDef("choice-scout");},SmacMoraleMode::Mode0);
                    GT_ASSERT(result.removed_base && !result.completed_unit,"starvation manufactured before faction disposition");}
                const bool restarted=scenario<2;const size_t count=restarted ? scenario==1 ? 7 : 4 : 0;
                GT_ASSERT(bm->GetBases().empty() && !wrapper->wrapobj && !um->GetUnit(91) && um->GetUnits().size()==count,
                    "last Base or old roster survived disposition, or replacement count differs: bases="+std::to_string(bm->GetBases().size())
                    +" units="+std::to_string(um->GetUnits().size())+" expected="+std::to_string(count)+" CRT="+std::to_string(fixture.GameplayRandom().Crt()));
                const std::vector<int32_t> designs=scenario==1 ? std::vector<int32_t>{0,1,0,2,2,2,2} : std::vector<int32_t>{0,1,0,2};
                map::tile::Tile* site=nullptr;
                for(size_t i=0;i<count;++i) {
                    const auto* created=um->GetUnit(first+(accept ? 1 : 0)+i);GT_ASSERT(created,"restart factory skipped its next actual identity");
                    if(!site) {site=created->GetTile();}
                    GT_ASSERT(created->GetTile()==site && created->Conventional().design_id==designs[i]
                        && created->Conventional().stored_morale==1 && created->Conventional().home==ConventionalUnitState::Home::None,
                        "restart factory lost package order, common site or fresh state");
                }
                bool exact=false;fixture.ResourceState([&](auto& state){const auto& e=state.economy.at(1);const auto& t=state.technology.factions[0];
                    exact=state.roster[0].active==restarted && state.human_defeat_request==(restarted ? std::nullopt : std::optional<int32_t>(1))
                        && state.capabilities && state.capabilities->factions[0]==faction::SmacCapabilityState::Faction{}
                        && state.capabilities->factions[1].available[0]==12345
                        && state.roster[0].restart_prohibited==(scenario==4) && e.credits==110
                        && e.pending_debit==0 && e.research_fraction==70 && t.target==5 && t.labs==40 && !t.cached_threshold
                        && e.rates.psych==(restarted ? 0 : 2) && e.rates.labs==(restarted ? 5 : 3);});
                GT_ASSERT(exact && fixture.GameplayRandom().Shared()==pair.Shared() && fixture.GameplayRandom().Crt()!=pair.Crt()
                    && game->GetRandom()->GetStateString()==random,"disposition reset retained research, flags, treasury or search RNG incorrectly");
                // A second nonzero initializer clears only its owner's row;
                // a restarted owner with surviving Units does not initialize again.
                fixture.ResourceState([&](auto& state){state.capabilities->factions[0].available.fill(70000);});
                fixture.SettleEconomy(1);
                fixture.ResourceState([&](auto& state){exact=state.economy.at(1).credits==(restarted ? 110 : 120)
                    && state.capabilities->factions[0].available[0]==(restarted ? 70000 : 0)
                    && state.capabilities->factions[1].available[0]==12345;});
                GT_ASSERT(exact,"empty-faction tail omitted or duplicated its second initialization");
                saved=fixture.Snapshot();knowledge_wire=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();random_wire=random;
                fixture.ResourceState([&](auto& state){expected_state=state;});
                for(const auto& row:um->GetUnits()) {expected_units.emplace(row.first,Unit::Serialize(row.second).ToString());}GT_OK();
            });if(!error.empty()) {return std::string(accept ? "accept " : "starvation ")+std::to_string(scenario)+": "+error;}
            ConventionalUnitFixture restored;restored.ConfigureProduction(RestartSource,"GAIANS");
            const auto restore=restored.Run([&](GSE_CALLABLE)->std::string {
                restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE) {});
                auto* game=restored.Game();bool retained=false;restored.ResourceState([&](auto& state){retained=state==*expected_state;});
                GT_ASSERT(retained && game->GetBM()->GetBases().empty() && game->GetUM()->GetUnits().size()==expected_units.size()
                    && game->GetTurnId()==size_t(scenario==1 ? 100 : scenario==2 ? 101 : 3)
                    && game->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge_wire && game->GetRandom()->GetStateString()==random_wire,
                    "actual Start replayed loss initialization, sight or saved simulation state");
                for(const auto& row:expected_units) {GT_ASSERT(game->GetUM()->GetUnit(row.first) && Unit::Serialize(game->GetUM()->GetUnit(row.first)).ToString()==row.second,
                    "actual Start replayed or changed a replacement factory");}GT_OK();
            });if(!restore.empty()) {return "loss restore: "+restore;}
        }}GT_OK();
    });
    task->AddTest("SMAC last Base Accept continues through actual founding save Start and next upkeep",[](GSE*,context::Context*) -> std::string {
        using namespace game::backend::unit;
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(RestartSource,"GAIANS");
        fixture.ResourceState([](auto& state){
            auto& e=state.economy.at(1);e.credits=100;e.pending_debit=7;e.research_fraction=70;e.rates={2,3};
            auto& t=state.technology.factions[0];t.target=5;t.cached_threshold=1000;t.labs=0;
            state.social.factions[0].current[10]=0;
        });
        std::array<std::string,8> names;names[0]=Roles().worker;for(size_t i=0;i<7;++i){names[i+1]=Roles().specialists[i];}
        fixture.BasePresentationRoles(names);std::string saved;size_t founded_id=0;size_t survivor_count=0;
        const auto first=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* bm=game->GetBM();auto* um=game->GetUM();
            game->AdvanceTurn(3);base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");SelectionPresentations(fixture);
            bm->DefineFacility(new FacilityDef("restart-hq","HQ","",0,{},1));
            for(size_t y=0;y<64;++y) {for(size_t x=y&1;x<128;x+=2) {game->GetMap()->GetTile(x,y)->moisture=MOISTURE_RAINY;}}
            const auto colony=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
            base->SetProductionState(GSE_CALL,ProductionState{colony,30,colony,27,std::vector<ProductionItem>{}});
            game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(true,true,128,64));
            const auto next=Unit::GetNextId();fixture.BeginColonyChoice(GSE_CALL,base->m_id);
            auto* api=static_cast<value::Object*>(game->Wrap(GSE_CALL));auto* pending=static_cast<value::Object*>(Call(GSE_CALL,api,"get_pending_smac_colony"));
            game->Event(GSE_CALL,"choose_colony",{{"request_id",pending->Get("request_id")},{"choice",VALUE(value::String,,"accept")}});fixture.DrainEvents();
            GT_ASSERT(Call(GSE_CALL,api,"get_pending_smac_colony")->type==VT_NULL && bm->GetBases().empty() && um->GetUnits().size()==4,
                "last Base Accept did not finish the actual suspended turn with its package");
            auto* pod=um->GetUnit(next+1);GT_ASSERT(pod && pod->Conventional().design_id==0,"Accept omitted the first replacement Pod");
            auto* site=pod->GetTile();game->Event(GSE_CALL,"found_base",{{"unit",pod->Wrap(GSE_CALL)}});fixture.DrainEvents();
            auto* founded=site->base;GT_ASSERT(founded && founded->m_pops.size()==1 && founded->m_facilities.count("restart-hq")==1
                && founded->GetAccumulatedNutrients()==0 && founded->GetProductionState()->minerals==10 && founded->GetProductionState()->current_item.smac_source_id==2,
                "restarted Pod failed to found its actual first replacement Base with HQ and founding credit");
            founded_id=founded->m_id;survivor_count=um->GetUnits().size();saved=fixture.Snapshot();GT_OK();
        });if(!first.empty()) {return "restart/found: "+first;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(RestartSource,"GAIANS");restored.BasePresentationRoles(names);
        restored.ProductionPresentations({"choice-scout","choice-rover","choice-former","choice-colony"});
        return restored.Run([&](GSE_CALLABLE)->std::string {
            // World bodies restore simulation state into an already admitted
            // roster. The campaign envelope supplies this Player record.
            restored.Game()->GetState()->m_slots->GetSlot(0).GetPlayer()->SetDifficultyLevel("Librarian");
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* base=game->GetBM()->GetBase(founded_id);
            GT_ASSERT(base && base->GetAccumulatedNutrients()==0 && base->GetProductionState()->minerals==10,"Start replayed the founded Base");
            game->Event(GSE_CALL,"complete_turn",{});restored.DrainEvents();
            GT_ASSERT(game->GetTurnId()==4 && base->m_pops.size()==1 && base->GetAccumulatedNutrients()==2
                && base->GetProductionState()->minerals==1 && base->GetProductionState()->baseline_minerals==1
                && game->GetUM()->GetUnits().size()==survivor_count+1,"replacement Base failed its next actual upkeep or Scout production");
            bool exact=false;std::string diagnostic;restored.ResourceState([&](auto& state){const auto& e=state.economy.at(1);const auto& t=state.technology.factions[0];
                exact=state.roster[0].active && e.credits==111 && e.research_fraction==70 && t.labs==1 && t.target==5;
                diagnostic=" credits="+std::to_string(e.credits)+" fraction="+std::to_string(e.research_fraction)+" labs="+std::to_string(t.labs);});
            GT_ASSERT(exact,"continuous restart/founding/upkeep differs from treasury110->111 and research0->1 receipt"+diagnostic);GT_OK();
        });
    });
    task->AddTest("SMAC legacy base-less Governor recovery requires explicit acceptance and atomically founds",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(RestartSource,"GAIANS");std::string saved;
        const auto prepared=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* base=Populate(GSE_CALL,fixture,false);auto* owner=base->m_owner;
            game->GetBM()->DespawnBase(GSE_CALL,base->m_id);SelectionPresentations(fixture);
            game->GetUM()->SpawnConventionalUnit(GSE_CALL,92,game->GetUM()->GetUnitDef("choice-colony"),owner,game->GetMap()->GetTile(6,2),
                {0,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
            auto knowledge=KnowledgeValue(true,true,128,64).CopyData();knowledge.units.push_back({92,0,false});
            game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
            fixture.ResourceState([](auto& state){state.remembered_governor[1].reset();state.remembered_governor[0]=0x12345678u;state.remembered_governor[7]=0xffffffffu;});
            saved=fixture.Snapshot();GT_OK();
        });GT_ASSERT(prepared.empty(),"legacy recovery setup: "+prepared);
        ConventionalUnitFixture restored;restored.ConfigureProduction(RestartSource,"GAIANS");
        return restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();auto* bm=game->GetBM();auto* um=game->GetUM();auto* pod=um->GetUnit(92);
            std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t n=0;n<7;++n){roles[n+1]=Roles().specialists[n];}restored.BasePresentationRoles(roles);
            std::string expected;int prompts=0;
            game->On(GSE_CALL,"smac_governor_foundation",NATIVE_CALL(&expected,&prompts) {
                N_EXPECT_ARGS(1);N_GET(view,0,Object);expected=static_cast<value::String*>(view->Get("expected_state"))->value;++prompts;return VALUE(value::Undefined);
            }));
            const auto before=restored.Snapshot();const auto random=restored.GameplayRandom();
            game->Event(GSE_CALL,"found_base",{{"unit",pod->Wrap(GSE_CALL)}});restored.DrainEvents();
            GT_ASSERT(prompts==1 && !expected.empty() && restored.Snapshot()==before && um->GetUnit(92)==pod,"Opening recovery changed absent settings or consumed Pod");
            // Cancel is a UI-only dismissal: the native opening leaves no state.
            game->Event(GSE_CALL,"found_base",{{"unit",pod->Wrap(GSE_CALL)}});restored.DrainEvents();
            GT_ASSERT(prompts==2 && restored.Snapshot()==before,"Cancelled/reopened recovery silently initialized settings");
            game->Event(GSE_CALL,"found_base_with_default_governor",{{"unit",pod->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,expected+"stale")}});restored.DrainEvents();
            GT_ASSERT(restored.Snapshot()==before,"Stale default choice founded or initialized settings");
            const auto defaults=SmacBaseResourceContext::NewGameGovernorOptions(2);
            GT_ASSERT(Rejects([&]{bm->FoundSmacBase(GSE_CALL,*pod,roles,defaults);}) && restored.Snapshot()==before,"Late missing HQ facility initialized template before failed founding");
            bm->DefineFacility(new FacilityDef("legacy-recovery-hq","Headquarters","",0,{},1));
            game->Event(GSE_CALL,"found_base_with_default_governor",{{"unit",pod->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,expected)}});
            game->Event(GSE_CALL,"found_base_with_default_governor",{{"unit",pod->Wrap(GSE_CALL)},{"expected_state",VALUE(value::String,,expected)}});restored.DrainEvents();
            bool known=false;restored.ResourceState([&](auto& state){known=state.remembered_governor[1]==SmacBaseResourceContext::NewGameGovernorOptions(state.designs.global_difficulty)
                && state.remembered_governor[0]==0x12345678u && state.remembered_governor[7]==0xffffffffu;});
            GT_ASSERT(known && !um->GetUnit(92) && bm->GetBases().size()==1 && bm->GetBases().begin()->second->GetSmacResourceContext().governor_options==defaults
                && restored.GameplayRandom()==random,"Accepted recovery failed to join template/Base/Pod publication or overwrote peers/RNG");GT_OK();
        });
    });
    task->AddTest("SMAC zero Base founding uses first Base population and facility turn packages",[](GSE*,context::Context*) -> std::string {
        using namespace game::backend::unit;
        struct Case {int32_t turn,population;std::vector<int32_t> facilities;};
        const std::vector<Case> cases{{3,1,{1}},{50,1,{1}},{51,2,{1}},{74,2,{1}},{75,2,{1,3}},{99,2,{1,3}},
            {100,2,{1,3,6}},{150,3,{1,3,6,7}},{200,4,{1,3,6,7}},{201,5,{1,3,6,7}}};
        for(const auto& row:cases) {
            ConventionalUnitFixture fixture;fixture.ConfigureProduction(RestartSource,"GAIANS");
            const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
                auto* game=fixture.Game();auto* old=Populate(GSE_CALL,fixture,false);auto* owner=old->m_owner;auto* bm=game->GetBM();auto* um=game->GetUM();
                bm->DespawnBase(GSE_CALL,old->m_id);SelectionPresentations(fixture);game->AdvanceTurn(row.turn);
                for(const int32_t id:{1,3,6,7}) {bm->DefineFacility(new FacilityDef("package-"+std::to_string(id),"Facility","",0,{},id));}
                for(size_t y=0;y<64;++y) {for(size_t x=y&1;x<128;x+=2) {game->GetMap()->GetTile(x,y)->moisture=MOISTURE_RAINY;}}
                auto* pod=um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("choice-colony"),owner,game->GetMap()->GetTile(6,2),
                    {0,1,1,0,0,false,ConventionalUnitState::Home::None,SmacMoraleMode::Mode0});
                auto knowledge=KnowledgeValue(true,true,128,64).CopyData();knowledge.units.push_back({92,0,false});
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
                std::array<std::string,8> roles;roles[0]=Roles().worker;for(size_t i=0;i<7;++i){roles[i+1]=Roles().specialists[i];}
                auto* founded=bm->FoundSmacBase(GSE_CALL,*pod,roles);
                GT_ASSERT(founded->m_pops.size()==size_t(row.population) && founded->m_facilities.size()==row.facilities.size(),"first Base population or package count differs");
                for(const auto id:row.facilities) {GT_ASSERT(founded->m_facilities.count("package-"+std::to_string(id))==1,"first Base omitted a turn-based facility");}
                const auto& cache=founded->GetSmacUpkeepCache();
                GT_ASSERT(cache && cache->calculation.nutrients.demand==2*row.population && cache->support.units.size()==1
                    && cache->support.units[0].id==91 && !um->GetUnit(92),"first Base cache retained single-pop demand or consumed Pod");
                (void)fixture.Snapshot();GT_OK();
            });if(!result.empty()) {return "first Base turn "+std::to_string(row.turn)+": "+result;}
        }GT_OK();
    });
    task->AddTest("SMAC Colony abandonment rejects missing retained and invalid factory authority atomically",[](GSE*,context::Context*) -> std::string {
        for (const bool last:{false,true}) {
            ConventionalUnitFixture fixture; fixture.ConfigureProduction(ColonyChoiceSource,"GAIANS");
            const auto error=fixture.Run([&](GSE_CALLABLE) -> std::string {
                using namespace game::backend::unit;
                auto* game=fixture.Game();auto* bm=game->GetBM();auto* um=game->GetUM();
                auto* base=Populate(GSE_CALL,fixture,false);SelectionPresentations(fixture);const auto roles=Roles();base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian");
                const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
                base->SetProductionState(GSE_CALL,ProductionState{target,30,target,27,std::vector<ProductionItem>{}});
                Base* survivor=nullptr;
                if(!last) {
                    Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(roles.specialists[1]),1));
                    survivor=new Base(game,702,base->m_owner,base->m_faction,game->GetMap()->GetTile(6,2),"Uncalculated survivor",{},pops,20,0,
                        ProductionState{target,0,target,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{1,false,0,0,0,false});
                    bm->SpawnBase(GSE_CALL,survivor);SmacBaseAllocationState allocation{1,{{},1}};allocation.specialists.types[0]=1;
                    survivor->PublishSmacAllocation(GSE_CALL,allocation,roles);
                }
                auto knowledge=KnowledgeValue(true,true,128,64).CopyData();for(auto& tile:knowledge.tiles){tile.explored_by=2;tile.claimant=1;}
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));fixture.Updates();
                const auto before=fixture.Snapshot();const auto next=Unit::GetNextId();const auto pair=fixture.GameplayRandom();
                GT_ASSERT(Rejects([&]{bm->AbandonSmacBase(GSE_CALL,*base,um->GetUnitDef(last ? "live-scout" : "choice-colony"),SmacMoraleMode::Mode0);})
                    && fixture.Snapshot()==before && Unit::GetNextId()==next && fixture.GameplayRandom()==pair && fixture.Updates().empty(),"unavailable continuation published partial abandonment");
                if(survivor) {
                    GT_ASSERT(!survivor->GetSmacUpkeepCache(),"allocation/query fabricated retained calculation");
                    survivor->RecalculateSmacWorkers(GSE_CALL,roles);fixture.Updates();const auto calculated=fixture.Snapshot();
                    GT_ASSERT(Rejects([&]{bm->AbandonSmacBase(GSE_CALL,*base,um->GetUnitDef("live-scout"),SmacMoraleMode::Mode0);})
                        && fixture.Snapshot()==calculated && Unit::GetNextId()==next && fixture.GameplayRandom()==pair && fixture.Updates().empty(),"late factory rejection changed Base/home/queue authority");
                    // A real Famine resource pass leaves the compacted successor
                    // at N1 - demand2. Accept must reject before retiring the old
                    // Base or creating its Pod, not fail later inside that tail.
                    auto context=survivor->GetSmacResourceContext();context.resource_events=SmacBaseResourceContext::Famine;
                    survivor->SetSmacResourceContext(GSE_CALL,context);survivor->SetAccumulatedNutrients(0);
                    survivor->RecalculateSmacWorkers(GSE_CALL,roles);fixture.Updates();
                    GT_ASSERT(survivor->m_pops.size()==1 && survivor->GetAccumulatedNutrients()==0
                        && survivor->GetSmacUpkeepCache() && survivor->GetSmacUpkeepCache()->calculation.nutrients.surplus==-1,
                        "starving successor did not retain its actual negative resource pass");
                    const auto starving=fixture.Snapshot();const auto order=bm->GetBaseRecordOrder();const auto units=um->GetUnitRecordOrder();
                    const auto knowledge=game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
                    const auto scout=Unit::Serialize(um->GetUnit(91)).ToString();const auto random=game->GetRandom()->GetStateString();
                    GT_ASSERT(Rejects([&]{bm->AbandonSmacBase(GSE_CALL,*base,um->GetUnitDef("choice-colony"),SmacMoraleMode::Mode0);})
                        && fixture.Snapshot()==starving && bm->GetBaseRecordOrder()==order && um->GetUnitRecordOrder()==units
                        && game->GetMap()->GetSmacWorkerKnowledge()->Serialize()==knowledge && Unit::Serialize(um->GetUnit(91)).ToString()==scout
                        && Unit::GetNextId()==next && fixture.GameplayRandom()==pair && game->GetRandom()->GetStateString()==random && fixture.Updates().empty(),
                        "starving successor preflight partially abandoned a Base, manufactured or changed saved authority");
                }
                GT_OK();
            });if(!error.empty()){return error;}
        }return {};
    });
    task->AddTest("SMAC starvation center cleanup removes sea native and joins homes observations and dense links",[](GSE*,context::Context*)->std::string {
        using namespace game::backend::unit;
        const auto source=[](const std::string& filename) {
            const auto text=UpkeepSource(filename);if(filename!="alpha.txt") {return text;}
            std::istringstream input(text);std::string line,section,result;int index=-1;
            while(std::getline(input,line)) {
                if(!line.empty() && line[0]=='#') {section=line;index=-1;}
                else if(section=="#UNITS") {
                    if(index==9 || index==10) {
                        std::istringstream row(line);std::vector<std::string> fields;std::string field;while(std::getline(row,field,',')) {fields.push_back(field);}
                        fields.at(5)=index==9 ? "8":"10";fields.at(6)=index==9 ? "4":"0";
                        line.clear();for(const auto& value:fields) {if(!line.empty()) {line+=',';}line+=value;}
                    }++index;
                }result+=line+'\n';
            }return result;
        };
        for(const bool sea:{false,true}) {
        const std::vector<int> designs=sea ? std::vector<int>{9,8,8,8,8,8,9,10}:std::vector<int>{8,9,10};
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(source,"GAIANS");std::string seeded,saved;
        const auto result=fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();auto* bm=game->GetBM();auto* um=game->GetUM();auto* lost=Populate(GSE_CALL,fixture,true);
            game->GetMap()->GetTile(4,2)->terraforming=0;
            const auto scout=game->MakeSmacProductionItem(lost->m_owner,ProductionItemKind::Unit,2);lost->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
            const auto roles=Roles();auto context=lost->GetSmacResourceContext();context.resource_events=SmacBaseResourceContext::Famine;lost->SetSmacResourceContext(GSE_CALL,context);lost->SetAccumulatedNutrients(0);
            Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(roles.specialists[1]),1));
            auto* peer=new Base(game,702,lost->m_owner,lost->m_faction,game->GetMap()->GetTile(6,2),"Cleanup survivor",{},pops,20,10,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{1,false,0,0,0,false});bm->SpawnBase(GSE_CALL,peer);
            SmacBaseAllocationState allocation{1,{{},1}};allocation.specialists.types[0]=1;peer->PublishSmacAllocation(GSE_CALL,allocation,roles);
            auto home=um->GetUnit(91)->Conventional();home.home=ConventionalUnitState::Home::Base;home.home_base_id=lost->m_id;
            um->SpawnConventionalUnit(GSE_CALL,92,um->GetUnitDef("live-scout"),lost->m_owner,game->GetMap()->GetTile(10,6),home);
            for(const auto design:{8,9,10}) {
                const auto* name=design==8 ? "MindWorms":design==9 ? "IsleOfTheDeep":"LocustsOfChiron";
                um->DefineUnit(new StaticDef(name,um->GetMoraleSet("live-morale"),name,design==8 ? MT_LAND:design==9 ? MT_WATER:MT_AIR,999,new SpriteRender("fixture",0,0,1,1,0,0,1)));
            }
            if(sea) {for(auto* corner:lost->GetTile()->elevation.corners) {*corner=-500;}for(size_t y=0;y<game->GetMap()->GetHeight();++y) {for(size_t x=y&1;x<game->GetMap()->GetWidth();x+=2) {game->GetMap()->GetTile(x,y)->Update();}}fixture.ResourcePolicy(true);fixture.TerritoryTopology(game->GetMap()->PrepareFreshTerritoryTopology().Serialize());}
            // Admit a complete saved native-only stack at the Base, rather than
            // relaxing the live birth factory's occupied-Base admission.
            types::Buffer definitions;um->Serialize(definitions);types::Buffer loaded;loaded.WriteString(definitions.ReadString());
            for(int group=0;group<2;++group) {const auto count=definitions.ReadInt();loaded.WriteInt(count);for(int64_t n=0;n<count;++n) {loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());}}
            GT_ASSERT(definitions.ReadInt()==2,"Cleanup fixture existing Unit count changed");loaded.WriteInt(2+designs.size());loaded.WriteString(definitions.ReadString());loaded.WriteString(definitions.ReadString());
            for(size_t n=0;n<designs.size();++n) {
                auto record=SmacNativeUnitRecord::InitializeWild(2+n,designs[n],2);record.x=2;record.y=2;record.stack_previous=n==0 ? -1:1+n;record.stack_next=n+1==designs.size() ? -1:3+n;record.flags=0x20;
                if(sea) {record.state=0x101;record.waypoint_y[0]=321;if(designs[n]==8) {record.order=1;record.waypoint_x[0]=8;}}
                types::Buffer unit;unit.WriteInt(5);unit.WriteInt(2);unit.WriteInt(100+n);unit.WriteString(designs[n]==8 ? "MindWorms":designs[n]==9 ? "IsleOfTheDeep":"LocustsOfChiron");unit.WriteInt(-1);unit.WriteInt(2);unit.WriteInt(2);unit.WriteString(record.Serialize().ToString());loaded.WriteString(unit.ToString());
            }loaded.WriteInt(100+designs.size());
            auto knowledge=KnowledgeValue(true,true,128,64).CopyData();knowledge.units.push_back({92,0,false});
            for(uint64_t id=100;id<100+designs.size();++id) {knowledge.units.push_back({id,0,false});}
            knowledge.occupancy=map::SmacOccupancyCache::Blank(128,64,true);knowledge.occupancy->FoundBase({2,2},1);knowledge.occupancy->PlaceUnit({2,2},0,true);knowledge.occupancy->FoundBase({6,2},1);knowledge.occupancy->PlaceUnit({6,6},1,true);knowledge.occupancy->PlaceUnit({10,6},1,true);
            const auto candidate_knowledge=Knowledge::Create(knowledge).Serialize();
            auto ordinary=knowledge;ordinary.units.erase(std::remove_if(ordinary.units.begin(),ordinary.units.end(),[](const auto& row){return row.id>=100;}),ordinary.units.end());
            ordinary.occupancy->RemoveUnit({2,2},true);game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(ordinary));
            fixture.ResourceState([](auto& state){state.social.factions.front().current[2]=3;state.native_factory=faction::SmacNativeFactoryState{4,{{17,19}},1000,{}};for(const auto design:{8,9,10}) {state.native_factory->active_designs[design]=17;}});
            fixture.BasePresentationRoles({roles.worker,roles.specialists[0],roles.specialists[1],roles.specialists[2],roles.specialists[3],roles.specialists[4],roles.specialists[5],roles.specialists[6]});
            const auto replace=[](std::string whole,const std::string& before,const std::string& after) {
                types::Buffer old_frame,new_frame;old_frame.WriteString(before);new_frame.WriteString(after);const auto old=old_frame.ToString();const auto at=whole.find(old);
                if(at==std::string::npos || whole.find(old,at+1)!=std::string::npos) {throw std::runtime_error("Expected one owned cleanup snapshot frame");}
                whole.replace(at,old.size(),new_frame.ToString());return whole;
            };
            seeded=replace(replace(fixture.Snapshot(),definitions.ToString(),loaded.ToString()),game->GetMap()->GetSmacWorkerKnowledge()->Serialize(),candidate_knowledge);GT_OK();
        });if(!result.empty()) {return result;}
        ConventionalUnitFixture admitted;admitted.ConfigureProduction(source,"GAIANS");
        const auto executed=admitted.Run([&](GSE_CALLABLE)->std::string {
            admitted.RestoreThroughStart(GSE_CALL,seeded,[](GSE_CALLABLE){});
            admitted.PrepareBaseCallbacks();auto* setup=static_cast<value::Callable*>(admitted.Script(GSE_CALL,"./GLSMAC_data/tests/_base_growth"));setup->Run(GSE_CALL,{admitted.Game()->Wrap(GSE_CALL)});
            auto* game=admitted.Game();auto* bm=game->GetBM();auto* um=game->GetUM();auto* lost=bm->GetBase(701);const auto roles=Roles();
            const auto before=admitted.Snapshot();{auto discarded=um->PrepareBaseCenterRemoval(GSE_CALL,*lost);}
            GT_ASSERT(admitted.Snapshot()==before,"Preparing center cleanup mutated registry or factory");
            auto* isle=static_cast<value::Object*>(um->GetUnit(101)->Wrap(GSE_CALL));auto* tile=lost->GetTile();const auto random=admitted.GameplayRandom();
            admitted.BasePresentationRoles({roles.worker,roles.specialists[0],roles.specialists[1],roles.specialists[2],roles.specialists[3],roles.specialists[4],roles.specialists[5],roles.specialists[6]});
            game->Event(GSE_CALL,"complete_turn",{});admitted.DrainEvents();
            if(sea) {
                GT_ASSERT(!bm->GetBase(701) && !tile->base && um->GetUnit(100) && um->GetUnit(106) && um->GetUnit(107),"Sea Base removal lost carrier or air native");
                for(size_t id=101;id<=105;++id) {const auto& cargo=um->GetUnit(id)->Native();GT_ASSERT(cargo.order==1 && cargo.waypoint_x[0]==8 && cargo.waypoint_y[0]==0 && cargo.state==0x101,"Native mode0 did not retain five cargo associations and order writes");}
                GT_ASSERT(um->GetUnit(100)->Native().state==0x100 && um->GetUnit(106)->Native().state==0x100 && um->GetUnit(107)->Native().state==0x100,"Native mode0 failed whole-stack processed-bit clearing");
            } else {
            GT_ASSERT(!bm->GetBase(701) && !tile->base && !um->GetUnit(101) && !isle->wrapobj && um->GetUnit(100) && um->GetUnit(102),"Old-land removal did not select only the sea native or retire its wrapper");
            GT_ASSERT(um->GetUnit(100)->Native().id==2 && um->GetUnit(100)->Native().stack_next==3 && um->GetUnit(102)->Native().id==3 && um->GetUnit(102)->Native().stack_previous==2,"Center cleanup failed native dense/link compaction");
            }
            GT_ASSERT(um->GetUnit(92)->Conventional().home_base_id==702 && admitted.GameplayRandom()==random,"Center cleanup dropped home repair or changed RNG");
            const auto after=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
            GT_ASSERT(after.occupancy->At({2,2}).presence==2 && std::none_of(after.units.begin(),after.units.end(),[&](const auto& row){return !sea && row.id==101;})
                && std::none_of(after.bases.begin(),after.bases.end(),[](const auto& row){return row.id==701;}),"Territory publication resurrected a removed Unit or Base observation");
            admitted.ResourceState([&](const auto& state){if(state.native_factory->active_designs[9]!=(sea ? 17:16) || state.native_factory->active_designs[8]!=17 || state.native_factory->active_designs[10]!=17 || state.native_factory->active_plan_tally!=(sea ? 1000:999)) {throw std::runtime_error("Native center casualty debit differs");}});
            saved=admitted.Snapshot();GT_OK();
        });if(!executed.empty()) {return executed;}
        ConventionalUnitFixture restored;restored.ConfigureProduction(source,"GAIANS");
        const auto final=restored.Run([&](GSE_CALLABLE)->std::string {
            restored.RestoreThroughStart(GSE_CALL,saved,[](GSE_CALLABLE){});auto* game=restored.Game();
            GT_ASSERT(!game->GetBM()->GetBase(701) && (sea ? game->GetUM()->GetUnit(101)!=nullptr:game->GetUM()->GetUnit(101)==nullptr) && game->GetUM()->GetUnit(100) && game->GetUM()->GetUnit(102)
                && game->GetUM()->GetUnit(92)->Conventional().home_base_id==702,"Actual Start lost joined Base/Unit removal or home repair");GT_OK();
        });if(!final.empty()) {return final;}
        }return "";
    });
    task->AddTest("SMAC starvation removes first middle and last records through the real turn",[](GSE*,context::Context*) -> std::string {
        for (size_t lost_index=0;lost_index<3;++lost_index) {
            ConventionalUnitFixture fixture; fixture.ConfigureProduction(UpkeepSource,"GAIANS");
            std::string saved; size_t destination=0;
            const auto error=fixture.Run([&](GSE_CALLABLE) -> std::string {
                using namespace game::backend::unit;
                auto* game=fixture.Game(); auto* bm=game->GetBM(); auto* um=game->GetUM();
                auto* first=Populate(GSE_CALL,fixture,true);
                game->GetMap()->GetTile(4,2)->terraforming=0;
                const auto roles=Roles(); const auto scout=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,2);
                first->SetProductionState(GSE_CALL,ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}});
                std::array<Base*,3> bases{first,nullptr,nullptr};
                for(size_t i=1;i<3;++i) {
                    Base::pops_t pops; pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(roles.specialists[1]),1));
                    bases[i]=new Base(game,701+i,first->m_owner,first->m_faction,game->GetMap()->GetTile(2+4*i,2),"Survivor "+std::to_string(i),{},pops,20,10,
                        ProductionState{scout,0,scout,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{1,false,0,0,0,false});
                    bm->SpawnBase(GSE_CALL,bases[i]);
                    SmacBaseAllocationState allocation{1,{{},1}}; allocation.specialists.types[0]=1;
                    bases[i]->PublishSmacAllocation(GSE_CALL,allocation,roles);
                }
                auto* lost=bases[lost_index]; const auto lost_id=lost->m_id; auto* lost_tile=lost->GetTile();
                auto* lost_wrapper=static_cast<value::Object*>(lost->Wrap(GSE_CALL));
                auto* lost_pop=static_cast<value::Object*>(lost->m_pops.begin()->second.Wrap(GSE_CALL));
                auto context=lost->GetSmacResourceContext(); context.resource_events=SmacBaseResourceContext::Famine;
                lost->SetSmacResourceContext(GSE_CALL,context); lost->SetAccumulatedNutrients(0);
                auto data=KnowledgeValue(true,true,128,64).CopyData();
                for(auto& row:data.tiles) {row.explored_by=2;row.claimant=1;}
                for(size_t i=0;i<3;++i) {
                    auto record=um->GetUnit(91)->Conventional(); record.home=ConventionalUnitState::Home::Base;record.home_base_id=lost_id;
                    um->SpawnConventionalUnit(GSE_CALL,92+i,um->GetUnitDef("live-scout"),lost->m_owner,game->GetMap()->GetTile(i*2,8),record);
                    data.units.push_back({92+i,0,false});
                }
                fixture.ResourceState([](auto& state){state.social.factions.front().current[2]=3;});
                game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));
                const auto native=PublishUnrelatedNative(GSE_CALL,fixture);
                fixture.BasePresentationRoles({roles.worker,roles.specialists[0],roles.specialists[1],roles.specialists[2],roles.specialists[3],roles.specialists[4],roles.specialists[5],roles.specialists[6]});
                const auto before=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                const auto pair=fixture.GameplayRandom(); const auto order=bm->GetBaseRecordOrder();
                fixture.Updates();
                destination=bases[lost_index==1 ? 2 : 1]->m_id;
                game->Event(GSE_CALL,"complete_turn",{}); fixture.DrainEvents();
                GT_ASSERT(game->GetTurnId()==2 && !bm->GetBase(lost_id) && !lost_tile->base && !lost_wrapper->wrapobj && !lost_pop->wrapobj,"actual turn did not retire starving Base and Pop wrappers");
                const auto& updates=fixture.Updates(); size_t despawns=0;
                for(const auto& update:updates) {if(update.type==game::FrontendRequest::FR_BASE_DESPAWN && update.data.base_despawn.base_id==lost_id) {++despawns;}}
                GT_ASSERT(despawns==1,"starvation did not publish exactly one Base despawn");
                auto remaining=order; remaining.erase(remaining.begin()+lost_index);
                GT_ASSERT(bm->GetBaseRecordOrder()==remaining,"starvation changed survivor Base order or stable IDs");
                for(const auto id:remaining) {
                    const auto* survivor=bm->GetBase(id);
                    GT_ASSERT(survivor->GetProductionState()->minerals==1 && survivor->GetProductionState()->baseline_minerals==1,
                        "controller skipped or double-posted a surviving Base");
                }
                for(size_t id=92;id<95;++id) {GT_ASSERT(um->GetUnit(id)->Conventional().home_base_id==destination,"home repair used Unit position or wrong Base tie order");}
                GT_ASSERT(!um->GetUnit(91)->Conventional().home_base_id && um->GetUnitRecordOrder()==std::vector<size_t>({91,92,93,94,native.id}),"starvation changed unhomed Unit or Unit roster");
                GT_ASSERT(native.Matches(fixture),"Starvation changed unrelated native body or factory");
                const auto after=game->GetMap()->GetSmacWorkerKnowledge()->CopyData();
                GT_ASSERT(before.units==after.units && before.remembered==after.remembered && before.observers==after.observers,"starvation changed observation or remembered authority");
                for(size_t i=0;i<before.tiles.size();++i) {GT_ASSERT(before.tiles[i].explored_by==after.tiles[i].explored_by,"starvation replayed exploration");}
                GT_ASSERT(fixture.GameplayRandom()==pair,"starvation changed paired RNG");
                GT_ASSERT(game->GetCanonicalMineralSupport(*bm->GetBase(destination))==0,"repaired homes did not feed actual support");
                saved=fixture.Snapshot(); GT_OK();
            });
            if(!error.empty()) {return "starvation position "+std::to_string(lost_index)+": "+error;}
            ConventionalUnitFixture restored; restored.ConfigureProduction(UpkeepSource,"GAIANS");
            const auto continuation=restored.Run([&](GSE_CALLABLE) -> std::string {
                restored.Restore(GSE_CALL,saved,true); auto* game=restored.Game();
                for(size_t id=92;id<95;++id) {GT_ASSERT(game->GetUM()->GetUnit(id)->Conventional().home_base_id==destination,"restore lost stable repaired home");}
                GT_ASSERT(game->GetCanonicalMineralSupport(*game->GetBM()->GetBase(destination))==0,"restored home support differs"); GT_OK();
            });
            if(!continuation.empty()) {return continuation;}
        }
        return {};
    });
	task->AddTest( "SMAC live upkeep rejects missing restart factory authority without partial production", []( GSE*, context::Context* ) -> std::string {
		const auto source = []( const std::string& filename ) { auto text = UpkeepSource( filename );
			if ( filename == "alpha.txt" ) { const auto at = text.find( "#RULES\n" ); const auto end = text.find( '\n', at + 7 ); const auto next = end + 1; text.replace( next, text.find( ',', next ) - next, "3" ); } return text; };
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, true );
			base->m_owner->GetPlayer()->SetDifficultyLevel("Librarian"); // make search reach the deliberately missing factory
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
			base->SetProductionState( GSE_CALL, ProductionState{ scout, 999, scout, 0, std::vector< ProductionItem >{} } ); base->SetAccumulatedNutrients( -1 );
			const auto before = fixture.Snapshot(); const auto id = Unit::GetNextId();
			GT_ASSERT( Rejects( [ & ] { base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t ) { return game->GetUM()->GetUnitDef( "live-scout" ); }, SmacMoraleMode::Mode0 ); } )
				&& fixture.Snapshot() == before && Unit::GetNextId() == id, "pending starvation removal partially published upkeep" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live upkeep shortage uses prior paid flags and restores newborn continuation", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( UpkeepSource, "GAIANS" );
		std::string snapshot; size_t newborn_id = 0, next_loss = 0;
		const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); game->AdvanceTurn( 2 );
			auto* base = Populate( GSE_CALL, fixture, true, true ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 ); const auto cost = game->GetSmacProductionCost( base->m_owner, scout );
			const auto resolver = [ & ]( int32_t ) { return game->GetUM()->GetUnitDef( "live-scout" ); };
			for ( int i = 0; i < 3; ++i ) {
				base->SetProductionState( GSE_CALL, ProductionState{ scout, cost, scout, 0, std::vector< ProductionItem >{} } );
				GT_ASSERT( base->ProcessSmacProduction( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 ).completed_unit, "homed deficit setup did not complete" );
			}
			GT_ASSERT( base->PrepareSmacResourceCalculation( SmacBaseResourcePhase::Upkeep ).minerals.surplus == -1, "actual home support did not produce deficit" );
			base->SetProductionState( GSE_CALL, ProductionState{ scout, cost, scout, 0, std::vector< ProductionItem >{} } );
			const auto order = game->GetUM()->GetUnitRecordOrder(); next_loss = order.at( 3 );
			const auto pair = fixture.GameplayRandom();
			const auto result = base->ProcessSmacUpkeep( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( result.completed_unit && result.removed_units == std::vector<size_t>{ order.at( 2 ) }
				&& game->GetUM()->GetUnit( 91 ) && game->GetUM()->GetUnit( *result.completed_unit ), "shortage selected free far unit or unmarked newborn" );
			newborn_id = *result.completed_unit;
			GT_ASSERT( result.first_calculation.minerals.surplus == -1 && result.final_calculation->minerals.surplus == -1
				&& base->GetProductionState()->minerals == 0 && base->GetProductionState()->baseline_minerals == 0
				&& base->GetAccumulatedNutrients() == 13 && fixture.GameplayRandom() == pair, "shortage reposted stores or recounted newborn before selection" );
			snapshot = fixture.Snapshot(); GT_OK();
		} );
		if ( !error.empty() ) { return error; }
		ConventionalUnitFixture restored; restored.ConfigureProduction( UpkeepSource, "GAIANS" );
		return restored.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			restored.Restore( GSE_CALL, snapshot, true ); auto* game = restored.Game(); auto* base = game->GetBM()->GetBase( 701 );
			const auto result = base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t ) { return game->GetUM()->GetUnitDef( "live-scout" ); }, unit::SmacMoraleMode::Mode0 );
			GT_ASSERT( !result.completed_unit && result.removed_units == std::vector<size_t>{ next_loss }
				&& game->GetUM()->GetUnit( newborn_id ) && result.final_calculation->minerals.surplus == 0
				&& base->GetProductionState()->minerals == 0 && base->GetAccumulatedNutrients() == 13, "restored shortage lost record order or posted recovered surplus" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live upkeep shortage preserves paid roster ties wrapping and current SUPPORT", []( GSE*, context::Context* ) -> std::string {
		for ( const int rating : { 0, -4 } ) {
			ConventionalUnitFixture fixture; fixture.ConfigureProduction( UpkeepSource, "GAIANS" );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				using namespace game::backend::unit;
				auto* game = fixture.Game(); game->AdvanceTurn( 2 ); auto* base = Populate( GSE_CALL, fixture, true, true );
				auto* um = game->GetUM(); auto data = KnowledgeValue( true, true, 128, 64 ).CopyData();
				const auto record = um->GetUnit( 91 )->Conventional();
				for ( const auto entry : { std::pair<size_t,size_t>{50,64}, {70,12}, {60,120}, {40,6} } ) {
					um->SpawnConventionalUnit( GSE_CALL, entry.first, um->GetUnitDef( "live-scout" ), base->m_owner, game->GetMap()->GetTile( entry.second, 2 ), record );
					data.units.push_back( { entry.first, 0, false } );
				}
				fixture.ResourceState( [ & ]( auto& state ) { auto& social = state.social.factions.front(); social.current[2] = rating; social.retained->at(2) = rating == 0 ? -4 : 0; } );
				game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( data ) );
				const auto stock = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Stockpile, 38 );
				base->SetProductionState( GSE_CALL, ProductionState{ stock, 10, stock, 10, std::vector<ProductionItem>{} } );
				const auto before = fixture.GameplayRandom();
				const auto result = base->ProcessSmacUpkeep( GSE_CALL, Roles(), {}, SmacMoraleMode::Mode0 );
				const std::vector<size_t> expected = rating == 0 ? std::vector<size_t>{70,60} : std::vector<size_t>{50,70,60,91,40};
				GT_ASSERT( result.removed_units == expected && result.final_calculation->minerals.demand == ( rating == 0 ? 1 : 0 ), "shortage changed wrapped distance, record ties, free allowance or current two-mineral cost" );
				GT_ASSERT( base->GetProductionState()->minerals == 10 && base->GetProductionState()->baseline_minerals == 10
					&& base->GetAccumulatedNutrients() == 13 && fixture.GameplayRandom() == before, "shortage changed stores/baselines/RNG" ); GT_OK();
			} );
			if ( !error.empty() ) { return error; }
		}
		return {};
	} );
	task->AddTest( "SMAC live upkeep growth recount can remove the staged newborn", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( UpkeepSource, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); game->AdvanceTurn( 2 ); auto* base = Populate( GSE_CALL, fixture, true, true );
			game->GetMap()->GetTile( 4, 2 )->terraforming = 0;
			fixture.ResourceState( []( auto& state ) { state.social.factions.front().current[2] = -4; } );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
			base->SetProductionState( GSE_CALL, ProductionState{ scout, game->GetSmacProductionCost( base->m_owner, scout ), scout, 0, std::vector<ProductionItem>{} } );
			base->SetAccumulatedNutrients( 20 );
			const auto result = base->ProcessSmacUpkeep( GSE_CALL, Roles(), [ & ]( int32_t ) { return game->GetUM()->GetUnitDef( "live-scout" ); }, SmacMoraleMode::Mode0 );
			GT_ASSERT( result.completed_unit && result.removed_units == std::vector<size_t>({91,*result.completed_unit})
				&& game->GetUM()->GetUnits().empty() && game->GetUM()->GetUnitRecordOrder().empty()
				&& game->GetMap()->GetSmacWorkerKnowledge()->CopyData().units.empty(), "growth did not count and retire the newborn with its older paid home unit" );
			GT_ASSERT( base->m_pops.size() == 2 && result.final_calculation->minerals.demand == 0
				&& base->GetProductionState()->minerals == 0 && base->GetProductionState()->baseline_minerals == 0, "growth shortage reposted minerals or changed population" );
			GT_ASSERT( !fixture.Snapshot().empty(), "growth shortage left unsavable authority" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live production commits actual Scout Former queue home and second-pass support", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false ); auto* um = game->GetUM();
			um->DefineUnit( new StaticDef( "live-former", um->GetMoraleSet( "live-morale" ), "Former port marker", MT_LAND, 0, new FormerMarkerRender() ) );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( false, true, 128, 64 ) );
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
			const auto former = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 1 );
			const auto resolver = [ & ]( int32_t id ) { return um->GetUnitDef( id == 1 ? "live-former" : "live-scout" ); };
			const auto pair = fixture.GameplayRandom(); const auto jsf = game->GetRandom()->GetStateString(); auto* pop = &base->m_pops.at( 19 );
			base->SetProductionState( GSE_CALL, ProductionState{ scout, game->GetSmacProductionCost( base->m_owner, scout ) - 3, scout, 71, std::vector< ProductionItem >{ former } } );
			const auto first = base->ProcessSmacProduction( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( first.completed_unit && first.queue_advanced && first.first_calculation.minerals.surplus == 3, "actual first pass did not repair and complete at equality" );
			auto* newborn = um->GetUnit( *first.completed_unit );
			GT_ASSERT( newborn->Conventional().design_id == 2 && newborn->Conventional().home_base_id == base->m_id && newborn->GetTile() == base->GetTile()
				&& newborn->Conventional().spent == 0 && newborn->Conventional().stored_morale == 1, "actual homed Scout differs" );
			GT_ASSERT( base->GetProductionState()->current_item == former && base->GetProductionState()->baseline_item == former
				&& base->GetProductionState()->queue->empty() && base->GetProductionState()->minerals == 0 && base->GetProductionState()->baseline_minerals == 0
				&& &base->m_pops.at( 19 ) == pop && base->GetSmacAllocation()->selected == 5, "queue baseline Pop transaction differs" );
			for ( int i = 0; i < 2; ++i ) {
				auto state = *base->GetProductionState(); state.minerals = game->GetSmacProductionCost( base->m_owner, state.current_item ) - 3; base->SetProductionState( GSE_CALL, state );
				const auto made = base->ProcessSmacProduction( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
				GT_ASSERT( made.completed_unit && um->GetUnit( *made.completed_unit )->Conventional().design_id == 1, "empty queue did not repeat Former" );
			}
			const auto second = base->PrepareSmacResourceCalculation( SmacBaseResourcePhase::Upkeep );
			GT_ASSERT( second.minerals.demand == 1 && second.minerals.surplus == 2 && base->GetProductionState()->minerals == 0, "third home support or read-only second pass differs" );
			const auto following = base->ProcessSmacProduction( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( !following.completed_unit && base->GetProductionState()->minerals == 2 && base->GetProductionState()->baseline_minerals == 2
				&& base->GetAccumulatedNutrients() == 13 && fixture.GameplayRandom() == pair && game->GetRandom()->GetStateString() == jsf, "following production changed food/RNG or completed twice" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live production late rejection preserves prepared Unit Base Pops and queue", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, true ); auto* um = game->GetUM(); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 ); const auto facility = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Facility, 2 );
			base->SetProductionState( GSE_CALL, ProductionState{ scout, game->GetSmacProductionCost( base->m_owner, scout ), scout, 17, std::vector< ProductionItem >{ facility } } );
			fixture.Updates(); const auto before = fixture.Snapshot(); const auto id = Unit::GetNextId(); auto* pop = &base->m_pops.at( 19 );
			const auto resolver = [ & ]( int32_t ) { return um->GetUnitDef( "live-scout" ); };
			GT_ASSERT( Rejects( [ & ] { base->ProcessSmacProduction( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 ); } ) && fixture.Snapshot() == before
				&& Unit::GetNextId() == id && &base->m_pops.at( 19 ) == pop && um->GetUnits().size() == 1, "late queue rejection published partial production" );
			auto state = *base->GetProductionState(); state.queue->clear(); base->SetProductionState( GSE_CALL, state );
			const auto made = base->ProcessSmacProduction( GSE_CALL, Roles(), resolver, SmacMoraleMode::Mode0 );
			GT_ASSERT( made.completed_unit && base->GetSmacAllocation()->specialists.total_count == 1 && base->GetSmacAllocation()->specialists.types[ 0 ] == 1, "valid Doctor replaced during completion" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker upkeep admits second human participant but not manual control", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureTwoResourceParticipants( Source );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false, false, 6, 2 );
			auto data = KnowledgeValue().CopyData(); data.observers[ 2 ] = Knowledge::Observer{ false, {} };
			for ( auto& tile : data.tiles ) { if ( tile.claimant == 1 ) { tile.claimant = 2; } if ( tile.explored_by == 2 ) { tile.explored_by = 4; } }
			game->PublishSmacWorkerKnowledge( GSE_CALL, Knowledge::Create( data ) );
			const auto target = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 );
			GT_ASSERT( game->GetSmacProductionCost( base->m_owner, target ) == 98, "second participant did not use human Tiny factor123" );
			base->RecalculateSmacWorkers( GSE_CALL, Roles(), SmacBaseResourcePhase::Upkeep );
			GT_ASSERT( base->GetSmacAllocation()->selected == 5 && game->GetBM()->GetWorkingPop( { 4, 2 } ) == &base->m_pops.at( 19 ), "second participant upkeep did not repair actual workers" );
			const auto before = Base::Serialize( base ).ToString();
			GT_ASSERT( Rejects( [ & ] { base->CommandSmacWorkerTile( GSE_CALL, game->GetMap()->GetTile( 4, 2 ) ); } )
				&& Base::Serialize( base ).ToString() == before, "human-roster calculation granted local manual control" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker recalculation repairs actual unavailable assignments", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue() );
			auto* pop = &base->m_pops.at( 19 ); auto* wrapper = static_cast< value::Object* >( pop->Wrap( GSE_CALL ) );
			const auto prior = *base->GetSmacAllocation(); const auto knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
			const auto rng = game->GetRandom()->GetStateString(); const auto pair = fixture.GameplayRandom(); const auto production = base->GetProductionState();
			RecalculateViaWrapper( GSE_CALL, base );
			const auto actual = *base->GetSmacAllocation();
			GT_ASSERT( actual.selected == 5 && actual.specialists.total_count == 0 && actual.specialists.types == prior.specialists.types, "actual unavailable worker was not repaired onto the explored forest" );
			GT_ASSERT( &base->m_pops.at( 19 ) == pop && Pop::Unwrap( wrapper ) == pop && pop->m_variant == 1
				&& !game->GetBM()->GetWorkingPop( { 3, 1 } ) && game->GetBM()->GetWorkingPop( { 4, 2 } ) == pop, "live publication lost stable Pop or reverse membership" );
			auto* intake = static_cast< value::Object* >( Call( GSE_CALL, static_cast< value::Object* >( base->Wrap( GSE_CALL ) ), "get_intake" ) );
			GT_ASSERT( static_cast< value::Int* >( intake->Get( "NUTRIENTS" ) )->value == 3
				&& static_cast< value::Int* >( intake->Get( "MINERALS" ) )->value == 3, "actual intake callback did not consume the repaired live worker" );
			GT_ASSERT( game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge && game->GetRandom()->GetStateString() == rng
				&& fixture.GameplayRandom() == pair && base->GetProductionState() == production && base->GetAccumulatedNutrients() == 13, "allocation changed unrelated authority" );
			bool refresh = false; for ( const auto& update : fixture.Updates() ) { if ( update.type == game::FrontendRequest::FR_BASE_UPDATE && update.data.base_update.base_id == 701 ) { refresh = true; } }
			GT_ASSERT( refresh, "repaired live allocation did not enqueue player presentation" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker recalculation retains valid Doctors and typed production", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, true ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
			const auto allocation = base->GetSmacAllocation(); auto* pop = &base->m_pops.at( 19 );
			for ( const auto kind : { ProductionItemKind::Unit, ProductionItemKind::Facility, ProductionItemKind::Stockpile } ) {
				base->SetProductionState( GSE_CALL, Production( kind ) );
				const auto without_cache = [&] {
					const auto saved = Base::Serialize( base ).ToString(); types::Buffer cache;
					cache.WriteString( SerializeSmacBaseUpkeepCache( base->GetSmacUpkeepCache() ).ToString() );
					return saved.substr( 0, saved.size() - cache.lenw );
				};
				const auto before = without_cache();
				base->RecalculateSmacWorkers( GSE_CALL, Roles() );
				GT_ASSERT( without_cache() == before && base->GetSmacUpkeepCache() && base->GetSmacAllocation() == allocation && &base->m_pops.at( 19 ) == pop
					&& !pop->GetWorkedTileCoords() && pop->m_def->m_smac_specialist_id == 1, "ordinary recalculation optimized a complete valid Doctor allocation" );
				const auto calculated = Base::Serialize( base ).ToString();
				base->RecalculateSmacWorkers( GSE_CALL, Roles(), SmacBaseResourcePhase::Upkeep );
				GT_ASSERT( Base::Serialize( base ).ToString() == calculated, "unmanaged upkeep changed a complete valid Doctor allocation" );
				GT_ASSERT( Rejects( [ & ] { base->RecalculateSmacWorkers( GSE_CALL, Roles(), static_cast< SmacBaseResourcePhase >( 99 ) ); } )
					&& Base::Serialize( base ).ToString() == calculated, "unknown allocation phase changed Base" );
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker recalculation rejects before atomic publication", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue() );
			for ( int scenario = 1; scenario < 6; ++scenario ) {
				auto roles = Roles(); auto context = base->GetSmacResourceContext(); auto* candidate = game->GetMap()->GetTile( 4, 2 );
				if ( scenario == 1 ) { base->SetProductionState( GSE_CALL, std::nullopt ); }
				if ( scenario == 2 ) { base->SetProductionState( GSE_CALL, Production( ProductionItemKind::Project ) ); }
				if ( scenario == 3 ) { base->SetProductionState( GSE_CALL, Production( ProductionItemKind::Orbital ) ); }
				if ( scenario == 4 ) { candidate->terraforming |= TERRAFORMING_MIRROR; }
				if ( scenario == 5 ) { roles.specialists[ 6 ] = roles.specialists[ 0 ]; } // late publication preflight
				const auto before = fixture.Snapshot(); const auto requests = fixture.QueuedFrontendRequests().size();
				GT_ASSERT( Rejects( [ & ] { base->RecalculateSmacWorkers( GSE_CALL, roles ); } ), "unsupported live allocation context was accepted" );
				GT_ASSERT( fixture.Snapshot() == before && fixture.QueuedFrontendRequests().size() == requests
					&& game->GetBM()->GetWorkingPop( { 3, 1 } ) == &base->m_pops.at( 19 ) && !game->GetBM()->GetWorkingPop( { 4, 2 } ), "rejection published partial worker/world state" );
				context.governor_options = 0x40; base->SetSmacResourceContext( GSE_CALL, context ); base->SetProductionState( GSE_CALL, Production() ); candidate->terraforming = TERRAFORMING_FOREST;
			}
			GT_OK();
		} );
	} );
	task->AddTest( "SMAC live worker recalculation resumes actual saved world authority", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture source; source.ConfigureResources( Source, "GAIANS" );
		std::string saved, knowledge, result; auto pair = source.GameplayRandom();
		const auto error = source.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = source.Game(); auto* base = Populate( GSE_CALL, source, false ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue() );
			saved = source.Snapshot(); knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize(); pair = source.GameplayRandom();
			base->RecalculateSmacWorkers( GSE_CALL, Roles() ); result = Base::Serialize( base ).ToString(); GT_OK();
		} ); GT_ASSERT( error.empty(), "source continuation failed: " + error );
		// A separate actual decoder destination: replacing a live known world
		// requires its normal owner retirement, not the fixture's UM-only clear.
		ConventionalUnitFixture destination; destination.ConfigureResources( Source, "GAIANS" );
		return destination.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			destination.Restore( GSE_CALL, saved, true ); auto* game = destination.Game(); auto* base = game->GetBM()->GetBase( 701 );
			GT_ASSERT( base->GetSmacAllocation()->selected == 3 && game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge, "restore replayed allocation or knowledge producers" );
			RecalculateViaWrapper( GSE_CALL, base );
			GT_ASSERT( Base::Serialize( base ).ToString() == result && destination.GameplayRandom() == pair, "restored next live allocation differs" ); GT_OK();
		} );
	} );

	task->AddTest( "SMAC manual worker events publish Pop index and presentation atomically", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false );
			auto context = base->GetSmacResourceContext(); context.governor_options = 0x1000040; base->SetSmacResourceContext( GSE_CALL, context );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
			auto* pop = &base->m_pops.at( 19 ); auto* wrapper = pop->Wrap( GSE_CALL );
			const auto padding = base->GetSmacAllocation()->specialists.types; const auto pair = fixture.GameplayRandom();
			const auto rng = game->GetRandom()->GetStateString(); const auto knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
			auto* initialize = static_cast< value::Callable* >( fixture.Script( GSE_CALL, "./GLSMAC_data/tests/_canonical_worker_click" ) );
			auto* click = static_cast< value::Callable* >( initialize->Run( GSE_CALL, { game->Wrap( GSE_CALL ), base->Wrap( GSE_CALL ) } ) );
			click->Run( GSE_CALL, { VALUE( value::Int,, 250 ), VALUE( value::Int,, 75 ) } ); fixture.DrainEvents();
			GT_ASSERT( base->GetSmacAllocation()->selected == 1 && base->GetSmacAllocation()->specialists.total_count == 1
				&& pop->m_def->m_smac_specialist_id == 1 && !game->GetBM()->GetWorkingPop( { 3, 1 } ), "event did not create and retain the actual automatic Doctor" );
			GT_ASSERT( base->GetSmacResourceContext().governor_options == 0x1000000, "manual command cleared more than citizen management" );
			click->Run( GSE_CALL, { VALUE( value::Int,, 300 ), VALUE( value::Int,, 100 ) } ); fixture.DrainEvents();
			GT_ASSERT( base->GetSmacAllocation()->selected == 5 && base->GetSmacAllocation()->specialists.total_count == 0
				&& game->GetBM()->GetWorkingPop( { 4, 2 } ) == pop && &base->m_pops.at( 19 ) == pop && Pop::Unwrap( wrapper ) == pop
				&& base->GetSmacAllocation()->specialists.types == padding, "work event lost selection, stable Pop or unused specialist padding" );
			GT_ASSERT( fixture.GameplayRandom() == pair && game->GetRandom()->GetStateString() == rng
				&& game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge, "manual event changed RNG or knowledge" );
			bool refresh = false; for ( const auto& update : fixture.Updates() ) { if ( update.type == game::FrontendRequest::FR_BASE_UPDATE && update.data.base_update.base_id == 701 ) { refresh = true; } }
			GT_ASSERT( refresh, "manual command did not publish Base presentation" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC manual worker click replacement center and unavailable semantics", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
			const auto click = [ & ]( size_t x, size_t y ) {
				game->Event( GSE_CALL, "work_base_tile", { { "base", base->Wrap( GSE_CALL ) }, { "tile", game->GetMap()->GetTile( x, y )->Wrap( GSE_CALL ) } } ); fixture.DrainEvents();
			};
			click( 4, 2 ); GT_ASSERT( base->GetSmacAllocation()->selected == 5, "manual replacement did not replace an existing complete worker allocation" );
			click( 3, 1 ); GT_ASSERT( base->GetSmacAllocation()->selected == 3, "manual replacement incorrectly optimized back to the better forest" );
			click( 2, 2 ); GT_ASSERT( base->GetSmacAllocation()->selected == 5, "center click did not clear selection and perform new allocation" );
			auto context = base->GetSmacResourceContext(); context.governor_options |= SmacBaseResourceContext::MANAGE_CITIZENS; base->SetSmacResourceContext( GSE_CALL, context );
			const auto allocation = base->GetSmacAllocation(); click( 4, 0 );
			GT_ASSERT( base->GetSmacAllocation() == allocation && !base->GetSmacResourceContext().ManageCitizens(), "unavailable click changed allocation or failed to clear citizen management" );
			game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue() );
			auto stale = *base->GetSmacAllocation(); stale.selected = 3; base->PublishSmacAllocation( GSE_CALL, stale, Roles() );
			click( 4, 2 );
			GT_ASSERT( base->GetSmacAllocation()->selected == 1 && base->GetSmacAllocation()->specialists.total_count == 1
				&& base->m_pops.at( 19 ).m_def->m_smac_specialist_id == 1, "click did not inspect selection after preliminary repair" ); GT_OK();
		} );
	} );
	task->AddTest( "SMAC manual worker events consume actual homed support and frozen priorities", []( GSE*, context::Context* ) -> std::string {
		for ( const bool homed : { false, true } ) {
			ConventionalUnitFixture fixture; fixture.ConfigureResources( []( const std::string& filename ) {
				auto source = Source( filename );
				if ( filename != "alpha.txt" ) {
					const std::string original = "Leader,F,-1,0,0,1,1,";
					source.replace( source.find( original ), original.size(), "Leader,F,500,0,0,0,0," );
				}
				return source;
			}, "GAIANS" );
			const auto error = fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
				auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, false, homed );
				auto allocation = *base->GetSmacAllocation(); allocation.selected = 5;
				base->PublishSmacAllocation( GSE_CALL, allocation, Roles() );
				fixture.ResourceState( []( auto& state ) { state.social.factions.front().current.at( 2 ) = -4; } );
				auto* unit = game->GetUM()->GetUnit( 91 );
				auto context = base->GetSmacResourceContext(); context.drone_riots = true; base->SetSmacResourceContext( GSE_CALL, context );
				game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
				GT_ASSERT( game->GetCanonicalMineralSupport( *base ) == ( homed ? 2 : 0 ), "actual home did not produce measured support" );
				const auto record = unit->Conventional(); const auto pair = fixture.GameplayRandom(); const auto rng = game->GetRandom()->GetStateString();
				const auto knowledge = game->GetMap()->GetSmacWorkerKnowledge()->Serialize();
				RecalculateViaWrapper( GSE_CALL, base );
				game->Event( GSE_CALL, "work_base_tile", { { "base", base->Wrap( GSE_CALL ) }, { "tile", game->GetMap()->GetTile( 3, 1 )->Wrap( GSE_CALL ) } } );
				fixture.DrainEvents();
				// Forest M=2, total M=3, F=500: mineral score is 336 without
				// support, 1008 with support2. Only the former is below999.
				GT_ASSERT( base->GetSmacAllocation()->selected == ( homed ? 5u : 3u ), "manual event ignored home support or current frozen aggression" );
				GT_ASSERT( base->GetSmacResourceContext().drone_riots && !base->GetSmacResourceContext().ManageCitizens()
					&& unit->Conventional() == record && fixture.GameplayRandom() == pair && game->GetRandom()->GetStateString() == rng
					&& game->GetMap()->GetSmacWorkerKnowledge()->Serialize() == knowledge, "worker publication changed home, riots, RNG or knowledge" ); GT_OK();
			} );
			if ( !error.empty() ) { return error; }
		}
		GT_OK();
	} );

	task->AddTest( "SMAC manual worker event failures preserve authority and broad fences", []( GSE*, context::Context* ) -> std::string {
		ConventionalUnitFixture fixture; fixture.ConfigureResources( Source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, true ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true ) );
			const auto before = fixture.Snapshot();
			GT_ASSERT( Rejects( [ & ] { game->Event( GSE_CALL, "unrelated", {} ); } ), "worker command opened arbitrary event dispatch" );
			GT_ASSERT( Rejects( [ & ] { game->Event( GSE_CALL, "work_base_tile", { { "base", base->Wrap( GSE_CALL ) }, { "tile", game->GetMap()->GetTile( 4, 2 )->Wrap( GSE_CALL ) }, { "pop", base->m_pops.at( 19 ).Wrap( GSE_CALL ) } } ); } ), "canonical click accepted legacy Pop override" );
			GT_ASSERT( fixture.Snapshot() == before, "early command rejection changed state" );
			auto* target = game->GetMap()->GetTile( 4, 2 ); target->terraforming |= TERRAFORMING_MIRROR;
			const auto rejected = fixture.Snapshot(); const auto requests = fixture.QueuedFrontendRequests().size();
			game->Event( GSE_CALL, "work_base_tile", { { "base", base->Wrap( GSE_CALL ) }, { "tile", target->Wrap( GSE_CALL ) } } );
			GT_ASSERT( Rejects( [ & ] { fixture.DrainEvents(); } ), "unsupported final tile query did not reject" );
			GT_ASSERT( fixture.Snapshot() == rejected && fixture.QueuedFrontendRequests().size() == requests && base->GetSmacResourceContext().ManageCitizens()
				&& !game->GetBM()->GetWorkingPop( { 4, 2 } ), "late command failure published allocation, context or reverse index" ); GT_OK();
		} );
	} );
}
}
