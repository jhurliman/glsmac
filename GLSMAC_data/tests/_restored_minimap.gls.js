// Real minimap initialization/listeners and real native Game; only the asset-
// dependent DOM, sound and timer surfaces are replaced by synchronous factories.
const minimap = #include('../default/ui/parts/game/bottom_bar/mini_map');
return (game) => {
	let year = null;
	let energy = null;
	let turn_button = null;
	let plays = 0;
	let timers = 0;
	let overview = null;
	let world_click = null;
	let world_geometry = null;
	let info_geometry = null;
	let map_listeners = {};
	const style = {set: (data) => {}};
	style.extend = (name) => { return style; };
	const button = (data) => {
		const result = data + {
			on: (name, callback) => {
				if (name == 'click' && #is_defined(data.right) && data.right == 66) { world_click = callback; world_geometry = data; }
			},
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
	const widget = (properties) => {
		const result = properties + {
			listen: (object, event, handler) => { object.on(event, handler); },
			show: () => {}, hide: () => {},
		};
		overview = result;
		return result;
	};
	let panel = null;
	panel = (data) => {
		if (#is_defined(data.width) && data.width == 116) { info_geometry = data; }
		return {
			panel: panel,
			button: button,
			text: (properties) => {
				if (year == null) { year = properties; }
				else { energy = properties; }
				return properties;
			},
			widget: widget,
		};
	};
	minimap.init({game: game, map: {on: (name, callback) => { map_listeners[name] = callback; }},
		ui: {class: (name) => { return style; }}, el: {panel: panel, button: button, widget: widget}});
	return () => { return {year: year.text, energy: energy.text, text: turn_button.text, plays: plays, timers: timers,
		overview: overview.data.overview, geometry: {align: overview.align, right: overview.right, top: overview.top, width: overview.width, height: overview.height},
		world_geometry: world_geometry, info_geometry: info_geometry, world_click: world_click, map_event: (name) => { map_listeners[name]({}); }}; };
};
