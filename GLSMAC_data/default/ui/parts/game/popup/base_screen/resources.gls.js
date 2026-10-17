return {

	init: (p) => {

		this.format_resource_balance = p.ui.format_resource_balance;

		this.cell_width = 4;
		this.total_cells = 58; // original SMAC has 39 but leaves a lot of space for no reason(?)

		this.frame = p.body.panel({
			class: 'base-screen-side-middle-frame',
			align: 'top right',
			top: 259,
			right: 3,
			height: 89,
		});

		const cw = this.cell_width;

		p.ui.class('base-screen-resources-cell').set({
			width: cw,
			height: 9,
			background: 'rgb(53,61,115)',
		});
		p.ui.class('base-screen-resources-cell-nutrients').extend('base-screen-resources-cell').set({
			align: 'left top',
		});
		p.ui.class('base-screen-resources-cell-minerals').extend('base-screen-resources-cell').set({
			align: 'left center',
		});
		p.ui.class('base-screen-resources-cell-energy').extend('base-screen-resources-cell').set({
			align: 'left bottom',
		});

		p.ui.class('base-screen-resources-cell-nutrients-loss').extend('base-screen-resources-cell-nutrients').set({
			background: 'interface.pcx:crop(397,22,417,41)',
		});
		p.ui.class('base-screen-resources-cell-nutrients-profit').extend('base-screen-resources-cell-nutrients').set({
			background: 'interface.pcx:crop(418,22,438,41)',
		});
		p.ui.class('base-screen-resources-cell-minerals-loss').extend('base-screen-resources-cell-minerals').set({
			background: 'interface.pcx:crop(397,43,417,62)',
		});
		p.ui.class('base-screen-resources-cell-minerals-profit').extend('base-screen-resources-cell-minerals').set({
			background: 'interface.pcx:crop(418,43,438,62)',
		});
		p.ui.class('base-screen-resources-cell-energy-loss').extend('base-screen-resources-cell-energy').set({
			background: 'interface.pcx:crop(397,1,417,20)',
		});
		p.ui.class('base-screen-resources-cell-energy-profit').extend('base-screen-resources-cell-energy').set({
			background: 'interface.pcx:crop(418,1,438,20)',
		});

		p.ui.class('base-screen-resources-label-left').extend('base-screen-side-header-text').set({
			left: 6,
		});
		p.ui.class('base-screen-resources-label-right').extend('base-screen-side-header-text').set({
			left: 359,
		});

		const c_nutrients = 'rgb(116,156,56)';
		const c_minerals = 'rgb(144,184,228)';
		const c_energy = 'rgb(224,156,28)';

		this.frame.panel({
			class: 'base-screen-side-header',
			top: 3,
		})
			.text({
				class: 'base-screen-side-header-text',
				color: c_nutrients,
				text: 'NUTRIENTS',
			})
		;

		this.frame.panel({
			class: 'base-screen-side-header',
			top: 24,
		})
			.text({
				class: 'base-screen-side-header-text',
				color: c_minerals,
				text: 'MINERALS',
			})
		;

		this.frame.panel({
			class: 'base-screen-side-header',
			top: 45,
		})
			.text({
				class: 'base-screen-side-header-text',
				color: c_energy,
				text: 'ENERGY',
			})
		;

		const body = this.frame.panel({
			class: 'base-screen-body',
			left: 132,
			top: 3,
			right: 3,
			bottom: 24,
		});
		this.cells = body.area({
			left: 66,
			right: 90,
			top: 6,
			bottom: 5,
		});
		this.labels = {
			nutrients: {
				left: body.text({
					class: 'base-screen-resources-label-left',
					align: 'top left',
					top: 4,
					color: c_nutrients,
				}),
				right: body.text({
					class: 'base-screen-resources-label-right',
					align: 'top left',
					top: 4,
					color: c_nutrients,
				}),
			},
			minerals: {
				left: body.text({
					class: 'base-screen-resources-label-left',
					align: 'center left',
					color: c_minerals,
				}),
				right: body.text({
					class: 'base-screen-resources-label-right',
					align: 'center left',
					color: c_minerals,
				}),
			},
			energy: {
				left: body.text({
					class: 'base-screen-resources-label-left',
					align: 'bottom left',
					bottom: 4,
					color: c_energy,
				}),
				right: body.text({
					class: 'base-screen-resources-label-right',
					align: 'bottom left',
					bottom: 4,
					color: c_energy,
				}),
			},
		};

		const info = this.frame.panel({
			class: 'base-screen-body',
			align: 'bottom',
			left: 3,
			right: 3,
			bottom: 3,
			height: 20,
		});
		this.intake_caption = info.area({
			align: 'left',
			width: 442,
		})
			.text({
				class: 'base-screen-frame-info-text',
				text: 'INTAKE - CONSUMPTION',
				align: 'center',
			})
		;
		info.area({
			align: 'right',
			width: 80,
		})
			.text({
				class: 'base-screen-frame-info-text',
				text: '= SURPLUS',
				align: 'left center',
			})
		;

		return this.frame;
	},

	set: (data) => {
		const types = ['nutrients', 'minerals', 'energy'];

		this.cells.clear();
		this.intake_caption.text = #is_defined(data.minerals.loss) && #is_defined(data.energy.loss)
			? 'INTAKE - CONSUMPTION' : 'INTAKE - CONSUMPTION (N/A = unavailable)';

		const w = this.cell_width + 1;

		for (type of types) {
			const d = data[type];
			let balance = undefined;
			if (#is_defined(d.loss)) {
				balance = this.format_resource_balance(d.profit, d.loss, this.total_cells);
			} else {
				// Unavailable costs cannot imply a surplus or a filled balance bar.
				balance = { loss_cells: 0, profit_cells: 0,
					intake_consumption: #to_string(d.profit) + ' - N/A', surplus: 'N/A' };
			}

			// cells
			let left = 0;
			for (let i = 0; i < this.total_cells; i++) {
				let cls = 'base-screen-resources-cell-' + type;
				if (i < balance.loss_cells) {
					cls += '-loss';
				} else if (i >= this.total_cells - balance.profit_cells) {
					cls += '-profit';
				}
				this.cells.surface({
					class: cls,
					left: left,
				});
				left += w;
			}

			// labels
			const l = this.labels[type];
			l.left.text = balance.intake_consumption;
			l.right.text = balance.surplus;
		}
	},

};
