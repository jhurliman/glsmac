const view = #include('../default/ui/parts/game/diplomacy/diplomacy');
let handlers = {}; let submitted = []; let status = 'turn_active'; let contacts = [];
const node = (data) => {
    data.children = []; data.hidden = false;
    data.on = (name, cb) => { data[name] = cb; };
    data.listen = (object, name, cb) => { handlers[name] = cb; };
    data.show = () => { data.hidden = false; }; data.hide = () => { data.hidden = true; };
    data.clear = () => { data.children = []; };
    const child = (props) => { const value = node(props); data.children :+value; return value; };
    data.window = child; data.scrollview = child; data.button = child;
    if (!#is_defined(data.text)) { data.text = child; }
    return data;
};
view.init({root: node({}), modules: {popup: {popup: null}, menu: {close_all: () => {}}}, game: {
    get_ruleset: () => { return 'smac'; }, get_turn_status: () => { return status; },
    get_smac_diplomacy: () => { return contacts; },
    event: (name, args) => { submitted :+{name: name, args: args}; },
}});
view.open(); test.assert(view.active && #sizeof(view.rows.children) == 1);
view.close(); test.assert(!view.active && #sizeof(submitted) == 0);
contacts = [{target: 2, name: 'Gaia', relation: 'Contact established', unavailable: '', expected_state: 'native-state'}];
handlers.map_knowledge({}); test.assert(view.active && #sizeof(view.rows.children) == 3);
const retired_truce = view.rows.children[1].click;
view.rows.children[2].click({});
test.assert(view.waiting && submitted == [{name: 'offer_smac_peace', args: {target: 2, kind: 'treaty', expected_state: 'native-state'}}]);
retired_truce({}); test.assert(#sizeof(submitted) == 1);
contacts = [{target: 2, name: 'Gaia', relation: 'Treaty', unavailable: 'Negotiations resume in 16 turns', expected_state: 'new-state'}];
handlers.smac_diplomacy_response({target: 2, accepted: true, text: 'Treaty accepted'});
test.assert(!view.waiting && view.status.text == 'Treaty accepted' && #sizeof(view.rows.children) == 2);
retired_truce({}); test.assert(#sizeof(submitted) == 1);
view.close(); status = 'please_wait';
contacts :+{target: 3, name: 'Sparta', relation: 'Contact established', unavailable: '', expected_state: 'another-state'};
handlers.map_knowledge({}); test.assert(!view.active && view.pending_contact);
status = 'turn_active'; handlers.turn_status({status: status}); test.assert(view.active && !view.pending_contact);
handlers.turn({}); test.assert(!view.active);
const menu = #include('../default/ui/parts/game/menu/right_menu'); let opened = 0;
const entries = menu.init({create: (entries) => { return entries; }, open_diplomacy: () => { opened++; }});
test.assert(#sizeof(entries) == 1 && entries[0].label == 'Commlink'); entries[0].open(); test.assert(opened == 1);
