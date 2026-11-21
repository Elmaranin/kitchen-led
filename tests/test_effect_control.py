from pathlib import Path
import os
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
SDK = Path(os.environ.get("IDF_PATH", "/tmp/kitchen-led-esp8266-sdk"))
CJSON = SDK / "components/json/cJSON"


@unittest.skipUnless(shutil.which("cc") and (CJSON / "cJSON.c").exists(),
                     "A host C compiler and the SDK cJSON sources are required")
class EffectControlTests(unittest.TestCase):
    def test_commands_and_effect_parameters(self):
        with tempfile.TemporaryDirectory() as directory:
            temporary = Path(directory)
            headers = temporary / "freertos"
            headers.mkdir()
            (headers / "FreeRTOS.h").write_text(
                "#define portENTER_CRITICAL() ((void)0)\n"
                "#define portEXIT_CRITICAL() ((void)0)\n")
            binary = temporary / "effect-control-test"
            compiled = subprocess.run(
                ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", str(temporary),
                 "-I", str(ROOT / "main"), "-I", str(CJSON),
                 str(ROOT / "main/effect_control.c"), str(ROOT / "main/led_effects.c"),
                 str(CJSON / "cJSON.c"),
                 str(ROOT / "tests/effect_control_test.c"), "-lm", "-o", str(binary)],
                capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            tested = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(tested.returncode, 0, tested.stdout + tested.stderr)


if __name__ == "__main__":
    unittest.main()
