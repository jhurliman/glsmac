let view = {};
view = {
    init: (p) => { view.p = p; view.active = false; view.workshop = null; view.create(); },
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
        view.p.ui.class('workshop-redesign-ok').set({font: 'arialn.ttf:16:mono-bold', textalign: 'top center', textbaseline: 16,
            smac_button_sprite: [0,655], _hover: {smac_button_sprite: [1,655]}, _active: {smac_button_sprite: [2,655]}});
        view.ok = view.window.button({class: 'workshop-redesign-ok', left: 33, width: 655, height: 20});
        view.ok.on('click', () => { view.finish(view.selected); return true; });
        view.window.on('keydown', (e) => {
            // Flags-zero generic dialogs return the selected row on Escape
            // as well as Enter; there is no flags40 cancellation conversion.
            if ((e.modifiers == {} && (e.code == 'ESCAPE' || e.code == 'ENTER')) || (e.modifiers == {shift: true} && e.code == 'ENTER')) { view.finish(view.selected); }
            else if (view.kind == 0 && e.modifiers == {} && (e.code == 'UP' || e.code == 'HOME')) { view.selected = 0; view.paint(); }
            else if (view.kind == 0 && e.modifiers == {} && (e.code == 'DOWN' || e.code == 'END')) { view.selected = 1; view.paint(); }
            return true;
        }); view.window.hide();
    },
    open: (workshop, name, count, kind) => {
        view.workshop = workshop; view.kind = kind;
        const words = kind == 0 ? view.art.smacworkshopredesigntext(name, count) : view.art.smacworkshopintrinsictext(kind, name); const colors = words.colors;
        view.colors = colors; view.illustration.background = 'unit_sm.pcx:smacsoftware()';
        view.title.text = #uppercase(words.title); view.title.color = colors[226]; view.title.left = (663 - view.title.measure().width) / 2;
        for (node of view.body) { node.remove(); } view.body = [];
        let current = ''; let baseline = 50; let last = null; let last_color = (0 - 1);
        for (token of words.tokens) {
            view.probe.text = current + token.text;
            if (current != '' && view.probe.measure().width > 449) { current = ''; baseline += 20; last = null; }
            for (span of token.spans) {
                if (last == null || last_color != span.color) {
                    view.probe.text = current; const left = 239 + (current == '' ? 0 : view.probe.measure().width);
                    last = view.window.text({left: left, baseline: baseline, font: 'arialn.ttf:16:mono-bold', color: colors[span.color], text: ''});
                    view.body :+ last; last_color = span.color;
                }
                last.text = last.text + span.text; current += span.text;
            }
        }
        const lines = (baseline - 50) / 20 + 1; const top = #max(164, 44 + 20 * lines);
        view.art.smac_frame = [[9,23,0,700,305],[3,27,4,696,28],[6,30,7,693,25],[0,236,35,693,38 + 20 * lines],[3,233,32,696,41 + 20 * lines]];
        view.rows[0].button.top = top + 1; view.rows[1].button.top = top + 23;
        if (kind == 0) { view.rows[0].button.show(); view.rows[1].button.show(); }
        else { view.rows[0].button.hide(); view.rows[1].button.hide(); }
        if (kind == 0) { view.rows[0].label.text = words.rows[0]; view.rows[1].label.text = words.rows[1]; }
        view.ok.top = kind == 0 ? top + 53 : #max(167, 47 + 20 * lines); view.ok.text = words.ok; view.ok.color = colors[0];
        view.selected = 0; view.hovered = (0 - 1); view.active = true; view.paint(); view.window.show();
    },
    paint: () => {
        let index = 0; for (row of view.rows) {
            row.button.smac_frame = [[index == view.selected ? 29 : (index == view.hovered ? 28 : 27),0,0,655,20]];
            row.label.color = index != view.selected && index == view.hovered ? view.colors[223] : view.colors[0]; index++;
        }
    },
    finish: (answer) => {
        if (!view.active) { return; } view.active = false; view.window.hide(); if (view.kind == 0) { view.workshop.answer_overwrite(answer == 1); } else { view.workshop.answer_intrinsic(); }
    },
    hide: () => { view.active = false; view.workshop = null; view.window.hide(); },
};
return view;
