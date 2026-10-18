return {
    init: (p) => {
        this.p = p; this.active = false; this.base = null;
        this.window = p.ui.root.window({class: 'game-popup', zindex: 0.9, width: 710, height: 480, blocker: 'rgba(0, 0, 0, 180)'});
        this.window.hide();
        this.window.text({class: 'game-popup-text', top: 10, left: 12, text: 'Pending production queue'});
        this.window.text({class: 'game-popup-text', top: 35, left: 12, right: 12, text: 'Current production is unchanged. New choices must be eligible now.'});
        this.caption = this.window.text({class: 'game-popup-text', top: 60, left: 12, text: ''});
        this.choices = this.window.scrollview({left: 12, width: 210, top: 85, bottom: 55, has_hscroll: false, has_vscroll: true, vscroll_class: 'default-scroll-v'});
        this.rows = this.window.panel({left: 230, right: 12, top: 85, bottom: 55});
        this.window.button({class: 'game-popup-button', align: 'bottom left', left: 12, width: 330, bottom: 12, height: 28, text: 'Accept queue'}).on('click', () => { this.submit(); return true; });
        this.window.button({class: 'game-popup-button', align: 'bottom left', left: 354, right: 12, bottom: 12, height: 28, text: 'Cancel'}).on('click', () => { this.hide_view(); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.hide_view(); } return true; });
        for (event of ['research_choice', 'colony_choice', 'colony_screen', 'turn', 'smac_ending']) { this.window.listen(p.game, event, (e) => { this.hide_view(); }); }
        this.window.listen(p.game, 'turn_status', (e) => { if (e.status == 'please_wait') { this.hide_view(); } });
        return this.window;
    },
    open: (base, data) => {
        this.base = base; this.data = data; this.entries = [];
        for (item of data.items) { this.entries :+ {existing: item.existing, name: item.name}; }
        this.choices.clear();
        let choice_top = 0;
        for (choice of data.choices) {
            const id = choice.source_id; const name = choice.name;
            this.choices.button({class: 'game-popup-button', left: 4, right: 4, top: choice_top, height: 28, text: 'Add ' + name}).on('click', () => {
                if (#sizeof(this.entries) < this.data.limit) { this.entries :+ {source_id: id, name: name}; this.refresh(); }
                return true;
            });
            choice_top += 32;
        }
        this.active = true; this.refresh(); this.window.show();
    },
    refresh: () => {
        this.rows.clear();
        this.caption.text = #to_string(#sizeof(this.entries)) + ' / ' + #to_string(this.data.limit) + ' pending (port storage limit)';
        if (#sizeof(this.entries) == 0) { this.rows.text({class: 'game-popup-text', top: 8, left: 8, text: 'Empty queue: repeat current production.'}); }
        let index = 0;
        for (entry of this.entries) {
            const at = index; const top = index * 32;
            this.rows.text({class: 'game-popup-text', top: top + 6, left: 4, text: #to_string(at + 1) + '. ' + entry.name});
            if (at > 0) { this.rows.button({class: 'game-popup-button', top: top, left: 260, width: 40, height: 28, text: 'Up'}).on('click', () => { this.move(at, at - 1); return true; }); }
            if (at + 1 < #sizeof(this.entries)) { this.rows.button({class: 'game-popup-button', top: top, left: 304, width: 52, height: 28, text: 'Down'}).on('click', () => { this.move(at, at + 1); return true; }); }
            this.rows.button({class: 'game-popup-button', top: top, left: 360, right: 0, height: 28, text: 'Remove'}).on('click', () => { this.remove(at); return true; });
            index++;
        }
    },
    move: (from, to) => { const value = this.entries[from]; this.entries[from] = this.entries[to]; this.entries[to] = value; this.refresh(); },
    remove: (at) => { let next = []; let i = 0; for (entry of this.entries) { if (i != at) { next :+ entry; } i++; } this.entries = next; this.refresh(); },
    submit: () => {
        if (!this.active) { return; }
        let entries = [];
        for (item of this.entries) { entries :+ (#is_defined(item.existing) ? {existing: item.existing} : {source_id: item.source_id}); }
        const base = this.base; const expected = this.data.expected_state;
        this.p.game.event('edit_smac_queue', {base: base, expected_state: expected, entries: entries});
        this.hide_view();
    },
    on_base_update: (base) => { if (this.active && (base.id != this.base.id || base.get_smac_specialist_selection_state() != this.data.expected_state)) { this.hide_view(); } },
    hide_view: () => { this.active = false; this.base = null; this.window.hide(); },
};
