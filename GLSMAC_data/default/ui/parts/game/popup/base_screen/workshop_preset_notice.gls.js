let view = {};
view = {
    init: (p) => {
        view.active = false;
        view.window = p.ui.root.window({class: 'smac-original-window', zindex: 0.98, align: 'top left', left: 149, top: 255, width: 758, height: 300,
            background: 'transparent', border: 'transparent', blocker: 'rgba(0,0,0,180)', overflow: 'hidden'});
        view.art = view.window.surface({left: 0, top: 0, width: 758, height: 300,
            smac_frame: [[9,23,0,703,305],[3,27,4,699,28],[6,30,7,696,25],[0,236,35,696,58],[3,233,32,699,61]]});
        view.window.surface({left: 15, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(729,1,736,529)'});
        view.window.surface({left: 703, top: 0, width: 8, height: 529, background: 'console_x2.pcx:smacsoftware():crop(738,1,745,529)'});
        view.window.surface({left: 28, top: 32, width: 200, height: 120, background: 'unit_sm.pcx:smacsoftware()'});
        const caption = view.window.panel({left: 30, top: 0, width: 666, height: 28, overflow: 'hidden'});
        view.title = caption.text({baseline: 21, font: 'arialbd.ttf:14:mono'});
        view.body = view.window.text({left: 239, baseline: 50, font: 'arialn.ttf:16:mono-bold'});
        p.ui.class('workshop-preset-ok').set({font: 'arialn.ttf:16:mono-bold', textalign: 'top center', textbaseline: 16,
            smac_button_sprite: [0,658], _hover: {smac_button_sprite: [1,658]}, _active: {smac_button_sprite: [2,658]}});
        view.ok = view.window.button({class: 'workshop-preset-ok', left: 33, top: 167, width: 658, height: 20});
        view.ok.on('click', () => { view.dismiss(); return true; });
        view.window.on('keydown', (e) => {
            if ((e.modifiers == {} && (e.code == 'ENTER' || e.code == 'ESCAPE')) || (e.modifiers == {shift: true} && e.code == 'ENTER')) { view.dismiss(); } return true;
        }); view.window.hide();
    },
    open: (workshop) => {
        view.workshop = workshop;
        const words = view.art.smacworkshoppresettext(); view.title.text = #uppercase(words.title); view.title.color = words.colors[226];
        view.title.left = (666 - view.title.measure().width) / 2; view.body.text = words.body; view.body.color = words.colors[226];
        view.ok.text = words.ok; view.ok.color = words.colors[0]; view.active = true; view.window.show();
    },
    dismiss: () => { if (!view.active) { return; } const workshop = view.workshop; view.hide(); workshop.finish_preset(39); },
    hide: () => { view.active = false; view.workshop = null; view.window.hide(); },
};
return view;
