from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest


ROOT = Path(__file__).resolve().parents[1]
HEADERS = {
    "esp_err.h": """#pragma once
typedef int esp_err_t;
#define ESP_OK 0
#define ESP_FAIL -1
#define ESP_ERR_INVALID_STATE 1
#define ESP_ERR_INVALID_ARG 2
""",
    "sdkconfig.h": """#define CONFIG_ESP8266_DEFAULT_CPU_FREQ_MHZ 80
#define CONFIG_KITCHEN_LED_COLOR_ORDER_RGB 1
#define CONFIG_KITCHEN_LINE_2_SWAP_RG 1
#define CONFIG_KITCHEN_LINE_1_GPIO 4
#define CONFIG_KITCHEN_LINE_2_GPIO 5
#define CONFIG_KITCHEN_LINE_3_GPIO 12
#define CONFIG_KITCHEN_LINE_4_GPIO 13
#define CONFIG_KITCHEN_LINE_1_LEDS 15
#define CONFIG_KITCHEN_LINE_2_LEDS 50
#define CONFIG_KITCHEN_LINE_3_LEDS 15
#define CONFIG_KITCHEN_LINE_4_LEDS 15
""",
    "driver/gpio.h": """#pragma once
#include <stdint.h>
#include "esp_err.h"
typedef struct {
    uint64_t pin_bit_mask;
    int mode, pull_up_en, pull_down_en, intr_type;
} gpio_config_t;
#define GPIO_MODE_OUTPUT 1
#define GPIO_PULLUP_DISABLE 0
#define GPIO_PULLDOWN_DISABLE 0
#define GPIO_PULLDOWN_ENABLE 1
#define GPIO_INTR_DISABLE 0
esp_err_t gpio_set_level(unsigned pin, uint32_t level);
esp_err_t gpio_config(const gpio_config_t *config);
""",
    "rom/ets_sys.h": "#include <stdint.h>\nvoid ets_delay_us(uint32_t delay);\n"
                     "void vPortETSIntrLock(void);\nvoid vPortETSIntrUnlock(void);\n",
    "freertos/FreeRTOS.h": "#define portENTER_CRITICAL() ((void)0)\n#define portEXIT_CRITICAL() ((void)0)\n",
    "freertos/task.h": "",
}


@unittest.skipUnless(shutil.which("cc"), "A host C compiler is required")
class RelayDriverTests(unittest.TestCase):
    def run_state_test(self, conflict=False, long_strip=False, grb=False):
        with tempfile.TemporaryDirectory() as directory:
            temporary = Path(directory)
            for name, content in HEADERS.items():
                if grb and name == "sdkconfig.h":
                    content = content.replace("#define CONFIG_KITCHEN_LED_COLOR_ORDER_RGB 1",
                                              "#define CONFIG_KITCHEN_LED_COLOR_ORDER_GRB 1")
                if conflict and name == "sdkconfig.h":
                    content = content.replace("CONFIG_KITCHEN_LINE_1_GPIO 4", "CONFIG_KITCHEN_LINE_1_GPIO 14")
                if long_strip and name == "sdkconfig.h":
                    content = content.replace("CONFIG_KITCHEN_LINE_1_LEDS 15", "CONFIG_KITCHEN_LINE_1_LEDS 144")
                path = temporary / name
                path.parent.mkdir(parents=True, exist_ok=True)
                path.write_text(content)
            binary = temporary / "relay-state-test"
            compiled = subprocess.run(
                ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", *(["-DTEST_PIN_CONFLICT"] if conflict else []),
                 *(["-DTEST_LONG_STRIP"] if long_strip else []), "-I", str(temporary),
                 "-I", str(ROOT / "main"), str(ROOT / "main/kitchen_led.c"),
                 str(ROOT / "tests/relay_state_test.c"), "-o", str(binary)],
                capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            tested = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(tested.returncode, 0, tested.stdout + tested.stderr)

    def test_default_state_and_gpio_failures(self):
        self.run_state_test()

    def test_led_line_cannot_use_relay_pin(self):
        self.run_state_test(conflict=True)

    def test_144_pixel_frame_and_last_pixel_bounds(self):
        self.run_state_test(long_strip=True)

    def test_grb_byte_order(self):
        self.run_state_test(long_strip=True, grb=True)

    def test_relay_command_validation(self):
        with tempfile.TemporaryDirectory() as directory:
            binary = Path(directory) / "relay-command-test"
            compiled = subprocess.run(
                ["cc", "-std=c11", "-Wall", "-Wextra", "-Werror", "-I", str(ROOT / "main"),
                 str(ROOT / "main/relay_command.c"), str(ROOT / "tests/relay_command_test.c"),
                 "-o", str(binary)], capture_output=True, text=True)
            self.assertEqual(compiled.returncode, 0, compiled.stdout + compiled.stderr)
            tested = subprocess.run([str(binary)], capture_output=True, text=True)
            self.assertEqual(tested.returncode, 0, tested.stdout + tested.stderr)


if __name__ == "__main__":
    unittest.main()
