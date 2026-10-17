// In-memory engine boundaries for the real base rules and population events.
// Keep rule calculations in default/game/bases.gls.js, not in these mocks.
const configure_bases = #include('../default/game/bases');
const population_events = {
	add_base_pop: #include('../default/game/event/add_base_pop'),
	remove_base_pop: #include('../default/game/event/remove_base_pop'),
};

const custom_properties = () => {
	const values = {};
	return {
		get: (key) => { return values[key]; },
		set: (key, value) => { values[key] = value; },
		unset: (key) => { values[key] = #undefined; },
		has: (key) => { return #is_defined(values[key]); },
	};
};

const create_tile = (x, nutrients) => {
	const tile = custom_properties() + {
		x: x,
		y: 0,
	};
	tile.set('nutrients', nutrients);
	tile.get_working_pop = () => { return tile.get('working_pop'); };
	tile.get_resources = () => {
		return {NUTRIENTS: tile.get('nutrients'), MINERALS: 0, ENERGY: 0};
	};
	return tile;
};

const create_fixture = () => {
	const game_handlers = {};
	const base_handlers = {};
	const pops = [];
	const worked = [];
	const events = [];
	const center = create_tile(0, 2);
	const tiles = [create_tile(2, 2), create_tile(4, 4), create_tile(6, 0)];
	const owner = {id: 0};
	let next_pop_id = 1;
	let accumulated_nutrients = 0;
	const base = custom_properties();
	base.has_smac_resource_context = false;
	base.get_accumulated_nutrients = () => { return accumulated_nutrients; };
	base.set_accumulated_nutrients = (value) => { accumulated_nutrients = value; };
	base.get_owner = () => { return owner; };
	base.get_tile = () => { return center; };
	base.get_size = () => { return #sizeof(pops); };
	base.get_pops = () => { return pops; };
	base.get_worked_tiles = () => { return worked; };
	base.get_unworked_tiles = () => {
		let result = [];
		for (tile of tiles) {
			if (!#is_defined(tile.get_working_pop())) {
				result :+ tile;
			}
		}
		return result;
	};
	base.add_worked_tile = (tile) => {
		for (existing of worked) {
			test.assert(existing != tile, 'A worked tile must not be added twice');
		}
		worked :+ tile;
	};
	base.remove_worked_tile = (tile) => {
		let found = false;
		for (let i = 0; i < #sizeof(worked); i++) {
			if (worked[i] == tile) {
				worked :- i;
				found = true;
				break;
			}
		}
		test.assert(found, 'Cannot remove a tile that is not worked');
	};
	base.set_pop_assignment = (pop, type, tile) => {
		const old_tile = pop.get_worked_tile();
		if (#is_defined(old_tile)) {
			old_tile.unset('working_pop');
			base.remove_worked_tile(old_tile);
		}
		pop.set('worked_tile', tile);
		pop.type = type;
		if (#is_defined(tile)) {
			tile.set('working_pop', pop);
			base.add_worked_tile(tile);
		}
	};
	const insert_pop = (id, variant, data) => {
		const pop = custom_properties() + {id: id, variant: variant, type: data.type};
		pop.get_type = () => { return pop.type; };
		pop.get_base = () => { return base; };
		pop.get_worked_tile = () => { return pop.get('worked_tile'); };
		pop.set_type = (type) => { base.set_pop_assignment(pop, type, type == 'WORKER' ? pop.get_worked_tile() : #undefined); };
		pops :+ pop;
		if (#is_defined(data.worked_tile)) {
			base.set_pop_assignment(pop, data.type, data.worked_tile);
		}
		return pop;
	};
	base.create_pop = (data) => {
		const pop = insert_pop(next_pop_id, 0, data);
		next_pop_id++;
		return pop;
	};
	base.restore_pop = (data) => { return insert_pop(data.id, data.variant, data); };
	base.destroy_pop = (pop) => {
		base.set_pop_assignment(pop, pop.get_type(), #undefined);
		let found = false;
		for (let i = 0; i < #sizeof(pops); i++) {
			if (pops[i] == pop) {
				pops :- i;
				found = true;
				break;
			}
		}
		test.assert(found, 'Cannot destroy a missing population');
	};
	base.get_intake = () => { return base_handlers.get_base_intake({base: base}); };
	base.get_consumption = () => { return base_handlers.get_base_consumption({base: base}); };

	const bm = {
		on: (name, handler) => { base_handlers[name] = handler; },
		get_bases: () => { return [base]; },
	};
	const tm = {
		get_map_width: () => { return 20; },
		get_map_height: () => { return 20; },
	};
	const game = custom_properties() + {
		get_ruleset: () => { return 'smac'; },
		configure_smac_base_presentations: (roles) => {}, // definitions are substituted
		get_bm: () => { return bm; },
		get_tm: () => { return tm; },
		on: (name, handler) => { game_handlers[name] = handler; },
	};
	game.event = (name, data) => {
		// Definitions need no engine assets for these population-rule tests.
		if (name == 'define_base_pop' || name == 'define_base_facility') {
			return;
		}
		const handler = population_events[name];
		test.assert(#is_defined(handler), 'Unexpected population event: ' + name);
		const event = {game: game, caller: 0, data: data, name: name};
		test.assert(!#is_defined(handler.validate(event)), 'Population event must validate');
		event.applied = handler.apply(event);
		events :+ event;
		return event;
	};

	configure_bases(game);
	game_handlers.start({game: game});
	return {
		game: game,
		base: base,
		center: center,
		tiles: tiles,
		get_events: () => { return events; },
		seed_pop: (type, tile) => {
			const pop = base.create_pop({type: type});
			if (#is_defined(tile)) {
				game.get('f_base_pop_work_tile')(base, pop, tile);
			}
			return pop;
		},
		turn: () => { game_handlers.turn({}); },
		rollback: (event) => { population_events[event.name].rollback(event); },
	};
};

return {create_fixture: create_fixture, events: population_events};
