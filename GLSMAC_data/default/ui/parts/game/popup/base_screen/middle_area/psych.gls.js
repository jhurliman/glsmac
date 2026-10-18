return {
    init: (p) => {
        this.el = p.area.panel({class: 'base-screen-middle-area'});
        this.items = this.el.listview({
            left: 4, right: 4, top: 4, bottom: 4, itemsize: 20,
            has_hscroll: true, has_vscroll: true,
            hscroll_class: 'default-scroll-h', vscroll_class: 'default-scroll-v',
        });
    },
    set: (presentation) => {
        this.items.clear();
        const line = (text) => { this.items.text({class: 'base-screen-frame-text', text: text}); };
        if (#typeof(presentation) != 'Object' || #typeof(presentation.current) != 'Object') {
            let reason = 'Citizen/Psych details unavailable';
            if (#typeof(presentation) == 'Object' && #typeof(presentation.reason) == 'String' && presentation.reason != '') { reason = presentation.reason; }
            line(reason);
            return;
        }
        const c = presentation.current;
        line('Population ' + #to_string(c.population) + '; specialists ' + #to_string(c.specialists));
        line('Psych ' + #to_string(c.psych_points) + '; Police ' + #to_string(c.police));
        const counts = (n) => { return 'T ' + #to_string(n.talents) + ' / C ' + #to_string(n.content) + ' / D ' + #to_string(n.drones); };
        const stages = [
            {name: 'Initial', value: c.initial}, {name: 'Psych', value: c.after_psych},
            {name: 'Facilities', value: c.after_facilities}, {name: 'Police', value: c.after_police},
            {name: 'Final', value: c.final},
        ];
        for (stage of stages) { line(stage.name + ': ' + counts(stage.value)); }
        if (c.final.extra_drone_steps > 0) { line('Additional drone pressure: ' + #to_string(c.final.extra_drone_steps)); }
        if (#typeof(presentation.retained) == 'Object') { line('Last pass: ' + counts(presentation.retained.final)); }
        else { line('Last pass unavailable'); }
    },
};
