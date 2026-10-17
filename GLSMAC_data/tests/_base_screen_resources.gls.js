// All production base-screen sections run. Only asset/DOM construction and
// pointer-event registration are replaced; Base/Tile/resource callbacks are real.
const screen = #include('../default/ui/parts/game/popup/base_screen');
return (game, base, ui) => {
	const node = (properties) => {
		const result = properties;
		result.children = [];
		result.on = (name, callback) => {};
		result.listen = (object, name, callback) => {};
		result.show = () => {};
		result.hide = () => {};
		result.clear = () => { result.children = []; };
		const child = (props) => {
			const value = node(props);
			result.children :+value;
			return value;
		};
		result.panel = child;
		result.area = child;
		result.button = child;
		result.surface = child;
		result.widget = child;
		result.listview = child;
		if (!#is_defined(result.text)) { result.text = child; }
		return result;
	};
	const style = { set: (properties) => {} };
	style.extend = (name) => { return style; };
	ui.class = (name) => { return style; };
	ui.root = node({});
	game.get_year = () => { return 2100; };
	screen.init({
		ui: ui, game: game, hide: () => {}, modules: { bottom_bar: { height: 200 } },
		create: (title, width, height, initialize) => { initialize(node({}), () => {}); },
	});
	const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
	const check_cells = (cells, prefix, expected) => {
		const counts = { full: 0, pending: 0, deficit: 0, empty: 0 };
		for (cell of cells.children) {
			let found = false;
			for (kind in counts) {
				if (cell.class == prefix + kind) { counts[kind] = counts[kind] + 1; found = true; }
			}
			check(found, 'unknown progress cell class');
		}
		for (kind in counts) { check(counts[kind] == expected[kind], 'wrong ' + kind + ' progress cell count'); }
	};
	return (expected) => {
		screen.set({ base: base });
		let section_count = 0;
		for (section in screen.sections) { section_count++; }
		check(section_count == 10, 'complete screen sections were not initialized');
		check(screen.sections.nutrients.growth_label.text == expected.growth, 'wrong growth label: ' + screen.sections.nutrients.growth_label.text);
		check_cells(screen.sections.nutrients.growth_cells, 'base-screen-nutrients-cell-', expected.cells);
		check(#sizeof(screen.sections.resources.cells.children) == 174, 'resource cell count changed with yield magnitude');
		check(screen.sections.resources.labels.nutrients.right.text == expected.nutrients, 'wrong exact nutrient balance');
		check(screen.sections.resources.labels.energy.right.text == expected.energy, 'wrong exact energy balance');
		for (type of ['labs', 'psych', 'economy']) {
			const labels = screen.sections.energy.labels[type];
			const text = #pad(#to_string(expected[type]), 'left', 3, ' ');
			check(labels.value.text == text && labels.total.text == text, 'wrong energy allocation: ' + type);
			check(labels.bonus.text == '  0', 'placeholder energy bonus changed');
			check(labels.percentage.text == (type == 'psych' ? ' 20%' : ' 40%'), 'placeholder percentage changed');
		}
		const production = screen.sections.bottom_bar.parts.production;
		check(production.production_label.text == '6 TURNS', 'placeholder production rounding changed');
		check_cells(production.production_cells, 'base-screen-bottombar-production-cell-', { full: 8, pending: 4, deficit: 0, empty: 18 });
		check(screen.sections.bottom_bar.parts.middle_area.basename.text == base.name, 'screen did not finish its final section');
		return screen;
	};
};
