return {
    init: (p) => {
        this.p = p; this.active = false; this.pending = false; this.next_request = 0;
        this.window = p.ui.root.window({zindex: 0.9, align: 'top left', left: 216, top: 168, width: 623, height: 387, blocker: 'rgba(0, 0, 0, 180)'});
        this.window.surface({left: 0, top: 0, width: 623, height: 368, background: 'deswin.pcx:smacsoftware()'});
        this.components = this.window.surface({left: 0, top: 0, width: 623, height: 368});
        this.window.hide();
        const window = this.window;
        const caption = (left, top, width, height, baseline, size, bold) => {
            const frame = window.panel({left: left, top: top, width: width, height: height, overflow: 'hidden'});
            return frame.text({class: 'game-popup-text', align: 'top center', baseline: baseline, font: (bold ? 'arialbd.ttf:' : 'arial.ttf:') + #to_string(size) + ':mono', color: bold ? 'rgb(120,164,212)' : 'rgb(176,212,235)'});
        };
        const heading = (left, top, text) => { const label = caption(left, top, 99, 85, 11, 12, true); label.text = text; };
        heading(30, 7, 'CHASSIS'); heading(30, 101, 'WEAPON'); heading(30, 195, 'SHIELDING'); heading(462, 7, 'REACTOR');
        for (top of [101, 195]) {
            heading(462, top, 'SPECIAL'); const sub = caption(462, top, 99, 85, 25, 12, true); sub.text = top == 101 ? 'ABILITY 1' : 'ABILITY 2';
            const none = caption(462, top, 99, 85, 58, 10, false); none.text = 'NONE';
        }
        this.chassis = caption(30, 7, 99, 85, 73, 10, false);
        this.chassis_stat = caption(30, 7, 99, 85, 83, 10, false);
        this.weapon = caption(30, 101, 99, 85, 73, 10, false);
        this.weapon_stat = caption(30, 101, 99, 85, 83, 10, false);
        this.reactor = caption(462, 7, 99, 85, 70, 10, false);
        this.reactor_stat = caption(462, 7, 99, 85, 80, 10, false);
        this.armor_name = caption(30, 195, 99, 85, 73, 10, false);
        this.armor_stat = caption(30, 195, 99, 85, 83, 10, false);
        this.model = window.surface({left: 167, top: 36, width: 256, height: 256});
        this.title = caption(140, 9, 311, 311, 44, 16, true);
        this.stats = caption(140, 9, 311, 311, 293, 16, true);
        this.cost = caption(140, 9, 311, 311, 60, 12, true);
        this.message = caption(140, 260, 311, 40, 13, 12, true); this.message.text = '';
        this.draft_name = '';
        this.armor = this.window.button({left: 30, top: 195, width: 99, height: 85, background: 'transparent', overflow: 'hidden'});
        this.armor.on('click', () => { this.open_picker(); return true; });
        this.window.button({class: 'game-popup-button', left: 27, top: 321, width: 105, height: 20, font: 'arialbd.ttf:14:mono', text: 'Apply'}).on('click', () => { this.submit(false); return true; });
        this.window.button({class: 'game-popup-button', left: 27, top: 347, width: 105, height: 20, font: 'arialbd.ttf:14:mono', text: 'Done'}).on('click', () => { this.submit(true); return true; });
        this.window.button({class: 'game-popup-button', left: 459, top: 347, width: 105, height: 20, font: 'arialbd.ttf:14:mono', text: 'Cancel'}).on('click', () => { this.hide_view(); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.hide_view(); } if (e.modifiers == {} && e.code == 'ENTER') { this.submit(true); } return true; });
        this.picker = p.ui.root.window({zindex: 0.95, align: 'top left', left: 216, top: 168, width: 623, height: 387, background: 'rgb(0,0,0)', blocker: 'rgba(0, 0, 0, 180)'});
        this.picker_canvas = this.picker.surface({left: 0, top: 0, width: 623, height: 387});
        this.picker.hide(); this.cards = [];
        this.confirming = false;
        this.overwrite_window = p.ui.root.window({class: 'game-popup', zindex: 0.98, width: 440, height: 188, blocker: 'rgba(0, 0, 0, 180)'});
        this.overwrite_window.text({class: 'game-popup-text', top: 12, left: 12, text: 'Operations Director'});
        this.overwrite_body = this.overwrite_window.panel({top: 40, left: 12, right: 12, height: 96, overflow: 'hidden'}); this.overwrite_lines = [];
        this.overwrite_window.button({class: 'game-popup-button', align: 'bottom left', left: 12, width: 200, bottom: 12, height: 28, text: 'Never mind.'}).on('click', () => { this.answer_overwrite(false); return true; });
        this.overwrite_window.button({class: 'game-popup-button', align: 'bottom right', right: 12, width: 200, bottom: 12, height: 28, text: 'Proceed with redesign.'}).on('click', () => { this.answer_overwrite(true); return true; });
        this.overwrite_window.on('keydown', (e) => { if (e.modifiers == {} && (e.code == 'ESCAPE' || e.code == 'ENTER')) { this.answer_overwrite(false); } return true; });
        this.overwrite_window.hide();
        this.picker.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.picker.hide(); } return true; });
        this.window.listen(p.game, 'smac_workshop_commit', (e) => {
            if (!this.active || !this.pending || e.base.id != this.base.id || e.request_id != this.pending_id) { return; }
            this.pending = false; this.selection = e.selection;
            if (#is_defined(e.restore_draft) && e.restore_draft) { this.selected_design = e.design_id; this.selected_armor = e.restored_armor; this.draft_name = e.restored_name; }
            this.update_armor();
            if (e.stale) { this.message.text = 'Game state changed. Please check your design.'; return; }
            if (e.failed) { this.message.text = 'This design cannot be saved.'; return; }
            if (#is_defined(e.confirmation) && e.confirmation) {
                this.confirming = true; this.confirm_done = this.pending_done; this.confirm_expected = e.selection.expected_state;
                this.render_overwrite(e.overwrite_name, e.overwrite_count);
                this.overwrite_window.show(); return;
            }
            if (#is_defined(e.declined) && e.declined) {
                this.message.text = ''; return;
            }
            this.selected_design = e.design_id;
            if (e.intrinsic_name_warning) { this.draft_name = ''; this.message.text = 'The original unit name is retained.'; }
            else { this.message.text = ''; }
            if (e.done) { this.hide_view(); }
        });
        for (event of ['research_choice', 'colony_choice', 'colony_screen', 'turn', 'smac_ending', 'base_despawn']) {
            this.window.listen(p.game, event, (e) => { this.hide_view(); });
        }
        this.window.listen(p.game, 'turn_status', (e) => { if (e.status == 'please_wait') { this.hide_view(); } });
        return this.window;
    },
    open: (base, data) => {
        this.base = base; this.selection = data; this.active = true; this.pending = false; this.selected_armor = 0; this.selected_design = 2; this.confirming = false; this.overwrite_window.hide();
        this.chassis.text = #uppercase(data.chassis); this.weapon.text = #uppercase(data.weapon); this.reactor.text = #uppercase(data.reactor);
        this.chassis_stat.text = 'Land ' + #to_string(data.speed); this.weapon_stat.text = 'Weapon ' + #to_string(data.offense); this.reactor_stat.text = 'Power ' + #to_string(data.power);
        this.title.text = #uppercase(data.scout_name); this.draft_name = ''; this.message.text = ''; this.update_armor(); this.window.show();
    },
    armor_strength: (strength) => { return strength < 0 ? 'Psi' : #to_string(strength); },
    update_armor: () => {
        this.components.smacworkshopcomponents(this.selected_armor);
        for (row of this.selection.armor) { if (row.id == this.selected_armor) {
            this.model.smacworkshopdesign(0, 0, this.selected_armor, 1, (0 - 1), this.selection.faction_art, 0.0, this.selection.offense, this.selection.weapon_mode, row.rating);
            this.armor_name.text = row.id == 0 ? 'NONE' : #uppercase(row.short_name); this.armor_stat.text = 'Level ' + this.armor_strength(row.rating);
            if (row.design_name != '') { this.title.text = #uppercase(row.design_name); }
            this.cost.text = 'COST:  ' + #to_string(row.cost);
            this.stats.text = #to_string(this.selection.offense) + '-' + this.armor_strength(row.rating) + '-' + #to_string(this.selection.speed); } }
    },
    open_picker: () => {
        if (!this.active || this.pending || this.confirming) { return; }
        for (card of this.cards) { card.remove(); } this.cards = [];
        this.picker_canvas.smacworkshoparmorpicker(this.selected_armor, this.selection.armor);
        let i = 0;
        for (row of this.selection.armor) {
            const id = row.id; const available = row.available; const strength = this.armor_strength(row.rating);
            const card = this.picker.button({background: 'transparent', overflow: 'hidden', left: 27 + (i % 5) * 108, top: 4 + #floor(#to_float(i) / 5.0) * 94, width: 105, height: 91});
            if (row.technology_available) {
                const name = card.text({align: 'top center', baseline: 10, font: 'arial.ttf:10:mono', color: 'rgb(176,212,235)', text: ''});
                let lines = []; let line = '';
                for (word of #split(#uppercase(row.name), ' ')) {
                    const candidate = line == '' ? word : line + ' ' + word; name.text = candidate;
                    if (line != '' && name.measure().width > 99) { lines :+ line + ' '; line = word; }
                    else { line = candidate; }
                }
                lines :+ line; name.remove(); let baseline = 10;
                for (text of lines) { card.text({align: 'top center', baseline: baseline, font: 'arial.ttf:10:mono', color: 'rgb(176,212,235)', text: text}); baseline += 13; }
                card.text({align: 'top left', left: 3, baseline: 85, font: 'arial.ttf:10:mono', color: 'rgb(176,212,235)', text: 'Shielding'});
                card.text({align: 'top right', right: 3, baseline: 86, font: 'arialbd.ttf:12:mono', color: 'rgb(176,212,235)', text: strength});
            }
            card.on('click', () => { if (available) { this.selected_armor = id; this.update_armor(); this.picker.hide(); } return true; });
            this.cards :+ card; i++;
        }
        this.picker.show();
    },
    submit: (done) => {
        if (!this.active || this.pending || this.confirming) { return; }
        this.submit_choice(done, (0 - 1));
    },
    render_overwrite: (name, count) => {
        for (line of this.overwrite_lines) { line.remove(); } this.overwrite_lines = [];
        const text = 'We have no empty design slots. If we create this design, all units of our old "' + name + '" type (' + #to_string(count) + ' units) will be disbanded.';
        const body = this.overwrite_body;
        const probe = body.text({class: 'game-popup-text', left: 0, top: 0, font: 'arial.ttf:14:mono', text: ''});
        let lines = []; let line = '';
        for (word of #split(text, ' ')) {
            const candidate = line == '' ? word : line + ' ' + word; probe.text = candidate;
            if (line != '' && probe.measure().width > 410) { lines :+ line; line = word; }
            else { line = candidate; }
        }
        lines :+ line; probe.remove(); let top = 0;
        for (text of lines) { this.overwrite_lines :+ body.text({class: 'game-popup-text', left: 0, top: top, font: 'arial.ttf:14:mono', text: text}); top += 18; }
    },
    answer_overwrite: (accept) => {
        if (!this.active || this.pending || !this.confirming) { return; }
        this.confirming = false; this.overwrite_window.hide(); this.submit_choice(this.confirm_done, accept ? 1 : 0);
    },
    submit_choice: (done, choice) => {
        if (!this.active || this.pending) { return; }
        this.pending_done = done;
        this.pending = true; this.next_request = this.next_request + 1; this.pending_id = this.next_request;
        const request = this.pending_id;
        const base = this.base; const expected = choice < 0 ? this.selection.expected_state : this.confirm_expected;
        const armor = this.selected_armor; const name = this.draft_name; const selected = this.selected_design;
        let fields = {base: base, selected_design: selected, expected_state: expected, armor: armor, name: name, done: done, request_id: request};
        if (choice >= 0) { fields = fields + {overwrite: choice}; }
        this.p.game.event('commit_smac_workshop', fields);
    },
    hide_view: () => { this.active = false; this.pending = false; this.base = null; this.confirming = false; this.overwrite_window.hide(); this.picker.hide(); this.window.hide(); },
};
