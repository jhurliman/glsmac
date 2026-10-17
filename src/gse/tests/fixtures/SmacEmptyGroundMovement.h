#pragma once

#include <cstdint>

namespace gse::tests::fixtures::smac_empty_ground {

// Owned semantic reductions of original captures; no executable/asset bytes.
// movement-admission-and-spending-cases.json SHA256 c6db902cff4485a0107bb806817e350902a4593cb526a684636dfd84697c0220
// movement-fungus-and-failure-marker-cases.json SHA256 5c5bd5e8ce6e72868e68d63fc527a2be188ba9271ee8d774b995dd60d481bedb
struct Case {
	const char* name;
	int32_t spent, allowance, cost;
	bool cost_queried, fungus, marker;
	uint32_t seed;
	int32_t draw; // -1 means no draw
	uint32_t after;
	bool moved;
	int32_t spent_after;
	bool marker_after, source_present, destination_present;
	int32_t wrapper_return;
};

// Three exhausted rows did not query cost; supplied zero is unused API input.
inline constexpr Case ADMISSION[] = {
	{ "flat_full", 0, 3, 3, true, false, false, 3u, -1, 3u, true, 3, false, false, true, 1 },
	{ "flat_left2_draw0", 1, 3, 3, true, false, false, 3u, 0, 1018897798u, true, 4, false, false, true, 1 },
	{ "flat_left2_draw1", 1, 3, 3, true, false, false, 1u, 1, 1015568748u, true, 4, false, false, true, 1 },
	{ "flat_left2_draw2", 1, 3, 3, true, false, false, 0u, 2, 1013904223u, false, 3, true, true, false, 1 },
	{ "flat_left1_draw0", 2, 3, 3, true, false, false, 3u, 0, 1018897798u, true, 5, false, false, true, 1 },
	{ "flat_left1_draw1", 2, 3, 3, true, false, false, 1u, 1, 1015568748u, false, 3, true, true, false, 1 },
	{ "flat_left1_draw2", 2, 3, 3, true, false, false, 0u, 2, 1013904223u, false, 3, true, true, false, 1 },
	{ "exhausted_flat", 3, 3, 0, false, false, false, 3u, -1, 3u, false, 3, false, true, false, 0 },
	{ "overspent_flat", 4, 3, 0, false, false, false, 3u, -1, 3u, false, 4, false, true, false, 0 },
	{ "rock_full", 0, 3, 6, true, false, false, 3u, -1, 3u, true, 6, false, false, true, 1 },
	{ "rock_left2_draw0", 1, 3, 6, true, false, false, 3u, 0, 1018897798u, true, 7, false, false, true, 1 },
	{ "rock_left2_draw1", 1, 3, 6, true, false, false, 16u, 1, 1040536623u, true, 7, false, false, true, 1 },
	{ "rock_left2_draw2", 1, 3, 6, true, false, false, 1u, 2, 1015568748u, false, 3, true, true, false, 1 },
	{ "rock_left2_draw3", 1, 3, 6, true, false, false, 4u, 3, 1020562323u, false, 3, true, true, false, 1 },
	{ "rock_left2_draw4", 1, 3, 6, true, false, false, 2u, 4, 1017233273u, false, 3, true, true, false, 1 },
	{ "rock_left2_draw5", 1, 3, 6, true, false, false, 0u, 5, 1013904223u, false, 3, true, true, false, 1 },
	{ "rock_left1_draw0", 2, 3, 6, true, false, false, 3u, 0, 1018897798u, true, 8, false, false, true, 1 },
	{ "rock_left1_draw1", 2, 3, 6, true, false, false, 16u, 1, 1040536623u, false, 3, true, true, false, 1 },
	{ "rock_left1_draw2", 2, 3, 6, true, false, false, 1u, 2, 1015568748u, false, 3, true, true, false, 1 },
	{ "rock_left1_draw3", 2, 3, 6, true, false, false, 4u, 3, 1020562323u, false, 3, true, true, false, 1 },
	{ "rock_left1_draw4", 2, 3, 6, true, false, false, 2u, 4, 1017233273u, false, 3, true, true, false, 1 },
	{ "rock_left1_draw5", 2, 3, 6, true, false, false, 0u, 5, 1013904223u, false, 3, true, true, false, 1 },
	{ "rock_forest_full", 0, 3, 9, true, false, false, 3u, -1, 3u, true, 9, false, false, true, 1 },
	{ "fungus_full_draw0", 0, 3, 9, true, true, false, 33u, 0, 1068833548u, true, 9, false, false, true, 1 },
	{ "fungus_full_draw1", 0, 3, 9, true, true, false, 3u, 1, 1018897798u, true, 9, false, false, true, 1 },
	{ "fungus_full_draw2", 0, 3, 9, true, true, false, 16u, 2, 1040536623u, true, 9, false, false, true, 1 },
	{ "fungus_full_draw3", 0, 3, 9, true, true, false, 1u, 3, 1015568748u, false, 3, true, true, false, 1 },
	{ "fungus_full_draw4", 0, 3, 9, true, true, false, 4u, 4, 1020562323u, false, 3, true, true, false, 1 },
	{ "fungus_full_draw5", 0, 3, 9, true, true, false, 62u, 5, 1117104773u, false, 3, true, true, false, 1 },
	{ "fungus_full_draw6", 0, 3, 9, true, true, false, 2u, 6, 1017233273u, false, 3, true, true, false, 1 },
	{ "fungus_full_draw7", 0, 3, 9, true, true, false, 50u, 7, 1097130473u, false, 3, true, true, false, 1 },
	{ "fungus_full_draw8", 0, 3, 9, true, true, false, 0u, 8, 1013904223u, false, 3, true, true, false, 1 },
	{ "fungus_left2_draw0", 1, 3, 9, true, true, false, 33u, 0, 1068833548u, true, 10, false, false, true, 1 },
	{ "fungus_left2_draw1", 1, 3, 9, true, true, false, 3u, 1, 1018897798u, true, 10, false, false, true, 1 },
	{ "fungus_left2_draw2", 1, 3, 9, true, true, false, 16u, 2, 1040536623u, false, 3, true, true, false, 1 },
	{ "fungus_left2_draw3", 1, 3, 9, true, true, false, 1u, 3, 1015568748u, false, 3, true, true, false, 1 },
	{ "fungus_left2_draw4", 1, 3, 9, true, true, false, 4u, 4, 1020562323u, false, 3, true, true, false, 1 },
	{ "fungus_left2_draw5", 1, 3, 9, true, true, false, 62u, 5, 1117104773u, false, 3, true, true, false, 1 },
	{ "fungus_left2_draw6", 1, 3, 9, true, true, false, 2u, 6, 1017233273u, false, 3, true, true, false, 1 },
	{ "fungus_left2_draw7", 1, 3, 9, true, true, false, 50u, 7, 1097130473u, false, 3, true, true, false, 1 },
	{ "fungus_left2_draw8", 1, 3, 9, true, true, false, 0u, 8, 1013904223u, false, 3, true, true, false, 1 },
	{ "speeder_fungus_full_draw0", 0, 6, 9, true, true, false, 33u, 0, 1068833548u, true, 9, false, false, true, 1 },
	{ "speeder_fungus_full_draw1", 0, 6, 9, true, true, false, 3u, 1, 1018897798u, true, 9, false, false, true, 1 },
	{ "speeder_fungus_full_draw2", 0, 6, 9, true, true, false, 16u, 2, 1040536623u, true, 9, false, false, true, 1 },
	{ "speeder_fungus_full_draw3", 0, 6, 9, true, true, false, 1u, 3, 1015568748u, true, 9, false, false, true, 1 },
	{ "speeder_fungus_full_draw4", 0, 6, 9, true, true, false, 4u, 4, 1020562323u, true, 9, false, false, true, 1 },
	{ "speeder_fungus_full_draw5", 0, 6, 9, true, true, false, 62u, 5, 1117104773u, true, 9, false, false, true, 1 },
	{ "speeder_fungus_full_draw6", 0, 6, 9, true, true, false, 2u, 6, 1017233273u, false, 6, true, true, false, 1 },
	{ "speeder_fungus_full_draw7", 0, 6, 9, true, true, false, 50u, 7, 1097130473u, false, 6, true, true, false, 1 },
	{ "speeder_fungus_full_draw8", 0, 6, 9, true, true, false, 0u, 8, 1013904223u, false, 6, true, true, false, 1 },
	{ "speeder_fungus_left3_draw0", 3, 6, 9, true, true, false, 33u, 0, 1068833548u, true, 12, false, false, true, 1 },
	{ "speeder_fungus_left3_draw1", 3, 6, 9, true, true, false, 3u, 1, 1018897798u, true, 12, false, false, true, 1 },
	{ "speeder_fungus_left3_draw2", 3, 6, 9, true, true, false, 16u, 2, 1040536623u, true, 12, false, false, true, 1 },
	{ "speeder_fungus_left3_draw3", 3, 6, 9, true, true, false, 1u, 3, 1015568748u, false, 6, true, true, false, 1 },
	{ "speeder_fungus_left3_draw4", 3, 6, 9, true, true, false, 4u, 4, 1020562323u, false, 6, true, true, false, 1 },
	{ "speeder_fungus_left3_draw5", 3, 6, 9, true, true, false, 62u, 5, 1117104773u, false, 6, true, true, false, 1 },
	{ "speeder_fungus_left3_draw6", 3, 6, 9, true, true, false, 2u, 6, 1017233273u, false, 6, true, true, false, 1 },
	{ "speeder_fungus_left3_draw7", 3, 6, 9, true, true, false, 50u, 7, 1097130473u, false, 6, true, true, false, 1 },
	{ "speeder_fungus_left3_draw8", 3, 6, 9, true, true, false, 0u, 8, 1013904223u, false, 6, true, true, false, 1 },
	{ "supplied_failed_flag_left1_draw0", 2, 3, 3, true, false, true, 3u, 0, 1018897798u, true, 5, false, false, true, 1 },
	{ "supplied_failed_flag_left1_draw1", 2, 3, 3, true, false, true, 1u, 1, 1015568748u, true, 5, false, false, true, 1 },
	{ "supplied_failed_flag_left1_draw2", 2, 3, 3, true, false, true, 0u, 2, 1013904223u, true, 5, false, false, true, 1 },
	{ "road_last_point", 2, 3, 1, true, false, false, 3u, -1, 3u, true, 3, false, false, true, 1 },
	{ "tube_last_point", 2, 3, 0, true, false, false, 3u, -1, 3u, true, 2, false, false, true, 1 },
	{ "tube_exhausted", 3, 3, 0, false, false, false, 3u, -1, 3u, false, 3, false, true, false, 0 },
};

// Only ordinary empty destinations. Positive PLANET changes supplied cost, not exemption.
inline constexpr Case FUNGUS[] = {
	{ "empty_fungus_full_draw0", 0, 3, 9, true, true, false, 33u, 0, 1068833548u, true, 9, false, false, true, 1 },
	{ "empty_fungus_full_draw1", 0, 3, 9, true, true, false, 3u, 1, 1018897798u, true, 9, false, false, true, 1 },
	{ "empty_fungus_full_draw2", 0, 3, 9, true, true, false, 16u, 2, 1040536623u, true, 9, false, false, true, 1 },
	{ "empty_fungus_full_draw3", 0, 3, 9, true, true, false, 1u, 3, 1015568748u, false, 3, true, true, false, 1 },
	{ "empty_fungus_full_draw4", 0, 3, 9, true, true, false, 4u, 4, 1020562323u, false, 3, true, true, false, 1 },
	{ "empty_fungus_full_draw5", 0, 3, 9, true, true, false, 62u, 5, 1117104773u, false, 3, true, true, false, 1 },
	{ "empty_fungus_full_draw6", 0, 3, 9, true, true, false, 2u, 6, 1017233273u, false, 3, true, true, false, 1 },
	{ "empty_fungus_full_draw7", 0, 3, 9, true, true, false, 50u, 7, 1097130473u, false, 3, true, true, false, 1 },
	{ "empty_fungus_full_draw8", 0, 3, 9, true, true, false, 0u, 8, 1013904223u, false, 3, true, true, false, 1 },
	{ "positive_planet_rocky_fungus_draw0", 0, 3, 6, true, true, false, 3u, 0, 1018897798u, true, 6, false, false, true, 1 },
	{ "positive_planet_rocky_fungus_draw1", 0, 3, 6, true, true, false, 16u, 1, 1040536623u, true, 6, false, false, true, 1 },
	{ "positive_planet_rocky_fungus_draw2", 0, 3, 6, true, true, false, 1u, 2, 1015568748u, true, 6, false, false, true, 1 },
	{ "positive_planet_rocky_fungus_draw3", 0, 3, 6, true, true, false, 4u, 3, 1020562323u, false, 3, true, true, false, 1 },
	{ "positive_planet_rocky_fungus_draw4", 0, 3, 6, true, true, false, 2u, 4, 1017233273u, false, 3, true, true, false, 1 },
	{ "positive_planet_rocky_fungus_draw5", 0, 3, 6, true, true, false, 0u, 5, 1013904223u, false, 3, true, true, false, 1 },
};

struct Sequence {
	uint32_t seed;
	int32_t first_draw, first_cost;
	uint32_t failed_seed;
	int32_t failed_spent; bool failed_marker;
	int32_t reset_spent; bool reset_marker; uint32_t reset_seed;
	int32_t next_cost, next_draw; uint32_t final_seed;
	bool moved; int32_t spent; bool marker; int32_t wrapper_return;
};
inline constexpr Sequence UPKEEP[] = {
	{ 8u, 1, 9, 1027220423u, 3, true, 0, true, 1027220423u, 9, 0, 18026106u, true, 9, false, 1 },
	{ 0u, 8, 9, 1013904223u, 3, true, 0, true, 1013904223u, 9, 1, 1196435762u, true, 9, false, 1 },
	{ 3u, 1, 9, 1018897798u, 3, true, 0, true, 1018897798u, 9, 2, 2365144877u, true, 9, false, 1 },
	{ 9u, 4, 9, 1028884948u, 3, true, 0, true, 1028884948u, 9, 3, 407595811u, true, 9, false, 1 },
	{ 1u, 3, 9, 1015568748u, 3, true, 0, true, 1015568748u, 9, 4, 1586005467u, true, 9, false, 1 },
	{ 4u, 4, 9, 1020562323u, 3, true, 0, true, 1020562323u, 9, 5, 2754714582u, true, 9, false, 1 },
	{ 7u, 6, 9, 1025555898u, 3, true, 0, true, 1025555898u, 9, 6, 3923423697u, false, 3, true, 1 },
	{ 2u, 6, 9, 1017233273u, 3, true, 0, true, 1017233273u, 9, 7, 1975575172u, false, 3, true, 1 },
	{ 5u, 8, 9, 1022226848u, 3, true, 0, true, 1022226848u, 9, 8, 3144284287u, false, 3, true, 1 },
};

}
