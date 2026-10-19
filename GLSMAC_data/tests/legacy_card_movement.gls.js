const preview = #include('../default/ui/parts/game/bottom_bar/object_preview');
// Exercise the real legacy card formatter and actual GSE numeric builtins.
test.assert(preview.format_movement(0.0) == '0', 'legacy zero movement gained a decimal suffix');
test.assert(preview.format_movement(1.0) == '1', 'legacy whole movement gained a decimal suffix');
test.assert(preview.format_movement(2.0) == '2', 'legacy multi-move allowance gained a decimal suffix');
test.assert(preview.format_movement(1.999) == '2', 'legacy rounded whole allowance gained a decimal suffix');
// Legacy records have no retained quantum. Preserve their existing decimal
// precision rather than inventing native ground fractions or aircraft state.
test.assert(preview.format_movement(0.6666666667) == '0.67', 'legacy decimal rounding changed');
test.assert(preview.format_movement(1.25) == '1.25', 'legacy decimal allowance changed');
