#pragma once
#include <array>
#include <charconv>
#include <cstdint>
#include <stdexcept>
#include <string>
#include <string_view>

namespace config::smac_preferences {
using Words=std::array<uint32_t,3>;
struct Binding {int id,word;uint32_t mask;bool inverted;};
// RE-SMAC-PREFERENCES-SEMANTICS-001: original initializer/collector.
inline constexpr std::array<Binding,67> Bindings{{
    {7,0,0x00000020u,false}, // tutorial_messages
    {8,0,0x00004000u,false}, // automatic_unit_design
    {9,1,0x00100000u,false}, // prune_obsolete_designs
    {10,0,0x00001000u,false}, // edge_scroll
    {11,0,0x00000002u,false}, // turn_autosave
    {12,0,0x00000001u,false}, // end_turn_pause
    {13,0,0x00000004u,true}, // fast_enemy_neutral_moves
    {14,0,0x00008000u,true}, // fast_allied_moves
    {15,0,0x20000000u,true}, // single_click_radio
    {16,1,0x00040000u,false}, // unit_click_cancels_orders
    {17,1,0x00008000u,false}, // confirm_combat_odds
    {18,1,0x00000010u,false}, // pause_after_battle
    {19,0,0x00000008u,false}, // fast_combat
    {28,1,0x00000040u,false}, // fast_ordered_moves
    {20,1,0x00000080u,false}, // fast_all_moves
    {21,0,0x00080000u,false}, // ordered_units_without_centering
    {23,1,0x00000004u,false}, // base_zoom_without_recentering
    {24,1,0x00200000u,false}, // detailed_main_menus
    {25,1,0x00000400u,false}, // detailed_context_menus
    {26,1,0x00000100u,false}, // right_click_menu
    {29,0,0x00010000u,false}, // air_return_at_fuel_limit
    {30,0,0x01000000u,false}, // former_forests
    {31,0,0x00020000u,false}, // former_elevation
    {32,0,0x02000000u,false}, // former_advanced_improvements
    {33,1,0x00000008u,false}, // former_fungus_removal
    {34,1,0x00000020u,false}, // former_sensors
    {42,1,0x00400000u,true}, // former_transport_links
    {35,0,0x00100000u,false}, // pact_contact_cancels_movement
    {36,0,0x00200000u,false}, // treaty_contact_cancels_movement
    {37,0,0x00400000u,false}, // truce_contact_cancels_movement
    {38,0,0x00800000u,false}, // vendetta_contact_cancels_movement
    {39,0,0x40000000u,false}, // ignore_other_unit_classes
    {40,0,0x80000000u,false}, // wake_landed_passengers
    {41,1,0x00000800u,false}, // automatic_monolith
    {64,1,0x00020000u,false}, // stop_voice_on_popup_close
    {65,0,0x10000000u,false}, // project_movies
    {66,0,0x00040000u,true}, // interludes
    {71,1,0x01000000u,true}, // monuments
    {67,0,0x00000080u,false}, // map_animation
    {68,0,0x08000000u,false}, // window_animation
    {69,1,0x00080000u,false}, // scrollbar_animation
    {22,1,0x00000200u,false}, // unit_blink
    {43,2,0x00000001u,false}, // facility_completion
    {44,2,0x00000002u,false}, // civilian_completion
    {45,2,0x00000400u,false}, // combat_completion
    {46,2,0x00000004u,false}, // prototype_completion
    {47,2,0x00002000u,false}, // governor_queue_completion
    {48,2,0x00000008u,false}, // riot_start
    {49,2,0x00000010u,false}, // riot_end
    {50,2,0x00000020u,false}, // golden_age_start
    {51,2,0x00000040u,false}, // golden_age_end
    {52,2,0x00000080u,false}, // low_nutrients
    {54,2,0x00000200u,false}, // stale_build_orders
    {55,2,0x00000800u,false}, // population_cap
    {56,2,0x00001000u,false}, // delayed_transcendence
    {57,2,0x00004000u,false}, // starvation
    {58,2,0x00008000u,false}, // mineral_shortage
    {59,2,0x00010000u,false}, // energy_shortage
    {60,2,0x00020000u,false}, // random_events
    {72,1,0x00000001u,false}, // fog
    {73,0,0x00000100u,false}, // land_grid
    {78,1,0x00800000u,false}, // ocean_grid
    {74,0,0x00000200u,false}, // base_grid
    {75,1,0x00002000u,false}, // base_names
    {76,1,0x00001000u,false}, // base_production
    {77,1,0x00010000u,false}, // flat_terrain
    {79,1,0x02000000u,true}, // active_unit_path
}};
inline constexpr std::array<Binding,3> Audio{{{0,0,0x400u,false},{1,0,0x800u,false},{2,1,0x4000u,false}}};
struct Draft {std::array<bool,80> checked{};std::array<bool,3> audio{};};
inline Words Defaults(bool laptop,int difficulty,bool restore=false) {
    return {{(laptop?0xa3e1dd16u:0xbbe1dd96u)|(!restore && difficulty<2?0x20u:0u),
        laptop?0x00327168u:0x003a7168u,0x0003c3a9u}};
}
inline Draft Decode(const Words& words) {
    Draft draft;
    for(const auto& b:Bindings) {draft.checked[b.id]=bool(words[b.word]&b.mask)!=b.inverted;}
    for(const auto& b:Audio) {draft.audio[b.id]=bool(words[b.word]&b.mask);}return draft;
}
inline Words Encode(const Draft& draft) {
    Words words{};
    for(const auto& b:Bindings) {if(draft.checked[b.id]!=b.inverted) {words[b.word]|=b.mask;}}
    for(const auto& b:Audio) {if(draft.audio[b.id]) {words[b.word]|=b.mask;}}return words;
}
// Local YAML representation; original INI binary-string import is separate.
inline std::string Store(const Words& words) {
    return std::to_string(words[0])+","+std::to_string(words[1])+","+std::to_string(words[2]);
}
inline Words Load(const std::string& text) {
    Words words{};size_t first=0;
    for(size_t i=0;i<3;++i) {
        const auto end=i<2?text.find(',',first):text.size();
        if(end==std::string::npos || end<=first || end-first>10) {throw std::invalid_argument("Invalid stored SMAC preferences");}
        const auto parsed=std::from_chars(text.data()+first,text.data()+end,words[i]);
        if(parsed.ec!=std::errc() || parsed.ptr!=text.data()+end) {throw std::invalid_argument("Invalid stored SMAC preference word");}
        first=end+1;
    }
    return words;
}
}
