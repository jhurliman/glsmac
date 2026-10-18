return {

	init: (p) => {
		this.p = p;

		this.frame = p.body.panel({
			class: 'default-panel',
			align: 'top left',
			top: 59,
			bottom: 7,
			left: 137,
			width: 106,
		});

		this.frame.text({
			class: 'base-screen-frame-text', font: 'arialnb.ttf:11',
			text: 'QUEUE (READ ONLY)',
			align: 'top',
			top: 2,
			left: 2,
			right: 2,
		});

		this.items = this.frame.listview({
			class: 'default-panel-inner',
			top: 24,
			bottom: 45,
			left: 3,
			right: 3,
			itemsize: 14,
			padding: 5,
			has_hscroll: false,
			has_vscroll: true,
			vscroll_class: 'default-scroll-v',
		});

		this.frame.button({
			class: 'game-popup-button',
			text: 'CHANGE',
			align: 'bottom',
			bottom: 23,
			left: 2,
			right: 2,
		}).on('click', () => { this.p.utils.show_production_selection(); return true; });
		this.frame.text({
			class: 'base-screen-frame-text', font: 'arialnb.ttf:11',
			text: 'Hurry unavailable',
			align: 'bottom',
			bottom: 2,
			left: 2,
			right: 2,
		});

	},

	set_stored: (presentation) => {
		if (!#is_defined(presentation) || !#is_defined(presentation.queue)) { this.set_unavailable(); return; }
		this.items.clear();
		if (#sizeof(presentation.queue) == 0) {
			this.items.text({ class: 'base-screen-frame-text', text: 'Empty queue' });
		}
		for (name of presentation.queue) {
			this.items.text({ class: 'base-screen-frame-text', text: name });
		}
	},

	set_unavailable: () => {
		this.items.clear();
		this.items.text({ class: 'base-screen-frame-text', text: 'Unavailable' });
	},

	set: (data) => {

		this.items.clear();

		let i = 0;
		for (item of data) {
			this.items.text({
				class: 'base-screen-frame-text',
				text: item,
			});
			i++;
			if (i == 8) {
				break;
			}
		}
		while (i < 8) {
			this.items.text({
				class: 'base-screen-frame-text',
				text: 'Empty Slot',
			});
			i++;
		}
		
	},

};
