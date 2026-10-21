return {

	hide_buttons: () => {
		for (button of this.buttons) {
			button.remove();
		}
		this.buttons = [];
	},

	show_default: () => {
		this.hide_buttons();

        let btn_rename = null;
        if (this.p.utils.can_rename()) {
            btn_rename = this.frame.button({class: 'base-screen-popup-bottom-button', align: 'left', left: 3, width: 210, text: 'RENAME'});
            btn_rename.on('click', () => { this.p.utils.open_rename(); return true; });
        } else { btn_rename = this.frame.text({class: 'base-screen-frame-text', top: 5, align: 'left', left: 3, text: 'Rename unavailable'}); }
		const btn_ok = this.frame.button({
			class: 'base-screen-popup-bottom-button',
			align: 'right',
			right: 3,
			width: 210,
			text: 'OK',
			is_ok: true,
			is_cancel: true,
		});

		btn_ok.on('click', (e) => {
			this.p.hide();
			return false;
		});

		this.buttons = [btn_rename, btn_ok];
	},

	show_production_selection: () => {
		this.hide_buttons();

		const btn_ok = this.frame.button({
			class: 'base-screen-popup-bottom-button',
			align: 'left',
			left: 215,
			width: 104,
			text: 'OK',
			is_ok: true,
		});

		const btn_cancel = this.frame.button({
			class: 'base-screen-popup-bottom-button',
			align: 'left',
			left: 321,
			width: 104,
			text: 'CANCEL',
			is_cancel: true,
		});

		this.buttons = [
			this.frame.text({
				class: 'base-screen-frame-text', font: 'arialnb.ttf:11',
				top: 7,
				align: 'left',
				left: 3,
				text: 'Help unavailable',
			}),
			this.frame.button({class: 'base-screen-popup-bottom-button', align: 'left', left: 109, width: 104, text: 'WORKSHOP'}),
			btn_ok,
			btn_cancel,
		];

        this.buttons[1].on('click', () => { this.p.utils.open_workshop(); return true; });

		btn_cancel.on('click', (e) => {
			this.p.utils.hide_production_selection();
			return true;
		});

		btn_ok.on('click', (e) => {
			this.p.utils.update_production_selection();
			return true;
		});

	},

	init: (p) => {

		this.p = p;
		this.buttons = [];

		this.frame = p.body.panel({
			class: 'base-screen-frame',
			align: 'bottom center',
			width: 428,
			height: 28,
			bottom: -32,
		});

		p.ui.class('base-screen-popup-bottom-button').extend('game-popup-button').set({
			top: 3,
			bottom: 3,
			width: 210, // TODO: why doesn't this work?
		});

		this.show_default();

		return this.frame;
	},

};
