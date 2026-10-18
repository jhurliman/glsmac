// Production screen/control callbacks; asset/DOM and native query values doubled.
const screen = #include('../default/ui/parts/game/popup/base_screen');
const top = #include('../default/ui/parts/game/popup/base_screen/top_buttons');
const buttons = #include('../default/ui/parts/game/popup/base_screen/buttons');
const queue = #include('../default/ui/parts/game/popup/base_screen/bottom_bar/queue');
const bottom = #include('../default/ui/parts/game/popup/base_screen/bottom_bar');
const portraits = #include('../default/ui/parts/game/popup/base_screen/bottom_bar/middle_area');
const resource = #include('../default/ui/parts/game/popup/base_screen/middle_area/resource');
let retained_response = false;
let commands = []; let closes = 0; let middle_data = null; let bottom_data = null;
const node = (data) => {
    data.handlers = {}; data.children = []; data.hidden = false;
    data.on = (name, callback) => { data.handlers[name] = callback; return data; };
    data.listen = (object, name, callback) => {};
    data.hide = () => { data.hidden = true; }; data.show = () => { data.hidden = false; };
    data.clear = () => { data.children = []; }; data.remove = () => {};
    const child = (properties) => { const result = node(properties); data.children :+ result; return result; };
    data.panel = child; data.window = child; data.button = child; data.listview = child;
    data.area = child; data.surface = child; data.widget = child;
    if (!#is_defined(data.text)) { data.text = child; }
    return data;
};
const style = {set: (data) => {}}; style.extend = (name) => { return style; };
const ui = {root: node({}), class: (name) => { return style; }};
const game = {event: (name, data) => { commands :+ name; }, get_year: () => { return 2101; },
    get_pending_smac_colony_screen: () => { return {}; }, get_player: () => { return {id: 1}; },
    get_bm: () => { return {get_pop_renders: (owner) => { return {worker: ['sprite']}; }}; }};
const base = {id: 701, name: 'Transferred Base', has_smac_resource_context: true,
    get_owner: () => { return {id: 1, get_faction: () => { return {is_progenitor: false}; }}; },
    get_smac_rename_state: () => { return 'owned-state'; },
    get_smac_specialist_selection_state: () => { return 'owned-state'; },
    get_smac_base_navigation: () => { return undefined; },
    get_smac_resource_calculation: () => { return {retained_only: retained_response, nutrients: {intake: 2, demand: 2}, minerals: {intake: 2, demand: 0, surplus: 2}, raw_intake: {ENERGY: 1}}; },
    get_smac_energy_calculation: () => { if (retained_response) { throw 'Inactive energy calculation must not run'; } return undefined; },
    get_accumulated_nutrients: () => { return 0; },
    get_smac_nutrient_presentation: () => { if (retained_response) { throw 'Inactive growth forecast must not run'; } return {stored: 0}; },
    get_facilities: () => { return {}; }, get_pops: () => { return []; }, get_production_state: () => { return undefined; },
    get_smac_production_presentation: (surplus) => { return undefined; },
    get_smac_support_presentation: () => { return undefined; }};
const sink = {set: (data) => {}, set_stored: (data) => {}, set_commerce: (data) => {}, set_unavailable: () => {},
    on_base_update: (base) => {}, hide_view: () => {}, reset: () => {}};
const p = {ui: ui, game: game, body: node({}), hide: () => { closes++; screen.on_hide(); }, utils: {
    can_rename: screen.can_rename, can_edit_queue: screen.can_edit_queue,
    open_rename: screen.open_rename, open_queue_editor: screen.open_queue_editor, open_hurry: screen.open_hurry,
    show_production_selection: screen.show_production_selection,
    open_specialist_selection: (base, id) => { commands :+ 'specialist'; }, navigate_base: (direction) => {}}};
top.init(p); buttons.init(p); queue.init(p); portraits.init(p);
screen.p = p; screen.production_selection = null;
screen.sections = {top_buttons: top, buttons: buttons, nutrients: sink, economy: sink, game_state: sink,
    facilities: sink, resources: sink, energy: sink, specialist_selection: sink, rename: sink, queue_editor: sink, hurry: sink,
    middle_area: {set: (data) => { middle_data = data; }},
    bottom_bar: {set: (data) => { bottom_data = data; }, frame: node({})}};
screen.set({base: base, read_only: true}); screen.on_show();
test.assert(screen.read_only && middle_data.read_only && bottom_data.read_only);
test.assert(top.governor_button.hidden && top.energy_button.hidden && !top.read_only_notice.hidden);
test.assert(!screen.can_rename() && !screen.can_edit_queue() && buttons.buttons[0].text == 'Rename unavailable');
screen.open_rename(); screen.open_queue_editor(); screen.open_hurry(); screen.open_specialist_selection(base, 19);
screen.show_production_selection(); screen.update_production_selection();
test.assert(screen.production_selection == null && #sizeof(commands) == 0);
test.assert(top.governor_button.handlers.click({}) && top.energy_button.handlers.click({}) && !top.active);
retained_response = true;
screen.set({base: base}); test.assert(screen.read_only && top.governor_button.hidden);
test.assert(top.read_only_notice.text == 'Faction inactive. Showing last resource calculation; close to continue.', #to_string(top.read_only_notice.text) + ' / ' + #to_string(base.get_smac_resource_calculation().retained_only));
retained_response = false;
let queue_read_only = false; let portrait_data = null;
bottom.parts = {queue: {set_read_only: (value) => { queue_read_only = value; }, set_stored: (value) => {}},
    production: {set_stored: (state, presentation, surplus) => {}}, support: sink, middle_area: {set: (data) => { portrait_data = data; }}};
bottom.set(bottom_data); test.assert(queue_read_only && portrait_data.read_only);
queue.set_read_only(true);
test.assert(queue.change_button.hidden && queue.edit_button.hidden && queue.hurry_button.hidden);
test.assert(queue.change_button.handlers.click({}) && queue.edit_button.handlers.click({}) && queue.hurry_button.handlers.click({}));
const pop = {id: 19, variant: 0, get_type: () => { return 'worker'; }};
portraits.set({base: base, name: base.name, owner: base.get_owner(), pops: [pop], read_only: true});
test.assert(portraits.pops.children[0].handlers.mousedown({}) && #sizeof(commands) == 0);
const tile = (x, y) => { return {x: x, y: y,
    get_N: () => { return tile(x, y - 2); }, get_NE: () => { return tile(x + 1, y - 1); },
    get_E: () => { return tile(x + 2, y); }, get_SE: () => { return tile(x + 1, y + 1); },
    get_S: () => { return tile(x, y + 2); }, get_SW: () => { return tile(x - 1, y + 1); },
    get_W: () => { return tile(x - 2, y); }, get_NW: () => { return tile(x - 1, y - 1); }}; };
base.get_tile = () => { return tile(2, 2); }; base.get_worked_tiles = () => { return []; };
resource.init({area: node({}), ui: ui, game: game}); resource.set({base: base, read_only: true});
test.assert(resource.el.handlers.mousedown({button: 'left'}) && #sizeof(commands) == 0);
test.assert(buttons.buttons[1].handlers.click({}) == false && closes == 1 && !screen.is_open && !screen.read_only);
screen.set({base: base});
test.assert(!top.governor_button.hidden && !top.energy_button.hidden && top.read_only_notice.hidden && screen.can_rename());
queue.set_read_only(false); test.assert(!queue.change_button.hidden && !queue.edit_button.hidden && !queue.hurry_button.hidden);
