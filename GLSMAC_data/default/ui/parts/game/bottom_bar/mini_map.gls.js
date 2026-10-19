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

		// Source console coordinates at the fixed 1024x768 logical stage.
		const minimap = p.el.widget({
			type: 'minimap', align: 'top left',
			left: 780, top: 88, width: 239, height: 119,
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
			class: 'bottombar-panel-inner', align: 'top left',
			left: 785, top: 214, width: 116, height: 32,
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
			left: 904, top: 214,
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

		// 'turn complete' button

		const btn_turn_complete = this.frame.button({
			left: 2,
			top: 2,
			width: 233,
			height: 23,
			border: 'rgb(228, 104, 24),2',
			color: 'rgb(228, 104, 24),2',
			font: 'arialnb.ttf:17',
			text: 'TURN COMPLETE',
		});

		const blink_rate = 25;
		const blink_speed = 0.025;

		const blinker = btn_turn_complete.surface({
			left: 4,
			top: 4,
			right: 4,
			bottom: 4,
			background: 'console_x2_a.pcx:crop(198, 113, 375, 129)',
			opacity: 0.0,
		});
		let blink_timer = null;
		let blink_forward = true;
		let blink_opacity = 0.0;
		let is_mouseover = false;
		let allow_turn_complete = true;
		// TODO: use _hover and _active
		btn_turn_complete.on('mouseover', (e) => {
			if (allow_turn_complete && !is_mouseover) {
				is_mouseover = true;
				blinker.opacity = 0.5;
			}
			return false;
		});
		btn_turn_complete.on('mouseout', (e) => {
			if (is_mouseover) {
				is_mouseover = false;
				blinker.opacity = blink_opacity;
			}
			return false;
		});
		btn_turn_complete.on('mousedown', (e) => {
			if (allow_turn_complete && is_mouseover) {
				blinker.opacity = 0.8;
			}
			return false;
		});
		btn_turn_complete.on('mouseup', (e) => {
			if (allow_turn_complete && is_mouseover) {
				blinker.opacity = 0.5;
			} else {
				blinker.opacity = blink_opacity;
			}
			return false;
		});

		const f_blink_stop = () => {
			if (blink_timer != null) {
				blink_timer.stop();
				blink_timer = null;
				blink_opacity = 0.0;
				blinker.opacity = 0.0;
			}
		};
		const f_blink_start = () => {
			f_blink_stop();
			blink_timer = blinker.timer(blink_rate, () => {
				if (blink_forward) {
					blink_opacity += blink_speed;
					if (blink_opacity >= 1.0) {
						blink_opacity = 1.0;
						blink_forward = false;
					}
				} else {
					blink_opacity -= blink_speed;
					if (blink_opacity <= 0.25) {
						blink_opacity = 0.25;
						blink_forward = true;
					}
				}
				if (!is_mouseover) {
					blinker.opacity = blink_opacity;
				}
				return true;
			});
		};

		const turn_complete_sound = btn_turn_complete.sound({
			sound: 'cpu turn complete.wav',
			volume: 0.5,
		});

		let turn_status = 'active';
		const f_set_status = (status) => {
			if (turn_status != status) {
				turn_status = status;
				switch (turn_status) {
					case 'please_wait': {
						f_blink_stop();
						btn_turn_complete.text = 'PLEASE WAIT';
						allow_turn_complete = false;
						break;
					}
					case 'active': {
						f_blink_stop();
						btn_turn_complete.text = 'TURN COMPLETE';
						allow_turn_complete = true;
						break;
					}
					case 'turn_complete': {
						btn_turn_complete.text = 'TURN COMPLETE';
						allow_turn_complete = true;
						f_blink_start();
						turn_complete_sound.play();
						break;
					}
					case 'waiting_for_players': {
						f_blink_stop();
						btn_turn_complete.text = 'WAITING FOR PLAYERS';
						allow_turn_complete = true;
						break;
					}
					default: {
						#print('warning: unknown turn status: ' + status);
					}
				}
				if (!allow_turn_complete && is_mouseover) {
					is_mouseover = false;
					blinker.opacity = blink_opacity;
				}
			}
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
