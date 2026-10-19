#pragma once
#include "types/Buffer.h"
#include <cstdint>

namespace game::backend::faction {
// Port-owned continuation metadata, not the original executable's save layout.
// The owning Game must execute each phase's real work before completing it.
class SmacFactionTurn {
public:
    enum class Phase:int32_t {Global,WildlifeUpkeep,WildlifeCommand,OwnerPending,OwnerUpkeep,OwnerCommand,Tail};
    struct Ticket {
        int32_t turn;int64_t sequence;Phase phase;int32_t owner;
        bool operator==(const Ticket& other) const;
    };
    static constexpr size_t MAX_WIRE_BYTES=256;
    static SmacFactionTurn NewWorld(uint8_t actors,int32_t human_focus);
    Ticket Current() const {return {m_turn,m_sequence,m_phase,m_owner};}
    uint8_t Actors() const {return m_actors;}
    uint8_t Passed() const {return m_passed;}
    uint8_t UpkeepEntered() const {return m_upkeep;}
    int32_t Focus() const {return m_focus;}
    void Validate() const;
    bool operator==(const SmacFactionTurn& other) const;
    void BeginRound(Ticket expected);
    void CompleteWildlifeUpkeep(Ticket expected);
    void CompleteWildlifeCommand(Ticket expected,uint8_t active);
    void BeginOwnerUpkeep(Ticket expected,uint8_t active);
    void SkipInactiveOwner(Ticket expected,uint8_t active);
    void CompleteOwnerUpkeep(Ticket expected,uint8_t active);
    void CompleteOwnerCommand(Ticket expected,uint8_t active);
    void CompleteRound(Ticket expected);
    types::Buffer Serialize() const;
    static SmacFactionTurn Deserialize(const std::string& wire);
private:
    int32_t m_turn=0,m_owner=0,m_focus=0;
    int64_t m_sequence=0;
    Phase m_phase=Phase::Global;
    uint8_t m_actors=0,m_passed=0,m_upkeep=0;
    void Require(Ticket expected,Phase phase) const;
    void RequireActive(uint8_t active) const;
    void NextOwner(int32_t after,uint8_t active);
    void Commit(SmacFactionTurn next);
};
}
