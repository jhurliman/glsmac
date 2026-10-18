return {

	cell_columns: 11,

	init: (p) => {

		this.p = p;

		this.frame = p.body.panel({
			class: 'base-screen-frame',
			align: 'top left',
			top: 59,
			bottom: 7,
			left: 6,
			width: 129,
		});

		const body = this.frame.panel({
			class: 'default-panel-inner',
		});

		this.production_title = body.text({
			class: 'base-screen-frame-title',
			text: 'NOTHING',
			align: 'top center',
			top: 64,
		});

		p.ui.class('base-screen-bottombar-production-cell');
		p.ui.class('base-screen-bottombar-production-cell-empty').extend('base-screen-bottombar-production-cell').set({
			background: 'interface.pcx:crop(397,43,416,62)',
		});
		p.ui.class('base-screen-bottombar-production-cell-pending').extend('base-screen-bottombar-production-cell-empty').set({
			border: 'rgb(231,235,231),0',
		});
		p.ui.class('base-screen-bottombar-production-cell-full').extend('base-screen-bottombar-production-cell').set({
			background: 'interface.pcx:crop(418,43,437,62)',
		});

		this.production_cells = body.area({
			align: 'top center',
			left: 6,
			right: 6,
			top: 85,
		});

		this.production_label = body.text({
			class: 'base-screen-frame-text',
			align: 'bottom center',
			bottom: 20,
		});

		body.on('mousedown', (e) => {
			this.p.utils.show_production_selection();
			return true;
		});
	},

	set_stored: (production, presentation, surplus) => {
		this.production_title.text = #is_defined(presentation) ? presentation.name : 'UNAVAILABLE';
		this.production_cells.clear();
		this.production_cells.text({
			class: 'base-screen-frame-text',
			align: 'top center',
			text: #is_defined(production) ? 'Stored: ' + #to_string(production.minerals) : 'Stored: unavailable',
		});
        if (#is_defined(production) && #is_defined(presentation) && #is_defined(presentation.cost) && presentation.cost > 0) {
            this.production_cells.clear();
            this.production_cells.text({class: 'base-screen-frame-text', align: 'top center', text: 'Stored: ' + #to_string(production.minerals) + ' / ' + #to_string(presentation.cost)});
            let remaining = 'Remaining: unavailable';
            if (production.minerals >= (0 - 2147483648)) {
                let missing = presentation.cost - production.minerals;
                if (missing < 0) { missing = 0; }
                remaining = 'Remaining: ' + #to_string(missing);
            }
            this.production_cells.text({class: 'base-screen-frame-text', align: 'top center', top: 18, text: remaining});
            if (#is_defined(surplus)) {
                this.production_cells.text({class: 'base-screen-frame-text', align: 'top center', top: 36, text: 'Net/turn: ' + (surplus >= 0 ? '+' : '') + #to_string(surplus)});
            }
        }
		this.production_label.text = #is_defined(presentation) && #is_defined(presentation.forecast) ? presentation.forecast : 'Forecast unavailable';
	},

	set: (data) => {
		this.production_title.text = 'NOTHING';

		this.p.utils.set_cells(
			108,
			60,
			data.columns,
			data.rows,
			data.filled,
			data.pending,
			0,
			this.production_cells,
			'base-screen-bottombar-production-cell',
			this.production_label,
			(progress) => {
				return progress.turns + ' TURNS';
			},
		);

	},

};
