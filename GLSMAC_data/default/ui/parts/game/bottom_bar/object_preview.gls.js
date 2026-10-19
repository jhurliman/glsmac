return {

	get_morale: (type, value) => {
		if (!#is_defined(this.moralesets[type])) {
			this.moralesets[type] = this.um.get_moraleset('NATIVE');
		}
		if (value >= #sizeof(this.moralesets[type])) {
			#print('Morale not found: ' + type + '/' + #to_string(value));
			return '(???)';
		}
		return this.moralesets[type][value];
	},

	format_movement: (movement) => {
		return #to_string(#to_float(#round(movement * 100.0)) / 100.0);
	},

	set_image: (object) => {
		if (object == null) {
			if (this.last_object_class != null) {
				this.preview.remove();
				this.preview = null;
			}
			this.last_object_class = null;
			return;
		}
		const cls = #classof(object);
		if (this.last_object_class != cls) {
			if (this.last_object_class != null) {
				this.preview.remove();
				this.preview = null;
			}
			this.last_object_class = cls;
		}
		let type = '';
		let data = {};
		switch (cls) {
			case 'Unit': {
				type = 'unit-preview';
				data.unit = object;
				break;
			}
			case 'Base': {
				type = 'base-preview';
				data.base = object;
				break;
			}
			default: {
				throw Error('Unknown object class: ' + cls);
			}
		}
		if (this.preview == null) {
			this.preview = this.frame.widget({
				class: 'bottombar-object-preview',
				type: type,
				data: data,
				height: cls == 'Unit' ? 176 : 68,
			});
		} else {
			this.preview.data = data;
		}
	},

	set_lines: (object) => {

		if (#is_defined(this.lines)) {
			this.lines.remove(); // TODO: fix this.lines.clear();
		}

		if (object == null) {
			this.lines = #undefined;
			return;
		}

		this.lines = this.frame.area({left: 0, right: 0, top: 0, bottom: 0});
		let row = 0;

		const f_line = (text, size, align) => {
			const line = this.lines.text({
				align: 'top ' + align,
				color: 'rgb(116,156,56)',
				font: (size == 16 ? 'arialbd.ttf' : 'arial.ttf') + ':12:mono',
				text: text,
				left: align == 'left' ? 0 : #undefined,
			});
			// Native baseline 139, then 13-pixel rows; clip origin is y=66.
			line.top = 73 + row * 13 - line.measure().height;
			row++;
			return line;
		};

		switch (#classof(object)) {
			case 'Unit': {
				const def = object.get_def();

				f_line(def.name, 16, 'center');

				const tuple = this.p.get_stats_str(object);
				// Retain the legacy artillery offense decoration without nesting it.
				const legacy_artillery = !object.is_conventional && !object.is_native && def.id == 'SporeLauncher';
				f_line(legacy_artillery ? tuple : '(' + tuple + ')', 14, 'center');

				if (object.is_conventional) {
					const state = object.get_conventional_state();
					const stats = object.query_conventional_stats(state.render_morale_mode);
					f_line(stats.morale_name, 14, 'left');
					let home = 'Independent';
					if (state.home == 'base') {
						home = 'Home unavailable';
						for (base of this.p.game.get_bm().get_bases()) {
							if (base.id == state.home_base_id) { home = base.name; }
						}
					}
					f_line(home, 14, 'left');
					const remaining = stats.remaining_allowance > 999 ? 999 : stats.remaining_allowance;
					if (remaining > 0) {
						f_line('Moves: ' + this.p.format_points(remaining, stats.quantum), 14, 'left');
					} else {
						const exhausted = f_line('ALREADY MOVED', 14, 'left');
						exhausted.color = 'rgb(255,0,0)';
					}
					if (state.order == 2) { f_line('Hold', 14, 'left'); }
				} else if (object.is_native) {
					const stats = object.query_native_stats(0);
					f_line(stats.morale_name, 14, 'left');
					f_line('', 14, 'left');
					const native_remaining = stats.remaining_allowance > 999 ? 999 : stats.remaining_allowance;
					if (native_remaining > 0) {
						f_line('Moves: ' + this.p.format_points(native_remaining, stats.quantum), 14, 'left');
					} else {
						const native_exhausted = f_line('ALREADY MOVED', 14, 'left');
						native_exhausted.color = 'rgb(255,0,0)';
					}
				} else {
					f_line(this.get_morale('NATIVE', object.morale), 14, 'left');
					f_line('', 14, 'left');
					if (!object.is_immovable) {
						f_line('Moves: ' + this.format_movement(object.movement), 14, 'left');
					}
				}

				break;
			}
			case 'Base': {
				f_line(object.name, 16, 'center');
				const production = object.get_production_state();
				const presentation = object.get_smac_production_presentation();
				let description = 'Production unavailable';
				if (#is_defined(presentation)) {
					description = 'Producing: ' + presentation.name;
					if (#is_defined(production)) {
						description += ' (' + #to_string(production.minerals);
						if (#is_defined(presentation.cost)) { description += '/' + #to_string(presentation.cost); }
						description += ')';
					}
				}
				// Measure the actual font rather than splitting at a guessed
				// character count: production names vary considerably in width.
				let line = f_line('', 14, 'center');
				let text = '';
				for (word of #split(description, ' ')) {
					const candidate = text == '' ? word : text + ' ' + word;
					line.text = candidate;
					if (text != '' && line.measure().width > this.frame.width - 12) {
						line.text = text;
						line = f_line(word, 14, 'center');
						text = word;
					} else { text = candidate; }
				}

				break;
			}
			default: {
				throw Error('Unknown object class: ' + #classof(object));
			}
		}
	},

	show: (object) => {
		this.object = object;
		this.object_class = object == null ? null : #classof(object);
		this.object_id = object == null ? null : object.id;
		this.set_image(object);
		this.set_lines(object);
	},

	init: (p) => {

		this.preview = null;
		this.object = null;
		this.object_class = null;
		this.object_id = null;
		this.last_object_class = null;
		this.moralesets = {};

		this.p = p;
		this.um = p.game.get_um();

		p.ui.class('bottombar-object-preview').set({
			align: 'top left',
			top: 0,
			left: 0,
			width: 116,
			height: 68,
		});

		const frame_outer = p.el.panel({
			class: 'bottombar-panel',
			align: 'top left',
			top: 60,
			bottom: 9,
			left: 8,
			width: 128,
		});
		this.frame = frame_outer.panel({
			class: 'bottombar-panel-inner',
			left: 6, right: 6, top: 6, bottom: 6,
		});

		p.map.on('unit_preview', (e) => {
			this.show(e.unit);
		});
		p.map.on('base_preview', (e) => {
			this.show(e.base);
		});
		this.frame.listen(p.game, 'update_base', (e) => {
			if (this.object_class == 'Base' && this.object_id == e.base.id) {
				this.show(e.base);
			}
		});
		this.frame.listen(p.game, 'base_despawn', (e) => {
			if (this.object_class == 'Base' && this.object_id == e.base_id) { this.show(null); }
		});

	},

};
