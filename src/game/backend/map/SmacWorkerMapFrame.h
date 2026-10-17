#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace game::backend::map {

// Detached worker-consumer snapshot. List indices identify these records only;
// they are not live Unit/Base IDs, transport slots, or original save offsets.
class SmacWorkerMapFrame {
public:
	static constexpr size_t MAX_TILE_COUNT = 512 * 256 / 2;
	static constexpr size_t MAX_WIRE_BYTES = 2 * 1024 * 1024; // port storage budget
	static constexpr uint32_t RADIUS_MASK = ( uint32_t( 1 ) << 21 ) - 1;
	using Coordinate = std::array< int32_t, 2 >;
	struct Tile {
		bool sea;
		uint8_t explored_by; // bits 0..7, including native knowledge
		std::optional< int32_t > claimant; // none, native0, or playable1..7
		bool operator==( const Tile& other ) const noexcept;
	};
	struct Relation {
		bool foreign_unit_exemption, territory_recognition;
		bool operator==( const Relation& other ) const noexcept;
	};
	struct Base {
		int32_t owner; // playable1..7
		Coordinate coordinate;
		uint32_t selected_radius; // exactly the projected low21, not selected32 authority
		bool operator==( const Base& other ) const noexcept;
	};
	struct Unit {
		int32_t owner; // native0 or playable1..7; no live identity inferred
		Coordinate coordinate;
		uint8_t visible_to;
		bool active_convoy;
		bool operator==( const Unit& other ) const noexcept;
	};
	struct Data {
		int32_t width, height, observer;
		bool horizontal_wrap, ordinary_local_mode, foreign_disclosure;
		uint8_t all_map_revealed;
		// Native index0 has no diplomacy exemption; its recognition bit is retained.
		std::array< Relation, 8 > observer_relations;
		// Y ascending, X from row parity in steps of two, including unqueried tiles.
		std::vector< Tile > tiles;
		std::vector< Base > bases;
		std::vector< Unit > units;
		bool operator==( const Data& other ) const noexcept;
	};
	struct ObservedSquare {
		Coordinate coordinate;
		bool explored;
		std::optional< int32_t > claimant, recognized_owner;
		bool operator==( const ObservedSquare& other ) const noexcept;
	};
	struct Evaluation {
		uint32_t selected_radius, unavailable;
		std::array< std::optional< ObservedSquare >, 21 > squares;
		bool operator==( const Evaluation& other ) const noexcept;
	};
	// Copies input after validation. No mutable caller aliases enter the frame.
	static SmacWorkerMapFrame Create( const Data& data );
	Data CopyData() const;
	Evaluation Evaluate( size_t base_index ) const;
	std::string Serialize() const;
	static SmacWorkerMapFrame Deserialize( const std::string& wire );
	bool operator==( const SmacWorkerMapFrame& other ) const noexcept;

private:
	explicit SmacWorkerMapFrame( Data data );
	Data m_data;
};

}
