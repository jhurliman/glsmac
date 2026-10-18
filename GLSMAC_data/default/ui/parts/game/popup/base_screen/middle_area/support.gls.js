const support_lines = #include('../support_lines');
return {

	init: (p) => {

		this.el = p.area.panel({
			class: 'base-screen-middle-area',
		});

		this.items = this.el.listview({
			left: 4, right: 4, top: 4, bottom: 4, itemsize: 20,
			has_hscroll: true, has_vscroll: true,
			hscroll_class: 'default-scroll-h', vscroll_class: 'default-scroll-v',
		});
		
	},

	set: (presentation) => {
		this.items.clear();
		for (line of support_lines(presentation, true)) {
			this.items.text({class: 'base-screen-frame-text', text: line});
		}
	},
};
