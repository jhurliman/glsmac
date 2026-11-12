const view = #include('../default/ui/parts/game/research/overview');
let handlers = {};let queries = 0;
const node = (data) => {
    data.children = [];data.hidden = false;
    data.on = (name, cb) => { data[name] = cb; };
    data.listen = (object, name, cb) => { handlers[name] = cb; };
    data.show = () => { data.hidden = false; };data.hide = () => { data.hidden = true; };
    data.clear = () => { data.children = []; };
    const child = (props) => { const value = node(props);data.children :+value;return value; };
    data.window = child;data.scrollview = child;data.button = child;
    if (!#is_defined(data.text)) { data.text = child; }
    return data;
};
let snapshot = {blind: false, target_name: 'Centauri Ecology', labs: 17, priorities: [], known_names: ['Industrial Base', 'Information Networks']};
const research = {pending: null};
view.init({root: node({}), modules: {research: research, popup: {popup: null}, menu: {close_all: () => {}}}, game: {
    get_ruleset: () => { return 'smac'; },get_player: () => { return 'local'; },
    get_technology_state: (player) => { test.assert(player == 'local');queries++;return snapshot; }
}});
view.open();test.assert(view.active && view.status.text == 'Researching: Centauri Ecology' && #sizeof(view.known.children) == 2);
test.assert(view.labs.text == 'Accumulated labs: 17  |  Known technologies: 2');
handlers.turn({});test.assert(!view.active);
snapshot = {blind: true, target_name: 'Secret target', labs: 4, priorities: [1, 0, 0, 1], known_names: []};
view.open();test.assert(view.status.text == 'Priorities: Explore, Conquer');view.close();
view.p.modules.research.pending = {request_id: 3};view.open();test.assert(!view.active && queries == 2);
