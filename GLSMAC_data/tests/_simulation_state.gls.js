// Real GSE consumers of startup state. Mutating these copies must not mutate Game.
return (game) => {
	let observations = [];
	const observe = (phase) => {
		let participants = [];
		for (slot of game.get_players()) {
			const before = game.get_technology_state(slot);
			const id = game.get_simulation_faction_id(slot);
			let copy = game.get_technology_state(slot);
			copy.labs = 7654321;
			copy.owned_technologies[2] = !copy.owned_technologies[2];
			const after = game.get_technology_state(slot);
			const designs_before = game.get_startup_designs(slot);
			let designs_copy = game.get_startup_designs(slot);
			designs_copy.global_difficulty = 123;
			designs_copy.starting_scout = 511;
			if (#sizeof(designs_copy.designs) > 0) {
				designs_copy.designs[0].cost = 99;
				designs_copy.designs[0].active = !designs_copy.designs[0].active;
				designs_copy.designs[0].prototyped = !designs_copy.designs[0].prototyped;
			}
			const designs_after = game.get_startup_designs(slot);
			participants :+ {slot: slot.id, id: id, before: before, after: after, designs_before: designs_before, designs_after: designs_after};
		}
		observations :+ {phase: phase, turn: game.get_turn(), participants: participants};
	};
	game.on('create_world', (e) => { observe('create'); });
	game.on('start', (e) => { observe('start'); });
	return () => { return observations; };
};
