return (i) => {
	let choices = null;
	let items = [['RANDOM', 'Random faction']];
	for (faction of i.glsmac.game.get_fm().list()) {
		// Use the configured ruleset's stable IDs, not display-name inference.
		const id = faction.id;
		const name = faction.name;
		items :+[id, name];
	}
	i.popup.show({
		title: 'Select Faction',
		width: 480,
		height: 80 + #sizeof(items) * 20,
		generator: (body) => {
			choices = body.choicelist({
				left: 10, right: 10, top: 16,
				keyboard: true,
				itemclass: 'popup-list-button',
				itemheight: 19, itempadding: 1,
				items: items,
			});
		},
		buttons: [
			{
				style: {text: 'OK', align: 'left', is_ok: true},
				onclick: () => {
					if (choices.value == 'RANDOM') { i.glsmac.start_game(); }
					else { i.glsmac.start_game(choices.value); }
					return true;
				},
			},
			{
				style: {text: 'Cancel', align: 'right', is_cancel: true},
				onclick: () => { i.popup.back(); return true; },
			},
		],
	});
};
