return {
    refresh: () => {
        const pending = this.p.game.get_pending_first_base_name();
        if (pending == null) {
            if (this.name != null) { this.name.remove(); this.name = null; }
            this.pending = null; this.submitted = false; this.window.hide(); return;
        }
        if (this.pending != null && this.pending.request_id == pending.request_id) { return; }
        this.pending = pending; this.submitted = false;
        if (this.name == null) {
            this.name = this.body.input({class: 'smac-original-input', left: 53, top: 1, width: 430, height: 20, max_bytes: 23, value: ''});
            this.name.on('input', () => { this.submit(); return true; });
        }
        this.name.value = '' + pending.name;
        this.status.text = '';
        this.p.modules.menu.close_all(); this.window.show(); this.name.focus();
    },
    submit: () => {
        if (this.pending == null || this.submitted) { return; }
        const request_id = this.pending.request_id;
        const name = '' + this.name.value;
        this.submitted = true;
        try { this.p.game.finish_first_base_name(request_id, name); }
        catch { : (error) => { this.submitted = false; this.status.text = error.reason; } }
    },
    init: (p) => {
        this.p = p; this.pending = null; this.submitted = false; this.name = null;
        const original = #include('../../smac_dialog')(p.root, 'first_base', 'Enter a name for your first base.');
        this.window = original.window; this.body = original.body;
        this.window.zindex = 0.9;
        this.window.hide();
        this.body.text({class: 'popup-text', left: 6, top: 0, text: 'Name:'});
        this.status = this.window.text({class: 'popup-text', font: 'arialnb.ttf:14', left: 33, right: 67, top: 110, text: ''});
        // The installed FIRSTBASE template has one OK control. Escape still
        // transfers current text through the original modal result boundary.
        const item = original.footer.button({class: 'popup-button', left: 0, top: 0,
            width: 480, height: 20, text: 'OK'});
        item.on('click', () => { this.submit(); return true; });
        this.window.on('keydown', (e) => {
            if (e.modifiers == {} && (e.code == 'ESCAPE' || e.code == 'ENTER')) { this.submit(); }
            if (e.modifiers == {ctrl: true, shift: true} && e.code == 'Q') { this.p.glsmac.reset(); }
            return true;
        });
        this.window.listen(p.game, 'first_base_name', (e) => { this.refresh(); });
        this.refresh();
    },
};
