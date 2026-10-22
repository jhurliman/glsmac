return {
    init: (p) => {
        this.p = p; this.opening = false; this.active = false; this.pending = false; this.next_request = 0;
        this.picker_active = false; this.picker_input = #include('./workshop_picker_input');
        this.data_formatter = #include('./workshop_data');
        this.automatic_suggestions = true; // Original fresh preference-store default.
        this.preview_position = [0,0,0];
        this.ability_slots = [(0 - 1), (0 - 1)];
        this.ability_dialog = #include('./workshop_abilities'); this.ability_dialog.init(p);
        this.rename_dialog = #include('./workshop_rename'); this.rename_dialog.init(p);
        this.retire_dialog = #include('./workshop_retire'); this.retire_dialog.init(p);
        this.upgrade_dialog = #include('./workshop_upgrade'); this.upgrade_dialog.init(p);
        this.window = p.ui.root.window({zindex: 0.9, align: 'top left', left: 216, top: 168, width: 623, height: 387, blocker: 'rgba(0, 0, 0, 180)'});
        this.window.surface({left: 0, top: 0, width: 623, height: 368, background: 'deswin.pcx:smacsoftware()'});
        this.components = this.window.surface({left: 0, top: 0, width: 623, height: 368});
        this.component_models = [];
        for (placement of [{kind: 0, left: 30, top: 7, offset: 88}, {kind: 1, left: 30, top: 101, offset: 86}, {kind: 3, left: 462, top: 7, offset: 90}]) {
            const frame = this.window.panel({left: placement.left, top: placement.top, width: 99, height: 85, overflow: 'hidden'});
            this.component_models :+ {kind: placement.kind, model: frame.surface({left: (0 - 79), top: (0 - placement.offset), width: 256, height: 256})};
        }
        this.window.hide();
        const window = this.window; this.centered_captions = [];
        const caption = (left, top, width, height, baseline, size, bold) => {
            const frame = window.panel({left: left, top: top, width: width, height: height, overflow: 'hidden'});
            const label = frame.text({class: 'game-popup-text', align: 'top left', text: '', baseline: baseline, font: (bold ? 'arialbd.ttf:' : 'arial.ttf:') + #to_string(size) + ':mono', color: bold ? 'rgb(120,164,212)' : 'rgb(176,212,235)'});
            this.centered_captions :+ {label: label, width: width, color: bold ? 224 : 222}; return label;
        };
        const heading = (left, top, text) => { const label = caption(left, top, 99, 85, 11, 12, true); label.text = text; return label; };
        this.chassis_heading = heading(30, 7, ''); this.weapon_heading = heading(30, 101, ''); this.armor_heading = heading(30, 195, ''); this.reactor_heading = heading(462, 7, '');
        this.ability_labels = []; this.ability_frames = []; this.ability_body = []; this.ability_headings = [];
        for (top of [101, 195]) {
            const none = caption(462, top, 99, 85, 61, 10, false); none.text = 'NONE';
            this.ability_labels :+ none;
            this.ability_frames :+ window.panel({left: 462, top: top, width: 99, height: 85, overflow: 'hidden'});
        }
        this.ability_probe = window.text({font: 'arial.ttf:10:mono', text: ''}); this.ability_probe.hide();
        this.name_frames = []; this.name_body = [];
        for (placement of [{left: 30, top: 7}, {left: 30, top: 101}, {left: 30, top: 195}, {left: 462, top: 7}]) {
            this.name_frames :+ window.panel({left: placement.left, top: placement.top, width: 99, height: 85, overflow: 'hidden'});
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
        this.model.on('workshopframe', () => { if (this.active) { this.paint_main_preview(); } return true; });
        this.title = caption(140, 9, 311, 311, 44, 16, true);
        // Native layout rectangle is y256..272; clipping remains the whole
        // preview, so glyph descenders below that layout rectangle survive.
        this.stats = caption(140, 9, 311, 311, 261, 16, true);
        this.cost = caption(140, 9, 311, 311, 60, 12, true);
        this.message = caption(140, 260, 311, 40, 13, 12, true); this.message.text = '';
        const data_frame = window.panel({left: 140, top: 9, width: 311, height: 311, overflow: 'hidden'});
        this.data_heading = data_frame.text({left: 20, baseline: 108, font: 'arialbd.ttf:12:mono', color: 'rgb(120,164,212)', text: ''});
        this.data_rows = []; let data_index = 0;
        while (data_index < 6) { this.data_rows :+ data_frame.text({left: 20, baseline: 122 + 14 * data_index, font: 'arialbd.ttf:12:mono', color: 'rgb(120,164,212)', text: ''}); data_index++; }
        this.draft_name = '';
        this.armor = this.window.button({left: 30, top: 195, width: 99, height: 85, background: 'transparent', overflow: 'hidden'});
        this.armor.on('click', () => { this.open_picker(); return true; });
        this.chassis_button = this.window.button({left: 30, top: 7, width: 99, height: 85, background: 'transparent'});
        this.chassis_button.on('click', () => { this.open_component_picker(0); return true; });
        this.reactor_button = this.window.button({left: 462, top: 7, width: 99, height: 85, background: 'transparent'});
        this.reactor_button.on('click', () => { this.open_component_picker(3); return true; });
        this.weapon_button = this.window.button({left: 30, top: 101, width: 99, height: 85, background: 'transparent'});
        this.weapon_button.on('click', () => { this.open_component_picker(1); return true; });
        this.ability_buttons = [];
        const workshop = this;
        for (slot of [0, 1]) {
            const index = slot;
            const button = this.window.button({left: 462, top: 101 + 94 * slot, width: 99, height: 85, background: 'transparent'});
            button.on('click', () => { this.ability_dialog.open(workshop, index); return true; }); this.ability_buttons :+ button;
        }
        window.surface({left: 0, top: 0, width: 623, height: 387, smac_frame: [[3,24,344,567,370]]});
        p.ui.class('workshop-main-action').set({font: 'arialbd.ttf:14:mono', color: 'rgb(0,0,0)', textalign: 'top left', textbaseline: 15,
            smac_frame: [[27,0,0,105,20]], _hover: {smac_frame: [[29,0,0,105,20]]}, _active: {smac_frame: [[29,0,0,105,20]]}});
        this.action_probe = window.text({font: 'arialbd.ttf:14:mono', text: ''}); this.action_probe.hide();
        this.obsolete_button = window.button({class: 'workshop-main-action', left: 243, top: 347, width: 105, height: 20, text: ''});
        this.obsolete_button.on('click', () => { this.obsolete(); return true; });
        this.rename_button = window.button({class: 'workshop-main-action', left: 135, top: 347, width: 105, height: 20, text: ''});
        this.rename_button.on('click', () => { workshop.rename_dialog.begin(workshop); return true; });
        this.retire_button = window.button({class: 'workshop-main-action', left: 459, top: 321, width: 105, height: 20, text: ''});
        this.retire_button.on('click', () => { workshop.retire_dialog.begin(workshop); return true; });
        this.upgrade_button = window.button({class: 'workshop-main-action', left: 351, top: 347, width: 105, height: 20, text: ''});
        this.upgrade_button.on('click', () => { workshop.upgrade_dialog.begin(workshop); return true; });
        this.apply_button = this.window.button({class: 'workshop-main-action', left: 27, top: 321, width: 105, height: 20, text: ''});
        this.apply_button.on('click', () => { this.submit(false); return true; });
        this.done_button = this.window.button({class: 'workshop-main-action', left: 27, top: 347, width: 105, height: 20, text: ''});
        this.done_button.on('click', () => { this.submit(true); return true; });
        this.cancel_button = this.window.button({class: 'workshop-main-action', left: 459, top: 347, width: 105, height: 20, text: ''});
        this.cancel_button.on('click', () => { this.close_view(); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.close_view(); } if (e.modifiers == {} && e.code == 'ENTER') { this.submit(true); } return true; });
        this.picker = p.ui.root.window({zindex: 0.95, align: 'top left', left: 216, top: 168, width: 623, height: 387, background: 'rgb(0,0,0)', blocker: 'rgba(0, 0, 0, 180)'});
        this.picker_canvas = this.picker.surface({left: 0, top: 0, width: 623, height: 387});
        this.picker_scroll = this.picker.scrollbar({class: 'default-scroll-v', left: 580, top: 21, width: 24, height: 246,
            fromto_size: 24, slider_size: 24, min: 0.0, max: 2.0, value: 0.0});
        this.picker_scroll.left = 580; this.picker_scroll.width = 24; this.picker_scroll.top = 21; this.picker_scroll.height = 246;
        this.picker_scroll.on('change', (e) => {
            if (this.picker_active) { this.picker_state.position = #floor(e.value + 0.5); this.picker_state.first = 5 * this.picker_state.position;
                this.picker_first = this.picker_state.first; this.paint_picker(); } return true; });
        this.picker_scroll.hide();
        this.picker.hide(); this.cards = [];
        this.confirming = false; this.intrinsic_message = false;
        this.redesign_dialog = #include('./workshop_redesign'); this.redesign_dialog.init(p);
        this.overwrite_window = this.redesign_dialog.window;
        this.picker.on('keydown', (e) => { this.picker_key(e); return true; });
        this.picker_canvas.on('mousemove', (e) => { this.picker_pointer(e, false); return true; });
        this.picker_canvas.on('mouseup', (e) => { if (e.button == 'left') { this.picker_pointer(e, true); } return true; });
        this.window.listen(p.game, 'smac_workshop_open', (e) => {
            if (!this.opening || e.base.id != this.base.id || e.request_id != this.opening_id) { return; }
            this.opening = false;
            const success = !e.stale && !e.failed;
            if (#is_defined(this.p.utils) && #is_defined(this.p.utils.workshop_open_result)) {
                this.p.utils.workshop_open_result(success, e.stale ? 'Game state changed. Please reopen Workshop.' : 'Workshop inputs are unavailable for this save.');
            }
            if (success) { this.open(this.base, e.selection); }
        });
        this.window.listen(p.game, 'smac_workshop_commit', (e) => {
            if (!this.active || !this.pending || e.base.id != this.base.id || e.request_id != this.pending_id) { return; }
            const selecting = this.pending_selection;
            this.pending = false; this.selection = e.selection;
            if (selecting && e.stale) {
                const base = this.base; const selected = this.selected_design; const chassis = this.selected_chassis; const weapon = this.selected_weapon;
                const armor = this.selected_armor; const reactor = this.selected_reactor; const abilities = this.selected_abilities; const name = this.draft_name;
                this.selection = base.get_smac_workshop(selected, chassis, weapon, armor, reactor, abilities, name);
            }
            if (#is_defined(e.restore_draft) && e.restore_draft) {
                this.selected_design = e.design_id; this.selected_armor = e.restored_armor; this.draft_name = e.restored_name;
                if (#is_defined(e.selection.draft)) { this.read_draft(e.selection.draft); }
                if (!selecting) { this.recenter_strip(e.design_id); }
            }
            this.update_labels(); this.update_armor();
            this.render_strip();
            if (e.stale) { this.message.text = 'Game state changed. Please check your design.'; return; }
            if (#is_defined(e.intrinsic_notice) && e.intrinsic_notice > 0) {
                this.message.text = '';
                if (e.intrinsic_notice == 3) { return; }
                this.confirming = true; this.intrinsic_message = true; this.notice_done = e.done;
                if (e.intrinsic_name_warning) { this.draft_name = ''; }
                this.redesign_dialog.open(this, e.intrinsic_notice_name, 0, e.intrinsic_notice); return;
            }
            if (e.failed) { this.message.text = 'This design cannot be saved.'; return; }
            if (#is_defined(e.confirmation) && e.confirmation) {
                this.confirming = true; this.confirm_done = this.pending_done; this.confirm_expected = e.selection.expected_state;
                this.render_overwrite(e.overwrite_name, e.overwrite_count);
                this.overwrite_window.show(); return;
            }
            if (#is_defined(e.declined) && e.declined) {
                this.message.text = ''; return;
            }
            if (!#is_defined(this.selection.status)) { this.selected_design = e.design_id; }
            if (e.intrinsic_name_warning) { this.draft_name = ''; this.message.text = 'The original unit name is retained.'; }
            else { this.message.text = ''; }
            if (e.done) { this.close_view(); }
        });
        this.window.listen(p.game, 'smac_workshop_obsolete', (e) => {
            if (!this.active || !this.pending || !this.pending_obsolete || e.base.id != this.base.id || e.request_id != this.pending_id) { return; }
            this.pending = false; this.pending_obsolete = false; this.selection = e.selection;
            if (#is_defined(e.selection.draft)) { this.read_draft(e.selection.draft); this.selected_design = e.selection.draft.selected_design; }
            if (!e.stale && !e.failed) { this.strip_offset = e.strip_offset; }
            else { this.recenter_strip(this.selected_design); }
            this.strip_hover = (0 - 1); this.update_labels(); this.update_armor(); this.render_strip();
            this.message.text = e.stale ? 'Game state changed. Please check your design.' : (e.failed ? 'This design cannot be made obsolete.' : '');
        });
        this.window.listen(p.game, 'smac_workshop_preset', (e) => {
            if (!this.active || !this.pending || !this.pending_preset || e.base.id != this.base.id || e.request_id != this.pending_id) { return; }
            this.pending = false; this.pending_preset = false;
            if (e.stale || e.failed) {
                this.message.text = e.stale ? 'Game state changed. Please check your design.' : 'Preset inputs are not available.';
                if (e.stale) {
                    const base = this.base; const selected = this.selected_design; const chassis = this.selected_chassis; const weapon = this.selected_weapon;
                    const armor = this.selected_armor; const reactor = this.selected_reactor; const abilities = this.selected_abilities; const name = this.draft_name;
                    this.selection = base.get_smac_workshop(selected, chassis, weapon, armor, reactor, abilities, name);
                }
                return;
            }
            this.selection = e.selection; this.read_draft(e.selection.draft); this.selected_design = e.selection.draft.selected_design;
            if (e.matched) { this.finish_preset(e.feedback); }
            else { this.preset_notice.open(this); }
        });
        this.window.listen(p.game, 'smac_workshop_rename', (e) => { this.rename_dialog.reply(e); });
        this.window.listen(p.game, 'smac_workshop_retire', (e) => { this.retire_dialog.reply(e); });
        this.window.listen(p.game, 'smac_workshop_upgrade', (e) => { this.upgrade_dialog.reply(e); });
        for (event of ['research_choice', 'colony_choice', 'colony_screen', 'turn', 'smac_ending', 'base_despawn']) {
            this.window.listen(p.game, event, (e) => { this.hide_view(); });
        }
        this.window.listen(p.game, 'turn_status', (e) => { if (e.status == 'please_wait') { this.hide_view(); } });
        // The keyed console shell leaves the parent action row visible.
        // Its layer also keeps the console above the parent's modal dimmer.
        this.console = p.ui.root.window({zindex: 0.905, align: 'top left', left: 0, top: 511, width: 1024, height: 257});
        this.console_backing = this.console.surface({left: 0, top: 50, width: 1024, height: 207});
        this.console.surface({left: 0, top: 0, width: 1024, height: 257, background: 'console2.pcx:smacsoftware()'});
        this.console.surface({left: 0, top: 0, width: 1024, height: 257, smac_frame: [[3,7,59,242,189],[0,10,62,239,186],[3,266,70,759,168],[0,269,73,756,165]]});
        this.console.surface({left: 0, top: 0, width: 1024, height: 257, smac_frame: [[3,783,60,1016,248],[6,786,63,1013,84]]});
        this.presets_heading = this.console.text({left: 786, baseline: 79, font: 'arialbd.ttf:14:mono', text: ''});
        p.ui.class('workshop-preset-action').set({font: 'arialbd.ttf:14:mono', textalign: 'top left', textbaseline: 16,
            smac_frame: [[27,0,0,112,21]], _hover: {smac_frame: [[29,0,0,112,21]]}, _active: {smac_frame: [[29,0,0,112,21]]}});
        this.preset_buttons = []; let preset = 0;
        while (preset < 14) {
            const action = preset + 9;
            const button = this.console.button({class: 'workshop-preset-action', left: 786 + (preset / 7) * 115, top: 87 + (preset % 7) * 23, width: 112, height: 21});
            button.on('click', () => { this.preset(action); return true; }); this.preset_buttons :+ button; preset++;
        }
        this.status_frame = this.console.panel({left: 272, top: 76, width: 481, height: 86, overflow: 'hidden'});
        this.status_title = this.status_frame.text({align: 'top left', baseline: 18, font: 'arialbd.ttf:18:mono', text: ''});
        this.status_lines = []; this.console.hide();
        this.strip = p.ui.root.window({zindex: 0.91, align: 'top left', left: 8, top: 703, width: 756, height: 56, background: 'black'});
        this.strip_canvas = this.strip.surface({left: 0, top: 0, width: 756, height: 56});
        this.strip_cards = []; this.strip_offset = 0; this.strip_hover = (0 - 1); this.strip.hide();
        p.ui.class('workshop-strip-left').set({
            background: 'interface.pcx:smacsoftware():crop(397,153,408,194)', _hover: {background: 'interface.pcx:smacsoftware():crop(410,153,421,194)'}, _active: {background: 'interface.pcx:smacsoftware():crop(423,153,434,194)'}});
        p.ui.class('workshop-strip-right').set({
            background: 'interface.pcx:smacsoftware():crop(397,196,408,237)', _hover: {background: 'interface.pcx:smacsoftware():crop(410,196,421,237)'}, _active: {background: 'interface.pcx:smacsoftware():crop(423,196,434,237)'}});
        this.strip_left = this.strip.button({class: 'workshop-strip-left', left: 4, top: 7, width: 12, height: 42});
        this.strip_right = this.strip.button({class: 'workshop-strip-right', left: 740, top: 7, width: 12, height: 42});
        this.strip_left.on('click', () => { this.scroll_strip(false); return true; });
        this.strip_right.on('click', () => { this.scroll_strip(true); return true; });
        this.preset_notice = #include('./workshop_preset_notice'); this.preset_notice.init(p);
        return this.window;
    },
    set_action_caption: (button, caption) => {
        const text = #uppercase(caption); this.action_probe.text = text;
        // Native main controls truncate the centered advance to an integer.
        button.text = text; button.textleft = 3 + (99 - this.action_probe.measure().width) / 2;
        button.color = this.parent_text.colors[65];
    },
    begin: (base, data, selected) => {
        if (this.opening || this.active || !#is_defined(data.preset_state)) { return; }
        if (#is_defined(this.p.config)) { this.automatic_suggestions = this.p.config.get_automatic_unit_designs(); }
        this.base = base; this.opening = true; this.next_request = this.next_request + 1; this.opening_id = this.next_request;
        const request = this.opening_id; const expected = data.preset_state;
        this.p.game.event('open_smac_workshop', {base: base, selected_design: selected, request_id: request, expected_state: expected});
    },
    open: (base, data) => {
        if (!this.active) { this.model.smacworkshopclock(2); this.preview_position = [0,0,0]; }
        if (!#is_defined(this.parent_text)) { this.parent_text = this.components.smacworkshopparenttext(); }
        for (entry of this.centered_captions) { entry.label.color = this.parent_text.colors[entry.color]; }
        this.presets_heading.text = #uppercase(this.parent_text.presets_title); this.presets_heading.color = this.parent_text.colors[240];
        this.presets_heading.left = 786 + (227 - this.presets_heading.measure().width) / 2;
        let preset_index = 0;
        for (button of this.preset_buttons) {
            const text = #uppercase(this.parent_text.presets[preset_index]); this.action_probe.text = text;
            button.text = text; button.textleft = (112 - this.action_probe.measure().width) / 2; button.color = this.parent_text.colors[65]; preset_index++;
        }
        this.pending_preset = false;
        this.mineral_factor = data.mineral_factor;
        this.base = base; this.selection = data; this.active = true; this.pending = false; this.pending_obsolete = false; this.pending_rename = false; this.selected_armor = 0; this.selected_design = 2; this.confirming = false; this.intrinsic_message = false; this.redesign_dialog.hide();
        this.set_action_caption(this.apply_button, this.parent_text.apply);
        this.set_action_caption(this.done_button, this.parent_text.done);
        this.set_action_caption(this.cancel_button, this.parent_text.cancel);
        this.set_action_caption(this.obsolete_button, this.parent_text.obsolete);
        this.set_action_caption(this.rename_button, this.parent_text.rename);
        this.pending_upgrade = false; this.set_action_caption(this.upgrade_button, this.parent_text.upgrade);
        this.pending_retire = false; this.set_action_caption(this.retire_button, this.parent_text.retire);
        this.selected_chassis = 0; this.selected_weapon = 0; this.selected_reactor = 1; this.selected_abilities = 0; this.draft_name = '';
        this.ability_slots = [(0 - 1), (0 - 1)];
        if (#is_defined(data.draft)) { this.read_draft(data.draft); this.selected_design = data.draft.selected_design; }
        this.strip_hover = (0 - 1); this.recenter_strip(this.selected_design);
        this.update_labels(); this.title.text = #uppercase(data.scout_name); this.message.text = ''; this.update_armor(); this.window.show(); this.render_strip(); this.model.smacworkshopclock(1);
    },
    render_strip: () => {
        for (card of this.strip_cards) { card.remove(); } this.strip_cards = [];
        if (!this.active || !#is_defined(this.selection.designs) || #sizeof(this.selection.designs) == 0) { this.strip.hide(); return; }
        const count = #sizeof(this.selection.designs);
        let rows = []; let scanned = 0;
        while (scanned < count && #sizeof(rows) < 10) { rows :+ this.selection.designs[(this.strip_offset + scanned) % count] + {}; scanned++; }
        this.visible_designs = rows;
        const colors = this.paint_strip();
        let column = 0;
        for (row of rows) {
            const id = row.id;
            const color = row.excluded && row.id >= 64 ? (#is_defined(row.unit_count) && row.unit_count == 0 ? colors.unused : colors.obsolete) : colors.normal;
            const card = this.strip.button({left: 19 + 72 * column, top: 3, width: 70, height: 50, background: 'transparent', overflow: 'hidden'});
            const title_box = card.panel({left: 5, top: 2, width: 60, height: 13, overflow: 'hidden'});
            const title = title_box.text({align: 'top center', baseline: 10, font: 'arial.ttf:10:mono', color: color, text: ''});
            const slot = row.id % 64 + 1;
            if (row.active) {
                let caption = row.id < 64 ? '* ' + row.stored_name : (row.excluded ? colors.obsolete_label + ' (' + #to_string(slot) + ')' : #to_string(slot) + '. ' + row.stored_name);
                title.text = caption; let limit = 64;
                while (title.measure().width > 60 && limit > 0) { limit--; title.text = ('' + caption)[0::limit]; }
                const words = #split(title.text, ' '); if (#sizeof(words) > 2) { title.text = words[0] + ' ' + words[1]; }
                const stats_box = card.panel({left: 2, top: 35, width: 66, height: 13, overflow: 'hidden'});
                const components = row.components;
                stats_box.text({align: 'top center', baseline: 10, font: 'arial.ttf:10:mono', color: color, text: components.stat_line});
            } else {
                title.text = colors.empty_label;
                const slot_box = card.panel({left: 2, top: 15, width: 66, height: 13, overflow: 'hidden'});
                slot_box.text({align: 'top center', baseline: 10, font: 'arial.ttf:10:mono', color: color, text: '#' + #to_string(slot)});
            }
            card.on('click', () => { this.select_design(id); return true; }); this.strip_cards :+ card; column++;
            card.on('mouseover', () => { if (this.active) { this.strip_hover = id; this.paint_strip(); } return true; });
            card.on('mouseout', () => { if (this.active && this.strip_hover == id) { this.strip_hover = (0 - 1); this.paint_strip(); } return true; });
        }
        this.strip.show();
    },
    paint_strip: () => {
        const rows = this.visible_designs; const art = this.selection.faction_art; const selected = this.selected_design; const hovered = this.strip_hover;
        return this.strip_canvas.smacworkshopstrip(rows, art, selected, hovered);
    },
    recenter_strip: (target) => {
        if (#is_defined(this.selection.designs) && #sizeof(this.selection.designs) >= 5) {
            const count = #sizeof(this.selection.designs); let index = 0;
            while (index < count && this.selection.designs[index].id != target) { index++; }
            if (index < count) { this.strip_offset = (index + count - 5) % count; }
        }
    },
    scroll_strip: (right) => {
        if (!this.active || this.pending || this.confirming || !#is_defined(this.selection.designs)) { return; }
        const count = #sizeof(this.selection.designs); if (count == 0) { return; }
        if (right) { this.strip_offset = this.strip_offset + 1; if (this.strip_offset == count) { this.strip_offset = 0; } }
        else { this.strip_offset = this.strip_offset - 1; if (this.strip_offset < 0) { this.strip_offset = count; } }
        this.render_strip();
    },
    select_design: (id) => {
        if (!this.active || this.pending || this.confirming) { return; }
        this.pending_done = false; this.pending_selection = true; this.pending = true; this.next_request = this.next_request + 1; this.pending_id = this.next_request;
        const request = this.pending_id; const base = this.base; const expected = this.selection.expected_state; const name = this.draft_name;
        this.p.game.event('select_smac_workshop', {base: base, selected_design: id, expected_state: expected, name: name, request_id: request});
    },
    obsolete: () => {
        if (!this.active || this.pending || this.confirming || this.picker_active || this.ability_dialog.active) { return; }
        this.pending = true; this.pending_obsolete = true; this.next_request = this.next_request + 1; this.pending_id = this.next_request;
        const base = this.base; const selected = this.selected_design; const name = this.draft_name;
        const offset = this.strip_offset; const expected = this.selection.expected_state; const request = this.pending_id;
        let list = []; if (#is_defined(this.selection.designs)) { for (row of this.selection.designs) { list :+ row.id; } }
        this.p.game.event('obsolete_smac_workshop', {base: base, selected_design: selected, name: name, old_list: list, strip_offset: offset, expected_state: expected, request_id: request});
    },
    finish_preset: (feedback) => {
        // Original stock sound-bank slots39/43 are unbound. Retain the request
        // boundary and ordering without substituting another sound sample.
        this.last_preset_feedback = feedback;
        this.recenter_strip(this.selected_design); this.update_labels(); this.update_armor(); this.render_strip(); this.message.text = '';
    },
    preset: (action) => {
        if (!this.active || this.pending || this.confirming || this.picker_active || this.ability_dialog.active || this.preset_notice.active || !#is_defined(this.selection.preset_state)) { return; }
        this.pending = true; this.pending_preset = true; this.next_request = this.next_request + 1; this.pending_id = this.next_request;
        const base = this.base; const selected = this.selected_design; const chassis = this.selected_chassis; const weapon = this.selected_weapon;
        const armor = this.selected_armor; const reactor = this.selected_reactor; const abilities = this.selected_abilities; const name = this.draft_name;
        const expected = this.selection.preset_state; const request = this.pending_id;
        this.p.game.event('preset_smac_workshop', {base: base, selected_design: selected, chassis: chassis, weapon: weapon, armor: armor, reactor: reactor,
            abilities: abilities, name: name, action: action, request_id: request, expected_state: expected});
    },
    read_draft: (draft) => {
        this.selected_chassis = draft.chassis; this.selected_weapon = draft.weapon; this.selected_reactor = draft.reactor; this.selected_abilities = draft.abilities;
        this.selected_armor = draft.armor; this.draft_name = draft.name;
        let mask = draft.abilities; let bit = 0; let slots = [];
        while (bit < 24) { if (mask % 2 == 1 && #sizeof(slots) < 2) { slots :+ bit; } mask = #floor(#to_float(mask) / 2.0); bit++; }
        while (#sizeof(slots) < 2) { slots :+ (0 - 1); }
        this.ability_slots = slots;
        if (#is_defined(this.selection.ability_slots)) { this.ability_slots = this.selection.ability_slots; }
    },
    wrap_card_text: (text, width, font) => {
        this.ability_probe.font = font; let lines = []; let current = ''; let index = 0; const words = #split(text, ' ');
        for (word of words) {
            const token = word + (index < #sizeof(words) - 1 ? ' ' : '');
            this.ability_probe.text = current + token;
            if (current != '' && this.ability_probe.measure().width > width) { lines :+ current; current = ''; }
            current = current + token; index++;
        }
        if (current != '') { lines :+ current; }
        return lines;
    },
    paint_component_name: (label, frame, text, bottom, reactor) => {
        label.text = text; this.ability_probe.font = 'arial.ttf:10:mono'; this.ability_probe.text = text;
        const wrapped = this.ability_probe.measure().width > 99;
        if (!reactor && !wrapped) { label.show(); return 1; }
        label.hide(); const lines = this.wrap_card_text(text, 99, 'arial.ttf:10:mono');
        let baseline = reactor ? bottom - 10 * (#sizeof(lines) - 1) : bottom - 10;
        const color = this.parent_text.colors[222];
        for (line of lines) {
            this.ability_probe.text = line; const left = #floor(#to_float(99 - this.ability_probe.measure().width) / 2.0);
            this.name_body :+ frame.text({left: left, baseline: baseline, font: 'arial.ttf:10:mono', color: color, text: line}); baseline += 10;
        }
        return #sizeof(lines);
    },
    paint_component_names: () => {
        for (node of this.name_body) { node.remove(); } this.name_body = [];
        this.paint_component_name(this.chassis, this.name_frames[0], #uppercase(this.selection.chassis), 73, false);
        this.paint_component_name(this.weapon, this.name_frames[1], #uppercase(this.selection.weapon), 73, false);
        this.paint_component_name(this.armor_name, this.name_frames[2], this.armor_name.text, 73, false);
        const lines = this.paint_component_name(this.reactor, this.name_frames[3], #uppercase(this.selection.reactor), 70, true);
        for (component of this.component_models) { if (component.kind == 3) { component.model.top = (0 - 90) - 5 * (lines - 1); } }
    },
    paint_ability_cards: () => {
        for (label of this.ability_body) { label.remove(); } this.ability_body = [];
        for (label of this.ability_headings) { label.remove(); } this.ability_headings = [];
        const colors = this.parent_text.colors; let slot = 0;
        for (none of this.ability_labels) {
            const bit = this.ability_slots[slot];
            const frame = this.ability_frames[slot];
            const headings = this.wrap_card_text(#uppercase(this.parent_text.special_ability + ' ' + #to_string(slot + 1)), 89, 'arialbd.ttf:12:mono');
            let heading_baseline = 11;
            for (line of headings) {
                this.ability_probe.text = line; const left = #floor(#to_float(99 - this.ability_probe.measure().width) / 2.0);
                this.ability_headings :+ frame.text({left: left, baseline: heading_baseline, font: 'arialbd.ttf:12:mono', color: colors[224], text: line}); heading_baseline += 14;
            }
            this.ability_probe.font = 'arial.ttf:10:mono';
            if (bit < 0) { none.text = #uppercase(this.parent_text.none); none.baseline = 47 + 7 * #sizeof(headings); none.color = colors[224]; none.show(); }
            else {
                none.hide();
                const tokens = this.components.smacworkshopabilitycardtext(this.selection.ability_names[bit], this.selection.ability_descriptions[bit]);
                let lines = []; let line = {text: '', spans: []};
                for (token of tokens) {
                    this.ability_probe.text = line.text + token.text;
                    if (line.text != '' && this.ability_probe.measure().width > 99) { lines :+ line; line = {text: '', spans: []}; }
                    line.text = line.text + token.text;
                    for (span of token.spans) { line.spans :+ span; }
                }
                if (line.text != '') { lines :+ line; }
                let baseline = 82 - 10 * (#sizeof(lines) - 1);
                for (row of lines) {
                    this.ability_probe.text = row.text;
                    const left = #floor(#to_float(99 - this.ability_probe.measure().width) / 2.0); let prefix = '';
                    for (span of row.spans) {
                        this.ability_probe.text = prefix; const x = left + this.ability_probe.measure().width;
                        const label = frame.text({left: x, baseline: baseline, font: 'arial.ttf:10:mono', color: colors[span.color], text: span.text});
                        this.ability_body :+ label; prefix = prefix + span.text;
                    }
                    baseline += 10;
                }
            }
            slot++;
        }
    },
    layout_captions: () => {
        for (entry of this.centered_captions) { entry.label.left = #floor(#to_float(entry.width - entry.label.measure().width) / 2.0); }
    },
    update_labels: () => {
        this.update_status();
        this.chassis.text = #uppercase(this.selection.chassis); this.weapon.text = #uppercase(this.selection.weapon); this.reactor.text = #uppercase(this.selection.reactor);
        const equipment = this.selection.offense == 0;
        const label = equipment ? this.parent_text.equipment : this.parent_text.weapon;
        this.weapon_heading.text = #uppercase(label);
        this.chassis_heading.text = #uppercase(this.parent_text.chassis); this.armor_heading.text = #uppercase(this.parent_text.shielding); this.reactor_heading.text = #uppercase(this.parent_text.reactor);

        this.chassis_stat.text = this.parent_text.domains[0] + ' ' + #to_string(this.selection.speed); this.weapon_stat.text = label + ' ' + #to_string(equipment ? this.selection.weapon_cost : this.selection.offense); this.reactor_stat.text = this.parent_text.power + ' ' + #to_string(this.selection.power);
        if (#is_defined(this.selection.chassis_choices)) { for (row of this.selection.chassis_choices) { if (row.id == this.selected_chassis) {
            this.chassis_stat.text = this.parent_text.domains[row.domain] + ' ' + #to_string(row.value);
        } } }
        this.paint_ability_cards();
        this.chassis_stat.color = this.parent_text.colors[224]; this.weapon_stat.color = this.parent_text.colors[224];
        this.armor_stat.color = this.parent_text.colors[224]; this.reactor_stat.color = this.parent_text.colors[224];
        this.layout_captions();
    },
    update_status: () => {
        if (!#is_defined(this.selection.status)) { this.console.hide(); return; }
        const previous = this.selected_design;
        this.selected_design = this.selection.draft.selected_design;
        const text = this.parent_text; const status = this.selection.status;
        this.console_backing.smacworkshopconsolebacking();
        let message = text.status_labels[status];
        if (status == 3) { message += ' (' + text.slot + ' #' + #to_string(this.selected_design % 64 + 1) + ')'; }
        message = #uppercase(message);
        this.status_title.text = #uppercase(text.status_title); this.status_title.color = text.colors[224];
        this.status_title.left = (481 - this.status_title.measure().width) / 2;
        for (line of this.status_lines) { line.remove(); } this.status_lines = [];
        const frame = this.status_frame;
        const probe = frame.text({font: 'arialbd.ttf:18:mono', text: ''});
        let lines = []; let line = '';
        for (word of #split(message, ' ')) {
            const candidate = line == '' ? word : line + ' ' + word; probe.text = candidate;
            if (line != '' && probe.measure().width > 481) { lines :+ line; line = word; }
            else { line = candidate; }
        }
        lines :+ line; probe.remove(); let baseline = 78 - 22 * (#sizeof(lines) - 1);
        for (value of lines) {
            const label = frame.text({align: 'top left', baseline: baseline, font: 'arialbd.ttf:18:mono', color: text.colors[224], text: value});
            label.left = (481 - label.measure().width) / 2; this.status_lines :+ label; baseline += 22;
        }
        this.console.show();
        if (previous != this.selected_design && #is_defined(this.visible_designs) && #is_defined(this.selection.designs)) { this.paint_strip(); }
    },
    paint_main_preview: () => {
        if (this.selection.preview_icon >= 0) {
            this.model.left = 140; this.model.top = 9; this.model.width = 311; this.model.height = 311;
            this.model.smacworkshopcachedpreview(this.selection.preview_icon, this.selection.preview_variant);
        } else {
            this.model.left = 167; this.model.top = 36; this.model.width = 256; this.model.height = 256;
            this.preview_position = this.model.smacworkshopdesign(this.selected_chassis, this.selected_weapon, this.selected_armor, this.selected_reactor,
                (0 - 1), this.selection.faction_art, 0.0, this.selection.offense, this.selection.weapon_mode, this.preview_strength, this.window.is_visible(), this.selected_abilities, this.preview_position);
        }
    },
    armor_strength: (strength) => { return strength < 0 ? this.parent_text.psi : #to_string(strength); },
    update_armor: () => {
        for (row of this.selection.armor) { if (row.id == this.selected_armor) {
            for (component of this.component_models) {
                if (component.kind == 3) { this.preview_position = this.components.smacworkshopcomponents(this.selected_armor, true, this.preview_position); }
                this.preview_position = component.model.smacworkshopdesign(this.selected_chassis, this.selected_weapon, this.selected_armor, this.selected_reactor, component.kind, this.selection.faction_art, 0.0, this.selection.offense, this.selection.weapon_mode, row.rating, this.window.is_visible(), this.selected_abilities, this.preview_position);
            }
            this.preview_strength = row.rating; this.paint_main_preview();
            this.armor_name.text = row.id == 0 ? #uppercase(this.parent_text.none) : #uppercase(row.short_name); this.armor_stat.text = row.armor_class >= 3 ? '' : this.parent_text.level + ' ' + this.armor_strength(row.rating);
            if (row.design_name != '') { this.title.text = #uppercase(row.design_name); }
            const rows = row.cost_rows < this.selected_reactor ? this.selected_reactor : row.cost_rows;
            const quantity = #to_string(rows * this.mineral_factor);
            const price = row.cost_rows < this.selected_reactor ? '(' + this.parent_text.minimum + ': ' + quantity + ')' : quantity;
            this.cost.text = #uppercase(this.parent_text.labels[5] + '  ' + price + (row.fast_land_warning ? '!' : ''));
            this.cost.color = this.parent_text.colors[row.fast_land_warning ? 249 : 224];
            const formatter = this.data_formatter; const text = this.parent_text; const data = this.selection;
            const slots = this.ability_slots; const reactor = this.selected_reactor; const workshop = this;
            const lines = formatter(text.labels, text.psi, data, row, slots, reactor);
            workshop.data_heading.text = #uppercase(text.labels[6]); workshop.data_heading.color = text.colors[224];
            let index = 0; for (label of workshop.data_rows) { label.text = lines[index]; label.color = text.colors[224]; index++; }
            this.stats.text = row.stat_line; } }
        this.paint_component_names(); this.layout_captions();
    },
    open_picker: () => {
        this.open_component_picker(2);
    },
    accept_component: (kind, choice) => {
        if (!this.active || this.pending || this.confirming) { return; }
        const base = this.base; const selected = this.selected_design; const chassis = this.selected_chassis; const weapon = this.selected_weapon;
        const armor = this.selected_armor; const reactor = this.selected_reactor; const abilities = this.selected_abilities; const name = this.draft_name;
        const first = this.ability_slots[0]; const second = this.ability_slots[1]; const automatic = this.automatic_suggestions;
        const result = base.get_smac_workshop_edit(selected, chassis, weapon, armor, reactor, abilities, name, first, second, kind, choice, automatic);
        if (!#is_defined(result)) { this.picker_active = false; this.picker.hide(); this.message.text = 'Workshop is no longer available.'; return; }
        this.selection = result; this.read_draft(result.draft); this.update_labels(); this.update_armor(); this.picker_active = false; this.picker.hide();
    },
    open_component_picker: (kind) => {
        if (!this.active || this.pending || this.confirming) { return; }
        this.picker_kind = kind; this.picker_first = 0; this.picker_rows = this.selection.armor; this.picker_selected = this.selected_armor;
        if (kind == 0) { this.picker_rows = this.selection.chassis_choices; this.picker_selected = this.selected_chassis; }
        if (kind == 1) { this.picker_rows = this.selection.weapon_choices; this.picker_selected = this.selected_weapon; }
        if (kind == 3) { this.picker_rows = this.selection.reactor_choices; this.picker_selected = this.selected_reactor; }
        let initial = 0; for (row of this.picker_rows) { if (row.id == this.picker_selected) { break; } initial++; }
        while (initial >= this.picker_first + 20) { this.picker_first = this.picker_first + 5; }
        const count = #sizeof(this.picker_rows); const first = this.picker_first;
        this.picker_state = {count: count, selected: initial, first: first,
            position: #floor(#to_float(first) / 5.0), availability: 0};
        this.picker_valid = []; for (row of this.picker_rows) { this.picker_valid :+ (row.technology_available ? 1 : 0); }
        this.picker_active = true;
        if (count > 18) {
            const range = #floor(#to_float(count) / 5.0) - 2;
            this.picker_scroll.max = #to_float(range); this.picker_scroll.value = #to_float(this.picker_first) / 5.0;
            this.picker_scroll.speed = 50.0 / #to_float(range);
            // The count>18 constructor hides the control. Only the separate
            // overflow attachment for count>20 shows it.
            if (count > 20) { this.picker_scroll.height = 313; this.picker_scroll.show(); }
            else { this.picker_scroll.height = 246; this.picker_scroll.hide(); }
        } else { this.picker_scroll.hide(); }
        this.paint_picker(); this.picker.show();
    },
    paint_picker: () => {
        const kind = this.picker_kind;
        for (card of this.cards) { card.remove(); } this.cards = [];
        let rows = []; let ordinal = this.picker_first;
        while (ordinal < #sizeof(this.picker_rows) && ordinal < this.picker_first + 20) { rows :+ this.picker_rows[ordinal] + {}; ordinal++; }
        this.picker_page = rows; this.paint_picker_models();
        let i = 0;
        for (row of rows) {
            let strength = '';
            if (kind == 2) { strength = row.rating < 0 ? this.picker_text.psi : #to_string(row.rating); }
            else { strength = kind == 1 && row.psi ? this.picker_text.psi : #to_string(row.value); }
            const card = this.picker.panel({background: 'transparent', overflow: 'hidden', left: 27 + (i % 5) * 108, top: 4 + #floor(#to_float(i) / 5.0) * 94, width: 105, height: 91});
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
                let description = this.picker_text.power;
                if (kind == 0) { description = this.picker_text.domains[row.domain]; }
                if (kind == 1) { description = row.mode < 3 ? this.picker_text.weapon : this.picker_text.equipment; }
                if (kind == 2) { description = this.picker_text.shielding; }
                card.text({align: 'top left', left: 3, baseline: 85, font: 'arial.ttf:10:mono', color: 'rgb(176,212,235)', text: description});
                card.text({align: 'top right', right: 3, baseline: 86, font: 'arialbd.ttf:12:mono', color: 'rgb(176,212,235)', text: strength});
            }
            this.cards :+ card; i++;
        }
    },
    paint_picker_models: () => {
        const kind = this.picker_kind; const rows = this.picker_page;
        if (kind == 2) { this.picker_text = this.picker_canvas.smacworkshoparmorpicker(this.picker_selected, rows); }
        else {
            const selected = this.picker_selected;
            const chassis = this.selected_chassis; const weapon = this.selected_weapon; const reactor = this.selected_reactor;
            const faction = this.selection.faction_art; const power = this.selection.offense; const mode = this.selection.weapon_mode;
            let strength = 1; for (row of this.selection.armor) { if (row.id == this.selected_armor) { strength = row.rating; } }
            this.picker_text = this.picker_canvas.smacworkshopcomponentpicker(kind, selected, rows, chassis, weapon, reactor, faction, power, mode, strength);
        }
    },
    picker_key: (e) => {
        if (!this.picker_active) { return; }
        let key = 0;
        for (entry of [{code: 'UP', key: 38}, {code: 'KP_UP', key: 104}, {code: 'DOWN', key: 40}, {code: 'KP_DOWN', key: 98},
            {code: 'LEFT', key: 37}, {code: 'KP_LEFT', key: 100}, {code: 'RIGHT', key: 39}, {code: 'KP_RIGHT', key: 102},
            {code: 'HOME', key: 36}, {code: 'END', key: 35}, {code: 'PAGEUP', key: 33}, {code: 'PAGEDOWN', key: 34},
            {code: 'ENTER', key: 13}, {code: 'ESCAPE', key: 27}]) { if (entry.code == e.code) { key = entry.key; } }
        const workshop = this; const input = this.picker_input; const state = this.picker_state; const valid = this.picker_valid;
        const result = input.keyboard(state, key, valid); workshop.apply_picker_input(result);
    },
    picker_pointer: (e, release) => {
        if (!this.picker_active) { return; }
        const workshop = this; const input = this.picker_input; const state = this.picker_state; const valid = this.picker_valid;
        const x = e.x; const y = e.y;
        // Native canvas and callback share their origin. Hit x4 differs from
        // painted frame x27; do not add a compensating client offset.
        const result = release ? input.release(state, x, y, valid) : input.hover(state, x, y, valid);
        workshop.apply_picker_input(result);
    },
    apply_picker_input: (result) => {
        const before = this.picker_state; const state = result.state; this.picker_state = state; this.picker_first = state.first;
        this.picker_selected = (0 - 1);
        if (state.selected >= 0 && state.selected < state.count) { this.picker_selected = this.picker_rows[state.selected].id; }
        let close = false;
        for (event of result.events) {
            if (event[0] == 'scroll_position') { this.picker_scroll.value = #to_float(event[1]); }
            if (event[0] == 'close') { close = true; }
        }
        if (close) {
            let choice = (0 - 1);
            if (state.selected >= 0 && state.selected < state.count && state.availability != 0) { choice = this.picker_rows[state.selected].id; }
            this.accept_component(this.picker_kind, choice); return;
        }
        if (before.first != state.first) { this.paint_picker(); }
        else if (before.selected != state.selected) { this.paint_picker_models(); }
    },
    submit: (done) => {
        if (!this.active || this.pending || this.confirming) { return; }
        this.submit_choice(done, (0 - 1));
    },
    render_overwrite: (name, count) => { this.intrinsic_message = false; this.redesign_dialog.open(this, name, count, 0); },
    answer_intrinsic: () => {
        if (!this.active || this.pending || !this.confirming || !this.intrinsic_message) { return; }
        const done = this.notice_done; this.confirming = false; this.intrinsic_message = false; this.redesign_dialog.hide();
        if (done) { this.close_view(); }
    },
    answer_overwrite: (accept) => {
        if (!this.active || this.pending || !this.confirming || this.intrinsic_message) { return; }
        this.confirming = false; this.redesign_dialog.hide(); this.submit_choice(this.confirm_done, accept ? 1 : 0);
    },
    submit_choice: (done, choice) => {
        if (!this.active || this.pending) { return; }
        this.pending_done = done;
        this.pending_selection = false;
        this.pending = true; this.next_request = this.next_request + 1; this.pending_id = this.next_request;
        const request = this.pending_id;
        const base = this.base; const expected = choice < 0 ? this.selection.expected_state : this.confirm_expected;
        const armor = this.selected_armor; const name = this.draft_name; const selected = this.selected_design;
        const chassis = this.selected_chassis; const weapon = this.selected_weapon; const reactor = this.selected_reactor; const abilities = this.selected_abilities;
        let fields = {base: base, selected_design: selected, expected_state: expected, armor: armor, chassis: chassis, weapon: weapon, reactor: reactor, abilities: abilities, name: name, done: done, request_id: request};
        if (choice >= 0) { fields = fields + {overwrite: choice}; }
        this.p.game.event('commit_smac_workshop', fields);
    },
    close_view: () => {
        this.hide_view();
        if (#is_defined(this.p.utils) && #is_defined(this.p.utils.workshop_closed)) { this.p.utils.workshop_closed(); }
    },
    hide_view: () => { this.opening = false; this.preset_notice.hide(); this.model.smacworkshopclock(0); this.picker_active = false; this.ability_dialog.hide(); this.rename_dialog.hide(); this.retire_dialog.hide(); this.upgrade_dialog.hide(); this.active = false; this.pending = false; this.base = null; this.confirming = false; this.intrinsic_message = false; this.redesign_dialog.hide(); this.picker.hide(); this.window.hide(); this.strip.hide(); this.console.hide(); },
};
