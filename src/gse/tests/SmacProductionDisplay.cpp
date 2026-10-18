#include "SmacProductionDisplay.h"
#include "game/backend/base/SmacProductionDisplay.h"
#include "task/gsetests/GSETests.h"
namespace gse::tests {
void AddSmacProductionDisplayTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC production forecast14 literal current surplus labels and signed32 boundaries",GT(){
        using game::backend::base::FormatSmacProductionForecast;
        GT_ASSERT(FormatSmacProductionForecast(40,7,4)==std::optional<std::string>("TURNS:  9"),"native forecast ordinary");
        GT_ASSERT(FormatSmacProductionForecast(40,8,4)==std::optional<std::string>("TURNS:  8"),"native forecast exact_division");
        GT_ASSERT(FormatSmacProductionForecast(40,39,4)==std::optional<std::string>("TURNS:  1"),"native forecast one_missing");
        GT_ASSERT(FormatSmacProductionForecast(40,40,4)==std::optional<std::string>("TURNS:  0"),"native forecast funded");
        GT_ASSERT(FormatSmacProductionForecast(40,47,4)==std::optional<std::string>("TURNS:  0"),"native forecast overfunded");
        GT_ASSERT(FormatSmacProductionForecast(40,7,0)==std::optional<std::string>("No Minerals"),"native forecast zero");
        GT_ASSERT(FormatSmacProductionForecast(40,7,-2)==std::optional<std::string>("No Minerals"),"native forecast negative");
        GT_ASSERT(FormatSmacProductionForecast(40,40,0)==std::optional<std::string>("No Minerals"),"native forecast funded_zero");
        GT_ASSERT(FormatSmacProductionForecast(40,40,-2)==std::optional<std::string>("No Minerals"),"native forecast funded_negative");
        GT_ASSERT(FormatSmacProductionForecast(40,7,4)==std::optional<std::string>("TURNS:  9"),"native forecast riot");
        GT_ASSERT(FormatSmacProductionForecast(40,40,4)==std::optional<std::string>("TURNS:  0"),"native forecast riot_funded");
        GT_ASSERT(FormatSmacProductionForecast(200000,0,100000)==std::optional<std::string>("TURNS:  3"),"native forecast surplus_clamp");
        GT_ASSERT(FormatSmacProductionForecast(1000001,0,1)==std::optional<std::string>("TURNS:  999999"),"native forecast display_clamp");
        GT_ASSERT(FormatSmacProductionForecast(2147483647,0,2)==std::optional<std::string>("TURNS:  0"),"native forecast wrapped_numerator");
        GT_ASSERT(!FormatSmacProductionForecast(0,0,4) && !FormatSmacProductionForecast(0,0,0),"zero cost invented an ordinary forecast block");
        bool rejected=false;try{(void)FormatSmacProductionForecast(40,INT64_MAX,1);}catch(const std::invalid_argument&){rejected=true;}
        GT_ASSERT(rejected,"wide port store silently narrowed into original display");GT_OK();
    });
}
}
