#pragma once

#include <unordered_set>
#include <unordered_map>
#include <mutex>
#include <string>
#include <map>
#include <vector>

#include "gse/GCWrappable.h"

#include "gse/value/Object.h"

#include "RenderInfo.h"
#include "Pop.h"
#include "SmacBaseResourceContext.h"

namespace gse::tests { class BaseAllocationFixture; }

namespace game {
namespace backend {

class Game;

namespace map {
class Map;
namespace tile {
class Tile;
}
}

namespace slot {
class Slot;
}
namespace faction { class Faction; }

namespace base {

class Base;
class PopDef;
class FacilityDef;

CLASS( BaseManager, gse::GCWrappable )
public:

	typedef std::unordered_map< std::string, PopDef* > popdefs_t;
	typedef std::unordered_map< std::string, FacilityDef* > facilitydefs_t;

	BaseManager( Game* game );
	~BaseManager();

	void Clear();

	PopDef* GetPopDef( const std::string& id ) const;
	// Takes ownership only on successful return, including frontend publication.
	void DefinePop( PopDef* pop_def );
	void UndefinePop( const std::string& id );
	void ValidateSmacPopDef( const PopDef* def ) const;

	FacilityDef* GetFacilityDef( const std::string& id ) const;
	void DefineFacility( FacilityDef* facility_def );
	void UndefineFacility( const std::string& id );
	void ValidateSmacFacilityDef( const FacilityDef* def ) const;

	Base* GetBase( const size_t id ) const;
	Pop* GetWorkingPop( const map::tile::coords_t& coord ) const;
	std::string ValidateAssignment( const size_t base_id, const map::tile::coords_t& center, const size_t pop_id, const PopDef* def, const Pop::worked_tile_t& tile ) const;
	void SpawnBase( GSE_CALLABLE, Base* base );
	void DespawnBase( GSE_CALLABLE, const size_t base_id );
	SmacBaseResourceContext PrepareStartingResourceContext( const slot::Slot* owner ) const;
	Base* SpawnStartingSmacBase( GSE_CALLABLE, slot::Slot* owner, map::tile::Tile* tile,
		const std::string& worker, const std::array< std::string, 7 >& specialists );
	void ValidateSmacResourceContext( const SmacBaseResourceContext& context, const slot::Slot* owner, const faction::Faction* faction ) const;

	const std::map< size_t, Base* >& GetBases() const;
	// Explicit current record order. Live registration appends; removal keeps
	// survivor order; V9 serializes this sequence and restores it verbatim.
	const std::vector< size_t >& GetBaseRecordOrder() const { return m_base_order; }
	const popdefs_t& GetBasePopDefs() const;

	void ProcessUnprocessed( GSE_CALLABLE );
	void PushUpdates();

	WRAPDEFS_PTR( BaseManager )

	void TriggerUpdates( GSE_CALLABLE );

	void Serialize( types::Buffer& buf ) const;
	void Deserialize( GSE_CALLABLE, types::Buffer& buf );

	void RefreshBase( const Base* base );

	void AddUpdateTrigger( Base* base ); // TODO: combine with RefreshBase?

	void GetReachableObjects( std::unordered_set< Object* >& reachable_objects ) override;

private:
	friend class Base;
	friend class gse::tests::BaseAllocationFixture;
	std::array< bool, 89 > GetSmacSpecialistKnowledge( const Base& base ) const;
	void RequireSmacWorkerRecalculation() const;
	using tile_key_t = std::pair< size_t, size_t >;
	using pop_key_t = std::pair< size_t, size_t >;
	// Derived lookup only. IDs/coordinates remain safe after the map is destroyed.
	std::map< tile_key_t, pop_key_t > m_work_assignments;
	void ChangeAssignment( const size_t base_id, const size_t pop_id, const Pop::worked_tile_t& old_tile, const Pop::worked_tile_t& new_tile );
	void RemoveAssignments( const Base* base );
	Game* m_game = nullptr;

	popdefs_t m_base_popdefs = {};
	facilitydefs_t m_base_facilitydefs = {};

	std::map< size_t, Base* > m_bases = {};
	std::vector< size_t > m_base_order;
	std::vector< types::Buffer > m_unprocessed_bases = {};

	std::unordered_set< std::string > m_registered_base_names = {};

	enum base_update_op_t : uint8_t {
		BUO_NONE = 0,
		BUO_SPAWN = 1 << 0,
		BUO_REFRESH = 1 << 1,
		BUO_DESPAWN = 1 << 2,
	};
	struct base_update_t {
		base_update_op_t ops = BUO_NONE;
		const Base* base = nullptr;
	};
	std::unordered_map< size_t, base_update_t > m_base_updates = {};
	static void QueueBaseUpdateInto( std::unordered_map< size_t, base_update_t >& updates, const Base* base, base_update_op_t op );

	void QueueBaseUpdate( const Base* base, const base_update_op_t op );
	// Register saved state and frontend updates without new-base gameplay hooks.
	void RegisterBase( Base* base );
	void PrepareBaseName( Base* base ) const;

private:
	std::mutex m_updated_bases_mutex;
	std::unordered_set< Base* > m_updated_bases = {};
	// The existing callback loop holds the set mutex. Native allocation
	// publication must reject reentry rather than recursively taking that lock.
	bool m_dispatching_updates = false;

};

}
}
}
