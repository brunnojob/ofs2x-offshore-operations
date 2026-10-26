import subprocess
import unittest


class CommandLineTests(unittest.TestCase):
    def run_cli(self, commands):
        return subprocess.run(['./build/offshore'], input=commands, text=True, capture_output=True, check=False)

    def test_complete_drill(self):
        result = self.run_cli('crew alice vessel-1 1 BST 200\nstation muster 1 BST\nstart drill-1 vessel-1\ncheck-in alice muster 100\nstatus\nfinish\nstatus\nquit\n')
        self.assertEqual(result.returncode, 0)
        self.assertEqual(result.stderr, '')
        self.assertIn('accepted\nactive\nmuster 1/1 shortage=0\n', result.stdout)
        self.assertIn('seconds\ninactive\n', result.stdout)

    def test_extra_arguments_do_not_mutate_state(self):
        result = self.run_cli('station muster 1\nstart drill-1 vessel-1 extra\nstatus\nquit\n')
        self.assertEqual(result.returncode, 1)
        self.assertIn('unexpected argument: extra', result.stderr)
        self.assertEqual(result.stdout, 'inactive\nmuster 0/1 shortage=1\n')

    def test_fixed_arity_commands(self):
        for command in ['status extra', 'finish extra', 'audit extra', 'help extra', 'quit extra', 'check-in alice muster 100 extra']:
            with self.subTest(command=command):
                result = self.run_cli(command + '\nquit\n')
                self.assertEqual(result.returncode, 1)
                self.assertIn('unexpected argument: extra', result.stderr)
                self.assertEqual(result.stdout, '')

    def test_invalid_input(self):
        for command in ['unknown', 'crew alice vessel-1 2', 'crew alice vessel-1 1 BST nope', 'station muster -1', 'crew alice vessel-1 1 BST']:
            with self.subTest(command=command):
                result = self.run_cli(command + '\nquit\n')
                self.assertEqual(result.returncode, 1)
                self.assertIn('error:', result.stderr)

    def test_blank_lines(self):
        result = self.run_cli('\n \t\nquit\n')
        self.assertEqual((result.returncode, result.stdout, result.stderr), (0, '', ''))


if __name__ == '__main__':
    unittest.main()
