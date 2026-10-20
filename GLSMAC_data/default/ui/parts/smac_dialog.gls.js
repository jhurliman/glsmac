// Cold-launch SMAC mode0 templates. Coordinates are logical1024x768 pixels.
return (root, kind, title) => {
    const faction = kind == 'faction';
    const console = kind == 'first_base';
    const width = faction ? 547 : (console ? 580 : 682);
    const height = faction ? 218 : (console ? 300 : 114);
    const body_width = faction ? 525 : (console ? 480 : 660);
    const body_height = faction ? 130 : 26;
    const shift = console ? 23 : 0;
    const frame_right = console ? width - 55 : width;
    const body_bottom = faction ? 171 : 67;
    let placement = {align: 'bottom right', right: 5, bottom: 5};
    if (console) { placement = {align: 'top left', left: 238, top: 255}; }
    const window = root.window({class: 'smac-original-window', title: title,
        width: width, height: height, header_height: 0, header_background: 'transparent',
        blocker: 'transparent', overflow: 'hidden',
        smac_frame: [[9, shift, 0, frame_right, console ? 305 : height],
            [3, shift + 4, 4, frame_right - 4, 28],
            [6, shift + 7, 7, frame_right - 7, 25],
            [3, shift + 7, 35, shift + body_width + 13, body_bottom]]} + placement);
    if (console) {
        // Original console-visible template clips the outer two rail pieces
        // below the window. The inner rails retain their native529px height.
        window.surface({left: 15, top: 0, width: 8, height: 529,
            background: 'console_x2.pcx:smacsoftware():crop(729, 1, 736, 529)'});
        window.surface({left: width - 55, top: 0, width: 8, height: 529,
            background: 'console_x2.pcx:smacsoftware():crop(738, 1, 745, 529)'});
    }
    window.text({font: 'arialbd.ttf:14:mono', color: 'rgb(109, 126, 178)',
        transform: 'uppercase', align: 'top center', left: shift + 10,
        right: console ? 65 : 10, top: 10, text: title});
    // The original caption baseline is21, independently of ink-box height.

    const body = window.panel({left: shift + 10, top: 38, width: body_width, height: body_height});
    const footer = window.panel({left: shift + 10, top: faction ? 183 : 79,
        width: body_width, height: 20});
    return {window: window, body: body, footer: footer, button_width: faction ? 171 : (console ? 480 : 216), button_stride: faction ? 177 : (console ? 0 : 222)};
};
