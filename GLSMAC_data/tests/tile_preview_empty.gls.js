const preview = #include('../default/ui/parts/game/bottom_bar/tile_preview');
let mouse = null;
let frame = null;
frame = { panel: (properties) => { return frame; }, on: (name, callback) => { mouse = callback; } };
preview.init({el: frame, ui: {class: (name) => { return {set: (properties) => {}}; }}, map: {on: (name, callback) => {}}});
test.assert(mouse({button: 'left'}) == true);
test.assert(preview.show_resources == false);

// Exercise the production caller: its independent work surface covers the
// original clip; text still starts at baseline122 (local56 after origin66).
let image = null;
let line = null;
preview.tile = {x: 8, y: 8};
preview.frame = {widget: (properties) => {image = properties; return properties;}};
preview.set_image();
test.assert(image.width == 89 && image.height == 117);
test.assert(image.left == 0 && image.top == 0);
preview.row = 0;
preview.lines = {text: (properties) => {
	line = {measure: () => {return {height: 13};}};
	return line;
}};
preview.line('Elev: 60m');
test.assert(line.top + line.measure().height == 56);
