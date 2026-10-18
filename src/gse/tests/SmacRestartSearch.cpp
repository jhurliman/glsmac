#include "SmacRestartSearch.h"
#include "fixtures/SmacRestartSearch.h"
#include "fixtures/SmacRestartCacheMiss.h"
#include "game/backend/base/SmacWorkedSquareCollection.h"
#include "task/gsetests/GSETests.h"
#include <algorithm>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace smac_restart_fixture;
Tile Read(const Case& row,search::Coordinate point) {
	for(const auto& entry:row.overrides) {if(entry.first==point) {return entry.second;}}return row.fallback;
}
search::Candidate Preprocess(const Case& row,search::Coordinate point) {
	const auto tile=Read(row,point);search::Candidate result;
	result.elevation=tile.elevation;result.rockiness=tile.rockiness;result.region=tile.region;result.fungus=tile.fungus;
	result.occupied=tile.occupied;result.valid_occupant=tile.valid_occupant;result.territory_owner=tile.territory;
	if(tile.region==1) {result.area=row.area;result.fertility=row.fertility;result.present_factions=row.present;}
	result.landmark_bucket=32; // explicitly empty positive-detail input in these two native worlds
	const auto& geometry=row.input.geometry;
	const game::backend::base::SmacWorkerRadiusMap radius{point,geometry.width,geometry.height,geometry.wrap};
	for(int offset=0;offset<21;++offset) {
		const auto neighbor=game::backend::base::SmacWorkedSquareCollection::Resolve(radius,offset);if(!neighbor) {continue;}
		const auto value=Read(row,*neighbor);if(value.elevation>=3 && value.moisture && value.rockiness<=1 && !value.fungus) {++result.wet_neighbors;}
	}
	for(const auto& base:row.bases) {
		const auto center=Read(row,base.point);if(center.region!=tile.region) {continue;}
		auto dx=std::abs(int64_t(point[0])-base.point[0]);const auto dy=std::abs(int64_t(point[1])-base.point[1]);
		if(geometry.wrap) {dx=std::min(dx,int64_t(geometry.width)-dx);}
		const auto distance=static_cast<int32_t>(std::max(dx,dy)-(std::abs(dx-dy)+2)/4);
		if(distance<=result.nearest_distance) {result.nearest=search::Base{base.owner,center.region};result.nearest_distance=distance;}
	}
	return result;
}
}
void AddSmacRestartSearchTests(task::gsetests::GSETests* task) {
	task->AddTest("SMAC restart search matches all 1793 original candidates gates and CRT states",GT(){
		size_t candidates=0;
		for(const auto& row:cases) {
			const game::backend::SmacGameplayRandomState initial(row.shared,row.crt);size_t observed=0,reads=0;
			const auto result=search::Search(row.input,initial,[&](auto point){return Preprocess(row,point);},[&](auto point) {
				const auto tile=Read(row,point);if(tile.rating!=1 || tile.suppress || tile.detail) {throw std::runtime_error("native cached-only boundary differs");}++reads;
			},[&](const auto& actual) {
				if(observed>=row.trace.size()) {throw std::runtime_error("restart generated an extra candidate");}
				const auto& expected=row.trace[observed++];
				if(actual.number!=expected.number || actual.point!=expected.point || actual.crt!=expected.crt || actual.outcome!=expected.outcome) {
					throw std::runtime_error(std::string(row.name)+" native candidate differs at "+std::to_string(actual.number)+": "+search::OutcomeName(actual.outcome)+" expected "+search::OutcomeName(expected.outcome));
				}
			});
			GT_ASSERT(observed==row.trace.size() && result.pairs==observed && result.pairs*2==row.draws,"native attempt budget differs");
			GT_ASSERT(result.site==row.selected && result.random.Crt()==row.final_crt && result.random.Shared()==row.shared
				&& result.nearest_distance==row.distance && result.rating_reads==row.rating_reads && reads==row.rating_reads,"native restart result RNG scratch or rating reads differ");
			GT_ASSERT(initial.Crt()==row.crt && initial.Shared()==row.shared,"prepared restart changed input RNG");candidates+=observed;
		}GT_ASSERT(candidates==1793,"native candidate coverage changed");GT_OK();
	});
	task->AddTest("SMAC restart search resolves original cache misses and selects a zero-rated monolith",GT(){
		for(const auto& row:smac_restart_miss_fixture::cases) {
			const auto& geometry=row.geometry;using namespace game::backend::map;
			const smac_site::Query query=[&](auto point) {
				for(const auto& entry:row.overrides) {if(entry.first==point) {return entry.second;}}return row.fallback;
			};
			std::vector<uint8_t> cache(size_t(geometry.width)*geometry.height/2,row.before);size_t writes=0;
			const search::Input input{geometry,row.turn,1,0,3,32,0};
			const auto result=search::Search(input,game::backend::SmacGameplayRandomState(0,row.crt),[&](auto point) {
				const auto tile=query(point);search::Candidate candidate;
				candidate.elevation=tile.elevation;candidate.rockiness=tile.rockiness;candidate.region=tile.region;candidate.fungus=tile.fungus;
				candidate.occupied=tile.base;candidate.valid_occupant=true;candidate.area=row.areas[tile.region];candidate.fertility=960;candidate.landmark_bucket=32;
				const game::backend::base::SmacWorkerRadiusMap radius{point,geometry.width,geometry.height,geometry.wrap};
				for(int offset=0;offset<21;++offset) {const auto nearby=game::backend::base::SmacWorkedSquareCollection::Resolve(radius,offset);if(!nearby) {continue;}
					const auto value=query(*nearby);candidate.wet_neighbors+=value.elevation>=3 && value.moisture && value.rockiness<=1 && !value.fungus;
				}return candidate;
			},[&](auto point) {
				auto& rating=cache[size_t(point[1])*(geometry.width/2)+point[0]/2];
				if(!rating) {rating=static_cast<uint8_t>(smac_site::Quality(geometry,point,row.turn,row.areas,query).rating);++writes;}
			});
			GT_ASSERT(result.site==std::optional<search::Coordinate>(row.selected) && result.pairs==1 && result.rating_reads==1 && writes==1
				&& result.random.Crt()==row.final_crt && result.random.Shared()==0 && cache[size_t(row.selected[1])*(geometry.width/2)+row.selected[0]/2]==row.after,
				"native cache-miss selection or side effect differs: "+std::string(row.name));
		}GT_OK();
	});
}
}
