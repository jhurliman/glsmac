return {

	validate: (e) => {
		const unit = e.data.unit;
		if (unit.is_conventional) { return 'Conventional turn commands are not activated'; }
		if (unit.owner != e.caller) {
			return 'Unit can only be moved by it\'s owner';
		}
		if (unit.get_tile().is_locked()) {
			return 'Unit tile is locked';
		}
		if (unit.movement <= 0.0) {
			return 'Unit is out of moves';
		}
	},

	apply: (e) => {
		const unit = e.data.unit;
		if (unit.is_conventional) { throw Error('Conventional turn commands are not activated'); }

		const result = {
			original_movement: unit.movement,
		};

		unit.movement = 0.0;

		return result;
	},

	rollback: (e) => {
		const unit = e.data.unit;
		if (unit.is_conventional) { throw Error('Conventional turn commands are not activated'); }
		unit.movement = e.applied.original_movement;
	},

};
