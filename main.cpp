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
static uint8_t keysA[COLS];
static uint8_t keysB[COLS];
static bool state = 0;
#define is_pressed(keys, row, col) (!!(keys[col] & (1<<row)))

DigitalOut led(LED1);

void usb_deepsleep() {
	//1. Set bit AP_CLK in the USBCLKCTRL register (Table 41) to 0 (default) to enable automatic control of the USB need_clock signal.
	DEBUG_PRINTF("1\r\n");
	LPC_SYSCON->USBCLKCTRL &= ~(1<<0);
	//2. Wait until USB activity is suspended by polling the DSUS bit in the DSVCMD_STAT register (DSUS = 1).
	DEBUG_PRINTF("2\r\n");
	while ( (LPC_USB->DEVCMDSTAT & (1<<17)) == 0) { }
	//3. The USB need_clock signal will be deasserted after another 2 ms. Poll the USBCLKST register until the USB need_clock status bit is 0 (Table 42).
	DEBUG_PRINTF("3\r\n");
	while (LPC_SYSCON->USBCLKST != 0) {}
	//4. Once the USBCLKST register returns 0, enable the USB activity wake-up interrupt in the NVIC (# 30) and clear it.
	DEBUG_PRINTF("4\r\n");
	NVIC_EnableIRQ(USBWakeup_IRQn);
	NVIC_ClearPendingIRQ(USBWakeup_IRQn);
	//5. Set bit 1 in the USBCLKCTRL register to 1 to trigger the USB activity wake-up interrupt on the rising edge of the USB need_clock signal.
	DEBUG_PRINTF("5\r\n");
	LPC_SYSCON->USBCLKCTRL |= 1<<1;
	//6. Enable the wake-up from Deep-sleep or Power-down modes on this interrupt by enabling the USB need_clock signal in the STARTERP1 register (Table 44, bit 19).
	DEBUG_PRINTF("6\r\n");
	LPC_SYSCON->STARTERP1 |= 1<<19;
	//7. Enter Deep-sleep or Power-down modes by writing to the PCON register.
	DEBUG_PRINTF("7\r\n");
	LPC_PMU->PCON = 0x1;
	//8. Execute a WFI instruction.
	DEBUG_PRINTF("8\r\n");
	__WFI();
}

void usbwakeup() {}

int main() {
	i2c.frequency(250000);

	keyboardInterruptIn.mode(PullUp);
	keyboardInterruptIn.fall(keyboardInterrupt);

	keyboardMatrixController.init();
	pollCount = 10;

	while (1) {
		while (pollCount-- > 0) {
			uint8_t (&keysCurr)[COLS] = state ? keysA : keysB;
			uint8_t (&keysPrev)[COLS] = state ? keysB : keysA;

			keyboardMatrixController.scanKeyboard(keysCurr);

			bool queue = false;

			for (int col = 0; col < COLS; col++) {
				const uint8_t changed = keysPrev[col] ^ keysCurr[col];
				if (changed) queue = true;
				for (int row = 0; row < ROWS; row++) {
					if (changed & (1<<row)) {
						bool pressed = keysCurr[col] & (1<<row);
						DEBUG_PRINTF_KEYEVENT("changed: col=%d, row=%d / pressed=%d\r\n", col, row, pressed);
						keymap.execute(row, col, pressed);
					}
				}
			}
			state = !state;

			if (queue) {
				bool ok = keyboard.queueCurrentReportData();
				if (!ok) {
					DEBUG_PRINTF_KEYEVENT("send() failed");
				}
			}
		}

		// sleep();
	}
}

