const view = #include('../default/ui/parts/game/popup/base_screen/middle_area/psych');
let lines = [];
const rows = {clear: () => { lines = []; }, text: (data) => { lines :+data.text; }};
view.init({area: {panel: (data) => { return {listview: (props) => { test.assert(props.has_hscroll && props.has_vscroll); return rows; }}; }}});
const counts = (t, c, d, s) => { return {talents: t, content: c, drones: d, extra_drone_steps: s}; };
const current = {
    population: 7, specialists: 1, psych_points: 4, police: 0,
    initial: counts(0, 0, 6, 1), after_psych: counts(0, 1, 5, 0),
    after_facilities: counts(0, 3, 3, 0), after_police: counts(0, 4, 2, 0), final: counts(0, 4, 2, 0),
};
view.set({current: current, retained: null, reason: ''});
test.assert(#sizeof(lines) == 8 && lines[0] == 'Population 7; specialists 1' && lines[1] == 'Psych 4; Police 0');
test.assert(lines[2] == 'Initial: T 0 / C 0 / D 6' && lines[6] == 'Final: T 0 / C 4 / D 2' && lines[7] == 'Last pass unavailable');
view.set({current: null, retained: null, reason: 'Citizen history unavailable'});
test.assert(#sizeof(lines) == 1 && lines[0] == 'Citizen history unavailable');
current.final.extra_drone_steps = 2;
view.set({current: current, retained: {final: counts(1, 5, 0, 0)}, reason: ''});
test.assert(#sizeof(lines) == 9 && lines[7] == 'Additional drone pressure: 2' && lines[8] == 'Last pass: T 1 / C 5 / D 0');
view.set(undefined);
test.assert(#sizeof(lines) == 1 && lines[0] == 'Citizen/Psych details unavailable');
