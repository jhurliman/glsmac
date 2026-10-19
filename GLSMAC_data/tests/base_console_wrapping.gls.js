// Literal current-GDI widths from the original Base-card receipt; no font asset.
const wrap = #include('../default/ui/parts/game/bottom_bar/wrapped_lines');
const rows0 = wrap('New Austin', (text) => { if (text == 'New') { return 25; } if (text == 'New Austin') { return 64; } if (text == 'Austin') { return 36; } throw TestError('Unexpected measured candidate: ' + text); }, 116);
test.assert(#sizeof(rows0) == 1, 'Original Base wrap row count');
test.assert(rows0[0] == 'New Austin', 'Original Base wrapped substring including separator');
const rows1 = wrap('Producing: Scout Patrol (4/10)', (text) => { if (text == 'Producing:') { return 59; } if (text == 'Producing: Scout') { return 93; } if (text == 'Producing: Scout Patrol') { return 128; } if (text == 'Producing: Scout Patrol (4/10)') { return 163; } if (text == 'Scout') { return 31; } if (text == 'Scout Patrol') { return 66; } if (text == 'Scout Patrol (4/10)') { return 101; } if (text == 'Patrol') { return 32; } if (text == 'Patrol (4/10)') { return 67; } if (text == '(4/10)') { return 32; } throw TestError('Unexpected measured candidate: ' + text); }, 116);
test.assert(#sizeof(rows1) == 2, 'Original Base wrap row count');
test.assert(rows1[0] == 'Producing: Scout ', 'Original Base wrapped substring including separator');
test.assert(rows1[1] == 'Patrol (4/10)', 'Original Base wrapped substring including separator');
const rows2 = wrap('University Research City', (text) => { if (text == 'University') { return 56; } if (text == 'University Research') { return 114; } if (text == 'University Research City') { return 138; } if (text == 'Research') { return 55; } if (text == 'Research City') { return 79; } if (text == 'City') { return 21; } throw TestError('Unexpected measured candidate: ' + text); }, 116);
test.assert(#sizeof(rows2) == 2, 'Original Base wrap row count');
test.assert(rows2[0] == 'University Research ', 'Original Base wrapped substring including separator');
test.assert(rows2[1] == 'City', 'Original Base wrapped substring including separator');
const rows3 = wrap('Producing: Isle of the Deep (74/80)', (text) => { if (text == 'Producing:') { return 59; } if (text == 'Producing: Isle') { return 82; } if (text == 'Producing: Isle of') { return 95; } if (text == 'Producing: Isle of the') { return 115; } if (text == 'Producing: Isle of the Deep') { return 148; } if (text == 'Producing: Isle of the Deep (74/80)') { return 190; } if (text == 'Isle') { return 20; } if (text == 'Isle of') { return 33; } if (text == 'Isle of the') { return 53; } if (text == 'Isle of the Deep') { return 86; } if (text == 'Isle of the Deep (74/80)') { return 128; } if (text == 'of') { return 10; } if (text == 'of the') { return 30; } if (text == 'of the Deep') { return 63; } if (text == 'of the Deep (74/80)') { return 105; } if (text == 'the') { return 17; } if (text == 'the Deep') { return 50; } if (text == 'the Deep (74/80)') { return 92; } if (text == 'Deep') { return 30; } if (text == 'Deep (74/80)') { return 72; } if (text == '(74/80)') { return 39; } throw TestError('Unexpected measured candidate: ' + text); }, 116);
test.assert(#sizeof(rows3) == 2, 'Original Base wrap row count');
test.assert(rows3[0] == 'Producing: Isle of the ', 'Original Base wrapped substring including separator');
test.assert(rows3[1] == 'Deep (74/80)', 'Original Base wrapped substring including separator');
const equal = wrap('equal fits', (text) => { return 116; }, 116);
test.assert(equal[0] == 'equal fits', 'Native equality must fit');
