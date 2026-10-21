let view = {};
view = {
    init: (p) => { view.p = p; view.active = false; view.workshop = null; },
    create: () => {
        view.window = view.p.ui.root.window({class: 'smac-original-window', zindex: 0.98, align: 'top left', left: 148, top: 255, width: 760, height: 300,
            background: 'transparent', border: 'transparent', blocker: 'rgba(0,0,0,180)', overflow: 'hidden'});
        const art = view.window.surface({left: 0, top: 0, width: 760, height: 300,
            smac_frame: [[9,23,0,705,305],[3,27,4,701,28],[6,30,7,698,25],[3,30,35,696,67]]});
        view.words = art.smacworkshoprenametext(); const colors = view.words.colors;
        view.window.surface({left: 15, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(729,1,736,529)'});
        view.window.surface({left: 705, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(738,1,745,529)'});
        const caption = view.window.panel({left: 30, top: 0, width: 668, height: 28, overflow: 'hidden'});
        const title = caption.text({align: 'top left', baseline: 21, font: 'arialbd.ttf:14:mono', color: colors[222], text: #uppercase(view.words.title)});
        title.left = (668 - title.measure().width) / 2;
        const row = view.window.panel({left: 33, top: 38, width: 660, height: 26, overflow: 'hidden'});
        row.text({left: 3, baseline: 17, font: 'arialn.ttf:16:mono-bold', color: colors[224], text: view.words.label});
        view.input = row.input({left: 53, top: 1, width: 610, height: 20, max_bytes: 31, native_edit: true, textbaseline: 16,
            font: 'arialn.ttf:16:mono-bold', color: colors[224], background: colors[0], selection_color: colors[0], selection_background: colors[36], caret_color: colors[224], value: ''});
        view.input.on('input', () => { view.finish(true); return true; });
        view.p.ui.class('workshop-rename-answer').set({font: 'arialn.ttf:16:mono-bold', color: colors[0], textalign: 'top center', textbaseline: 16,
            smac_frame: [[27,0,0,327,20]], _hover: {smac_frame: [[28,0,0,327,20]]}, _active: {smac_frame: [[29,0,0,327,20]]}});
        view.window.button({class: 'workshop-rename-answer', left: 33, top: 79, width: 327, height: 20, text: view.words.ok}).on('click', () => { view.finish(true); return true; });
        view.window.button({class: 'workshop-rename-answer', left: 366, top: 79, width: 327, height: 20, text: view.words.cancel}).on('click', () => { view.finish(false); return true; });
        view.window.on('keydown', (e) => {
            if (e.modifiers == {} && e.code == 'ESCAPE') { view.finish(false); }
            else if (e.modifiers == {} && e.code == 'ENTER') { view.finish(true); }
            return true;
        }); view.window.hide();
    },
    begin: (workshop) => {
        if (!workshop.active || workshop.pending || workshop.confirming || workshop.picker_active || workshop.ability_dialog.active || view.active) { return; }
        view.workshop = workshop;
        const base = workshop.base; const selected = workshop.selected_design; const chassis = workshop.selected_chassis; const weapon = workshop.selected_weapon;
        const armor = workshop.selected_armor; const reactor = workshop.selected_reactor; const abilities = workshop.selected_abilities; const name = workshop.draft_name;
        const expected = workshop.selection.expected_state;
        workshop.pending = true; workshop.pending_rename = true; workshop.next_request = workshop.next_request + 1; workshop.pending_id = workshop.next_request;
        const request = workshop.pending_id;
        view.fields = {base: base, selected_design: selected, chassis: chassis, weapon: weapon, armor: armor, reactor: reactor, abilities: abilities,
            name: name, expected_state: expected, request_id: request};
        view.awaiting_answer = false; view.p.game.event('open_smac_workshop_rename', view.fields);
    },
    reply: (e) => {
        const workshop = view.workshop;
        if (workshop == null || !workshop.active || !workshop.pending || !workshop.pending_rename
            || e.base.id != workshop.base.id || e.request_id != workshop.pending_id || e.answer != view.awaiting_answer) { return; }
        workshop.pending = false; workshop.pending_rename = false; workshop.selection.expected_state = e.expected_state;
        if (e.stale || e.failed) { workshop.message.text = e.stale ? 'Game state changed. Please check your design.' : 'This design cannot be renamed.'; view.hide(); return; }
        if (e.answer) {
            workshop.draft_name = e.name;
            if (#is_defined(workshop.selection.draft)) { workshop.selection.draft.name = e.name; }
            workshop.title.text = #uppercase(e.display_name); workshop.message.text = ''; view.hide(); workshop.paint_main_preview(); return;
        }
        view.default_name = e.default_name;
        if (!#is_defined(view.window)) { view.create(); }
        view.input.value = e.default_name; view.active = true; view.window.show(); view.input.focus(); view.input.select_all();
    },
    finish: (accept) => {
        if (!view.active) { return; }
        view.active = false; view.window.hide();
        if (!accept) { view.workshop = null; return; }
        const workshop = view.workshop; const submitted = view.input.value; const default_name = view.default_name;
        let fields = view.fields + {displayed_default: default_name, submitted: submitted};
        fields.expected_state = workshop.selection.expected_state;
        workshop.pending = true; workshop.pending_rename = true; workshop.next_request = workshop.next_request + 1; workshop.pending_id = workshop.next_request;
        fields.request_id = workshop.pending_id; view.awaiting_answer = true;
        view.p.game.event('answer_smac_workshop_rename', fields);
    },
    hide: () => { view.active = false; view.workshop = null; if (#is_defined(view.window)) { view.window.hide(); } },
};
return view;
