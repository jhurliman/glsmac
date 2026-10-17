#include "SmacUnitSupport.h"
#include "game/backend/base/SmacUnitSupport.h"
#include "fixtures/SmacUnitSupport.h"
#include "task/gsetests/GSETests.h"

#include <algorithm>
#include <limits>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace game::backend::base;
using Support = SmacUnitSupport;
using D = SmacSupportDisposition;
SmacSupportUnit Ordinary( uint64_t id ) {
	return {id,true,SmacSupportHome::EvaluatingBase,SmacSupportPackage::Combat,false,SmacSupportDesign::PlayerDesigned,false,false,SmacSupportTerrain::Land};
}
std::vector< uint64_t > Paid( const SmacUnitSupportResult& result ) {
	std::vector< uint64_t > paid;
	for ( const auto& unit : result.units ) { if ( unit.disposition == D::Paid ) { paid.push_back( unit.id ); } }
	return paid;
}
bool Rejects( const std::function< void() >& call ) { try { call(); } catch ( const std::invalid_argument& ) { return true; } return false; }
}

void AddSmacUnitSupportTests( task::gsetests::GSETests* task ) {
	task->AddTest( "SMAC unit support matches 28 original ordered support outputs", GT() {
		for ( const auto& row : smac_support_fixture::cases ) {
			const std::vector< SmacSupportUnit > units( row.units.begin(), row.units.begin() + row.count );
			for ( int pass = 0; pass < 2; ++pass ) {
				const auto result = Support::Recalculate( row.rating, row.population, units );
				GT_ASSERT( result.units.size() == row.count && result.mineral_support == row.cost && result.eligible_count == row.eligible, "original totals: " + std::string(row.id) );
				uint32_t paid = 0;
				for ( size_t i = 0; i < result.units.size(); ++i ) {
					GT_ASSERT( result.units[i].id == units[i].id, "input order/identity changed" );
					if ( result.units[i].disposition == D::Paid ) { paid |= uint32_t(1) << i; }
				}
				GT_ASSERT( paid == row.paid_mask && result.paid_count == int32_t(Paid(result).size()), "original paid indicators: " + std::string(row.id) );
			}
		}
		GT_OK();
	} );
	task->AddTest( "SMAC unit support matches 23 original Clean recalculations", GT() {
		int transitions = 0;
		for ( const auto& row : smac_support_fixture::cases ) {
			if ( row.clean_index < 0 ) { continue; }
			++transitions;
			std::vector< SmacSupportUnit > units( row.units.begin(), row.units.begin() + row.count );
			const auto before = Support::Recalculate( row.rating, row.population, units );
			GT_ASSERT( before.units[row.clean_index].disposition == D::Paid, "control did not begin with a paid unit" );
			units[row.clean_index].clean_reactor = true;
			const auto after = Support::Recalculate( row.rating, row.population, units );
			GT_ASSERT( after.mineral_support == row.clean_cost && after.units[row.clean_index].disposition == D::Exempt, "original Clean transition: " + std::string(row.id) );
			GT_ASSERT( before.mineral_support == row.cost && before.units[row.clean_index].disposition == D::Paid, "recalculation aliased an earlier result" );
		}
		GT_ASSERT( transitions == 23, "transition projection count changed" );
		GT_OK();
	} );
	task->AddTest( "SMAC unit support redistributes free places in explicit order", GT() {
		// SUP-027/028 independently captured the native terrain transition.
		auto native = Ordinary(90); native.design = SmacSupportDesign::BuiltIn; native.psi_weapon = true; native.fungus = true;
		std::vector< SmacSupportUnit > units{native,Ordinary(80),Ordinary(7),Ordinary(1)};
		const std::vector<uint64_t> first{1}, displaced{7,1}, reordered{90,1};
		GT_ASSERT( Paid(Support::Recalculate(0,2,units)) == first, "qualifying native consumed a free place" );
		units[0].fungus = false;
		GT_ASSERT( Paid(Support::Recalculate(0,2,units)) == displaced, "leaving fungus did not redistribute support" );
		std::swap(units[0],units[2]);
		GT_ASSERT( Paid(Support::Recalculate(0,2,units)) == reordered, "records sorted by identity instead of supplied order" );
		units[0].clean_reactor = true;
		GT_ASSERT( Paid(Support::Recalculate(0,2,units)) == first, "new exemption retained an occupied free place" );
		units[0].clean_reactor = false; units[0].home = SmacSupportHome::OtherBase;
		GT_ASSERT( Paid(Support::Recalculate(0,2,units)) == first, "rehome retained a free place" );
		units[0].home = SmacSupportHome::EvaluatingBase; units[0].owned_by_base_faction = false;
		GT_ASSERT( Paid(Support::Recalculate(0,2,units)) == first, "ownership change retained a free place" );
		std::vector<SmacSupportUnit> many;
		for ( uint64_t id = 0; id < 7; ++id ) { many.push_back(Ordinary(id)); }
		GT_ASSERT( Support::Recalculate(3,6,many).paid_count == 1 && Support::Recalculate(3,5,many).paid_count == 2 && Support::Recalculate(0,6,many).paid_count == 5, "population/rating recalculation used an old allowance" );
		GT_OK();
	} );
	task->AddTest( "SMAC unit support distinguishes ignored flags and native terrain exemptions", GT() {
		std::vector<SmacSupportUnit> units{Ordinary(0),Ordinary(1),Ordinary(2),Ordinary(3),Ordinary(4),Ordinary(5)};
		units[0].owned_by_base_faction = false;
		units[1].home = SmacSupportHome::None; units[2].home = SmacSupportHome::OtherBase;
		units[3].package = SmacSupportPackage::Supply;
		const auto result = Support::Recalculate(0,2,units);
		GT_ASSERT( result.units[0].disposition == D::Ignored && result.units[1].disposition == D::Ignored && result.units[2].disposition == D::Ignored && result.units[3].disposition == D::Exempt && result.units[4].disposition == D::Free && result.units[5].disposition == D::Free, "ignored records cannot be used to clear another base's paid flag" );
		// Semantic predicate controls extend the captured shelf/deep boundary;
		// they do not claim legal native placement or a territory producer.
		auto native = Ordinary(0); native.design = SmacSupportDesign::BuiltIn; native.psi_weapon = true; native.fungus = true;
		for ( auto band : {SmacSupportTerrain::Land,SmacSupportTerrain::Shelf} ) {
			native.terrain = band; GT_ASSERT( Support::Recalculate(-4,2,{native}).eligible_count == 0, "native land/shelf exemption lost" );
		}
		for ( auto band : {SmacSupportTerrain::DeepOcean,SmacSupportTerrain::Trench} ) {
			native.terrain = band; GT_ASSERT( Support::Recalculate(-4,2,{native}).mineral_support == 2, "deep/trench fungus incorrectly exempt" );
		}
		native.terrain = SmacSupportTerrain::Land; native.design = SmacSupportDesign::PlayerDesigned;
		GT_ASSERT( Support::Recalculate(-4,2,{native}).mineral_support == 2, "player-designed psi treated as built-in native" );
		native.design = SmacSupportDesign::BuiltIn; native.psi_weapon = false;
		GT_ASSERT( Support::Recalculate(-4,2,{native}).mineral_support == 2, "built-in conventional treated as native psi" );
		GT_OK();
	} );
	task->AddTest( "SMAC unit support bounds population and original lookup arithmetic", GT() {
		const auto low = Support::Recalculate(std::numeric_limits<int32_t>::min(),0,{Ordinary(1)});
		const auto high = Support::Recalculate(std::numeric_limits<int32_t>::max()-4,127,{Ordinary(1)});
		GT_ASSERT( low.free_allowance == 0 && low.mineral_support == 2 && high.free_allowance == 127 && high.mineral_support == 0, "nonoverflowing lookup clamp boundary" );
		GT_ASSERT( Support::Recalculate(4,0,{}).free_allowance == 4 && Support::Recalculate(0,127,{}).free_allowance == 2, "high population changed ordinary rating" );
		for ( int32_t population : {-1,128} ) { GT_ASSERT( Rejects([&]{Support::Recalculate(0,population,{});}), "invalid population admitted" ); }
		for ( int32_t rating : {std::numeric_limits<int32_t>::max()-3,std::numeric_limits<int32_t>::max()} ) { GT_ASSERT( Rejects([&]{Support::Recalculate(rating,2,{});}), "overflowing original lookup addition admitted" ); }
		GT_ASSERT( int64_t(Support::MAX_ORDERED_UNITS)*2 <= std::numeric_limits<int32_t>::max(), "count bound does not protect worst-case cost" );
		GT_OK();
	} );
	task->AddTest( "SMAC unit support rejects ambiguous records before returning a result", GT() {
		const auto first = Ordinary(std::numeric_limits<uint64_t>::max());
		std::vector<SmacSupportUnit> units{first,Ordinary(0)};
		const auto before = Support::Recalculate(-4,2,units);
		units[1].id = first.id;
		GT_ASSERT( Rejects([&]{Support::Recalculate(-4,2,units);}), "duplicate identity admitted" );
		for ( int field = 0; field < 4; ++field ) {
			units[1] = Ordinary(0);
			if ( field == 0 ) { units[1].home = static_cast<SmacSupportHome>(99); }
			if ( field == 1 ) { units[1].package = static_cast<SmacSupportPackage>(99); }
			if ( field == 2 ) { units[1].design = static_cast<SmacSupportDesign>(99); }
			if ( field == 3 ) { units[1].terrain = static_cast<SmacSupportTerrain>(99); }
			GT_ASSERT( Rejects([&]{Support::Recalculate(-4,2,units);}), "unsupported semantic record admitted" );
		}
		GT_ASSERT( before.mineral_support == 4 && before.units[0].id == first.id && before.units[1].id == 0 && units[0].id == first.id, "failed recomputation mutated owned prior result/input" );
		GT_OK();
	} );
}
}
