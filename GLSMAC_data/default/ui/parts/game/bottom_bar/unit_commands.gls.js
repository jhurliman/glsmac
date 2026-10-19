return {
	clear: () => {
		this.foundation = null;
		this.unit = null;
		this.unit_id = null;
		this.frame.hide();
	},

	refresh: () => {
		this.buttons = [];
		this.frame.clear();
		if (!#is_defined(this.unit) || this.unit == null || !this.unit.is_conventional) {
			this.clear();
			return;
		}
		const commands = this.unit.query_conventional_commands();
		if (commands.kind == 'none') { this.frame.hide(); return; }
        if (this.foundation != null && this.foundation.unit.id == this.unit_id) {
            const prompt = this.foundation; const prompt_width = this.frame.width - 12;
            this.frame.text({left: 6, top: 4, font: 'arialnb.ttf:14', color: 'rgb(232,212,140)', text: 'Choose Governor settings before founding.'});
            this.frame.text({left: 6, top: 22, font: 'arialnb.ttf:12', color: 'rgb(184,192,96)', text: 'Use the default settings for newly founded Bases?'});
            const accept = this.frame.button({class: 'unit-command-button', left: 6, top: 42, width: prompt_width, height: 22, text: 'Use default Governor settings'});
            accept.on('click', () => {
                if (this.foundation != prompt) { return true; }
                const unit = prompt.unit; const expected = prompt.expected_state;
                this.p.game.event('found_base_with_default_governor', {unit: unit, expected_state: expected});
                this.foundation = null; this.refresh(); return true;
            });
            const cancel = this.frame.button({class: 'unit-command-button', left: 6, top: 66, width: prompt_width, height: 22, text: 'Cancel'});
            cancel.on('click', () => { if (this.foundation == prompt) { this.foundation = null; this.refresh(); } return true; });
            this.buttons :+ accept; this.buttons :+ cancel; this.frame.show(); return;
        }
		const orders = ['None', 'Farm', 'Road', 'Forest', 'Solar', 'Mine'];
		const heading = commands.held ? 'Holding position — stored work: ' + #to_string(commands.work_progress) : (commands.kind == 'former'
			? 'Former — Order: ' + orders[commands.work_order] + '  |  Stored work: ' + #to_string(commands.work_progress)
			: 'Colony Pod — Found a Base on an available site');
		this.frame.text({left: 6, top: 4, font: 'arialnb.ttf:14', color: 'rgb(232,212,140)', text: heading});
		this.frame.text({left: 6, top: 22, font: 'arialnb.ttf:12', color: 'rgb(184,192,96)',
			text: #sizeof(commands.actions) == 0 ? 'No available commands here.' : 'Click a command or press its key.'});
		const labels = ['Cancel [C]', 'Farm [F]', 'Road [R]', 'Forest [O]', 'Solar [S]', 'Mine [M]', 'Found Base [B]', 'Activate unit'];
		const selected_id = this.unit_id;
		let index = 0;
		for (action of commands.actions) {
			const selected_action = action;
			const column = index % 3;
			const row = index / 3;
			const width = (this.frame.width - 18) / 3;
			const button = this.frame.button({class: 'unit-command-button', left: 6 + column * (width + 3), top: 42 + row * 24,
				width: width, height: 22, text: labels[selected_action]});
			button.on('click', () => {
				// Revalidate the current selection and offered action; stale UI never
				// sends a retired wrapper or substitutes a newly selected Unit.
				if (!#is_defined(this.unit) || this.unit == null || this.unit_id != selected_id) { return true; }
				const current = this.unit.query_conventional_commands();
				let available = false;
				for (offered of current.actions) { if (offered == selected_action) { available = true; } }
				if (available) {
					const unit = this.unit;
					if (selected_action == 7) { this.p.game.event('unit_activate', {unit: unit}); }
					else if (selected_action == 6) { this.p.game.event('found_base', {unit: unit}); }
					else if (selected_action == 0) { this.p.game.event('cancel_unit_work', {unit: unit}); }
					else { this.p.game.event('work_unit', {unit: unit, action: selected_action}); }
				}
				// The native event is queued. Refresh when its committed Unit update arrives.
				return true;
			});
			this.buttons :+button;
			index++;
		}
		this.frame.show();
	},

	init: (p) => {
		this.p = p;
		this.foundation = null;
		this.unit = null;
		this.unit_id = null;
		this.buttons = [];
		p.ui.class('unit-command-button').extend('bottombar-panel-button').set({font: 'arialnb.ttf:13', color: 'rgb(232,212,140)'});
		this.frame = p.el.panel({class: 'bottombar-panel-inner', left: 266, right: 293, top: 72, bottom: 89, zindex: 0.7});
		this.frame.hide();
		this.frame.on('resize', (e) => { this.refresh(); });
		this.frame.listen(p.game, 'smac_governor_foundation', (e) => { this.unit = e.unit; this.unit_id = e.unit.id; this.foundation = e; this.refresh(); });
		this.frame.listen(p.game, 'unit_select', (e) => { this.unit = e.unit; this.unit_id = e.unit.id; this.refresh(); });
		this.frame.listen(p.map, 'unit_preview', (e) => {
			if (e.unit != null && this.unit_id == e.unit.id) { this.unit = e.unit; this.refresh(); }
		});
		this.frame.listen(p.game, 'unit_despawn', (e) => { if (this.unit_id == e.unit_id) { this.clear(); } });
		this.frame.listen(p.game, 'base_select', (e) => { this.clear(); });
		this.frame.listen(p.game, 'tile_select', (e) => { this.clear(); });
		this.frame.listen(p.game, 'turn', (e) => { this.clear(); });
		this.frame.listen(p.game, 'turn_status', (e) => { this.refresh(); });
		this.frame.listen(p.game, 'research_choice', (e) => { this.refresh(); });
	},
};
