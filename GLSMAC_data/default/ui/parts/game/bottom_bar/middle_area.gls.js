return {

	process_message: (text) => {
		this.pp.pages.messages.process_message(text);
	},

	init: (p) => {

		this.pp = {};
		const margin = 263;
		const page_inset = 3;
		const button_strip = 30;

		p.ui.class('bottombar-panel-page-button').extend('bottombar-panel-button').set({
			width: 21,
			height: 21,
			font: 'arialnb.ttf:17',
			color: 'black',
		});

		p.ui.class('bottombar-panel-page').set({
			left: page_inset,
			top: 3,
			bottom: 3,
			right: button_strip,
		});

		this.pp = {
			ui: p.ui,
			game: p.game,
			// Query live viewport geometry. A stored DOM wrapper's width may be
			// its construction snapshot, before the parent margins have settled.
			get_info_width: () => { return p.ui.get_width() - margin * 2 - page_inset - button_strip; },
			frame: p.el.panel({
				class: 'bottombar-panel',
				left: margin,
				top: 69,
				right: margin,
				bottom: 86,
			}),
			pages: {},
		};

		this.pp.pages_order = ['info_panels', 'messages'];
		for (s of this.pp.pages_order) {
			this.pp.pages[s] = #include('middle_area/' + s);
		}
		let first = true;
		for (p of this.pp.pages_order) {
			const s = this.pp.pages[p];
			s.init(this.pp);
			const page = s.page;
			if (first) {
				first = false;
			} else {
				page.hide();
			}
			s.button.on('on', (e) => {
				page.show();
				if (#is_defined(s.on_show)) {
					s.on_show();
				}
				return true;
			});
			s.button.on('off', (e) => {
				page.hide();
				return true;
			});
		}

	},

};
