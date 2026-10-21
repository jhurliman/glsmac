let view = {};
view = {
    init: (p) => { view.p = p; view.active = false; view.workshop = null; },
    create: () => {
        view.window = view.p.ui.root.window({class: 'smac-original-window', zindex: 0.98, align: 'top left', left: 150, top: 255, width: 755, height: 300,
            background: 'transparent', border: 'transparent', blocker: 'rgba(0,0,0,180)', overflow: 'hidden'});
        view.art = view.window.surface({left: 0, top: 0, width: 755, height: 300});
        view.window.surface({left: 15, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(729,1,736,529)'});
        view.rail = view.window.surface({left: 700, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(738,1,745,529)'});
        view.illustration = view.window.surface({left: 28, top: 32, width: 200, height: 120});
        view.caption = view.window.panel({left: 30, top: 0, width: 663, height: 28, overflow: 'hidden'});
        view.title = view.caption.text({baseline: 21, font: 'arialbd.ttf:14:mono'});
        view.probe = view.window.text({left: 0, top: 0, font: 'arialn.ttf:16:mono-bold'}); view.probe.hide();
        view.body = []; view.rows = []; view.answers = [];
        view.window.on('keydown', (e) => {
            if (e.modifiers == {} && e.code == 'ESCAPE') { view.finish(view.kind == 0 ? (0 - 1) : view.selected); }
            else if ((e.modifiers == {} || e.modifiers == {shift: true}) && e.code == 'ENTER') { view.finish(view.selected); }
            else if (e.modifiers == {} && #sizeof(view.options) > 0) {
                if (e.code == 'UP' || e.code == 'HOME') { view.selected = e.code == 'HOME' ? 0 : #max(0, view.selected - 1); view.paint(); }
                else if (e.code == 'DOWN' || e.code == 'END') { view.selected = e.code == 'END' ? #sizeof(view.options) - 1 : #min(#sizeof(view.options) - 1, view.selected + 1); view.paint(); }
                else if (view.kind == 0 && e.code == 'LEFT' && view.selected + view.rows_per_column < #sizeof(view.options)) { view.selected = view.selected + view.rows_per_column; view.paint(); }
                else if (view.kind == 0 && e.code == 'RIGHT' && view.selected >= view.rows_per_column) { view.selected = view.selected - view.rows_per_column; view.paint(); }
            } return true;
        }); view.window.hide();
    },
    begin: (workshop) => {
        if (!workshop.active || workshop.pending || workshop.confirming || workshop.picker_active || workshop.ability_dialog.active || workshop.rename_dialog.active || workshop.retire_dialog.active || view.active) { return; }
        view.workshop = workshop; const base = workshop.base; const source = workshop.selected_design; const name = workshop.draft_name;
        const expected = workshop.selection.expected_state; const offset = workshop.strip_offset;
        view.fields = {base: base, selected_design: source, target_design: (0 - 1), name: name, expected_state: expected, request_id: 0, confirm: (0 - 1), strip_offset: offset};
        view.send(0 - 1);
    },
    send: (answer) => {
        const workshop = view.workshop; let fields = view.fields + {};
        workshop.next_request = workshop.next_request + 1; workshop.pending_id = workshop.next_request;
        workshop.pending = true; workshop.pending_upgrade = true; fields.request_id = workshop.pending_id; fields.confirm = answer;
        view.p.game.event('upgrade_smac_workshop', fields);
    },
    reply: (e) => {
        const workshop = view.workshop;
        if (workshop == null || !workshop.active || !workshop.pending || !workshop.pending_upgrade || e.base.id != workshop.base.id || e.request_id != workshop.pending_id) { return; }
        workshop.pending = false; workshop.pending_upgrade = false; workshop.selection.expected_state = e.expected_state;
        if (e.stale || e.failed) { workshop.message.text = e.stale ? 'Game state changed. Please check your design.' : 'This design cannot be upgraded.'; view.hide(); return; }
        view.fields.expected_state = e.expected_state; view.fields.target_design = e.target_design; view.result = e;
        if (e.upgraded) {
            workshop.selection = e.selection;
            if (#is_defined(e.selection.draft)) { workshop.read_draft(e.selection.draft); workshop.selected_design = e.selection.draft.selected_design; }
            workshop.strip_offset = e.strip_offset; workshop.strip_hover = (0 - 1); workshop.update_labels(); workshop.update_armor(); workshop.render_strip();
            if (e.zero_count_notice) { view.show(5); } else { view.hide(); } return;
        }
        if (e.declined) { workshop.message.text = ''; view.hide(); return; }
        if (e.confirmation) { view.show(e.unprototyped ? 2 : 1); return; }
        if (!e.affordable) { view.show(e.unprototyped ? 4 : 3); return; }
        if (#sizeof(e.choices) == 0) { view.show(6); return; }
        view.show(0);
    },
    show: (kind) => {
        if (!#is_defined(view.window)) { view.create(); }
        const e = view.result; view.kind = kind;
        const words = view.art.smacworkshopupgradetext(kind, e.source_text, e.target_text, e.count, e.total, e.available);
        view.colors = words.colors; const chooser = kind == 0; const confirm = kind == 1 || kind == 2;
        let columns = 1; let allocation = 660; let rows_per_column = 2;
        if (chooser) {
            const count = #sizeof(e.choices); columns = count <= 12 ? 1 : count / 12 + 1; rows_per_column = (count + columns - 1) / columns;
            let measured = 0;
            for (choice of e.choices) { view.probe.text = choice.stored_name + '|' + choice.stat_line + (choice.prototype ? ' *' : ''); measured = #max(measured, view.probe.measure().width); }
            allocation = #max(660, columns * (measured + 6));
        }
        const width = chooser ? allocation + 97 : (confirm ? 755 : 758); const inner = width - 92; const wrap = chooser ? allocation - 3 : width - 306; const x = chooser ? 33 : 239;
        view.window.width = width; view.window.left = (1024 - width + 32) / 2; view.art.width = width; view.rail.left = width - 55; view.caption.width = inner;
        view.rows_per_column = rows_per_column;
        view.title.text = #uppercase(words.title); view.title.color = words.colors[226]; view.title.left = (inner - view.title.measure().width) / 2;
        if (chooser) { view.illustration.hide(); } else { view.illustration.background = (kind == 5 ? 'unit_sm.pcx' : e.illustration) + ':smacsoftware()'; view.illustration.show(); }
        for (node of view.body) { node.remove(); } view.body = [];
        for (row of view.rows) { row.button.remove(); } view.rows = [];
        for (button of view.answers) { button.remove(); } view.answers = [];
        let current = ''; let baseline = 50; let last = null; let last_color = (0 - 1);
        for (token of words.tokens) {
            view.probe.text = current + token.text;
            if (current != '' && view.probe.measure().width > wrap) { current = ''; baseline += 20; last = null; }
            for (span of token.spans) {
                if (last == null || last_color != span.color) {
                    view.probe.text = current; const left = x + (current == '' ? 0 : view.probe.measure().width);
                    last = view.window.text({left: left, baseline: baseline, font: 'arialn.ttf:16:mono-bold', color: words.colors[span.color], text: ''}); view.body :+ last; last_color = span.color;
                }
                last.text = last.text + span.text; current += span.text;
            }
        }
        const lines = (baseline - 50) / 20 + 1; const list_top = chooser ? 44 + 20 * lines : #max(164, 44 + 20 * lines);
        view.options = [];
        if (chooser) { for (choice of e.choices) { view.options :+ {value: choice.id, text: choice.stored_name, stats: choice.stat_line + (choice.prototype ? ' *' : '')}; } }
        else { let value = 0; for (row of words.rows) { view.options :+ {value: value, text: row, stats: ''}; value++; } }
        view.selected = 0; view.hovered = (0 - 1); view.list_top = list_top; view.row_width = chooser ? allocation / columns - 3 : 655;
        const height = chooser ? #max(300, 22 * rows_per_column + 20 * lines + 120) : 300;
        view.window.height = height; view.window.top = 555 - height; view.art.height = height;
        let index = 0;
        for (option of view.options) {
            const at = index; const column = chooser ? index / rows_per_column : 0; const row_index = chooser ? index % rows_per_column : index;
            const left = column * (view.row_width + 3); const row_top = 1 + row_index * 22;
            const button = view.window.button({left: 33 + left, top: list_top + row_top, width: view.row_width, height: 20, smac_frame_phase: [0 + left,0 + row_top], background: 'transparent'});
            const label = button.text({left: 3, baseline: 16, font: 'arialn.ttf:16:mono-bold', text: option.text});
            const stats = button.text({baseline: 16, font: 'arialn.ttf:16:mono-bold', text: option.stats}); stats.left = view.row_width - 3 - stats.measure().width;
            button.on('mousedown', (ev) => { if (ev.button == 'left') { view.selected = at; view.paint(); } return true; });
            button.on('dblclick', (ev) => { if (ev.button == 'left') { view.selected = at; view.finish(at); } return true; });
            button.on('mouseover', () => { view.hovered = at; view.paint(); return true; });
            button.on('mouseout', () => { view.hovered = (0 - 1); view.paint(); return true; });
            view.rows :+ {button: button, label: label, stats: stats}; index++;
        }
        const bottom = list_top + 22 * (chooser ? rows_per_column : #sizeof(view.options)); let button_top = bottom + 9;
        if (!chooser && !confirm) { button_top = #max(167, 47 + 20 * lines); }
        if (chooser) {
            view.art.smac_frame = [[9,23,0,width - 55,height + 5],[3,27,4,width - 59,28],[6,30,7,width - 62,25],[0,30,35,width - 62,38 + 20 * lines],[3,27,32,width - 59,bottom + 3]];
        } else {
            view.art.smac_frame = [[9,23,0,width - 55,height + 5],[3,27,4,width - 59,28],[6,30,7,width - 62,25],[0,236,35,width - 62,38 + 20 * lines],[3,233,32,width - 59,41 + 20 * lines]];
        }
        const button_width = chooser ? (allocation - 9) / 2 : width - 100;
        const button_class = 'workshop-upgrade-answer-' + #to_string(button_width);
        view.p.ui.class(button_class).set({font: 'arialn.ttf:16:mono-bold', textalign: 'top center', textbaseline: 16,
            smac_button_sprite: [0,button_width], _hover: {smac_button_sprite: [1,button_width]}, _active: {smac_button_sprite: [2,button_width]}});
        const ok = view.window.button({class: button_class, left: chooser ? 34 : 33, top: button_top, width: button_width, height: 20, color: words.colors[0], text: words.ok});
        ok.on('click', () => { view.finish(view.selected); return true; }); view.answers :+ ok;
        if (chooser) {
            const cancel = view.window.button({class: button_class, left: 34 + button_width + 6, top: button_top, width: button_width, height: 20, color: words.colors[0], text: words.cancel});
            cancel.on('click', () => { view.finish(0 - 1); return true; }); view.answers :+ cancel;
        }
        view.active = true; view.paint(); view.window.show();
    },
    paint: () => {
        let index = 0;
        for (row of view.rows) {
            row.button.smac_frame = [[index == view.selected ? 29 : (index == view.hovered ? 28 : 27),0,0,0 + view.row_width,20]];
            const color = index != view.selected && index == view.hovered ? view.colors[223] : view.colors[0]; row.label.color = color; row.stats.color = color; index++;
        }
    },
    finish: (selected) => {
        if (!view.active) { return; } const kind = view.kind; view.active = false; view.window.hide();
        if (kind == 0 && selected >= 0) { view.fields.target_design = view.options[selected].value; view.send(0 - 1); }
        else if (kind == 1 || kind == 2) { view.send(selected); }
        else { view.hide(); }
    },
    hide: () => { view.active = false; view.workshop = null; if (#is_defined(view.window)) { view.window.hide(); } },
};
return view;
