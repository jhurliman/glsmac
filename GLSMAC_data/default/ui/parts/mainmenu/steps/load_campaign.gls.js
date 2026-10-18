return (i) => {
	let input = null;
	let status = null;
	i.popup.show({
		title: 'Load GLSMAC Campaign', width: 620, height: 200,
		generator: (body) => {
			body.text({class: 'popup-text', top: 10, left: 15, text: 'Load a GLSMAC .glsmac campaign file.'});
			body.text({class: 'popup-text', top: 30, left: 15, text: 'Original .SAV files are not supported.'});
			body.text({class: 'popup-text', top: 50, left: 15, text: 'Relative paths use the game working directory.'});
			input = body.input({class: 'popup-input', top: 78, left: 15, right: 15, value: 'campaign.glsmac'});
			status = body.text({class: 'popup-text', top: 110, left: 15, right: 15, text: ''});
		},
		buttons: [
			{style: {text: 'Load', align: 'left', is_ok: true}, onclick: () => {
				const path = input.value;
				let accepted = false;
				try { i.glsmac.load_campaign(path); accepted = true; }
				catch { : (error) => { if (#is_defined(status)) { status.text = error.reason; } } }
				if (accepted) { i.popup.dismiss(); }
				return true;
			}},
			{style: {text: 'Cancel', align: 'right', is_cancel: true}, onclick: () => { i.popup.back(); return true; }},
		],
	});
};
