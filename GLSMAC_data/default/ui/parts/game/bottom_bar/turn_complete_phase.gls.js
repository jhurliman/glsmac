// One native status redraw, not one graphics repaint. The timer reschedules
// from now and never catches up missed wall-clock intervals.
return () => {
    let counter = 0;
    let next = 0;
    return (animate) => {
        if (!animate) { counter = 0; next = 0; }
        if (counter >= 19) { counter = 0; next = 1; }
        const frame = next;
        next++; counter++;
        return frame;
    };
};
