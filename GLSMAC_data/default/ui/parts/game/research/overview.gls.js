return {
    init: (p) => {
        this.p = p; this.active = false;
        this.window = p.root.window({class: 'game-popup', zindex: 0.9, width: 620, height: 420, blocker: 'rgba(0, 0, 0, 180)'});
        this.window.hide();
        this.heading = this.window.text({class: 'game-popup-text', left: 12, top: 12, right: 12, text: 'Research status'});
        this.status = this.window.text({class: 'game-popup-text', left: 12, top: 42, right: 12, text: ''});
        this.labs = this.window.text({class: 'game-popup-text', left: 12, top: 70, right: 12, text: ''});
        this.known = this.window.scrollview({left: 12, right: 12, top: 106, bottom: 52, has_vscroll: true, has_hscroll: false, vscroll_class: 'default-scroll-v'});
        this.window.button({class: 'game-popup-button', left: 12, right: 12, bottom: 12, height: 28, text: 'Close'}).on('click', () => { this.close(); return true; });
        this.window.on('keydown', (e) => { if (e.modifiers == {} && e.code == 'ESCAPE') { this.close(); } return true; });
        for (event of ['turn', 'research_choice', 'smac_ending']) { this.window.listen(p.game, event, (e) => { this.close(); }); }
    },
    open: () => {
        const p = this.p;
        if (p.game.get_ruleset() != 'smac' || p.modules.popup.popup != null) { return; }
        for (name of ['first_base', 'research', 'colony']) {
            const module = p.modules[name];
            if (#is_defined(module) && #is_defined(module.pending) && module.pending != null) { return; }
        }
        const data = p.game.get_technology_state(p.game.get_player());
        this.status.text = data.blind ? 'Blind research' : (#is_defined(data.target_name) ? 'Researching: ' + data.target_name : 'No technology selected');
        if (data.blind && #sizeof(data.priorities) == 4) {
            let priorities = ''; const names = ['Explore', 'Discover', 'Build', 'Conquer'];
            for (let i = 0; i < 4; i++) { if (data.priorities[i] != 0) { priorities += (priorities == '' ? '' : ', ') + names[i]; } }
            this.status.text = 'Priorities: ' + (priorities == '' ? 'none' : priorities);
        }
        this.labs.text = 'Accumulated labs: ' + #to_string(data.labs) + '  |  Known technologies: ' + #to_string(#sizeof(data.known_names));
        this.known.clear();let top = 0;
        for (name of data.known_names) { this.known.text({class: 'game-popup-text', top: top, left: 4, text: name}); top += 24; }
        p.modules.menu.close_all();this.active = true;this.window.show();
    },
    close: () => { this.active = false;this.window.hide(); },
};
