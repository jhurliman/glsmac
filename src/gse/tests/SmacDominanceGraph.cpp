#include "SmacDominanceGraph.h"
#include "game/backend/faction/SmacDominanceGraph.h"
#include "task/gsetests/GSETests.h"

namespace gse::tests {
namespace {
using Graph=game::backend::faction::SmacDominanceGraph;
using History=game::backend::faction::SmacDominanceHistory;
struct GraphCase {
    const char* id;size_t turn;int multiplier;Graph::Rect rect;
    std::array<int32_t,8> scores;int step,divisor;size_t lines;uint64_t fnv;
};
#include "fixtures/SmacDominanceGraphCases.inc"
uint64_t Hash(const std::vector<Graph::Line>& lines) {
    uint64_t hash=UINT64_C(14695981039346656037);
    for(const auto& line:lines) {for(const int32_t field:{line.x1,line.y1,line.x2,line.y2,int32_t(line.color)}) {
        const auto word=static_cast<uint32_t>(field);
        for(int shift=0;shift<32;shift+=8) {hash=(hash^((word>>shift)&255))*UINT64_C(1099511628211);}
    }}
    return hash;
}
}
void AddSmacDominanceGraphTests(task::gsetests::GSETests* task) {
    task->AddTest("Dominance graph matches seventeen native builders and all6860 line requests",GT(){
        const std::array<std::string,8> names{"","FACTION1","FACTION2","FACTION3","FACTION4","SPARTANS","FACTION6","FACTION7"};
        const std::array<uint8_t,8> normal{249,111,252,255,251,65,102,187},alternate{1,175,4,248,3,57,109,192};
        size_t count=0;
        for(const auto& c:OriginalGraphCases) {
            History history;
            for(size_t turn=0;turn<1000;++turn) {
                history.samples[turn][0]=1234;
                for(size_t owner=1;owner<8;++owner) {history.samples[turn][owner]=static_cast<int16_t>(((turn*owner+17*owner)%300)*c.multiplier);}
            }
            history.Publish(c.turn,c.scores);
            const auto graph=Graph::Build(c.turn,c.rect,c.scores,history,names,normal,alternate);
            GT_ASSERT(graph.clip==c.rect && graph.horizontal_step==c.step && graph.vertical_divisor==c.divisor
                && graph.lines.size()==c.lines && Hash(graph.lines)==c.fnv,"Original graph line requests differ: "+std::string(c.id));
            count+=graph.lines.size();
        }
        GT_ASSERT(count==6860,"Native graph context coverage changed");GT_OK();
    });
    task->AddTest("Dominance graph refuses unclosed turn capacity and alternate columns",GT(){
        History history;std::array<int32_t,8> scores{};std::array<std::string,8> names{};std::array<uint8_t,8> colors{};
        for(const auto turn:{1000u,1001u}) {
            bool rejected=false;try {Graph::Build(turn,{501,76,721,162},scores,history,names,colors,colors);}catch(const std::invalid_argument&) {rejected=true;}
            GT_ASSERT(rejected,"High turn read adjoining history storage");
        }
        bool rejected=false;try {Graph::Build(2,{0,0,220,86},scores,history,names,colors,colors);}catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected,"Unmeasured graph-column geometry admitted");GT_OK();
    });
}
}
