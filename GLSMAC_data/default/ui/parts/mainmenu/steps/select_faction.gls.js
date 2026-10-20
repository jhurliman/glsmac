return (i) => {
	let choices = null;
    let remembered = 'RANDOM';
	let items = [['RANDOM', 'Random faction']];
	for (faction of i.glsmac.game.get_fm().list()) {
		// Use the configured ruleset's stable IDs, not display-name inference.
		const id = faction.id;
		const name = #is_defined(faction.display_identity) ? faction.display_identity.faction_name : faction.name;
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
            choices.value = remembered;
		},
		buttons: [
			{
				style: {text: 'OK', align: 'left', is_ok: true},
				onclick: () => {
					const selected_id = '' + choices.value;
                    remembered = selected_id;
					if (selected_id == 'RANDOM') { i.glsmac.start_game(); }
					else {
                        let selected = null;
                        for (faction of i.glsmac.game.get_fm().list()) {
                            if (faction.id == selected_id) { selected = faction; }
                        }
                        if (selected != null && #is_defined(selected.display_identity) && #is_defined(selected.display_identity.setup)) {
                            i.steps.select_identity(i, selected);
                        } else { i.glsmac.start_game(selected_id); }
                    }
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
