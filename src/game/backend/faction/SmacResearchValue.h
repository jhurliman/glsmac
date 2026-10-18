#pragma once

#include <array>
#include <cstdint>

namespace game::backend::faction {

struct SmacResearchTechnology {
    std::array< int32_t, 4 > ratings; // Power/Tech/Wealth/Growth, installed file order
    std::array< int32_t, 2 > prerequisites; // -1 None, -2 Disable, otherwise technology ID
    uint32_t flags, possession;
};

// Resolved catalog identities and retained producer values at the selection
// boundary. A live caller must supply these; this is not a set of defaults or
// a replacement for regional/ecology/ownership producers.
struct SmacHumanResearchInputs {
    int32_t faction_id;
    std::array< SmacResearchTechnology, 89 > technologies;
    std::array< int32_t, 72 > facility_prerequisites;
    std::array< int32_t, 2 > reactor_prerequisites;
    int32_t former_prerequisite, foil_prerequisite;
    struct Weapon { int32_t power, prerequisite; };
    std::array< Weapon, 21 > weapons;
    std::array< int32_t, 4 > priorities; // Explore/Discover/Build/Conquer
    int32_t base_count, intrinsic_planet, intrinsic_probe, psi_bonus, population_bonus;
    int32_t maximum_attack, maximum_defense, turn, cloud_cover, ecology_aggregate;
    bool blind, endgame_pressure;
    std::array< uint32_t, 8 > relations;
    std::array< uint32_t, 128 > regional_known_tiles;
    std::array< int32_t, 128 > regional_strategy;
    std::array< std::array< int32_t, 128 >, 8 > regional_bases;
    std::array< uint32_t, 128 > region_area;
};

// RE-SMAC-BLIND-RESEARCH-001: complete ordinary human mode-zero valuation.
// Signed32 multiplication/addition wrap before each truncating division.
int32_t CalculateSmacHumanResearchValue( const SmacHumanResearchInputs& input, int32_t technology );

}
