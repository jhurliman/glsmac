const factions = [
	['Gaians', 'gaians', {}],
	['Hive', 'hive', {}],
	['University', 'univ', {}],
	['Morganites', 'morgan', {}],
	['Spartans', 'spartans', {}],
	['Believers', 'believe', {}],
	['Peacekeepers', 'peace', {}],
	['Consciousness', 'cyborg', {}],
	['Pirates', 'pirates', {is_naval: true}],
	['Drones', 'drone', {}],
	['Angels', 'angels', {}],
	['Planetcult', 'fungboy', {}],
	['Caretakers', 'caretake', {is_progenitor: true}],
	['Usurpers', 'usurper', {is_progenitor: true}],
];

const get_roster = (ruleset) => {
	if (ruleset != 'smac' && ruleset != 'smax') {
		throw Error('Unknown gameplay ruleset: ' + ruleset);
	}
	let roster = [];
	const count = ruleset == 'smac' ? 7 : #sizeof(factions);
	for (let idx = 0; idx < count; idx++) {
		// Bind the entry value: appending an indexed expression retains its reference.
		const faction = factions[idx];
		roster :+faction;
	}
	return roster;
};

return {
	get_roster: get_roster,

	configure: (fm, ruleset) => {
		for (f of get_roster(ruleset)) {
			fm.add(#uppercase(f[0]), {
				name: f[0],
				colors: fm.import_colors(f[1] + '.pcx'),
				bases: {
					render: {
						type: 'sprite_grid',
						file: f[1] + '.pcx',
						grid_x: 1, grid_y: 1,
						cell_width: 100, cell_height: 75,
						cell_padding: 1,
					},
					names: fm.import_base_names(f[1] + '.txt'),
				},
			} + f[2]);
		}
	},

};
