#include "SmacDominanceHistory.h"
#include "ConventionalUnits.h"
#include "fixtures/SmacStartupSource.h"
#include "task/gsetests/GSETests.h"
#include "game/backend/Game.h"
#include "game/backend/faction/SmacDominanceHistory.h"
#include "game/backend/faction/SmacRankingScore.h"
#include "game/backend/map/Map.h"
#include "game/backend/base/BaseManager.h"
#include "game/backend/base/PopDef.h"

namespace gse::tests {
namespace {
using namespace game::backend;
using History=faction::SmacDominanceHistory;
struct WriterCase {const char* id;size_t turn;std::array<int32_t,8> scores;std::array<int16_t,8> row;uint64_t payload_fnv;};
#include "fixtures/SmacDominanceWriterCases.inc"
uint64_t Fnv(const std::string& bytes) {uint64_t hash=UINT64_C(14695981039346656037);for(const unsigned char byte:bytes) {hash=(hash^byte)*UINT64_C(1099511628211);}return hash;}
}
void AddSmacDominanceHistoryTests(task::gsetests::GSETests* task) {
    task->AddTest("Dominance history native nine writer contexts preserve signed shorts slot0 and cap",GT(){
        History history;
        GT_ASSERT(history.Serialize()==std::string(16000,'\0'),"Fresh history is not the full cleared native block");
        for(const auto& c:OriginalWriterCases) {
            const auto before=history.Serialize();history.Publish(c.turn,c.scores);
            if(c.turn<History::ROWS) {GT_ASSERT(history.samples[c.turn]==c.row,"Native current row differs: "+std::string(c.id));}
            else {GT_ASSERT(history.Serialize()==before,"Turn1000 rewrote retained history");}
            GT_ASSERT(Fnv(history.Serialize())==c.payload_fnv,"Native full history payload differs: "+std::string(c.id));
        }
        history.samples[1][0]=-1234;const auto past=history.samples[1];
        history.Publish(999,{99,1,2,3,4,5,6,7});
        GT_ASSERT(history.samples[1]==past,"Late publication rewrote old history or slot0");
        std::string raw(16000,'\0');for(size_t i=0;i<raw.size();++i) {raw[i]=static_cast<char>((i*37+11)&255);}
        GT_ASSERT(History::Deserialize(raw).Serialize()==raw,"Native raw payload lost signed/slot0/future bits");
        bool rejected=false;try {History::Deserialize(raw.substr(1));}catch(const std::invalid_argument&) {rejected=true;}
        GT_ASSERT(rejected,"Truncated history accepted");GT_OK();
    });
    task->AddTest("Dominance history actual simulation codec preserves full payload and V30 absence",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            const auto& settings=fixture.Game()->GetState()->m_settings.global;
            bool exact=false;bool fresh=false;
            fixture.ResourceState([&](auto& state) {
                fresh=state.dominance_history && state.dominance_history->Serialize()==std::string(16000,'\0');
                state.dominance_history.emplace();
                for(size_t turn=0;turn<1000;++turn) {for(size_t owner=0;owner<8;++owner) {state.dominance_history->samples[turn][owner]=static_cast<int16_t>(int(turn*8+owner)-4000);}}
                const auto wire=state.Serialize(*settings.GetSmacTechnologyRules(),*settings.GetSmacFactionStartupRules(),*settings.GetSmacPredefinedUnitRules()).ToString();
                const auto restore=[&](const std::string& bytes) {return faction::SmacSimulationState::Deserialize(bytes,*settings.GetSmacTechnologyRules(),*settings.GetSmacFactionStartupRules(),*settings.GetSmacPredefinedUnitRules());};
                const auto restored=restore(wire);exact=restored==state;
                types::Buffer header(wire);const auto magic=header.ReadInt();(void)header.ReadInt();
                types::Buffer old_header;old_header.WriteInt(magic);old_header.WriteInt(30);
                types::Buffer tail;tail.WriteBool(true);tail.WriteString(state.dominance_history->Serialize());
                tail.WriteBool(state.dominance_native_keys.has_value());if(state.dominance_native_keys) {for(const auto& key:*state.dominance_native_keys) {tail.WriteString(key);}}
                tail.WriteBool(state.conquest_winner.has_value());if(state.conquest_winner) {tail.WriteInt(*state.conquest_winner);}
                tail.WriteBool(state.faction_turn.has_value());if(state.faction_turn) {tail.WriteString(state.faction_turn->Serialize().ToString());}
                if(state.strategy) {for(const auto& entry:*state.strategy) {
                    tail.WriteBool(entry.second.fight.has_value());if(entry.second.fight) {tail.WriteInt(*entry.second.fight);}
                }}
                for(const auto& summary:state.air_summary) {
                    tail.WriteBool(summary.has_value());if(summary) {
                        tail.WriteInt(summary->foreign_threats);tail.WriteInt(summary->own_plan0);tail.WriteInt(summary->own_plan4);
                    }
                }
                const auto old=old_header.ToString()+wire.substr(header.lenr,wire.size()-header.lenr-tail.lenw);
                const auto legacy=restore(old);bool missing_fight=legacy.strategy.has_value();
                if(legacy.strategy) {for(const auto& entry:*legacy.strategy) {missing_fight=missing_fight && !entry.second.fight;}}
                bool missing_air=true;for(const auto& summary:legacy.air_summary) {missing_air=missing_air && !summary;}
                exact=exact && !legacy.dominance_history && missing_fight && missing_air;
                bool rejected=false;try {restore(wire.substr(0,wire.size()-1));}catch(const std::exception&) {rejected=true;}
                exact=exact && rejected;
            });
            GT_ASSERT(fresh,"Actual fresh simulation did not clear the complete native history block");
            GT_ASSERT(exact,"Actual simulation codec lost history, accepted truncation or invented migrated samples");
            const auto saved=fixture.Snapshot();fixture.Restore(GSE_CALL,saved);
            GT_ASSERT(fixture.Snapshot()==saved,"World restore changed retained history");GT_OK();
        });
    });
    task->AddTest("Dominance history actual turn provider samples before increment and consumed scores",GT(){
        ConventionalUnitFixture fixture;fixture.ConfigureProduction(smac_startup_fixture::OrdinarySource,"GAIANS");
        return fixture.Run([&](GSE_CALLABLE)->std::string {
            auto* game=fixture.Game();const auto& rules=game->GetState()->m_settings.global;
            auto* world=game->GetMap();
            map::SmacWorkerKnowledge::Data knowledge{int32_t(world->GetWidth()),int32_t(world->GetHeight()),true,true,0,{},{},{}};
            knowledge.observers[1]=map::SmacWorkerKnowledge::Observer{};
            knowledge.tiles.resize(world->GetWidth()*world->GetHeight()/2,{2,std::nullopt});
            knowledge.remembered[1].resize(knowledge.tiles.size());
            game->PublishSmacWorkerKnowledge(GSE_CALL,map::SmacWorkerKnowledge::Create(knowledge));
            std::array<std::string,8> roles;roles[0]="history-worker";
            game->GetBM()->DefinePop(new base::PopDef(roles[0],"Worker",{},{},base::PopDef::PF_TILE_WORKER));
            for(uint8_t i=0;i<7;++i) {roles[i+1]="history-specialist"+std::to_string(i);game->GetBM()->DefinePop(new base::PopDef(roles[i+1],"Specialist",{},{},base::PopDef::PF_NONE,i));}
            fixture.BasePresentationRoles(roles);
            std::array<int32_t,8> scores{};
            fixture.ResourceState([&](auto& state) {
                state.dominance_history.emplace();state.dominance_history->samples[0][0]=-77;
                state.ranking_scores.emplace();state.ranking_scores->at(0)=1234;
                for(const auto& owner:state.roster) {
                    auto& economy=state.economy.at(owner.simulation_id);economy.population_aggregate=17;economy.design_counts.emplace();
                    if(owner.active) {scores[owner.simulation_id]=faction::CalculateSmacOrdinaryRankingScore(*state.technology.Find(owner.simulation_id),17,*economy.design_counts,*rules.GetSmacTechnologyRules(),rules.GetSmacPredefinedUnitRules()->catalog,state.designs);}
                }
            });
            const auto turn=game->GetTurnId();game->Event(GSE_CALL,"complete_turn",{});fixture.DrainEvents();bool exact=false;
            fixture.ResourceState([&](auto& state) {
                exact=game->GetTurnId()==turn+1 && state.dominance_history && state.ranking_scores && state.ranking_scores->at(0)==1234;
                for(size_t owner=1;owner<8;++owner) {exact=exact && state.dominance_history->samples[turn][owner]==scores[owner] && state.ranking_scores->at(owner)==-1;}
                exact=exact && state.dominance_history->samples[0][0]==-77 && state.dominance_history->samples[turn+1][1]==0;
            });
            GT_ASSERT(exact,"Real turn provider used consumed scores, wrote incoming turn or changed slot0");GT_OK();
        });
    });
}
}
