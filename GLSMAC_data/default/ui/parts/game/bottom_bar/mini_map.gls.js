return {

	init: (p) => {

		p.ui.class('bottombar-minimap-info-text').set({
			font: 'arialn.ttf:14',
			color: 'rgb(60, 148, 124)',
		});

		p.ui.class('bottombar-minimap-button').set({
			align: 'left',
			width: 54,
			height: 32,
			top: 3,
		});

		this.frame = p.el.panel({
			align: 'bottom right',
			top: 59,
			bottom: 6,
			right: 7,
			width: 236,
		});

		// Original 1024x768 rectangles, anchored to the right console artwork.
		const minimap = p.el.widget({
			type: 'minimap', align: 'top right',
			right: 5, top: 88, width: 239, height: 119,
			data: {},
		});
		let had_overview = false;
		const refresh_overview = () => {
			const frame = p.game.get_smac_minimap();
			if (#is_defined(frame)) {
				minimap.data = {overview: frame};
				had_overview = true;
			} else if (had_overview) {
				minimap.data = {overview: {}};
			}
		};
		refresh_overview();
		// A refused move can reveal sight without moving or refreshing a preview.
		minimap.listen(p.game, 'map_knowledge', (e) => { refresh_overview(); });
		minimap.listen(p.map, 'unit_preview', (e) => { refresh_overview(); });
		minimap.listen(p.map, 'base_preview', (e) => { refresh_overview(); });

		const info_block = p.el.panel({
			class: 'bottombar-panel-inner', align: 'top right',
			right: 123, top: 214, width: 116, height: 32,
		});
		const info_year = info_block.text({
			class: 'bottombar-minimap-info-text',
			align: 'top left',
			top: 3,
			left: 3,
			text: 'Mission Year ' + #to_string(p.game.get_year()),
		});
		const info_energy = info_block.text({
			class: 'bottombar-minimap-info-text',
			align: 'bottom left',
			bottom: 4,
			left: 3,
			text: 'Energy: N/A',
		});

		const refresh_energy = () => {
			const credits = p.game.get_smac_credits();
			info_energy.text = #is_defined(credits) ? 'Energy: ' + #to_string(credits) : 'Energy: N/A';
		};
		// Initial construction covers restored sessions, which do not emit a
		// gameplay turn just to open the UI. Later reads follow actual state events.
		refresh_energy();
		minimap.listen(p.game, 'turn', (e) => {
			info_year.text = 'Mission Year ' + #to_string(e.year);
			refresh_energy();
			refresh_overview();
		});

		// Verified World artwork. A separate native Detail renderer is the next slice.

		// TODO: make _hover and _active work without class
		p.ui.class('bottombar-minimap-button-left').extend('bottombar-minimap-button').set({
			background: 'interface.pcx:crop(532, 143, 585, 174)',
			_hover: {
				background: 'interface.pcx:crop(532, 176, 585, 207)',
			},
			_active: {
				background: 'interface.pcx:crop(532, 209, 585, 240)',
			},
		});
		p.el.button({
			class: 'bottombar-minimap-button-left',
			text: '',
			align: 'top right', right: 66, top: 214,
		}).on('click', (e) => {
			refresh_overview();
			return true;
		});

		p.ui.class('bottombar-minimap-button-right').extend('bottombar-minimap-button').set({
			background: 'interface.pcx:crop(587, 143, 640, 174)',
			_hover: {
				background: 'interface.pcx:crop(587, 176, 640, 207)',
			},
			_active: {
				background: 'interface.pcx:crop(587, 209, 640, 240)',
			},
		});
		// Keep the source Detail artwork above, but do not expose the former
		// debug action that merely blanked the map. This is not a completed
		// two-mode console; Detail requires its own measured zoom-7 view.

        // Original233x23 turn.flc control at console-local783,61.
        const btn_turn_complete = this.frame.button({left: 2, top: 2, width: 233, height: 23});
        const artwork = btn_turn_complete.surface({left: 0, top: 0, width: 233, height: 23});
        const label = btn_turn_complete.text({
            align: 'top left', font: 'arialbd.ttf:14:mono', color: 'rgb(228,104,24)', text: 'TURN COMPLETE',
        });
        const set_label = (text) => {
            label.text = text;
            const size = label.measure();
            label.left = (233 - size.width) / 2;
            label.top = 17 - size.height;
        };
        const advance = (#include('turn_complete_phase'))();
        let turn_status = p.game.get_turn_status();
        const redraw = () => { artwork.smacturnframe(advance(turn_status == 'turn_complete')); };
        let is_mouseover = false;
        btn_turn_complete.on('mouseover', (e) => {
            if (!is_mouseover) { is_mouseover = true; redraw(); }
            return false;
        });
        btn_turn_complete.on('mouseout', (e) => {
            if (is_mouseover) { is_mouseover = false; redraw(); }
            return false;
        });
        // Async repeats from the delivered callback's current time. A modal
        // prevents phase advancement. Original global/effect-hold admission is
        // not yet exposed by the port; this is not a claim of its full gate.
        artwork.timer(100, () => {
            if (!p.modules.popup.is_shown()) { redraw(); }
            return true;
        });

		const turn_complete_sound = btn_turn_complete.sound({
			sound: 'cpu turn complete.wav',
			volume: 0.5,
		});

        const f_set_status = (status) => {
            const previous = turn_status;
            turn_status = status;
            switch (status) {
                case 'please_wait': { set_label('PLEASE WAIT'); break; }
                case 'waiting_for_players': {
                    // Retain the port's functional status until actual native
                    // network masks/mode are available; do not infer them here.
                    set_label('WAITING FOR PLAYERS'); break;
                }
                default: { set_label('TURN COMPLETE'); }
            }
            redraw();
            if (previous != status && status == 'turn_complete') { turn_complete_sound.play(); }
        };
		minimap.listen(p.game, 'turn_status', (e) => {
			refresh_energy();
			f_set_status(e.status);
		});
		f_set_status(p.game.get_turn_status());

		btn_turn_complete.on('click', (e) => {
			switch (turn_status) {
				case 'active': {
					p.modules.popup.show('turn_confirmation', (result) => {
						if (result) {
							f_set_status('please_wait');
							p.game.event('complete_turn', {});
						}
					});
					break;
				}
				case 'turn_complete': {
					f_set_status('please_wait');
					p.game.event('complete_turn', {});
					break;
				}
				case 'waiting_for_players': {
					f_set_status('please_wait');
					p.game.event('uncomplete_turn', {});
					break;
				}
			}
			return true;
		});

	},

};
