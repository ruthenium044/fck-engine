#ifndef FCK_EVENTS_H_INCLUDED
#define FCK_EVENTS_H_INCLUDED

#include <fckc_inttypes.h>

typedef enum fck_event_input_type
{
	FCK_EVENT_INPUT_TYPE_NONE,
	FCK_EVENT_INPUT_TYPE_DEVICE,
	FCK_EVENT_INPUT_TYPE_TEXT,
} fck_event_input_type;

typedef enum fck_input_device_type
{
	FCK_INPUT_DEVICE_TYPE_NONE,
	FCK_INPUT_DEVICE_TYPE_KEYBOARD,
	FCK_INPUT_DEVICE_TYPE_MOUSE
} fck_input_device_type;

typedef enum fck_mouse_event_type
{
	FCK_MOUSE_EVENT_TYPE_BUTTON_NONE,
	FCK_MOUSE_EVENT_TYPE_BUTTON_LEFT,
	FCK_MOUSE_EVENT_TYPE_BUTTON_RIGHT,
	FCK_MOUSE_EVENT_TYPE_BUTTON_MIDDLE,
	FCK_MOUSE_EVENT_TYPE_BUTTON_4,
	FCK_MOUSE_EVENT_TYPE_BUTTON_5,
	FCK_MOUSE_EVENT_TYPE_WHEEL,
	FCK_MOUSE_EVENT_TYPE_POSITION,
} fck_mouse_event_type;

typedef enum fck_keyboard_event_type
{
	FCK_KEYBOARD_EVENT_TYPE_NONE,
	FCK_KEYBOARD_EVENT_TYPE_UP,
	FCK_KEYBOARD_EVENT_TYPE_DOWN,
} fck_keyboard_event_type;

// Taken from SDL_Scancode
typedef enum fck_pkey
{
	FCK_PKEY_UNKNOWN = 0,

	FCK_PKEY_A = 4,
	FCK_PKEY_B = 5,
	FCK_PKEY_C = 6,
	FCK_PKEY_D = 7,
	FCK_PKEY_E = 8,
	FCK_PKEY_F = 9,
	FCK_PKEY_G = 10,
	FCK_PKEY_H = 11,
	FCK_PKEY_I = 12,
	FCK_PKEY_J = 13,
	FCK_PKEY_K = 14,
	FCK_PKEY_L = 15,
	FCK_PKEY_M = 16,
	FCK_PKEY_N = 17,
	FCK_PKEY_O = 18,
	FCK_PKEY_P = 19,
	FCK_PKEY_Q = 20,
	FCK_PKEY_R = 21,
	FCK_PKEY_S = 22,
	FCK_PKEY_T = 23,
	FCK_PKEY_U = 24,
	FCK_PKEY_V = 25,
	FCK_PKEY_W = 26,
	FCK_PKEY_X = 27,
	FCK_PKEY_Y = 28,
	FCK_PKEY_Z = 29,

	FCK_PKEY_1 = 30,
	FCK_PKEY_2 = 31,
	FCK_PKEY_3 = 32,
	FCK_PKEY_4 = 33,
	FCK_PKEY_5 = 34,
	FCK_PKEY_6 = 35,
	FCK_PKEY_7 = 36,
	FCK_PKEY_8 = 37,
	FCK_PKEY_9 = 38,
	FCK_PKEY_0 = 39,

	FCK_PKEY_RETURN = 40,
	FCK_PKEY_ESCAPE = 41,
	FCK_PKEY_BACKSPACE = 42,
	FCK_PKEY_TAB = 43,
	FCK_PKEY_SPACE = 44,

	FCK_PKEY_MINUS = 45,
	FCK_PKEY_EQUALS = 46,
	FCK_PKEY_LEFTBRACKET = 47,
	FCK_PKEY_RIGHTBRACKET = 48,
	FCK_PKEY_BACKSLASH = 49,
	FCK_PKEY_NONUSHASH = 50,
	FCK_PKEY_SEMICOLON = 51,
	FCK_PKEY_APOSTROPHE = 52,
	FCK_PKEY_GRAVE = 53,
	FCK_PKEY_COMMA = 54,
	FCK_PKEY_PERIOD = 55,
	FCK_PKEY_SLASH = 56,

	FCK_PKEY_CAPSLOCK = 57,

	FCK_PKEY_F1 = 58,
	FCK_PKEY_F2 = 59,
	FCK_PKEY_F3 = 60,
	FCK_PKEY_F4 = 61,
	FCK_PKEY_F5 = 62,
	FCK_PKEY_F6 = 63,
	FCK_PKEY_F7 = 64,
	FCK_PKEY_F8 = 65,
	FCK_PKEY_F9 = 66,
	FCK_PKEY_F10 = 67,
	FCK_PKEY_F11 = 68,
	FCK_PKEY_F12 = 69,

	FCK_PKEY_PRINTSCREEN = 70,
	FCK_PKEY_SCROLLLOCK = 71,
	FCK_PKEY_PAUSE = 72,
	FCK_PKEY_INSERT = 73,

	FCK_PKEY_HOME = 74,
	FCK_PKEY_PAGEUP = 75,
	FCK_PKEY_DELETE = 76,
	FCK_PKEY_END = 77,
	FCK_PKEY_PAGEDOWN = 78,
	FCK_PKEY_RIGHT = 79,
	FCK_PKEY_LEFT = 80,
	FCK_PKEY_DOWN = 81,
	FCK_PKEY_UP = 82,

	FCK_PKEY_NUMLOCKCLEAR = 83,

	FCK_PKEY_KP_DIVIDE = 84,
	FCK_PKEY_KP_MULTIPLY = 85,
	FCK_PKEY_KP_MINUS = 86,
	FCK_PKEY_KP_PLUS = 87,
	FCK_PKEY_KP_ENTER = 88,
	FCK_PKEY_KP_1 = 89,
	FCK_PKEY_KP_2 = 90,
	FCK_PKEY_KP_3 = 91,
	FCK_PKEY_KP_4 = 92,
	FCK_PKEY_KP_5 = 93,
	FCK_PKEY_KP_6 = 94,
	FCK_PKEY_KP_7 = 95,
	FCK_PKEY_KP_8 = 96,
	FCK_PKEY_KP_9 = 97,
	FCK_PKEY_KP_0 = 98,
	FCK_PKEY_KP_PERIOD = 99,

	FCK_PKEY_NONUSBACKSLASH = 100,

	FCK_PKEY_APPLICATION = 101,
	FCK_PKEY_POWER = 102,

	FCK_PKEY_KP_EQUALS = 103,
	FCK_PKEY_F13 = 104,
	FCK_PKEY_F14 = 105,
	FCK_PKEY_F15 = 106,
	FCK_PKEY_F16 = 107,
	FCK_PKEY_F17 = 108,
	FCK_PKEY_F18 = 109,
	FCK_PKEY_F19 = 110,
	FCK_PKEY_F20 = 111,
	FCK_PKEY_F21 = 112,
	FCK_PKEY_F22 = 113,
	FCK_PKEY_F23 = 114,
	FCK_PKEY_F24 = 115,
	FCK_PKEY_EXECUTE = 116,
	FCK_PKEY_HELP = 117, /**< AL Integrated Help Center */
	FCK_PKEY_MENU = 118, /**< Menu (show menu) */
	FCK_PKEY_SELECT = 119,
	FCK_PKEY_STOP = 120,  /**< AC Stop */
	FCK_PKEY_AGAIN = 121, /**< AC Redo/Repeat */
	FCK_PKEY_UNDO = 122,  /**< AC Undo */
	FCK_PKEY_CUT = 123,   /**< AC Cut */
	FCK_PKEY_COPY = 124,  /**< AC Copy */
	FCK_PKEY_PASTE = 125, /**< AC Paste */
	FCK_PKEY_FIND = 126,  /**< AC Find */
	FCK_PKEY_MUTE = 127,
	FCK_PKEY_VOLUMEUP = 128,
	FCK_PKEY_VOLUMEDOWN = 129,
	/* not sure whether there's a reason to enable these */
	/*     FCK_PKEY_LOCKINGCAPSLOCK = 130,  */
	/*     FCK_PKEY_LOCKINGNUMLOCK = 131, */
	/*     FCK_PKEY_LOCKINGSCROLLLOCK = 132, */
	FCK_PKEY_KP_COMMA = 133,
	FCK_PKEY_KP_EQUALSAS400 = 134,

	FCK_PKEY_INTERNATIONAL1 = 135, /**< used on Asian keyboards, see
	                                        footnotes in USB doc */
	FCK_PKEY_INTERNATIONAL2 = 136,
	FCK_PKEY_INTERNATIONAL3 = 137, /**< Yen */
	FCK_PKEY_INTERNATIONAL4 = 138,
	FCK_PKEY_INTERNATIONAL5 = 139,
	FCK_PKEY_INTERNATIONAL6 = 140,
	FCK_PKEY_INTERNATIONAL7 = 141,
	FCK_PKEY_INTERNATIONAL8 = 142,
	FCK_PKEY_INTERNATIONAL9 = 143,
	FCK_PKEY_LANG1 = 144, /**< Hangul/English toggle */
	FCK_PKEY_LANG2 = 145, /**< Hanja conversion */
	FCK_PKEY_LANG3 = 146, /**< Katakana */
	FCK_PKEY_LANG4 = 147, /**< Hiragana */
	FCK_PKEY_LANG5 = 148, /**< Zenkaku/Hankaku */
	FCK_PKEY_LANG6 = 149, /**< reserved */
	FCK_PKEY_LANG7 = 150, /**< reserved */
	FCK_PKEY_LANG8 = 151, /**< reserved */
	FCK_PKEY_LANG9 = 152, /**< reserved */

	FCK_PKEY_ALTERASE = 153, /**< Erase-Eaze */
	FCK_PKEY_SYSREQ = 154,
	FCK_PKEY_CANCEL = 155, /**< AC Cancel */
	FCK_PKEY_CLEAR = 156,
	FCK_PKEY_PRIOR = 157,
	FCK_PKEY_RETURN2 = 158,
	FCK_PKEY_SEPARATOR = 159,
	FCK_PKEY_OUT = 160,
	FCK_PKEY_OPER = 161,
	FCK_PKEY_CLEARAGAIN = 162,
	FCK_PKEY_CRSEL = 163,
	FCK_PKEY_EXSEL = 164,

	FCK_PKEY_KP_00 = 176,
	FCK_PKEY_KP_000 = 177,
	FCK_PKEY_THOUSANDSSEPARATOR = 178,
	FCK_PKEY_DECIMALSEPARATOR = 179,
	FCK_PKEY_CURRENCYUNIT = 180,
	FCK_PKEY_CURRENCYSUBUNIT = 181,
	FCK_PKEY_KP_LEFTPAREN = 182,
	FCK_PKEY_KP_RIGHTPAREN = 183,
	FCK_PKEY_KP_LEFTBRACE = 184,
	FCK_PKEY_KP_RIGHTBRACE = 185,
	FCK_PKEY_KP_TAB = 186,
	FCK_PKEY_KP_BACKSPACE = 187,
	FCK_PKEY_KP_A = 188,
	FCK_PKEY_KP_B = 189,
	FCK_PKEY_KP_C = 190,
	FCK_PKEY_KP_D = 191,
	FCK_PKEY_KP_E = 192,
	FCK_PKEY_KP_F = 193,
	FCK_PKEY_KP_XOR = 194,
	FCK_PKEY_KP_POWER = 195,
	FCK_PKEY_KP_PERCENT = 196,
	FCK_PKEY_KP_LESS = 197,
	FCK_PKEY_KP_GREATER = 198,
	FCK_PKEY_KP_AMPERSAND = 199,
	FCK_PKEY_KP_DBLAMPERSAND = 200,
	FCK_PKEY_KP_VERTICALBAR = 201,
	FCK_PKEY_KP_DBLVERTICALBAR = 202,
	FCK_PKEY_KP_COLON = 203,
	FCK_PKEY_KP_HASH = 204,
	FCK_PKEY_KP_SPACE = 205,
	FCK_PKEY_KP_AT = 206,
	FCK_PKEY_KP_EXCLAM = 207,
	FCK_PKEY_KP_MEMSTORE = 208,
	FCK_PKEY_KP_MEMRECALL = 209,
	FCK_PKEY_KP_MEMCLEAR = 210,
	FCK_PKEY_KP_MEMADD = 211,
	FCK_PKEY_KP_MEMSUBTRACT = 212,
	FCK_PKEY_KP_MEMMULTIPLY = 213,
	FCK_PKEY_KP_MEMDIVIDE = 214,
	FCK_PKEY_KP_PLUSMINUS = 215,
	FCK_PKEY_KP_CLEAR = 216,
	FCK_PKEY_KP_CLEARENTRY = 217,
	FCK_PKEY_KP_BINARY = 218,
	FCK_PKEY_KP_OCTAL = 219,
	FCK_PKEY_KP_DECIMAL = 220,
	FCK_PKEY_KP_HEXADECIMAL = 221,

	FCK_PKEY_LCTRL = 224,
	FCK_PKEY_LSHIFT = 225,
	FCK_PKEY_LALT = 226, /**< alt, option */
	FCK_PKEY_LGUI = 227, /**< windows, command (apple), meta */
	FCK_PKEY_RCTRL = 228,
	FCK_PKEY_RSHIFT = 229,
	FCK_PKEY_RALT = 230, /**< alt gr, option */
	FCK_PKEY_RGUI = 231, /**< windows, command (apple), meta */

	FCK_PKEY_MODE = 257, /**< I'm not sure if this is really not covered
	                      *   by any of the above, but since there's a
	                      *   special SDL_KMOD_MODE for it I'm adding it here
	                      */

	/* @} */ /* Usage page 0x07 */

	/**
	 *  \name Usage page 0x0C
	 *
	 *  These values are mapped from usage page 0x0C (USB consumer page).
	 *
	 *  There are way more keys in the spec than we can represent in the
	 *  current scancode range, so pick the ones that commonly come up in
	 *  real world usage.
	 */
	/* @{ */

	FCK_PKEY_SLEEP = 258, /**< Sleep */
	FCK_PKEY_WAKE = 259,  /**< Wake */

	FCK_PKEY_CHANNEL_INCREMENT = 260, /**< Channel Increment */
	FCK_PKEY_CHANNEL_DECREMENT = 261, /**< Channel Decrement */

	FCK_PKEY_MEDIA_PLAY = 262,           /**< Play */
	FCK_PKEY_MEDIA_PAUSE = 263,          /**< Pause */
	FCK_PKEY_MEDIA_RECORD = 264,         /**< Record */
	FCK_PKEY_MEDIA_FAST_FORWARD = 265,   /**< Fast Forward */
	FCK_PKEY_MEDIA_REWIND = 266,         /**< Rewind */
	FCK_PKEY_MEDIA_NEXT_TRACK = 267,     /**< Next Track */
	FCK_PKEY_MEDIA_PREVIOUS_TRACK = 268, /**< Previous Track */
	FCK_PKEY_MEDIA_STOP = 269,           /**< Stop */
	FCK_PKEY_MEDIA_EJECT = 270,          /**< Eject */
	FCK_PKEY_MEDIA_PLAY_PAUSE = 271,     /**< Play / Pause */
	FCK_PKEY_MEDIA_SELECT = 272,         /* Media Select */

	FCK_PKEY_AC_NEW = 273,        /**< AC New */
	FCK_PKEY_AC_OPEN = 274,       /**< AC Open */
	FCK_PKEY_AC_CLOSE = 275,      /**< AC Close */
	FCK_PKEY_AC_EXIT = 276,       /**< AC Exit */
	FCK_PKEY_AC_SAVE = 277,       /**< AC Save */
	FCK_PKEY_AC_PRINT = 278,      /**< AC Print */
	FCK_PKEY_AC_PROPERTIES = 279, /**< AC Properties */

	FCK_PKEY_AC_SEARCH = 280,    /**< AC Search */
	FCK_PKEY_AC_HOME = 281,      /**< AC Home */
	FCK_PKEY_AC_BACK = 282,      /**< AC Back */
	FCK_PKEY_AC_FORWARD = 283,   /**< AC Forward */
	FCK_PKEY_AC_STOP = 284,      /**< AC Stop */
	FCK_PKEY_AC_REFRESH = 285,   /**< AC Refresh */
	FCK_PKEY_AC_BOOKMARKS = 286, /**< AC Bookmarks */

	/* @} */ /* Usage page 0x0C */

	/**
	 *  \name Mobile keys
	 *
	 *  These are values that are often used on mobile phones.
	 */
	/* @{ */

	FCK_PKEY_SOFTLEFT = 287,  /**< Usually situated below the display on phones and
	                                   used as a multi-function feature key for selecting
	                                   a software defined function shown on the bottom left
	                                   of the display. */
	FCK_PKEY_SOFTRIGHT = 288, /**< Usually situated below the display on phones and
	                                   used as a multi-function feature key for selecting
	                                   a software defined function shown on the bottom right
	                                   of the display. */
	FCK_PKEY_CALL = 289,      /**< Used for accepting phone calls. */
	FCK_PKEY_ENDCALL = 290,   /**< Used for rejecting phone calls. */

	/* @} */ /* Mobile keys */

	/* Add any other keys here. */

	FCK_PKEY_RESERVED = 400, /**< 400-500 reserved for dynamic keycodes */

	FCK_PKEY_COUNT = 512 /**< not a key, just marks the number of scancodes for array bounds */

} fck_pkey;

typedef fckc_u32 fck_vkey;

#define FCK_VKEY_EXTENDED_MASK (1u << 29)
#define FCK_VKEY_SCANCODE_MASK (1u << 30)
#define FCK_PKEY_TO_VKEY(X) (X | FCK_VKEY_SCANCODE_MASK)

#define FCK_VKEY_UNKNOWN 0x00000000u              /**< 0 */
#define FCK_VKEY_RETURN 0x0000000du               /**< '\r' */
#define FCK_VKEY_ESCAPE 0x0000001bu               /**< '\x1B' */
#define FCK_VKEY_BACKSPACE 0x00000008u            /**< '\b' */
#define FCK_VKEY_TAB 0x00000009u                  /**< '\t' */
#define FCK_VKEY_SPACE 0x00000020u                /**< ' ' */
#define FCK_VKEY_EXCLAIM 0x00000021u              /**< '!' */
#define FCK_VKEY_DBLAPOSTROPHE 0x00000022u        /**< '"' */
#define FCK_VKEY_HASH 0x00000023u                 /**< '#' */
#define FCK_VKEY_DOLLAR 0x00000024u               /**< '$' */
#define FCK_VKEY_PERCENT 0x00000025u              /**< '%' */
#define FCK_VKEY_AMPERSAND 0x00000026u            /**< '&' */
#define FCK_VKEY_APOSTROPHE 0x00000027u           /**< '\'' */
#define FCK_VKEY_LEFTPAREN 0x00000028u            /**< '(' */
#define FCK_VKEY_RIGHTPAREN 0x00000029u           /**< ')' */
#define FCK_VKEY_ASTERISK 0x0000002au             /**< '*' */
#define FCK_VKEY_PLUS 0x0000002bu                 /**< '+' */
#define FCK_VKEY_COMMA 0x0000002cu                /**< ',' */
#define FCK_VKEY_MINUS 0x0000002du                /**< '-' */
#define FCK_VKEY_PERIOD 0x0000002eu               /**< '.' */
#define FCK_VKEY_SLASH 0x0000002fu                /**< '/' */
#define FCK_VKEY_0 0x00000030u                    /**< '0' */
#define FCK_VKEY_1 0x00000031u                    /**< '1' */
#define FCK_VKEY_2 0x00000032u                    /**< '2' */
#define FCK_VKEY_3 0x00000033u                    /**< '3' */
#define FCK_VKEY_4 0x00000034u                    /**< '4' */
#define FCK_VKEY_5 0x00000035u                    /**< '5' */
#define FCK_VKEY_6 0x00000036u                    /**< '6' */
#define FCK_VKEY_7 0x00000037u                    /**< '7' */
#define FCK_VKEY_8 0x00000038u                    /**< '8' */
#define FCK_VKEY_9 0x00000039u                    /**< '9' */
#define FCK_VKEY_COLON 0x0000003au                /**< ':' */
#define FCK_VKEY_SEMICOLON 0x0000003bu            /**< ';' */
#define FCK_VKEY_LESS 0x0000003cu                 /**< '<' */
#define FCK_VKEY_EQUALS 0x0000003du               /**< '=' */
#define FCK_VKEY_GREATER 0x0000003eu              /**< '>' */
#define FCK_VKEY_QUESTION 0x0000003fu             /**< '?' */
#define FCK_VKEY_AT 0x00000040u                   /**< '@' */
#define FCK_VKEY_LEFTBRACKET 0x0000005bu          /**< '[' */
#define FCK_VKEY_BACKSLASH 0x0000005cu            /**< '\\' */
#define FCK_VKEY_RIGHTBRACKET 0x0000005du         /**< ']' */
#define FCK_VKEY_CARET 0x0000005eu                /**< '^' */
#define FCK_VKEY_UNDERSCORE 0x0000005fu           /**< '_' */
#define FCK_VKEY_GRAVE 0x00000060u                /**< '`' */
#define FCK_VKEY_A 0x00000061u                    /**< 'a' */
#define FCK_VKEY_B 0x00000062u                    /**< 'b' */
#define FCK_VKEY_C 0x00000063u                    /**< 'c' */
#define FCK_VKEY_D 0x00000064u                    /**< 'd' */
#define FCK_VKEY_E 0x00000065u                    /**< 'e' */
#define FCK_VKEY_F 0x00000066u                    /**< 'f' */
#define FCK_VKEY_G 0x00000067u                    /**< 'g' */
#define FCK_VKEY_H 0x00000068u                    /**< 'h' */
#define FCK_VKEY_I 0x00000069u                    /**< 'i' */
#define FCK_VKEY_J 0x0000006au                    /**< 'j' */
#define FCK_VKEY_K 0x0000006bu                    /**< 'k' */
#define FCK_VKEY_L 0x0000006cu                    /**< 'l' */
#define FCK_VKEY_M 0x0000006du                    /**< 'm' */
#define FCK_VKEY_N 0x0000006eu                    /**< 'n' */
#define FCK_VKEY_O 0x0000006fu                    /**< 'o' */
#define FCK_VKEY_P 0x00000070u                    /**< 'p' */
#define FCK_VKEY_Q 0x00000071u                    /**< 'q' */
#define FCK_VKEY_R 0x00000072u                    /**< 'r' */
#define FCK_VKEY_S 0x00000073u                    /**< 's' */
#define FCK_VKEY_T 0x00000074u                    /**< 't' */
#define FCK_VKEY_U 0x00000075u                    /**< 'u' */
#define FCK_VKEY_V 0x00000076u                    /**< 'v' */
#define FCK_VKEY_W 0x00000077u                    /**< 'w' */
#define FCK_VKEY_X 0x00000078u                    /**< 'x' */
#define FCK_VKEY_Y 0x00000079u                    /**< 'y' */
#define FCK_VKEY_Z 0x0000007au                    /**< 'z' */
#define FCK_VKEY_LEFTBRACE 0x0000007bu            /**< '{' */
#define FCK_VKEY_PIPE 0x0000007cu                 /**< '|' */
#define FCK_VKEY_RIGHTBRACE 0x0000007du           /**< '}' */
#define FCK_VKEY_TILDE 0x0000007eu                /**< '~' */
#define FCK_VKEY_DELETE 0x0000007fu               /**< '\x7F' */
#define FCK_VKEY_PLUSMINUS 0x000000b1u            /**< '\xB1' */
#define FCK_VKEY_CAPSLOCK 0x40000039u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CAPSLOCK) */
#define FCK_VKEY_F1 0x4000003au                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F1) */
#define FCK_VKEY_F2 0x4000003bu                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F2) */
#define FCK_VKEY_F3 0x4000003cu                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F3) */
#define FCK_VKEY_F4 0x4000003du                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F4) */
#define FCK_VKEY_F5 0x4000003eu                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F5) */
#define FCK_VKEY_F6 0x4000003fu                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F6) */
#define FCK_VKEY_F7 0x40000040u                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F7) */
#define FCK_VKEY_F8 0x40000041u                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F8) */
#define FCK_VKEY_F9 0x40000042u                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F9) */
#define FCK_VKEY_F10 0x40000043u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F10) */
#define FCK_VKEY_F11 0x40000044u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F11) */
#define FCK_VKEY_F12 0x40000045u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F12) */
#define FCK_VKEY_PRINTSCREEN 0x40000046u          /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PRINTSCREEN) */
#define FCK_VKEY_SCROLLLOCK 0x40000047u           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SCROLLLOCK) */
#define FCK_VKEY_PAUSE 0x40000048u                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAUSE) */
#define FCK_VKEY_INSERT 0x40000049u               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_INSERT) */
#define FCK_VKEY_HOME 0x4000004au                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HOME) */
#define FCK_VKEY_PAGEUP 0x4000004bu               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEUP) */
#define FCK_VKEY_END 0x4000004du                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_END) */
#define FCK_VKEY_PAGEDOWN 0x4000004eu             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PAGEDOWN) */
#define FCK_VKEY_RIGHT 0x4000004fu                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RIGHT) */
#define FCK_VKEY_LEFT 0x40000050u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LEFT) */
#define FCK_VKEY_DOWN 0x40000051u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DOWN) */
#define FCK_VKEY_UP 0x40000052u                   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UP) */
#define FCK_VKEY_NUMLOCKCLEAR 0x40000053u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_NUMLOCKCLEAR) */
#define FCK_VKEY_KP_DIVIDE 0x40000054u            /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DIVIDE) */
#define FCK_VKEY_KP_MULTIPLY 0x40000055u          /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MULTIPLY) */
#define FCK_VKEY_KP_MINUS 0x40000056u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MINUS) */
#define FCK_VKEY_KP_PLUS 0x40000057u              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUS) */
#define FCK_VKEY_KP_ENTER 0x40000058u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_ENTER) */
#define FCK_VKEY_KP_1 0x40000059u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_1) */
#define FCK_VKEY_KP_2 0x4000005au                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_2) */
#define FCK_VKEY_KP_3 0x4000005bu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_3) */
#define FCK_VKEY_KP_4 0x4000005cu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_4) */
#define FCK_VKEY_KP_5 0x4000005du                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_5) */
#define FCK_VKEY_KP_6 0x4000005eu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_6) */
#define FCK_VKEY_KP_7 0x4000005fu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_7) */
#define FCK_VKEY_KP_8 0x40000060u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_8) */
#define FCK_VKEY_KP_9 0x40000061u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_9) */
#define FCK_VKEY_KP_0 0x40000062u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_0) */
#define FCK_VKEY_KP_PERIOD 0x40000063u            /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PERIOD) */
#define FCK_VKEY_APPLICATION 0x40000065u          /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_APPLICATION) */
#define FCK_VKEY_POWER 0x40000066u                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_POWER) */
#define FCK_VKEY_KP_EQUALS 0x40000067u            /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EQUALS) */
#define FCK_VKEY_F13 0x40000068u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F13) */
#define FCK_VKEY_F14 0x40000069u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F14) */
#define FCK_VKEY_F15 0x4000006au                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F15) */
#define FCK_VKEY_F16 0x4000006bu                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F16) */
#define FCK_VKEY_F17 0x4000006cu                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F17) */
#define FCK_VKEY_F18 0x4000006du                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F18) */
#define FCK_VKEY_F19 0x4000006eu                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F19) */
#define FCK_VKEY_F20 0x4000006fu                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F20) */
#define FCK_VKEY_F21 0x40000070u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F21) */
#define FCK_VKEY_F22 0x40000071u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F22) */
#define FCK_VKEY_F23 0x40000072u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F23) */
#define FCK_VKEY_F24 0x40000073u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_F24) */
#define FCK_VKEY_EXECUTE 0x40000074u              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXECUTE) */
#define FCK_VKEY_HELP 0x40000075u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_HELP) */
#define FCK_VKEY_MENU 0x40000076u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MENU) */
#define FCK_VKEY_SELECT 0x40000077u               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SELECT) */
#define FCK_VKEY_STOP 0x40000078u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_STOP) */
#define FCK_VKEY_AGAIN 0x40000079u                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AGAIN) */
#define FCK_VKEY_UNDO 0x4000007au                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_UNDO) */
#define FCK_VKEY_CUT 0x4000007bu                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CUT) */
#define FCK_VKEY_COPY 0x4000007cu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_COPY) */
#define FCK_VKEY_PASTE 0x4000007du                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PASTE) */
#define FCK_VKEY_FIND 0x4000007eu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_FIND) */
#define FCK_VKEY_MUTE 0x4000007fu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MUTE) */
#define FCK_VKEY_VOLUMEUP 0x40000080u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEUP) */
#define FCK_VKEY_VOLUMEDOWN 0x40000081u           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_VOLUMEDOWN) */
#define FCK_VKEY_KP_COMMA 0x40000085u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COMMA) */
#define FCK_VKEY_KP_EQUALSAS400 0x40000086u       /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EQUALSAS400) */
#define FCK_VKEY_ALTERASE 0x40000099u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_ALTERASE) */
#define FCK_VKEY_SYSREQ 0x4000009au               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SYSREQ) */
#define FCK_VKEY_CANCEL 0x4000009bu               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CANCEL) */
#define FCK_VKEY_CLEAR 0x4000009cu                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEAR) */
#define FCK_VKEY_PRIOR 0x4000009du                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_PRIOR) */
#define FCK_VKEY_RETURN2 0x4000009eu              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RETURN2) */
#define FCK_VKEY_SEPARATOR 0x4000009fu            /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SEPARATOR) */
#define FCK_VKEY_OUT 0x400000a0u                  /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OUT) */
#define FCK_VKEY_OPER 0x400000a1u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_OPER) */
#define FCK_VKEY_CLEARAGAIN 0x400000a2u           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CLEARAGAIN) */
#define FCK_VKEY_CRSEL 0x400000a3u                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CRSEL) */
#define FCK_VKEY_EXSEL 0x400000a4u                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_EXSEL) */
#define FCK_VKEY_KP_00 0x400000b0u                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_00) */
#define FCK_VKEY_KP_000 0x400000b1u               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_000) */
#define FCK_VKEY_THOUSANDSSEPARATOR 0x400000b2u   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_THOUSANDSSEPARATOR) */
#define FCK_VKEY_DECIMALSEPARATOR 0x400000b3u     /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_DECIMALSEPARATOR) */
#define FCK_VKEY_CURRENCYUNIT 0x400000b4u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYUNIT) */
#define FCK_VKEY_CURRENCYSUBUNIT 0x400000b5u      /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CURRENCYSUBUNIT) */
#define FCK_VKEY_KP_LEFTPAREN 0x400000b6u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTPAREN) */
#define FCK_VKEY_KP_RIGHTPAREN 0x400000b7u        /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTPAREN) */
#define FCK_VKEY_KP_LEFTBRACE 0x400000b8u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LEFTBRACE) */
#define FCK_VKEY_KP_RIGHTBRACE 0x400000b9u        /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_RIGHTBRACE) */
#define FCK_VKEY_KP_TAB 0x400000bau               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_TAB) */
#define FCK_VKEY_KP_BACKSPACE 0x400000bbu         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BACKSPACE) */
#define FCK_VKEY_KP_A 0x400000bcu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_A) */
#define FCK_VKEY_KP_B 0x400000bdu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_B) */
#define FCK_VKEY_KP_C 0x400000beu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_C) */
#define FCK_VKEY_KP_D 0x400000bfu                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_D) */
#define FCK_VKEY_KP_E 0x400000c0u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_E) */
#define FCK_VKEY_KP_F 0x400000c1u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_F) */
#define FCK_VKEY_KP_XOR 0x400000c2u               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_XOR) */
#define FCK_VKEY_KP_POWER 0x400000c3u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_POWER) */
#define FCK_VKEY_KP_PERCENT 0x400000c4u           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PERCENT) */
#define FCK_VKEY_KP_LESS 0x400000c5u              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_LESS) */
#define FCK_VKEY_KP_GREATER 0x400000c6u           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_GREATER) */
#define FCK_VKEY_KP_AMPERSAND 0x400000c7u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AMPERSAND) */
#define FCK_VKEY_KP_DBLAMPERSAND 0x400000c8u      /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLAMPERSAND) */
#define FCK_VKEY_KP_VERTICALBAR 0x400000c9u       /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_VERTICALBAR) */
#define FCK_VKEY_KP_DBLVERTICALBAR 0x400000cau    /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DBLVERTICALBAR) */
#define FCK_VKEY_KP_COLON 0x400000cbu             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_COLON) */
#define FCK_VKEY_KP_HASH 0x400000ccu              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HASH) */
#define FCK_VKEY_KP_SPACE 0x400000cdu             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_SPACE) */
#define FCK_VKEY_KP_AT 0x400000ceu                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_AT) */
#define FCK_VKEY_KP_EXCLAM 0x400000cfu            /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_EXCLAM) */
#define FCK_VKEY_KP_MEMSTORE 0x400000d0u          /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSTORE) */
#define FCK_VKEY_KP_MEMRECALL 0x400000d1u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMRECALL) */
#define FCK_VKEY_KP_MEMCLEAR 0x400000d2u          /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMCLEAR) */
#define FCK_VKEY_KP_MEMADD 0x400000d3u            /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMADD) */
#define FCK_VKEY_KP_MEMSUBTRACT 0x400000d4u       /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMSUBTRACT) */
#define FCK_VKEY_KP_MEMMULTIPLY 0x400000d5u       /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMMULTIPLY) */
#define FCK_VKEY_KP_MEMDIVIDE 0x400000d6u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_MEMDIVIDE) */
#define FCK_VKEY_KP_PLUSMINUS 0x400000d7u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_PLUSMINUS) */
#define FCK_VKEY_KP_CLEAR 0x400000d8u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEAR) */
#define FCK_VKEY_KP_CLEARENTRY 0x400000d9u        /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_CLEARENTRY) */
#define FCK_VKEY_KP_BINARY 0x400000dau            /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_BINARY) */
#define FCK_VKEY_KP_OCTAL 0x400000dbu             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_OCTAL) */
#define FCK_VKEY_KP_DECIMAL 0x400000dcu           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_DECIMAL) */
#define FCK_VKEY_KP_HEXADECIMAL 0x400000ddu       /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_KP_HEXADECIMAL) */
#define FCK_VKEY_LCTRL 0x400000e0u                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LCTRL) */
#define FCK_VKEY_LSHIFT 0x400000e1u               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LSHIFT) */
#define FCK_VKEY_LALT 0x400000e2u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LALT) */
#define FCK_VKEY_LGUI 0x400000e3u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_LGUI) */
#define FCK_VKEY_RCTRL 0x400000e4u                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RCTRL) */
#define FCK_VKEY_RSHIFT 0x400000e5u               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RSHIFT) */
#define FCK_VKEY_RALT 0x400000e6u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RALT) */
#define FCK_VKEY_RGUI 0x400000e7u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_RGUI) */
#define FCK_VKEY_MODE 0x40000101u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MODE) */
#define FCK_VKEY_SLEEP 0x40000102u                /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SLEEP) */
#define FCK_VKEY_WAKE 0x40000103u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_WAKE) */
#define FCK_VKEY_CHANNEL_INCREMENT 0x40000104u    /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CHANNEL_INCREMENT) */
#define FCK_VKEY_CHANNEL_DECREMENT 0x40000105u    /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CHANNEL_DECREMENT) */
#define FCK_VKEY_MEDIA_PLAY 0x40000106u           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PLAY) */
#define FCK_VKEY_MEDIA_PAUSE 0x40000107u          /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PAUSE) */
#define FCK_VKEY_MEDIA_RECORD 0x40000108u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_RECORD) */
#define FCK_VKEY_MEDIA_FAST_FORWARD 0x40000109u   /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_FAST_FORWARD) */
#define FCK_VKEY_MEDIA_REWIND 0x4000010au         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_REWIND) */
#define FCK_VKEY_MEDIA_NEXT_TRACK 0x4000010bu     /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_NEXT_TRACK) */
#define FCK_VKEY_MEDIA_PREVIOUS_TRACK 0x4000010cu /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PREVIOUS_TRACK) */
#define FCK_VKEY_MEDIA_STOP 0x4000010du           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_STOP) */
#define FCK_VKEY_MEDIA_EJECT 0x4000010eu          /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_EJECT) */
#define FCK_VKEY_MEDIA_PLAY_PAUSE 0x4000010fu     /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_PLAY_PAUSE) */
#define FCK_VKEY_MEDIA_SELECT 0x40000110u         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_MEDIA_SELECT) */
#define FCK_VKEY_AC_NEW 0x40000111u               /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_NEW) */
#define FCK_VKEY_AC_OPEN 0x40000112u              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_OPEN) */
#define FCK_VKEY_AC_CLOSE 0x40000113u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_CLOSE) */
#define FCK_VKEY_AC_EXIT 0x40000114u              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_EXIT) */
#define FCK_VKEY_AC_SAVE 0x40000115u              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_SAVE) */
#define FCK_VKEY_AC_PRINT 0x40000116u             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_PRINT) */
#define FCK_VKEY_AC_PROPERTIES 0x40000117u        /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_PROPERTIES) */
#define FCK_VKEY_AC_SEARCH 0x40000118u            /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_SEARCH) */
#define FCK_VKEY_AC_HOME 0x40000119u              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_HOME) */
#define FCK_VKEY_AC_BACK 0x4000011au              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BACK) */
#define FCK_VKEY_AC_FORWARD 0x4000011bu           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_FORWARD) */
#define FCK_VKEY_AC_STOP 0x4000011cu              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_STOP) */
#define FCK_VKEY_AC_REFRESH 0x4000011du           /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_REFRESH) */
#define FCK_VKEY_AC_BOOKMARKS 0x4000011eu         /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_AC_BOOKMARKS) */
#define FCK_VKEY_SOFTLEFT 0x4000011fu             /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SOFTLEFT) */
#define FCK_VKEY_SOFTRIGHT 0x40000120u            /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_SOFTRIGHT) */
#define FCK_VKEY_CALL 0x40000121u                 /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_CALL) */
#define FCK_VKEY_ENDCALL 0x40000122u              /**< SDL_SCANCODE_TO_KEYCODE(SDL_SCANCODE_ENDCALL) */
#define FCK_VKEY_LEFT_TAB 0x20000001u             /**< Extended key Left Tab */
#define FCK_VKEY_LEVEL5_SHIFT 0x20000002u         /**< Extended key Level 5 Shift */
#define FCK_VKEY_MULTI_KEY_COMPOSE 0x20000003u    /**< Extended key Multi-key Compose */
#define FCK_VKEY_LMETA 0x20000004u                /**< Extended key Left Meta */
#define FCK_VKEY_RMETA 0x20000005u                /**< Extended key Right Meta */
#define FCK_VKEY_LHYPER 0x20000006u               /**< Extended key Left Hyper */
#define FCK_VKEY_RHYPER 0x20000007u               /**< Extended key Right Hyper */

/**
 * Valid key modifiers (possibly OR'd together).
 *
 * \since This datatype is available since SDL 3.2.0.
 */
typedef fckc_u32 fck_vkey_mod;

#define FCK_VKEY_MOD_NONE 0x0000u                                   /**< no modifier is applicable. */
#define FCK_VKEY_MOD_LSHIFT 0x0001u                                 /**< the left Shift key is down. */
#define FCK_VKEY_MOD_RSHIFT 0x0002u                                 /**< the right Shift key is down. */
#define FCK_VKEY_MOD_LEVEL5 0x0004u                                 /**< the Level 5 Shift key is down. */
#define FCK_VKEY_MOD_LCTRL 0x0040u                                  /**< the left Ctrl (Control) key is down. */
#define FCK_VKEY_MOD_RCTRL 0x0080u                                  /**< the right Ctrl (Control) key is down. */
#define FCK_VKEY_MOD_LALT 0x0100u                                   /**< the left Alt key is down. */
#define FCK_VKEY_MOD_RALT 0x0200u                                   /**< the right Alt key is down. */
#define FCK_VKEY_MOD_LGUI 0x0400u                                   /**< the left GUI key (often the Windows key) is down. */
#define FCK_VKEY_MOD_RGUI 0x0800u                                   /**< the right GUI key (often the Windows key) is down. */
#define FCK_VKEY_MOD_NUM 0x1000u                                    /**< the Num Lock key (may be located on an extended keypad) is down. */
#define FCK_VKEY_MOD_CAPS 0x2000u                                   /**< the Caps Lock key is down. */
#define FCK_VKEY_MOD_MODE 0x4000u                                   /**< the !AltGr key is down. */
#define FCK_VKEY_MOD_SCROLL 0x8000u                                 /**< the Scroll Lock key is down. */
#define FCK_VKEY_MOD_CTRL (FCK_VKEY_MOD_LCTRL | FCK_VKEY_MOD_RCTRL) /**< Any Ctrl key is down. */
#define FCK_VKEY_MOD_SHIFT (FCK_VKEY_MOD_LSHIFT | FCK_VKEY_MOD_RSHIFT) /**< Any Shift key is down. */
#define FCK_VKEY_MOD_ALT (FCK_VKEY_MOD_LALT | FCK_VKEY_MOD_RALT)       /**< Any Alt key is down. */
#define FCK_VKEY_MOD_GUI (FCK_VKEY_MOD_LGUI | FCK_VKEY_MOD_RGUI)       /**< Any GUI key is down. */

typedef union fck_event_input_common {
	struct
	{
		fck_event_input_type type;
		fckc_u32 size;
		fckc_u64 timestamp;
	};
	fckc_u64 pad;
} fck_event_input_common;

typedef struct fck_event_input_device
{
	fck_event_input_common common;
	fck_input_device_type device_type;
} fck_event_input_device;

typedef struct fck_event_input_device_mouse
{
	fck_event_input_common common;
	fck_input_device_type device_type;

	fck_mouse_event_type type : 15;
	fckc_u32 is_down : 1;
	fckc_u32 clicks : 16;

	fckc_f32 x;
	fckc_f32 y;
	fckc_f32 dx;
	fckc_f32 dy;
} fck_event_input_device_mouse;

typedef struct fck_event_input_device_keyboard
{
	fck_event_input_common common;
	fck_input_device_type device_type;

	fck_keyboard_event_type type;
	fck_vkey_mod mod;

	fck_pkey pkey;
	fck_vkey vkey;
	fckc_u32 pad;
} fck_event_input_device_keyboard;

typedef struct fck_event_input_text
{
	fck_event_input_common common;
	const char *text;
} fck_event_input_text;

typedef struct fck_event
{
	fck_event_input_common common;
	fckc_u8 opaque[48];
} fck_event;

#endif // !FCK_EVENTS_H_INCLUDED
