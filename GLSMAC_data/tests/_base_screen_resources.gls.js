// All production base-screen sections run. Only asset/DOM construction and
// pointer-event registration are replaced; Base/Tile/resource callbacks are real.
const screen = #include('../default/ui/parts/game/popup/base_screen');
return (game, base, ui) => {
	const listeners = {};
	let hidden = 0;
	const node = (properties) => {
		const result = properties;
		result.children = [];
		result.on = (name, callback) => {};
		result.listen = (object, name, callback) => { listeners[name] = callback; };
		result.show = () => {};
		result.hide = () => {};
		result.clear = () => { result.children = []; };
		const child = (props) => {
			const value = node(props);
			result.children :+value;
			return value;
		};
		result.input = child;
		result.remove = () => {};
		result.window = child;
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
		ui: ui, game: game, hide: () => {}, modules: { bottom_bar: { height: 200 }, popup: {
			hide: (name) => { if (name != 'base_screen') { throw TestError('retirement hid another popup'); } hidden++; screen.on_hide(); },
		} },
		create: (title, width, height, initialize) => { initialize(node({}), () => {}); },
	});
	const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
	const check_retirement = () => {
		screen.on_show();
		const before = hidden;
		listeners.base_despawn({base_id: base.id + 1});
		check(screen.is_open && hidden == before, 'unrelated Base closed this screen');
		listeners.base_despawn({base_id: base.id});
		check(!screen.is_open && hidden == before + 1 && screen.base == null && screen.base_id == null, 'removed Base screen or retained wrapper survived');
		listeners.base_despawn({base_id: base.id});
		check(hidden == before + 1, 'duplicate retirement hid another screen');
	};
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
		check(section_count == 13, 'complete screen sections were not initialized');
		if (#is_defined(expected.canonical) && expected.canonical) {
			check(screen.sections.nutrients.growth_label.text == 'Growth unavailable', 'canonical growth forecast was invented');
			const stored = screen.sections.nutrients.growth_cells.children;
			check(#sizeof(stored) == 1 && stored[0].text == expected.stored, 'wrong canonical stored nutrients');
			check(#sizeof(screen.sections.resources.cells.children) == 174, 'canonical resource cells changed');
			check(screen.sections.resources.labels.nutrients.right.text == expected.nutrients, 'wrong canonical nutrient demand/surplus');
			check(screen.sections.resources.labels.nutrients.left.text == '006 - 007', 'canonical intake/demand was substituted');
			check(screen.sections.resources.intake_caption.text == 'INTAKE - CONSUMPTION (N/A = unavailable)', 'unavailable legend missing');
			check(screen.sections.resources.labels.minerals.left.text == expected.minerals + ' - 000', 'actual mineral support was not shown');
			check(screen.sections.resources.labels.minerals.right.text == '+' + expected.minerals, 'wrong mineral balance');
			for (type of ['energy']) {
				check(screen.sections.resources.labels[type].left.text == expected[type] + ' - N/A', 'actual intake or unavailable consumption missing');
				check(screen.sections.resources.labels[type].right.text == 'N/A', 'canonical surplus was invented');
				for (cell of screen.sections.resources.cells.children) {
					check(cell.class != 'base-screen-resources-cell-' + type + '-profit' && cell.class != 'base-screen-resources-cell-' + type + '-loss', 'unknown cost produced a balance bar');
				}
			}
			for (type of ['labs', 'psych', 'economy']) {
				for (label in screen.sections.energy.labels[type]) {
					check(screen.sections.energy.labels[type][label].text == 'N/A', 'canonical placeholder energy allocation survived');
				}
			}
			const production = screen.sections.bottom_bar.parts.production;
			check(production.production_label.text == 'Forecast unavailable', 'canonical production forecast was invented');
			check(#sizeof(production.production_cells.children) == 1 && production.production_cells.children[0].text == expected.production, 'wrong optional stored minerals');
			const queue = screen.sections.bottom_bar.parts.queue.items.children;
			check(#sizeof(queue) == 1 && queue[0].text == 'Unavailable', 'canonical placeholder production queue survived');
			check(screen.sections.game_state.info_energy.text == 'N/A' && screen.sections.game_state.info_ecodamage.text == 'Eco-Damage: N/A', 'canonical placeholder account or ecodamage survived');
			check(screen.sections.bottom_bar.parts.middle_area.basename.text == base.name, 'canonical screen did not finish');
			check_retirement();
			return screen;
		}
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
		check_retirement();
		return screen;
	};
};
