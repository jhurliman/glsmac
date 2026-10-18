return {
    init: (p) => {
        this.p = p; this.base = null; this.active = false; this.options = []; this.rows = [];
        this.labels = ['Governor active', 'Allow multiple or no priorities', 'Automate newly built units', 'Manage citizens and specialists', 'Manage production', 'Produce scouts', 'Produce land combat units', 'Produce naval combat units', 'Produce air combat units', 'Produce land defense units', 'Produce air defense units', 'Produce prototypes', 'Produce transports', 'Produce probe teams', 'Produce terraformers', 'Produce colony pods', 'Produce facilities', 'Produce Secret Projects', 'Hurry production'];
        this.buttons = p.body.panel({align: 'top', top: 3, left: 3, right: 3, height: 26, border: 'rgb(136,69,28),2'});
        this.buttons.button({class: 'game-popup-button', align: 'left', left: 4, width: 180, top: 1, bottom: 1, text: 'Governor settings'}).on('click', () => { this.open(); return true; });
        this.energy = #include('./energy_rates'); this.energy.init(p);
        this.buttons.button({class: 'game-popup-button', align: 'left', left: 190, width: 180, top: 1, bottom: 1, text: 'Energy allocation'}).on('click', () => { this.energy.open(); return true; });
        this.window = p.ui.root.window({class: 'game-popup', zindex: 0.9, width: 660, height: 420, blocker: 'rgba(0, 0, 0, 180)'});
        this.window.hide();
        this.window.text({class: 'game-popup-text', top: 10, left: 12, text: 'Governor settings'});
        let index = 0;
        for (label of this.labels) {
            const id = index;
            const column = index >= 10 ? 1 : 0;
            const row = index >= 10 ? index - 10 : index;
            const button = this.window.button({class: 'game-popup-button', align: 'top left', top: 38 + row * 27, left: 12 + column * 320, width: 310, height: 25, text: label});
            button.on('click', () => { this.options[id] = !this.options[id]; this.refresh(); return true; });
            this.rows :+ button; index++;
        }
        this.note = this.window.text({class: 'game-popup-text', top: 314, left: 12, text: ''});
        this.window.text({class: 'game-popup-text', top: 338, left: 12, text: 'Saved settings become the default for newly founded Bases.'});
        this.window.button({class: 'game-popup-button', align: 'bottom left', left: 12, width: 310, bottom: 12, height: 28, text: 'Apply'}).on('click', () => { this.submit(); return true; });
        this.window.button({class: 'game-popup-button', align: 'bottom right', right: 12, width: 310, bottom: 12, height: 28, text: 'Cancel'}).on('click', () => { this.hide_view(); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.hide_view(); } return true; });
        for (event of ['research_choice', 'colony_choice', 'colony_screen', 'turn', 'smac_ending']) { this.window.listen(p.game, event, (e) => { this.hide_view(); }); }
        this.window.listen(p.game, 'turn_status', (e) => { if (e.status == 'please_wait') { this.hide_view(); } });
        return this.buttons;
    },
    set_base: (base) => { this.active = false; this.window.hide(); this.base = base; this.energy.set_base(base); },
    open: () => {
        if (this.base == null || !this.base.has_smac_resource_context) { return; }
        const data = this.base.get_smac_governor_settings(); if (!#is_defined(data)) { return; }
        this.selection = data; this.options = [];
        for (label of this.labels) { this.options :+ false; }
        let remainder = data.controls; let power = 262144; let index = 18;
        while (index >= 0) { this.options[index] = remainder >= power; if (this.options[index]) { remainder -= power; } power /= 2; index--; }
        this.active = true; this.refresh(); this.window.show();
    },
    refresh: () => {
        let index = 0; for (button of this.rows) { button.text = (this.options[index] ? '[x] ' : '[ ] ') + this.labels[index]; index++; }
        this.unavailable = this.options[0] && (this.options[2] || this.options[3] || this.options[4]);
        this.note.text = this.unavailable ? 'Active production, citizen and new-unit automation unavailable.' : 'Apply settings to this Base.';
    },
    submit: () => {
        if (!this.active || this.unavailable) { return; }
        let controls = 0; let power = 1;
        for (selected of this.options) { if (selected) { controls += power; } power *= 2; }
        const base = this.base; const expected = this.selection.expected_state;
        this.p.game.event('apply_smac_governor', {base: base, expected_state: expected, controls: controls});
        this.hide_view();
    },
    hide_view: () => { this.active = false; this.window.hide(); this.energy.hide_view(); },
};
