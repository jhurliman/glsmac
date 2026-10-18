return {
    init: (p) => {
        this.p = p; this.active = false; this.base = null; this.valid_amount = false;
        this.window = p.ui.root.window({class: 'game-popup', zindex: 0.9, width: 680, height: 320, blocker: 'rgba(0, 0, 0, 180)'});
        this.window.hide();
        this.window.text({class: 'game-popup-text', top: 10, left: 12, text: 'Hurry production'});
        this.info = this.window.text({class: 'game-popup-text', top: 40, left: 12, right: 12});
        this.budget = this.window.text({class: 'game-popup-text', top: 65, left: 12, right: 12});
        this.input_label = this.window.text({class: 'game-popup-text', top: 100, left: 12, text: 'Partial credits:'});
        this.input = this.window.input({class: 'popup-input', top: 94, left: 150, width: 180, value: '0'});
        this.input.on('change', (e) => { this.change_amount(e.value); return true; });
        this.input.on('input', (e) => { this.submit(false); return true; });
        this.preview = this.window.text({class: 'game-popup-text', top: 135, left: 12, right: 12});
        this.window.text({class: 'game-popup-text', top: 175, left: 12, right: 12, text: 'Partial payment prevents more Hurry here until your next turn.'});
        this.window.text({class: 'game-popup-text', top: 200, left: 12, right: 12, text: 'Even zero mineral gains count. Production completes during the next turn.'});
        this.full_button = this.window.button({class: 'game-popup-button', align: 'bottom left', left: 12, width: 212, bottom: 12, height: 28});
        this.full_button.on('click', () => { this.submit(true); return true; });
        this.partial_button = this.window.button({class: 'game-popup-button', align: 'bottom left', left: 234, width: 212, bottom: 12, height: 28, text: 'Partial payment'});
        this.partial_button.on('click', () => { this.submit(false); return true; });
        this.window.button({class: 'game-popup-button', align: 'bottom left', left: 456, right: 12, bottom: 12, height: 28, text: 'Cancel'}).on('click', () => { this.hide_view(); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.hide_view(); } return true; });
        for (event of ['research_choice', 'colony_choice', 'colony_screen', 'turn', 'smac_ending']) { this.window.listen(p.game, event, (e) => { this.hide_view(); }); }
        this.window.listen(p.game, 'turn_status', (e) => { if (e.status == 'please_wait') { this.hide_view(); } });
        return this.window;
    },
    open: (base, data) => {
        this.base = base; this.selection = data; this.active = true; this.valid_amount = false;
        if (!data.available) {
            this.info.text = data.reason; this.budget.text = ''; this.preview.text = '';
            this.input.hide(); this.input_label.hide(); this.full_button.hide(); this.partial_button.hide();
        } else {
            this.input.show(); this.input_label.show(); this.full_button.show(); this.partial_button.show();
            this.info.text = 'Stored ' + #to_string(data.stored) + ' / ' + #to_string(data.cost) + ' minerals; full quote: ' + #to_string(data.quote) + ' credits.';
            this.budget.text = 'Treasury: ' + #to_string(data.credits) + '; reserved spending: ' + #to_string(data.reserved) + '.';
            this.full_button.text = data.can_full ? 'Full payment: ' + #to_string(data.quote) : 'Full payment unavailable';
            let amount = data.credits - data.reserved;
            if (amount > data.quote) { amount = data.quote; }
            this.input.value = #to_string(amount);
            this.change_amount(#to_string(amount));
        }
        this.window.show();
    },
    change_amount: (text) => {
        if (!this.active || !this.selection.available) { return; }
        this.valid_amount = false;
        try {
            const amount = #to_int(text);
            if (amount < 0 || amount > this.selection.quote || #to_string(amount) != text) { throw InvalidAmount('Use whole credits from zero through the quote'); }
            const base = this.base;
            const data = base.get_smac_hurry_preview(amount);
            if (!#is_defined(data) || data.expected_state != this.selection.expected_state) { this.hide_view(); return; }
            this.amount = amount; this.valid_amount = true;
            this.preview.text = 'Partial payment: ' + #to_string(data.paid) + ' credits; stored minerals become ' + #to_string(data.minerals_after) + '.';
        } catch { : (error) => {
            this.preview.text = 'Enter whole credits from 0 to ' + #to_string(this.selection.quote) + '.';
        } }
    },
    submit: (full) => {
        if (!this.active || !this.selection.available || (full && !this.selection.can_full) || (!full && !this.valid_amount)) { return; }
        const base = this.base; const expected = this.selection.expected_state; const amount = full ? 0 : this.amount;
        this.p.game.event('hurry_smac_production', {base: base, expected_state: expected, full: full, amount: amount});
        this.hide_view();
    },
    on_base_update: (base) => {
        if (!this.active) { return; }
        if (base.id != this.base.id) { this.hide_view(); return; }
        const data = base.get_smac_hurry_preview(0);
        if (!#is_defined(data) || data.expected_state != this.selection.expected_state) { this.hide_view(); }
    },
    hide_view: () => { this.active = false; this.base = null; this.window.hide(); },
};
