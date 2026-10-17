// Actual command-panel callbacks, with only DOM/style construction replaced.
return (game) => {
	const panel = #include('../default/ui/parts/game/bottom_bar/unit_commands');
	const node = (properties) => {
		const result = properties;
		result.children = [];
		result.handlers = {};
		result.width = 600;
		result.on = (name, callback) => { result.handlers[name] = callback; };
		result.listen = (object, name, callback) => { object.on(name, callback); };
		let visible = true;
		result.show = () => { visible = true; };
		result.hide = () => { visible = false; };
		result.is_visible = () => { return visible; };
		result.clear = () => { result.children = []; };
		const child = (props) => {
			const value = node(props);
			result.children :+value;
			return value;
		};
		result.panel = child;
		result.button = child;
		if (!#is_defined(result.text)) { result.text = child; }
		return result;
	};
	const style = {set: (properties) => {}};
	style.extend = (name) => { return style; };
	panel.init({game: game, map: game.get_map(), el: node({}), ui: {class: (name) => {return style;}}});
	const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
	let remembered = null;
	return (operation, expected, heading) => {
		if (operation == 'hidden') { check(!panel.frame.is_visible(), 'commands remained visible'); return; }
		if (operation == 'remembered') { check(remembered != null, 'missing saved callback'); remembered({}); return; }
		if (operation == 'check') {
			check(panel.frame.is_visible(), 'commands are hidden');
			let labels = '';
			for (button of panel.buttons) { if (labels != '') { labels += '|'; } labels += button.text; }
			check(labels == expected, 'wrong commands: ' + labels);
			check(panel.frame.children[0].text == heading, 'wrong saved order: ' + panel.frame.children[0].text);
			return;
		}
		for (button of panel.buttons) {
			if (button.text == expected) {
				if (operation == 'remember') { remembered = button.handlers.click; }
				else { check(operation == 'click', 'unknown operation'); button.handlers.click({}); }
				return;
			}
		}
		throw TestError('command button not found: ' + expected);
	};
};
