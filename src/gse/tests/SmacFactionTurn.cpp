#include "SmacFactionTurn.h"
#include "game/backend/faction/SmacFactionTurn.h"
#include "game/backend/faction/SmacSocialState.h"
#include "fixtures/SmacFactionStartupSource.h"
#include "task/gsetests/GSETests.h"
#include <yaml-cpp/yaml.h>
#include <functional>

namespace gse::tests {
namespace {
using Turn=game::backend::faction::SmacFactionTurn;
using Phase=Turn::Phase;
bool Rejects(const std::function<void()>& f) {try {f();}catch(const std::exception&){return true;}return false;}
Turn Saved(const Turn& state) {return Turn::Deserialize(state.Serialize().ToString());}
std::string Wire(const Turn& state,size_t changed,int64_t value) {
    types::Buffer b;const auto t=state.Current();const int64_t words[]{0x534d4654,1,t.turn,t.sequence,static_cast<int32_t>(t.phase),state.Actors(),state.Focus(),t.owner,state.Passed(),state.UpkeepEntered()};
    for(size_t i=0;i<10;++i) {b.WriteInt(i==changed ? value:words[i]);}return b.ToString();
}
}
void AddSmacFactionTurnTests(task::gsetests::GSETests* task) {
    task->AddTest("SMAC faction cursor matches original first-round dispatch fields and social timing",GT(){
        namespace faction=game::backend::faction;namespace fixture=smac_faction_startup_fixture;
        const auto native=YAML::LoadFile("docs/behavior/seven-first-upkeep-authority-cases.json");
        auto turn=Turn::NewWorld(uint8_t(native["before"]["active"].as<uint8_t>()&0xfe),native["before"]["focus"].as<int32_t>());
        const auto tech=fixture::Technologies();const auto profiles=loader::txt::FactionTXTLoader::ParseStartupRules(fixture::Alpha(),fixture::Source,tech);
        std::vector<faction::SmacSimulationAdmission> admissions;
        for(int32_t owner=1;owner<=7;++owner) {admissions.push_back({owner,game::backend::settings::SmacFactionStartupRules::Bindings()[owner-1].port_id});}
        auto social=faction::SmacSocialState::NewWorld(admissions,tech,profiles);
        for(int32_t owner=1;owner<=7;++owner) {social.BuildStartupIntrinsic(owner,tech,profiles);}
        turn=Saved(turn);turn.BeginRound(turn.Current());turn=Saved(turn);
        GT_ASSERT(turn.Current().phase==Phase::WildlifeUpkeep && turn.Current().owner==0 && turn.Current().turn==1,"Global step skipped wildlife or advanced the wrong year");
        turn.CompleteWildlifeUpkeep(turn.Current());turn=Saved(turn);turn.CompleteWildlifeCommand(turn.Current(),0xfe);
        for(int32_t owner=1;owner<=7;++owner) {
            const auto before=turn.Current();
            GT_ASSERT(before.phase==Phase::OwnerPending && before.owner==owner,"Faction cursor skipped native owner order");
            turn.BeginOwnerUpkeep(before,0xfe);social.RefreshOwnerForFirstCommand(owner,tech,profiles);
            turn=Saved(turn);turn.CompleteOwnerUpkeep(turn.Current(),0xfe);turn=Saved(turn);
            const auto expected=native["command_boundaries"][owner-1]["state"];
            GT_ASSERT(turn.Current().phase==Phase::OwnerCommand && turn.Current().owner==expected["scheduled"].as<int32_t>()
                && turn.Current().turn==expected["turn"].as<int32_t>() && turn.Focus()==expected["focus"].as<int32_t>()
                && turn.UpkeepEntered()==uint8_t((1u<<(owner+1))-2),"Native dispatch cursor or focused human changed");
            for(int32_t peer=1;peer<=7;++peer) {for(size_t axis=0;axis<11;++axis) {
                GT_ASSERT(social.Find(peer)->current[axis]==expected["factions"][peer-1]["social_current"][axis].as<int32_t>(),"Cursor refreshed a faction ahead of native upkeep");
            }}
            const auto ready=turn;GT_ASSERT(Rejects([&]{turn.BeginOwnerUpkeep(before,0xfe);}) && turn==ready,"Restored owner entry replayed upkeep");
            turn.CompleteOwnerCommand(turn.Current(),0xfe);turn=Saved(turn);
        }
        GT_ASSERT(turn.Current().phase==Phase::Tail && turn.Current().turn==1 && turn.Passed()==0xfe,"Last command advanced year before global tail");
        turn.CompleteRound(turn.Current());turn=Saved(turn);turn.BeginRound(turn.Current());
        GT_ASSERT(turn.Current().turn==2 && turn.Current().phase==Phase::WildlifeUpkeep && !turn.Passed() && !turn.UpkeepEntered(),"Next round retained prior upkeep or advanced twice");GT_OK();
    });
    task->AddTest("SMAC faction cursor retires inactive owners without fabricated upkeep or rewinding revived slots",GT(){
        constexpr uint8_t actors=(1u<<1)|(1u<<3)|(1u<<7);
        auto turn=Turn::NewWorld(actors,7);turn.BeginRound(turn.Current());turn.CompleteWildlifeUpkeep(turn.Current());turn.CompleteWildlifeCommand(turn.Current(),actors);
        const auto pending=turn;
        GT_ASSERT(Rejects([&]{turn.SkipInactiveOwner(turn.Current(),actors);}) && turn==pending,"Active owner was silently skipped");
        turn.SkipInactiveOwner(turn.Current(),uint8_t(actors&~(1u<<1)));
        GT_ASSERT(turn.Current().owner==3 && turn.Passed()==2 && !turn.UpkeepEntered(),"Retired owner received upkeep or lost its traversal position");
        turn.BeginOwnerUpkeep(turn.Current(),actors);turn.CompleteOwnerUpkeep(turn.Current(),uint8_t(1u<<7));
        GT_ASSERT(turn.Current().owner==7 && turn.Current().phase==Phase::OwnerPending && turn.UpkeepEntered()==8,"Eliminated owner was given a command phase");
        turn.BeginOwnerUpkeep(turn.Current(),actors);turn.CompleteOwnerUpkeep(turn.Current(),actors);turn.CompleteOwnerCommand(turn.Current(),actors);
        GT_ASSERT(turn.Current().phase==Phase::Tail && turn.Passed()==actors && turn.Focus()==7,"Revived earlier owner rewound the same round");GT_OK();
    });
    task->AddTest("SMAC faction cursor rejects stale forged and out-of-phase transitions atomically",GT(){
        auto turn=Turn::NewWorld(2,1);const auto initial=turn;
        GT_ASSERT(Rejects([&]{turn.CompleteOwnerCommand(turn.Current(),2);}) && turn==initial,"Command completed before the initial global pass");
        const auto stale=turn.Current();turn.BeginRound(stale);const auto running=turn;
        GT_ASSERT(Rejects([&]{turn.BeginRound(stale);}) && turn==running,"Duplicate global entry advanced twice");
        auto forged=turn.Current();++forged.sequence;
        GT_ASSERT(Rejects([&]{turn.CompleteWildlifeUpkeep(forged);}) && turn==running,"Forged sequence acquired transition authority");
        turn.CompleteWildlifeUpkeep(turn.Current());const auto wildlife=turn;
        GT_ASSERT(Rejects([&]{turn.CompleteWildlifeCommand(turn.Current(),6);}) && turn==wildlife,"Foreign active owner entered the cursor roster");
        turn.CompleteWildlifeCommand(turn.Current(),2);const auto pending=turn;
        GT_ASSERT(Rejects([&]{turn.BeginOwnerUpkeep(turn.Current(),0);}) && turn==pending,"Inactive owner began upkeep");
        turn.BeginOwnerUpkeep(turn.Current(),2);const auto upkeep=turn;
        GT_ASSERT(Rejects([&]{turn.CompleteOwnerCommand(turn.Current(),2);}) && turn==upkeep,"Command completion bypassed unfinished upkeep");GT_OK();
    });
    task->AddTest("SMAC faction cursor codec validates bounds phase history and exact restoration",GT(){
        auto turn=Turn::NewWorld(0xfe,1);turn.BeginRound(turn.Current());turn.CompleteWildlifeUpkeep(turn.Current());turn.CompleteWildlifeCommand(turn.Current(),0xfe);turn.BeginOwnerUpkeep(turn.Current(),0xfe);
        const auto wire=turn.Serialize().ToString();GT_ASSERT(Wire(turn,SIZE_MAX,0)==wire && Saved(turn)==turn,"Cursor codec changed exact owned state");
        for(size_t n=0;n<wire.size();++n) {GT_ASSERT(Rejects([&]{(void)Turn::Deserialize(wire.substr(0,n));}),"Truncated cursor restored");}
        GT_ASSERT(Rejects([&]{(void)Turn::Deserialize(wire+"x");}) && Rejects([&]{(void)Turn::Deserialize(std::string(Turn::MAX_WIRE_BYTES+1,'x'));}),"Extra cursor fields restored");
        for(const auto bad:std::vector<std::pair<size_t,int64_t>>{{0,0},{1,0},{2,-1},{2,int64_t(INT32_MAX)+1},{3,-1},{3,0},{4,7},{5,255},{5,256},{6,0},{6,8},{7,8},{8,2},{9,0},{9,6}}) {
            GT_ASSERT(Rejects([&]{(void)Turn::Deserialize(Wire(turn,bad.first,bad.second));}),"Malformed cursor phase or narrowed field restored");
        }
        auto exhausted=Turn::Deserialize(Wire(turn,3,INT64_MAX));const auto before=exhausted;
        GT_ASSERT(Rejects([&]{exhausted.CompleteOwnerUpkeep(exhausted.Current(),0xfe);}) && exhausted==before,"Cursor sequence overflow mutated state");
        for(const auto actors:{0,1,3}) {GT_ASSERT(Rejects([&]{(void)Turn::NewWorld(uint8_t(actors),1);}),"Invalid initial actor mask admitted");}GT_OK();
    });
}
}
