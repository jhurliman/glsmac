// Real minimap initialization/listeners and real native Game; only the asset-
// dependent DOM, sound and timer surfaces are replaced by synchronous factories.
const minimap = #include('../default/ui/parts/game/bottom_bar/mini_map');
return (game) => {
	let year = null;
	let energy = null;
	let turn_button = null;
	let plays = 0;
	let timers = 0;
    let turn_label = null;
    let turn_events = {};
    let frames = [];
    let tick = null;
    let timer_delay = null;
    let modal = false;
    let confirmation = null;
	let overview = null;
	let world_click = null;
    let detail_click = null;
    let detail_visible = false;
	let world_geometry = null;
	let info_geometry = null;
	let map_listeners = {};
	const style = {set: (data) => {}};
	style.extend = (name) => { return style; };
	const button = (data) => {
		const result = data + {
            show: () => { if (#is_defined(data.right) && data.right == 10) { detail_visible = true; } },
            hide: () => { if (#is_defined(data.right) && data.right == 10) { detail_visible = false; } },
			on: (name, callback) => {
                if (#is_defined(data.width) && data.width == 233) { turn_events[name] = callback; }
                if (name == 'click' && #is_defined(data.right) && data.right == 10) { detail_click = callback; }
				if (name == 'click' && #is_defined(data.right) && data.right == 66) { world_click = callback; world_geometry = data; }
			},
			sound: (properties) => { return {play: () => { plays++; }}; },
			surface: (properties) => {
				return properties + {
                    smacturnframe: (frame) => { frames :+ frame; return true; },
                    timer: (delay, callback) => {
                    timer_delay = delay;
                    tick = callback;
					timers++;
					return {stop: () => { timers--; }};
				}};
			},
		};
        if (#is_defined(data.width) && data.width == 233) {
            turn_button = result;
            result.text = (properties) => {
                const label = properties + {measure: () => { return {width: 119, height: 13}; }};
                turn_label = label; return label;
            };
        }
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
		modules: {popup: {is_shown: () => { return modal; }, show: (name, callback) => { confirmation = callback; }}},
		ui: {class: (name) => { return style; }}, el: {panel: panel, button: button, widget: widget}});
	return () => { return {year: year.text, energy: energy.text, text: turn_label.text, plays: plays, timers: timers,
		frames: frames, timer_delay: timer_delay, label: turn_label, turn_geometry: turn_button,
        tick: () => { return tick(); }, hover: (inside) => { return turn_events[inside ? 'mouseover' : 'mouseout']({}); },
        click: () => { return turn_events.click({}); }, modal: (shown) => { modal = shown; },
        confirm: (accepted) => { confirmation(accepted); },
		overview: overview.data.overview, geometry: {align: overview.align, right: overview.right, top: overview.top, width: overview.width, height: overview.height},
        detail: overview.data.detail, mode: overview.data.mode, detail_click: detail_click, detail_visible: detail_visible,
		world_geometry: world_geometry, info_geometry: info_geometry, world_click: world_click, map_event: (name) => { map_listeners[name]({}); }}; };
};
