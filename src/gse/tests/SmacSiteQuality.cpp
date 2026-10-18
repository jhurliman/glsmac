#include "SmacSiteQuality.h"
#include "fixtures/SmacSiteQuality.h"
#include "fixtures/SmacSeaSiteQuality.h"
#include "fixtures/SmacLateSiteQuality.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>

namespace gse::tests {
void AddSmacSiteQualityTests(task::gsetests::GSETests* task) {
	task->AddTest("SMAC land site quality matches 31 complete original queries",GT(){
		using namespace smac_site_fixture;
		for(const auto& row:cases) {
			site::Areas areas{};for(const auto& entry:row.areas) {areas.at(entry.first)=entry.second;}
			const auto query=[&](site::Coordinate point) {
				for(const auto& override:row.overrides) {if(override.first==point) {return override.second;}}
				return row.fallback;
			};
			const auto score=site::LandQuality(row.geometry,row.center,row.turn,areas,query);
			const auto generic=site::Quality(row.geometry,row.center,row.turn,areas,query);
			GT_ASSERT(generic.rating==score.rating && generic.raw==score.raw,"shared query changed the land branch");
			GT_ASSERT(score.rating==row.rating && score.raw==row.raw && score.points==row.points && score.production==row.production
				&& score.fungus_production==row.fungus_production && score.rivers==row.rivers && score.water==row.water && score.deep_water==row.deep_water
				&& score.outer_bases==row.outer_bases && score.rocky_neighbors==row.rocky && score.large_water==row.large && score.last==row.last,
				"original site quality differs: "+std::string(row.name)+" rating="+std::to_string(score.rating)+" raw="+std::to_string(score.raw));
		}
		GT_ASSERT(cases.size()==31,"original site case coverage changed");GT_OK();
	});
	task->AddTest("SMAC sea site quality matches 40 complete original queries and score terms",GT(){
		using namespace smac_sea_site_fixture;
		for(const auto& row:cases) {
			site::Areas areas{};for(const auto& entry:row.areas) {areas.at(entry.first)=entry.second;}
			const auto query=[&](site::Coordinate point) {
				for(const auto& override:row.overrides) {if(override.first==point) {return override.second;}}
				return row.fallback;
			};
			const auto score=site::Quality(row.geometry,row.center,row.turn,areas,query);
			GT_ASSERT(score.rating==row.rating && score.raw==row.raw && score.points==row.points && score.production==row.production
				&& score.fungus_production==row.fungus_production && score.rivers==row.rivers && score.water==row.water && score.deep_water==row.deep_water
				&& score.outer_bases==row.outer_bases && score.rocky_neighbors==row.rocky && score.large_water==row.large && score.last==row.last,
				"original sea site quality differs: "+std::string(row.name)+" rating="+std::to_string(score.rating)+" raw="+std::to_string(score.raw));
		}GT_ASSERT(cases.size()==40,"original sea-site case coverage changed");GT_OK();
	});
	task->AddTest("SMAC land site quality rejects unclosed query domains",GT(){
		using namespace smac_site_fixture;
		for(int scenario=0;scenario<7;++scenario) {
			auto geometry=cases[0].geometry;auto center=cases[0].center;int turn=3;auto tile=cases[0].fallback;
			if(scenario==0) {geometry.width=63;}if(scenario==1) {center[0]=-2;}if(scenario==2) {center[0]++;}
			if(scenario==3) {turn=-1;}if(scenario==4) {tile.elevation=2;}if(scenario==5) {tile.moisture=4;}if(scenario==6) {tile.region=128;}
			bool rejected=false;try {(void)site::LandQuality(geometry,center,turn,{},[&](auto){return tile;});} catch(const std::invalid_argument&) {rejected=true;}
			GT_ASSERT(rejected,"unsupported site query admitted");
		}GT_OK();
	});
	task->AddTest("SMAC site quality matches 40 original startup and late fungus score stages",GT(){
		using namespace smac_late_site_fixture;
		for(const auto& row:cases) {
			site::Areas areas{};for(const auto& entry:row.areas) {areas.at(entry.first)=entry.second;}
			const auto query=[&](site::Coordinate point) {
				for(const auto& override:row.overrides) {if(override.first==point) {return override.second;}}
				return row.fallback;
			};
			const auto score=site::Quality(row.geometry,row.center,row.turn,areas,query);
			GT_ASSERT(score.rating==row.rating && score.raw==row.raw && score.points==row.points && score.production==row.production
				&& score.fungus_production==row.fungus_production && score.rivers==row.rivers && score.water==row.water && score.deep_water==row.deep_water
				&& score.outer_bases==row.outer_bases && score.rocky_neighbors==row.rocky && score.large_water==row.large && score.last==row.last,
				"original turn-dependent site score differs: "+std::string(row.name)+" rating="+std::to_string(score.rating)+" raw="+std::to_string(score.raw));
		}GT_ASSERT(cases.size()==40,"original turn-dependent site case coverage changed");GT_OK();
	});
}
}
