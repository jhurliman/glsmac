#pragma once

#include "SmacWorkedSquareCollection.h"
#include <vector>

namespace game::backend::base {

// Semantic improvements, not the original packed map word or GLSMAC's flags.
struct SmacEcologySquare {
    enum Improvement : uint32_t {
        Road=1u, MagTube=2u, Mine=4u, Solar=8u, Farm=16u, SoilEnricher=32u,
        Condenser=64u, EchelonMirror=128u, Borehole=256u, Forest=512u
    };
    uint32_t improvements;
    bool worked; // Explicit selection bit, including the center.
};
struct SmacTerraformingEcology { int32_t raw_points, scaled; };
// Queries each valid radius square, whether worked, explored, owned or land.
// Widths below8 may alias radius positions and are outside measured coverage.
SmacTerraformingEcology CalculateSmacTerraformingEcology(const SmacWorkerRadiusMap& map,
    bool tree_farm,bool hybrid_forest,const std::function<SmacEcologySquare(int32_t,int32_t)>& query);

struct SmacEcologyBurdenInput {
    int32_t history,raw_terraforming_points,processed_minerals,population,mining_satellites;
    bool nanoreplicator,centauri_preserve,temple_of_planet,singularity_inductor,pholus_mutagen;
};
struct SmacEcologyBurden {
    int32_t scaled_terraforming,residual_terraforming,remaining_clean_allowance;
    int32_t divisor,mining_satellite_deduction,burden;
};
// Processed minerals are ordinary gross before support, outgoing convoys and
// late Space Elevator doubling. History is retained event state, not facilities.
SmacEcologyBurden CalculateSmacEcologyBurden(const SmacEcologyBurdenInput& input);

struct SmacEcologyScalingInput {
    bool human;
    int32_t human_difficulty,global_difficulty,major_atrocities,sea_level_offset,sea_level_rises_rule;
    int32_t native_life,technology_score,future_technology_counter,planet;
    bool voice_of_planet_exists,transcendence_victory,perihelion;
};
struct SmacEcologyDamage {
    int32_t after_human_term,after_global_doublings,difficulty_factor,after_division_by_six,damage;
};
// Signed32 overflow outside the measured domain rejects instead of invoking
// C++ undefined behavior or inventing native overflow behavior. No RNG/state writes.
SmacEcologyDamage CalculateSmacEcologyDamage(const SmacEcologyBurden& burden,const SmacEcologyScalingInput& input);
// Added after this Base's research, before the ecology CRT draw. This helper
// neither resets/updates the retained aggregate nor performs the later event.
int32_t SmacEcologyResearchContribution(int32_t damage,int32_t current_planet);

struct SmacBloomTile {
    int32_t altitude_class;
    bool fungus;
    bool real_base; // Original Base item with affiliation nibble0..7.
    // Semantic Mine/Solar/Farm/SoilEnricher/Forest flags from SmacEcologySquare.
    uint32_t removable_improvements;
};
struct SmacBloomBase { int32_t id; std::array<int32_t,2> coordinate; };
struct SmacBloomSelection {
    std::array<int32_t,2> coordinate;
    int32_t ordinal,score; // Original one-based offset order, excluding center.
};
// Detached event admission and ordered selection. Draws use the existing CRT
// stream (raw0..32767), never the shared gameplay stream. Callers must stage
// RNG with terrain/history changes; these helpers do not publish live state.
// Threshold always consumes one draw, including zero damage and suppressed events.
bool AdmitSmacEcologyBloom(int32_t damage,bool perihelion,uint32_t primary_rules,
    uint32_t secondary_state,const std::function<int32_t()>& draw);
// Bases are in global record order; equal nearest distances choose the later
// record. The query receives valid parity coordinates only. No landmark test
// occurs here: a protected winner still increments history before mutation.
std::optional<SmacBloomSelection> SelectSmacEcologyBloom(const SmacWorkerRadiusMap& map,
    int32_t base_id,const std::vector<SmacBloomBase>& bases,
    const std::function<SmacBloomTile(int32_t,int32_t)>& query,const std::function<int32_t()>& draw);

// Original semantic terrain fields, not GLSMAC's differently packed tile flags.
// Other fields (contour, climate, region, visibility, remembered items, etc.)
// remain with the caller and must not be reconstructed from altitude_class.
struct SmacBloomTerrainCell {
    int32_t altitude_class,rockiness;
    uint32_t items,landmarks;
    uint8_t affiliation; // Low nibble only; upper affiliation bits are preserved externally.
    bool operator==(const SmacBloomTerrainCell& other) const noexcept {
        return altitude_class==other.altitude_class && rockiness==other.rockiness && items==other.items
            && landmarks==other.landmarks && affiliation==other.affiliation;
    }
};
struct SmacBloomTerrainDelta {
    std::array<int32_t,2> coordinate;
    SmacBloomTerrainCell before,after;
};
struct SmacBloomClassChange {SmacBloomTerrainCell cell;uint32_t native_dirty_mask;};
// Same land/sea-category class-only setter used by the target2 shaper.
// Cross-category conversion has additional effects and is a separate producer.
SmacBloomClassChange ChangeSmacBloomClassOnly(SmacBloomTerrainCell cell,int32_t altitude,bool nonzero_turn,
    const std::function<int32_t()>& draw);
// A completed producer result. Consumers can inspect/copy it, but cannot
// forge deltas or combine two direct blooms into one event publication.
class SmacBloomTerrainPlan {
public:
    SmacBloomTerrainPlan(const SmacBloomTerrainPlan&) = default;
    SmacBloomTerrainPlan(SmacBloomTerrainPlan&&) = default;
    SmacBloomTerrainPlan& operator=(const SmacBloomTerrainPlan&) = default;
    SmacBloomTerrainPlan& operator=(SmacBloomTerrainPlan&&) = default;
    int32_t History() const noexcept { return m_history; }
    int32_t NominalCellBudget() const noexcept { return m_nominal_cell_budget; }
    uint32_t NativeDirtyMask() const noexcept { return m_native_dirty_mask; }
    bool NonzeroTurn() const noexcept { return m_nonzero_turn; }
    const std::optional<std::array<int32_t,2>>& FungusCell() const noexcept { return m_fungus_cell; }
    const std::vector<SmacBloomTerrainDelta>& Changes() const noexcept { return m_changes; }
private:
    SmacBloomTerrainPlan(int32_t history,bool nonzero_turn) : m_history(history),m_nonzero_turn(nonzero_turn) {}
    int32_t m_history, m_nominal_cell_budget = 1;
    uint32_t m_native_dirty_mask = 0;
    std::optional<std::array<int32_t,2>> m_fungus_cell;
    std::vector<SmacBloomTerrainDelta> m_changes;
    bool m_nonzero_turn;
    friend SmacBloomTerrainPlan PrepareSmacBloomTerrain(const SmacWorkerRadiusMap&,int32_t,int32_t,bool,
        const std::function<SmacBloomTerrainCell(int32_t,int32_t)>&,const std::function<int32_t()>&);
};
// Call only after a winner is selected. map.center is the selected square.
// Stages H increment and complete class-only terrain changes, including the
// original loop-index overwrite after the first eligible direct fungus write.
// No live tile, contour/corner, status, warming or unit publication occurs.
SmacBloomTerrainPlan PrepareSmacBloomTerrain(const SmacWorkerRadiusMap& map,int32_t history,int32_t difficulty,bool nonzero_turn,
    const std::function<SmacBloomTerrainCell(int32_t,int32_t)>& query,const std::function<int32_t()>& draw);

}
