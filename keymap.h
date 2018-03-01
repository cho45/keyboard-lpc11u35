#include "keyboard.h"
#include "keyboard--short-names.h"

class Keymap;
struct keyfunc_t {
	int8_t row;
	int8_t col;
	void (*func)(Keymap& keymap, bool pressed);
};


static const uint8_t ROWS = 8;
static const uint8_t COLS = 16;
static const uint8_t LAYERS = 2;

class Keymap {
	static const uint8_t KEYMAP_DEFINITION[LAYERS][ROWS][COLS];
	static const keyfunc_t KEYMAP_FUNCTIONS[];

	int8_t layer;
	MyUSBKeyboard& keyboard;
public:

	Keymap(MyUSBKeyboard& _keyboard) : layer(0), keyboard(_keyboard) {
	}

	static void switch_layer(Keymap& keymap, const bool pressed) {
		if (pressed) {
			keymap.layer++;
			if (keymap.layer > LAYERS) {
				keymap.layer = LAYERS - 1;
			}
		} else {
			keymap.layer--;
			if (keymap.layer < 0) {
				keymap.layer = 0;
			}
		}
		DEBUG_PRINTF_KEYEVENT("LAYER->%d\r\n", keymap.layer);
	}

	static void consumer_play_pause(Keymap& keymap, const bool pressed) {
		// TODO
	}

	void execute(const int row, const int col, const bool pressed) {
		for (int i = 0; ; i++) {
			const keyfunc_t& keyfunc = KEYMAP_FUNCTIONS[i];
			if (keyfunc.row == -1) break;
			if (keyfunc.col == col && keyfunc.row == row) {
				DEBUG_PRINTF_KEYEVENT("& %d:%d\r\n", keyfunc.row, keyfunc.row);
				keyfunc.func(*this, pressed);
				return;
			}
		}

		if (pressed) {
			uint8_t key = KEYMAP_DEFINITION[layer][row][col];
			if (key) {
				DEBUG_PRINTF_KEYEVENT("D%d %x\r\n", layer, key);
				keyboard.appendReportData(key);
			}
		} else {
			// ensure delete all keys on layers
			for (int i = 0; i < LAYERS; i++) {
				uint8_t key = KEYMAP_DEFINITION[i][row][col];
				DEBUG_PRINTF_KEYEVENT("U%d %x\r\n", layer, key);
				keyboard.deleteReportData(key);
			}
		}
	}
};

// unimplemented in hardware is __
#define __________ 0
// unimplemented in firmware is _undef
#define _undef 0

const uint8_t Keymap::KEYMAP_DEFINITION[LAYERS][ROWS][COLS] = {
	// layer 0 
	{
		/*   { 0          , 1          , 2          , 3          , 4          , 5          , 6          , 7          , 8          , 9          , 10         , 11         , 12         , 13         , 14         , 15 } */
		/*0*/{ _esc       , _F1        , _F2        , _F3        , _F4        , _F5        , _F6        , __________ , __________ , _F7        , _F8        , _F9        , _F10       , _F11       , _F12       , _undef }     , 
		/*1*/{ _esc       , _1         , _2         , _3         , _4         , _5         , _6         , _7         , _6         , _7         , _8         , _9         , _0         , _dash      , _equal     , _backslash } , 
		/*2*/{ _tab       , _Q         , _W         , _E         , _R         , _T         , _Y         , __________ , _T         , _Y         , _U         , _I         , _O         , _P         , _bracketL  , _grave }     , 
		/*3*/{ _ctrlL     , _A         , _S         , _D         , _F         , _G         , _H         , __________ , _G         , _H         , _J         , _K         , _L         , _semicolon , _quote     , _bracketR }  , 
		/*4*/{ _shiftL    , _Z         , _X         , _C         , _V         , _B         , _N         , __________ , _B         , _N         , _M         , _comma     , _period    , _slash     , _shiftR    , _bs }        , 
		/*5*/{ _altL      , _guiL      , _space     , __________ , __________ , _undef     , __________ , __________ , __________ , _arrowU    , _space     , __________ , _guiR      , _altR      , _undef     , _enter }     , 
		/*6*/{ __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , _arrowL    , _arrowD    , _arrowR    , __________ , __________ , __________ , __________ , __________ } , 
		/*7*/{ __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ } , 
	},
	// layer 1
	{
		/*   { 0          , 1          , 2          , 3          , 4          , 5          , 6          , 7          , 8          , 9          , 10         , 11         , 12         , 13         , 14         , 15 } */
		/*0*/{ _esc       , _F1        , _F2        , _F3        , _F4        , _F5        , _F6        , __________ , __________ , _F7        , _F8        , _F9        , _F10       , _F11       , _F12       , _undef }     , 
		/*1*/{ _esc       , _1         , _2         , _3         , _4         , _5         , _6         , _7         , _6         , _7         , _8         , _9         , _0         , _dash      , _equal     , _backslash } , 
		/*2*/{ _tab       , _Q         , _W         , _E         , _R         , _T         , _Y         , __________ , _T         , _Y         , _U         , _I         , _O         , _P         , _arrowU    , _del }     , 
		/*3*/{ _ctrlL     , _A         , _S         , _D         , _F         , _G         , _H         , __________ , _G         , _H         , _J         , _K         , _L         , _arrowL    , _arrowR    , _bracketR }  , 
		/*4*/{ _shiftL    , _Z         , _X         , _C         , _V         , _B         , _N         , __________ , _B         , _N         , _M         , _comma     , _period    , _arrowD    , _shiftR    , _bs }        , 
		/*5*/{ _altL      , _guiL      , _space     , __________ , __________ , _undef     , __________ , __________ , __________ , _arrowU    , _space     , __________ , _guiR      , _altR      , _undef     , _enter }     , 
		/*6*/{ __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , _arrowL    , _arrowD    , _arrowR    , __________ , __________ , __________ , __________ , __________ } , 
		/*7*/{ __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ , __________ } , 
	},
};

const keyfunc_t Keymap::KEYMAP_FUNCTIONS[] = {
	{ 5, 14, &Keymap::switch_layer },
	{ 5, 5, &Keymap::consumer_play_pause },
	{ -1, -1, 0 } /* for iteration */
};

#undef __________
#undef _undef

