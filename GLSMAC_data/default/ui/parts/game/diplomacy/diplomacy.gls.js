return {
    init: (p) => {
        this.p = p; this.active = false; this.waiting = false; this.known = []; this.pending_contact = false; this.generation = 0;
        this.window = p.root.window({class: 'game-popup', zindex: 0.9, width: 680, height: 540, blocker: 'rgba(0, 0, 0, 180)'});
        this.window.hide();
        this.window.text({class: 'game-popup-text', left: 12, top: 12, text: 'Commlink'});
        this.status = this.window.text({class: 'game-popup-text', left: 12, right: 12, top: 40, text: ''});
        this.rows = this.window.scrollview({left: 12, right: 12, top: 76, bottom: 54, has_vscroll: true, has_hscroll: false, vscroll_class: 'default-scroll-v'});
        this.window.button({class: 'game-popup-button', left: 12, right: 12, bottom: 12, height: 28, text: 'Close communications'}).on('click', () => { this.close(); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.close(); } return true; });
        for (event of ['turn', 'research_choice', 'colony_choice', 'colony_screen', 'smac_ending']) { this.window.listen(p.game, event, (e) => { this.close(); }); }
        this.window.listen(p.game, 'turn_status', (e) => { if (e.status == 'please_wait') { this.close(); } else { if (this.pending_contact) { this.open(); } } });
        this.window.listen(p.game, 'smac_diplomacy_response', (e) => {
            if (!this.active) { return; }
            this.waiting = false; this.status.text = e.text; this.refresh();
        });
        this.window.listen(p.game, 'map_knowledge', (e) => { this.notice_contacts(); });
    },
    open: () => {
        const p = this.p;
        if (p.game.get_ruleset() != 'smac' || p.game.get_turn_status() == 'please_wait' || p.modules.popup.popup != null) { return; }
        for (name of ['first_base', 'research', 'colony']) {
            const module = p.modules[name];
            if (#is_defined(module) && #is_defined(module.pending) && module.pending != null) { return; }
        }
        p.modules.menu.close_all(); this.active = true; this.waiting = false; this.pending_contact = false;
        this.status.text = 'Propose a free agreement or close communications.';
        this.refresh(); this.window.show();
    },
    refresh: () => {
        this.generation = this.generation + 1; const generation = this.generation; this.rows.clear(); const data = this.p.game.get_smac_diplomacy(); let top = 0;
        if (#sizeof(data) == 0) { this.rows.text({class: 'game-popup-text', left: 4, top: 4, text: 'No foreign commlink frequencies discovered.'}); }
        this.known = [];
        for (entry of data) {
            const row = entry; const request = row.expected_state; const target = row.target;
            this.known :+ target;
            this.rows.text({class: 'game-popup-text', left: 4, top: top, text: row.name + ': ' + row.relation});
            if (row.unavailable != '') {
                this.rows.text({class: 'game-popup-text', left: 4, top: top + 26, text: row.unavailable});
            } else {
                for (kind of ['truce', 'treaty']) {
                    const offer = kind;
                    this.rows.button({class: 'game-popup-button', left: offer == 'truce' ? 4 : 310, width: 292, top: top + 24, height: 28,
                        text: offer == 'truce' ? 'Propose Truce' : 'Propose Treaty of Friendship'}).on('click', () => {
                        if (this.active && !this.waiting && this.generation == generation) { this.offer(target, offer, request); } return true;
                    });
                }
            }
            top += 74;
        }
    },
    offer: (target, kind, expected) => {
        if (!this.active || this.waiting) { return; }
        this.waiting = true; this.status.text = 'Awaiting response...';
        try { this.p.game.event('offer_smac_peace', {target: target, kind: kind, expected_state: expected}); }
        catch { : (error) => { this.waiting = false; this.status.text = error.reason; } }
    },
    notice_contacts: () => {
        if (this.p.game.get_ruleset() != 'smac') { return; }
        const contacts = this.p.game.get_smac_diplomacy(); let discovered = false;
        for (row of contacts) {
            let found = false; for (target of this.known) { if (target == row.target) { found = true; } }
            if (!found) { discovered = true; this.known :+ row.target; }
        }
        if (discovered) { this.pending_contact = true; }
        if (this.pending_contact && !this.active) { this.open(); }
    },
    close: () => { this.generation = this.generation + 1; this.active = false; this.waiting = false; this.window.hide(); },
};
