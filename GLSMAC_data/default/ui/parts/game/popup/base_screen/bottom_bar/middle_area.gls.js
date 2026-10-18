return {

	init: (p) => {

		this.p = p;

		this.frame = p.body.panel({
			class: 'default-panel',
			border: 'transparent',
			left: 263,
			top: 69,
			right: 263,
			bottom: 86,
		});

		const header = this.frame.panel({
			class: 'default-panel',
			align: 'top',
			top: 2,
			left: 4,
			right: 4,
			height: 41,
		});

		const header_title = header.panel({
			class: 'default-panel-inner',
			top: 3,
			bottom: 3,
			left: 25,
			right: 25,
		});

		this.basename = header_title.text({
			align: 'top center', // TODO: fix vertical centering
			top: 0,
			font: 'arialnb.ttf:24',
			color: 'rgb(137,166,166)',
			transform: 'uppercase',
		});

		header_title.text({ class: 'base-screen-frame-text', font: 'arialnb.ttf:10',
			align: 'bottom center', bottom: 0, text: 'Base navigation unavailable' });

		p.ui.class('base-screen-bottombar-pop').set({
			width: 38,
			height: 48,
			align: 'left',
			top: 3,
			bottom: 3,
		});

		this.pops = this.frame.panel({
			class: 'default-panel',
			align: 'bottom',
			bottom: 2,
			left: 4,
			right: 60,
			height: 54,
		});

		const nerve = this.frame.panel({
			class: 'default-panel', width: 54, height: 54,
			align: 'bottom right', bottom: 2, right: 4,
		});
		nerve.text({ class: 'base-screen-frame-text', font: 'arialnb.ttf:10',
			align: 'top center', top: 12, text: 'Nerve staple' });
		nerve.text({ class: 'base-screen-frame-text', font: 'arialnb.ttf:10',
			align: 'top center', top: 25, text: 'unavailable' });

	},

	set: (data) => {

		this.basename.text = data.name;

		// prepare classes with population images
		const renders = this.p.game.get_bm().get_pop_renders(data.owner);
		for (id in renders) {
			const variants = renders[id];
			for (i in variants) {
				const variant = variants[i];
				this.p.ui.class('base-screen-bottombar-pop-' + id + '-' + #to_string(i)).extend('base-screen-bottombar-pop').set({
					background: variants[i],
				});
			}
		}

		this.pops.clear();
		let left = 3;
		let shift = 40;
		for (pop of data.pops) {
			let variant = pop.variant;
			const pop_type = pop.get_type();
			if (!#is_defined(renders[pop_type][variant])) {
				variant = 0;
			}
			this.pops.surface({
				class: 'base-screen-bottombar-pop-' + pop_type + '-' + #to_string(variant),
				left: left,
			});
			left += shift;
		}

	},

};
