return (i) => {
	let choices = null;
    let remembered = 'RANDOM';
	const fm = i.glsmac.game.get_fm();
	const has_preview = #is_defined(fm.import_selection_preview);
    const original = has_preview && i.glsmac.game.get_ruleset() == 'smac';
	let sources = {};
	let preview_data = {};
	let preview_panel = null;
	let details = false;
	if (has_preview) {
		for (entry of #include('../../../../factions').get_roster(i.glsmac.game.get_ruleset())) {
			const id = #uppercase(entry[0]);
			sources[id] = '' + entry[1];
			preview_data[id] = fm.import_selection_preview(entry[1] + '.txt', #uppercase(entry[1]));
		}
	}
	const render_preview = (id) => {
		if (preview_panel == null) { return; }
		preview_panel.clear();
		if (!#is_defined(preview_data[id])) { preview_panel.height = 0; return; }
		const data = preview_data[id];
		preview_panel.height = 190;
		const portrait = preview_panel.surface({left: 10, top: 10, width: 131, height: 157});
		portrait.smacfactionportrait(sources[id] + '.pcx');
		const paragraphs = details ? data.details : data.quotation;
		let top = 10;
		const probe = preview_panel.text({class: 'popup-text', font: 'arialn.ttf:16:mono-bold', text: ''});
		const draw_line = (line) => {
			preview_panel.text({class: 'popup-text', font: 'arialn.ttf:16:mono-bold', left: 152, top: top, text: line});
			top += 20;
		};
		for (paragraph of paragraphs) {
			let line = '';
			for (word of #split(paragraph, ' ')) {
				if (word != '') {
					const candidate = line == '' ? '' + word : line + ' ' + word;
					probe.text = candidate;
					if (line != '' && probe.measure().width > 578) { draw_line(line); line = '' + word; }
					else { line = candidate; }
				}
			}
			draw_line(line);
		}
		probe.remove();
		if (top + 10 > 190) { preview_panel.height = top + 10; }
	};
	let items = original ? [] : [['RANDOM', 'Random faction']];
    let row_index = 0;
	for (faction of i.glsmac.game.get_fm().list()) {
		// Use the configured ruleset's stable IDs, not display-name inference.
		const id = faction.id;
		const name = #is_defined(faction.display_identity) ? faction.display_identity.faction_name : faction.name;
		if (has_preview && #is_defined(preview_data[id])) {
			let row_style = {textalign: 'left', textleft: 6};
            if (original) { row_style = row_style + {left: 0, right: 0, top: row_index * 19,
                height: 16, textbaseline: 14, smac_frame_phase: [0, row_index * 19],
                righttext: '(' + preview_data[id].descriptor + ')'}; }
            items :+[id, original ? name : name + ' (' + preview_data[id].descriptor + ')', row_style];
		} else { items :+[id, name]; }
        row_index++;
	}
    if (original && #sizeof(items) > 0) { remembered = '' + (items[0])[0]; }
	let dialog = {
		title: 'Select Faction',
		width: 480,
		height: 80 + #sizeof(items) * 20,
		generator: (body) => {
			choices = body.choicelist({
				left: original ? 0 : 10, right: original ? 0 : 10, top: original ? 0 : 16,
				keyboard: true,
				itemclass: original ? 'smac-faction-row' : 'popup-list-button',
				itemheight: original ? 16 : 19, itempadding: original ? 0 : 1,
				items: items,
			});
            choices.value = remembered;
			if (has_preview) { choices.on('select', (e) => { remembered = '' + e.value; render_preview(remembered); return true; }); }
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
	};
	if (has_preview) {
		dialog.title = 'CHOOSE A FACTION';
		dialog.width = 580;
		dialog.placement = {align: 'bottom right', right: 10, bottom: 10};
		dialog.preview = (root) => {
			preview_panel = root.panel({class: 'popup', align: 'top left', left: 50, top: 50, width: 750, height: 0});
			render_preview(remembered); return preview_panel;
		};
		for (button of dialog.buttons) { button.style.width = 180; }
		dialog.buttons :+{style: {text: 'Faction Details', align: 'center', width: 190}, onclick: () => { details = !details; render_preview(remembered); return true; }};
	}
    if (original) { dialog.smac_kind = 'faction'; }
	i.popup.show(dialog);
};
