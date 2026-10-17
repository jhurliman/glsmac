return {

	validate: (e) => {
		if (e.data.unit.is_conventional) { return 'Conventional despawn rollback is not activated'; }
		if (e.caller != 0) {
			return 'Only master is allowed to despawn units';
		}
	},

	apply: (e) => {
		if (e.data.unit.is_conventional) { throw Error('Conventional despawn rollback is not activated'); }
		const unit = e.data.unit;
		// Assignment dereferences native Ptr fields. A property read directly in
		// an object literal can retain a pointer into the soon-deleted unit.
		const movement = unit.movement;
		const morale = unit.morale;
		const health = unit.health;
		const moved_this_turn = unit.moved_this_turn;
		// Copy scalar state before destruction invalidates every unit wrapper.
		const backup = {
			id: unit.id,
			def: unit.def,
			owner: unit.owner,
			tile: unit.tile,
			movement: movement,
			morale: morale,
			health: health,
			moved_this_turn: moved_this_turn,
		};
		e.game.um.despawn_unit(unit);
		return {unit: backup};
	},

	rollback: (e) => {
		e.game.um.restore_unit(e.applied.unit);
	},

};
