// Source43ca60/43c880. Availability is painter state, not a navigation filter.
let input = {};
input = {
    paint: (state, ordinal, selected, valid, events) => {
        events :+ ['paint', ordinal, selected];
        if (ordinal < state.first || ordinal >= state.first + 20 || ordinal >= state.count) { return 1; }
        return valid[ordinal];
    },
    keyboard: (state, key, valid) => {
        const q = state + {}; let events = [];
        const old_s = q.selected; const old_p = q.first; const n = q.count;
        let s = old_s; const p = old_p;
        const position = (value) => {
            const maximum = #floor(#to_float(n) / 5.0) - 2;
            q.position = value < 0 ? 0 : (value > maximum ? maximum : value);
            events :+ ['scroll_position', q.position];
        };
        if (key == 27) { q.selected = (0 - 1); events :+ ['close']; }
        else if (key == 13) { events :+ ['close']; }
        else if (key == 38 || key == 104) {
            if (s < 5) { return {state: q, events: events}; }
            s -= 5; while (s >= p + 20) { s -= 5; } q.selected = s;
            if (s < p) { q.first = p - 5; position(q.position - 1); }
        } else if (key == 40 || key == 98) {
            if (s > n - 6) {
                if (n <= 20) { return {state: q, events: events}; }
                position(q.position + 1); q.first = 5 * q.position;
            } else {
                s += 5; while (s < p) { s += 5; } q.selected = s;
                if (s >= p + 20) { q.first = p + 5; position(q.position + 1); }
            }
        } else if (key == 37 || key == 100 || key == 39 || key == 102) {
            const quotient = #to_float(s) / 5.0;
            const lo = 5 * (s < 0 ? #ceil(quotient) : #floor(quotient));
            const hi = lo + 4 < n - 1 ? lo + 4 : n - 1;
            const candidate = s + (key == 37 || key == 100 ? (0 - 1) : 1);
            q.selected = candidate < lo || hi < lo ? lo : (candidate < hi ? candidate : hi);
        } else if (key == 36) { q.first = 0; q.selected = 0; }
        else if (key == 35) {
            q.selected = n - 1;
            if (n > 20) { q.first = 5 * (#floor(#to_float(n - 1) / 5.0) + 1) - 20; }
        } else if (key == 33 || key == 34) {
            if (n <= 20) { return {state: q, events: events}; }
            position(q.position + (key == 33 ? (0 - 3) : 3)); q.first = 5 * q.position;
        }
        if (old_p != q.first) { events :+ ['full_repaint']; }
        else if (old_s != q.selected) {
            input.paint(q, old_s, false, valid, events);
            q.availability = input.paint(q, q.selected, true, valid, events);
        }
        events :+ ['invalidate']; return {state: q, events: events};
    },
    hover: (state, x, y, valid) => {
        const q = state + {}; let events = []; const s = q.selected;
        if (x >= 4 && x < 544 && y >= 4 && y < 380) {
            const candidate = q.first + 5 * #floor(#to_float(y - 4) / 94.0) + #floor(#to_float(x - 4) / 108.0);
            if (candidate == s) { return {state: q, events: events}; }
            if (candidate < q.count) {
                q.availability = input.paint(q, candidate, true, valid, events);
                if (s >= 0) { input.paint(q, s, false, valid, events); }
                q.selected = candidate; events :+ ['invalidate']; return {state: q, events: events};
            }
        }
        if (s >= 0) { input.paint(q, s, false, valid, events); }
        q.availability = 0; q.selected = (0 - 1); return {state: q, events: events};
    },
    release: (state, x, y, valid) => {
        const result = input.hover(state, x, y, valid);
        if (result.state.availability == 0) { result.state.selected = (0 - 1); }
        result.events :+ ['release_owned_capture']; result.events :+ ['close']; return result;
    },
};
return input;
