#include "SmacPredefinedUnitRules.h"
#include "fixtures/SmacRulesSource.h"
#include "task/gsetests/GSETests.h"

#include <cstring>
#include <functional>

namespace gse::tests {
namespace {
using namespace game::backend::settings;
namespace fixture = smac_rules_fixture;
using Profile = SmacPredefinedUnitRules;
bool Rejects( const std::function< void() >& operation ) {
	try { operation(); } catch ( const std::exception& ) { return true; } return false;
}
void Rebuild( Profile& p, const SmacTechnologyRules& t ) {
	auto& c = p.catalog; c = game::backend::unit::SmacPredefinedUnitCatalog::Build( c.source, c.chassis, c.weapons, c.defenses, c.reactors, c.abilities, t );
}
std::string ChangeInteger( const std::string& bytes, const size_t offset, const int64_t value ) {
	types::Buffer replacement; replacement.WriteInt( value ); auto result = bytes;
	result.replace( offset, replacement.lenw, replacement.ToString() ); return result;
}
std::vector< size_t > Integers( const std::string& bytes ) {
	std::vector< size_t > result;
	for ( size_t offset = 0; offset < bytes.size(); ) {
		uint32_t size = 0; std::memcpy( &size, bytes.data() + offset + 1, sizeof( size ) );
		if ( uint8_t( bytes[ offset ] ) == 2 ) { result.push_back( offset ); }
		offset += 6 + size;
	}
	return result;
}
}

void AddSmacPredefinedUnitRulesTests( task::gsetests::GSETests* task ) {
    task->AddTest("SMAC predefined V3 naming text preserves V2 unknown Sea and legacy V1 bytes",GT(){
        const auto tech=fixture::Technologies();const auto legacy=fixture::LegacyPredefinedUnits();const auto before=legacy.Serialize(tech).ToString();
        auto named=legacy;named.catalog.workshop_name_labels=game::backend::unit::ReadSmacWorkshopNameLabels(fixture::WorkshopLabelsSource());
        const auto wire=named.Serialize(tech).ToString();const auto restored=Profile::Deserialize(wire,tech);
        GT_ASSERT(restored==named && !(restored==legacy),"Naming text lost on transport or omitted from equality");
        types::Buffer sea;sea.WriteString(named.catalog.workshop_name_labels->at(7));
        auto v2=wire.substr(0,wire.size()-sea.lenw);v2=ChangeInteger(v2,0,2);
        const auto old_named=Profile::Deserialize(v2,tech);
        GT_ASSERT(old_named.catalog.workshop_name_labels->at(7).empty() && old_named.Serialize(tech).ToString()==v2,"V2 acquired guessed Sea text or changed bytes");
        GT_ASSERT(named.catalog.workshop_name_labels->at(7)=="Sea","Installed Sea label omitted");
        auto clear=restored;clear.catalog.workshop_name_labels.reset();
        GT_ASSERT(clear.Serialize(tech).ToString()==before,"Optional naming changed legacy V1 encoding");
        for(size_t i=0;i<8;++i) {auto changed=named;changed.catalog.workshop_name_labels->at(i)+=" translated";
            GT_ASSERT(!(changed==named) && Profile::Deserialize(changed.Serialize(tech).ToString(),tech)==changed,"Translated naming field lost");}
        for(size_t size=before.size();size<wire.size();++size) {GT_ASSERT(Rejects([&]{Profile::Deserialize(wire.substr(0,size),tech);}),"Truncated V2 naming fields accepted");}
        GlobalSettings old_host;old_host.Deserialize(fixture::Payload("smac",true,{17,INT32_MIN,INT32_MAX,-2,300,0,-7,8,9},{},{},{},{},before));
        const auto old_wire=old_host.Serialize().ToString();GlobalSettings old_copy;old_copy.Deserialize(old_wire);
        GT_ASSERT(old_copy.Serialize().ToString()==old_wire && !old_copy.GetSmacPredefinedUnitRules()->catalog.workshop_name_labels,"Legacy session acquired fresh naming labels");
        auto bad=named;bad.catalog.workshop_name_labels->at(6).clear();GT_ASSERT(Rejects([&]{bad.Serialize(tech);}),"Empty name marker accepted");
        GT_ASSERT(Rejects([&]{game::backend::unit::ReadSmacWorkshopNameLabels("#LABELS\n920\nshort\n");}),"Incomplete label source accepted");GT_OK();
    });
	task->AddTest( "SMAC predefined profile owns raw catalogs and verifies every derived field", GT() {
		const auto t = fixture::Technologies(); Profile saved;
		{ auto source = fixture::PredefinedSource(); auto parsed = Profile::Parse( source, t ); auto bytes = parsed.Serialize( t ).ToString();
			saved = Profile::Deserialize( bytes, t ); source.clear(); bytes.clear(); parsed.catalog.source.units.clear(); }
		const auto expected = fixture::LegacyPredefinedUnits(); auto copy = saved; auto moved = std::move( copy );
		GT_ASSERT( saved == expected && moved == expected && saved.catalog.units.size() == 14, "profile borrowed source or lost rows" );
		GT_ASSERT( saved.catalog.units[ 0 ].cost == 3 && saved.catalog.units[ 1 ].cost == 2 && saved.catalog.units[ 2 ].cost == 1, "host bypassed production cost calculation" );
		for ( size_t i = 0; i < saved.catalog.units.size(); ++i ) {
			for ( size_t field = 0; field < 18; ++field ) {
				auto bad = saved; auto& r = bad.catalog.units[ i ];
				switch ( field ) {
					case 0: ++r.source_id; break; case 1: ++r.chassis_id; break; case 2: ++r.weapon_id; break; case 3: ++r.defense_id; break;
					case 4: ++r.reactor; break; case 5: r.abilities ^= 1; break; case 6: ++r.automatic_plan; break; case 7: ++r.calculated_cost; break;
					case 8: ++r.calculated_carry; break; case 9: ++r.plan; break; case 10: ++r.cost; break; case 11: ++r.carry; break;
					case 12: ++r.icon; break; case 13: r.prerequisite.kind = SmacTechnologyReference::Kind::UNKNOWN; break;
					case 14: r.prerequisite.technology_id = 88; break; case 15: r.active = false; break; case 16: r.prototyped = true; break;
					case 17: r.faction_exclusion = 1; break;
				}
				GT_ASSERT( !( bad == saved ) && Rejects( [ & ]() { bad.Serialize( t ); } ), "forged derived field escaped rebuild validation" );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC predefined profile preserves raw metadata and unknown prerequisite identity", GT() {
		const auto t = fixture::Technologies(); const auto original = fixture::LegacyPredefinedUnits();
		std::vector< std::function< void( Profile& ) > > changes = {
			[]( auto& p ) { p.catalog.chassis.chassis.back().names[ 5 ].name += " suffix"; },
			[]( auto& p ) { p.catalog.chassis.chassis.back().names[ 5 ].grammar = "F2"; },
			[]( auto& p ) { p.catalog.chassis.chassis.back().range = INT32_MIN; },
			[]( auto& p ) { p.catalog.chassis.chassis.back().prerequisite = "Unknown"; },
			[]( auto& p ) { p.catalog.weapons.weapons.back().name += " suffix"; },
			[]( auto& p ) { p.catalog.weapons.weapons.back().short_name += " suffix"; },
			[]( auto& p ) { p.catalog.weapons.weapons.back().icon = INT32_MAX; },
			[]( auto& p ) { p.catalog.defenses.defenses.back().name += " suffix"; },
			[]( auto& p ) { p.catalog.reactors.reactors.back().power = INT32_MIN; },
			[]( auto& p ) { p.catalog.abilities.abilities.back().abbreviation = "A"; },
			[]( auto& p ) { p.catalog.abilities.abilities.back().description += "; retained"; },
			[]( auto& p ) { p.catalog.source.units.back().name += " suffix"; },
			[]( auto& p ) { p.catalog.source.units.back().icon = INT32_MIN; },
			[]( auto& p ) { p.catalog.source.units.back().prerequisite = "Unknown"; }
		};
		for ( const auto& change : changes ) {
			auto modified = original; change( modified ); Rebuild( modified, t );
			GT_ASSERT( !( modified == original ) && Profile::Deserialize( modified.Serialize( t ).ToString(), t ) == modified, "raw metadata omitted or normalized" );
		}
		auto unknown = original; unknown.catalog.source.units.back().prerequisite = "Unknown"; Rebuild( unknown, t );
		GT_ASSERT( unknown.catalog.units.back().prerequisite.kind == SmacTechnologyReference::Kind::UNKNOWN && !unknown.catalog.units.back().prerequisite.technology_id, "unknown prerequisite became available or disabled" );
		GT_OK();
	} );
	task->AddTest( "SMAC predefined profile rejects malformed frames and transport budgets", GT() {
		const auto t = fixture::Technologies(); const auto p = fixture::LegacyPredefinedUnits(); const auto bytes = p.Serialize( t ).ToString();
		for ( size_t length = 0; length < bytes.size(); ++length ) {
			GT_ASSERT( Rejects( [ & ]() { Profile::Deserialize( bytes.substr( 0, length ), t ); } ), "truncated profile accepted" );
		}
		GT_ASSERT( Rejects( [ & ]() { Profile::Deserialize( bytes + 'x', t ); } ), "trailing profile bytes accepted" );
		const auto integers = Integers( bytes );
		GT_ASSERT( Rejects( [ & ]() { Profile::Deserialize( ChangeInteger( bytes, integers[ integers.size() - 3 ], 7 ), t ); } ), "in-range forged derived icon bypassed rebuild" );
		for ( const auto offset : { integers.front(), integers[ 1 ], integers.back() } ) {
			for ( const auto value : { INT64_MIN, INT64_MAX } ) {
				GT_ASSERT( Rejects( [ & ]() { Profile::Deserialize( ChangeInteger( bytes, offset, value ), t ); } ), "hostile version/count/late field accepted" );
			}
		}
		GT_ASSERT( Rejects( [ & ]() { Profile::Deserialize( std::string( Profile::MAX_WIRE_BYTES + 1, 'x' ), t ); } ), "oversized profile accepted" );
		auto invalid = p; invalid.catalog.chassis.ruleset_id = "smax";
		GT_ASSERT( Rejects( [ & ]() { invalid.Serialize( t ); } ), "mixed identities accepted" );
		invalid = p; invalid.catalog.abilities.abilities.back().description = std::string( 513, 'x' );
		GT_ASSERT( Rejects( [ & ]() { invalid.Serialize( t ); } ), "text cap ignored" );
		invalid = p; for ( auto& a : invalid.catalog.abilities.abilities ) { a.description = std::string( 512, 'x' ); a.name = std::string( 128, 'n' ); }
		GT_ASSERT( Rejects( [ & ]() { invalid.Serialize( t ); } ), "aggregate profile cap ignored" );
		invalid = p; invalid.catalog.source.units.back().plan = static_cast< loader::txt::UnitPlan >( 256 ); Rebuild( invalid, t );
		GT_ASSERT( Rejects( [ & ]() { invalid.Serialize( t ); } ), "typed producer widened source-ingestion plan policy" );
		GT_OK();
	} );
    task->AddTest("SMAC Workshop host freezes naming text and rejects late replacement atomically",GT(){
        GlobalSettings host;host.SelectRuleset("smac");host.InitializeHostRules(fixture::Source);
        GT_ASSERT(host.GetSmacPredefinedUnitRules()->catalog.workshop_name_labels==game::backend::unit::ReadSmacWorkshopNameLabels(fixture::WorkshopLabelsSource()),"Host omitted installed naming fields");
        const auto before=host.Serialize().ToString();
        for(size_t i=0;i<8;++i) {auto changed=*host.GetSmacPredefinedUnitRules();changed.catalog.workshop_name_labels->at(i)+=" translated";
            const auto wire=changed.Serialize(fixture::Technologies()).ToString();
            GT_ASSERT(Rejects([&]{host.Deserialize(fixture::Payload("smac",true,{17,INT32_MIN,INT32_MAX,-2,300,0,-7,8,9},{},{},{},{},wire));}) && host.Serialize().ToString()==before,"Late naming replacement changed frozen session");
        }
        GlobalSettings pending;pending.SelectRuleset("smac");
        GT_ASSERT(Rejects([&]{pending.InitializeHostRules([](const std::string& name) {return name=="labels.txt" ? std::string("#LABELS\n920\nmissing\n"):fixture::Source(name);});})
            && !pending.IsRulesetLocked() && !pending.GetSmacPredefinedUnitRules() && !pending.GetSmacResourceRules(),"Missing late naming source published partial settings");
        pending.InitializeHostRules(fixture::Source);GT_ASSERT(pending.Serialize().ToString()==before,"Naming source retry changed session");GT_OK();
    });
	task->AddTest( "SMAC frozen predefined host profile stages failures and rejects late changes", GT() {
		GlobalSettings host; host.SelectRuleset( "smac" ); host.InitializeHostRules( fixture::Source ); const auto before = host.Serialize().ToString();
		GlobalSettings joined; joined.Deserialize( before ); GT_ASSERT( joined.Serialize().ToString() == before, "joined profile differs" );
		auto changed = fixture::PredefinedUnits(); changed.catalog.abilities.abilities.back().description += " changed";
		const auto bytes = changed.Serialize( fixture::Technologies() ).ToString();
		GT_ASSERT( Rejects( [ & ]() { joined.Deserialize( fixture::Payload( "smac", true, { 17, INT32_MIN, INT32_MAX, -2, 300, 0, -7, 8, 9 }, {}, {}, {}, {}, bytes ) ); } ), "changed late raw field accepted" );
		GT_ASSERT( joined.Serialize().ToString() == before, "locked profile rejection mutated settings" );
		GlobalSettings retry; retry.SelectRuleset( "smac" ); retry.game_name = "preserved";
		GT_ASSERT( Rejects( [ & ]() { retry.InitializeHostRules( []( const std::string& name ) {
			auto source = fixture::Source( name ); if ( name == "alpha.txt" ) { source.replace( source.find( "#UNITS" ), 6, "#WRONG" ); } return source;
		} ); } ), "missing unit table accepted" );
		GT_ASSERT( !retry.IsRulesetLocked() && !retry.GetSmacResourceRules() && !retry.GetSmacPredefinedUnitRules() && retry.game_name == "preserved", "late unit failure published partial profiles" );
		retry.InitializeHostRules( fixture::Source ); GT_ASSERT( retry.GetSmacPredefinedUnitRules().has_value(), "valid retry failed" );
		retry.ResetRuleset(); GT_ASSERT( !retry.GetSmacPredefinedUnitRules(), "reset retained catalog" );
		GT_OK();
	} );
}
}
