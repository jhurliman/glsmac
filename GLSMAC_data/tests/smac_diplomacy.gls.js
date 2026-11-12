const view = #include('../default/ui/parts/game/diplomacy/diplomacy');
let handlers = {}; let submitted = []; let status = 'turn_active'; let contacts = [];
const node = (data) => {
    data.children = []; data.hidden = false;
    data.on = (name, cb) => { data[name] = cb; };
    data.listen = (object, name, cb) => { handlers[name] = cb; };
    data.show = () => { data.hidden = false; }; data.hide = () => { data.hidden = true; };
    data.clear = () => { data.children = []; };
    const child = (props) => { const value = node(props); data.children :+value; return value; };
    data.panel = child; data.window = child; data.scrollview = child; data.button = child;
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

// War confirmation remains available independently of negotiation cooldown.
contacts = [{target: 2, name: 'Gaia', relation: 'Treaty', unavailable: 'Negotiations resume in 16 turns', expected_state: 'war-state',
    war_unavailable: '', war_warning: 'Breaking this Treaty can damage your integrity. Declare Vendetta?'}];
view.open(); test.assert(#sizeof(view.rows.children) == 3);
view.rows.children[2].click({}); test.assert(view.confirming != null && !view.confirmation.hidden);
const previous_count = #sizeof(submitted); view.cancel_war();
test.assert(view.confirming == null && #sizeof(submitted) == previous_count);
view.rows.children[2].click({}); view.submit_war();
test.assert(view.waiting && submitted[previous_count] == {name: 'declare_smac_vendetta', args: {target: 2, expected_state: 'war-state', proceed: true}});
handlers.smac_diplomacy_response({target: 2, accepted: true, text: 'Vendetta declared'});
test.assert(!view.waiting && view.confirming == null && view.status.text == 'Vendetta declared');
view.rows.children[2].click({}); handlers.turn({}); view.submit_war();
test.assert(#sizeof(submitted) == previous_count + 1 && !view.active);
