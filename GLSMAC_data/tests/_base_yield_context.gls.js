// Real base-screen layout and worker scoring, with a recording widget factory.
const panel = #include('../default/ui/parts/game/popup/base_screen/middle_area/resource');
return (game, base, other, target, query) => {
	const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
	check(target.get_resources().ENERGY == 100, 'legacy default player changed');
	check(target.get_resources(base.get_owner()).ENERGY == 102, 'legacy explicit player changed');
	check(target.get_resources(base.get_owner(), base).ENERGY == 1, 'explicit base context missing');
	check(target.get_resources(other.get_owner(), other).ENERGY == 9, 'base identity did not affect the same tile');
	const prospective = target.get_N();
	check(!#is_defined(prospective.get_working_pop()), 'prospective tile must start unassigned');
	check(prospective.get_resources(base.get_owner(), base).ENERGY == 1, 'unassigned tile lost evaluation context');
	const finder = game.get('f_base_find_best_or_worst_tiles');
	const center = base.get_tile();
	check((finder(base, [center, target], 1, 1))[0] == center, 'worker scoring lost first base context');
	check((finder(other, [center, target], 1, 1))[0] == target, 'worker scoring lost second base context');
	let resources = 0;
	panel.el = {
		clear: () => {},
		on: (name, callback) => {},
		widget: (properties) => {
			if (properties.type == 'tile-resources') {
				check(properties.data.base == base, 'base-screen widget omitted its evaluating base');
				query(properties.data);
				resources++;
			}
			return {};
		},
	};
	panel.set({ base: base });
	check(resources == 2, 'base-screen did not preview center and worked tile');
};
