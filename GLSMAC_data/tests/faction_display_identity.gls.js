const factions = #include('../default/factions');
for (ruleset of ['smac', 'smax']) {
	let imports = [];
	let configured = [];
	factions.configure({
		import_display_identity: (filename, section) => {
			imports :+ [filename, section];
			return {faction_name: 'Source ' + filename, leader_title: 'Title', leader_name: section};
		},
		import_colors: (filename) => { return {}; },
		import_base_names: (filename) => { return {land: [], water: []}; },
		add: (id, data) => { configured :+ [id, data]; },
	}, ruleset);
	const roster = factions.get_roster(ruleset);
	test.assert(#sizeof(configured) == #sizeof(roster) && #sizeof(imports) == #sizeof(roster));
	for (let index = 0; index < #sizeof(roster); index++) {
		const original = roster[index];
		const request = imports[index];
		const result = configured[index];
		test.assert(request == [original[1] + '.txt', #uppercase(original[1])]);
		test.assert(result[0] == #uppercase(original[0]));
		test.assert(result[1].display_identity == {faction_name: 'Source ' + request[0], leader_title: 'Title', leader_name: request[1]});
	}
}
