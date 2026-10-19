// Exercise the actual minimap caller, not a duplicate presentation model.
const create = #include('_restored_minimap');
let listeners = {};
let events = [];
const read = create({
    get_year: () => { return 2101; }, get_smac_credits: () => { return 0; },
    get_turn_status: () => { return 'turn_complete'; }, get_smac_minimap: () => { return #undefined; },
    on: (name, callback) => { listeners[name] = callback; },
    event: (name, data) => { events :+ name; },
});
let state = read();
test.assert(state.frames == [0] && state.timers == 1 && state.timer_delay == 100 && state.plays == 1,
    'initial ready control must consume frame0 once and preserve its ready announcement');
test.assert(state.label.font == 'arialbd.ttf:14:mono' && state.label.left == 57 && state.label.top == 4);
let i = 0;
for (i = 0; i < 41; i++) { test.assert(state.tick() == true); }
let expected = [];
for (i = 0; i < 19; i++) { expected :+ i; }
for (i = 1; i < 20; i++) { expected :+ i; }
for (i = 1; i < 5; i++) { expected :+ i; }
test.assert((read()).frames == expected, 'native second-frame wrap / empty ring hold changed');
state.modal(true); const before = #sizeof((read()).frames); state.tick();
test.assert(#sizeof((read()).frames) == before, 'modal timer advanced phase');
state.modal(false);
test.assert(state.hover(true) == false && state.hover(true) == false && state.hover(false) == false);
test.assert(#sizeof((read()).frames) == before + 2, 'hover repeated-inside redraw advanced animation');
listeners.turn_status({status: 'active'}); state.tick();
let frames = (read()).frames;
test.assert(frames[#sizeof(frames) - 1] == 0, 'unmoved control faded instead of rewinding frame0');
state.click(); state.confirm(false); test.assert(events == [], 'Cancel completed the turn');
state.click(); state.confirm(true);
test.assert(events == ['complete_turn'] && (read()).text == 'PLEASE WAIT');
listeners.turn_status({status: 'turn_complete'});frames = (read()).frames;
test.assert(frames[#sizeof(frames) - 1] == 1 && (read()).plays == 2,
    'inactive frame0 must leave next active redraw at frame1');
state.click(); test.assert(events == ['complete_turn', 'complete_turn']);
