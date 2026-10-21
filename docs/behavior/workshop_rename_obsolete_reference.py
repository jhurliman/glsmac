"""Workshop action5/6 scalar transitions; input strings represent original bytes."""


def rename(draft_name, flags, generated_default, accepted, submitted=''):
    default = draft_name
    if not default:
        default = generated_default
        flags &= ~2
    default = default[:31]
    if not accepted:
        return draft_name, flags, default
    submitted = submitted[:31]
    if not submitted:
        return '', flags & ~2, default
    if submitted != default:
        return submitted, flags | 2, default
    return draft_name, flags, default


def toggle(flags, excluded, owner):
    owner_bit = (1 << owner) & 255
    if excluded & owner_bit:
        if flags & 8:
            flags |= 0x200
        flags |= 8
    else:
        flags &= 0xfdf7
    return flags, excluded ^ owner_bit


def old_list_next(selected, old_list, offset):
    if not old_list:
        return selected
    for index in range(offset, offset + len(old_list)):
        if old_list[index % len(old_list)] == selected:
            return old_list[(index + 1) % len(old_list)]
    return selected


def rebuilt_strip_offset(selected, new_list, old_offset):
    if selected not in new_list:
        return old_offset
    return (new_list.index(selected) + len(new_list) - 5) % len(new_list)
