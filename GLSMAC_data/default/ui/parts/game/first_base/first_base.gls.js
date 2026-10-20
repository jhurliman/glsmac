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
            this.name = this.window.input({class: 'popup-input', left: 68, right: 10, top: 12, max_bytes: 23, value: ''});
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
        this.window = p.root.window({class: 'window', title: 'Enter a name for your first base.',
            align: 'top left', left: 10, top: 40, width: 320, height: 140,
            zindex: 0.9, blocker: 'transparent'});
        this.window.hide();
        this.window.text({class: 'popup-text', left: 10, top: 14, text: 'Name:'});
        this.status = this.window.text({class: 'popup-text', font: 'arialnb.ttf:14', left: 10, right: 10, top: 42, text: ''});
        for (style of [{text: 'OK', align: 'bottom left', left: 10}, {text: 'Cancel', align: 'bottom right', right: 10}]) {
            const item = this.window.button(style + {class: 'popup-button', bottom: 10, width: 130});
            // FIRSTBASE copies current text on every modal return, including
            // cancellation; it does not use ordinary Rename's discard policy.
            item.on('click', () => { this.submit(); return true; });
        }
        this.window.on('keydown', (e) => {
            if (e.modifiers == {} && (e.code == 'ESCAPE' || e.code == 'ENTER')) { this.submit(); }
            if (e.modifiers == {ctrl: true, shift: true} && e.code == 'Q') { this.p.glsmac.reset(); }
            return true;
        });
        this.window.listen(p.game, 'first_base_name', (e) => { this.refresh(); });
        this.refresh();
    },
};
