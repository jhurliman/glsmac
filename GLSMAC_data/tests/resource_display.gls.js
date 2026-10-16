// Exercise the real resource-panel formatter and its bounded cell rendering.
const panel = #include('../default/ui/parts/game/popup/base_screen/resources');
test.assert(panel._pad(2) == '002', 'ordinary positive padding changed');
test.assert(panel._pad(300) == '300', 'large positive amount changed');
test.assert(panel._pad(0 - 2) == '-2', 'negative sign moved behind zero padding');
const minimum = 0 - 9223372036854775807 - 1;
test.assert(panel._pad(minimum) == '-9223372036854775808', 'minimum signed amount changed');
let cells = 0;
panel.cells = {
	clear: () => { cells = 0; },
	surface: (properties) => { cells++; },
};
panel.cell_width = 4;
panel.total_cells = 58;
panel.labels = {
	nutrients: { left: {}, right: {} },
	minerals: { left: {}, right: {} },
	energy: { left: {}, right: {} },
};
panel.set({
	nutrients: { profit: 0 - 2, loss: 2 },
	minerals: { profit: 300, loss: 0 },
	energy: { profit: 3, loss: 1 },
});
test.assert(panel.labels.nutrients.left.text == '-2 - 002', 'negative intake was not displayed exactly');
test.assert(panel.labels.nutrients.right.text == '-004', 'negative surplus changed');
test.assert(panel.labels.minerals.left.text == '300 - 000', 'large intake was not displayed exactly');
test.assert(panel.labels.minerals.right.text == '+300', 'large surplus changed');
test.assert(cells == 174, 'cell allocation grew with resource amount');
