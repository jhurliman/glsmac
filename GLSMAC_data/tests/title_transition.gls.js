// Run the real sliding menu and history manager. Advance timer deliveries
// explicitly; DOM doubles expose label visibility and registered input only.
const sliding_factory = #include('../default/ui/parts/mainmenu/sliding');
let labels = [];
let buttons = [];
let timer = null;
let keydown = null;
let art = null;
let removed = 0;
let chosen = -1;
let timer_count = 0;
const m = {
	glsmac: {ui: {class: (name) => { return {set: (data) => {}}; }}},
	ui: {get_width: () => { return 1024; }, get_height: () => { return 768; }, on: (name, cb) => {}},
	root: {area: (data) => {
		return {
			remove: () => { removed++; },
			sound: (data) => {},
			on: (name, cb) => { keydown = cb; },
			surface: (data) => { art = data; return data; },
			timer: (ms, cb) => {
				test.assert(ms == 10);
				timer_count++;
				timer = {callback: cb, stopped: false, stop: () => { timer.stopped = true; }};
				return timer;
			},
			button: (data) => {
				const button = {handlers: {}};
				button.on = (name, cb) => { button.handlers[name] = cb; };
				button.text = (style) => {
					const label = style;
					label.visible = true;
					label.measure = () => { return {width: 90, height: 18}; };
					label.hide = () => { label.visible = false; };
					label.show = () => { label.visible = true; };
					labels:+label;
					return label;
				};
				buttons:+button;
				return button;
			},
		};
	}},
};
const sliding = sliding_factory(m);
const entries = [['Start Game', () => { chosen = 0; }], ['Quick Start', () => { chosen = 1; }]];
const visible = (expected) => {
	for (label of labels) { test.assert(label.visible == expected, 'label visibility at transition boundary'); }
};
const frame = (index) => {
	test.assert(art.background == 'openitem.flc:smacmenu(openinga.pcx,' + #to_string(index) + ',7,1024,768):stretch()');
};
const key = (code) => { test.assert(keydown({code: code, modifiers: []}) == true); };
sliding.show({native_title: true, entries: entries});
frame(0); visible(false);
key('DOWN'); key('ENTER');
test.assert(chosen == -1, 'opening activated action');
test.assert(timer_count == 1 && (labels[0]).color == 'rgb(176, 212, 235)', 'opening input changed selection or timer');
test.assert((buttons[1]).handlers.mousedown({}) == false);
for (let index = 1; index < 6; index++) {
	test.assert(timer.callback() == true); frame(index); visible(false);
}
test.assert(timer.callback() == false); visible(true); frame(5);
key('DOWN');
test.assert((labels[1]).color == 'rgb(176, 212, 235)');
key('ENTER');
visible(false); frame(6);
key('UP'); key('ENTER');
test.assert(timer_count == 2 && (labels[1]).color == 'rgb(176, 212, 235)', 'closing input changed selection or timer');
for (let closing = 7; closing < 12; closing++) {
	test.assert(timer.callback() == true); frame(closing); visible(false);
	test.assert(chosen == -1 && removed == 0, 'action ran before closing finished');
}
test.assert(timer.callback() == false);
test.assert(chosen == 1 && removed == 1);
// Removing an unfinished menu cancels its timer. A new menu starts hidden at0.
labels = []; buttons = [];
sliding.show({native_title: true, entries: entries});
const canceled = timer;
sliding.hide();
test.assert(canceled.stopped && removed == 2);
labels = []; buttons = [];
sliding.show({native_title: true, entries: entries});
frame(0); visible(false);
for (let reopening = 0; reopening < 5; reopening++) { test.assert(timer.callback() == true); }
test.assert(timer.callback() == false); visible(true);
sliding.hide();
