return (i) => {
	i.glsmac.deinit();
	i.sliding.show({
		native_title: true,
		entries: [
			['Start Game', () => {
				i.settings.local.game_mode = 'single';
				i.settings.local.network_role = 'none';
				i.steps.select_ruleset(i, () => {
					i.glsmac.init();
					i.steps.start_game(i);
				});
			}],
			['Quick Start', () => {
				i.settings.local.game_mode = 'single';
				i.settings.local.network_role = 'none';
				i.steps.select_ruleset(i, () => {
					i.glsmac.init();
					i.randomize_map();
					if (i.glsmac.game.get_ruleset() == 'smac') {
						i.steps.select_mapsize(i, () => { i.glsmac.start_game(); });
					} else { i.glsmac.start_game(); }
				});
			}],
			['Scenario', () => {
				i.steps.notimpl(i);
			}],
			['Load Game', () => {
				i.steps.load_campaign(i);
			}],
			['Multiplayer', () => {
				i.settings.local.game_mode = 'multi';
				i.steps.multiplayer_type(i);
			}],
			['View Credits', () => {
				i.steps.notimpl(i);
			}],
			['Exit Game', () => {
				i.glsmac.exit();
			}],
		]
	});
};
