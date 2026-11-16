import contextlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch


SCRIPT = Path(__file__).resolve().parents[1] / "tools/configure.py"
SPEC = importlib.util.spec_from_file_location("configure", SCRIPT)
configure = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(configure)


class ConfigureTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        self.root = Path(self.directory.name)
        self.local = self.root / ".secrets"
        root_patch = patch.object(configure, "ROOT", self.root)
        local_patch = patch.object(configure, "LOCAL", self.local)
        root_patch.start()
        local_patch.start()
        self.addCleanup(root_patch.stop)
        self.addCleanup(local_patch.stop)
        self.output = io.StringIO()
        with contextlib.redirect_stdout(self.output):
            configure.prepare()

    def apply(self, ssid, password):
        (self.local / "wifi.json").write_text(json.dumps({"ssid": ssid, "password": password}))
        with contextlib.redirect_stdout(self.output):
            configure.prepare(apply_wifi=True)

    def test_token_is_private_and_survives_repeated_setup(self):
        token = (self.local / "ota_token.txt").read_text().strip()
        self.assertRegex(token, r"^[0-9a-f]{64}$")
        with contextlib.redirect_stdout(self.output):
            configure.prepare()
        self.assertEqual((self.local / "ota_token.txt").read_text().strip(), token)
        self.assertIn(token, (self.local / "ota_secret.h").read_text())
        self.assertNotIn(token, self.output.getvalue())
        self.assertEqual((self.local / "ota_token.txt").stat().st_mode & 0o777, 0o600)

    def test_credentials_are_escaped_without_changing_other_settings(self):
        (self.root / "sdkconfig").write_text('CONFIG_OTHER=y\nCONFIG_KITCHEN_WIFI_SSID="old"\n')
        ssid = 'room "lights"\\network'
        password = 'private"password\\123'
        self.apply(ssid, password)
        config = (self.root / "sdkconfig").read_text()
        self.assertIn("CONFIG_OTHER=y", config)
        self.assertEqual(config.count("CONFIG_KITCHEN_WIFI_SSID="), 1)
        self.assertIn("CONFIG_KITCHEN_WIFI_SSID=" + json.dumps(ssid), config)
        self.assertIn("CONFIG_KITCHEN_WIFI_PASSWORD=" + json.dumps(password), config)
        self.assertNotIn(password, self.output.getvalue())

    def test_invalid_credentials_do_not_replace_config(self):
        original = "CONFIG_OTHER=y\n"
        (self.root / "sdkconfig").write_text(original)
        for ssid, password in [("", "password"), ("x" * 33, "password"),
                               ("wifi", "short"), ("wifi\nINJECT", "password"),
                               ("wifi", "pass\nword"), ("wifi", "g" * 64)]:
            with self.subTest(ssid=ssid, password_length=len(password)):
                with self.assertRaises(ValueError):
                    self.apply(ssid, password)
                self.assertEqual((self.root / "sdkconfig").read_text(), original)

    def test_open_network_and_hexadecimal_key_are_supported(self):
        self.apply("wifi", "")
        self.assertIn('CONFIG_KITCHEN_WIFI_PASSWORD=""', (self.root / "sdkconfig").read_text())
        self.apply("wifi", "a" * 64)
        self.assertIn('CONFIG_KITCHEN_WIFI_PASSWORD="' + "a" * 64 + '"',
                      (self.root / "sdkconfig").read_text())

    def test_ssid_limit_counts_utf8_bytes(self):
        with self.assertRaises(ValueError):
            self.apply("é" * 17, "password")
        self.apply("é" * 16, "password")


if __name__ == "__main__":
    unittest.main()
