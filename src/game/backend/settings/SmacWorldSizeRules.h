#pragma once

#include <array>
#include <cstdint>
#include <string>

#include "types/Buffer.h"

namespace game::backend::settings {

struct SmacWorldSizeRule {
	std::string name;
	int32_t source_x, source_y; // source X is half the horizontal coordinate span
	bool operator==( const SmacWorldSizeRule& other ) const;
};

struct SmacWorldSizeSelection {
	int32_t category, width, height; // accepted preset 0..4 or explicit custom 99
	bool operator==( const SmacWorldSizeSelection& other ) const;
	types::Buffer Serialize() const;
	static SmacWorldSizeSelection Deserialize( types::Buffer buffer );
	void Validate() const;
};

struct SmacWorldSizeRules {
	std::array< SmacWorldSizeRule, 5 > presets;
	void Validate() const;
	bool operator==( const SmacWorldSizeRules& other ) const;
	types::Buffer Serialize() const;
	static SmacWorldSizeRules Deserialize( types::Buffer buffer );
	SmacWorldSizeSelection SelectPreset( int64_t category ) const;
	static SmacWorldSizeSelection SelectCustom( int64_t width, int64_t height );
	void ValidateSelection( const SmacWorldSizeSelection& selection ) const;
};

}
