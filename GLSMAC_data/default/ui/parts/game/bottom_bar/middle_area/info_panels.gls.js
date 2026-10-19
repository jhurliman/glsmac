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

    request_graph: () => {
        if (!this.graph_visible) { return; }
        this.graph_dirty = true;
        if (this.graph_pending) { return; }
        this.graph_dirty = false;
        this.graph_request = this.graph_request + 1;
        if (this.get_width() != 455) {
            this.graph.hide(); this.history.show(); return;
        }
        this.graph_pending = true;
        if (!this.game.request_smac_dominance_graph(this.graph_request)) {
            this.graph_pending = false;
            this.graph.hide(); this.history.show();
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
		for (let value_row = 0; value_row < 3; value_row++) { this.values[value_row].text = 'unavailable'; }
		const summary = this.game.get_smac_faction_summary();
		if (#is_defined(summary)) {
			if (#is_defined(summary.integrity)) { this.values[0].text = summary.integrity; }
			if (#is_defined(summary.might)) { this.values[1].text = summary.might; }
			if (#is_defined(summary.council_votes)) { this.values[2].text = #to_string(summary.council_votes); }
		}
		for (let placed_value = 0; placed_value < 3; placed_value++) {
			this.place(this.values[placed_value], placed_value + 2, 5 + this.labels[placed_value + 2].measure().width);
		}
        this.request_graph();
	},

    on_hide: () => { this.graph_visible = false; this.graph_dirty = false; },

	on_show: () => {
        this.graph_visible = true;
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
		// Current ranking words are not a historical graph. Values stay absent
		// unless the real local participant query supplies their authority.
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
        this.graph_visible = true;
        this.graph_request = 0;
        this.graph_pending = false;
        this.graph_dirty = false;
        this.graph = this.panels[1].surface({left: 3, top: 3, width: 220, height: 86});
        this.graph.hide();
		this.heading = this.panels[1].text({class: 'bottombar-info-text', text: 'Faction Dominance'});
		this.history = this.panels[1].text({class: 'bottombar-info-text', text: 'History unavailable'});
		this.place(this.heading, 0, 5);
		this.place(this.history, 1, 5);
		this.button = p.frame.button({class: 'bottombar-panel-page-button', align: 'bottom right', bottom: 5, right: 5, text: 'I', group: 'pages'});
        this.page.on('resize', (e) => { this.refresh(this.get_width()); this.request_graph(); return true; });
        this.page.listen(p.game, 'smac_dominance_graph', (e) => {
            if (e.request_id != this.graph_request) { return; }
            this.graph_pending = false;
            if (this.get_width() == 455 && #is_defined(e.frame)) {
                this.graph.smacdominancegraph(e.frame);
                this.graph.show(); this.history.hide();
            } else {
                this.graph.hide(); this.history.show();
            }
            if (this.graph_dirty) { this.request_graph(); }
        });
		this.page.listen(p.game, 'turn', (e) => { this.refresh_data(); });
		this.page.listen(p.game, 'player_update', (e) => { this.refresh_data(); });
		this.page.listen(p.game, 'update_base', (e) => { this.refresh_data(); });
		this.page.listen(p.game, 'base_despawn', (e) => { this.refresh_data(); });
		this.page.listen(p.game.get_bm(), 'base_spawn', (e) => { this.refresh_data(); });
		this.refresh(this.get_width());
		this.refresh_data();
	},
};
