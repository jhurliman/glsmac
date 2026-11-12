return {

	init: (p) => {
		return p.create([

            {label: 'Preferences...', open: () => { p.open_preferences(0); }},
            {label: 'Warning Preferences...', open: () => { p.open_preferences(2); }},
            {label: 'Advanced Preferences...', open: () => { p.open_preferences(1); }},
            {label: 'Automation Preferences...', open: () => { p.open_preferences(3); }},
            {label: 'Audio/Visual Preferences...', open: () => { p.open_preferences(4); }},
            {label: 'Map Display Preferences...', open: () => { p.open_preferences(5); }},
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
