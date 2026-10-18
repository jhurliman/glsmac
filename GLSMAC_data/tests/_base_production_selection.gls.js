// The real picker and OK/Cancel handlers, backed by actual Base and Game wrappers.
// Only DOM construction is doubled; no target/cost/retool/event model lives here.
const screen = #include('../default/ui/parts/game/popup/base_screen');
const buttons = #include('../default/ui/parts/game/popup/base_screen/buttons');
return (game, base) => {
	const node = (properties) => {
		const result = properties;
		result.children = [];
		result.handlers = {};
		result.on = (name, callback) => { result.handlers[name] = callback; };
		result.remove = () => {};
		result.show = () => {};
		result.hide = () => {};
		const child = (properties) => { const value = node(properties); result.children :+value; return value; };
		result.panel = child;
		result.listview = child;
		result.button = child;
		if (!#is_defined(result.text)) { result.text = child; }
		return result;
	};
	const style = {set: (properties) => {}};
	style.extend = (name) => { return style; };
	screen.base = base;
	screen.body = node({});
	screen.p = {game: game};
	screen.production_selection = null;
	screen.sections = {buttons: buttons};
	buttons.init({body: screen.body, ui: {class: (name) => { return style; }}, hide: () => {}, utils: {
		hide_production_selection: () => { screen.hide_production_selection(); },
		update_production_selection: () => { screen.update_production_selection(); },
	}});
	const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
	return (source_id, action) => {
		if (action == 'open') {
			screen.show_production_selection();
			let position = 0;
			let found = false;
			for (choice of screen.production_data.choices) {
				if (choice.source_id == source_id) {
					screen.production_choices.children[position].handlers.click({});
					found = true;
				}
				position++;
			}
			check(found, 'actual eligible production choice missing');
		} else if (action == 'ok') {
			buttons.buttons[2].handlers.click({});
		} else if (action == 'cancel') {
			buttons.buttons[3].handlers.click({});
		} else { throw TestError('unknown picker action'); }
		return screen.production_selection != null;
	};
};
