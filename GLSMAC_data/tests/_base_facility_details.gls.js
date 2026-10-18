// Actual installed-facility view and callbacks, with DOM/assets doubled only.
const view = #include('../default/ui/parts/game/popup/base_screen/facilities');
return (game, base) => {
    const listeners = {};
    const node = (properties, interactive = true) => {
        const result = properties; result.children = []; result.handlers = {};
        result.on = (name, callback) => { if (!interactive && (name == 'mousedown' || name == 'click')) { throw TestError('Decorative Text cannot receive pointer input'); } result.handlers[name] = callback; };
        result.listen = (object, name, callback) => { listeners[name] = callback; };
        result.hide = () => {}; result.show = () => {};
        result.clear = () => { result.children = []; };
        result.measure = () => { return {width: #sizeof(#split(result.text, ' ')) * 70, height: 20}; };
        const child = (props) => { const value = node(props); result.children :+ value; return value; };
        result.panel = child; result.listview = child; result.window = child; result.button = child;
        result.surface = child; result.scrollview = child;
        if (!#is_defined(result.text)) { result.text = (props) => { const value = node(props, false); result.children :+value; return value; }; }
        return result;
    };
    const root = node({});
    view.init({body: root, ui: {root: root}, game: game});
    view.set({base: base, entries: base.get_facilities()});
    const check = (condition, reason) => { if (!condition) { throw TestError(reason); } };
    return (id, action) => {
        if (action == 'row') {
            check(view.list.children[0].handlers.mousedown({}), 'row callback did not consume pointer input');
            check(view.active && view.title.text != '', 'actual installed row did not open details');
            check(#sizeof(view.content.children) >= 5, 'detail view omitted artwork/information');
        } else if (action == 'open') { view.open(id); check(view.active, 'installed detail view did not reopen');
        } else if (action == 'close') {
            check(view.window.children[2].handlers.click({}), 'Close did not consume click');check(!view.active, 'Close kept details open');
        } else if (action == 'escape') {
            check(view.window.handlers.keydown({modifiers: {}, code: 'ESCAPE'}), 'Escape did not consume key');check(!view.active, 'Escape kept details open');
        } else if (action == 'turn') { listeners.turn({});check(!view.active, 'turn retained detail modal');
        } else if (action == 'refresh') { view.set({base: base, entries: base.get_facilities()});check(!view.active, 'Base refresh retained stale details');
        } else if (action == 'missing') { view.open(id);check(!view.active, 'removed installed facility opened a stale view');
        } else { throw TestError('Unknown detail test action'); }
    };
};
