return {

	TRENCH_LEVEL: -2000,
	OCEAN_LEVEL: -1000,
	SEA_LEVEL: 0,

	ROCKINESS_LEVELS: ['?', 'Flat', 'Rolling', 'Rocky'],
	MOISTURE_LEVELS: ['?', 'Arid', 'Moist', 'Rainy'],

	get_feature_name: (tile, feature) => {

		if (tile.is_water) {
			// water-only features
			switch (feature) {
				case 'xenofungus': {
					return 'Sea Fungus';
				}
				case 'geothermal': {
					return 'Geothermal';
				}
			}
		} else {
			// land-only features
			switch (feature) {
				case 'xenofungus': {
					return 'Xenofungus';
				}
				case 'river': {
					return 'River';
				}
				case 'jungle': {
					return 'Jungle';
				}
				case 'dunes': {
					return 'Dunes';
				}
				case 'uranium': {
					return 'Uranium';
				}
			}
		}

		// water-or-land features
		switch (feature) {
			case 'monolith': {
				return 'Monolith';
			}
		}

	},

	get_resource_name: (resource) => {
		switch (resource) {
			case 'nutrient': {
				return 'Nutrient bonus';
			}
			case 'energy': {
				return 'Energy bonus';
			}
			case 'minerals': {
				return 'Minerals bonus';
			}
		}
	},

	set_image: () => {
        if (!this.known) {
            if (#is_defined(this.preview)) {
                this.preview.remove();
                this.preview = #undefined;
            }
            return;
        }
		const tile = this.tile;
		if (!#is_defined(this.preview)) {
			this.preview = this.frame.widget({
				type: 'tile-preview',
				data: {
					tile: tile,
				},
				align: 'top left',
				left: 0,
				top: 0,
				width: 89,
				height: 117,
			});
		} else {
			this.preview.data = {
				tile: tile,
			};
		}
	},

	line: (text) => {
		const label = this.lines.text({
			class: 'tile-preview-line',
			text: text,
			left: 0,
		});
		label.top = 56 + this.row * 13 - label.measure().height;
		this.row = this.row + 1;
	},

	set_lines: () => {
		const tile = this.tile;
		if (#is_defined(this.lines)) {
			this.lines.remove(); // TODO: fix .clear()
		}
		this.lines = this.frame.area({left: 0, right: 0, top: 0, bottom: 0});
		this.row = 0;

        if (!this.known) {
            this.line('Unexplored');
        } else if (this.show_resources) {

			const resources = tile.get_resources();
			this.line('Nutrients: ' + #to_string(resources.NUTRIENTS));
			this.line('Minerals: ' + #to_string(resources.MINERALS));
			this.line('Energy: ' + #to_string(resources.ENERGY));

		} else {

			if (tile.is_water) {
				if (tile.elevation < this.TRENCH_LEVEL) {
					this.line('Ocean Trench');
				} else if (tile.elevation < this.OCEAN_LEVEL) {
					this.line('Ocean');
				} else {
					this.line('Ocean Shelf');
				}
				this.line('Depth: ' + #to_string(this.SEA_LEVEL - tile.elevation) + 'm');
			} else {
				this.line('Elev: ' + #to_string(tile.elevation - this.SEA_LEVEL) + 'm');
				let tilestr = '';
				if (tile.rockiness < #sizeof(this.ROCKINESS_LEVELS)) {
					tilestr += this.ROCKINESS_LEVELS[tile.rockiness];
				}
				tilestr += ' & ';
				if (tile.moisture < #sizeof(this.MOISTURE_LEVELS)) {
					tilestr += this.MOISTURE_LEVELS[tile.moisture];
				}
				this.line(tilestr);
			}

			for (f in tile.features) {
				if (tile.features[f]) {
					this.line(this.get_feature_name(tile, f));
				}
			}

			for (f in tile.bonuses) {
				if (tile.bonuses[f]) {
					this.line(this.get_resource_name(f));
				}
			}

			// TODO: terraforming

		}


		const txt = '(' + #to_string(tile.x) + ' , ' + #to_string(tile.y) + ')';
		if (!#is_defined(this.bottom_line)) {
			this.bottom_line = this.frame.text({
				class: 'tile-preview-line',
				align: 'bottom right',
				bottom: 4,
				right: 0,
				text: txt,
			});
		} else {
			this.bottom_line.text = txt;
		}
		this.bottom_line.align = 'top right';
		this.bottom_line.bottom = #undefined;
		this.bottom_line.top = 113 - this.bottom_line.measure().height;
	},

	init: (p) => {

		this.show_resources = false;
        this.known = true;

		p.ui.class('tile-preview-line').set({
			color: 'rgb(116,156,56)',
			font: 'arial.ttf:12:mono',
		});

		const frame_outer = p.el.panel({
			class: 'bottombar-panel',
			align: 'top left',
			top: 60,
			bottom: 68,
			left: 140,
			width: 101,
		});
		this.frame = frame_outer.panel({
			class: 'bottombar-panel-inner',
			left: 6, right: 6, top: 6, bottom: 6,
		});

		p.map.on('tile_preview', (e) => {
			this.tile = e.tile;
            this.known = #is_defined(e.known) ? e.known : true;
			this.set_image();
			this.set_lines();
		});

		frame_outer.on('mousedown', (e) => {
			if (!#is_defined(this.tile)) { return true; }
			this.show_resources = !this.show_resources;
			this.set_lines();
			return true;
		});

	},

};
