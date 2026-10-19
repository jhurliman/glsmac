// The complete production bottom bar registers its real Game/Map callbacks.
// Only asset/DOM, sound and timer construction are replaced; Unit wrappers,
// definitions and checked queries are supplied by the native fixture.
const bottom_bar = #include('../default/ui/parts/game/bottom_bar/bottom_bar');
return (game) => {
	let next_node_id = 1;
	const node = (properties) => {
		const result = properties;
		result.node_id = next_node_id;
		next_node_id++;
		result.children = [];
		result.get_children = () => { return result.children; };
		result.handlers = {};
		result.measure = () => { return {width: 20, height: 10}; };
		if (!#is_defined(result.width)) { result.width = 300; }
		result.on = (name, callback) => { result.handlers[name] = callback; };
		result.listen = (object, name, callback) => { object.on(name, callback); };
		result.show = () => {};
		result.hide = () => {};
		let removed = false;
		result.remove = () => { removed = true; };
		result.was_removed = () => { return removed; };
		result.clear = () => { result.children = []; };
		result.timer = (delay, callback) => { return { stop: () => {} }; };
		result.sound = (properties) => { return { play: () => {} }; };
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
		result.scrollview = child;
		if (!#is_defined(result.text)) { result.text = child; }
		return result;
	};
	const style = { set: (properties) => {} };
	style.extend = (name) => { return style; };
	bottom_bar.init({
		game: game, map: game.get_map(), root: node({}),
		ui: { get_width: () => { return 1024; }, class: (name) => { return style; } },
		modules: {
			menu: { on_close: (callback) => {} },
			popup: { hide: (name) => {} },
		},
	});
	const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
	check(bottom_bar.pp.format_points(3, 3) == '1' && bottom_bar.pp.format_points(2, 3) == '0 2/3'
		&& bottom_bar.pp.format_points(1, 3) == '0 1/3' && bottom_bar.pp.format_points(5, 3) == '1 2/3'
		&& bottom_bar.pp.format_points(2, 4) == '0 2/4', 'Native movement fraction formatting differs');
	let previous_item = null;
	let previous_unit = null;
	let previous_list = null;
	return (unit, expected) => {
		const sections = bottom_bar.pp.sections;
		if (#is_defined(expected.retired_id)) {
			const list = sections.objects_list;
			const retired_key = #to_string(expected.retired_id);
			const survivor_key = #to_string(unit.id);
			check(!#is_defined(list.units[retired_key]), 'retired Unit retained its clickable row');
			check(list.units[survivor_key].object == unit, 'same-tile survivor disappeared');
			check(previous_list.was_removed(), 'old clickable list was not removed');
			check(list.selected_object == null, 'retired selected Unit remains selected');
			previous_unit = null;
			previous_item = null;
			return;
		}
		const terrain = sections.tile_preview;
		check(terrain.preview.left == 2 && terrain.preview.top == 1 && terrain.preview.width == 80 && terrain.preview.height == 40,
			'terrain preview lost native80x40 extent/anchor');
		check(terrain.lines.children[0].top + terrain.lines.children[0].measure().height == 56
			&& terrain.bottom_line.top + terrain.bottom_line.measure().height == 113, 'terrain baselines drifted into preview');
		const lines = sections.object_preview.lines.children;
		const held = unit.is_conventional && unit.get_conventional_state().order == 2;
		check(#sizeof(lines) == (unit.is_immovable ? 4 : (held ? 6 : 5)), 'wrong preview line count');
		check(lines[0].font == 'arialbd.ttf:12:mono' && lines[1].font == 'arial.ttf:12:mono', 'unit card lost native font distinction');
		check(lines[0].top + lines[0].measure().height == 73 && lines[1].top + lines[1].measure().height == 86, 'unit card baselines drifted');
		if (unit.is_conventional) { check(lines[3].text == 'Independent', 'homeless unit lost source-backed home row'); }
		if (held) { check(lines[5].text == 'Hold', 'live Hold order row missing'); }
		check(lines[0].text == unit.get_def().name, 'wrong unit name');
		const legacy_artillery = !unit.is_conventional && !unit.is_native && unit.get_def().id == 'SporeLauncher';
		check(lines[1].text == (legacy_artillery ? expected.stats : '(' + expected.stats + ')'), 'wrong preview stats: ' + lines[1].text);
		check(lines[2].text == expected.morale, 'wrong preview morale: ' + lines[2].text);
		if (!unit.is_immovable) { check(lines[4].text == expected.moves, 'wrong remaining movement: ' + lines[4].text); }
		if (expected.moves == 'ALREADY MOVED') { check(lines[4].color == 'rgb(255,0,0)', 'Exhaustion lost native red warning'); }
		check(sections.object_preview.preview.data.unit == unit, 'preview points at a different Unit');
		const key = #to_string(unit.id);
		const entry = sections.objects_list.units[key];
		let stats_label = null;
		for (child of entry.item.get_children()) {
			if (#is_defined(child.class) && child.class == 'bottombar-objects-list-item-text') { stats_label = child; }
		}
		// A real DOM wrapper aliases its native text; a plain GSE object is a
		// value snapshot. Observe the retained handle used by the actual callback.
		if (#is_defined(entry.stats_label)) { stats_label = entry.stats_label; }
		check(stats_label != null, 'same-tile list stats label missing');
		check(stats_label.text == expected.stats, 'same-tile list stats were not refreshed: ' + #to_string(stats_label.text));
		check(sections.objects_list.selected_object == unit && sections.objects_list.active_item == entry.item, 'selection handler lost the actual Unit');
		if (previous_unit == unit) { check(previous_item == entry.item.node_id, 'refresh replaced the existing same-tile item'); }
		previous_unit = unit;
		previous_item = entry.item.node_id;
		previous_list = sections.objects_list.list;
	};
};
