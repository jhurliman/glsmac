// Shared checkbox transaction. Audio/Gamma previews have separate immediate
// effects in the original and deliberately do not pass through this model.
return (pages, read, defaults, save) => {
    const copy = (values) => {
        let result = {};
        for (page of pages) { for (row of page.rows) {
            const key = #to_string(row.id);
            if (!#is_defined(values[key]) || #typeof(values[key]) != 'Bool') { throw Error('Incomplete preference state'); }
            if (#is_defined(result[key])) { throw Error('Duplicate preference control'); }
            result[key] = values[key];
        } }
        return result;
    };
    const model = {
        active: false, page: 0, values: {},
        open: (page) => {
            if (this.active || page < 0 || page > 6) { return false; }
            const snapshot = copy(read());
            this.values = snapshot; this.page = page; this.active = true; return true;
        },
        set: (id, checked) => {
            const key = #to_string(id);
            if (!this.active || !#is_defined(this.values[key]) || #typeof(checked) != 'Bool') { return false; }
            this.values[key] = checked; return true;
        },
        select: (page) => {
            if (!this.active || page < 0 || page > 6) { return false; }
            this.page = page; return true;
        },
        more: () => {
            if (!this.active || (this.page != 2 && this.page != 6)) { return false; }
            this.page = this.page == 2 ? 6 : 2; return true;
        },
        restore: () => {
            if (!this.active) { return false; }
            this.values = copy(defaults()); return true;
        },
        finish: (accept) => {
            if (!this.active) { return false; }
            // A write failure must leave the modal and its draft available.
            if (accept) { save(copy(this.values)); }
            this.active = false; this.values = {}; return true;
        },
        key: (event) => {
            if (!this.active || event.modifiers != {}) { return false; }
            if (event.code == 'ESCAPE') { return this.finish(false); }
            if (event.code == 'ENTER') { return this.finish(true); }
            if (event.code == 'UP') { this.page = this.page == 0 ? 5 : this.page - 1; return true; }
            if (event.code == 'DOWN') { this.page = this.page >= 5 ? 0 : this.page + 1; return true; }
            return false;
        },
    };
    return model;
};
