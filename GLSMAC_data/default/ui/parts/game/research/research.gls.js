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
        this.is_blind = #is_defined(pending.blind) && pending.blind;
        this.confirm_button.text = this.is_blind ? 'Set Priorities' : 'Confirm Research';
        this.reset_button.text = this.is_blind ? 'Keep Priorities' : 'Return to Main Menu';
        this.prompt.text = pending.owner_name + (this.is_blind ? ': research priorities' : (pending.bonus ? ': choose a bonus discovery' : ': choose research'));
		this.selection.text = 'Select a technology, then confirm.';
		const categories = ['Explore', 'Discover', 'Build', 'Conquer'];
		const request_id = pending.request_id;
		let top = 0;
        if (this.is_blind) {
            const axes = ['Explore', 'Discover', 'Build', 'Conquer'];
            this.priority_values = [];
            for (let i = 0; i < 4; i++) {
                const axis = i;
                this.priority_values :+(pending.priorities[axis] != 0 ? 1 : 0);
                const label = (this.priority_values[axis] != 0 ? '[X] ' : '[ ] ') + axes[axis];
                const button = this.list.button({class: 'game-popup-button', left: 4, right: 4, top: top, height: 28, text: label});
                button.on('click', () => {
                    if (this.pending != null && this.pending.request_id == request_id && !this.submitted) {
                        this.priority_values[axis] = 1 - this.priority_values[axis];
                        button.text = (this.priority_values[axis] != 0 ? '[X] ' : '[ ] ') + axes[axis];
                    }
                    return true;
                });
                this.choice_buttons :+button;
                top += 32;
            }
            this.selection.text = 'Choose any priorities, including none. Keep preserves the previous values.';
        }
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

    submit_priorities: (cancel) => {
        if (this.pending == null || !this.is_blind || this.submitted) { return; }
        const request_id = this.pending.request_id;
        let mask = null;
        if (!cancel) {
            mask = 0;
            const weights = [1, 2, 4, 8];
            for (let i = 0; i < 4; i++) { mask += weights[i] * this.priority_values[i]; }
        }
        this.submitted = true;
        this.selection.text = 'Waiting for research selection…';
        this.p.game.event('set_research_priorities', {request_id: request_id, mask: mask});
    },

	submit: () => {
        if (this.is_blind) { this.submit_priorities(false); return; }
		if (this.pending == null || this.choice == null || this.submitted) { return; }
		// Capture before the object literal: its GSE this is the new payload.
		const request_id = this.pending.request_id;
		const technology_id = this.choice;
		this.submitted = true;
		this.selection.text = 'Waiting for research selection…';
		this.p.game.event('choose_research', {request_id: request_id, technology_id: technology_id});
		// Only an authoritative research_choice notification clears/replaces it.
	},

	save: () => {
		if (this.pending == null || this.submitted) { return; }
		const request_id = this.pending.request_id;
		this.window.hide();
		this.p.modules.popup.show('save_campaign', (result) => {
			if (this.pending != null && this.pending.request_id == request_id) { this.window.show(); }
		});
	},

	init: (p) => {
		this.p = p;
        this.is_blind = false;
        this.priority_values = [];
		this.pending = null;
		this.choice = null;
		this.submitted = false;
		this.choice_buttons = [];
		this.window = p.root.window({
			class: 'game-popup', zindex: 0.9, width: 620, height: 456,
			blocker: 'rgba(0, 0, 0, 180)',
		});
		this.window.hide();
		this.prompt = this.window.text({class: 'game-popup-text', align: 'top left', top: 12, left: 12, right: 12});
		this.list = this.window.scrollview({left: 8, right: 8, top: 42, bottom: 122, vscroll_class: 'default-scroll-v', has_vscroll: true, has_hscroll: false});
		this.selection = this.window.text({class: 'game-popup-text', align: 'bottom left', bottom: 91, left: 12, right: 12});
		this.save_button = this.window.button({class: 'game-popup-button', align: 'bottom left', bottom: 48, left: 12, right: 12, height: 28, text: 'Save Campaign…'});
		this.save_button.on('click', () => { this.save(); return true; });
		this.confirm_button = this.window.button({class: 'game-popup-button', align: 'bottom left', bottom: 12, left: 12, right: 320, height: 28, text: 'Confirm Research'});
		this.confirm_button.on('click', () => { this.submit(); return true; });
		this.reset_button = this.window.button({class: 'game-popup-button', align: 'bottom left', bottom: 12, left: 320, right: 12, height: 28, text: 'Return to Main Menu'});
		this.reset_button.on('click', () => { if (this.is_blind) { this.submit_priorities(true); } else { this.p.glsmac.reset(); } return true; });
		this.window.on('keydown', (e) => {
			if (e.modifiers == {} && e.code == 'ENTER') { this.submit(); }
            if (e.modifiers == {} && e.code == 'ESCAPE' && this.is_blind) { this.submit_priorities(true); }
			if (e.modifiers == {ctrl: true, shift: true} && e.code == 'Q') { this.p.glsmac.reset(); }
			return true; // Escape and gameplay keys cannot dismiss required choice.
		});
		this.window.listen(p.game, 'research_choice', (e) => { this.refresh(); });
		this.refresh(); // A first-turn request may predate start_ui.
	},
};
