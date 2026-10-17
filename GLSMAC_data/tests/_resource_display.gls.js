// Exercise the actual panel with the same native callable exposed by UI::Wrap.
const panel = #include('../default/ui/parts/game/popup/base_screen/resources');
return (format_balance) => {
	const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
	const minimum = 0 - 9223372036854775807 - 1;
	const maximum = 9223372036854775807;
	const cases = [
		{ profit: 0 - 2, loss: 2, left: '-2 - 002', right: '-004', losses: 2, profits: 0 },
		{ profit: 300, loss: 0, left: '300 - 000', right: '+300', losses: 0, profits: 58 },
		{ profit: 3, loss: 1, left: '003 - 001', right: '+002', losses: 1, profits: 2 },
		{ profit: 100, loss: 40, left: '100 - 040', right: '+060', losses: 40, profits: 18 },
		{ profit: 5, loss: 0 - 3, left: '005 - -3', right: '+008', losses: 0, profits: 8 },
		{ profit: minimum, loss: 2, left: '-9223372036854775808 - 002', right: '-9223372036854775810', losses: 2, profits: 0 },
		{ profit: minimum, loss: 0, left: '-9223372036854775808 - 000', right: '-9223372036854775808', losses: 0, profits: 0 },
		{ profit: minimum, loss: maximum, left: '-9223372036854775808 - 9223372036854775807', right: '-18446744073709551615', losses: 58, profits: 0 },
		{ profit: maximum, loss: minimum, left: '9223372036854775807 - -9223372036854775808', right: '+18446744073709551615', losses: 0, profits: 58 },
		{ profit: minimum, loss: minimum, left: '-9223372036854775808 - -9223372036854775808', right: '000', losses: 0, profits: 0 },
		{ profit: maximum, loss: maximum, left: '9223372036854775807 - 9223372036854775807', right: '000', losses: 58, profits: 0 },
		{ profit: 0, loss: 0, left: '000 - 000', right: '000', losses: 0, profits: 0 },
	];
	let cells = [];
	panel.cells = {
		clear: () => { cells = []; },
		surface: (properties) => { cells :+properties.class; },
	};
	panel.cell_width = 4;
	panel.total_cells = 58;
	panel.format_resource_balance = format_balance;
	panel.labels = {
		nutrients: { left: {}, right: {} },
		minerals: { left: {}, right: {} },
		energy: { left: {}, right: {} },
	};
	for (data of cases) {
		panel.set({ nutrients: data, minerals: data, energy: data });
		check(#sizeof(cells) == 174, 'cell allocation grew with resource amount');
		for (type of ['nutrients', 'minerals', 'energy']) {
			check(panel.labels[type].left.text == data.left, 'intake or consumption label changed: ' + data.left);
			check(panel.labels[type].right.text == data.right, 'balance label changed: ' + data.right);
			let losses = 0;
			let profits = 0;
			for (cls of cells) {
				if (cls == 'base-screen-resources-cell-' + type + '-loss') { losses++; }
				if (cls == 'base-screen-resources-cell-' + type + '-profit') { profits++; }
			}
			check(losses == data.losses && profits == data.profits, 'cell bounds or loss precedence changed');
		}
	}
	const none = format_balance(maximum, minimum, 0);
	check(none.surplus == '+18446744073709551615' && none.loss_cells == 0 && none.profit_cells == 0, 'zero-cell display changed its exact balance');
};
