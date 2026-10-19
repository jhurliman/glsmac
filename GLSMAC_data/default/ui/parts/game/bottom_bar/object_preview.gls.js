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

		this.lines = this.frame.listview({
			left: 3,
			right: 3,
			top: 65,
			bottom: 3,
			itemsize: 13,
		});

		const f_line = (text, size, align) => {
			let left = #undefined;
			if (align == 'left') {
				left = 3;
			}
			return this.lines.text({
				align: 'top ' + align,
				color: 'rgb(116,156,56)',
				font: 'arialnb.ttf:' + #to_string(size) + ':mono',
				text: text,
				left: left,
			});
		};

		switch (#classof(object)) {
			case 'Unit': {
				const def = object.get_def();

				f_line(def.name, 16, 'center');

				f_line(this.p.get_stats_str(object), 14, 'center');

				if (object.is_conventional) {
					const state = object.get_conventional_state();
					const stats = object.query_conventional_stats(state.render_morale_mode);
					f_line(stats.morale_name, 14, 'left');
					f_line('Moves: ' + this.format_movement(#to_float(stats.remaining_allowance) / #to_float(stats.quantum)), 14, 'left');
				} else if (object.is_native) {
					const stats = object.query_native_stats(0);
					f_line(stats.morale_name, 14, 'left');
					f_line('Moves: ' + this.format_movement(#to_float(stats.remaining_allowance) / #to_float(stats.quantum)), 14, 'left');
				} else {
					f_line(this.get_morale('NATIVE', object.morale), 14, 'left');
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
			align: 'top center',
			top: 12,
			width: 100,
			height: 56,
		});

		const frame_outer = p.el.panel({
			class: 'bottombar-panel',
			align: 'top left',
			top: 59,
			bottom: 7,
			left: 6,
			width: 129,
		});
		this.frame = frame_outer.panel({
			class: 'bottombar-panel-inner',
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
