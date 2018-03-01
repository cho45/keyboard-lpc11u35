
/**
 * MCP23017 のアドレスモードについて：
 *
 * Byte Mode: アドレスポインタの自動インクリメントがオフ(デフォルト)
 *     特定のGPIOを連続して監視するときに便利
 *
 *     IOCON.BANK=0: GPIOA と GPIOB が読み出すたびにトグルする (デフォルト)
 *       読み出すと1つアドレスをすすめる(またはアドレスを1つ戻す)
 *       常に 16bit 分の GPIO を読み書きするとき便利
 *
 *     IOCON.BANK=1: GPIOA と GPIOB が完全に別れたアドレスになる
 *
 * Sequential mode: アドレスポインタの自動インクリメントがオン
 *     I2C eeprom みたいな挙動になる
 *
 */
class MCP23017 {
	I2C& i2c;
	uint8_t address;

public:
	// BANK=1
	enum RegisterAddress {
		IODIRA_BANK = 0x00,
		IPOLA_BANK = 0x01,
		GPINTENA_BANK = 0x02,
		DEFVALA_BANK = 0x03,
		INTCONA_BANK = 0x04,
		IOCON_BANK = 0x05,
		GPPUA_BANK = 0x06,
		INTFA_BANK = 0x07,
		INTCAPA_BANK = 0x08,
		GPIOA_BANK = 0x09,
		OLATA_BANK = 0x0a,
		IODIRB_BANK = 0x10,
		IPOLB_BANK = 0x11,
		GPINTENB_BANK = 0x12,
		DEFVALB_BANK = 0x13,
		INTCONB_BANK = 0x14,
		GPPUB_BANK = 0x16,
		INTFB_BANK = 0x17,
		INTCAPB_BANK = 0x18,
		GPIOB_BANK = 0x19,
		OLATB_BANK = 0x1a,

		// BANK=0
		IODIRA = 0x00,
		IODIRB = 0x01,
		IPOLA = 0x02,
		IPOLB = 0x03,
		GPINTENA = 0x04,
		GPINTENB = 0x05,
		DEFVALA = 0x06,
		DEFVALB = 0x07,
		INTCONA = 0x08,
		INTCONB = 0x09,
		IOCON = 0x0a,
		GPPUA = 0x0c,
		GPPUB = 0x0d,
		INTFA = 0x0e,
		INTFB = 0x0f,
		INTCAPA = 0x10,
		INTCAPB = 0x11,
		GPIOA = 0x12,
		GPIOB = 0x13,
		OLATA = 0x14,
		OLATB = 0x15,
	};

	static const uint8_t BANK = 7;
	static const uint8_t MIRROR = 6;
	static const uint8_t SEQOP = 5;
	static const uint8_t DISSLW = 4;
	static const uint8_t HAEN = 3;
	static const uint8_t ODR = 2;
	static const uint8_t INTPOL = 1;
	
	// i2c.write(byte) returns 1 with success
	// i2c.write(int, const char*, int, bool) returns 0 with success.
	static const int I2C_WRITE_MULTIBYTES_SUCCESS = 0;

	/**
	 * _i2c is instance of I2C
	 * _address is 7-bit slave address of MCP23017
	 */

	MCP23017(
		I2C& _i2c,
		uint8_t _address
	) :
		i2c(_i2c),
		address(_address<<1)
	{
	}

	uint8_t read8(const RegisterAddress reg, int& error) const {
		char data[1];
		data[0] = reg;
		i2c.write(address, data, 1, true);
		error = i2c.read(address, data, 1, false);
		return data[0];
	}

	uint16_t read16(const RegisterAddress reg, int& error) const {
		char data[2];
		data[0] = reg;
		i2c.write(address, data, 1, true);
		error = i2c.read(address, data, 2, false);
		return (static_cast<uint16_t>(data[0]) << 8) | static_cast<uint16_t>(data[1]);
	}

	int write8(const RegisterAddress reg, uint8_t data) const {
		char d[2];
		d[0] = reg;
		d[1] = data;
		return i2c.write(address, d, 2, false) == I2C_WRITE_MULTIBYTES_SUCCESS;
	}

	int write16(const RegisterAddress reg, uint16_t data) const {
		char d[3];
		d[0] = reg;
		d[1] = data >> 8;
		d[2] = data & 0xff;
		return i2c.write(address, d, 3, false) == I2C_WRITE_MULTIBYTES_SUCCESS;
	}
};


