// Real Player/Faction wrappers and registered callbacks; only DOM construction
// is doubled. Source identity must never come from the controller's roster name.
const view = #include('../default/ui/parts/game/bottom_bar/middle_area/info_panels');
return (game) => {
	const node = (properties) => {
		const result = properties;
		result.width = 480;
		result.on = (name, callback) => {};
		result.listen = (object, name, callback) => { object.on(name, callback); };
		result.panel = node;
		result.button = node;
		if (!#is_defined(result.text)) { result.text = node; }
		return result;
	};
	const style = {set: (properties) => {}};style.extend = (name) => { return style; };
	view.init({game: game, frame: node({}), ui: {class: (name) => { return style; }}});
	return (name, leader) => {
		view.on_show();
		if (view.labels[0].text != name || view.labels[1].text != leader) { throw TestError('Console source faction/leader identity differs'); }
		if (view.labels[2].text != 'Integrity: unavailable' || view.labels[3].text != 'Might: unavailable') { throw TestError('Console invented an unimplemented rating'); }
	};
};
