// Real minimap initialization/listeners and real native Game; only the asset-
// dependent DOM, sound and timer surfaces are replaced by synchronous factories.
const minimap = #include('../default/ui/parts/game/bottom_bar/mini_map');
return (game) => {
	let year = null;
	let turn_button = null;
	let plays = 0;
	let timers = 0;
	const style = {set: (data) => {}};
	style.extend = (name) => { return style; };
	const button = (data) => {
		const result = data + {
			on: (name, callback) => {},
			sound: (properties) => { return {play: () => { plays++; }}; },
			surface: (properties) => {
				return properties + {timer: (delay, callback) => {
					timers++;
					return {stop: () => { timers--; }};
				}};
			},
		};
		if (#is_defined(data.width) && data.width == 233) { turn_button = result; }
		return result;
	};
	let panel = null;
	panel = (data) => {
		return {
			panel: panel,
			button: button,
			text: (properties) => {
				if (year == null) { year = properties; }
				return properties;
			},
			widget: (properties) => { return {
				listen: (object, event, handler) => { object.on(event, handler); },
				show: () => {}, hide: () => {},
			}; },
		};
	};
	minimap.init({game: game, ui: {class: (name) => { return style; }}, el: {panel: panel}});
	return () => { return {year: year.text, text: turn_button.text, plays: plays, timers: timers}; };
};
