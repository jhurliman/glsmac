// Native-style wrapping retains the separator on each completed wrapped row.
// Admission measures the next word without a trailing separator; the final
// row does not gain an invented trailing space. Callers supply actual font
// measurement and the source layout's admitted width.
return (text, measure, width) => {
    let rows = [];
    let current = '';
    for (word of #split(text, ' ')) {
        const candidate = current == '' ? word : current + ' ' + word;
        if (current != '' && measure(candidate) > width) {
            rows :+ current + ' ';
            current = word;
        } else { current = candidate; }
    }
    rows :+ current;
    return rows;
};
