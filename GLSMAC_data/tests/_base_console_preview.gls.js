// Real Base wrappers and published Game/Map callbacks; only DOM/font metrics
// are doubled. Native caller checks that displaying stores never mutates them.
const preview = #include('../default/ui/parts/game/bottom_bar/object_preview');
const roster = #include('../default/ui/parts/game/bottom_bar/objects_list');
return (game) => {
	const node = (properties) => {
		const result = properties;
		result.children = [];
		if (!#is_defined(result.width)) { result.width = 129; }
		result.remove = () => {};
        result.on = (name, callback) => {};
		result.listen = (object, name, callback) => { object.on(name, callback); };
		result.measure = () => { return {width: #sizeof(#split(result.text, ' ')) * 40, height: 10}; };
		const child = (properties) => { const value = node(properties); result.children :+ value; return value; };
		result.panel = child;
		result.area = child;
		result.widget = child;
		result.listview = child;
		if (!#is_defined(result.text)) { result.text = child; }
		return result;
	};
	preview.init({game: game, map: game.get_map(), el: node({}), ui: {class: (name) => { return {set: (properties) => {}}; }}});
	return (base, expected) => {
		if (base == null) {
			if (preview.object != null || preview.preview != null || #is_defined(preview.lines)) { throw TestError('Retired Base remained in console'); }
			return;
		}
		if (preview.object_id != base.id || preview.preview.data.base != base) { throw TestError('Console lost inspected Base'); }
		if (!#is_defined(preview.preview.data.card) || preview.preview.data.card != true) { throw TestError('Lower console Base caller omitted card context'); }
        roster.list = node({}); roster.bases = {}; roster.units = {}; roster.selected_object = null; roster.active_item = null;
        roster.add_object(base, 0);
        const item = roster.list.children[0];
        const card = item.children[0];
        if (!#is_defined(card.data.card) || card.data.card != true || card.data.roster != true || card.height != 48) { throw TestError('Lower console Base roster omitted its native card context'); }
		const rows = preview.lines.children;
		if (rows[0].text != base.name) { throw TestError('Console Base name stale'); }
		let description = '';
		for (let i = 1; i < #sizeof(rows); i++) {
			for (word of #split(rows[i].text, ' ')) {
				if (word != '') { description += (description == '' ? '' : ' ') + word; }
			}
		}
		if (description != expected) { throw TestError('Wrong production text: ' + description); }
	};
};
