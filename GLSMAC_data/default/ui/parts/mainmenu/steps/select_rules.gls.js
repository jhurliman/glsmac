return (i) => {
	i.sliding.show({
		native_menu: i.glsmac.game.get_ruleset() == 'smac',
		title: 'Game rules',
		entries: [
			['Play with Standard Rules', () => {
				if (i.glsmac.game.get_ruleset() == 'smac') { i.settings.global.select_campaign_rules(false); }
				i.steps.select_faction(i);
			}],
			['Play with Current Rules', () => {
				if (i.glsmac.game.get_ruleset() == 'smac') { i.settings.global.select_campaign_rules(true); }
				i.steps.select_faction(i);
			}],
			['Customize Rules', () => {
				// TODO: custom rules
				i.steps.notimpl(i);
			}],
		]
	});
};
