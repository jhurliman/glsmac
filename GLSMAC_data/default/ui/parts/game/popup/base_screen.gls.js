return {
	read_only: false,

	available_sections: [
		'top_buttons',
		'nutrients',
		'economy',
		'game_state',
		'middle_area',
		'facilities',
		'resources',
		'energy',
		'buttons',
		'bottom_bar',
		'specialist_selection',
		'rename',
		'queue_editor',
		'hurry',
        'workshop',
	],

	production_selection_no_hiding: {
		buttons: true,
		bottom_bar: true,
		specialist_selection: true,
		rename: true,
		queue_editor: true,
		hurry: true,
        workshop: true,
	},

	init: (p) => {

		this.sections = {};
		this.p = p;
		this.is_open = false;
		this.base_id = null;

		this.production_selection = null;

		p.ui.class('base-screen-frame').set({
			border: 'rgb(35,59,34),2',
		});
		p.ui.class('base-screen-block').extend('default-panel-inner').set({
			top: 3,
			left: 3,
			bottom: 3,
			right: 3,
		});

		p.ui.class('base-screen-header').extend('base-screen-block').set({
			align: 'top',
			height: 20,
		});
		p.ui.class('base-screen-body').extend('base-screen-block').set({
			align: 'top',
			top: 24,
		});

		p.ui.class('base-screen-side-frame').extend('base-screen-frame').set({
			width: 132,
		});
		p.ui.class('base-screen-side-middle-frame').extend('base-screen-frame').set({
			width: 539,
		});

		p.ui.class('base-screen-frame-title').set({
			align: 'center',
			font: 'arialnb.ttf:16',
			color: 'rgb(53,61,115)',
		});
		p.ui.class('base-screen-frame-text').set({
			font: 'arialnb.ttf:14', // TODO: investigate why non-bold fonts look bad (LOD settings?)
			color: 'rgb(59,111,128)',
		});

		p.ui.class('base-screen-frame-info-text').set({
			font: 'arialnb.ttf:14',
			color: 'rgb(42,101,120)',
		});

		p.ui.class('base-screen-frame-text-important').extend('base-screen-frame-text').set({
			color: 'rgb(120,164,212)',
		});

		p.ui.class('base-screen-side-header').extend('base-screen-body').set({
			left: 3,
			width: 127,
			height: 20,
		});

		p.ui.class('base-screen-side-header-text').set({
			font: 'arialnb.ttf:15',
			align: 'center',
		});

		return p.create('', 680, 442, (body, cb) => {

			this.body = body;

			body.listen(p.game, 'update_base', (e) => {
				if (this.is_open && e.base.id == this.base_id) {
					this.set({
						base: e.base,
					});
				}
			});
			body.listen(p.game, 'smac_specialist_menu', (e) => {
				if (this.is_open && #is_defined(e.base) && e.base.id == this.base_id) { this.sections.rename.hide_view(); this.sections.queue_editor.hide_view(); this.sections.hurry.hide_view(); this.sections.specialist_selection.open(e.base, e.selection); }
			});
			body.listen(p.game, 'smac_rename_menu', (e) => {
				if (this.is_open && #is_defined(e.base) && e.base.id == this.base_id) { this.sections.specialist_selection.hide_view(); this.sections.queue_editor.hide_view(); this.sections.hurry.hide_view(); this.sections.rename.open(e.base, e.selection); }
			});
			body.listen(p.game, 'base_despawn', (e) => {
				if (this.base_id == e.base_id) {
					this.sections.specialist_selection.hide_view();
					this.sections.rename.hide_view(); this.sections.queue_editor.hide_view(); this.sections.hurry.hide_view();
        if (#is_defined(this.sections.workshop)) { this.sections.workshop.hide_view(); }
					if (this.is_open) { this.p.modules.popup.hide('base_screen'); }
					this.base = null;
					this.base_id = null;
				}
			});

			const pp = {
				body: body,
				ui: p.ui,
				game: p.game,
				hide: p.hide,
				modules: p.modules,
				utils: {
					set_cells: parent.parent.set_cells,
					navigate_base: parent.parent.navigate_base,
					open_specialist_selection: parent.parent.open_specialist_selection,
					open_rename: parent.parent.open_rename,
                    open_workshop: parent.parent.open_workshop,
                    workshop_open_result: parent.parent.workshop_open_result,
                    workshop_closed: parent.parent.workshop_closed,
					open_queue_editor: parent.parent.open_queue_editor,
					open_hurry: parent.parent.open_hurry,
					can_edit_queue: parent.parent.can_edit_queue,
					can_rename: parent.parent.can_rename,
					show_production_selection: parent.parent.show_production_selection,
					hide_production_selection: parent.parent.hide_production_selection,
					update_production_selection: parent.parent.update_production_selection,
				},
			};

			for (s of this.available_sections) {
				this.sections[s] = #include('base_screen/' + s);
				this.sections[s].body = this.sections[s].init(pp);
			}

		});

	},

	set: (data) => {

		const game = this.p.game;
		const base = data.base;
		const owner = base.get_owner();
		const faction = owner.get_faction();

		this.base_id = base.id;
		if (#is_defined(data.read_only)) { this.read_only = data.read_only; }
		this.base = base;
		this.sections.top_buttons.set_base(base);

		const canonical = base.has_smac_resource_context;
		let intake = undefined;
		let consumption = undefined;
		let energy = undefined;
        let citizens = undefined;
		let ecodamage = canonical ? 'N/A' : 0;
        let mineral_surplus = undefined;
        let retained_resources = false;
		if (canonical) {
			// Fresh preparation only: drawing does not normalize stored food,
			// collect nutrients, post minerals or settle faction credits.
			const calculation = base.get_smac_resource_calculation();
            retained_resources = #is_defined(calculation.retained_only) && calculation.retained_only;
			if (#typeof(calculation.eco_damage) == 'Int') { ecodamage = calculation.eco_damage; }
            if (!retained_resources) { mineral_surplus = calculation.minerals.surplus; }
            citizens = {current: calculation.citizens, retained: calculation.retained_citizens, reason: calculation.citizens_unavailable};
			if (!retained_resources) { energy = base.get_smac_energy_calculation(); }
			intake = {NUTRIENTS: calculation.nutrients.intake, MINERALS: calculation.minerals.intake, ENERGY: calculation.raw_intake.ENERGY};
			consumption = {NUTRIENTS: calculation.nutrients.demand, MINERALS: calculation.minerals.demand};
			if (#is_defined(energy)) {
				intake.ENERGY = energy.raw_intake;
				consumption.ENERGY = energy.waste;
			}
		} else {
			intake = base.get_intake();
			consumption = base.get_consumption();
		}

        this.read_only = this.read_only || retained_resources;
        const read_only = this.read_only;
        this.sections.top_buttons.set_read_only(read_only);
        if (retained_resources) { this.sections.top_buttons.set_retained_resources(); }

		this.hide_production_selection();
		this.sections.buttons.show_default(); // Recompute availability after the viewed Base is bound.
		this.sections.specialist_selection.on_base_update(base);
		this.sections.rename.on_base_update(base);
		this.sections.queue_editor.on_base_update(base);
		this.sections.hurry.on_base_update(base);

		if (canonical) {
			let nutrients = undefined;
            if (!retained_resources) { nutrients = base.get_smac_nutrient_presentation(); }
			if (!#is_defined(nutrients)) { nutrients = {stored: base.get_accumulated_nutrients()}; }
			this.sections.nutrients.set_stored(nutrients);
		} else {
			this.sections.nutrients.set({
				rows: base.get_size() + 1,
				columns: game.get('map_growth_base'),
				filled: base.get_accumulated_nutrients(),
				profit: intake.NUTRIENTS,
				loss: consumption.NUTRIENTS,
			});
		}

		if (faction.is_progenitor) {
			this.sections.economy.set_energy_grid({
				// TODO
			});
		} else {
			this.sections.economy.set_commerce({
				// TODO
			});
		}

		this.sections.game_state.set({
			year: game.get_year(),
			energy: #is_defined(energy) ? energy.credits : (canonical ? 'N/A' : 0),
			ecodamage: ecodamage,
		});

		this.sections.facilities.set({base: base, entries: base.get_facilities()});

		const resource_data = {
			nutrients: {
				profit: intake.NUTRIENTS,
				loss: consumption.NUTRIENTS,
			},
			minerals: {
				profit: intake.MINERALS,
				loss: consumption.MINERALS,
			},
			energy: {
				profit: intake.ENERGY,
				loss: consumption.ENERGY,
			},
		};
		this.sections.resources.set(resource_data);

		if (canonical) {
			if (#is_defined(energy)) {
				const allocation = {};
				for (type of ['economy', 'labs', 'psych']) {
					const value = energy.allocated[type];
					const bonus = energy.final[type] - value;
					const percent = energy.rates[type] * 10;
					allocation[type] = {percent: percent, value: value, bonus: bonus};
				}
				this.sections.energy.set(allocation);
			} else {
				this.sections.energy.set_unavailable();
			}
		} else {
			const allocation_labs = 0.4;
			const allocation_psych = 0.2;

			const allocation = this.p.ui.format_placeholder_energy(resource_data.energy.profit, resource_data.energy.loss);
			const energy_data = {
				labs: {
					allocation: allocation_labs,
					value: allocation.labs,
					bonus: 0,
				},
				psych: {
					allocation: allocation_psych,
					value: allocation.psych,
					bonus: 0,
				},
			};
			energy_data.economy = {
				allocation: 1.0 - energy_data.labs.allocation - energy_data.psych.allocation,
				value: allocation.economy,
				bonus: 0,
			};
			this.sections.energy.set(energy_data);
		}

		this.sections.middle_area.set({
			base: base, citizens: citizens, read_only: read_only,
		});

		this.sections.bottom_bar.set({
			base: base,
            read_only: read_only,
            mineral_surplus: mineral_surplus,
		});
	},

	can_rename: () => { return !this.read_only && #is_defined(this.base) && this.base != null && #is_defined(this.base.get_smac_rename_state()); },
	can_edit_queue: () => { return !this.read_only && #is_defined(this.base) && this.base != null && #is_defined(this.base.get_smac_specialist_selection_state()); },

	open_hurry: () => {
		if (!this.is_open || !this.can_edit_queue()) { return; }
		const base = this.base; const data = base.get_smac_hurry_preview(0);
		if (!#is_defined(data)) { return; }
		this.hide_production_selection(); this.sections.specialist_selection.hide_view(); this.sections.rename.hide_view(); this.sections.queue_editor.hide_view();
		this.sections.hurry.open(base, data);
	},

	open_queue_editor: () => {
		if (!this.is_open || this.read_only) { return; }
		const base = this.base; const data = base.get_smac_queue_editor();
		if (!#is_defined(data)) { return; }
		this.hide_production_selection(); this.sections.specialist_selection.hide_view(); this.sections.rename.hide_view(); this.sections.hurry.hide_view();
		this.sections.queue_editor.open(base, data);
	},

    open_workshop: () => {
        if (!this.is_open || this.read_only) { return; }
        const data = this.base.get_smac_workshop();
        if (!#is_defined(data)) { return; }
        let selected = (0 - 1);
        if (#is_defined(this.production_choice)) { selected = #max((0 - 1), this.production_choice.source_id); }
        else {
            const production = this.base.get_production_state();
            if (#is_defined(production) && production.current_item.kind == 'unit' && #is_defined(production.current_item.smac_source_id)) { selected = production.current_item.smac_source_id; }
        }
        if (selected < 0) {
            // Native action1 routes facilities to help, not a New design.
            if (this.production_selection != null) { this.production_prompt.text = 'Facility help is unavailable.'; } return;
        }
        this.sections.workshop.begin(this.base, data, selected);
    },

    workshop_closed: () => { if (this.is_open && !this.read_only) { this.show_production_selection(); } },
    workshop_open_result: (success, message) => {
        if (!this.is_open) { return; }
        if (success) { this.hide_production_selection(); }
        else if (this.production_selection != null) { this.production_prompt.text = message; }
    },

	open_rename: () => {
		if (!this.is_open || !this.can_rename()) { return; }
		const base = this.base; const expected = base.get_smac_rename_state();
		this.hide_production_selection();
		this.p.game.event('open_smac_rename', {base: base, expected_state: expected});
	},

	open_specialist_selection: (base, pop_id) => {
		if (!this.is_open || this.read_only || base.id != this.base_id) { return; }
		const expected = base.get_smac_specialist_selection_state();
		if (!#is_defined(expected)) { return; }
		this.hide_production_selection();
		this.p.game.event('open_smac_specialist', {base: base, pop_id: pop_id, expected_state: expected});
	},

	navigate_base: (direction) => {
		// A required Colony Screen owns its completion callback and viewed Base.
		if (!this.is_open || this.p.game.get_pending_smac_colony_screen() != null) { return; }
		const neighbors = this.base.get_smac_base_navigation();
		if (!#is_defined(neighbors)) { return; }
		if (direction == 'previous') { this.p.game.select_base(neighbors.previous); }
		if (direction == 'next') { this.p.game.select_base(neighbors.next); }
	},

	on_hide: () => {
		this.sections.facilities.reset();
		this.sections.top_buttons.hide_view();
		this.sections.specialist_selection.hide_view();
		this.sections.rename.hide_view(); this.sections.queue_editor.hide_view(); this.sections.hurry.hide_view();
        if (#is_defined(this.sections.workshop)) { this.sections.workshop.hide_view(); }
		this.sections.bottom_bar.frame.hide();
		this.hide_production_selection();
		this.is_open = false;
		this.read_only = false;
	},

	on_show: () => {
		this.is_open = true;
		this.sections.bottom_bar.frame.show();
	},

	set_cells: (total_width, total_height, columns, rows, filled, profit, loss, cells_el, cell_baseclass, label_el, f_label) => {
		const progress = this.p.ui.format_resource_progress(profit, loss, filled, rows * columns);
		cells_el.clear();

		const width = #floor(#to_float(total_width) / #to_float(columns));
		const height = #floor(#to_float(total_height) / #to_float(rows));

		this.p.ui.class(cell_baseclass).set({
			width: width - 1,
			height: height - 1,
		});

		const offset_left = (total_width - (columns * width)) / 2;
		let left = offset_left;
		let top = (total_height - (rows * height)) / 2;

		let i = 0;
		let cls = '';

		for (let y = 0; y < rows; y++) {
			for (let x = 0; x < columns; x++) {
				if (i < progress.filled_cells) {
					if (progress.direction < 0 && i >= progress.end_cells) {
						cls = 'deficit';
					} else {
						cls = 'full';
					}
				} else if (i < progress.end_cells) {
					cls = 'pending';
				} else {
					cls = 'empty';
				}
				i++;
				cells_el.panel({
					class: cell_baseclass + '-' + cls,
					left: left + 1,
					top: top + 1,
				});
				left += width;
			}
			top += height;
			left = offset_left;
		}

		label_el.text = f_label(progress);
	},

	hide_production_selection: () => {
		if (this.production_selection != null) {
			this.production_selection.remove();
			this.production_selection = null;
			this.sections.buttons.show_default();
			for (s in this.sections) {
				if (!#is_defined(this.production_selection_no_hiding[s])) {
					this.sections[s].body.show();
				}
			}
		}
	},

	show_production_selection: () => {
		if (this.read_only) { return; }
		if (this.production_selection == null) {
			// Query before hiding anything: a stale/unavailable Base leaves the
			// ordinary screen intact. All gameplay values come from the native Base.
			let selection = undefined;
			if (this.base.has_smac_resource_context && this.base.get_owner().id == this.p.game.get_player().id) {
				selection = this.base.get_smac_production_choices();
			}
			for (s in this.sections) {
				if (!#is_defined(this.production_selection_no_hiding[s])) {
					this.sections[s].body.hide();
				}
			}
			this.production_selection = this.body.panel({
				class: 'default-panel',
				top: 0,
				left: 0,
				right: 0,
				bottom: 0,
			});
			this.sections.buttons.show_production_selection();
			this.production_choice = undefined;
			this.production_confirming = false;
			this.production_data = selection;
			this.production_prompt = this.production_selection.text({
				class: 'base-screen-frame-text-important', left: 20, top: 20,
				text: #is_defined(selection) ? 'Choose production (stored: ' + #to_string(selection.minerals) + ')' : 'Production selection unavailable for this Base',
			});
			if (#is_defined(selection)) {
				this.production_choices = this.production_selection.listview({
					left: 20, right: 20, top: 60, bottom: 12,
					itemsize: 40, padding: 2, has_hscroll: false, has_vscroll: true,
					vscroll_class: 'default-scroll-v',
				});
				for (choice of selection.choices) {
					const selected = choice;
					this.production_choices.button({
						class: 'game-popup-button', left: 0, right: 0,
						text: selected.name + ' — ' + #to_string(selected.cost) + ' minerals',
					}).on('click', () => {
						this.production_choice = selected;
						this.production_confirming = false;
						this.production_prompt.text = selected.name + ': stored ' + #to_string(selection.minerals)
							+ ', after switch ' + #to_string(selected.minerals) + '. Select OK.';
						return true;
					});
				}
			}
		}
	},

	update_production_selection: () => {
		if (this.read_only) { return; }
		if (this.production_selection != null) {
			if (!#is_defined(this.production_choice)) { return; }
			const choice = this.production_choice;
			// GSE object literals bind this to the new object. Capture the screen
			// values before either authenticated production event is constructed.
			const base = this.base;
			const expected_state = this.production_data.expected_state;
			if (choice.confirmation && !this.production_confirming) {
				// The changed-item prelude clears marked governor activation before
				// the loss decision. A later Cancel keeps production, not those bits.
				this.p.game.event('select_smac_production', {
					base: base, source_id: choice.source_id,
					expected_state: expected_state, confirm: false,
					expected_minerals: choice.minerals,
				});
				this.production_confirming = true;
				this.production_prompt.text = 'Retooling: ' + #to_string(this.production_data.minerals)
					+ ' → ' + #to_string(choice.minerals) + ' minerals. OK to confirm; Cancel keeps production.';
				return;
			}
			const confirm = this.production_confirming;
			this.p.game.event('select_smac_production', {
				base: base, source_id: choice.source_id,
				expected_state: expected_state, confirm: confirm,
				expected_minerals: choice.minerals,
			});
			this.hide_production_selection();
		}
	},

};
