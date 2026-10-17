// Exercise the actual default producer and definition handler with a native
// manager. Only event queue delivery is replaced by synchronous dispatch.
const facilities = #include('../default/game/facilities');
const handler = #include('../default/game/event/define_base_facility');
return (game) => {
	facilities.define({
		get_ruleset: game.get_ruleset,
		event: (name, data) => {
			if (name != 'define_base_facility') { throw TestError('Unexpected facility event'); }
			const event = { game: { bm: game.get_bm() }, caller: 0, data: data };
			if (#is_defined(handler.validate(event))) { throw TestError('Facility event rejected'); }
			handler.apply(event);
		},
	});
};
