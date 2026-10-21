let view = {};
view = {
    init: (p) => { view.p = p; view.active = false; view.workshop = null; },
    create: () => {
        view.window = view.p.ui.root.window({class: 'smac-original-window', zindex: 0.98, align: 'top left', left: 150, top: 255, width: 755, height: 300,
            background: 'transparent', border: 'transparent', blocker: 'rgba(0,0,0,180)', overflow: 'hidden'});
        view.art = view.window.surface({left: 0, top: 0, width: 755, height: 300});
        view.window.surface({left: 15, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(729,1,736,529)'});
        view.window.surface({left: 700, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(738,1,745,529)'});
        view.illustration = view.window.surface({left: 28, top: 32, width: 200, height: 120});
        view.caption = view.window.panel({left: 30, top: 0, width: 663, height: 28, overflow: 'hidden'});
        view.title = view.caption.text({baseline: 21, font: 'arialbd.ttf:14:mono'});
        view.probe = view.window.text({left: 239, top: 0, font: 'arialn.ttf:16:mono-bold'}); view.probe.hide();
        view.body = []; view.rows = [];
        for (number of [0, 1]) {
            const index = number;
            const row = view.window.button({left: 33, width: 655, height: 20, smac_frame_phase: [0,1], background: 'transparent'});
            const label = row.text({left: 3, baseline: 16, font: 'arialn.ttf:16:mono-bold'});
            row.on('mousedown', (e) => { if (e.button == 'left') { view.selected = index; view.paint(); } return true; });
            row.on('click', () => { view.selected = index; view.paint(); return true; });
            row.on('dblclick', (e) => { if (e.button == 'left') { view.selected = index; view.finish(index); } return true; });
            row.on('mouseover', () => { view.hovered = index; view.paint(); return true; });
            row.on('mouseout', () => { view.hovered = (0 - 1); view.paint(); return true; });
            view.rows :+ {button: row, label: label};
        }
        view.p.ui.class('workshop-retire-ok').set({font: 'arialn.ttf:16:mono-bold', textalign: 'top center', textbaseline: 16,
            smac_button_sprite: [0,655], _hover: {smac_button_sprite: [1,655]}, _active: {smac_button_sprite: [2,655]}});
        view.ok = view.window.button({class: 'workshop-retire-ok', left: 33, width: 655, height: 20});
        view.ok.on('click', () => { view.finish(view.selected); return true; });
        view.window.on('keydown', (e) => {
            // Flags-zero generic dialogs return the selected row on Escape
            // as well as Enter; there is no flags40 cancellation conversion.
            if ((e.modifiers == {} && (e.code == 'ESCAPE' || e.code == 'ENTER')) || (e.modifiers == {shift: true} && e.code == 'ENTER')) { view.finish(view.selected); }
            else if (e.modifiers == {} && (e.code == 'UP' || e.code == 'HOME')) { view.selected = 0; view.paint(); }
            else if (e.modifiers == {} && (e.code == 'DOWN' || e.code == 'END')) { view.selected = 1; view.paint(); }
            return true;
        }); view.window.hide();
    },
    begin: (workshop) => {
        if (!workshop.active || workshop.pending || workshop.confirming || workshop.picker_active || workshop.ability_dialog.active || workshop.rename_dialog.active || view.active) { return; }
        view.workshop = workshop;
        const base = workshop.base; const selected = workshop.selected_design; const name = workshop.draft_name;
        const expected = workshop.selection.expected_state; const offset = workshop.strip_offset;
        view.fields = {base: base, selected_design: selected, name: name, expected_state: expected, request_id: 0, confirm: (0 - 1), strip_offset: offset};
        view.send(0 - 1);
    },
    send: (answer) => {
        const workshop = view.workshop; let fields = view.fields + {};
        workshop.next_request = workshop.next_request + 1; workshop.pending_id = workshop.next_request;
        workshop.pending = true; workshop.pending_retire = true; fields.request_id = workshop.pending_id; fields.confirm = answer;
        view.p.game.event('retire_smac_workshop', fields);
    },
    reply: (e) => {
        const workshop = view.workshop;
        if (workshop == null || !workshop.active || !workshop.pending || !workshop.pending_retire || e.base.id != workshop.base.id || e.request_id != workshop.pending_id) { return; }
        workshop.pending = false; workshop.pending_retire = false; workshop.selection.expected_state = e.expected_state;
        if (e.stale || e.failed) { workshop.message.text = e.stale ? 'Game state changed. Please check your design.' : 'This design cannot be retired.'; view.hide(); return; }
        if (e.confirmation) {
            if (!#is_defined(view.window)) { view.create(); }
            const words = view.art.smacworkshopretiretext(e.stored_name, e.count); const colors = words.colors;
            view.colors = colors;
            view.illustration.background = e.illustration + ':smacsoftware()';
            view.title.text = #uppercase(words.title); view.title.color = colors[226]; view.title.left = (663 - view.title.measure().width) / 2;
            for (line of view.body) { line.remove(); } view.body = [];
            let lines = []; let current = ''; const tokens = #split(words.body, ' '); let index = 0;
            for (token of tokens) {
                const piece = token + (index + 1 < #sizeof(tokens) ? ' ' : '');
                view.probe.text = current + piece;
                if (current != '' && view.probe.measure().width > 449) { lines :+ current; current = piece; }
                else { current += piece; } index++;
            }
            if (current != '') { lines :+ current; }
            let baseline = 50;
            for (line of lines) { view.body :+ view.window.text({left: 239, baseline: baseline, font: 'arialn.ttf:16:mono-bold', color: colors[226], text: line}); baseline += 20; }
            const top = #sizeof(lines) > 6 ? 44 + 20 * #sizeof(lines) : 164; const bottom = top + 44;
            view.art.smac_frame = [[9,23,0,700,305],[3,27,4,696,28],[6,30,7,693,25],[0,236,35,693,38 + 20 * #sizeof(lines)],[3,233,32,696,41 + 20 * #sizeof(lines)]];
            view.rows[0].button.top = top + 1; view.rows[1].button.top = top + 23;
            view.rows[0].label.text = words.rows[0]; view.rows[1].label.text = words.rows[1];
            for (row of view.rows) { row.label.color = colors[0]; }
            view.ok.top = bottom + 9; view.ok.text = words.ok; view.ok.color = colors[0];
            view.selected = 0; view.hovered = (0 - 1); view.fields.expected_state = e.expected_state; view.active = true; view.paint(); view.window.show(); return;
        }
        if (e.retired) {
            workshop.selection = e.selection;
            if (#is_defined(e.selection.draft)) { workshop.read_draft(e.selection.draft); workshop.selected_design = e.selection.draft.selected_design; }
            workshop.strip_offset = e.strip_offset; workshop.strip_hover = (0 - 1); workshop.update_labels(); workshop.update_armor(); workshop.render_strip();
        }
        workshop.message.text = ''; view.hide();
    },
    paint: () => {
        let index = 0; for (row of view.rows) {
            row.button.smac_frame = [[index == view.selected ? 29 : (index == view.hovered ? 28 : 27),0,0,655,20]];
            row.label.color = index != view.selected && index == view.hovered ? view.colors[223] : view.colors[0]; index++;
        }
    },
    finish: (answer) => { if (!view.active) { return; } view.active = false; view.window.hide(); view.send(answer); },
    hide: () => { view.active = false; view.workshop = null; if (#is_defined(view.window)) { view.window.hide(); } },
};
return view;
