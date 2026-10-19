const roster = #include('../default/ui/parts/game/bottom_bar/objects_list');
let map_handlers = {};
let game_handlers = {};
let rebuilds = 0;
let node = null;
node = (properties) => {
    const result = properties;
    result.panel = node;
    result.on = (name, callback) => {};
    result.listen = (object, name, callback) => { object.on(name, callback); };
    result.scrollview = (properties) => { rebuilds++; return node(properties); };
    return result;
};
roster.init({
    map: {on: (name, callback) => { map_handlers[name] = callback; }},
    game: {on: (name, callback) => { game_handlers[name] = callback; }},
    el: node({}), ui: {class: (name) => { return {set: (properties) => {}}; }},
});
map_handlers.base_preview({base: null});
map_handlers.base_preview({base: #undefined});
test.assert(rebuilds == 0, 'missing Base preview changed the inspected roster');
