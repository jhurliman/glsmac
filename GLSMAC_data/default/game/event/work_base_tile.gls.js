return {

	validate: (e) => {
		if (e.caller != e.data.base.get_owner().id) {
			return 'Only base owner can control base pops';
		}
		if (e.data.pop.get_base() != e.data.base) {
			return 'Population does not belong to this base';
		}
		const working_pop = e.data.tile.get_working_pop();
		if (#is_defined(working_pop) && working_pop != e.data.pop) {
			return 'Tile is already being worked';
		}
	},

	apply: (e) => {
		const old_type = e.data.pop.get_type();
		const old_tile = e.data.pop.get_worked_tile();
		e.game.get('f_base_pop_work_tile')(e.data.base, e.data.pop, e.data.tile);
		return { old_type: old_type, old_tile: old_tile };
	},

	rollback: (e) => {
		e.data.base.set_pop_assignment(e.data.pop, e.applied.old_type, e.applied.old_tile);
	},

};
