const create = #include('../default/ui/parts/smac_checkbox');
let nodes = []; let callback = null; let values = [];
const child = (properties) => { nodes :+ properties; return properties; };
const container = {button: (properties) => {
    properties.surface = child;
    properties.on = (event, handler) => { test.assert(event == 'click'); callback = handler; };
    return properties;
}};
const control = create(container, 256, 34, 457, 'Design units automatically.', false, (value) => { values :+ value; });
test.assert(control.element.left == 256 && control.element.top == 34 && control.element.width == 457 && control.element.height == 24);
test.assert(nodes[0].left == 0 && nodes[0].top == 3 && nodes[0].width == 18 && nodes[0].height == 18 && !nodes[0].smac_checkbox);
test.assert(control.element.textleft == 18 && control.element.textbaseline == 19 && control.element.font == 'arialn.ttf:16:mono-bold');
test.assert(control.element.text == ' Design units automatically.' && control.element.color == 'rgb(120,164,212)');
test.assert(callback({x: 456, y: 23}) && control.checked && nodes[0].smac_checkbox && values == [true]);
test.assert(callback({x: 18, y: 0}) && !control.checked && !nodes[0].smac_checkbox && values == [true, false]);
// Reset/reopen changes draft appearance without masquerading as a user edit.
control.set(true); test.assert(control.checked && nodes[0].smac_checkbox && #sizeof(values) == 2);
control.set(false); test.assert(!control.checked && !nodes[0].smac_checkbox && #sizeof(values) == 2);
