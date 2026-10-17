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
			participants :+ {slot: slot.id, id: id, before: before, after: after};
		}
		observations :+ {phase: phase, turn: game.get_turn(), participants: participants};
	};
	game.on('create_world', (e) => { observe('create'); });
	game.on('start', (e) => { observe('start'); });
	return () => { return observations; };
};
