return {
	refresh: () => {
		const pending = this.p.game.get_pending_smac_colony();
		if (pending == null) {
			this.pending = null;
			this.submitted = false;
			this.window.hide();
			return;
		}
		if (this.pending != null && this.pending.request_id == pending.request_id) { return; }
		this.pending = pending;
		this.submitted = false;
		this.buttons.clear();
		this.choice_buttons = [];
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

	submit: (request_id, choice) => {
		if (this.pending == null || this.pending.request_id != request_id || this.submitted) { return; }
		this.submitted = true;
		this.status.text = 'Waiting for the production decision…';
		this.p.game.event('choose_colony', {request_id: request_id, choice: choice});
		// Keep the choice blocked until the owning game retires this request.
	},

	init: (p) => {
		this.p = p;
		this.pending = null;
		this.submitted = false;
		this.choice_buttons = [];
		this.window = p.root.window({class: 'game-popup', width: 568, height: 222, blocker: 'rgba(0, 0, 0, 180)'});
		this.window.hide();
		this.prompt = this.window.text({class: 'game-popup-text', top: 12, left: 12, right: 12});
		this.window.text({class: 'game-popup-text', top: 44, left: 12, right: 12, text: 'The Base has only one citizen. Completing the Pod abandons it.'});
		this.buttons = this.window.panel({left: 12, right: 12, top: 80, height: 32});
		this.status = this.window.text({class: 'game-popup-text', top: 126, left: 12, right: 12});
		this.reset_button = this.window.button({class: 'game-popup-button', bottom: 12, left: 12, right: 12, height: 28, text: 'Return to Main Menu'});
		this.reset_button.on('click', () => { this.p.glsmac.reset(); return true; });
		this.window.on('keydown', (e) => {
			if (e.modifiers == {ctrl: true, shift: true} && e.code == 'Q') { this.p.glsmac.reset(); }
			// Escape/window-close mappings are not established by the receipt.
			// Neither those keys nor Enter submit an invented production answer.
			return true;
		});
		this.window.listen(p.game, 'colony_choice', (e) => { this.refresh(); });
		this.refresh();
	},
};
