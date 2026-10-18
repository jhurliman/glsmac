#include "SmacRestartSearch.h"
#include <algorithm>
#include <stdexcept>

namespace game::backend::map::smac_restart {
namespace {
void Require(bool value,const char* message) {if(!value) {throw std::invalid_argument(message);}}
int32_t Margin(int32_t dimension,int32_t attempted) {
	return static_cast<int32_t>(std::clamp<int64_t>(int64_t(1024-attempted)*dimension/8192,0,9999));
}
int32_t Draw(SmacGameplayRandomState& random,int32_t range) {return range<=1 ? 0 : random.NextCrtRaw15()%range;}
}
const char* OutcomeName(Outcome outcome) {
	switch(outcome) {
		case Outcome::Terrain:return "terrain";case Outcome::MinimumDistance:return "minimum_base_distance";
		case Outcome::Altitude:return "altitude";case Outcome::Occupied:return "occupied";
		case Outcome::NorthMargin:return "north_margin";case Outcome::SouthMargin:return "south_margin";
		case Outcome::WetNeighborhood:return "wet_neighborhood";case Outcome::RelaxingDistance:return "relaxing_base_distance";
		case Outcome::Fertility:return "regional_fertility";case Outcome::Area:return "regional_area";
		case Outcome::Presence:return "regional_presence";case Outcome::Landmark:return "landmark_proximity";
		case Outcome::Claimed:return "claimed";case Outcome::CaptorRegion:return "captor_region";
		case Outcome::Limit:return "attempt_limit";case Outcome::Selected:return "selected";
	}
	throw std::invalid_argument("Unknown restart outcome");
}
Result Search(const Input& input,const SmacGameplayRandomState& initial,const Preprocess& preprocess,const ResolveRating& rating,const Observe& observe) {
	const auto width=input.geometry.width,height=input.geometry.height;
	Require(width>=2 && width<=512 && !(width&1) && height>=2 && height<=256 && input.turn>0
		&& input.owner>=1 && input.owner<=7 && input.captor>=0 && input.captor<=7 && input.difficulty>=0 && input.difficulty<=5
		&& input.scale>0 && input.scale<=512 && input.ocean>=0 && input.ocean<=2 && bool(preprocess) && bool(rating),"Unsupported human restart inputs");
	Result result{std::nullopt,initial,9999,0,0};
	for(int32_t n=0;n<=1024;++n) {
		const auto mx=input.geometry.wrap ? 0 : Margin(width,n),my=Margin(height,n);
		const auto raw_x=mx+Draw(result.random,width-2*mx),y=my+Draw(result.random,height-2*my);
		const Coordinate point{{2*(raw_x/2)+(y&1),y}};
		const auto candidate=preprocess(point); // includes the terminal candidate
		result.nearest_distance=candidate.nearest ? candidate.nearest_distance : 32;
		const auto k=n+1;++result.pairs;
		Outcome outcome=Outcome::Limit;
		if(k<=1024) {
			Require(candidate.elevation<=7 && candidate.rockiness<=3 && candidate.region<128 && candidate.wet_neighbors>=0 && candidate.wet_neighbors<=21
				&& candidate.area>=0 && candidate.fertility>=0 && candidate.fertility<=65536 && candidate.present_factions>=0 && candidate.present_factions<=7
				&& candidate.landmark_bucket>=0 && result.nearest_distance>=0 && result.nearest_distance<=9999,"Invalid decoded restart candidate");
			if(candidate.nearest) {Require(candidate.nearest->owner>=0 && candidate.nearest->owner<=7 && candidate.nearest->region<128,"Invalid nearest restart Base");}
			const auto distance=result.nearest_distance;
			if(candidate.elevation<3 || candidate.fungus || candidate.rockiness>1) {outcome=Outcome::Terrain;}
			else if(distance<int64_t(12)*input.scale/56) {outcome=Outcome::MinimumDistance;}
			else if(candidate.elevation>=5) {outcome=Outcome::Altitude;}
			else if(candidate.occupied && candidate.valid_occupant) {outcome=Outcome::Occupied;}
			else if(y<2) {outcome=Outcome::NorthMargin;}
			else if(y>=height-2) {outcome=Outcome::SouthMargin;}
			else if(candidate.wet_neighbors<12-k/80) {outcome=Outcome::WetNeighborhood;}
			else if(distance<int64_t(16-k/64)*input.scale/56) {outcome=Outcome::RelaxingDistance;}
			else if(int64_t(candidate.fertility)*(input.ocean+1)/2<8*(6-input.difficulty)+24-k/16-input.turn/16) {outcome=Outcome::Fertility;}
			else if(candidate.area<48*(5-input.difficulty)-k/4-input.turn/16) {outcome=Outcome::Area;}
			else if(candidate.present_factions && candidate.area/(candidate.present_factions+1)<64-k/64) {outcome=Outcome::Presence;}
			else {
				rating(point);++result.rating_reads;
				if(!candidate.landmark_bucket || candidate.landmark_bucket<(1024-k)/128) {outcome=Outcome::Landmark;}
				else if(candidate.territory_owner>0) {outcome=Outcome::Claimed;}
				else if(candidate.nearest && candidate.nearest->owner==input.captor && candidate.nearest->region==candidate.region) {outcome=Outcome::CaptorRegion;}
				else {outcome=Outcome::Selected;result.site=point;}
			}
		}
		if(observe) {observe({result.pairs,point,result.random.Crt(),outcome});}
		if(outcome==Outcome::Limit || outcome==Outcome::Selected) {return result;}
	}
	throw std::logic_error("Restart search omitted its terminal candidate");
}
}
