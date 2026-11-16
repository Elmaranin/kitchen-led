import contextlib
import importlib.util
import io
import json
from pathlib import Path
import tempfile
import subprocess
import unittest
import urllib.error
from unittest.mock import patch


SCRIPT = Path(__file__).resolve().parents[1] / "tools/ota.py"
SPEC = importlib.util.spec_from_file_location("ota", SCRIPT)
ota = importlib.util.module_from_spec(SPEC)
SPEC.loader.exec_module(ota)


def response(value):
    payload = json.dumps(value).encode() if isinstance(value, dict) else value
    return io.BytesIO(payload)


def status(version="0.2.0", slot="ota_0", pending=False):
    return response({"version": version, "slot": slot, "reboot_pending": pending})


class OtaClientTests(unittest.TestCase):
    def setUp(self):
        self.directory = tempfile.TemporaryDirectory()
        self.addCleanup(self.directory.cleanup)
        root = Path(self.directory.name)
        self.image = root / "firmware.bin"
        self.image.write_bytes(b"\xe9" + b"\0" * 255)
        self.token = root / "token.txt"
        self.token.write_text("private-test-token\n")
        self.output = io.StringIO()

    def run_update(self):
        with contextlib.redirect_stdout(self.output), patch.object(ota.time, "sleep"):
            ota.update("http://controller", self.image, self.token)

    def test_upload_uses_token_and_confirms_slot_change(self):
        responses = [status(), response(b"Accepted"), status(pending=True),
                     status("0.2.1", "ota_1")]
        with patch.object(ota.urllib.request, "urlopen", side_effect=responses) as opener:
            self.run_update()
        request = opener.call_args_list[1].args[0]
        self.assertEqual(request.full_url, "http://controller/update")
        self.assertEqual(request.get_header("Authorization"), "Bearer private-test-token")
        self.assertEqual(request.data, self.image.read_bytes())
        self.assertIn("OTA verified: 0.2.1 is running in ota_1.", self.output.getvalue())
        self.assertNotIn("private-test-token", self.output.getvalue())

    def test_reboot_connection_loss_is_retried(self):
        responses = [status(), response(b"Accepted"), urllib.error.URLError("Restarting"),
                     status("0.2.1", "ota_1")]
        with patch.object(ota.urllib.request, "urlopen", side_effect=responses):
            self.run_update()
        self.assertIn("OTA verified", self.output.getvalue())

    def test_rejected_upload_is_reported(self):
        error = urllib.error.HTTPError("http://controller/update", 401, "Unauthorized", {},
                                       io.BytesIO(b"Invalid access token"))
        with patch.object(ota.urllib.request, "urlopen", side_effect=[status(), error]):
            with self.assertRaisesRegex(RuntimeError, "Upload rejected.*401.*Invalid access token"):
                self.run_update()

    def test_old_slot_is_not_reported_as_a_success(self):
        with patch.object(ota.urllib.request, "urlopen",
                          side_effect=[status(), response(b"Accepted"), status("0.2.1")]), \
             patch.object(ota.time, "monotonic", side_effect=[0, 0, 100]):
            with self.assertRaisesRegex(RuntimeError, "could not be confirmed"):
                self.run_update()

    def test_invalid_image_is_not_uploaded(self):
        self.image.write_bytes(b"not a firmware image")
        with patch.object(ota.urllib.request, "urlopen", return_value=status()) as opener:
            with self.assertRaises(ValueError):
                self.run_update()
        self.assertEqual(opener.call_count, 1)

    def test_build_finishes_before_upload(self):
        sdk = self.image.parent / "sdk"
        (sdk / "tools").mkdir(parents=True)
        (sdk / "tools/idf.py").touch()
        steps = []
        with patch.dict(ota.os.environ, {"IDF_PATH": str(sdk)}), \
             patch.object(ota.subprocess, "run", side_effect=lambda *args, **kwargs: steps.append("build")) as build, \
             patch.object(ota, "update", side_effect=lambda *args: steps.append("upload")) as upload, \
             contextlib.redirect_stdout(self.output):
            ota.build_and_update("http://controller", self.image, self.token)
        self.assertEqual(steps, ["build", "upload"])
        self.assertEqual(build.call_args.args[0], [ota.sys.executable, str(sdk / "tools/idf.py"),
                                                  "-B", str(self.image.resolve().parent), "build"])
        upload.assert_called_once_with("http://controller", self.image, self.token)
        self.assertIn("Step 1/2", self.output.getvalue())
        self.assertIn("Step 2/2", self.output.getvalue())

    def test_failed_build_prevents_upload(self):
        sdk = self.image.parent / "sdk"
        (sdk / "tools").mkdir(parents=True)
        (sdk / "tools/idf.py").touch()
        with patch.dict(ota.os.environ, {"IDF_PATH": str(sdk)}), \
             patch.object(ota.subprocess, "run", side_effect=subprocess.CalledProcessError(1, "build")), \
             patch.object(ota, "update") as upload, contextlib.redirect_stdout(self.output):
            with self.assertRaises(subprocess.CalledProcessError):
                ota.build_and_update("http://controller", self.image, self.token)
        upload.assert_not_called()

    def test_missing_sdk_prevents_upload(self):
        with patch.dict(ota.os.environ, {"IDF_PATH": ""}), patch.object(ota, "update") as upload:
            with self.assertRaisesRegex(ValueError, "IDF_PATH"):
                ota.build_and_update("http://controller", self.image, self.token)
        upload.assert_not_called()


if __name__ == "__main__":
    unittest.main()
