// Real navigation admission and existing popup callback preservation.
const screen = #include('../default/ui/parts/game/popup/base_screen');
const popup = #include('../default/ui/parts/game/popup/popup');
let pending = null;
let targets = [];
let reads = 0;
let neighbors = {previous: {id: 902}, next: {id: 13}};
screen.base = {get_smac_base_navigation: () => { reads++; return neighbors; }};
screen.is_open = true;
screen.p = {game: {get_pending_smac_colony_screen: () => { return pending; }, select_base: (base) => { targets :+ base.id; }}};
screen.navigate_base('previous');
screen.navigate_base('next');
test.assert(targets == [902,13] && reads == 2);
neighbors = undefined;
screen.navigate_base('next');
test.assert(targets == [902,13] && reads == 3);
pending = {request_id: 7};
screen.navigate_base('previous');
test.assert(reads == 3);
pending = null;
screen.is_open = false;
screen.navigate_base('next');
test.assert(reads == 3);
// Re-selecting a Base while the screen is open must not replace the original
// close callback which restores the selected Unit (or owns a continuation).
let closed = 0;
popup.popup = {id: 'base_screen'};
popup.popup_cb = () => { closed++; };
popup.show('base_screen', () => { throw Error('Replaced original close callback'); });
popup.popup_cb();
test.assert(closed == 1);

// Actual bottom-bar controls bind the navigation callback and hide themselves
// for a singleton/foreign Base or required Colony Screen.
const section = #include('../default/ui/parts/game/popup/base_screen/bottom_bar/middle_area');
let actions = [];
const node = () => { return {
	hidden: false,
	panel: (data) => { return node(); },
	text: (data) => { return data; },
	button: (data) => { return node(); },
	on: (event, callback) => { this.click = callback; },
	show: () => { this.hidden = false; }, hide: () => { this.hidden = true; },
	clear: () => {},
}; };
section.init({body: node(), ui: {class: (name) => { return {set: (data) => {}}; }},
	game: {get_bm: () => { return {get_pop_renders: (owner) => { return {}; }}; }, get_pending_smac_colony_screen: () => { return pending; }},
	utils: {navigate_base: (direction) => { actions :+ direction; }}});
let neighbor_data = {previous: {}, next: {}};
const data = {name: 'First', owner: {}, pops: [], base: {get_smac_base_navigation: () => { return neighbor_data; }}};
section.set(data);
test.assert(!section.previous.hidden && !section.next.hidden);
section.previous.click(); section.next.click();
test.assert(actions == ['previous','next']);
pending = {request_id: 7};
section.set(data);
test.assert(section.previous.hidden && section.next.hidden);
pending = null; neighbor_data = undefined;
section.set(data);
test.assert(section.previous.hidden && section.next.hidden);
