/* Copyright (c) 2010-2011 mbed.org, MIT License
*
* Permission is hereby granted, free of charge, to any person obtaining a copy of this software
* and associated documentation files (the "Software"), to deal in the Software without
* restriction, including without limitation the rights to use, copy, modify, merge, publish,
* distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
* Software is furnished to do so, subject to the following conditions:
*
* The above copyright notice and this permission notice shall be included in all copies or
* substantial portions of the Software.
*
* THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
* BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
* NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
* DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
* OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
*/

#include "USBKeyboard.h"
#include "keyboard.h"

class MyUSBKeyboard: public USBHID {
	union InputReportData {
		HID_REPORT hid_report;
		struct {
			uint32_t length;
			uint8_t report_id;
			uint8_t modifier;
			uint8_t padding;
			uint8_t keycode[6];
		} data;
	};

	InputReportData inputReportData;
	uint8_t lock_status;

	static const uint8_t MODIFIER_LEFT_CONTROL = 1<<0;
	static const uint8_t MODIFIER_LEFT_SHIFT = 1<<1;
	static const uint8_t MODIFIER_LEFT_ALT = 1<<2;
	static const uint8_t MODIFIER_LEFT_GUI = 1<<3;
	static const uint8_t MODIFIER_RIGHT_CONTROL = 1<<4;
	static const uint8_t MODIFIER_RIGHT_SHIFT = 1<<5;
	static const uint8_t MODIFIER_RIGHT_ALT = 1<<6;
	static const uint8_t MODIFIER_RIGHT_GUI = 1<<7;

	static const uint8_t REPORT_ID_KEYBOARD = 1;
	static const uint8_t REPORT_ID_VOLUME = 3;


public:
	MyUSBKeyboard(uint16_t vendor_id = 0x1235, uint16_t product_id = 0x0050, uint16_t product_release = 0x0001):
		USBHID(0, 0, vendor_id, product_id, product_release, false),
		lock_status(0)
	{
		connect();
		memset(&inputReportData, 0, sizeof(inputReportData));
	};

	void appendReportData(const uint8_t keycode) {
		uint8_t modifier = toModifierBit(keycode);
		if (modifier) {
			inputReportData.data.modifier |= modifier;
			return;
		}


		for (int i = 0; i < 6; i++) {
			if (inputReportData.data.keycode[i] == 0) {
				inputReportData.data.keycode[i] = keycode;
				return;
			}
		}

		// report data is full. delete first key and add new key to last
		for (int i = 0; i < 5; i++) {
			inputReportData.data.keycode[i] = inputReportData.data.keycode[i+1];
		}
		inputReportData.data.keycode[5] = keycode;
	}

	void deleteReportData(const uint8_t keycode) {
		uint8_t modifier = toModifierBit(keycode);
		if (modifier) {
			inputReportData.data.modifier &= ~modifier;
			return;
		}

		uint8_t pressedKeyCount = 0;
		for (int i = 0; i < 6; i++) {
			if (inputReportData.data.keycode[i]) pressedKeyCount++;
		}

		for (int i = 0; i < 6; i++) {
			if (inputReportData.data.keycode[i] == keycode) {
				// remove specified key code
				inputReportData.data.keycode[i] = 0;
				pressedKeyCount--;
				// move over another key codes
				for (int j = 0; j < 5-i; j++) {
					inputReportData.data.keycode[i+j] = inputReportData.data.keycode[i+j+1];
				}
				// fill zero to ends
				for (int j = pressedKeyCount; j < 6; j++) {
					inputReportData.data.keycode[j] = 0;
				}
				return;
			}
		}
	}

	bool queueCurrentReportData() {
		inputReportData.data.report_id = REPORT_ID_KEYBOARD;
		inputReportData.data.length = 9;
		DEBUG_PRINTF_KEYEVENT("send %d bytes %02x %02x %02x %02x %02x %02x %02x %02x\r\n",
			inputReportData.hid_report.length,
			inputReportData.hid_report.data[0],
			inputReportData.hid_report.data[1],
			inputReportData.hid_report.data[2],
			inputReportData.hid_report.data[3],
			inputReportData.hid_report.data[4],
			inputReportData.hid_report.data[5],
			inputReportData.hid_report.data[6],
			inputReportData.hid_report.data[7],
			inputReportData.hid_report.data[8]
		);
		return send(&inputReportData.hid_report);
	}

	uint8_t toModifierBit(const uint8_t keycode) const {
		switch (keycode) {
			case KEY_LeftControl:  return MODIFIER_LEFT_CONTROL;
			case KEY_LeftShift:    return MODIFIER_LEFT_SHIFT;
			case KEY_LeftAlt:      return MODIFIER_LEFT_ALT;
			case KEY_LeftGUI:      return MODIFIER_LEFT_GUI;
			case KEY_RightControl: return MODIFIER_RIGHT_CONTROL;
			case KEY_RightShift:   return MODIFIER_RIGHT_SHIFT;
			case KEY_RightAlt:     return MODIFIER_RIGHT_ALT;
			case KEY_RightGUI:     return MODIFIER_RIGHT_GUI;
		}
		return 0;
	}

	bool isKeyPressed() {
		if (inputReportData.data.modifier != 0) {
			return true;
		}

		for (int i = 0; i < 5; i++) {
			if (inputReportData.data.keycode[i]) {
				return 1;
			}
		}
		return 0;
	}

	virtual bool EPINT_OUT_callback() {
		uint32_t bytesRead = 0;
		uint8_t led[MAX_HID_REPORT_SIZE+1];
		USBDevice::readEP(EPINT_OUT, led, &bytesRead, MAX_HID_REPORT_SIZE);

		// we take led[1] because led[0] is the report ID
		lock_status = led[1] & 0x07;

		// We activate the endpoint to be able to recceive data
		if (!readStart(EPINT_OUT, MAX_HID_REPORT_SIZE)) return false;
		return true;
	}


	virtual uint8_t* reportDesc() {
		static uint8_t reportDescriptor[] = {
			USAGE_PAGE(1), 0x01,                    // Generic Desktop
			USAGE(1), 0x06,                         // Keyboard
			COLLECTION(1), 0x01,                    // Application
			REPORT_ID(1),       REPORT_ID_KEYBOARD,

			USAGE_PAGE(1), 0x07,                    // Key Codes
			USAGE_MINIMUM(1), 0xE0,
			USAGE_MAXIMUM(1), 0xE7,
			LOGICAL_MINIMUM(1), 0x00,
			LOGICAL_MAXIMUM(1), 0x01,
			REPORT_SIZE(1), 0x01,
			REPORT_COUNT(1), 0x08,
			INPUT(1), 0x02,                         // Data, Variable, Absolute
			REPORT_COUNT(1), 0x01,
			REPORT_SIZE(1), 0x08,
			INPUT(1), 0x01,                         // Constant


			REPORT_COUNT(1), 0x05,
			REPORT_SIZE(1), 0x01,
			USAGE_PAGE(1), 0x08,                    // LEDs
			USAGE_MINIMUM(1), 0x01,
			USAGE_MAXIMUM(1), 0x05,
			OUTPUT(1), 0x02,                        // Data, Variable, Absolute
			REPORT_COUNT(1), 0x01,
			REPORT_SIZE(1), 0x03,
			OUTPUT(1), 0x01,                        // Constant


			REPORT_COUNT(1), 0x06,
			REPORT_SIZE(1), 0x08,
			LOGICAL_MINIMUM(1), 0x00,
			LOGICAL_MAXIMUM(1), 0x65,
			USAGE_PAGE(1), 0x07,                    // Key Codes
			USAGE_MINIMUM(1), 0x00,
			USAGE_MAXIMUM(1), 0x65,
			INPUT(1), 0x00,                         // Data, Array
			END_COLLECTION(0),

			// Media Control
			USAGE_PAGE(1), 0x0C,
			USAGE(1), 0x01,
			COLLECTION(1), 0x01,
			REPORT_ID(1), REPORT_ID_VOLUME,
			USAGE_PAGE(1), 0x0C,
			LOGICAL_MINIMUM(1), 0x00,
			LOGICAL_MAXIMUM(1), 0x01,
			REPORT_SIZE(1), 0x01,
			REPORT_COUNT(1), 0x07,
			USAGE(1), 0xB5,             // Next Track
			USAGE(1), 0xB6,             // Previous Track
			USAGE(1), 0xB7,             // Stop
			USAGE(1), 0xCD,             // Play / Pause
			USAGE(1), 0xE2,             // Mute
			USAGE(1), 0xE9,             // Volume Up
			USAGE(1), 0xEA,             // Volume Down
			INPUT(1), 0x02,             // Input (Data, Variable, Absolute)
			REPORT_COUNT(1), 0x01,
			INPUT(1), 0x01,
			END_COLLECTION(0),
		};
		reportLength = sizeof(reportDescriptor);
		return reportDescriptor;
	}
protected:
	virtual uint8_t * configurationDesc() {
#define DEFAULT_CONFIGURATION (1)
#define TOTAL_DESCRIPTOR_LENGTH ((1 * CONFIGURATION_DESCRIPTOR_LENGTH) \
                               + (1 * INTERFACE_DESCRIPTOR_LENGTH) \
                               + (1 * HID_DESCRIPTOR_LENGTH) \
                               + (2 * ENDPOINT_DESCRIPTOR_LENGTH))
		static uint8_t configurationDescriptor[] = {
			CONFIGURATION_DESCRIPTOR_LENGTH,    // bLength
			CONFIGURATION_DESCRIPTOR,           // bDescriptorType
			LSB(TOTAL_DESCRIPTOR_LENGTH),       // wTotalLength (LSB)
			MSB(TOTAL_DESCRIPTOR_LENGTH),       // wTotalLength (MSB)
			0x01,                               // bNumInterfaces
			DEFAULT_CONFIGURATION,              // bConfigurationValue
			0x00,                               // iConfiguration
			C_RESERVED | C_SELF_POWERED,        // bmAttributes
			C_POWER(0),                         // bMaxPower

			INTERFACE_DESCRIPTOR_LENGTH,        // bLength
			INTERFACE_DESCRIPTOR,               // bDescriptorType
			0x00,                               // bInterfaceNumber
			0x00,                               // bAlternateSetting
			0x02,                               // bNumEndpoints
			HID_CLASS,                          // bInterfaceClass
			HID_SUBCLASS_BOOT,                  // bInterfaceSubClass
			HID_PROTOCOL_KEYBOARD,              // bInterfaceProtocol
			0x00,                               // iInterface

			HID_DESCRIPTOR_LENGTH,              // bLength
			HID_DESCRIPTOR,                     // bDescriptorType
			LSB(HID_VERSION_1_11),              // bcdHID (LSB)
			MSB(HID_VERSION_1_11),              // bcdHID (MSB)
			0x00,                               // bCountryCode
			0x01,                               // bNumDescriptors
			REPORT_DESCRIPTOR,                  // bDescriptorType
			(uint8_t)(LSB(reportDescLength())), // wDescriptorLength (LSB)
			(uint8_t)(MSB(reportDescLength())), // wDescriptorLength (MSB)

			ENDPOINT_DESCRIPTOR_LENGTH,         // bLength
			ENDPOINT_DESCRIPTOR,                // bDescriptorType
			PHY_TO_DESC(EPINT_IN),              // bEndpointAddress
			E_INTERRUPT,                        // bmAttributes
			LSB(MAX_PACKET_SIZE_EPINT),         // wMaxPacketSize (LSB)
			MSB(MAX_PACKET_SIZE_EPINT),         // wMaxPacketSize (MSB)
			1,                                  // bInterval (milliseconds)

			ENDPOINT_DESCRIPTOR_LENGTH,         // bLength
			ENDPOINT_DESCRIPTOR,                // bDescriptorType
			PHY_TO_DESC(EPINT_OUT),             // bEndpointAddress
			E_INTERRUPT,                        // bmAttributes
			LSB(MAX_PACKET_SIZE_EPINT),         // wMaxPacketSize (LSB)
			MSB(MAX_PACKET_SIZE_EPINT),         // wMaxPacketSize (MSB)
			1,                                  // bInterval (milliseconds)
		};
		return configurationDescriptor;
	}
};
