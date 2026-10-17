return {
	refresh: () => {
		const pending = this.p.game.get_pending_smac_research();
		if (pending == null) {
			this.pending = null;
			this.choice = null;
			this.submitted = false;
			this.window.hide();
			return;
		}
		if (this.pending != null && this.pending.request_id == pending.request_id) { return; }
		this.pending = pending;
		this.choice = null;
		this.submitted = false;
		this.choice_buttons = [];
		this.list.clear();
		this.prompt.text = pending.owner_name + (pending.bonus ? ': choose a bonus discovery' : ': choose research');
		this.selection.text = 'Select a technology, then confirm.';
		const categories = ['Explore', 'Discover', 'Build', 'Conquer'];
		const request_id = pending.request_id;
		let top = 0;
		for (choice of pending.choices) {
			const offered = choice;
			const button = this.list.button({
				class: 'game-popup-button', left: 4, right: 4, top: top, height: 28,
				text: offered.name + ' (' + categories[offered.category] + ')',
			});
			button.on('click', () => {
				if (this.pending != null && this.pending.request_id == request_id && !this.submitted) {
					this.choice = offered.id;
					this.selection.text = 'Selected: ' + offered.name;
				}
				return true;
			});
			this.choice_buttons :+button;
			top += 32;
		}
		this.p.modules.menu.close_all();
		this.window.show();
	},

	submit: () => {
		if (this.pending == null || this.choice == null || this.submitted) { return; }
		// Capture before the object literal: its GSE this is the new payload.
		const request_id = this.pending.request_id;
		const technology_id = this.choice;
		this.submitted = true;
		this.selection.text = 'Waiting for research selection…';
		this.p.game.event('choose_research', {request_id: request_id, technology_id: technology_id});
		// Only an authoritative research_choice notification clears/replaces it.
	},

	init: (p) => {
		this.p = p;
		this.pending = null;
		this.choice = null;
		this.submitted = false;
		this.choice_buttons = [];
		this.window = p.root.window({
			class: 'game-popup', width: 620, height: 420,
			blocker: 'rgba(0, 0, 0, 180)',
		});
		this.window.hide();
		this.prompt = this.window.text({class: 'game-popup-text', top: 12, left: 12, right: 12});
		this.list = this.window.scrollview({left: 8, right: 8, top: 42, bottom: 86, has_vscroll: true, has_hscroll: false});
		this.selection = this.window.text({class: 'game-popup-text', bottom: 55, left: 12, right: 12});
		this.confirm_button = this.window.button({class: 'game-popup-button', bottom: 12, left: 12, right: 320, height: 28, text: 'Confirm Research'});
		this.confirm_button.on('click', () => { this.submit(); return true; });
		this.reset_button = this.window.button({class: 'game-popup-button', bottom: 12, left: 320, right: 12, height: 28, text: 'Return to Main Menu'});
		this.reset_button.on('click', () => { this.p.glsmac.reset(); return true; });
		this.window.on('keydown', (e) => {
			if (e.modifiers == {} && e.code == 'ENTER') { this.submit(); }
			if (e.modifiers == {ctrl: true, shift: true} && e.code == 'Q') { this.p.glsmac.reset(); }
			return true; // Escape and gameplay keys cannot dismiss required choice.
		});
		this.window.listen(p.game, 'research_choice', (e) => { this.refresh(); });
		this.refresh(); // A first-turn request may predate start_ui.
	},
};
