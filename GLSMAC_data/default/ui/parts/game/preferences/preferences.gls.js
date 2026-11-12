return {
    active: false,
    init: (p) => {
        this.p = p;
        p.root.on('keydown', (e) => { return this.shortcut(e); });
    },
    shortcut: (e) => {
        if (e.modifiers != {ctrl: true}) { return false; }
        let page = (0 - 1);
        if (e.code == 'P') { page = 0; } else if (e.code == 'O') { page = 1; }
        else if (e.code == 'W') { page = 2; } else if (e.code == 'A') { page = 3; }
        else if (e.code == 'I') { page = 4; } else if (e.code == 'M') { page = 5; }
        return page >= 0 ? this.open(page) : false;
    },
    allowed: () => {
        const p = this.p;
        if (this.active || p.game.get_ruleset() != 'smac' || p.modules.popup.popup != null) { return false; }
        for (name of ['first_base', 'research', 'colony']) {
            const module = p.modules[name];
            if (#is_defined(module) && #is_defined(module.pending) && module.pending != null) { return false; }
        }
        const editor = p.modules.popup.popup_defs.base_screen.sections.workshop;
        return !editor.active && !editor.opening;
    },
    difficulty: () => {
        const name = this.p.game.get_settings().global.difficulty_level;
        let index = 0;
        for (canonical of ['Citizen','Specialist','Talent','Librarian','Thinker','Transcend']) {
            if (name == canonical) { return index; } index++;
        }
        throw Error('Unknown SMAC difficulty');
    },
    create: () => {
        const prefs = this; const ui = this.p.ui;
        const window = ui.root.window({class: 'smac-original-window', zindex: 0.97, align: 'top left',
            width: 778, height: 390, background: 'transparent', border: 'transparent',
            blocker: 'rgba(0,0,0,0)', overflow: 'hidden'});
        window.hide();
        const art = window.surface({left: 0, top: 0, width: 778, height: 390,
            smac_frame: [[9,23,0,723,390],[9,27,4,719,386],[18,30,7,250,383],[18,253,7,716,383]]});
        try {
            this.pages = art.smacpreferencestext(); this.words = art.smacpreferencescaptions();
        } catch { : (error) => { window.remove(); throw Error(error.reason); } }
        this.window = window; this.buttons = []; this.groups = []; this.rows = []; this.focus = (0 - 1);
        this.probe = window.text({font: 'arial.ttf:12:mono', text: ''}); this.probe.hide();
        this.left_rail = window.surface({left: 15, top: 0, width: 8, height: 390});
        this.right_rail = window.surface({left: 723, top: 0, width: 8, height: 390});
        this.model = #include('./draft')(this.pages,
            () => { const state = prefs.p.glsmac.config.get_smac_preferences(false, prefs.difficulty()); prefs.audio = state.audio; return state.values; },
            () => { const state = prefs.p.glsmac.config.get_smac_preference_defaults(false); prefs.audio = state.audio; return state.values; },
            (values) => { prefs.p.glsmac.config.set_smac_preferences(values, prefs.audio); });
        let page = 0;
        for (data of this.pages) {
            const height = page == 2 ? 321 : (page == 6 || page == 4 ? 281 : 376);
            const group = window.panel({left: 256, top: 10, width: 457, height: height, overflow: 'hidden'});
            let top = 0;
            for (entry of data.rows) {
                const id = entry.id;
                const row = #include('../../smac_checkbox')(group, 0, top, 457, entry.label, false,
                    (checked) => { prefs.model.set(id, checked); });
                const record = {id: id}; record.control = row;
                this.rows :+ record; top += 24;
            }
            group.hide(); this.groups :+ group; page++;
        }
        let index = 0;
        while (index < 6) {
            const target = index;
            this.button(index, 33, 10 + 22 * index, this.words[index], () => { prefs.model.select(target); prefs.refresh(); }); index++;
        }
        this.button(6,33,164,this.words[6],() => { prefs.p.process_message('Gamma correction is not available yet.'); });
        this.button(90,33,314,this.words[7],() => { prefs.model.restore(); prefs.refresh(); });
        this.button(92,33,337,this.words[8],() => { prefs.finish(true); });
        this.button(91,33,360,this.words[9],() => { prefs.finish(false); });
        this.more = this.button(94,256,360,this.words[10],() => { prefs.model.more(); prefs.refresh(); });
        this.more.hide();
        window.on('keydown', (e) => { return prefs.key(e); });
    },
    button: (id,left,top,text,action) => {
        const class_name = 'smac-preferences-action-' + #to_string(id);
        this.skin(id, text);
        const button = this.window.button({class: class_name, left: left, top: top, width: 214, height: 20, text: text});
        button.on('click', () => { action(); return true; });
        const record = {id: id, top: top}; record.element = button;
        this.buttons :+ record; return button;
    },
    skin: (id,text) => {
        this.probe.text = text;
        const x = (214 - this.probe.measure().width) / 2;
        const selected = id < 6 && this.model.page == id;
        this.p.ui.class('smac-preferences-action-' + #to_string(id)).set({
            background: 'transparent', border: 'transparent', font: 'arial.ttf:12:mono',
            color: 'rgb(0,0,0)', textalign: 'top left', textleft: x, textbaseline: 14,
            smac_button_sprite: [selected ? 2 : 0,214],
            _hover: {smac_button_sprite: [selected ? 2 : 1,214]},
            _active: {smac_button_sprite: [2,214]}});
    },
    open: (page) => {
        if (!this.allowed()) { return false; }
        try {
            if (!#is_defined(this.window)) { this.create(); }
            if (!this.model.open(page)) { return false; }
            this.p.modules.menu.close_all(); this.active = true; this.focus = (0 - 1);
            this.resize(); this.refresh(); this.window.show();
        } catch { : (error) => {
            if (#is_defined(this.model) && this.model.active) { this.model.finish(false); }
            this.active = false;
            if (#is_defined(this.window)) { this.window.remove(); }
            this.window = undefined; this.model = undefined;
            this.p.process_message('Preferences unavailable: ' + error.reason);
        } }
        return true;
    },
    resize: () => {
        const small = this.p.ui.get_width() == 800;
        this.window.left = small ? 50 : this.p.ui.get_width() / 2 - 350;
        this.window.top = small ? 0 : this.p.ui.get_height() - 618;
        const file = small ? 'console_x.pcx' : 'console_x2.pcx';
        this.left_rail.background = file + ':smacsoftware():crop(' + #to_string(small ? 699 : 729) + ',1,' + #to_string(small ? 706 : 736) + ',390)';
        this.right_rail.background = file + ':smacsoftware():crop(' + #to_string(small ? 708 : 738) + ',1,' + #to_string(small ? 715 : 745) + ',390)';
        for (entry of this.buttons) { entry.element.top = entry.top - (small && entry.id >= 90 && entry.id <= 92 ? 50 : 0); }
        this.groups[4].height = small ? 201 : 281;
    },
    refresh: () => {
        for (row of this.rows) { const key = #to_string(row.id); row.control.set(this.model.values[key]); }
        let page = 0;
        for (group of this.groups) { if (page == this.model.page) { group.show(); } else { group.hide(); } page++; }
        this.more.text = this.words[this.model.page == 6 ? 11 : 10];
        this.paint_buttons();
        if (this.model.page == 2 || this.model.page == 6) { this.more.show(); } else { this.more.hide(); }
    },
    finish: (accept) => {
        if (!this.active) { return; }
        try { this.model.finish(accept); this.active = false; this.window.hide(); }
        catch { : (error) => { this.p.process_message(error.reason); } }
    },
    key: (e) => {
        if (!this.active) { return false; }
        if (e.modifiers == {} && e.code == 'ESCAPE') { this.finish(false); return true; }
        if (e.modifiers == {} && e.code == 'ENTER') { this.finish(true); return true; }
        if (e.code == 'TAB' && (e.modifiers == {} || e.modifiers == {shift: true})) {
            const count = #sizeof(this.buttons);
            this.focus = (this.focus + (e.modifiers == {} ? 1 : count - 1)) % count;
            while (!this.buttons[this.focus].element.is_visible()) {
                this.focus = (this.focus + (e.modifiers == {} ? 1 : count - 1)) % count;
            }
            return true;
        }
        if (this.model.key(e)) { this.refresh(); }
        return true;
    },
    paint_buttons: () => {
        for (entry of this.buttons) { this.skin(entry.id, entry.element.text); }
    },
};
