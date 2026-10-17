let input = null;
let status = null;

return {
	on_show: () => { status.text = ''; },
	init: (p) => {
		return p.create('SAVE GLSMAC CAMPAIGN', 620, 138, (body, close) => {
			body.text({class: 'game-popup-text', top: 6, left: 10, font: 'arialnb.ttf:16', text: 'Use .glsmac. An existing file at this path will be replaced.'});
			body.text({class: 'game-popup-text', top: 25, left: 10, text: 'Relative paths use the game working directory.'});
			input = body.input({class: 'popup-input', top: 48, left: 10, right: 10, value: 'campaign.glsmac'});
			status = body.text({class: 'game-popup-text', top: 78, left: 10, right: 10, text: ''});
			body.button({class: 'game-popup-button', top: 108, left: 10, right: 315, text: 'Save', is_ok: true}).on('click', () => {
				const path = input.value;
				try {
					const saved_path = p.game.save_campaign(path);
					input.value = saved_path;
					status.text = 'Campaign saved. The full path is shown above.';
				} catch { : (error) => { status.text = error.reason; } }
				return true;
			});
			body.button({class: 'game-popup-button', top: 108, left: 315, right: 10, text: 'Close', is_cancel: true}).on('click', () => {
				close(false);
				return true;
			});
		});
	},
};
