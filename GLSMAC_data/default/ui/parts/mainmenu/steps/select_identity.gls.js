return (i, faction) => {
    const defaults = #clone(faction.display_identity);
    let draft = #clone(defaults);
    // DOM properties are references in GSE. Materialize text before replacing
    // a popup, so the draft never points into an input that has been removed.
    const start = (status) => {
        let accepted = false;
        try { i.glsmac.start_game(faction.id, draft); accepted = true; }
        catch { : (error) => { status.text = error.reason; } }
        if (accepted) { i.popup.dismiss(); }
    };
    const customize = () => {
        let fields = [];
        let status = null;
        const accept = () => {
            draft.leader_name = '' + fields[0].value; draft.faction_name = '' + fields[1].value;
            draft.setup.noun = '' + fields[2].value; draft.setup.adjective = '' + fields[3].value;
            draft.leader_title = '' + fields[4].value; draft.setup.description = '' + fields[5].value;
            start(status); return true;
        };
        i.popup.show({
            title: 'Customize Your Faction', width: 600, height: 320,
            generator: (body) => {
                const rows = [
                    ['Name:', draft.leader_name], ['Formal:', draft.faction_name],
                    ['Noun:', draft.setup.noun], ['Adjective:', draft.setup.adjective],
                    ['Title:', draft.leader_title], ['Description:', draft.setup.description],
                ];
                let top = 12;
                for (row of rows) {
                    body.text({class: 'popup-text', left: 12, top: top, text: row[0]});
                    const field = body.input({class: 'popup-input', left: 140, right: 12, top: top, value: '' + row[1]});
                    field.on('input', accept);
                    fields :+field; top += 30;
                }
                body.text({class: 'popup-text', left: 12, top: top, text: 'Gender: ' + (draft.setup.female ? 'Female' : 'Male')});
                status = body.text({class: 'popup-text', left: 12, right: 12, top: top + 30, text: ''});
            },
            buttons: [
                {style: {text: 'Restore Defaults', align: 'left', width: 190}, onclick: () => {
                    draft = #clone(defaults);
                    const values = [draft.leader_name, draft.faction_name, draft.setup.noun, draft.setup.adjective, draft.leader_title, draft.setup.description];
                    for (let index = 0; index < #sizeof(fields); index++) { fields[index].value = '' + values[index]; }
                    status.text = ''; return true;
                }},
                {style: {text: 'OK', align: 'center', width: 190, is_ok: true}, onclick: accept},
                {style: {text: 'Cancel', align: 'right', width: 190, is_cancel: true}, onclick: () => { i.popup.back(); return true; }},
            ],
        });
    };
    let input = null;
    let status = null;
    const accept = () => { draft.leader_name = '' + input.value; start(status); return true; };
    i.popup.show({
        title: 'ENTER YOUR NAME', width: 600, height: 150,
        generator: (body) => {
            body.text({class: 'popup-text', left: 12, top: 12, text: 'Name (' + (draft.setup.female ? 'Female' : 'Male') + '):'});
            input = body.input({class: 'popup-input', left: 140, right: 12, top: 12, value: draft.leader_name});
            input.on('input', accept);
            status = body.text({class: 'popup-text', left: 12, right: 12, top: 48, text: ''});
        },
        buttons: [
            {style: {text: 'Customize', align: 'left', width: 190}, onclick: () => { draft.leader_name = '' + input.value; customize(); return true; }},
            {style: {text: 'OK', align: 'center', width: 190, is_ok: true}, onclick: accept},
            {style: {text: 'Cancel', align: 'right', width: 190, is_cancel: true}, onclick: () => { i.popup.back(); return true; }},
        ],
    });
};