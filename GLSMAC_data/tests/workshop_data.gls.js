// Eleven original DATA fragments; raw brace spans have equal foreground colors.
const data = #include('../default/ui/parts/game/popup/base_screen/workshop_data');
const labels = ['VEHICLE DESIGN', 'Moves:', 'Type:', 'Power:', 'PROTOTYPE COST:', 'COST:', 'Data', 'PRESETS', 'W:', 'A:', 'C:', 'R:', 'SP1:', 'SP2:', 'E:', 'P:'];
const cases = [
  {
    name: 'scout',
    data: {
      offense: 1,
      weapon_mode: 0,
      weapon_cost: 1,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: 1,
      armor_class: 0
    },
    slots: [ (0 - 1), (0 - 1)
    ],
    reactor: 1,
    expected: [
      'W: 1',
      'A: 1',
      'C: 1',
      'R: 1',
      'SP1: -',
      'SP2: -'
    ]
  },
  {
    name: 'colony',
    data: {
      offense: 0,
      weapon_mode: 8,
      weapon_cost: 10,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: 1,
      armor_class: 0
    },
    slots: [ (0 - 1), (0 - 1)
    ],
    reactor: 1,
    expected: [
      'E: 10',
      'A: 1',
      'C: 1',
      'R: 1',
      'SP1: -',
      'SP2: -'
    ]
  },
  {
    name: 'air_super',
    data: {
      offense: 1,
      weapon_mode: 0,
      weapon_cost: 1,
      movement: 9,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: 1,
      armor_class: 0
    },
    slots: [ (0 - 1), (0 - 1)
    ],
    reactor: 2,
    expected: [
      'W: 1',
      'A: 1',
      'C: 9',
      'R: 2',
      'SP1: -',
      'SP2: -'
    ]
  },
  {
    name: 'slots_0_1',
    data: {
      offense: 1,
      weapon_mode: 0,
      weapon_cost: 1,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: 1,
      armor_class: 0
    },
    slots: [
      0,
      1
    ],
    reactor: 1,
    expected: [
      'W: 1',
      'A: 1',
      'C: 1',
      'R: 1',
      'SP1:F',
      'SP2:R'
    ]
  },
  {
    name: 'slots_1_0',
    data: {
      offense: 1,
      weapon_mode: 0,
      weapon_cost: 1,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: 1,
      armor_class: 0
    },
    slots: [
      1,
      0
    ],
    reactor: 1,
    expected: [
      'W: 1',
      'A: 1',
      'C: 1',
      'R: 1',
      'SP1:R',
      'SP2:F'
    ]
  },
  {
    name: 'slot_gap',
    data: {
      offense: 1,
      weapon_mode: 0,
      weapon_cost: 1,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: 1,
      armor_class: 0
    },
    slots: [ (0 - 1),
      1
    ],
    reactor: 1,
    expected: [
      'W: 1',
      'A: 1',
      'C: 1',
      'R: 1',
      'SP1: -',
      'SP2:R'
    ]
  },
  {
    name: 'psi',
    data: {
      offense: (0 - 1),
      weapon_mode: 2,
      weapon_cost: 10,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: (0 - 1),
      armor_class: 2
    },
    slots: [ (0 - 1), (0 - 1)
    ],
    reactor: 2,
    expected: [
      'W: Psi',
      'A: Psi',
      'C: 1',
      'R: 2',
      'SP1: -',
      'SP2: -'
    ]
  },
  {
    name: 'declared_zero',
    data: {
      offense: 0,
      weapon_mode: 0,
      weapon_cost: 1,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: 0,
      armor_class: 0
    },
    slots: [ (0 - 1), (0 - 1)
    ],
    reactor: 1,
    expected: [
      'W: 0',
      'A: 0',
      'C: 1',
      'R: 1',
      'SP1: -',
      'SP2: -'
    ]
  },
  {
    name: 'declared_negative_two',
    data: {
      offense: (0 - 2),
      weapon_mode: 0,
      weapon_cost: 1,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: (0 - 2),
      armor_class: 0
    },
    slots: [ (0 - 1), (0 - 1)
    ],
    reactor: 1,
    expected: [
      'W: Psi',
      'A: Psi',
      'C: 1',
      'R: 1',
      'SP1: -',
      'SP2: -'
    ]
  },
  {
    name: 'declared_99',
    data: {
      offense: 99,
      weapon_mode: 0,
      weapon_cost: 1,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: 99,
      armor_class: 0
    },
    slots: [ (0 - 1), (0 - 1)
    ],
    reactor: 1,
    expected: [
      'W: *',
      'A: 99',
      'C: 1',
      'R: 1',
      'SP1: -',
      'SP2: -'
    ]
  },
  {
    name: 'declared_armor_class3',
    data: {
      offense: 1,
      weapon_mode: 0,
      weapon_cost: 1,
      movement: 1,
      ability_names: [
        'Super Former',
        'Deep Radar'
      ]
    },
    armor: {
      rating: 1,
      armor_class: 3
    },
    slots: [ (0 - 1), (0 - 1)
    ],
    reactor: 1,
    expected: [
      'W: 1',
      '',
      'C: 1',
      'R: 1',
      'SP1: -',
      'SP2: -'
    ]
  }
];
for (row of cases) { const actual = data(labels, 'Psi', row.data, row.armor, row.slots, row.reactor); if (actual != row.expected) { #print(row.name); #print(actual); } test.assert(actual == row.expected); }
