return (ui) => {

	ui.class('default-scroll-from').set({
		background: 'icons.pcx:crop(107, 4, 124, 21)',
		_hover: {
			background: 'icons.pcx:crop(132, 4, 149, 21)',
		},
		_active: {
			background: 'icons.pcx:crop(157, 4, 174, 21)',
		},
	});
	ui.class('default-scroll-to').set({
		background: 'icons.pcx:crop(107, 29, 124, 46)',
		_hover: {
			background: 'icons.pcx:crop(132, 29, 149, 46)',
		},
		_active: {
			background: 'icons.pcx:crop(157, 29, 174, 46)',
		},
	});
	ui.class('default-scroll-slider').set({
		background: 'icons.pcx:crop(260, 4, 276, 17)',
		_hover: {
			background: 'icons.pcx:crop(104, 104, 121, 115)',
		},
	});

	ui.class('default-scroll-v').set({
		scroll_type: 'vertical',
		fromto_size: 18,
		slider_size: 18,
		class_from: 'default-scroll-from',
		class_to: 'default-scroll-to',
		class_slider: 'default-scroll-slider',
		min: 0.0,
		max: 1.0,
		value: 0.0,
		speed: 0.5,
		width: 18,
		top: 3,
		bottom: 3,
		right: 3,
	});

	// Horizontal list overflow uses the same installed scrollbar artwork.
	ui.class('default-scroll-left').set({
		background: 'icons.pcx:crop(107,29,124,46):rotate()',
		_hover: {background: 'icons.pcx:crop(132,29,149,46):rotate()'},
		_active: {background: 'icons.pcx:crop(157,29,174,46):rotate()'},
	});
	ui.class('default-scroll-right').set({
		background: 'icons.pcx:crop(107,4,124,21):rotate()',
		_hover: {background: 'icons.pcx:crop(132,4,149,21):rotate()'},
		_active: {background: 'icons.pcx:crop(157,4,174,21):rotate()'},
	});
	ui.class('default-scroll-h').set({
		scroll_type: 'horizontal', fromto_size: 18, slider_size: 18,
		class_from: 'default-scroll-left', class_to: 'default-scroll-right',
		class_slider: 'default-scroll-slider',
		min: 0.0, max: 1.0, value: 0.0, speed: 0.5,
		height: 18, left: 3, right: 3, bottom: 3,
	});

};
