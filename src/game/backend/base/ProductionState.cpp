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
	if ( smac_source_id ) {
		const auto source = *smac_source_id;
		const bool valid = kind == ProductionItemKind::Unit ? source >= 0 && source < 512
			: kind == ProductionItemKind::Facility ? source >= 1 && source <= 33
			: kind == ProductionItemKind::Orbital ? source >= 34 && source <= 37
			: kind == ProductionItemKind::Project ? source >= 39 && source <= 71
			: source == 38;
		if ( !valid || id.size() > ProductionState::MAX_ID_BYTES ) { throw std::runtime_error( "Invalid canonical production identity" ); }
	}
}

bool ProductionItem::operator==( const ProductionItem& other ) const {
	return kind == other.kind && id == other.id && smac_source_id == other.smac_source_id;
}

void ProductionState::Validate() const {
	current_item.Validate();
	baseline_item.Validate();
	if ( queue ) {
		if ( queue->size() > MAX_QUEUE_ITEMS || !current_item.smac_source_id || !baseline_item.smac_source_id ) {
			throw std::runtime_error( "Canonical production requires bounded queue and explicit identities" );
		}
		for ( const auto& item : *queue ) { item.Validate(); if ( !item.smac_source_id ) { throw std::runtime_error( "Queued production identity is unresolved" ); } }
	}
	else if ( current_item.smac_source_id || baseline_item.smac_source_id ) { throw std::runtime_error( "Canonical production queue is unavailable" ); }
}

bool ProductionState::operator==( const ProductionState& other ) const {
	return current_item == other.current_item && minerals == other.minerals &&
		baseline_item == other.baseline_item && baseline_minerals == other.baseline_minerals && queue == other.queue;
}

types::Buffer SerializeProductionState( const std::optional< ProductionState >& state ) {
	if ( state ) { state->Validate(); }
	types::Buffer buffer;
	const auto write_item = [ & ]( const ProductionItem& item ) {
		buffer.WriteInt( static_cast< int64_t >( item.kind ) );
		buffer.WriteBool( item.smac_source_id.has_value() );
		if ( item.smac_source_id ) { buffer.WriteInt( *item.smac_source_id ); }
		buffer.WriteString( item.id );
	};
	buffer.WriteBool( state.has_value() );
	if ( state ) {
		write_item( state->current_item ); buffer.WriteInt( state->minerals );
		write_item( state->baseline_item ); buffer.WriteInt( state->baseline_minerals );
		buffer.WriteBool( state->queue.has_value() );
		if ( state->queue ) {
			buffer.WriteInt( state->queue->size() );
			for ( const auto& item : *state->queue ) { write_item( item ); }
		}
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
		std::optional< int32_t > source;
		if ( buffer.ReadBool() ) {
			const auto value = buffer.ReadInt();
			if ( value < 0 || value >= 512 ) { throw std::runtime_error( "Production source identity outside range" ); }
			source = static_cast< int32_t >( value );
		}
		ProductionItem item{ static_cast< ProductionItemKind >( kind ), source ? buffer.ReadString(ProductionState::MAX_ID_BYTES) : buffer.ReadString(), source };
		item.Validate(); return item;
	};
	if ( buffer.ReadBool() ) {
		auto current = read_item(); const auto minerals = buffer.ReadInt();
		auto baseline = read_item(); const auto baseline_minerals = buffer.ReadInt();
		std::optional< std::vector< ProductionItem > > queue;
		if ( buffer.ReadBool() ) {
			const auto count = buffer.ReadInt();
			if ( count < 0 || count > static_cast< int64_t >( ProductionState::MAX_QUEUE_ITEMS ) ) { throw std::runtime_error( "Production queue exceeds port budget" ); }
			queue.emplace(); queue->reserve( count );
			for ( int64_t i=0; i<count; ++i ) { queue->push_back(read_item()); }
		}
		result = ProductionState{ std::move(current), minerals, std::move(baseline), baseline_minerals, std::move(queue) };
		result->Validate();
	}
	if ( buffer.lenr != buffer.lenw ) { throw std::runtime_error( "Trailing production state data" ); }
	return result;
}

}
