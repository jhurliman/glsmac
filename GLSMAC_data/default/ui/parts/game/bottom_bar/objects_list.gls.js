return {

	set_active_item: (item) => {
		if (this.active_item == item) {
			return;
		}
		if (this.active_item != null) {
			this.active_item.active = false;
		}

		this.active_item = item;

		if (this.active_item != null) {
			this.active_item.active = true;
		}
	},

	add_object: (object, left) => {
		const cls = #classof(object);
		let type = null;
		let data = {};
		let top = 0;
		switch (cls) {
			case 'Unit': {
				type = 'unit-preview';
				data.unit = object;
                data.native_console = #is_defined(this.native_console) && this.native_console;
				data.roster = true;
				top = 0;
				break;
			}
			case 'Base': {
				type = 'base-preview';
				data.base = object;
                data.card = true;
				data.roster = true;
				top = 0;
				break;
			}
			default: {
				throw Error('Unknown object type: ' + #classof(object));
			}
		}

		let item = this.list.panel({
			class: 'bottombar-objects-list-item',
			id: 'item_' + cls,
			left: left,
		});
        const height = (cls == 'Base' || (#is_defined(this.native_console) && this.native_console)) ? 48 : 38;
		item.widget({
			type: type,
			data: data,
			class: 'bottombar-objects-list-item-preview',
			top: top,
			height: height,
		});
		const key = #to_string(object.id);
		switch (cls) {
			case 'Unit': {
				this.units[key] = {
					item: item,
					object: object
				};
				this.units[key].stats_label = item.text({
					class: 'bottombar-objects-list-item-text',
					text: parent.p.get_stats_str(object),
				});
				item.on('mousedown', (e) => {
					this.p.modules.popup.hide('base_screen');
					this.set_active_item(null);
					this.p.game.select_unit(object);
					return true;
				});
				break;
			}
			case 'Base': {
				this.bases[key] = {
					item: item,
					object: object
				};
				item.on('mousedown', (e) => {
					if (this.selected_object != object) {
						this.set_active_item(null);
						this.p.game.select_base(object);
					}
					return true;
				});
				break;
			}
		}
		item.on('mouseover', (e) => {
			this.p.sections.object_preview.show(object);
			return true;
		});
		item.on('mouseout', (e) => {
			this.p.sections.object_preview.show(this.selected_object);
			return true;
		});
		if (object == this.selected_object) {
			this.set_active_item(item);
		}
	},

	update_tile: (tile) => {

		this.set_active_item(null);

		if (#is_defined(this.list)) {
			this.list.remove();
			this.set_active_item(null);
			this.units = {};
			this.bases = {};
		}
		this.list = this.frame.scrollview({
			left: 0,
			right: 0,
			top: 0,
			bottom: -8,
			padding: 0,
			has_hscroll: true,
			has_vscroll: false,
		});

		this.list_width = 0;

		const base = tile.get_base();

		if (base != null) {
			this.add_object(base, this.list_width);
			this.list_width = this.list_width + this.object_width;
		}

		for (unit of tile.get_units()) {
			this.add_object(unit, this.list_width);
			this.list_width = this.list_width + this.object_width;
		}

		this.selected_tile = tile;
	},

	init: (p) => {

		this.selected_tile = null;
		this.selected_object = null;
		this.units = {};
		this.bases = {};
		this.active_item = null;
		this.is_turn_active = true;

		this.p = p;
        const native_console = #is_defined(p.native_console) && p.native_console; this.native_console = native_console;

		this.object_width = native_console ? 66 : 65;

		p.ui.class('bottombar-objects-list-item').set({
			width: 65,
			height: 48,
			_hover: {
				border: 'rgb(14,40,49),1',
			},
			_active: {
				border: 'rgb(28,80,99),1',
			},
		});

		p.ui.class('bottombar-objects-list-item-preview').set({
			width: 65,
			height: 38,
		});

		p.ui.class('bottombar-objects-list-item-text').set({
			font: 'arial.ttf:12:mono',
			color: 'rgb(235,235,235)',
			align: 'bottom center',
			bottom: 2,
		});

		const frame_outer = p.el.panel((native_console ? {} : {class: 'bottombar-panel'}) + {
			align: 'bottom',
			bottom: 9,
			left: 139,
			right: 260,
			height: 56,
		});
        if (native_console) { frame_outer.surface({left: 0, right: 0, top: 0, bottom: 0,
          smac_frame_phase: [139,192], smac_frame: [[21,0,0,625,56],[18,3,3,622,53]]}); }
		this.frame = frame_outer.panel((native_console ? {} : {class: 'bottombar-panel-inner'}) + {
			left: 4, right: 4, top: 4, bottom: 4,
		});

		this.frame.listen(p.map, 'unit_preview', (e) => {
			if (e.unit != null) {
				const tile = e.unit.get_tile();
				if (tile != this.selected_tile) {
					this.update_tile(tile);
				}
				const key = #to_string(e.unit.id);
				if (!#is_defined(this.units[key])) {
					this.add_object(e.unit, this.list_width);
					this.list_width = this.list_width + this.object_width;
				} else {
					this.units[key].stats_label.text = this.p.get_stats_str(e.unit);
				}
			}
		});

		this.frame.listen(p.map, 'base_preview', (e) => {
			if (e.base == null || !#is_defined(e.base)) { return; }
			const tile = e.base.get_tile();
			if (tile != this.selected_tile) {
				this.update_tile(tile);
			}
		});

		this.frame.listen(p.map, 'tile_preview', (e) => {
			if (e.tile != this.selected_tile) {
				this.update_tile(e.tile);
			}
		});

		this.frame.listen(p.game, 'unit_select', (e) => {
			this.p.modules.popup.hide('base_screen');
			const key = #to_string(e.unit.id);
			this.set_active_item(this.units[key].item);
			this.selected_object = e.unit;
		});

		this.frame.listen(p.game, 'base_select', (e) => {
			this.set_active_item(null);
			#async(0, () => { // workaround for objects list active border getting messed up, TODO: investigate and fix properly
				if (!#is_defined(e.base)) { return; } // removed before the deferred open
				const key = #to_string(e.base.id);
				this.set_active_item(this.bases[key].item);
				const last_selected_object = this.selected_object;
				this.selected_object = e.base;
				this.p.modules.popup.set('base_screen', {
					base: e.base,
				});
				this.p.modules.popup.show('base_screen', (result) => {
					this.selected_object = null;
					if (#is_defined(last_selected_object) && last_selected_object != null) {
						switch (#classof(last_selected_object)) {
							case 'Unit': {
								this.set_active_item(null);
								this.p.game.select_unit(last_selected_object);
								break;
							}
						}
					} else {
						this.p.game.select_tile(this.selected_tile);
					}
					// Capture returns through Base Control before the unit controller
					// polls its already saved ending request.
					if (#is_defined(this.p.modules.outcome)) { this.p.modules.outcome.refresh(); }
				});
			});
		});

		this.frame.listen(p.game, 'base_despawn', (e) => {
			const key = #to_string(e.base_id);
			if (#is_defined(this.bases[key])) {
				if (this.selected_object == this.bases[key].object) { this.selected_object = null; }
				// Same-tile preview refresh normally retains the existing list.
				// Rebuild it now so Enter/click cannot select the retired wrapper.
				this.update_tile(this.selected_tile);
			}
		});

		this.frame.listen(p.game, 'unit_despawn', (e) => {
			const key = #to_string(e.unit_id);
			if (#is_defined(this.units[key])) {
				if (this.selected_object == this.units[key].object) { this.selected_object = null; }
				this.update_tile(this.selected_tile);
			}
		});

		this.frame.listen(p.game, 'tile_select', (e) => {
			if (this.selected_object != null) {
				this.selected_object = null;
				this.set_active_item(null);
			}
		});

		this.frame.listen(p.game, 'turn', (e) => {
			if (this.selected_object != null) {
				this.selected_object = null;
				this.set_active_item(null);
			}
		});

		this.frame.listen(p.game, 'turn_status', (e) => {
			this.is_turn_active = e.status == 'active';
		});

		this.frame.on('keydown', (e) => {
			if (
				!this.p.modules.popup.is_shown() && // TODO: make universal key overrides in popups
				this.is_turn_active && // TODO: override by turn complete button
				e.modifiers == {} &&
				e.code == 'ENTER'
			) {
				// open base screen if base is present
				for (base of this.bases) {
					this.set_active_item(null);
					this.p.game.select_base(base.object);
					return true;
				}
				if (this.selected_object == null) {
					// select first unit if nothing is selected
					for (unit of this.units) {
						this.set_active_item(null);
						this.p.game.select_unit(unit.object);
						return true;
					}
				}
				return true;
			}
			return false;
		});

	},

};
