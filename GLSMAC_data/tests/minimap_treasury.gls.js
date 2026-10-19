const create = #include('_restored_minimap');
for (initial of [137, 0, (0 - 7), #undefined]) {
	let credits = initial;
	let reads = 0;
	let listeners = {};
	const game = {
		get_year: () => { return 2137; },
		get_turn_status: () => { return 'active'; },
		get_smac_minimap: () => { return #undefined; },
    get_smac_detail_minimap: () => { return #undefined; },
		get_smac_credits: () => { reads++; return credits; },
		on: (event, callback) => { listeners[event] = callback; },
	};
	const read = create(game);
	const expected = #is_defined(initial) ? 'Energy: ' + #to_string(initial) : 'Energy: N/A';
	test.assert((read()).energy == expected && (read()).year == 'Mission Year 2137' && reads == 1,
		'initial/restored minimap did not query actual treasury');
	credits = 246;
	listeners.turn({year: 2138});
	test.assert((read()).energy == 'Energy: 246' && (read()).year == 'Mission Year 2138' && reads == 2);
	credits = 249;
	listeners.turn_status({status: 'please_wait'});
	test.assert((read()).energy == 'Energy: 249' && (read()).text == 'PLEASE WAIT' && reads == 3);
	credits = #undefined;
	listeners.turn_status({status: 'active'});
	test.assert((read()).energy == 'Energy: N/A' && reads == 4, 'unavailable session retained a stale balance');
	// Capture and razing can change treasury during an ordinary active turn.
	credits = 349;
	listeners.update_base({});
	test.assert((read()).energy == 'Energy: 349' && reads == 5 && (read()).text == 'TURN COMPLETE',
		'in-turn Base update retained the old treasury or changed turn status');
	credits = 359;
	listeners.base_despawn({base_id: 701});
	test.assert((read()).energy == 'Energy: 359' && reads == 6, 'Base removal retained the old treasury');
	credits = #undefined;
	listeners.update_base({});
	test.assert((read()).energy == 'Energy: N/A' && reads == 7, 'Base update retained an unavailable balance');
}
