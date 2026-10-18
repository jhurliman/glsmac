const support_lines = #include('../support_lines');
return {

	init: (p) => {

		this.frame = p.body.panel({
			class: 'default-panel',
			align: 'bottom right',
			top: 59,
			bottom: 6,
			right: 7,
			width: 236,
		});

		const header = this.frame.panel({
			class: 'base-screen-header',
		});
		header.text({
			class: 'base-screen-frame-title',
			text: 'FORCES SUPPORTED',
		});
		const body = this.frame.panel({
			class: 'base-screen-body',
		});
		this.items = body.listview({
			left: 3, right: 3, top: 3, bottom: 3, itemsize: 18,
			has_hscroll: true, has_vscroll: true,
			hscroll_class: 'default-scroll-h', vscroll_class: 'default-scroll-v',
		});

	},

	set: (presentation) => {
		this.items.clear();
		for (line of support_lines(presentation, false)) {
			this.items.text({class: 'base-screen-frame-text', text: line});
		}
	},
};
