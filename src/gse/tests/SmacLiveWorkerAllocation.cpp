#include "SmacLiveWorkerAllocation.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/Player.h"
#include "game/backend/Random.h"
#include "game/backend/base/Base.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/SmacBaseUpkeepCache.h"
#include "game/backend/base/PopDef.h"
#include "game/backend/base/FacilityDef.h"
#include "game/backend/faction/Faction.h"
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
#include "game/FrontendRequest.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Callable.h"
#include "gse/value/Int.h"
#include "gse/value/String.h"
#include "gse/value/Undefined.h"
#include "types/Buffer.h"
#include <sstream>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend;
using namespace game::backend::base;
using namespace game::backend::map::tile;
using Knowledge = map::SmacWorkerKnowledge;

bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::exception& ) { return true; } return false; }

// Independent typed fixture: deliberately retained history, not a query of any
// current world. Marking order is unrelated to numeric Unit identity.
struct UpkeepCacheWire {
    int64_t version = 1;
    bool present = true;
    std::array<int64_t,13> calculation{4,3,2, 4,2,2,19,19, 2,3,2,1,1};
    std::array<int64_t,5> support{1,2,2,1,2};
    int64_t count = 4;
    std::vector<std::pair<int64_t,int64_t>> units{{93,0},{7,3},{51,1},{4,2}};
    types::Buffer Encode() const {
        types::Buffer out; out.WriteInt(version); out.WriteBool(present);
        if (present) {
            for (const auto value : calculation) { out.WriteInt(value); }
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
	auto source = smac_startup_fixture::Source( filename );
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
ProductionState Production( ProductionItemKind kind = ProductionItemKind::Unit ) {
	const ProductionItem item{ kind, kind == ProductionItemKind::Stockpile ? "" : "opaque-target" };
	return { item, 17, { ProductionItemKind::Stockpile, "" }, 5 };
}
Base* Populate( GSE_CALLABLE, ConventionalUnitFixture& fixture, bool doctor, bool homed = false, size_t owner_slot = 0, int32_t simulation_id = 1, size_t population = 1 ) {
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
	auto* base = new Base( game, 701, owner, owner->GetPlayer()->GetFaction(), game->GetMap()->GetTile( 2, 2 ), "Live allocation", {},
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

void AddSmacLiveWorkerAllocationTests( task::gsetests::GSETests* task ) {

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
        wire.version = 2;
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
        std::string saved, new_record, old_record; size_t founded_id=0, pod_id=0;
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
            const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
            base->SetProductionState(GSE_CALL,ProductionState{target,game->GetSmacProductionCost(base->m_owner,target)-5,target,0,std::vector<ProductionItem>{}});
            base->SetAccumulatedNutrients(0);
            pod_id=*base->ProcessSmacUpkeep(GSE_CALL,Roles(),[&](int32_t){return um->GetUnitDef("live-colony");},SmacMoraleMode::Mode0).completed_unit;
            auto* pod=um->GetUnit(pod_id); fixture.MoveEvent(GSE_CALL,pod,game->GetMap()->GetTile(4,2));
            um->ResetConventionalSpent(GSE_CALL,pod); fixture.MoveEvent(GSE_CALL,pod,game->GetMap()->GetTile(6,2)); um->ResetConventionalSpent(GSE_CALL,pod);
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
            GT_ASSERT(founded && game->GetBM()->GetBases().size()==2 && !um->GetUnit(pod_id)
                && um->GetUnitRecordOrder()==std::vector<size_t>{91},"actual founding event did not consume Pod or retain survivor order");
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
        return restored.Run([&](GSE_CALLABLE) -> std::string {
            restored.Restore(GSE_CALL,saved,true); auto* game=restored.Game(); auto* founded=game->GetBM()->GetBase(founded_id);
            GT_ASSERT(founded && Base::Serialize(founded).ToString()==new_record && Base::Serialize(game->GetBM()->GetBase(701)).ToString()==old_record
                && !game->GetUM()->GetUnit(pod_id) && game->GetUM()->GetUnitRecordOrder()==std::vector<size_t>{91},"restore replayed founding or resurrected Pod");
            (void)game->GetSmacWorkerAvailability(*founded); GT_OK();
        });
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
            for(int failure=0;failure<5;++failure) {
                if(failure==0) {site->features|=FEATURE_XENOFUNGUS;}
                if(failure==1) {site->rockiness=ROCKINESS_ROCKY;}
                if(failure==2) {roles[7]="absent-role";}
                if(failure==3) {auto data=game->GetMap()->GetSmacWorkerKnowledge()->CopyData(); data.tiles.at(2*64+3).claimant=2; game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(data));}
                if(failure==4) {auto value=pod->Conventional(); value.spent=3; um->UpdateConventionalUnit(GSE_CALL,pod,value);}
                const auto before=fixture.Snapshot(); const auto id=Base::GetNextId();
                GT_ASSERT(Rejects([&]{game->GetBM()->FoundSmacBase(GSE_CALL,*pod,roles);}) && fixture.Snapshot()==before && Base::GetNextId()==id,
                    "rejected founding changed Base/Unit/knowledge or ID");
                site->features=0; site->rockiness=ROCKINESS_FLAT; roles[7]=Roles().specialists[6];
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
                fixture.BasePresentationRoles(roles); game->Event(GSE_CALL,"found_base",{{"unit",pod->Wrap(GSE_CALL)}}); fixture.DrainEvents();
                auto* founded=site->base;
                GT_ASSERT(founded && founded->GetProductionState()->current_item.smac_source_id==64
                    && founded->GetProductionState()->minerals==(support==-2 ? 0 : game->GetState()->m_settings.global.GetSmacProductionRules()->retool_exemption),
                    "founding hardcoded Scout or substituted current SUPPORT for retained credit");
                GT_ASSERT(founded->GetSmacResourceContext().governor_options==(difficulty==0 ? 0xc0dfbe41u : 0x00dfbe41u)
                    && base->GetSmacResourceContext().governor_options==0x40u,"later founding copied another Base override instead of the initial faction default"); GT_OK();
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
			game->PublishSmacWorkerKnowledge(GSE_CALL,KnowledgeValue(false,true,128,64));
			const auto target=game->MakeSmacProductionItem(base->m_owner,ProductionItemKind::Unit,0);
			base->SetProductionState(GSE_CALL,ProductionState{target,27,target,77,std::vector<ProductionItem>{}});base->SetAccumulatedNutrients(0);
			next_unit=game::backend::unit::Unit::GetNextId();fixture.QueueProbeEvent(GSE_CALL,[&]{obsolete_command=true;});
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
			GT_ASSERT(Rejects([&]{game->Event(GSE_CALL,"choose_colony",{{"request_id",VALUE(value::Int,,request_id)},{"choice",VALUE(value::String,,"accept")}});}),"last-owned Accept must leave Delay available");
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
			bm->DefineFacility(new FacilityDef("screen-hq","HQ","",0,{},1));first->m_facilities.emplace("screen-hq",bm->GetFacilityDef("screen-hq"));
			auto allocation=*first->GetSmacAllocation();allocation.selected=5;first->PublishSmacAllocation(GSE_CALL,allocation,Roles());
			const auto former=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,1);
			game->GetMap()->GetTile(8,2)->terraforming=TERRAFORMING_FOREST;
			Base::pops_t pops;pops.emplace(std::piecewise_construct,std::forward_as_tuple(19),std::forward_as_tuple(nullptr,19,bm->GetPopDef(Roles().worker),0,game->GetMap()->GetTile(8,2)));
			auto* second=new Base(game,702,first->m_owner,first->m_faction,game->GetMap()->GetTile(6,2),"Next Base",{},pops,20,0,
				ProductionState{former,0,former,0,std::vector<ProductionItem>{}},SmacBaseResourceContext{1,false,0,0,0,false});
			bm->SpawnBase(GSE_CALL,second);second->PublishSmacAllocation(GSE_CALL,SmacBaseAllocationState{5,{{},0}},Roles());
			auto knowledge=KnowledgeValue(true,true,128,64).CopyData();for(auto& tile:knowledge.tiles){tile.explored_by=2;tile.claimant=1;}
			game->PublishSmacWorkerKnowledge(GSE_CALL,Knowledge::Create(knowledge));
			const auto colony=game->MakeSmacProductionItem(first->m_owner,ProductionItemKind::Unit,0);
			first->SetProductionState(GSE_CALL,ProductionState{colony,27,colony,77,std::vector<ProductionItem>{}});first->SetAccumulatedNutrients(0);
			fixture.QueueProbeEvent(GSE_CALL,[&]{obsolete=true;});fixture.BeginColonyChoice(GSE_CALL,first->m_id);
			auto* pending=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(game->Wrap(GSE_CALL)),"get_pending_smac_colony"));
			request=static_cast<value::Int*>(pending->Get("request_id"))->value;
			game->Event(GSE_CALL,"choose_colony",{{"request_id",VALUE(value::Int,,request)},{"choice",VALUE(value::String,,"screen")}});GT_OK();
		});GT_ASSERT(setup.empty(),"Screen setup: "+setup);fixture.DrainEvents();
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
			std::optional<int64_t> shown;
			auto* choices=static_cast<value::Object*>(Call(GSE_CALL,static_cast<value::Object*>(first->Wrap(GSE_CALL)),"get_smac_production_choices"));
			for(auto* entry:static_cast<value::Array*>(choices->Get("choices"))->value) {
				auto* choice=static_cast<value::Object*>(entry);
				if(static_cast<value::Int*>(choice->Get("source_id"))->value==1) {shown=static_cast<value::Int*>(choice->Get("minerals"))->value;}
			}
			GT_ASSERT(shown==30,"Screen production selector omitted the admitted Former quote");
			game->Event(GSE_CALL,"select_smac_production",{{"base",first->Wrap(GSE_CALL)},{"source_id",VALUE(value::Int,,1)},
				{"expected_state",VALUE(value::String,,SerializeProductionState(first->GetProductionState()).ToString())},
				{"expected_minerals",VALUE(value::Int,,*shown)},{"confirm",VALUE(value::Bool,,true)}});GT_OK();
		});GT_ASSERT(opened.empty(),"Screen entry: "+opened);fixture.DrainEvents();fixture.DrainEvents();
		const auto edited=fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* first=game->GetBM()->GetBase(701);auto* second=game->GetBM()->GetBase(702);
			bool research=false;fixture.ResourceState([&](auto& state){research=state.technology.factions[0].labs==2;});
			GT_ASSERT(first->GetSmacAllocation()->specialists.total_count==1 && first->GetProductionState()->current_item.smac_source_id==1
				&& first->GetAccumulatedNutrients()==1 && first->GetProductionState()->minerals==30
				&& game->GetCanonicalEconomy(first->m_owner).credits==101 && research && second->GetProductionState()->minerals==0,
				"Screen edits failed or retroactively reposted the current Base");
			GT_ASSERT((game->GetCanonicalBaseEnergy(*first).tail.final==SmacEnergyCategories{1,2,1}),"Screen worker click did not refresh actual E/P/L outputs");
			for(int i=0;i<2;++i) {game->Event(GSE_CALL,"close_colony_screen",{{"request_id",VALUE(value::Int,,request)}});}
			// Accepted while open, but ordered after close: its retired stamp must
			// prevent it from changing the Base after the suspended turn finishes.
			game->Event(GSE_CALL,"work_base_tile",{{"base",first->Wrap(GSE_CALL)},{"tile",game->GetMap()->GetTile(4,2)->Wrap(GSE_CALL)}});GT_OK();
		});GT_ASSERT(edited.empty(),"Screen edits: "+edited);fixture.DrainEvents();
		return fixture.Run([&](GSE_CALLABLE)->std::string {
			auto* game=fixture.Game();auto* first=game->GetBM()->GetBase(701);auto* second=game->GetBM()->GetBase(702);
			bool research=false;fixture.ResourceState([&](auto& state){research=state.technology.factions[0].labs==3;});
			GT_ASSERT(first->GetAccumulatedNutrients()==1 && first->GetProductionState()->minerals==30 && first->GetSmacAllocation()->specialists.total_count==1
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
                const auto result=bm->AbandonSmacBase(GSE_CALL,*lost,um->GetUnitDef("choice-colony"),SmacMoraleMode::Mode0);
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
                GT_ASSERT(after.units.size()==4 && after.units.back().id==result.newborn_id,"territory commit discarded newborn sight observation");
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
    task->AddTest("SMAC Colony abandonment rejects missing retained and last-Base authority atomically",[](GSE*,context::Context*) -> std::string {
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
                GT_ASSERT(Rejects([&]{bm->AbandonSmacBase(GSE_CALL,*base,um->GetUnitDef("choice-colony"),SmacMoraleMode::Mode0);})
                    && fixture.Snapshot()==before && Unit::GetNextId()==next && fixture.GameplayRandom()==pair && fixture.Updates().empty(),"unavailable continuation published partial abandonment");
                if(survivor) {
                    GT_ASSERT(!survivor->GetSmacUpkeepCache(),"allocation/query fabricated retained calculation");
                    survivor->RecalculateSmacWorkers(GSE_CALL,roles);fixture.Updates();const auto calculated=fixture.Snapshot();
                    GT_ASSERT(Rejects([&]{bm->AbandonSmacBase(GSE_CALL,*base,um->GetUnitDef("live-scout"),SmacMoraleMode::Mode0);})
                        && fixture.Snapshot()==calculated && Unit::GetNextId()==next && fixture.GameplayRandom()==pair && fixture.Updates().empty(),"late factory rejection changed Base/home/queue authority");
                }
                GT_OK();
            });if(!error.empty()){return error;}
        }return {};
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
                GT_ASSERT(!um->GetUnit(91)->Conventional().home_base_id && um->GetUnitRecordOrder()==std::vector<size_t>({91,92,93,94}),"starvation changed unhomed Unit or Unit roster");
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
	task->AddTest( "SMAC live upkeep rejects last-owned-Base removal without partial production", []( GSE*, context::Context* ) -> std::string {
		const auto source = []( const std::string& filename ) { auto text = UpkeepSource( filename );
			if ( filename == "alpha.txt" ) { const auto at = text.find( "#RULES\n" ); const auto end = text.find( '\n', at + 7 ); const auto next = end + 1; text.replace( next, text.find( ',', next ) - next, "3" ); } return text; };
		ConventionalUnitFixture fixture; fixture.ConfigureProduction( source, "GAIANS" );
		return fixture.Run( [ & ]( GSE_CALLABLE ) -> std::string {
			using namespace game::backend::unit;
			auto* game = fixture.Game(); auto* base = Populate( GSE_CALL, fixture, true ); game->PublishSmacWorkerKnowledge( GSE_CALL, KnowledgeValue( true, true, 128, 64 ) );
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
			const auto scout = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Unit, 2 ); const auto facility = game->MakeSmacProductionItem( base->m_owner, ProductionItemKind::Facility, 1 );
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
				base->SetProductionState( GSE_CALL, Production( kind ) ); const auto before = Base::Serialize( base ).ToString();
				base->RecalculateSmacWorkers( GSE_CALL, Roles() );
				GT_ASSERT( Base::Serialize( base ).ToString() == before && base->GetSmacAllocation() == allocation && &base->m_pops.at( 19 ) == pop
					&& !pop->GetWorkedTileCoords() && pop->m_def->m_smac_specialist_id == 1, "ordinary recalculation optimized a complete valid Doctor allocation" );
				base->RecalculateSmacWorkers( GSE_CALL, Roles(), SmacBaseResourcePhase::Upkeep );
				GT_ASSERT( Base::Serialize( base ).ToString() == before, "unmanaged upkeep changed a complete valid Doctor allocation" );
				GT_ASSERT( Rejects( [ & ] { base->RecalculateSmacWorkers( GSE_CALL, Roles(), static_cast< SmacBaseResourcePhase >( 99 ) ); } )
					&& Base::Serialize( base ).ToString() == before, "unknown allocation phase changed Base" );
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
