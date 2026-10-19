#include "SmacFactionTurn.h"
#include <climits>
#include <stdexcept>

namespace game::backend::faction {
namespace {
void Check(bool condition,const char* message) {if(!condition) {throw std::invalid_argument(message);}}
int32_t Int32(types::Buffer& b) {const auto v=b.ReadInt();Check(v>=INT32_MIN && v<=INT32_MAX,"Faction cursor integer exceeds signed32");return static_cast<int32_t>(v);}
uint8_t Mask(types::Buffer& b) {const auto v=b.ReadInt();Check(v>=0 && v<=255,"Faction cursor mask exceeds byte");return static_cast<uint8_t>(v);}
}
bool SmacFactionTurn::Ticket::operator==(const Ticket& other) const {return turn==other.turn && sequence==other.sequence && phase==other.phase && owner==other.owner;}
SmacFactionTurn SmacFactionTurn::NewWorld(uint8_t actors,int32_t human_focus) {
    SmacFactionTurn result;result.m_actors=actors;result.m_focus=human_focus;result.Validate();return result;
}
bool SmacFactionTurn::operator==(const SmacFactionTurn& other) const {
    return Current()==other.Current() && m_focus==other.m_focus && m_actors==other.m_actors && m_passed==other.m_passed && m_upkeep==other.m_upkeep;
}
void SmacFactionTurn::Validate() const {
    Check(m_turn>=0 && m_sequence>=0 && m_actors && !(m_actors&1) && m_focus>=1 && m_focus<=7
        && (m_actors&(1u<<m_focus)),"Faction cursor lacks admitted actors or human focus");
    Check(!(m_passed&~m_actors) && !(m_upkeep&~m_actors),"Faction cursor history includes an unbound actor");
    Check(m_phase>=Phase::Global && m_phase<=Phase::Tail,"Unknown faction cursor phase");
    if(!m_turn) {
        Check(m_phase==Phase::Global && !m_owner && !m_sequence && !m_passed && !m_upkeep,"Unstarted faction cursor carries processed state");return;
    }
    Check(m_sequence>0,"Started faction cursor lacks a transition sequence");
    if(m_phase==Phase::OwnerPending || m_phase==Phase::OwnerUpkeep || m_phase==Phase::OwnerCommand) {
        Check(m_owner>=1 && m_owner<=7 && (m_actors&(1u<<m_owner)),"Faction cursor scheduled owner is unbound");
        const auto prior=uint8_t(m_actors&((1u<<m_owner)-1));
        Check(m_passed==prior,"Faction cursor passed-owner order is inconsistent");
        const auto current=uint8_t(1u<<m_owner);
        Check(!(m_upkeep&~uint8_t(prior|current)) && bool(m_upkeep&current)==(m_phase!=Phase::OwnerPending),"Faction cursor upkeep history disagrees with its phase");
    } else {
        Check(!m_owner,"Non-owner faction phase has a scheduled positive owner");
        if(m_phase==Phase::WildlifeUpkeep || m_phase==Phase::WildlifeCommand) {Check(!m_passed && !m_upkeep,"Wildlife phase carries premature faction upkeep");}
        else {Check(m_passed==m_actors && !(m_upkeep&~m_passed),"Faction round tail has unfinished traversal");}
    }
}
void SmacFactionTurn::Require(Ticket expected,Phase phase) const {
    Validate();Check(Current()==expected && m_phase==phase,"Stale or out-of-phase faction transition");
    Check(m_sequence<INT64_MAX,"Faction transition sequence exhausted");
}
void SmacFactionTurn::RequireActive(uint8_t active) const {Check(!(active&~m_actors),"Active faction is outside the bound cursor roster");}
void SmacFactionTurn::Commit(SmacFactionTurn next) {++next.m_sequence;next.Validate();*this=next;}
void SmacFactionTurn::NextOwner(int32_t after,uint8_t active) {
    for(int32_t owner=after+1;owner<=7;++owner) {
        const auto bit=uint8_t(1u<<owner);if(!(m_actors&bit)) {continue;}
        if(active&bit) {m_owner=owner;m_phase=Phase::OwnerPending;return;}
        m_passed|=bit; // retired actors do not receive an invented upkeep
    }
    m_owner=0;m_phase=Phase::Tail;
}
void SmacFactionTurn::BeginRound(Ticket expected) {
    Require(expected,Phase::Global);Check(m_turn<INT32_MAX,"Faction round exceeds signed32 turn");
    auto next=*this;++next.m_turn;next.m_passed=next.m_upkeep=0;next.m_phase=Phase::WildlifeUpkeep;Commit(next);
}
void SmacFactionTurn::CompleteWildlifeUpkeep(Ticket expected) {
    Require(expected,Phase::WildlifeUpkeep);auto next=*this;next.m_phase=Phase::WildlifeCommand;Commit(next);
}
void SmacFactionTurn::CompleteWildlifeCommand(Ticket expected,uint8_t active) {
    Require(expected,Phase::WildlifeCommand);RequireActive(active);auto next=*this;next.NextOwner(0,active);Commit(next);
}
void SmacFactionTurn::BeginOwnerUpkeep(Ticket expected,uint8_t active) {
    Require(expected,Phase::OwnerPending);RequireActive(active);Check(active&(1u<<m_owner),"Inactive owner requires explicit retirement");
    auto next=*this;next.m_upkeep|=uint8_t(1u<<m_owner);next.m_phase=Phase::OwnerUpkeep;Commit(next);
}
void SmacFactionTurn::SkipInactiveOwner(Ticket expected,uint8_t active) {
    Require(expected,Phase::OwnerPending);RequireActive(active);Check(!(active&(1u<<m_owner)),"Cannot skip an active owner's upkeep");
    auto next=*this;next.m_passed|=uint8_t(1u<<m_owner);next.NextOwner(m_owner,active);Commit(next);
}
void SmacFactionTurn::CompleteOwnerUpkeep(Ticket expected,uint8_t active) {
    Require(expected,Phase::OwnerUpkeep);RequireActive(active);auto next=*this;
    if(active&(1u<<m_owner)) {next.m_phase=Phase::OwnerCommand;}
    else {next.m_passed|=uint8_t(1u<<m_owner);next.NextOwner(m_owner,active);}
    Commit(next);
}
void SmacFactionTurn::CompleteOwnerCommand(Ticket expected,uint8_t active) {
    Require(expected,Phase::OwnerCommand);RequireActive(active);auto next=*this;
    next.m_passed|=uint8_t(1u<<m_owner);next.NextOwner(m_owner,active);Commit(next);
}
void SmacFactionTurn::CompleteRound(Ticket expected) {
    Require(expected,Phase::Tail);auto next=*this;next.m_phase=Phase::Global;Commit(next);
}
types::Buffer SmacFactionTurn::Serialize() const {
    Validate();types::Buffer b;b.WriteInt(0x534d4654);b.WriteInt(1);b.WriteInt(m_turn);b.WriteInt(m_sequence);
    b.WriteInt(static_cast<int32_t>(m_phase));b.WriteInt(m_actors);b.WriteInt(m_focus);b.WriteInt(m_owner);b.WriteInt(m_passed);b.WriteInt(m_upkeep);
    Check(b.lenw<=MAX_WIRE_BYTES,"Faction cursor wire budget exceeded");return b;
}
SmacFactionTurn SmacFactionTurn::Deserialize(const std::string& wire) {
    Check(wire.size()<=MAX_WIRE_BYTES,"Faction cursor wire budget exceeded");types::Buffer b(wire);
    Check(b.ReadInt()==0x534d4654 && b.ReadInt()==1,"Unsupported faction cursor record");SmacFactionTurn result;
    result.m_turn=Int32(b);result.m_sequence=b.ReadInt();result.m_phase=static_cast<Phase>(Int32(b));result.m_actors=Mask(b);
    result.m_focus=Int32(b);result.m_owner=Int32(b);result.m_passed=Mask(b);result.m_upkeep=Mask(b);
    Check(b.lenr==b.lenw,"Unexpected faction cursor trailing fields");result.Validate();return result;
}
}
