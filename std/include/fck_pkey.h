#ifndef FCK_PKEY_INCLUDED
#define FCK_PKEY_INCLUDED

// taken from sdl_scancode - shamelessly
typedef enum fck_pkey_type
{
	fck_pkey_unknown = 0,

	fck_pkey_a = 4,
	fck_pkey_b = 5,
	fck_pkey_c = 6,
	fck_pkey_d = 7,
	fck_pkey_e = 8,
	fck_pkey_f = 9,
	fck_pkey_g = 10,
	fck_pkey_h = 11,
	fck_pkey_i = 12,
	fck_pkey_j = 13,
	fck_pkey_k = 14,
	fck_pkey_l = 15,
	fck_pkey_m = 16,
	fck_pkey_n = 17,
	fck_pkey_o = 18,
	fck_pkey_p = 19,
	fck_pkey_q = 20,
	fck_pkey_r = 21,
	fck_pkey_s = 22,
	fck_pkey_t = 23,
	fck_pkey_u = 24,
	fck_pkey_v = 25,
	fck_pkey_w = 26,
	fck_pkey_x = 27,
	fck_pkey_y = 28,
	fck_pkey_z = 29,

	fck_pkey_1 = 30,
	fck_pkey_2 = 31,
	fck_pkey_3 = 32,
	fck_pkey_4 = 33,
	fck_pkey_5 = 34,
	fck_pkey_6 = 35,
	fck_pkey_7 = 36,
	fck_pkey_8 = 37,
	fck_pkey_9 = 38,
	fck_pkey_0 = 39,

	fck_pkey_return = 40,
	fck_pkey_escape = 41,
	fck_pkey_backspace = 42,
	fck_pkey_tab = 43,
	fck_pkey_space = 44,

	fck_pkey_minus = 45,
	fck_pkey_equals = 46,
	fck_pkey_leftbracket = 47,
	fck_pkey_rightbracket = 48,
	fck_pkey_backslash = 49,
	fck_pkey_nonushash = 50,
	fck_pkey_semicolon = 51,
	fck_pkey_apostrophe = 52,
	fck_pkey_grave = 53,
	fck_pkey_comma = 54,
	fck_pkey_period = 55,
	fck_pkey_slash = 56,

	fck_pkey_capslock = 57,

	fck_pkey_f1 = 58,
	fck_pkey_f2 = 59,
	fck_pkey_f3 = 60,
	fck_pkey_f4 = 61,
	fck_pkey_f5 = 62,
	fck_pkey_f6 = 63,
	fck_pkey_f7 = 64,
	fck_pkey_f8 = 65,
	fck_pkey_f9 = 66,
	fck_pkey_f10 = 67,
	fck_pkey_f11 = 68,
	fck_pkey_f12 = 69,

	fck_pkey_printscreen = 70,
	fck_pkey_scrolllock = 71,
	fck_pkey_pause = 72,
	fck_pkey_insert = 73,

	fck_pkey_home = 74,
	fck_pkey_pageup = 75,
	fck_pkey_delete = 76,
	fck_pkey_end = 77,
	fck_pkey_pagedown = 78,
	fck_pkey_right = 79,
	fck_pkey_left = 80,
	fck_pkey_down = 81,
	fck_pkey_up = 82,

	fck_pkey_numlockclear = 83,

	fck_pkey_kp_divide = 84,
	fck_pkey_kp_multiply = 85,
	fck_pkey_kp_minus = 86,
	fck_pkey_kp_plus = 87,
	fck_pkey_kp_enter = 88,
	fck_pkey_kp_1 = 89,
	fck_pkey_kp_2 = 90,
	fck_pkey_kp_3 = 91,
	fck_pkey_kp_4 = 92,
	fck_pkey_kp_5 = 93,
	fck_pkey_kp_6 = 94,
	fck_pkey_kp_7 = 95,
	fck_pkey_kp_8 = 96,
	fck_pkey_kp_9 = 97,
	fck_pkey_kp_0 = 98,
	fck_pkey_kp_period = 99,

	fck_pkey_nonusbackslash = 100,

	fck_pkey_application = 101,
	fck_pkey_power = 102,

	fck_pkey_kp_equals = 103,
	fck_pkey_f13 = 104,
	fck_pkey_f14 = 105,
	fck_pkey_f15 = 106,
	fck_pkey_f16 = 107,
	fck_pkey_f17 = 108,
	fck_pkey_f18 = 109,
	fck_pkey_f19 = 110,
	fck_pkey_f20 = 111,
	fck_pkey_f21 = 112,
	fck_pkey_f22 = 113,
	fck_pkey_f23 = 114,
	fck_pkey_f24 = 115,
	fck_pkey_execute = 116,
	fck_pkey_help = 117, /**< al integrated help center */
	fck_pkey_menu = 118, /**< menu (show menu) */
	fck_pkey_select = 119,
	fck_pkey_stop = 120,  /**< ac stop */
	fck_pkey_again = 121, /**< ac redo/repeat */
	fck_pkey_undo = 122,  /**< ac undo */
	fck_pkey_cut = 123,   /**< ac cut */
	fck_pkey_copy = 124,  /**< ac copy */
	fck_pkey_paste = 125, /**< ac paste */
	fck_pkey_find = 126,  /**< ac find */
	fck_pkey_mute = 127,
	fck_pkey_volumeup = 128,
	fck_pkey_volumedown = 129,
	/* not sure whether there's a reason to enable these */
	/*     fck_pkey_lockingcapslock = 130,  */
	/*     fck_pkey_lockingnumlock = 131, */
	/*     fck_pkey_lockingscrolllock = 132, */
	fck_pkey_kp_comma = 133,
	fck_pkey_kp_equalsas400 = 134,

	fck_pkey_international1 = 135, /**< used on asian keyboards, see
											footnotes in usb doc */
	fck_pkey_international2 = 136,
	fck_pkey_international3 = 137, /**< yen */
	fck_pkey_international4 = 138,
	fck_pkey_international5 = 139,
	fck_pkey_international6 = 140,
	fck_pkey_international7 = 141,
	fck_pkey_international8 = 142,
	fck_pkey_international9 = 143,
	fck_pkey_lang1 = 144, /**< hangul/english toggle */
	fck_pkey_lang2 = 145, /**< hanja conversion */
	fck_pkey_lang3 = 146, /**< katakana */
	fck_pkey_lang4 = 147, /**< hiragana */
	fck_pkey_lang5 = 148, /**< zenkaku/hankaku */
	fck_pkey_lang6 = 149, /**< reserved */
	fck_pkey_lang7 = 150, /**< reserved */
	fck_pkey_lang8 = 151, /**< reserved */
	fck_pkey_lang9 = 152, /**< reserved */

	fck_pkey_alterase = 153, /**< erase-eaze */
	fck_pkey_sysreq = 154,
	fck_pkey_cancel = 155, /**< ac cancel */
	fck_pkey_clear = 156,
	fck_pkey_prior = 157,
	fck_pkey_return2 = 158,
	fck_pkey_separator = 159,
	fck_pkey_out = 160,
	fck_pkey_oper = 161,
	fck_pkey_clearagain = 162,
	fck_pkey_crsel = 163,
	fck_pkey_exsel = 164,

	fck_pkey_kp_00 = 176,
	fck_pkey_kp_000 = 177,
	fck_pkey_thousandsseparator = 178,
	fck_pkey_decimalseparator = 179,
	fck_pkey_currencyunit = 180,
	fck_pkey_currencysubunit = 181,
	fck_pkey_kp_leftparen = 182,
	fck_pkey_kp_rightparen = 183,
	fck_pkey_kp_leftbrace = 184,
	fck_pkey_kp_rightbrace = 185,
	fck_pkey_kp_tab = 186,
	fck_pkey_kp_backspace = 187,
	fck_pkey_kp_a = 188,
	fck_pkey_kp_b = 189,
	fck_pkey_kp_c = 190,
	fck_pkey_kp_d = 191,
	fck_pkey_kp_e = 192,
	fck_pkey_kp_f = 193,
	fck_pkey_kp_xor = 194,
	fck_pkey_kp_power = 195,
	fck_pkey_kp_percent = 196,
	fck_pkey_kp_less = 197,
	fck_pkey_kp_greater = 198,
	fck_pkey_kp_ampersand = 199,
	fck_pkey_kp_dblampersand = 200,
	fck_pkey_kp_verticalbar = 201,
	fck_pkey_kp_dblverticalbar = 202,
	fck_pkey_kp_colon = 203,
	fck_pkey_kp_hash = 204,
	fck_pkey_kp_space = 205,
	fck_pkey_kp_at = 206,
	fck_pkey_kp_exclam = 207,
	fck_pkey_kp_memstore = 208,
	fck_pkey_kp_memrecall = 209,
	fck_pkey_kp_memclear = 210,
	fck_pkey_kp_memadd = 211,
	fck_pkey_kp_memsubtract = 212,
	fck_pkey_kp_memmultiply = 213,
	fck_pkey_kp_memdivide = 214,
	fck_pkey_kp_plusminus = 215,
	fck_pkey_kp_clear = 216,
	fck_pkey_kp_clearentry = 217,
	fck_pkey_kp_binary = 218,
	fck_pkey_kp_octal = 219,
	fck_pkey_kp_decimal = 220,
	fck_pkey_kp_hexadecimal = 221,

	fck_pkey_lctrl = 224,
	fck_pkey_lshift = 225,
	fck_pkey_lalt = 226, /**< alt, option */
	fck_pkey_lgui = 227, /**< windows, command (apple), meta */
	fck_pkey_rctrl = 228,
	fck_pkey_rshift = 229,
	fck_pkey_ralt = 230, /**< alt gr, option */
	fck_pkey_rgui = 231, /**< windows, command (apple), meta */

	fck_pkey_mode = 257, /**< i'm not sure if this is really not covered
						  *   by any of the above, but since there's a
						  *   special sdl_kmod_mode for it i'm adding it here
						  */

						  /* @} */ /* usage page 0x07 */

						  /**
						   *  \name usage page 0x0c
						   *
						   *  these values are mapped from usage page 0x0c (usb consumer page).
						   *
						   *  there are way more keys in the spec than we can represent in the
						   *  current scancode range, so pick the ones that commonly come up in
						   *  real world usage.
						   */
						   /* @{ */

	fck_pkey_sleep = 258, /**< sleep */
	fck_pkey_wake = 259,  /**< wake */

	fck_pkey_channel_increment = 260, /**< channel increment */
	fck_pkey_channel_decrement = 261, /**< channel decrement */

	fck_pkey_media_play = 262,           /**< play */
	fck_pkey_media_pause = 263,          /**< pause */
	fck_pkey_media_record = 264,         /**< record */
	fck_pkey_media_fast_forward = 265,   /**< fast forward */
	fck_pkey_media_rewind = 266,         /**< rewind */
	fck_pkey_media_next_track = 267,     /**< next track */
	fck_pkey_media_previous_track = 268, /**< previous track */
	fck_pkey_media_stop = 269,           /**< stop */
	fck_pkey_media_eject = 270,          /**< eject */
	fck_pkey_media_play_pause = 271,     /**< play / pause */
	fck_pkey_media_select = 272,         /* media select */

	fck_pkey_ac_new = 273,        /**< ac new */
	fck_pkey_ac_open = 274,       /**< ac open */
	fck_pkey_ac_close = 275,      /**< ac close */
	fck_pkey_ac_exit = 276,       /**< ac exit */
	fck_pkey_ac_save = 277,       /**< ac save */
	fck_pkey_ac_print = 278,      /**< ac print */
	fck_pkey_ac_properties = 279, /**< ac properties */

	fck_pkey_ac_search = 280,    /**< ac search */
	fck_pkey_ac_home = 281,      /**< ac home */
	fck_pkey_ac_back = 282,      /**< ac back */
	fck_pkey_ac_forward = 283,   /**< ac forward */
	fck_pkey_ac_stop = 284,      /**< ac stop */
	fck_pkey_ac_refresh = 285,   /**< ac refresh */
	fck_pkey_ac_bookmarks = 286, /**< ac bookmarks */
	fck_pkey_softleft = 287,     /**< usually situated below the display on phones and
										  used as a multi-function feature key for selecting
										  a software defined function shown on the bottom left
										  of the display. */
	fck_pkey_softright = 288,    /**< usually situated below the display on phones and
										  used as a multi-function feature key for selecting
										  a software defined function shown on the bottom right
										  of the display. */
	fck_pkey_call = 289,         /**< used for accepting phone calls. */
	fck_pkey_endcall = 290,      /**< used for rejecting phone calls. */

	/* @} */ /* mobile keys */

	/* add any other keys here. */

	fck_pkey_reserved = 400, /**< 400-500 reserved for dynamic keycodes */

	fck_pkey_count = 512 /**< not a key, just marks the number of scancodes for array bounds */

} fck_pkey_type;

#endif // !FCK_PKEY_INCLUDED
