// Actual canonical nutrient section: no fabricated next-turn forecast.
const section = #include('../default/ui/parts/game/popup/base_screen/nutrients');
let grid = undefined;
let fallback = undefined;
section.growth_cells = {clear: () => { grid = undefined; fallback = undefined; }, text: (data) => { fallback = data.text; }};
section.growth_label = {};
section.p = {utils: {set_cells: (width, height, columns, rows, filled, profit, loss, cells, cls, label, formatter) => {
	grid = {columns: columns, rows: rows, filled: filled};
	test.assert(width == 112 && height == 90 && profit == 2 && loss == 0);
	label.text = formatter({});
}}};
section.set_stored({stored: 13, normalized_stored: 13, threshold: 20, cost_per_step: 10, rows: 2, surplus: 2, display_label: 'Growth: 5 turns'});
test.assert(grid == {columns: 10, rows: 2, filled: 13} && section.growth_label.text == 'Growth: 5 turns');
for (data of [
	{stored: 5, normalized_stored: 5, threshold: 0, cost_per_step: 0, rows: 2, display_label: 'Growth: 1 turn'},
	{stored: 5, normalized_stored: 5, threshold: 114, cost_per_step: 57, rows: 2, display_label: 'Growth: 1 turn'},
	{stored: 5, normalized_stored: 5, threshold: 460, cost_per_step: 10, rows: 46, display_label: 'Growth: 229 turns'},
]) {
	section.set_stored(data);
	test.assert(!#is_defined(grid) && fallback == 'Stored: 5');
}
section.set_stored({stored: -3});
test.assert(!#is_defined(grid) && fallback == 'Stored: -3' && section.growth_label.text == 'Growth unavailable');
