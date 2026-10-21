#!/usr/bin/env python3
"""Reject known SDK macro identifiers before spending time on platform builds.

This is a deliberately narrow token check, not a C++ preprocessor. `small` is
an object-like char macro in the Windows RPC SDK (rpcndr.h); NOMINMAX does not
suppress it. Comments, literals, include names and preprocessor directives are
not identifier uses. Add names only after an observed SDK collision, with a
source and a repository audit; do not ban ordinary words speculatively.
"""
from pathlib import Path
import re
import sys

# Raw string delimiters have at most16 characters; matching the closing
# delimiter keeps a quoted source example from looking like production code.
TOKENS = re.compile(
    r'(?P<skip>//(?:\\\r?\n|[^\r\n])*|/\*.*?\*/|(?:u8|u|U|L)?R"(?P<delimiter>[^ ()\\\t\r\n]{0,16})\(.*?\)(?P=delimiter)"'
    r'|"(?:\\.|[^"\\])*"|\'(?:\\.|[^\'\\])*\'|^[ \t]*\#(?:[^\n\\]|\\[^\n]|\\\n)*)'
    r'|(?P<identifier>[A-Za-z_][A-Za-z_0-9]*)', re.DOTALL | re.MULTILINE)
FORBIDDEN = {'small': 'Windows RPC SDK rpcndr.h defines small as char'}


def collisions(text):
    """Return line/name/source for actual tokens, excluding quoted references."""
    return [(text.count('\n', 0, match.start()) + 1, match.group('identifier'),
             FORBIDDEN[match.group('identifier')])
            for match in TOKENS.finditer(text)
            if match.group('identifier') in FORBIDDEN]


def scan(root):
    found = []
    for path in sorted((root / 'src').rglob('*')):
        if path.is_file() and path.suffix in {'.h', '.hpp', '.cpp', '.cc', '.cxx'}:
            for line, name, source in collisions(path.read_text(encoding='utf-8-sig')):
                found.append(f'{path.relative_to(root).as_posix()}:{line}: identifier {name!r}: {source}; rename this identifier')
    return found


def main():
    errors = scan(Path(__file__).resolve().parents[1])
    if errors:
        print('\n'.join(errors), file=sys.stderr)
        return 1
    print('Known Windows SDK macro identifier check passed.')
    return 0


if __name__ == '__main__':
    sys.exit(main())
