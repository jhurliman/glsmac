#include "SmacRestartPreprocessing.h"
#include "fixtures/SmacRestartPreprocessing.h"
#include "game/backend/map/SmacRestartSearch.h"
#include "task/gsetests/GSETests.h"
#include <stdexcept>
#include <algorithm>

namespace gse::tests {
namespace {
using namespace smac_restart_preprocessing_fixture;
namespace restart=game::backend::map::smac_restart;
uint8_t Detail(const Case& row,site::Coordinate point) {
	for(const auto& entry:row.details) {if(entry.first==point) {return entry.second;}}return 0;
}
}
void AddSmacRestartPreprocessingTests(task::gsetests::GSETests* task) {
	task->AddTest("SMAC restart preprocessing matches native scale and 23 ordered landmark scans",GT(){
		for(const auto& row:scales) {GT_ASSERT(restart::ScaleFromGeometry(row.geometry)==row.value,"native geometry scale differs");}
		size_t total=0;
		for(const auto& row:cases) {
			size_t reads=0;const auto bucket=restart::LandmarkBucket(row.geometry,row.center,[&](auto point) {
				if(reads>=row.reads.size() || point!=row.reads[reads]) {throw std::runtime_error(std::string(row.name)+" native Q read order differs at "+std::to_string(reads));}
				++reads;return Detail(row,point);
			});
			GT_ASSERT(bucket==row.bucket && reads==row.reads.size(),"native Q bucket or short circuit differs: "+std::string(row.name));total+=reads;
		}
		GT_ASSERT(total==2074 && cases.size()==23 && scales.size()==6,"native preprocessing coverage changed");GT_OK();
	});
	task->AddTest("SMAC restart search consumes original Q producers at eight native attempt thresholds",GT(){
		for(const auto& gate:gates) {
			const auto found=std::find_if(cases.begin(),cases.end(),[&](const auto& row){return std::string(row.name)==gate.producer;});
			GT_ASSERT(found!=cases.end(),"native Q gate lost its producer case");const auto& row=*found;
			const auto q=restart::LandmarkBucket(row.geometry,row.center,[&](auto point){return Detail(row,point);});
			size_t candidates=0,rating_reads=0;bool passed=false,observed=false;
			(void)restart::Search({row.geometry,3,1,0,3,restart::ScaleFromGeometry(row.geometry),0},game::backend::SmacGameplayRandomState(0,1),[&](auto) {
				restart::Candidate value;++candidates;
				// The native gate receipt supplies k after its real Q producer.
				// Earlier synthetic terrain rejections advance this loop to that k.
				value.elevation=candidates==size_t(gate.attempt) ? 3 : 0;value.region=1;
				value.area=1024;value.fertility=960;value.wet_neighbors=21;value.landmark_bucket=q;return value;
			},[&](auto){++rating_reads;},[&](const auto& attempt) {
				if(attempt.number==size_t(gate.attempt)) {
					observed=true;passed=attempt.outcome==restart::Outcome::Selected;
					if(!passed && attempt.outcome!=restart::Outcome::Landmark) {throw std::runtime_error("native Q discriminator failed an earlier gate");}
				}
			});
			GT_ASSERT(observed && passed==gate.passed && rating_reads==1,"native Q threshold or rating-stage order differs");
		}GT_ASSERT(gates.size()==8,"native Q gate coverage changed");GT_OK();
	});
	task->AddTest("SMAC restart search reads native resident scale at three custom distance gates",GT(){
		for(const auto& row:scale_gates) {
			std::optional<restart::Outcome> first;
			(void)restart::Search({row.geometry,3,1,0,3,row.resident,0},game::backend::SmacGameplayRandomState(0,1),[&](auto) {
				restart::Candidate value;value.elevation=3;value.region=1;value.area=1024;value.fertility=960;value.wet_neighbors=21;
				value.landmark_bucket=32;value.nearest=restart::Base{2,1};value.nearest_distance=row.distance;return value;
			},[](auto){},[&](const auto& attempt){if(!first) {first=attempt.outcome;}});
			GT_ASSERT(first && (*first!=restart::Outcome::MinimumDistance)==row.passed,"custom restart gate derived scale from height or ignored resident S");
		}GT_ASSERT(scale_gates.size()==3,"native resident scale gate coverage changed");GT_OK();
	});
}
}
