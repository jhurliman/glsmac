import importlib.util
from pathlib import Path
import tempfile
import unittest

spec = importlib.util.spec_from_file_location('windows_macros', Path(__file__).resolve().parents[1] / 'check_windows_macros.py')
policy = importlib.util.module_from_spec(spec)
spec.loader.exec_module(policy)


class WindowsMacroTests(unittest.TestCase):
    def test_report_declaration_and_uses_but_not_prefixes(self):
        actual = policy.collisions('int small = 1;\nconsume(small);\nint small_world = 2;')
        self.assertEqual([(line, name) for line, name, _ in actual], [(1, 'small'), (2, 'small')])

    def test_ignore_documentation_literals_and_sdk_directives(self):
        text = r'''// small
/* small */ const char* s = "small\" small";
char quote = '\''; auto raw = u8R"tag(small " inside )tag";
#include <small.h>
#define SDK_WORD small
#undef small
int fine;
'''
        self.assertEqual(policy.collisions(text), [])

    def test_line_spliced_comment_is_not_code(self):
        self.assertEqual(policy.collisions('// continued ' + chr(92) + '\nsmall\nint actual;'), [])

    def test_literal_then_actual_identifier_keeps_line_number(self):
        self.assertEqual([(line, name) for line, name, _ in policy.collisions('R"(small\n)";\nreturn small;')], [(3, 'small')])

    def test_scan_checks_native_tests_and_production_only(self):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            (root / 'src/tests').mkdir(parents=True)
            (root / 'src/tests/case.cpp').write_text('int small;', encoding='utf-8')
            (root / 'src/record.h').write_text('int small_record;', encoding='utf-8')
            (root / 'notes.md').write_text('small', encoding='utf-8')
            found = policy.scan(root)
            self.assertEqual(len(found), 1)
            self.assertTrue(found[0].startswith('src/tests/case.cpp:1:'))


if __name__ == '__main__':
    unittest.main()
