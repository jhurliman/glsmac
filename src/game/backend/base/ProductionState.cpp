#include "ProductionState.h"

#include <stdexcept>
#include <utility>

namespace game::backend::base {

void ProductionItem::Validate() const {
	switch ( kind ) {
		case ProductionItemKind::Unit:
		case ProductionItemKind::Facility:
		case ProductionItemKind::Orbital:
		case ProductionItemKind::Project:
			if ( id.empty() ) { throw std::runtime_error( "Production item requires an identity" ); }
			break;
		case ProductionItemKind::Stockpile:
			if ( !id.empty() ) { throw std::runtime_error( "Stockpile cannot have a catalog identity" ); }
			break;
		default:
			throw std::runtime_error( "Unknown production item kind" );
	}
}

bool ProductionItem::operator==( const ProductionItem& other ) const {
	return kind == other.kind && id == other.id;
}

void ProductionState::Validate() const {
	current_item.Validate();
	baseline_item.Validate();
}

bool ProductionState::operator==( const ProductionState& other ) const {
	return current_item == other.current_item && minerals == other.minerals &&
		baseline_item == other.baseline_item && baseline_minerals == other.baseline_minerals;
}

types::Buffer SerializeProductionState( const std::optional< ProductionState >& state ) {
	if ( state ) { state->Validate(); }
	types::Buffer buffer;
	buffer.WriteBool( state.has_value() );
	if ( state ) {
		buffer.WriteInt( static_cast< int64_t >( state->current_item.kind ) );
		buffer.WriteString( state->current_item.id );
		buffer.WriteInt( state->minerals );
		buffer.WriteInt( static_cast< int64_t >( state->baseline_item.kind ) );
		buffer.WriteString( state->baseline_item.id );
		buffer.WriteInt( state->baseline_minerals );
	}
	return buffer;
}

std::optional< ProductionState > DeserializeProductionState( types::Buffer& buffer ) {
	std::optional< ProductionState > result;
	const auto read_item = [ & ]() {
		const auto kind = buffer.ReadInt();
		if ( kind < static_cast< int64_t >( ProductionItemKind::Unit ) || kind > static_cast< int64_t >( ProductionItemKind::Stockpile ) ) {
			throw std::runtime_error( "Unknown production item kind" );
		}
		ProductionItem item{ static_cast< ProductionItemKind >( kind ), buffer.ReadString() };
		item.Validate();
		return item;
	};
	if ( buffer.ReadBool() ) {
		auto current = read_item();
		const auto minerals = buffer.ReadInt();
		auto baseline = read_item();
		const auto baseline_minerals = buffer.ReadInt();
		result = ProductionState{ std::move( current ), minerals, std::move( baseline ), baseline_minerals };
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Trailing production state data" ); }
	return result;
}

}
