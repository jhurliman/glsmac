// Real research UI callbacks; only DOM and reset construction are doubled.
const research = #include('../default/ui/parts/game/research/research');
return (game, reset) => {
	let popup = null;
	const node = (properties) => {
		const result = properties;
		result.handlers = {};
		result.children = [];
		result.on = (name, callback) => { result.handlers[name] = callback; };
		result.listen = (object, name, callback) => { object.on(name, callback); };
		result.visible = false;
		result.show = () => { result.visible = true; };
		result.hide = () => { result.visible = false; };
		result.clear = () => { result.children = []; };
		const child = (props) => { const value = node(props); result.children :+value; return value; };
		result.window = child;
		result.scrollview = child;
		result.button = child;
		if (!#is_defined(result.text)) { result.text = child; }
		return result;
	};
	research.init({game: game, root: node({}), glsmac: {reset: reset}, modules: {menu: {close_all: () => {}},
		popup: {show: (name, callback) => { popup = {name: name, callback: callback}; }}}});
	return {
		state: () => { return {pending: research.pending, selected: research.choice, submitted: research.submitted, prompt: research.prompt.text, labels: research.choice_buttons, visible: research.window.visible}; },
		choose: (index) => { research.choice_buttons[index].handlers.click({}); },
		confirm: () => { research.confirm_button.handlers.click({}); },
		key: (code, modifiers) => { research.window.handlers.keydown({code: code, modifiers: modifiers}); },
		reset: () => { research.reset_button.handlers.click({}); },
		save: () => { research.save_button.handlers.click({}); },
		popup: () => { return popup; },
	};
};
