return {
	init: (p) => {
		this.p = p;
		this.active = false;
		this.base = null;
		this.window = p.ui.root.window({class: 'game-popup', zindex: 0.9, width: 520, height: 388, blocker: 'rgba(0, 0, 0, 180)'});
		this.window.hide();
		this.title = this.window.text({class: 'game-popup-text', left: 12, right: 12, top: 12});
		this.info = this.window.text({class: 'game-popup-text', font: 'arialnb.ttf:14', left: 12, right: 12, top: 44});
		this.choices = this.window.listview({left: 12, right: 12, top: 76, bottom: 90, itemsize: 28, padding: 2,
			has_hscroll: false, has_vscroll: true, vscroll_class: 'default-scroll-v'});
		this.status = this.window.text({class: 'game-popup-text', align: 'bottom left', left: 12, right: 12, bottom: 54});
		this.window.button({class: 'game-popup-button', align: 'bottom left', left: 12, right: 266, bottom: 12, height: 28, text: 'Confirm'}).on('click', () => { this.submit(); return true; });
		this.window.button({class: 'game-popup-button', align: 'bottom left', left: 266, right: 12, bottom: 12, height: 28, text: 'Cancel'}).on('click', () => { this.cancel(); return true; });
		this.window.on('keydown', (e) => {
			if (e.modifiers == {} && e.code == 'ESCAPE') { this.cancel(); }
			if (e.modifiers == {} && e.code == 'ENTER') { this.submit(); }
			return true;
		});
		for (event of ['research_choice', 'colony_choice', 'colony_screen', 'turn', 'smac_ending']) {
			this.window.listen(p.game, event, (e) => { this.hide_view(); });
		}
		this.window.listen(p.game, 'turn_status', (e) => { if (e.status == 'please_wait') { this.hide_view(); } });
		return this.window;
	},
	open: (base, selection) => {
		this.base = base;
		this.selection = selection;
		this.selected = selection.current_type;
		this.active = true;
		this.title.text = base.name + ': specialist selection';
		this.info.text = selection.creation_limited ? 'More than 16 live specialists is unavailable.'
			: selection.size_explanation_requested ? 'Some types require Base size ' + #to_string(selection.minimum_size) + '.' : 'Choose Worker or an available specialist.';
		this.choices.clear();
		for (choice of selection.choices) {
			const selected = choice;
			if (selected.type == selection.current_type) { this.status.text = 'Selected: ' + selected.name; }
			this.choices.button({class: 'game-popup-button', left: 0, right: 0, text: selected.name}).on('click', () => {
				this.selected = selected.type; this.status.text = 'Selected: ' + selected.name; return true;
			});
		}
		this.window.show();
	},
	submit: () => {
		if (!this.active) { return; }
		const base = this.base;
		const data = this.selection;
		const selected = this.selected;
		this.p.game.event('select_smac_specialist', {base: base, pop_id: data.pop_id, request_id: data.request_id,
			expected_state: data.expected_state, type: selected});
		this.hide_view();
	},
	cancel: () => {
		if (!this.active) { return; }
		const base = this.base;
		const data = this.selection;
		this.p.game.event('cancel_smac_specialist', {base: base, pop_id: data.pop_id, request_id: data.request_id, expected_state: data.expected_state});
		this.hide_view();
	},
	on_base_update: (base) => {
		if (this.active && (base.id != this.base.id || base.get_smac_specialist_selection_state() != this.selection.expected_state)) { this.hide_view(); }
	},
	hide_view: () => { this.active = false; this.base = null; this.window.hide(); },
};
