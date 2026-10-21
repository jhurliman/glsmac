// Stock MainFontSize16 checklist row; its caller owns draft/accept semantics.
return (container, left, top, width, label, checked, changed) => {
    const row = container.button({left: left, top: top, width: width, height: 24,
        background: 'transparent', border: 'transparent', overflow: 'hidden',
        font: 'arialn.ttf:16:mono-bold', color: 'rgb(120,164,212)',
        textalign: 'top left', textleft: 18, textbaseline: 19, text: ' ' + label});
    const icon = row.surface({left: 0, top: 3, width: 18, height: 18, smac_checkbox: checked});
    const control = {element: row, checked: checked, set: (value) => {
        this.checked = value; icon.smac_checkbox = value;
    }};
    row.on('click', () => { control.set(!control.checked); changed(control.checked); return true; });
    return control;
};
