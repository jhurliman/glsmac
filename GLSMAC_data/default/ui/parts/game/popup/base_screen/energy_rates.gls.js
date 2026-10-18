return {
    init: (p) => {
        this.p = p; this.base = null; this.active = false; this.closing = false; this.dismiss = false; this.request_id = 0;
        this.locked = [false, false, false]; this.rows = [];
        this.window = p.ui.root.window({class: 'game-popup', zindex: 0.9, width: 660, height: 360, blocker: 'rgba(0, 0, 0, 180)'});
        this.window.hide();
        this.window.text({class: 'game-popup-text', top: 10, left: 12, text: 'Faction energy allocation'});
        this.window.text({class: 'game-popup-text', top: 37, left: 12, text: 'Changes take effect immediately in all owned Bases.'});
        const names = ['Economy', 'Labs', 'Psych'];
        const axes = [0, 2, 1];
        let index = 0;
        for (name of names) {
            const row = index; const axis = axes[row]; const minus = 29 + 2 * row; const plus = minus + 1;
            this.window.text({class: 'game-popup-text', top: 76 + row * 48, left: 18, text: name});
            const value = this.window.text({class: 'game-popup-text', top: 76 + row * 48, left: 170, text: ''});
            this.window.button({class: 'game-popup-button', align: 'top left', top: 70 + row * 48, left: 260, width: 62, height: 28, text: '-'}).on('click', () => { this.adjust(minus); return true; });
            this.window.button({class: 'game-popup-button', align: 'top left', top: 70 + row * 48, left: 330, width: 62, height: 28, text: '+'}).on('click', () => { this.adjust(plus); return true; });
            const lock = this.window.button({class: 'game-popup-button', align: 'top left', top: 70 + row * 48, left: 430, width: 210, height: 28, text: ''});
            lock.on('click', () => { if (this.active && !this.closing) { this.locked[axis] = !this.locked[axis]; this.render(this.selection); } return true; });
            this.rows :+ {value: value, lock: lock, axis: axis}; index++;
        }
        this.summary = this.window.text({class: 'game-popup-text', top: 220, left: 12, text: ''});
        this.window.text({class: 'game-popup-text', top: 250, left: 12, text: 'Enter keeps these rates. Escape restores the opening rates.'});
        this.window.button({class: 'game-popup-button', align: 'bottom left', left: 12, width: 310, bottom: 12, height: 28, text: 'Keep'}).on('click', () => { this.close(false); return true; });
        this.window.button({class: 'game-popup-button', align: 'bottom right', right: 12, width: 310, bottom: 12, height: 28, text: 'Cancel'}).on('click', () => { this.close(true); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.close(true); } if (e.modifiers == {} && e.code == 'ENTER') { this.close(false); } return true; });
        this.window.listen(p.game, 'smac_energy_rates_menu', (e) => { this.receive(e); });
    },
    set_base: (base) => { this.base = base; },
    open: () => {
        if (this.base == null || this.active) { return; }
        const base = this.base; const preview = base.get_smac_energy_rate_preview();
        if (!#is_defined(preview)) { return; }
        const expected = preview.expected_state; this.dismiss = false;
        this.p.game.event('open_smac_energy_rates', {base: base, expected_state: expected});
    },
    receive: (e) => {
        if (e.closed) {
            if (this.active && e.request_id == this.request_id) { this.active = false; this.closing = false; this.window.hide(); }
            return;
        }
        if (e.request_id < this.request_id) { return; }
        if (e.request_id != this.request_id) { this.locked = [false, false, false]; this.closing = false; }
        this.active = true; this.request_id = e.request_id; this.selection = e.selection;
        this.render(e.selection);
        if (this.dismiss) { this.close(true); return; }
        this.window.show();
    },
    render: (data) => {
        const rates = [data.economy, data.labs, data.psych]; let index = 0;
        for (row of this.rows) { row.value.text = #to_string(rates[index] * 10) + '%'; row.lock.text = this.locked[row.axis] ? '[x] Locked' : '[ ] Unlocked'; index++; }
        let economy = 0; let labs = 0; let psych = 0;
        for (base of data.bases) { economy += base.economy; labs += base.labs; psych += base.psych; }
        this.summary.text = 'Current Base outputs: ' + #to_string(economy) + ' economy, ' + #to_string(labs) + ' labs, ' + #to_string(psych) + ' psych.';
    },
    adjust: (button) => {
        if (!this.active || this.closing) { return; }
        let locks = 0; let power = 1;
        for (locked of this.locked) { if (locked) { locks += power; } power *= 2; }
        const request = this.request_id;
        this.p.game.event('adjust_smac_energy_rates', {request_id: request, button: button, locks: locks});
    },
    close: (cancel) => {
        if (!this.active || this.closing) { return; }
        const request = this.request_id;
        this.p.game.event('close_smac_energy_rates', {request_id: request, cancel: cancel});
        this.closing = true;
    },
    hide_view: () => { this.dismiss = true; if (this.active) { this.close(true); } },
};
