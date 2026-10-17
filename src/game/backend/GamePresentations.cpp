#include "Game.h"

#include "unit/UnitManager.h"
#include "unit/Def.h"
#include "unit/StaticDef.h"
#include "unit/CvrScoutRender.h"
#include "unit/FormerMarkerRender.h"
#include "unit/ColonyMarkerRender.h"
#include "unit/MoraleSet.h"
#include "base/BaseManager.h"
#include "gc/Space.h"
#include <set>
#include <thread>

namespace game::backend {

void Game::ConfigureSmacPresentations( GSE_CALLABLE, const std::array<std::string,4>& roles ) {
	bool configure=false;
	{
		std::lock_guard lock(m_admission_mutex);
		configure=m_owner_transition && m_owner_transition->phase==OwnerTransition::Phase::Running
			&& m_owner_transition->thread==std::this_thread::get_id()
			&& m_active_admission && m_owner_transition->admission==m_active_admission && !m_active_admission->cancelled
			&& m_active_admission->state==m_state && m_state->IsGameSessionActive(m_active_admission->admitted_binding)
			&& (m_owner_transition->stage==StartupStage::InitializeConfigure
				|| (m_owner_transition->stage==StartupStage::Start && m_state->IsMaster()
					&& m_active_admission->kind==WorldAdmission::Kind::INITIALIZATION
					&& !m_active_admission->previous && !m_active_admission->map_path));
	}
	if (!configure || gc_space!=GetGCSpace() || !gc_space->IsAccumulating() || !m_um || !m_bm
		|| !m_um->GetUnits().empty() || !m_bm->GetBases().empty() || m_current_turn.GetId()!=0) {
		throw std::runtime_error("Presentation roles must be configured by startup before entities");
	}
	std::set<std::string> unique;
	for (const auto& role:roles) {
		if (role.empty() || role.size()>256 || role.find('\0')!=std::string::npos || !unique.insert(role).second) {
			throw std::runtime_error("Presentation roles require four distinct bounded definition IDs");
		}
	}
	if (m_smac_presentations && *m_smac_presentations!=roles) { throw std::runtime_error("Presentation roles already configured"); }
	m_smac_presentations=roles;
}

unit::Def* Game::ResolveSmacProductionPresentation( const slot::Slot* owner, int32_t design_id ) const {
	RequireSmacLocalHumanOwner(owner);
	if (!m_smac_presentations || !m_um) { throw std::runtime_error("Canonical presentation roles are not configured"); }
	const auto& participant=RequireSmacProductionOwner(owner);
	const unit::ConventionalUnitState state{design_id,participant.simulation_id,1,0,0,false,
		unit::ConventionalUnitState::Home::None,unit::SmacMoraleMode::Mode0};
	const auto context=ResolveConventionalIdentity(state,owner,*m_simulation);
	using Profile=unit::ConventionalUnitContext::Presentation;
	size_t index;
	switch(context.presentation) {
		case Profile::Scout:index=0;break;
		case Profile::SelectedSpartanRover:index=1;break;
		case Profile::FormerMarker:index=2;break;
		case Profile::ColonyMarker:index=3;break;
		default:throw std::runtime_error("Unsupported canonical presentation role");
	}
	auto* definition=m_um->GetUnitDef(m_smac_presentations->at(index));
	if (!definition || definition->m_type!=unit::DT_STATIC || !definition->m_moraleset
		|| m_um->GetMoraleSet(definition->m_moraleset->m_id)!=definition->m_moraleset) {
		throw std::runtime_error("Configured presentation definition or morale set is unavailable");
	}
	const auto* presentation=static_cast<const unit::StaticDef*>(definition);
	const auto* cvr=dynamic_cast<const unit::CvrScoutRender*>(presentation->m_render);
	const bool valid=index==0 ? cvr && cvr->m_type==unit::Render::RT_CVR_INITIALIZED_SCOUT
		&& cvr->composition==unit::CvrScoutRender::Composition::InitializedScout
		: index==1 ? cvr && cvr->m_type==unit::Render::RT_CVR_SPARTAN_ROVER
		&& cvr->composition==unit::CvrScoutRender::Composition::SpartanRover
		: index==2 ? dynamic_cast<const unit::FormerMarkerRender*>(presentation->m_render)
		&& presentation->m_render->m_type==unit::Render::RT_PORT_FORMER_MARKER
		: dynamic_cast<const unit::ColonyMarkerRender*>(presentation->m_render)
		&& presentation->m_render->m_type==unit::Render::RT_PORT_COLONY_MARKER;
	if (!valid || presentation->m_movement_type!=unit::MT_LAND) { throw std::runtime_error("Configured presentation does not match the actual design profile"); }
	return definition;
}

}
