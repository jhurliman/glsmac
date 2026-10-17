return (i, on_selected) => {
	const next = () => {
		if (#is_defined(on_selected)) {
			on_selected();
		} else if (i.settings.global.map.type == 'random') {
			i.steps.select_difficulty_level(i);
		} else { // custom
			i.steps.customize_ocean_coverage(i);
		}
	};
	if (i.glsmac.game.get_ruleset() == 'smac') {
		let entries = [];
		const labels = ['Tiny Planet', 'Small Planet', 'Standard Planet', 'Large Planet', 'Huge Planet'];
		for (let idx = 0; idx < 5; idx++) {
			const category = idx;
			entries :+[labels[idx], () => { i.settings.global.set_world_size(category); next(); }];
		}
		entries :+['Custom Size', () => { #include('select_custom_mapsize')(i, next); }];
		i.sliding.show({title: 'Select size of planet', entries: entries});
		return;
	}
	i.sliding.show({
		title: 'Select size of planet',
		entries: [
			['Tiny Planet', () => {
				i.settings.global.map.size_x = 68;
				i.settings.global.map.size_y = 34;
				next();
			}],
			['Small Planet', () => {
				i.settings.global.map.size_x = 88;
				i.settings.global.map.size_y = 44;
				next();
			}],
			['Standard Planet', () => {
				i.settings.global.map.size_x = 112;
				i.settings.global.map.size_y = 56;
				next();
			}],
			['Large Planet', () => {
				i.settings.global.map.size_x = 140;
				i.settings.global.map.size_y = 70;
				next();
			}],
			['Huge Planet', () => {
				i.settings.global.map.size_x = 180;
				i.settings.global.map.size_y = 90;
				next();
			}],
		],
	});
};
