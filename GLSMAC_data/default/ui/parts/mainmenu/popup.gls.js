return (m) => {
	let current_menublock = null;
	let current_preview = null;

	let active_entry_index = null;

	const remove_active_menublock = () => {
		if (current_preview != null) { current_preview.remove(); current_preview = null; }
		if (current_menublock != null) {
			current_menublock.remove();
			current_menublock = null;
			active_entry_index = null;
		}
	};

	return {

		manager: #include('manager')('popup', this),

		back: () => {
			remove_active_menublock();
			this.manager.pop(m, true);
		},

		select: (index) => {
			if (index != active_entry_index) {
				if (active_entry_index != null) {
					((current_entries[active_entry_index]).obj).removeclass('menublock-item-active');
				}
				active_entry_index = index;
				((current_entries[active_entry_index]).obj).addclass('menublock-item-active');
			}
		},

		show: (data) => {

			this.manager.push('popup', data);

			remove_active_menublock();

			let window_class = 'window';
			if (!#is_defined(data.title)) {
				window_class += '-noheader';
			}

			let original = null;
            if (#is_defined(data.smac_kind)) { original = #include('../smac_dialog')(m.root, data.smac_kind, data.title); }
			const placement = #is_defined(data.placement) ? data.placement : {};
			if (original == null) { current_menublock = m.root.window({
				class: window_class,
				title: data.title,
				width: data.width,
				height: data.height,
				blocker: 'transparent',
			} + placement); } else { current_menublock = original.window; }

			let body_class = 'popup-body popup-body-noheader';
			if (!#is_defined(data.buttons)) {
				body_class += ' popup-body-nofooter';
			}
			const body = original == null ? current_menublock.panel({class: body_class}) : original.body;

			if (#is_defined(data.generator)) {
				data.generator(body);
			}
			if (#is_defined(data.preview)) { current_preview = data.preview(m.root); }

			if (#is_defined(data.buttons)) {
				const footer = original == null ? current_menublock.panel({class: 'popup-footer'}) : original.footer;
				for (button of data.buttons) {
                    let style = button.style + {class: 'popup-button'};
                    if (original != null) {
                        let slot = 0;
                        if (style.text == 'OK') { slot = 1; }
                        if (style.text == 'Cancel') { slot = 2; }
                        style = style + {top: 0, height: 20};
                        style.align = 'top left'; style.left = slot * original.button_stride;
                        style.width = original.button_width;
                    }
					const btn = footer.button(style);
					btn.on('click', button.onclick);
				}
			}

			current_menublock.on('keydown', (e) => {
				if (!#is_empty(e.modifiers)) {
					return false;
				}
				if (e.code == 'ESCAPE') {
					if (#is_defined(data.on_cancel)) { return data.on_cancel(); }
					this.back();
					return true;
				}
				return false;
			});

			return {};

		},

		error: (text, cb) => {
			this.show({
				title: 'ERROR',
				width: 400,
				height: 120,
				generator: (body) => {
					body.text({
						class: 'popup-text',
						align: 'center',
						text: text,
					});
				},
				buttons: [
					{
						style: {
							text: 'OK',
							align: 'center',
							is_ok: true,
							is_cancel: true,
						},
						onclick: (e) => {
							if (#is_defined(cb)) {
								cb();
							} else if (
								#is_defined(m.glsmac.config.quickstart) ||
								#is_defined(m.glsmac.config.host) ||
								#is_defined(m.glsmac.config.join)
							) {
								m.glsmac.exit();
							} else {
								parent.parent.back();
							}
							return true;
						},
					},
				],
			});
		},

		dismiss: () => {
			remove_active_menublock();
			this.manager.discard();
		},

		hide: () => {
			remove_active_menublock();
			this.manager.close(m);
		},

	};


};
