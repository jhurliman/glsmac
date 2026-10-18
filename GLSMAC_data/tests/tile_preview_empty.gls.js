const preview = #include('../default/ui/parts/game/bottom_bar/tile_preview');
let mouse = null;
let frame = null;
frame = { panel: (properties) => { return frame; }, on: (name, callback) => { mouse = callback; } };
preview.init({el: frame, ui: {class: (name) => { return {set: (properties) => {}}; }}, map: {on: (name, callback) => {}}});
test.assert(mouse({button: 'left'}) == true);
test.assert(preview.show_resources == false);
