return {
	init: (p) => {
		this.buttons = p.body.panel({
			align: 'top', top: 3, left: 3, right: 3, height: 26,
			border: 'rgb(136,69,28),2',
		});
		this.buttons.text({
			class: 'base-screen-frame-text', align: 'center',
			text: 'Governor and production priorities unavailable',
		});
		return this.buttons;
	},
};
