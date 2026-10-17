return (i, next) => {
	let width = null;
	let height = null;
	i.popup.show({
		title: 'Custom Planet Size', width: 480, height: 160,
		generator: (body) => {
			body.text({class: 'popup-text', text: 'Width (32–512, even):', left: 15, top: 13});
			width = body.input({class: 'popup-input', left: 240, top: 12, width: 210, value: #to_string(i.settings.global.map.size_x)});
			body.text({class: 'popup-text', text: 'Height (16–256, even):', left: 15, top: 43});
			height = body.input({class: 'popup-input', left: 240, top: 42, width: 210, value: #to_string(i.settings.global.map.size_y)});
		},
		buttons: [
			{style: {text: 'OK', align: 'left', is_ok: true}, onclick: () => {
				let accepted = false;
				try { i.settings.global.set_world_size(99, #to_int(width.value), #to_int(height.value)); accepted = true; }
				catch { : (error) => { i.popup.error(error.reason); } }
				if (accepted) { i.popup.dismiss(); next(); }
				return true;
			}},
			{style: {text: 'Cancel', align: 'right', is_cancel: true}, onclick: () => { i.popup.dismiss(); return true; }},
		],
	});
};
