const preview = #include('../default/ui/parts/game/bottom_bar/tile_preview');
let event = null;
let mouse = null;
let frame = null;
let texts = [];
let coordinates = '';
let widgets = 0;
let removed_widgets = 0;
let removed_lines = 0;
let resource_reads = 0;
const label = () => {return {measure: () => {return {height: 13};}};};
frame = {
    panel: (properties) => {return frame;},
    on: (name, callback) => {mouse = callback;},
    area: (properties) => {
        texts = [];
        return {
            text: (properties) => {texts :+ properties.text; return label();},
            remove: () => {removed_lines = removed_lines + 1;},
        };
    },
    text: (properties) => {coordinates = properties.text; return label();},
    widget: (properties) => {
        widgets = widgets + 1;
        return {remove: () => {removed_widgets = removed_widgets + 1;}};
    },
};
preview.init({el: frame, ui: {class: (name) => {return {set: (properties) => {}};}},
    map: {on: (name, callback) => {event = callback;}}});
const known = {x: 8, y: 8, is_water: false, elevation: 60, rockiness: 1, moisture: 2,
    features: {}, bonuses: {}, get_resources: () => {
        resource_reads = resource_reads + 1;
        return {NUTRIENTS: 1, MINERALS: 2, ENERGY: 3};
    }};
event({tile: known, known: true});
test.assert(widgets == 1 && #sizeof(texts) == 2 && texts[0] == 'Elev: 60m');
// Only coordinates are available. Any accidental terrain read fails the test.
event({tile: {x: 8, y: 8}, known: false});
test.assert(widgets == 1 && removed_widgets == 1 && removed_lines == 1);
test.assert(#sizeof(texts) == 1 && texts[0] == 'Unexplored');
test.assert(coordinates == '(8 , 8)' && !#is_defined(preview.preview));
test.assert(mouse({button: 'left'}) == true);
test.assert(resource_reads == 0 && #sizeof(texts) == 1 && texts[0] == 'Unexplored');
// Repeated unknown selection neither restores the old image nor reads details.
event({tile: {x: 10, y: 8}, known: false});
test.assert(widgets == 1 && removed_widgets == 1 && texts[0] == 'Unexplored');
event({tile: known, known: true});
test.assert(widgets == 2 && resource_reads == 1 && #sizeof(texts) == 3);
test.assert(texts[0] == 'Nutrients: 1');
