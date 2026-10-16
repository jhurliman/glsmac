return (i, next) => {
	const select = (id) => {
		i.glsmac.deinit();
		i.glsmac.select_ruleset(id);
		i.popup.hide();
		next();
		return true;
	};
	i.popup.show({
		title: 'Game Ruleset',
		width: 480,
		height: 130,
		generator: (body) => {
			body.text({
				class: 'popup-text',
				align: 'center',
				text: 'Choose a game. Both rulesets are in development.',
			});
		},
		buttons: [
			{
				style: {text: 'Alpha Centauri', align: 'left'},
				onclick: () => { return select('smac'); },
			},
			{
				style: {text: 'Alien Crossfire', align: 'right'},
				onclick: () => { return select('smax'); },
			},
		],
	});
};
