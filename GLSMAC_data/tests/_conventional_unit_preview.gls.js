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
		if (!#is_defined(result.width)) { result.width = 300; }
		result.on = (name, callback) => { result.handlers[name] = callback; };
		result.listen = (object, name, callback) => { object.on(name, callback); };
		result.show = () => {};
		result.hide = () => {};
		result.remove = () => {};
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
		ui: { class: (name) => { return style; } },
		modules: {
			menu: { on_close: (callback) => {} },
			popup: { hide: (name) => {} },
		},
	});
	const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
	let previous_item = null;
	let previous_unit = null;
	return (unit, expected) => {
		const sections = bottom_bar.pp.sections;
		const lines = sections.object_preview.lines.children;
		check(#sizeof(lines) == (unit.is_immovable ? 3 : 4), 'wrong preview line count');
		check(lines[0].text == unit.get_def().name, 'wrong unit name');
		check(lines[1].text == expected.stats, 'wrong preview stats: ' + lines[1].text);
		check(lines[2].text == expected.morale, 'wrong preview morale: ' + lines[2].text);
		if (!unit.is_immovable) { check(lines[3].text == expected.moves, 'wrong remaining movement: ' + lines[3].text); }
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
	};
};
