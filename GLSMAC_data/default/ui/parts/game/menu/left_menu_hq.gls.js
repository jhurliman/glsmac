return {
    init: (p) => {
        return p.create([{label: 'Design Workshop...  U', open: () => { p.open_workshop(); }},
            {label: 'Research status...', open: () => { p.open_research(); }}]);
    },
};
