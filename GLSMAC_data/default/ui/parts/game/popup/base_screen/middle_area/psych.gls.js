return {

	init: (p) => {

		this.el = p.area.panel({
			class: 'base-screen-middle-area',
		});

		this.el.text({
			class: 'base-screen-frame-text', align: 'center',
			text: 'Citizen/Psych details unavailable',
		});
		
	},

};
