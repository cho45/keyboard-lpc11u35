#include "mbed.h"
#include "config.h"

#include "MyUSBKeyboard.h"
#include "KeyboardMatrixController.h"
#include "keymap.h"

static MyUSBKeyboard keyboard;
static I2C i2c(P0_5, P0_4);
static KeyboardMatrixController keyboardMatrixController(i2c);
static Keymap keymap(keyboard);

// Interrupt from MCP23017
// (pulled-up and two MCP23017 is configured with open drain INT)
static InterruptIn keyboardInterruptIn(P0_2);

// delay for interrupt
static volatile int8_t pollCount = 50;

static void keyboardInterrupt() {
	// just for wakeup
	pollCount = 25;
}

// ROWS=8
// COLS=16
// 列ごとに1バイトにパックしてキーの状態を保持する
static uint8_t keys[3][COLS];
static uint8_t state = 0;
#define is_pressed(keys, row, col) (!!(keys[col] & (1<<row)))

// 120Hz = 8.3ms
// USB polling interval min is 8ms on Windows
// (ref. https://docs.microsoft.com/en-us/windows-hardware/drivers/ddi/content/usbspec/ns-usbspec-_usb_endpoint_descriptor)
static const uint8_t HID_QUEUE_DURATION_MS = 8;
static const uint8_t BOUNCE_TIME = 4;

DigitalOut led(LED1);

int main() {
	// 100k
	// i2c.frequency(100000);
	// 400k (max @3.3V for MCP23017)
	i2c.frequency(400000);

	keyboardInterruptIn.mode(PullUp);
	keyboardInterruptIn.fall(keyboardInterrupt);

	keyboardMatrixController.init();

	while (1) {
		for (; pollCount > 0; pollCount--) {

			uint8_t (&keysCurr)[COLS] = keys[(state - 0 + 3) % 3];
			uint8_t (&keysPrev)[COLS] = keys[(state - 1 + 3) % 3];
			uint8_t (&keysLast)[COLS] = keys[(state - 2 + 3) % 3];

			keyboardMatrixController.scanKeyboard(keysCurr);

			bool queue = false;

			for (int col = 0; col < COLS; col++) {
				const uint8_t filtered = (~(keysPrev[col] ^ keysCurr[col]) & keysCurr[col]);
				const uint8_t changed = keysLast[col] ^ filtered;
				keysLast[col] = filtered;
				if (changed) queue = true;
				for (int row = 0; row < ROWS; row++) {
					if (changed & (1<<row)) {
						bool pressed = keysCurr[col] & (1<<row);
						DEBUG_PRINTF_KEYEVENT("changed: col=%d, row=%d / pressed=%d\r\n", col, row, pressed);
						keymap.execute(row, col, pressed);
					}
				}
			}
			state = (state + 1) % 3;

			if (queue) {
				// ensure unpress event
				pollCount++;
				bool ok = keyboard.queueCurrentReportData();
				if (!ok) {
					DEBUG_PRINTF_KEYEVENT("send() failed");
				}
			}
			wait_ms(BOUNCE_TIME);
		}

		// sleep causes USB stall
		// sleep();
	}
}

