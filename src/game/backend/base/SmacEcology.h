#pragma once

#include "SmacWorkedSquareCollection.h"

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

}
