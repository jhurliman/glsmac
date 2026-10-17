// Called with real backend wrappers after State -> Packet -> State decoding.
return (game, settings, expected) => {
	const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
	check(game.get_ruleset() == expected, 'game.get_ruleset lost host identity');
	check(settings.ruleset == expected, 'global settings wrapper lost host identity');
	let rejected = false;
	try { settings.ruleset = 'smax'; } catch {
		GSEInvalidAssignment: (e) => { rejected = true; },
	}
	check(rejected, 'ruleset can be changed through the generic settings wrapper');
	check(game.get_ruleset() == expected, 'rejected write mutated identity');
};
