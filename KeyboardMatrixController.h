#include "config.h"
#include "mcp23017.h"

class KeyboardMatrixController {
	I2C& i2c;
	MCP23017 gpio1;
	MCP23017 gpio2;
	bool gpio1_ready;
	bool gpio2_ready;

	static const uint8_t GPIO1_SLAVE_ADDRESS = 0b0100000;
	static const uint8_t GPIO2_SLAVE_ADDRESS = 0b0100100;

	/**
	 * COL=GPIOA (output normaly positive)
	 * ROW=GPIOB (input pulled-up)
	 */

	bool setupGpio(MCP23017& gpio) {
		int ok;
		DEBUG_PRINTF("SET IOCON\r\n");
		ok = gpio.write8(
			MCP23017::IOCON,
			0<<MCP23017::BANK |
			1<<MCP23017::MIRROR |
			1<<MCP23017::SEQOP |
			0<<MCP23017::DISSLW |
			1<<MCP23017::ODR // int pin is open drain
		);
		if (!ok) return false;

		// IODIR
		//   1: input
		//   0: output
		DEBUG_PRINTF("SET IODIRA\r\n");
		ok = gpio.write16(
			MCP23017::IODIRA,
			0b0000000011111111
		);
		if (!ok) return false;

		// INPUT POLARITY
		//   1: inverse polarity
		//   0: raw
		DEBUG_PRINTF("SET IPOLB\r\n");
		ok = gpio.write8(
			MCP23017::IPOLB,
			0b11111111
		);
		if (!ok) return false;

		// INTERRUPT-ON-CHANGE Enable
		DEBUG_PRINTF("SET GPINTENB\r\n");
		ok = gpio.write8(
			MCP23017::GPINTENB,
			0b11111111
		);
		if (!ok) return false;

		// INTERRUPT-ON-CHANGE Control
		//   1: compared with DEFVAL
		//   0: compared to previous value
		DEBUG_PRINTF("SET INTCONB\r\n");
		ok = gpio.write8(
			MCP23017::INTCONB,
			0b00000000
		);
		if (!ok) return false;

		// PULL-UP (for input pin)
		//   1: pull-up enabled
		//   0: pull-up disabled
		DEBUG_PRINTF("SET GPPUB\r\n");
		ok = gpio.write8(
			MCP23017::GPPUB,
			0b11111111
		);
		if (!ok) return false;

		DEBUG_PRINTF("SET GPIOA\r\n");
		ok = gpio1.write8(
			MCP23017::GPIOA,
			0b00000000
		);
		if (!ok) return false;

		return true;
	}

public:
	KeyboardMatrixController(I2C& _i2c) :
		i2c(_i2c),
		gpio1(i2c, GPIO1_SLAVE_ADDRESS),
		gpio2(i2c, GPIO2_SLAVE_ADDRESS)
	{
	}

	void init() {
		DEBUG_PRINTF("init gpio1\r\n");
		gpio1_ready = setupGpio(gpio1);
		DEBUG_PRINTF("gpio1 initialized: %s\r\n", gpio1_ready ? "success" : "failed");

		DEBUG_PRINTF("init gpio2\r\n");
		gpio2_ready = setupGpio(gpio2);
		DEBUG_PRINTF("gpio2 initialized: %s\r\n", gpio2_ready ? "success" : "failed");

	}

	// __attribute__((used, long_call, section(".data")))
	void scanKeyboard(uint8_t* keys) {
		int ok;

		disableInterrupt();

		if (gpio1_ready) {
			for (int i = 0; i < 8; i++) {
				ok = gpio1.write8(
					MCP23017::GPIOA,
					~(1<<i)
				);
				wait_us(1);
				keys[i] = gpio1.read8(MCP23017::GPIOB, ok);
			}

			// set all output to negative for interrupt
			ok = gpio1.write8(
				MCP23017::GPIOA,
				0b00000000
			);
		}


		if (gpio2_ready) {
			for (int i = 0; i < 8; i++) {
				ok = gpio2.write8(
					MCP23017::GPIOA,
					~(1<<i)
				);
				wait_us(1);
				keys[i+8] = gpio2.read8(MCP23017::GPIOB, ok);
			}

			// set all output to negative for interrupt
			ok = gpio2.write8(
				MCP23017::GPIOA,
				0b00000000
			);
		}

		enableInterrupt();
	}

	int disableInterrupt() {
		int ok;
		if (gpio1_ready) {
			// Disable interrupt
			ok = gpio1.write8(
				MCP23017::GPINTENB,
				0b00000000
			);
		}

		if (gpio2_ready) {
			// Disable interrupt
			ok = gpio2.write8(
				MCP23017::GPINTENB,
				0b00000000
			);
		}
		return ok;
	}

	int enableInterrupt() {
		int ok;
		if (gpio1_ready) {
			// Enable interrupt
			ok = gpio1.write8(
				MCP23017::GPINTENB,
				0b11111111
			);
		}
		
		/*
		// Clear interrupt
		gpio1.read8(MCP23017::GPIOB, ok);
		gpio2.read8(MCP23017::GPIOB, ok);
		*/
		if (gpio2_ready) {
			// Enable interrupt
			ok = gpio2.write8(
				MCP23017::GPINTENB,
				0b11111111
			);
		}

		return ok;
	}
};
