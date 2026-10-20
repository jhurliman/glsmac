return (i) => {
	i.sliding.show({
		native_menu: i.glsmac.game.get_ruleset() == 'smac',
		entries: [
			['Make Random Map', () => {
				i.settings.global.map.type = 'random';
				i.randomize_map();
				i.steps.select_mapsize(i);
			}],
			['Customize Random Map', () => {
				i.settings.global.map.type = 'custom';
				i.steps.select_mapsize(i);
			}],
			['The Map of Planet', () => {
				if (i.glsmac.game.get_ruleset() != 'smac') { i.steps.notimpl(i); return; }
				i.settings.global.select_installed_planet();
				i.steps.select_difficulty_level(i);
			}],
			['Huge Map of Planet', () => {
				i.steps.notimpl(i);
			}],
			['Load Map File', () => {
				i.steps.notimpl(i);
			}],
		]
	});
};
