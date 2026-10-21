return {

	init: (p) => {
		return p.create([
			{
				label: 'GAME',
				open: () => {
					p.menu.open('left_menu_game', {
						align: 'bottom',
						left: p.menu.menu_width,
						bottom: 256 + 166 - p.menu.item_height * 3,
					});
				},
				close: () => {
					p.menu.close('left_menu_game');
				},
			},
            {
                label: 'HQ',
                open: () => {
                    p.menu.open('left_menu_hq', {align: 'bottom', left: p.menu.menu_width, width: 230,
                        bottom: 256 + 166 - p.menu.item_height * 4});
                },
                close: () => { p.menu.close('left_menu_hq'); },
            },
		]);
	},

};
