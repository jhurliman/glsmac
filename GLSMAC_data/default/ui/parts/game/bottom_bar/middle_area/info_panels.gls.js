return {
	// Text actors use their lower geometry edge as the baseline. Keep the
	// original baseline independent of each string's measured ink height.
	place: (label, row, left) => {
		label.left = left;
		label.top = 14 + row * 13 - label.measure().height;
	},
	refresh: (width) => {
		const padding = 3;
		let panel_width = (width - padding) / 2;
		if (panel_width < 1) { panel_width = 1; }
		for (let i = 0; i < 2; i++) {
			this.panels[i].left = i * (panel_width + padding);
			this.panels[i].width = panel_width;
		}
	},

	refresh_data: () => {
		// A roster controller name is not the faction leader's name. Source
		// identity is retained with the faction; old snapshots keep its absence.
		const player = this.game.get_player();
		const faction = player.get_faction();
		let name = 'Faction unavailable';
		let leader = 'Leader unavailable';
		if (#is_defined(faction)) {
			name = faction.name;
			const identity = faction.display_identity;
			if (#is_defined(identity)) {
				name = identity.faction_name;
				leader = identity.leader_title + ' ' + identity.leader_name;
			}
		}
		this.labels[0].text = name;
		this.labels[1].text = leader;
		this.place(this.labels[0], 0, 5);
		this.place(this.labels[1], 1, 5);
	},

	on_show: () => {
		this.refresh(this.get_width());
		this.refresh_data();
	},

	init: (p) => {
		this.game = p.game;
		this.get_width = p.get_info_width;
		this.panels = [];
		this.labels = [];
		p.ui.class('bottombar-info-panel').extend('bottombar-panel-inner').set({top: 0, bottom: 0, overflow: 'hidden'});
		p.ui.class('bottombar-info-text').set({font: 'arial.ttf:12:mono', color: 'rgb(116,156,56)', align: 'top left'});
		p.ui.class('bottombar-info-value').extend('bottombar-info-text').set({color: 'rgb(184,192,96)'});
		// Native column clips at 1024: [269,73,495,165) / [498,73,724,165).
		this.page = p.frame.panel({left: 7, top: 7, right: 39, bottom: 7, overflow: 'hidden'});
		for (let i = 0; i < 2; i++) { this.panels :+ this.page.panel({class: 'bottombar-info-panel'}); }
		for (let label_index = 0; label_index < 5; label_index++) {
			this.labels :+ this.panels[0].text({class: 'bottombar-info-text', text: ''});
		}
		// Current ranking words are not a historical graph, and integrity/might
		// adjectives have no admitted source projection yet.
		this.labels[2].text = 'Integrity: ';
		this.labels[3].text = 'Might: ';
		this.labels[4].text = 'Planetary Council Votes: ';
		this.values = [];
		for (let rating_row = 2; rating_row < 5; rating_row++) {
			this.place(this.labels[rating_row], rating_row, 5);
			const value = this.panels[0].text({class: 'bottombar-info-value', text: 'unavailable'});
			this.place(value, rating_row, 5 + this.labels[rating_row].measure().width);
			this.values :+ value;
		}
		this.heading = this.panels[1].text({class: 'bottombar-info-text', text: 'Faction Dominance'});
		this.history = this.panels[1].text({class: 'bottombar-info-text', text: 'History unavailable'});
		this.place(this.heading, 0, 5);
		this.place(this.history, 1, 5);
		this.button = p.frame.button({class: 'bottombar-panel-page-button', align: 'bottom right', bottom: 5, right: 5, text: 'I', group: 'pages'});
		this.page.on('resize', (e) => { this.refresh(this.get_width()); return true; });
		this.page.listen(p.game, 'turn', (e) => { this.refresh_data(); });
		this.page.listen(p.game, 'player_update', (e) => { this.refresh_data(); });
		this.refresh(this.get_width());
		this.refresh_data();
	},
};
