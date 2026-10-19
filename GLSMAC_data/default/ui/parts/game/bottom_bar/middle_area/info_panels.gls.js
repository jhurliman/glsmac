return {
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
	},

	on_show: () => {
		this.refresh(this.page.width);
		this.refresh_data();
	},

	init: (p) => {
		this.game = p.game;
		this.panels = [];
		this.labels = [];
		p.ui.class('bottombar-info-panel').extend('bottombar-panel-inner').set({top: 0, bottom: 0});
		p.ui.class('bottombar-info-text').set({font: 'arialnb.ttf:14:mono', color: 'rgb(116,156,56)', align: 'top left', left: 4});
		this.page = p.frame.panel({class: 'bottombar-panel-page', overflow: 'hidden'});
		for (let i = 0; i < 2; i++) { this.panels :+ this.page.panel({class: 'bottombar-info-panel'}); }
		for (let label_index = 0; label_index < 4; label_index++) {
			this.labels :+ this.panels[0].text({class: 'bottombar-info-text', top: 4 + label_index * 13, text: ''});
		}
		// Current ranking words are not a historical graph, and integrity/might
		// adjectives have no admitted source projection yet.
		this.labels[2].text = 'Integrity: unavailable';
		this.labels[3].text = 'Might: unavailable';
		this.panels[1].text({class: 'bottombar-info-text', top: 4, text: 'Faction Dominance'});
		this.panels[1].text({class: 'bottombar-info-text', top: 20, text: 'History unavailable'});
		this.button = p.frame.button({class: 'bottombar-panel-page-button', align: 'bottom right', bottom: 5, right: 5, text: 'I', group: 'pages'});
		this.page.on('resize', (e) => { this.refresh(e.width); });
		this.page.listen(p.game, 'turn', (e) => { this.refresh_data(); });
		this.page.listen(p.game, 'player_update', (e) => { this.refresh_data(); });
		this.refresh(this.page.width);
		this.refresh_data();
	},
};
