return {
	refresh: () => {
		const pending = this.p.game.get_pending_smac_colony();
		if (pending == null) {
			this.pending = null;
			this.submitted = false;
			this.window.hide();
			return;
		}
		const former = #is_defined(pending.kind) && pending.kind == 'former_owner';
		if (this.pending != null && this.pending.request_id == pending.request_id && !former) { return; }
		this.pending = pending;
		this.submitted = false;
		this.buttons.clear();
		this.choice_buttons = [];
		this.former_choice = null;
		this.former_confirm.hide(); this.former_cancel.hide();
		this.window.height = 222; this.buttons.height = 32; this.status.top = 126;
		this.description.text = 'The Base has only one citizen. Completing the Pod abandons it.';
		if (former) {
			this.window.height = 384; this.buttons.height = 144; this.status.top = 242;
			this.prompt.text = pending.base_name + ': former faction';
			this.description.text = 'This older save lacks the Base\'s former faction. Select it to continue the riot decision.';
			this.status.text = 'Select a faction, then confirm. Cancel keeps this question available.';
			const request_id = pending.request_id;
			let index = 0;
			for (choice of pending.choices) {
				const offered = choice;
				const button = this.buttons.button({class: 'game-popup-button', left: (index % 2) * 270, width: 262, top: (index / 2) * 32, height: 28, text: offered.name});
				button.on('click', () => {
					if (this.pending != null && this.pending.request_id == request_id && !this.submitted) {
						this.former_choice = offered.id; this.status.text = 'Selected: ' + offered.name;
					}
					return true;
				});
				this.choice_buttons :+button; index++;
			}
			this.former_confirm.show(); this.former_cancel.show();
			this.p.modules.menu.close_all(); this.window.show(); return;
		}
		this.prompt.text = pending.base_name + ': Colony Pod completion';
		this.status.text = 'Choose how to continue.';
		const request_id = pending.request_id;
		const choices = [
			{value: 'delay', label: 'Delay Production'},
			{value: 'screen', label: 'Base Control Screen'},
			{value: 'accept', label: 'Accept Abandonment'},
		];
		let left = 0;
		for (choice of choices) {
			const action = choice.value;
			const button = this.buttons.button({class: 'game-popup-button', left: left, width: 172, top: 0, height: 30, text: choice.label});
			button.on('click', () => { this.submit(request_id, action); return true; });
			this.choice_buttons :+button;
			left += 180;
		}
		this.p.modules.menu.close_all();
		this.window.show();
	},

	answer_former: (cancel) => {
		if (this.pending == null || this.submitted || (!cancel && this.former_choice == null)) { return; }
		const request_id = this.pending.request_id;
		const answer = cancel ? -1 : this.former_choice;
		this.p.game.event('choose_former_faction', {request_id: request_id, former_owner: answer});
		if (cancel) {
			this.former_choice = null; this.status.text = 'Decision postponed. The turn remains paused; you can save or choose later.';
		} else { this.submitted = true; this.status.text = 'Preparing the riot decision…'; }
	},

	submit: (request_id, choice) => {
		if (this.pending == null || this.pending.request_id != request_id || this.submitted) { return; }
		// Preflight may reject Accept while leaving this question available.
		// Only a successfully queued answer blocks further choices.
		this.p.game.event('choose_colony', {request_id: request_id, choice: choice});
		this.submitted = true;
		this.status.text = 'Waiting for the production decision…';
		// Keep the choice blocked until the owning game retires this request.
	},

	refresh_screen: () => {
		const pending = this.p.game.get_pending_smac_colony_screen();
		if (pending == null) {
			if (this.screen != null) {
				this.screen = null;
				this.screen_submitted = false;
				this.p.modules.popup.clear();
				if (this.pending == null) { this.window.hide(); }
			}
			return;
		}
		if (this.screen != null && this.screen.request_id == pending.request_id) { return; }
		let selected = null;
		for (base of this.p.game.get_bm().get_bases()) {
			if (base.id == pending.base_id) { selected = base; }
		}
		if (selected == null) { return; }
		this.screen = pending;
		this.screen_submitted = false;
		this.window.hide();
		const request_id = pending.request_id;
		this.p.modules.popup.clear();
		this.p.modules.popup.set('base_screen', {base: selected});
		this.p.modules.popup.show('base_screen', (result) => {
			if (this.screen == null || this.screen.request_id != request_id || this.screen_submitted) { return; }
			const current = this.p.game.get_pending_smac_colony_screen();
			if (current == null || current.request_id != request_id) { return; }
			this.screen_submitted = true;
			this.buttons.clear();
			this.prompt.text = 'Continuing the turn…';
			this.status.text = 'Waiting for Base Control to close.';
			this.window.show();
			this.p.game.event('close_colony_screen', {request_id: request_id});
		});
	},

	save: () => {
		if (this.pending == null || this.submitted || this.screen != null) { return; }
		const request_id = this.pending.request_id;
		this.window.hide();
		this.p.modules.popup.show('save_campaign', (result) => {
			if (this.pending != null && this.pending.request_id == request_id) { this.window.show(); }
		});
	},

	init: (p) => {
		this.p = p;
		this.pending = null;
		this.submitted = false;
		this.screen = null;
		this.screen_submitted = false;
		this.choice_buttons = [];
		this.window = p.root.window({class: 'game-popup', zindex: 0.9, width: 568, height: 222, blocker: 'rgba(0, 0, 0, 180)'});
		this.window.hide();
		this.prompt = this.window.text({class: 'game-popup-text', top: 12, left: 12, right: 12});
		this.description = this.window.text({class: 'game-popup-text', top: 44, left: 12, right: 12, text: 'The Base has only one citizen. Completing the Pod abandons it.'});
		this.buttons = this.window.panel({left: 12, right: 12, top: 80, height: 32});
		this.status = this.window.text({class: 'game-popup-text', top: 126, left: 12, right: 12});
		this.former_confirm = this.window.button({class: 'game-popup-button', align: 'bottom left', bottom: 48, left: 12, right: 288, height: 28, text: 'Confirm Former Faction'});
		this.former_confirm.on('click', () => { this.answer_former(false); return true; });
		this.former_cancel = this.window.button({class: 'game-popup-button', align: 'bottom left', bottom: 48, left: 288, right: 12, height: 28, text: 'Cancel Decision'});
		this.former_cancel.on('click', () => { this.answer_former(true); return true; });
		this.former_confirm.hide(); this.former_cancel.hide();
		this.save_button = this.window.button({class: 'game-popup-button', align: 'bottom left', bottom: 12, left: 12, right: 288, height: 28, text: 'Save Campaign…'});
		this.save_button.on('click', () => { this.save(); return true; });
		this.reset_button = this.window.button({class: 'game-popup-button', align: 'bottom left', bottom: 12, left: 288, right: 12, height: 28, text: 'Return to Main Menu'});
		this.reset_button.on('click', () => { this.p.glsmac.reset(); return true; });
		this.window.on('keydown', (e) => {
			if (e.modifiers == {ctrl: true, shift: true} && e.code == 'Q') { this.p.glsmac.reset(); }
			// Escape/window-close mappings are not established by the receipt.
			// Neither those keys nor Enter submit an invented production answer.
			return true;
		});
		this.window.listen(p.game, 'colony_choice', (e) => { this.refresh(); });
		this.window.listen(p.game, 'colony_screen', (e) => { this.refresh_screen(); });
		this.refresh();
		this.refresh_screen();
	},
};
