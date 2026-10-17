#pragma once

#include "SmacWorkerMapFrame.h"

namespace game::backend::map {

// Owned worker-query knowledge only. Terrain, entity ownership/coordinates,
// complete Base allocation and their live authentication belong to callers.
class SmacWorkerKnowledge {
public:
	static constexpr size_t MAX_WIRE_BYTES = 4 * 1024 * 1024; // port storage budget
	// Remembered copies of actual GLSMAC fields, not the original packed word.
	struct Improvements {
		uint16_t features, terraforming;
		bool base_present, unit_present;
		bool operator==( const Improvements& other ) const noexcept;
	};
	struct Tile {
		uint8_t explored_by;
		std::optional< int32_t > claimant; // none differs from native0
		bool operator==( const Tile& other ) const noexcept;
	};
	struct Observer {
		bool foreign_disclosure;
		std::array< SmacWorkerMapFrame::Relation, 8 > relations;
		bool operator==( const Observer& other ) const noexcept;
	};
	struct UnitObservation {
		uint64_t id; // explicit port Unit ID, 1..INT64_MAX; never an original list index
		uint8_t visible_to;
		bool active_convoy;
		bool operator==( const UnitObservation& other ) const noexcept;
	};
	struct Data {
		int32_t width, height;
		bool horizontal_wrap, ordinary_local_mode;
		uint8_t all_map_revealed;
		std::array< std::optional< Observer >, 8 > observers;
		// Y ascending, X from row parity in steps of two. No duplicated sea flag.
		std::vector< Tile > tiles;
		std::vector< UnitObservation > units;
		// Empty observer row = memory unavailable. Present row has one optional
		// entry per map tile; absence is unknown, never current terrain by default.
		std::array< std::vector< std::optional< Improvements > >, 8 > remembered;
		bool operator==( const Data& other ) const noexcept;
	};
	struct LiveUnit {
		uint64_t id;
		int32_t owner;
		SmacWorkerMapFrame::Coordinate coordinate;
	};
	static SmacWorkerKnowledge Create( const Data& data );
	Data CopyData() const;
	std::string Serialize() const;
	static SmacWorkerKnowledge Deserialize( const std::string& wire );
	bool operator==( const SmacWorkerKnowledge& other ) const noexcept;
	// Caller supplies authenticated current entities and terrain. Exactly one
	// live record must match every saved observation ID; input ordering may vary.
	// The frame preserves knowledge-record order and supplied Base order.
	SmacWorkerMapFrame Project( int32_t observer, const std::vector< bool >& sea,
		const std::vector< SmacWorkerMapFrame::Base >& bases, const std::vector< LiveUnit >& units ) const;

private:
	explicit SmacWorkerKnowledge( Data data );
	Data m_data;
};

}
