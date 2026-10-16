// Called by the backend fixture with a real Base wrapper inside a write scope.
const initialize_bases = #include('../default/game/bases');
const add_pop = #include('../default/game/event/add_base_pop');
const remove_pop = #include('../default/game/event/remove_base_pop');

return (base) => {
	const check = (condition, reason) => {
		if (!condition) { throw TestError(reason); }
	};
	let start = null;
	const helpers = {};
	// Only initialization services are substituted. Nutrient access, population
	// creation/destruction, and the production event handlers remain real.
	const game = {
		get_bm: () => { return { on: (name, callback) => {} }; },
		get_tm: () => { return { get_map_width: () => { return 8; }, get_map_height: () => { return 8; } }; },
		event: (name, data) => {},
		on: (name, callback) => { if (name == 'start') { start = callback; } },
		set: (name, value) => { helpers[name] = value; },
		get: (name) => { return helpers[name]; },
	};
	initialize_bases(game);
	start({});
	check(base.get_size() == 1, 'expected one initial population');
	for (nutrients of [0, 19, 20, 37]) {
		base.set_accumulated_nutrients(nutrients);
		helpers.f_base_reset_nutrients(game, base);
		const expected = nutrients > 20 ? nutrients - 20 : 0;
		check(base.get_accumulated_nutrients() == expected, 'growth subtraction or zero clamp changed');
	}
	for (nutrients of [0, 37]) {
		base.set_accumulated_nutrients(nutrients);
		const data = { base: base, type: 'DOCTOR' };
		const applied = add_pop.apply({ game: game, data: data });
		check(base.get_size() == 2, 'add event did not create population');
		const expected = nutrients > 20 ? nutrients - 20 : 0;
		check(base.get_accumulated_nutrients() == expected, 'add event did not store nutrient cost');
		add_pop.rollback({ game: game, data: data, applied: applied });
		check(base.get_size() == 1, 'add rollback did not remove population');
		check(base.get_accumulated_nutrients() == nutrients, 'add rollback did not restore nutrients');
		const remove_data = { base: base, pop: (base.get_pops())[0] };
		const removed = remove_pop.apply({ game: game, data: remove_data });
		check(base.get_size() == 0, 'remove event did not remove population');
		check(base.get_accumulated_nutrients() == expected, 'remove event did not store nutrient cost');
		remove_pop.rollback({ game: game, data: remove_data, applied: removed });
		check(base.get_size() == 1, 'remove rollback did not recreate population');
		check(base.get_accumulated_nutrients() == nutrients, 'remove rollback did not restore nutrients');
	}
};
