#pragma once
#include <array>
#include <cstdint>
#include <cstddef>
#include <vector>
#include <functional>
#include <optional>
namespace game::backend::base {
struct SmacCitizenCounts {
    int32_t talents=0,drones=0,extra_drone_steps=0;
    bool operator==(const SmacCitizenCounts& b) const {return talents==b.talents && drones==b.drones && extra_drone_steps==b.extra_drone_steps;}
};
struct SmacCitizenPrefix {int32_t content_allowance,ordinary,assimilation,after_faction,faction_talents;};
SmacCitizenPrefix CalculateSmacCitizenPrefix(int32_t population,bool human,int32_t difficulty,int32_t drone_parameter,int32_t talent_parameter,uint8_t assimilation);
struct SmacBureaucracyInput {int32_t interval,drones;};
SmacBureaucracyInput CalculateSmacCitizenBureaucracy(int32_t population,bool human,int32_t difficulty,int32_t efficiency,int32_t width,int32_t height,int32_t owner_bases,int32_t global_position);
struct SmacCitizenFacilityInputs {int32_t drone_adjustment,paradise_talents,project_talents;};
SmacCitizenFacilityInputs CalculateSmacCitizenFacilityInputs(int32_t population,const std::array<bool,34>& facilities,int32_t genejack_rule,bool virtual_world,bool transit,bool genome,bool immortality);
struct SmacCitizenConversion {
    int32_t population,excluded,ordinary,bureaucracy,faction_talents,social_talent,psych,facility_drones,paradise_talents;
};
struct SmacCitizenEarlyStages {SmacCitizenCounts initial,psych,facilities;};
SmacCitizenEarlyStages ConvertSmacCitizens(const SmacCitizenConversion&);
struct SmacCitizenPoliceUnit {uint32_t package_mode;bool nonlethal;};
struct SmacCitizenPoliceStages {SmacCitizenCounts garrison,pacifism,normalized;};
SmacCitizenPoliceStages PoliceSmacCitizens(SmacCitizenCounts,int32_t population,int32_t excluded,int32_t police,int32_t pacifism,bool self_aware,const std::vector<SmacCitizenPoliceUnit>&);
struct SmacCitizenFinalStages {SmacCitizenCounts projects,suppressed,final;};
SmacCitizenFinalStages FinishSmacCitizens(SmacCitizenCounts,int32_t population,int32_t excluded,int32_t project_talents,bool longevity,int32_t economics,bool sphere,uint8_t nerve);
// A retained resource-pass result. Current views calculate a separate value;
// decoding this record never refreshes population, Psych or conquest history.
struct SmacCitizenCalculation {
    int32_t population,excluded,bureaucracy,social_talent,psych_points,police,facility_drones,paradise_talents;
    SmacCitizenPrefix prefix;
    SmacCitizenEarlyStages early;
    SmacCitizenCounts after_police,final;
    bool operator==(const SmacCitizenCalculation&) const;
};
struct SmacCitizenStatus {
    bool admitted,golden,riots,riot_dispatch,riot_new,golden_start,golden_over,riots_over,map_refresh;
};
SmacCitizenStatus CalculateSmacCitizenStatus(int32_t turn,int32_t population,SmacCitizenCounts,bool previous_golden,bool previous_riots,bool matrix,uint8_t nerve);
struct SmacRiotFacilityResult {
    int32_t draws=0,attempts=0;
    std::optional<int32_t> removed;
    bool exhausted=false;
};
// Only the removal search; exhausted=true is an explicit later consequence
// boundary. A caller must resolve that boundary before publishing state.
SmacRiotFacilityResult CalculateSmacRiotFacilityLoss(bool ongoing,int32_t police,int32_t population,
    int32_t owner_bases,uint8_t assimilation,int32_t difficulty,
    const std::array<bool,39>& facilities,const std::array<bool,39>& explicit_grants,
    const std::function<int32_t()>& retained_rank,const std::function<int32_t()>& draw);

struct SmacRiotRelationship {bool pact=false,treaty=false,half_distance=false;};
struct SmacRiotBase {size_t record_position;int32_t owner,x,y;};
struct SmacRiotCandidateScore {size_t record_position;int32_t owner;std::optional<int32_t> weight_owner;int32_t score;};
struct SmacRiotRecipient {
    std::optional<int32_t> selected;
    std::vector<SmacRiotCandidateScore> candidates;
    bool TransferAdmitted() const {return selected && *selected>0;}
};
SmacRiotRecipient CalculateSmacRiotRecipient(int32_t police,bool human,int32_t owner,int32_t former_owner,
    uint32_t activity_stamp,int32_t x,int32_t y,int32_t width,bool horizontal_wrap,
    const std::vector<SmacRiotBase>&,const std::array<SmacRiotRelationship,8>&);

}
