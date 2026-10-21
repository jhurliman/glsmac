// Frozen source-derived43ca60 cases, not original executable input runs.
const input = #include('../default/ui/parts/game/popup/base_screen/workshop_picker_input');
let valid = []; let index = 0; while (index < 21) { valid :+ (index == 20 ? 0 : 1); index++; }
const cases = [
  {
    id: 'down_cross_unknown',
    before: {
      count: 21,
      selected: 15,
      first: 0,
      position: 0,
      availability: 1
    },
    key: 40,
    after: {
      count: 21,
      selected: 20,
      first: 5,
      position: 1,
      availability: 1
    },
    events: [
      [
        'scroll_position',
        1
      ],
      [
        'full_repaint'
      ],
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'down_last_row_scroll',
    before: {
      count: 21,
      selected: 20,
      first: 5,
      position: 1,
      availability: 1
    },
    key: 40,
    after: {
      count: 21,
      selected: 20,
      first: 10,
      position: 2,
      availability: 1
    },
    events: [
      [
        'scroll_position',
        2
      ],
      [
        'full_repaint'
      ],
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'down_last_limit',
    before: {
      count: 21,
      selected: 20,
      first: 10,
      position: 2,
      availability: 1
    },
    key: 40,
    after: {
      count: 21,
      selected: 20,
      first: 10,
      position: 2,
      availability: 1
    },
    events: [
      [
        'scroll_position',
        2
      ],
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'up_scroll',
    before: {
      count: 21,
      selected: 5,
      first: 5,
      position: 1,
      availability: 0
    },
    key: 38,
    after: {
      count: 21,
      selected: 0,
      first: 0,
      position: 0,
      availability: 0
    },
    events: [
      [
        'scroll_position',
        0
      ],
      [
        'full_repaint'
      ],
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'short_left',
    before: {
      count: 21,
      selected: 20,
      first: 5,
      position: 1,
      availability: 1
    },
    key: 37,
    after: {
      count: 21,
      selected: 20,
      first: 5,
      position: 1,
      availability: 1
    },
    events: [
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'short_right',
    before: {
      count: 21,
      selected: 20,
      first: 5,
      position: 1,
      availability: 1
    },
    key: 39,
    after: {
      count: 21,
      selected: 20,
      first: 5,
      position: 1,
      availability: 1
    },
    events: [
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'page_offscreen',
    before: {
      count: 21,
      selected: 0,
      first: 0,
      position: 0,
      availability: 1
    },
    key: 34,
    after: {
      count: 21,
      selected: 0,
      first: 10,
      position: 2,
      availability: 1
    },
    events: [
      [
        'scroll_position',
        2
      ],
      [
        'full_repaint'
      ],
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'home_stale_scroll',
    before: {
      count: 21,
      selected: 20,
      first: 10,
      position: 2,
      availability: 1
    },
    key: 36,
    after: {
      count: 21,
      selected: 0,
      first: 0,
      position: 2,
      availability: 1
    },
    events: [
      [
        'full_repaint'
      ],
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'end_stale_scroll',
    before: {
      count: 21,
      selected: 0,
      first: 0,
      position: 0,
      availability: 0
    },
    key: 35,
    after: {
      count: 21,
      selected: 20,
      first: 5,
      position: 0,
      availability: 0
    },
    events: [
      [
        'full_repaint'
      ],
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'selection_only_unknown',
    before: {
      count: 21,
      selected: 15,
      first: 5,
      position: 1,
      availability: 1
    },
    key: 40,
    after: {
      count: 21,
      selected: 20,
      first: 5,
      position: 1,
      availability: 0
    },
    events: [
      [
        'paint',
        15,
        false
      ],
      [
        'paint',
        20,
        true
      ],
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'escape_repaint_blank',
    before: {
      count: 21,
      selected: 15,
      first: 5,
      position: 1,
      availability: 0
    },
    key: 27,
    after: {
      count: 21,
      selected: (0 - 1),
      first: 5,
      position: 1,
      availability: 1
    },
    events: [
      [
        'close'
      ],
      [
        'paint',
        15,
        false
      ],
      [
        'paint', (0 - 1),
        true
      ],
      [
        'invalidate'
      ]
    ]
  },
  {
    id: 'extended_enter_no_close',
    before: {
      count: 21,
      selected: 15,
      first: 5,
      position: 1,
      availability: 1
    },
    key: 65549,
    after: {
      count: 21,
      selected: 15,
      first: 5,
      position: 1,
      availability: 1
    },
    events: [
      [
        'invalidate'
      ]
    ]
  }
];
for (row of cases) { const before = row.before + {}; const result = input.keyboard(before, row.key, valid); if (result.state != row.after || result.events != row.events || before != row.before) { #print(row.id); #print(result); #print(row.after); } test.assert(result.state == row.after && result.events == row.events && before == row.before); }

const state = {count: 21, selected: 0, first: 5, position: 1, availability: 1};
const hover = input.hover(state, 4, 286, valid);
test.assert(hover.state.selected == 20 && hover.state.availability == 0 && hover.events[0] == ['paint', 20, true] && hover.events[1] == ['paint', 0, false]);
const release = input.release(state, 4, 286, valid); test.assert(release.state.selected == (0 - 1) && release.events[#sizeof(release.events) - 1] == ['close']);
const outside = input.hover(state, 544, 4, valid); test.assert(outside.state.selected == (0 - 1) && outside.state.availability == 0 && #sizeof(outside.events) == 1);
const initial = {count: 21, selected: 3, first: 0, position: 0, availability: 0};
const enter = input.keyboard(initial, 13, valid); test.assert(enter.state == initial && enter.events == [['close'], ['invalidate']]);
const same = input.hover(initial, 328, 4, valid); test.assert(same.state == initial && same.events == []);
const same_release = input.release(initial, 328, 4, valid); test.assert(same_release.state.selected == (0 - 1));
// Signed division and global row clamps, including short catalogs.
for (key of [37,39]) { const result = input.keyboard({count: 4, selected: (0 - 1), first: 0, position: 0, availability: 0}, key, valid); test.assert(result.state.selected == 0); }
const down = input.keyboard({count: 9, selected: (0 - 1), first: 0, position: 0, availability: 0}, 40, valid); test.assert(down.state.selected == 4);
