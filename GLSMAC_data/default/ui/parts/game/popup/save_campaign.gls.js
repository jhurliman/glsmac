let input = null;
let status = null;
let pending_request = null;

return {
	on_show: () => { status.text = pending_request == null ? '' : 'Saving campaign...'; },
	init: (p) => {
        p.game.on('campaign_saved', (result) => {
            if (pending_request == null || result.request_id != pending_request) { return; }
            pending_request = null;
            if (!#is_defined(status) || !#is_defined(input)) { return; }
            if (result.error != '') { status.text = result.error; }
            else {
                input.value = result.path;
                status.text = 'Campaign saved. The full path is shown above.';
            }
        });
		return p.create('SAVE GLSMAC CAMPAIGN', 620, 138, (body, close) => {
			body.text({class: 'game-popup-text', top: 6, left: 10, font: 'arialnb.ttf:16', text: 'Use .glsmac. An existing file at this path will be replaced.'});
			body.text({class: 'game-popup-text', top: 25, left: 10, text: 'Relative paths use the game working directory.'});
			input = body.input({class: 'popup-input', top: 48, left: 10, right: 10, value: 'campaign.glsmac'});
			status = body.text({class: 'game-popup-text', top: 78, left: 10, right: 10, text: ''});
			body.button({class: 'game-popup-button', top: 108, left: 10, right: 315, text: 'Save', is_ok: true}).on('click', () => {
				if (pending_request != null) { return true; }
				const path = input.value;
				try {
					pending_request = p.game.save_campaign(path);
					status.text = 'Saving campaign...';
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
