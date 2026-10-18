return {
    init: (p) => {
        this.p = p; this.active = false; this.base = null;
        this.window = p.ui.root.window({class: 'game-popup', zindex: 0.9, width: 520, height: 190, blocker: 'rgba(0, 0, 0, 180)'});
        this.window.hide();
        this.window.text({class: 'game-popup-text', top: 12, left: 12, text: 'Rename Base'});
        this.window.text({class: 'game-popup-text', top: 38, left: 12, right: 12, text: '24 bytes; printable ASCII display only.'});
        this.input = this.window.input({class: 'popup-input', top: 68, left: 12, right: 12, value: ''});
        this.input.on('change', (e) => { this.input.value = (e.value + '')[0::24]; return true; });
        this.input.on('input', (e) => { this.submit(); return true; });
        this.window.button({class: 'game-popup-button', align: 'bottom left', left: 12, right: 266, bottom: 12, height: 28, text: 'Accept'}).on('click', () => { this.submit(); return true; });
        this.window.button({class: 'game-popup-button', align: 'bottom left', left: 266, right: 12, bottom: 12, height: 28, text: 'Cancel'}).on('click', () => { this.cancel(); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.cancel(); } return true; });
        for (event of ['research_choice', 'colony_choice', 'colony_screen', 'turn', 'smac_ending']) { this.window.listen(p.game, event, (e) => { this.hide_view(); }); }
        this.window.listen(p.game, 'turn_status', (e) => { if (e.status == 'please_wait') { this.hide_view(); } });
        return this.window;
    },
    open: (base, selection) => { this.base = base; this.selection = selection; this.input.value = selection.name; this.active = true; this.window.show(); },
    submit: () => {
        if (!this.active) { return; }
        const base = this.base; const data = this.selection; const name = this.input.value;
        this.p.game.event('commit_smac_rename', {base: base, request_id: data.request_id, expected_state: data.expected_state, name: name});
        this.hide_view();
    },
    cancel: () => {
        if (!this.active) { return; }
        const base = this.base; const data = this.selection;
        this.p.game.event('cancel_smac_rename', {base: base, request_id: data.request_id, expected_state: data.expected_state});
        this.hide_view();
    },
    on_base_update: (base) => { if (this.active && (base.id != this.base.id || base.get_smac_rename_state() != this.selection.expected_state)) { this.hide_view(); } },
    hide_view: () => { this.active = false; this.base = null; this.window.hide(); },
};
