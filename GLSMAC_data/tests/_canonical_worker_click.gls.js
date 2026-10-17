// Actual resource-widget hit testing and game.event; only DOM/assets are doubles.
const resource = #include('../default/ui/parts/game/popup/base_screen/middle_area/resource');
return (game, base) => {
	const handlers = {};
	const panel = {
		clear: () => {}, widget: (properties) => { return properties; },
		on: (name, callback) => { handlers[name] = callback; },
	};
	resource.init({
		game: game, area: { panel: (properties) => { return panel; } },
		ui: { class: (name) => { return { set: (properties) => {} }; } },
	});
	return (x, y) => {
		resource.set({ base: base });
		handlers.mousedown({ button: 'left', x: x, y: y });
	};
};
