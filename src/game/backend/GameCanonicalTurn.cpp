#include "Game.h"

#include "Player.h"
#include "map/Map.h"
#include "gc/Space.h"
#include "gse/value/Int.h"

namespace game::backend {

void Game::RequireCanonicalStartupTurn( size_t turn_id ) const {
	if ( !m_map || !m_map->m_worker_knowledge ) { return; }
	RequireConventionalStartupClaim();
	ValidateCurrentSimulation();
	if ( m_current_turn.GetId()!=0 || turn_id!=1 || !GetGCSpace()->IsAccumulating()
		|| !m_map->m_worker_knowledge_ready || m_state->GetConnection()
		|| !m_player || m_player->GetRole()!=Player::PR_SINGLE ) {
		throw std::runtime_error("Canonical first transition requires the owning claimed local startup");
	}
	const auto data=m_map->m_worker_knowledge->CopyData();
	for ( int32_t observer=1; observer<=7; ++observer ) {
		if (data.observers[observer]) { (void)ProjectSmacWorkerKnowledge(*m_map->m_worker_knowledge,observer,nullptr,true); }
	}
}

bool Game::HandleCanonicalStartupTurnEvent( GSE_CALLABLE, const std::string& name,
	const gse::value::object_properties_t& args, size_t caller, bool apply ) {
	if ( name!="advance_turn" || !m_map || !m_map->m_worker_knowledge ) { return false; }
	const auto turn=args.find("turn_id");
	if ( caller!=m_slot_num || args.size()!=1 || turn==args.end() || !turn->second
		|| turn->second->type!=gse::VT_INT || static_cast<gse::value::Int*>(turn->second)->value!=1 ) {
		throw std::runtime_error("Canonical startup turn event requires its exact local first transition");
	}
	RequireCanonicalStartupTurn(1);
	if (apply) { CheckRW(GSE_CALL); AdvanceTurn(1); }
	return true;
}

}
