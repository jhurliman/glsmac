#include "SmacSiteRatingCache.h"
#include "fixtures/SmacSiteRatingCache.h"
#include "game/backend/map/GlsmacTerritoryTopology.h"
#include "task/gsetests/GSETests.h"
#include <algorithm>
#include <stdexcept>

namespace gse::tests {
namespace {
using namespace smac_site_cache_fixture;
using Cache=site::RatingCache;
std::string Decode(const char* encoded) {std::string result;for(;*encoded;++encoded) {result+=char(*encoded-'0');}return result;}
std::vector<site::Coordinate> Exclusions(const Cache& cache) {
	const auto geometry=cache.GetGeometry();std::vector<site::Coordinate> result;
	for(int y=0;y<geometry.height;++y) {for(int x=y&1;x<geometry.width;x+=2) {if(cache.Excluded({{x,y}})) {result.push_back({{x,y}});}}}
	return result;
}
bool SameSet(std::vector<site::Coordinate> left,std::vector<site::Coordinate> right) {
	std::sort(left.begin(),left.end());std::sort(right.begin(),right.end());return left==right;
}
}
void AddSmacSiteRatingCacheTests(task::gsetests::GSETests* task) {
	task->AddTest("SMAC site cache direct misses preserve exclusions and do not refill neighbors",GT(){
		const site::Coordinate point{{10,8}};const auto before=Cache::Fresh({32,16,true}).FoundBase(point);
		size_t reads=0;const auto zero=before.Resolve(point,[&](auto actual)->uint8_t {if(actual!=point) {throw std::runtime_error("direct query visited another tile");}++reads;return 0;});
		const auto filled=zero.Resolve(point,[&](auto)->uint8_t {++reads;return 7;});
		const auto reused=filled.Resolve(point,[&](auto)->uint8_t {throw std::runtime_error("nonzero cache recalculated");});
		GT_ASSERT(reads==2 && zero==before && filled.Excluded(point) && filled.Rating(point)==7 && reused==filled,"direct miss lost exclusions or cached zero as a hit");
		auto packed=filled.Serialize();packed[8*16+5]=16;
		GT_ASSERT(packed==before.Serialize(),"direct miss rewrote neighbor history");GT_OK();
	});
	task->AddTest("SMAC site cache matches seven original mixed refill passes and zero misses",GT(){
		for(const auto& row:cases) {
			const auto before=Cache::Restore(row.geometry,Decode(row.before));size_t visited=0;
			const auto after=before.Refill([&](site::Coordinate point)->uint8_t {
				if(visited>=row.queries.size() || point!=row.queries[visited].point) {throw std::runtime_error("native cache query order differs");}
				return row.queries[visited++].rating;
			});
			GT_ASSERT(visited==row.queries.size() && after.Serialize()==Decode(row.after),"original cache result differs: "+std::string(row.name));
			GT_ASSERT(before.Serialize()==Decode(row.before),"prepared refill published into input cache");
			GT_ASSERT(Cache::Restore(row.geometry,after.Serialize())==after,"site-cache codec changed retained state");
		}GT_OK();
	});
	task->AddTest("SMAC site cache matches native removal repair across all surviving Bases",GT(){
		std::string packed(256,char(13));
		for(const auto point:lost_inner) {packed[size_t(point[1])*16+point[0]/2]|=16;}
		for(const auto point:stray) {packed[size_t(point[1])*16+point[0]/2]|=16;}
		const auto removed=Cache::Restore({32,16,true},packed).RemoveBase(lost,survivors);
		GT_ASSERT(SameSet(Exclusions(removed),removed_excluded),"all-survivor repair lost a distant sea Base or stray exclusion");
		std::vector<site::Coordinate> visited;
		(void)removed.Refill([&](auto point)->uint8_t {visited.push_back(point);return 1;});
		GT_ASSERT(visited==removed_queries && visited.size()==91,"native removal repair changed full-map refill eligibility/order");GT_OK();
	});
	task->AddTest("SMAC site cache matches original 989 founding and 52 removal query order",GT(){
		auto cache=Cache::Fresh({64,32,true}).FoundBase({{16,16}}).FoundBase({{20,16}});
		GT_ASSERT(SameSet(Exclusions(cache),founding_excluded),"founding exclusion union differs");
		std::vector<site::Coordinate> visited;cache=cache.Refill([&](auto point)->uint8_t {visited.push_back(point);return 1;});
		GT_ASSERT(visited==founding_queries && visited.size()==989,"complete native founding refill differs");
		cache=cache.RemoveBase({{20,16}},{{{16,16}}});
		GT_ASSERT(SameSet(Exclusions(cache),removal_excluded),"removal exclusion repair differs");
		visited.clear();cache=cache.Refill([&](auto point)->uint8_t {visited.push_back(point);return 1;});
		GT_ASSERT(visited==removal_queries && visited.size()==52,"complete native removal refill differs");GT_OK();
	});
	task->AddTest("SMAC site cache preserves nonzero excluded scores and postpass flags",GT(){
		const site::Geometry geometry{32,16,true};const auto seed=Cache::Restore(geometry,std::string(256,char(13)));
		const auto marked=seed.FoundBase({{10,8}});size_t inner=0,outer=0,other=0;
		for(int y=0;y<16;++y) {for(int x=y&1;x<32;x+=2) {
			const site::Coordinate point{{x,y}};
			if(marked.Excluded(point)) {++inner;GT_ASSERT(marked.Rating(point)==13,"marking Base cleared inner rating");}
			else if(!marked.Rating(point)) {++outer;}else {++other;}
		}}
		GT_ASSERT(inner==21 && outer==28 && other==207,"native Base-mark footprint differs");
		const auto cleared=marked.ClearRatings();GT_ASSERT(Exclusions(cleared)==Exclusions(marked),"terrain postpass erased exclusions");
		for(const auto byte:cleared.Serialize()) {GT_ASSERT(!(byte&15),"terrain postpass retained a rating");}
		const auto local=seed.InvalidateLocal({{10,8}});size_t zeros=0;for(const auto byte:local.Serialize()) {zeros+=byte==0;}
		GT_ASSERT(zeros==21,"local invalidation differs from native inner21");
		using Topology=game::backend::map::GlsmacTerritoryTopology;
		auto topology=Topology::Build(32,16,true,std::vector<bool>(256,false)).WithSiteRatings(marked);
		GT_ASSERT(Topology::Deserialize(topology.Serialize())==topology,"world topology did not retain nonzero excluded ratings");
		for(int variant=0;variant<2;++variant) {auto malformed=topology.Serialize();if(variant==0) {malformed[15]=4;}else {malformed.back()=32;}
			bool rejected=false;try {(void)Topology::Deserialize(malformed);}catch(const std::runtime_error&) {rejected=true;}
			GT_ASSERT(rejected,"world topology accepted malformed site-cache flags");}
		const auto postpass=topology.WithTerrainPostpass(std::vector<bool>(256,false),std::vector<bool>(256,true));
		GT_ASSERT(postpass.SiteRatings()==cleared,"owned terrain postpass did not preserve exclusions while clearing ratings");
		GT_ASSERT(topology.Rebuild(std::vector<bool>(256,false)).SiteRatings()==marked,"region fill refreshed site-cache history");GT_OK();
	});
	task->AddTest("SMAC site cache rejects malformed saves and preserves preparation on query failure",GT(){
		const auto before=Cache::Fresh({32,16,true});bool rejected=false;
		try {(void)before.Refill([](auto)->uint8_t {throw std::runtime_error("query failure");});} catch(const std::runtime_error&) {rejected=true;}
		GT_ASSERT(rejected && before==Cache::Fresh({32,16,true}),"failed query changed owned cache");
		for(int variant=0;variant<3;++variant) {
			auto packed=before.Serialize();if(variant==0) {packed.pop_back();}if(variant==1) {packed[0]=32;}if(variant==2) {packed.back()=char(255);}
			rejected=false;try {(void)Cache::Restore({32,16,true},packed);} catch(const std::invalid_argument&) {rejected=true;}
			GT_ASSERT(rejected,"malformed rating-cache payload accepted");
		}GT_OK();
	});
}
}
