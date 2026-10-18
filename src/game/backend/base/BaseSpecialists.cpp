#include "Base.h"

#include "BaseManager.h"
#include "PopDef.h"
#include "game/backend/Game.h"
#include "game/backend/State.h"
#include "game/backend/faction/Faction.h"
#include "gse/value/Array.h"
#include "gse/value/Bool.h"
#include "gse/value/Int.h"
#include "gse/value/Undefined.h"

namespace game::backend::base {

std::array< bool, 89 > BaseManager::GetSmacSpecialistKnowledge( const Base& base ) const {
	const auto context = base.GetSmacResourceContext();
	const auto& global = m_game->GetState()->m_settings.global;
	if ( !global.GetSmacTechnologyRules() || !global.GetSmacFactionStartupRules() ) { THROW( "SMAC specialist query requires frozen technology and faction profiles" ); }
	const auto& value = m_game->m_simulation->value;
	value.technology.Validate( *global.GetSmacTechnologyRules(), *global.GetSmacFactionStartupRules() );
	const auto* owner = value.technology.Find( context.owner_simulation_id );
	if ( !owner || owner->admission.faction_id != base.m_faction->m_id ) { THROW( "SMAC specialist query has no matching technology owner" ); }
	return owner->owned;
}

uint32_t Base::ValidateSmacSpecialistPopulation() const {
	GetSmacResourceContext();
	if ( GetPopulation() > 127 ) { THROW( "SMAC specialist query population exceeds admitted 0..127 domain" ); }
	for ( const auto& item : m_pops ) {
		const auto& pop = item.second;
		if ( pop.m_id != item.first || pop.m_base != this ) { THROW( "SMAC specialist query population identity is invalid" ); }
		ValidatePopulationRole( pop.m_def, pop.GetWorkedTileCoords() );
	}
	return static_cast< uint32_t >( GetPopulation() );
}

SmacSpecialistMenu Base::GetSmacSpecialistMenu( std::optional< size_t > pop_id ) const {
	const auto population = ValidateSmacSpecialistPopulation();
	std::optional< uint8_t > current;
	if ( pop_id ) {
		const auto found = m_pops.find( *pop_id );
		if ( found == m_pops.end() ) { THROW( "SMAC specialist query population does not belong to this Base" ); }
		current = found->second.m_def->m_smac_specialist_id;
		if ( current ) {
			size_t specialists = 0;
			for ( const auto& item : m_pops ) { if ( !item.second.GetWorkedTileCoords() ) { ++specialists; } }
			if ( specialists > 16 ) { THROW( "SMAC specialist current-type query requires an explicit overflow allocation policy" ); }
		}
	}
	const auto& global = m_game->GetState()->m_settings.global;
	return QuerySmacSpecialistMenu( *global.GetSmacSpecialistRules(), *global.GetSmacTechnologyRules(),
		m_game->GetBM()->GetSmacSpecialistKnowledge( *this ), population, current );
}

uint8_t Base::SelectAutomaticSmacSpecialist() const {
	const auto population = ValidateSmacSpecialistPopulation();
	const auto& global = m_game->GetState()->m_settings.global;
	return base::SelectAutomaticSmacSpecialist( *global.GetSmacSpecialistRules(), *global.GetSmacTechnologyRules(),
		m_game->GetBM()->GetSmacSpecialistKnowledge( *this ), population );
}

gse::Value* Base::CopySmacSpecialistMenu( GSE_CALLABLE, std::optional< size_t > pop_id ) const {
	const auto result = GetSmacSpecialistMenu( pop_id );
	gse::value::array_elements_t types;
	for ( uint8_t id = 0; id < 7; ++id ) { if ( result.eligible[ id ] ) { types.push_back( VALUE( gse::value::Int,, id ) ); } }
	return VALUE( gse::value::Object,, GSE_CALL_NOGC, {
		{ "current_type", result.current ? static_cast< gse::Value* >( VALUE( gse::value::Int,, *result.current ) ) : VALUE( gse::value::Undefined ) },
		{ "eligible_types", VALUE( gse::value::Array,, types ) },
		{ "size_explanation_requested", VALUE( gse::value::Bool,, result.size_explanation_requested ) },
	} );
}

}
