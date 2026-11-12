return {
    init: (p) => {
        this.p = p;
        p.root.on('keydown', (e) => { return this.shortcut(e); });
    },
    shortcut: (e) => {
        if (e.code != 'U' || e.modifiers != {}) { return false; }
        return this.open();
    },
    open: () => {
        const p = this.p;
        if (#is_defined(p.modules.preferences) && p.modules.preferences.active) { return false; }
        if (p.game.get_ruleset() != 'smac' || p.modules.popup.popup != null) { return false; }
        for (name of ['first_base', 'research', 'colony']) {
            const module = p.modules[name];
            if (#is_defined(module) && #is_defined(module.pending) && module.pending != null) { return false; }
        }
        const editor = p.modules.popup.popup_defs.base_screen.sections.workshop;
        if (editor.active || editor.opening) { return false; }
        try {
            const player = p.game.get_player();
            const data = player.get_smac_workshop();
            if (!#is_defined(data)) { return false; }
            p.modules.menu.close_all();
            editor.begin(player, data, (0 - 1));
        } catch { : (error) => {
            editor.hide_view();
            p.process_message('Workshop unavailable: ' + error.reason);
        } }
        return true;
    },
};
