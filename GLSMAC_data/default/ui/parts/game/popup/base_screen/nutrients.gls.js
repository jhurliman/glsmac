return {

	init: (p) => {

		this.p = p;

		this.frame = p.body.panel({
			class: 'base-screen-side-frame',
			align: 'top left',
			top: 32,
			left: 3,
			height: 154,
		});

		const header = this.frame.panel({
			class: 'base-screen-header',
		});
		header.text({
			class: 'base-screen-frame-title',
			text: 'NUTRIENTS',
		});

		const body = this.frame.panel({
			class: 'base-screen-body',
		});

		p.ui.class('base-screen-nutrients-cell');
		p.ui.class('base-screen-nutrients-cell-empty').extend('base-screen-nutrients-cell').set({
			background: 'interface.pcx:crop(397,22,416,41)',
		});
		p.ui.class('base-screen-nutrients-cell-pending').extend('base-screen-nutrients-cell-empty').set({
			border: 'rgb(212,208,116),0',
		});
		p.ui.class('base-screen-nutrients-cell-full').extend('base-screen-nutrients-cell').set({
			background: 'interface.pcx:crop(418,22,437,41)',
		});
		p.ui.class('base-screen-nutrients-cell-deficit').extend('base-screen-nutrients-cell-full').set({
			border: 'rgb(156,16,16),0',
		});

		this.growth_cells = body.area({
			left: 6,
			top: 10,
			right: 6,
			bottom: 34,
		});

		this.growth_label = body.text({
			class: 'base-screen-frame-text',
			align: 'bottom center',
			bottom: 12,
		});

		return this.frame;
	},

	// Original label and prospective food overlay on current resource inputs.
	// This is not a complete prediction of the next upkeep:
	// production and allocation can change before the population step runs.
	set_stored: (data) => {
		const known = #is_defined(data.threshold);
		const label = known ? data.display_label : 'Growth unavailable';
		// Keep modded or zero-width boxes numeric rather than dividing by zero
		// or creating an unbounded number of invisible one-pixel cells.
		if (known && data.cost_per_step > 0 && data.cost_per_step <= 56 && data.rows > 0 && data.rows <= 45) {
			this.p.utils.set_cells(112, 90, data.cost_per_step, data.rows,
				data.stored, data.surplus, 0, this.growth_cells,
				'base-screen-nutrients-cell', this.growth_label, (progress) => { return label; });
		} else {
			this.growth_cells.clear();
			this.growth_cells.text({class: 'base-screen-frame-text', align: 'center', text: 'Stored: ' + #to_string(data.stored)});
			this.growth_label.text = label;
		}
	},

	set: (data) => {
		this.p.utils.set_cells(
			112,
			90,
			data.columns,
			data.rows,
			data.filled,
			data.profit,
			data.loss,
			this.growth_cells,
			'base-screen-nutrients-cell',
			this.growth_label,
			(progress) => {
				let progress_text = '';
				if (progress.direction > 0) {
					progress_text = 'Growth: ' + progress.turns;
					if (progress.turns == '1') {
						progress_text += ' turn';
					} else {
						progress_text += ' turns';
					}
				} else if (progress.direction < 0) {
					progress_text = 'Hunger!';
				} else {
					progress_text = 'Growth: -stagnant-';
				}
				return progress_text;
			},
		);
	},

};
