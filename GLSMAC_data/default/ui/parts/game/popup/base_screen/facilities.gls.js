return {
    init: (p) => {
        this.p = p; this.base = null; this.active = false;
        this.frame = p.body.panel({class: 'base-screen-side-frame', align: 'top right', top: 32, right: 3, height: 225});
        const header = this.frame.panel({class: 'base-screen-header'});
        header.text({class: 'base-screen-frame-title', text: 'BASE FACILITIES'});
        this.list = this.frame.listview({class: 'base-screen-body', itemsize: 17, padding: 4});
        this.window = p.ui.root.window({class: 'game-popup', zindex: 0.9, width: 640, height: 340, blocker: 'rgba(0, 0, 0, 180)'});
        this.title = this.window.text({class: 'game-popup-text', top: 12, left: 16, right: 16, text: ''});
        this.content = this.window.panel({top: 44, left: 16, right: 16, bottom: 52});
        this.window.button({class: 'game-popup-button', align: 'bottom left', left: 238, width: 164, bottom: 12, height: 28, text: 'Close'}).on('click', () => { this.hide_view(); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.hide_view(); } return true; });
        for (event of ['research_choice', 'colony_choice', 'colony_screen', 'turn', 'smac_ending', 'smac_specialist_menu', 'smac_rename_menu']) {
            this.window.listen(p.game, event, (e) => { this.hide_view(); });
        }
        this.window.listen(p.game, 'turn_status', (e) => { if (e.status == 'please_wait') { this.hide_view(); } });
        this.window.hide();
        return this.frame;
    },
    set: (data) => {
        this.hide_view(); this.base = data.base; this.list.clear();
        for (id in data.entries) {
            const facility_id = id;
            this.list.text({class: 'base-screen-frame-text', text: data.entries[id].name}).on('mousedown', (e) => {
                this.open(facility_id); return true;
            });
        }
    },
    open: (id) => {
        if (this.base == null) { return; }
        const data = this.base.get_smac_facility_details(id);
        if (!#is_defined(data)) { this.hide_view(); return; }
        this.content.clear(); this.title.text = data.name;
        if (#is_defined(data.art)) {
            this.content.surface({align: 'top left', left: 0, top: 0, width: 215, height: 215, background: data.art});
        } else {
            this.content.text({class: 'game-popup-text', left: 0, top: 40, text: 'No artwork available'});
        }
        const cost = #is_defined(data.cost) ? #to_string(data.cost) + ' minerals' : 'Unavailable';
        const maintenance = #is_defined(data.current_maintenance) ? #to_string(data.current_maintenance) + ' energy / turn' : 'Unavailable';
        const prerequisite = #is_defined(data.prerequisite) ? data.prerequisite : 'Unavailable';
        this.content.text({class: 'game-popup-text', left: 232, top: 0, text: 'Construction: ' + cost});
        this.content.text({class: 'game-popup-text', left: 232, top: 26, text: 'Current upkeep: ' + maintenance});
        this.content.text({class: 'game-popup-text', left: 232, top: 52, text: 'Requires: ' + prerequisite});
        const description = this.content.scrollview({left: 232, top: 84, right: 0, bottom: 0, has_hscroll: false, has_vscroll: true, vscroll_class: 'default-scroll-v'});
        let line = ''; let top = 0;
        let label = description.text({class: 'game-popup-text', left: 0, top: top, height: 20, text: ''});
        const text = data.description == '' ? 'Description unavailable.' : data.description;
        for (word of #split(text, ' ')) {
            const candidate = line == '' ? word : line + ' ' + word;
            label.text = candidate;
            if (label.measure().width > 350 && line != '') {
                label.text = line; top += 20; line = word;
                label = description.text({class: 'game-popup-text', left: 0, top: top, height: 20, text: line});
            } else { line = candidate; }
        }
        this.active = true; this.window.show();
    },
    reset: () => { this.hide_view(); this.base = null; this.list.clear(); },
    hide_view: () => { this.active = false; this.window.hide(); },
};
