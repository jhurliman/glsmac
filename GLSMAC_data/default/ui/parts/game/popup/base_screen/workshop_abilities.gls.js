let view = {};
view = {
    init: (p) => { view.p = p; view.active = false; view.buttons = []; view.lines = []; },
    create: () => {
        view.window = view.p.ui.root.window({class: 'smac-original-window', zindex: 0.97, align: 'top left',
            left: 178, top: 210, width: 700, height: 345, background: 'transparent', border: 'transparent',
            blocker: 'rgba(0, 0, 0, 180)', overflow: 'hidden'});
        view.art = view.window.surface({left: 0, top: 0, width: 700, height: 345});
        view.rail_left = view.window.surface({left: 15, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(729,1,736,529)'});
        view.rail_right = view.window.surface({left: 645, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(738,1,745,529)'});
        view.title_frame = view.window.panel({left: 33, top: 0, width: 602, height: 28, overflow: 'hidden'});
        view.title = view.title_frame.text({align: 'top center', baseline: 21,
            font: 'arialbd.ttf:14:mono', color: 'rgb(109,126,178)'});
        view.list_parent = view.window.panel({left: 33, top: 38, width: 620, height: 225, overflow: 'hidden'});
        view.list = view.list_parent.panel({left: 0, top: 0, width: 600, height: 225, overflow: 'hidden'});
        view.scroll = view.window.scrollbar({class: 'default-scroll-v', left: 657, top: 21, width: 24, height: 245,
            fromto_size: 24, min: 0.0, max: 1.0, value: 0.0});
        view.scroll.left = 657; view.scroll.width = 24; view.scroll.top = 21; view.scroll.height = 245;
        view.scroll.on('change', (e) => {
            if (view.active && !view.notice) { view.first = #floor(e.value + 0.5); view.hovered = (0 - 1); view.draw_rows(); }
            return true;
        });
        view.ok = view.window.button({class: 'game-popup-button', left: 33, top: 278, width: 297, height: 20,
            font: 'arialn.ttf:16:mono-bold'});
        view.cancel = view.window.button({class: 'game-popup-button', left: 336, top: 278, width: 297, height: 20,
            font: 'arialn.ttf:16:mono-bold'});
        view.ok.on('click', () => { view.finish(true); return true; });
        view.cancel.on('click', () => { view.finish(false); return true; });
        view.window.on('keydown', (e) => { view.key(e); return true; }); view.window.hide();
    },
    open: (workshop, slot) => {
        if (!workshop.active || workshop.pending || workshop.confirming) { return; }
        const base = workshop.base; const selected = workshop.selected_design; const chassis = workshop.selected_chassis;
        const weapon = workshop.selected_weapon; const armor = workshop.selected_armor; const reactor = workshop.selected_reactor;
        const abilities = workshop.selected_abilities; const name = workshop.draft_name;
        const first = workshop.ability_slots[0]; const second = workshop.ability_slots[1];
        const data = base.get_smac_workshop_abilities(selected, chassis, weapon, armor, reactor, abilities, name, first, second, slot);
        if (!#is_defined(data)) { return; }
        if (!#is_defined(view.window)) { view.create(); }
        view.workshop = workshop; view.slot = slot; view.active = true; view.notice = data.guard != '';
        const required = data.required_technology; const art = view.art;
        view.words = art.smacworkshopabilitytext(required);
        view.rows = []; view.selected = 0; view.hovered = (0 - 1); view.first = 0; view.focus = 0;
        for (line of view.lines) { line.remove(); } view.lines = [];
        for (button of view.buttons) { button.remove(); } view.buttons = []; view.spans = [];
        view.ok.text = view.words.ok; view.cancel.text = view.words.cancel;
        let width = 700; let list_height = 16;
        if (view.notice) {
            width = 760; list_height = 40;
            view.title.text = #uppercase(data.guard == 'missile' ? view.words.missile_title : view.words.second_title);
            const text = data.guard == 'missile' ? view.words.missile_body : view.words.second_body;
            let baseline = 16;
            for (line of #split(text, '\n')) {
                view.lines :+ view.list.text({left: 6, baseline: baseline, font: 'arialn.ttf:16:mono-bold', color: 'rgb(109,126,178)', text: line}); baseline += 20;
            }
            view.list_parent.width = 680; view.list.width = 660; view.ok.width = 660; view.cancel.hide(); view.scroll.hide();
        } else {
            for (row of data.rows) {
                view.rows :+ {id: row.id, left: row.id == 24 ? view.words.none : row.name + ': ' + row.description,
                    right: row.id == 24 ? '' : view.cost(row.cost)};
            }
            view.capacity = #sizeof(view.rows) < 12 ? #sizeof(view.rows) : 12;
            list_height = 19 * view.capacity - 3; view.title.text = #uppercase(view.words.title);
            view.list_parent.width = 620; view.list.width = 600; view.ok.width = 297; view.cancel.show();
            if (#sizeof(view.rows) > view.capacity) {
                const range = #sizeof(view.rows) - view.capacity; view.scroll.max = #to_float(range);
                view.scroll.value = 0.0; view.scroll.speed = 50.0 / #to_float(range);
                view.scroll.slider_size = #floor(#to_float(list_height - 28) * #to_float(view.capacity) / #to_float(#sizeof(view.rows)));
                view.scroll.show();
            } else { view.scroll.hide(); }
        }
        const natural_height = list_height + 120; const height = natural_height < 300 ? 300 : natural_height;
        view.window.width = width; view.window.height = height; view.window.left = #floor(#to_float(1024 - width + 32) / 2.0); view.window.top = 555 - height;
        view.art.width = width; view.art.height = height; view.title_frame.width = width - 98; view.rail_right.left = width - 55;
        view.list_parent.height = list_height; view.list.height = list_height;
        view.ok.top = list_height + 53; view.cancel.top = list_height + 53; view.scroll.height = list_height + 20;
        view.art.smac_frame = [[9, 23, 0, width - 55, height + 5], [3, 27, 4, width - 59, 28],
            [6, 30, 7, width - 62, 25], [3, 30, 35, width - 64, list_height + 41]];
        if (!view.notice) { view.draw_rows(); }
        view.window.show();
    },
    cost: (value) => {
        let text = view.words.cost + ': ';
        if (!#is_defined(value)) { return text; }
        text += #to_string(value.quantity); let formula = '';
        if (value.formula == (0 - 1)) { formula = view.words.attack + '/' + view.words.defense; }
        if (value.formula == (0 - 2)) { formula = view.words.attack + '-1'; }
        if (value.formula == (0 - 3)) { formula = view.words.defense + '-1'; }
        if (value.formula == (0 - 4)) { formula = view.words.speed + '-1'; }
        if (value.formula == (0 - 5)) { formula = view.words.attack + '+' + view.words.defense + '-2'; }
        if (value.formula == (0 - 6)) { formula = view.words.attack + '+' + view.words.speed + '-2'; }
        if (value.formula == (0 - 7)) { formula = view.words.defense + '+' + view.words.speed + '-2'; }
        if (value.domain >= 0) { formula = view.words.domains[value.domain]; }
        if (formula != '') { text += ' (' + formula + ')'; } return text;
    },
    draw_rows: () => {
        for (button of view.buttons) { button.remove(); } view.buttons = []; view.spans = [];
        let index = view.first;
        while (index < #sizeof(view.rows) && index < view.first + view.capacity) {
            const ordinal = index; const row = view.rows[index]; const top = 19 * (index - view.first);
            const button = view.list.button({left: 0, top: top, width: 600, height: 16, overflow: 'hidden',
                background: 'transparent', border: 'transparent', text: '',
                smac_frame_phase: [0, top]});
            const clip = button.panel({left: 6, top: 0, width: 591, height: 16, overflow: 'hidden'});
            const left = clip.text({align: 'top left', baseline: 14, font: 'arialn.ttf:16:mono-bold', text: row.left});
            const right = clip.text({align: 'top right', baseline: 14, font: 'arialn.ttf:16:mono-bold', text: row.right});
            view.spans :+ {left: left, right: right};
            button.on('click', () => { view.choose(ordinal); return true; });
            button.on('mouseover', () => { view.hovered = ordinal; view.paint(); return true; });
            button.on('mouseout', () => { if (view.hovered == ordinal) { view.hovered = (0 - 1); view.paint(); } return true; });
            view.buttons :+ button; index++;
        }view.paint();
    },
    paint: () => {
        let index = view.first; let visible = 0;
        for (button of view.buttons) {
            const selected = index == view.selected; const hovered = index == view.hovered;
            button.smac_frame = [[selected ? 29 : (hovered ? 28 : 27), 0, 0, 600, 16]];
            const color = !selected && hovered ? 'rgb(144,184,228)' : 'rgb(0,0,0)';
            view.spans[visible].left.color = color; view.spans[visible].right.color = color; index++; visible++;
        }
    },
    choose: (index) => {
        const first = view.first; view.selected = index; view.focus = 0;
        if (view.selected < view.first) { view.first = view.selected; }
        if (view.selected >= view.first + view.capacity) { view.first = view.selected - view.capacity + 1; }
        view.scroll.value = #to_float(view.first);
        if (first == view.first) { view.paint(); } else { view.draw_rows(); }
    },
    key: (e) => {
        if (!view.active) { return; }
        if (e.code == 'ESCAPE' && e.modifiers == {}) { view.finish(false); return; }
        if (e.code == 'ENTER' && e.modifiers == {}) { view.finish(view.focus != 2); return; }
        if (view.notice) { return; }
        if (e.code == 'TAB') { view.focus = (view.focus + (e.modifiers == {shift: true} ? 2 : 1)) % 3; return; }
        if (view.focus != 0 || e.modifiers != {}) { return; }
        if (e.code == 'UP') { view.choose(view.selected > 0 ? view.selected - 1 : 0); }
        else if (e.code == 'DOWN') { view.choose(view.selected + 1 < #sizeof(view.rows) ? view.selected + 1 : view.selected); }
        else if (#is_defined(e.key)) {
            let scanned = 1;
            while (scanned <= #sizeof(view.rows)) {
                const index = (view.selected + scanned) % #sizeof(view.rows);
                const label = view.rows[index].left;
                if (#uppercase(('' + label)[0::1]) == #uppercase(e.key)) { view.choose(index); break; } scanned++;
            }
        }
    },
    finish: (accept) => {
        if (!view.active) { return; }
        const workshop = view.workshop; const slot = view.slot;
        let choice = (0 - 1); if (accept && !view.notice) { choice = view.rows[view.selected].id; }
        view.hide(); if (choice >= 0) { workshop.accept_component(4 + slot, choice); }
    },
    hide: () => { view.active = false; view.workshop = null; if (#is_defined(view.window)) { view.window.hide(); } },
};

return view;
