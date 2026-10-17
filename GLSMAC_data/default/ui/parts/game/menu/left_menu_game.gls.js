return {

	init: (p) => {
		return p.create([
			{
				label: 'Save Campaign',
				open: () => { p.save_campaign(); },
			},
			{
				label: 'Start New Game',
				open: () => {
					p.maybe_quit(false);
				},
			},
			{
				label: 'Quit',
				open: () => {
					p.maybe_quit(true);
				},
			},
		]);
	},

};
