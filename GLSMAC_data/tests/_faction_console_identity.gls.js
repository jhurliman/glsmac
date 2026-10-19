// Real Player/Faction wrappers and registered callbacks; only DOM construction
// is doubled. Source identity must never come from the controller's roster name.
const view = #include('../default/ui/parts/game/bottom_bar/middle_area/info_panels');
return (game) => {
	const node = (properties) => {
		const result = properties;
		result.width = 734; // deliberately stale construction width, as in DOM wrappers
		result.on = (name, callback) => {};
		result.listen = (object, name, callback) => { object.on(name, callback); };
		result.panel = node;
		result.button = node;
        result.surface = node;
        result.hide = () => {}; result.show = () => {};
        result.smacdominancegraph = (frame) => { return true; };
		result.measure = () => {
			let width = 100;
			if (result.text == 'Integrity: ') { width = 48; }
			if (result.text == 'Might: ') { width = 35; }
			return {width: width, height: 11};
		};
		if (!#is_defined(result.text)) { result.text = node; }
		return result;
	};
	let styles = {};
	const style_for = (name) => {
		const style = {set: (properties) => { styles[name] = properties; }};
		style.extend = (base) => { return style; };
		return style;
	};
	view.init({game: game, get_info_width: () => { return 455; }, frame: node({}), ui: {class: style_for}});
	return (name, leader) => {
		view.on_show();
		if (view.panels[0].width != 226 || view.panels[1].width != 226 || view.panels[1].left != 229) { throw TestError('Info columns used stale wrapper geometry'); }
		if (view.labels[0].text != name || view.labels[1].text != leader) { throw TestError('Console source faction/leader identity differs'); }
		if (view.labels[2].text != 'Integrity: ' || view.labels[3].text != 'Might: ' || view.labels[4].text != 'Planetary Council Votes: ') { throw TestError('Native rating prefixes differ'); }
		for (let row = 0; row < 5; row++) {
			if (view.labels[row].left != 5 || view.labels[row].top + view.labels[row].measure().height != 14 + row * 13) { throw TestError('Faction text baseline/spacing differs'); }
		}
		const summary = game.get_smac_faction_summary();
		let expected_values = ['unavailable', 'unavailable', 'unavailable'];
		if (#is_defined(summary)) {
			if (#is_defined(summary.integrity)) { expected_values[0] = summary.integrity; }
			if (#is_defined(summary.might)) { expected_values[1] = summary.might; }
			if (#is_defined(summary.council_votes)) { expected_values[2] = #to_string(summary.council_votes); }
		}
		for (let value_index = 0; value_index < 3; value_index++) {
			if (view.values[value_index].text != expected_values[value_index]) { throw TestError('Console lost real summary or invented missing authority'); }
		}
		if (view.values[0].left != 53 || view.values[1].left != 40) { throw TestError('Inline values ignored measured prefix advances'); }
		if (view.heading.text != 'Faction Dominance' || view.heading.top + view.heading.measure().height != 14) { throw TestError('Dominance heading has wrong baseline'); }
		if (styles['bottombar-info-text'].font != 'arial.ttf:12:mono' || styles['bottombar-info-text'].color != 'rgb(116,156,56)' || styles['bottombar-info-value'].color != 'rgb(184,192,96)') { throw TestError('Original regular Arial or label/value colors differ'); }
		if (view.page.left != 7 || view.page.top != 7 || view.page.right != 39 || view.page.bottom != 7 || styles['bottombar-info-panel'].overflow != 'hidden') { throw TestError('Native text clips differ'); }
	};
};
