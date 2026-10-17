// Real choice callbacks; only DOM construction and reset are doubled.
const colony = #include('../default/ui/parts/game/colony/colony');
return (game, reset) => {
	let popup = null;
	const popups = {
		clear: () => { popup = null; },
		set: (name, data) => {},
		show: (name, callback) => { popup = {name: name, callback: callback}; },
	};
	const node = (properties) => {
		const result = properties;
		result.handlers = {};
		result.children = [];
		result.visible = false;
		result.on = (name, callback) => { result.handlers[name] = callback; };
		result.listen = (object, name, callback) => { object.on(name, callback); };
		result.show = () => { result.visible = true; };
		result.hide = () => { result.visible = false; };
		result.clear = () => { result.children = []; };
		const child = (props) => { const value = node(props); result.children :+value; return value; };
		result.window = child;
		result.panel = child;
		result.button = child;
		if (!#is_defined(result.text)) { result.text = child; }
		return result;
	};
	colony.init({game: game, root: node({}), glsmac: {reset: reset}, modules: {menu: {close_all: () => {}}, popup: popups}});
	return {
		state: () => { return {pending: colony.pending, submitted: colony.submitted, visible: colony.window.visible, buttons: colony.choice_buttons}; },
		choose: (index) => { colony.choice_buttons[index].handlers.click({}); },
		key: (code, modifiers) => { colony.window.handlers.keydown({code: code, modifiers: modifiers}); },
		reset: () => { colony.reset_button.handlers.click({}); },
		save: () => { colony.save_button.handlers.click({}); },
		popup: () => { return popup; },
	};
};
