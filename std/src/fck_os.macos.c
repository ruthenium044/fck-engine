#define FCK_STD_EXPORT
#include "fck_os.h"

#include <fck_events.h>
#include <fckc_assert.h>
#include <fckc_atomic.h>
#include <fckc_math.h>

// #include <AppKit/NSEvent.h>
#include <ApplicationServices/ApplicationServices.h>
#include <CoreFoundation/CFDate.h>
#include <CoreVideo/CVDisplayLink.h>

#include <Carbon/Carbon.h>

#include <objc/message.h>
#include <objc/runtime.h>

#include <dlfcn.h>

#include <kll.h>
#include <kll_heap.h>
#include <kll_malloc.h>

typedef CFTimeInterval NSTimeInterval;
typedef CGRect NSRect;
typedef CGPoint NSPoint;
typedef CGSize NSSize;

typedef unsigned long NSUInteger;
typedef long NSInteger;
typedef unsigned short NSUShort;

#define NS_ENUM(type, name)                                                                                                                \
	type name;                                                                                                                             \
	enum

typedef NS_ENUM(NSUInteger, NSBackingStoreType) {
	NSBackingStoreRetained = 0,
	NSBackingStoreNonretained = 1,
	NSBackingStoreBuffered = 2
};

typedef NS_ENUM(NSUInteger, NSWindowStyleMask) {
	NSWindowStyleMaskBorderless = 0,
	NSWindowStyleMaskTitled = 1 << 0,
	NSWindowStyleMaskClosable = 1 << 1,
	NSWindowStyleMaskMiniaturizable = 1 << 2,
	NSWindowStyleMaskResizable = 1 << 3,
	NSWindowStyleMaskTexturedBackground = 1 << 8, /* deprecated */
	NSWindowStyleMaskUnifiedTitleAndToolbar = 1 << 12,
	NSWindowStyleMaskFullScreen = 1 << 14,
	NSWindowStyleMaskFullSizeContentView = 1 << 15,
	NSWindowStyleMaskUtilityWindow = 1 << 4,
	NSWindowStyleMaskDocModalWindow = 1 << 6,
	NSWindowStyleMaskNonactivatingPanel = 1 << 7,
	NSWindowStyleMaskHUDWindow = 1 << 13
};

typedef NS_ENUM(NSUInteger, NSEventModifier) {
	NSEventModifierCapsLock = 16,   // Set if Caps Lock key is pressed.
	NSEventModifierShift = 17,      // Set if Shift key is pressed.
	NSEventModifierControl = 18,    // Set if Control key is pressed.
	NSEventModifierOption = 19,     // Set if Option or Alternate key is pressed.
	NSEventModifierCommand = 20,    // Set if Command key is pressed.
	NSEventModifierNumericPad = 21, // Set if any key in the numeric keypad is pressed.
	NSEventModifierHelp = 22,       // Set if the Help key is pressed.
	NSEventModifierFunction = 23,   // Set if any function key is pressed.
};

NSEventModifier fck_pkey_to_modifier[] = {
	[kVK_CapsLock] = NSEventModifierCapsLock,    //
	[kVK_Shift] = NSEventModifierShift,          //
	[kVK_RightShift] = NSEventModifierShift,     //
	[kVK_Control] = NSEventModifierControl,      //
	[kVK_RightControl] = NSEventModifierControl, //
	[kVK_Option] = NSEventModifierOption,        //
	[kVK_RightOption] = NSEventModifierOption,   //
	[kVK_Command] = NSEventModifierCommand,      //
	[kVK_RightCommand] = NSEventModifierCommand, //
	//[Numpad??] = NSEventModifierFunction,    //
	[kVK_Help] = NSEventModifierHelp, //
	[kVK_Function] = NSEventModifierFunction,
	// TODO:
    // NSEventModifierNumericPad = 21,
    // NSEventModifierHelp = 22,
    // NSEventModifierFunction = 23,

};

typedef NS_ENUM(NSUInteger, NSEventModifierFlags) {
	NSEventModifierFlagCapsLock = 1 << NSEventModifierCapsLock,     // Set if Caps Lock key is pressed.
	NSEventModifierFlagShift = 1 << NSEventModifierShift,           // Set if Shift key is pressed.
	NSEventModifierFlagControl = 1 << NSEventModifierControl,       // Set if Control key is pressed.
	NSEventModifierFlagOption = 1 << NSEventModifierOption,         // Set if Option or Alternate key is pressed.
	NSEventModifierFlagCommand = 1 << NSEventModifierCommand,       // Set if Command key is pressed.
	NSEventModifierFlagNumericPad = 1 << NSEventModifierNumericPad, // Set if any key in the numeric keypad is pressed.
	NSEventModifierFlagHelp = 1 << NSEventModifierHelp,             // Set if the Help key is pressed.
	NSEventModifierFlagFunction = 1 << NSEventModifierFunction,     // Set if any function key is pressed.
};

typedef enum NSApplicationActivationPolicy
{
	NSApplicationActivationPolicyRegular,
	NSApplicationActivationPolicyAccessory,
	NSApplicationActivationPolicyProhibited
} NSApplicationActivationPolicy;

/*
 * arm:    objc_msgSend_fpret not used
 * i386:   objc_msgSend_fpret used for `float`, `double`, `long double`.
 * x86-64: objc_msgSend_fpret used for `long double`.
 *
 * arm:    objc_msgSend_fp2ret not used
 * i386:   objc_msgSend_fp2ret not used
 * x86-64: objc_msgSend_fp2ret used for `_Complex long double`.
 */

/* ARM just uses objc_msgSend */
#ifdef __arm64__
#define abi_objc_msgSend_stret objc_msgSend
#define abi_objc_msgSend_fpret objc_msgSend
#else /* __i386__ */
/* x86 just uses abi_objc_msgSend_fpret and (NSColor *)objc_msgSend_id respectively */
#define abi_objc_msgSend_stret objc_msgSend_stret
#define abi_objc_msgSend_fpret objc_msgSend_fpret
#endif

typedef void NSEvent;
typedef void NSString;

typedef void NSObject;
typedef void NSApplication;
typedef void NSWindow;
typedef void NSView;

#define objc_msgSend_bool ((BOOL (*)(id, SEL))objc_msgSend)
#define objc_msgSend_id ((id (*)(id, SEL))objc_msgSend)
#define objc_msgSend_address ((void *(*)(id, SEL))objc_msgSend)
#define objc_msgSend_string ((NSString * (*)(id, SEL)) objc_msgSend)
#define objc_msgSend_ushort ((NSUShort (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_int ((void (*)(id, SEL, NSInteger))objc_msgSend)
#define objc_msgSend_uint ((NSUInteger (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_id ((void (*)(id, SEL, id))objc_msgSend)
#define objc_msgSend_id_eventmask_block ((id (*)(id, SEL, NSEventMask, void *))objc_msgSend)
#define objc_msgSend_void_bool ((void (*)(id, SEL, BOOL))objc_msgSend)

// TODO: RESPECT ABI!
#define objc_msgSend_double ((double (*)(id, SEL))objc_msgSend)
#define objc_msgSend_cgfloat ((CGFloat (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void_rect_bool ((void (*)(id, SEL, NSRect, BOOL))objc_msgSend)
#define objc_msgSend_rect ((NSRect (*)(id, SEL))objc_msgSend)

#define objc_msgSend_int ((NSInteger (*)(id, SEL))objc_msgSend)
#define objc_msgSend_void ((void (*)(id, SEL))objc_msgSend)

#define NSAlloc(nsclass) objc_msgSend_id((id)(nsclass), sel_registerName("alloc"))
#define NSRelease(nsclass) objc_msgSend_id((id)(nsclass), sel_registerName("release"))

#define name(target_type) sizeof(target_type) ? #target_type : NULL

fck_pkey fck_macos_keys[255] = {
	[kVK_ANSI_A] = FCK_PKEY_A,
	[kVK_ANSI_S] = FCK_PKEY_S,
	[kVK_ANSI_D] = FCK_PKEY_D,
	[kVK_ANSI_F] = FCK_PKEY_F,
	[kVK_ANSI_H] = FCK_PKEY_H,
	[kVK_ANSI_G] = FCK_PKEY_G,
	[kVK_ANSI_Z] = FCK_PKEY_Z,
	[kVK_ANSI_X] = FCK_PKEY_X,
	[kVK_ANSI_C] = FCK_PKEY_C,
	[kVK_ANSI_V] = FCK_PKEY_V,
	[kVK_ISO_Section] = FCK_PKEY_NONUSBACKSLASH, //
	[kVK_ANSI_B] = FCK_PKEY_B,
	[kVK_ANSI_Q] = FCK_PKEY_Q,
	[kVK_ANSI_W] = FCK_PKEY_W,
	[kVK_ANSI_E] = FCK_PKEY_E,
	[kVK_ANSI_R] = FCK_PKEY_R,
	[kVK_ANSI_Y] = FCK_PKEY_Y,
	[kVK_ANSI_T] = FCK_PKEY_T,
	[kVK_ANSI_1] = FCK_PKEY_1,
	[kVK_ANSI_2] = FCK_PKEY_2,
	[kVK_ANSI_3] = FCK_PKEY_3,
	[kVK_ANSI_4] = FCK_PKEY_4,
	[kVK_ANSI_6] = FCK_PKEY_6,
	[kVK_ANSI_5] = FCK_PKEY_5,
	[kVK_ANSI_Equal] = FCK_PKEY_EQUALS,
	[kVK_ANSI_9] = FCK_PKEY_9,
	[kVK_ANSI_7] = FCK_PKEY_7,
	[kVK_ANSI_Minus] = FCK_PKEY_MINUS,
	[kVK_ANSI_8] = FCK_PKEY_8,
	[kVK_ANSI_0] = FCK_PKEY_0,
	[kVK_ANSI_RightBracket] = FCK_PKEY_RIGHTBRACKET,
	[kVK_ANSI_O] = FCK_PKEY_O,
	[kVK_ANSI_U] = FCK_PKEY_U,
	[kVK_ANSI_LeftBracket] = FCK_PKEY_LEFTBRACKET,
	[kVK_ANSI_I] = FCK_PKEY_I,
	[kVK_ANSI_P] = FCK_PKEY_P,
	[kVK_Return] = FCK_PKEY_RETURN,
	[kVK_ANSI_L] = FCK_PKEY_L,
	[kVK_ANSI_J] = FCK_PKEY_J,
	[kVK_ANSI_Quote] = FCK_PKEY_APOSTROPHE,
	[kVK_ANSI_K] = FCK_PKEY_K,
	[kVK_ANSI_Semicolon] = FCK_PKEY_SEMICOLON,
	[kVK_ANSI_Backslash] = FCK_PKEY_BACKSLASH,
	[kVK_ANSI_Comma] = FCK_PKEY_COMMA,
	[kVK_ANSI_Slash] = FCK_PKEY_SLASH,
	[kVK_ANSI_N] = FCK_PKEY_N,
	[kVK_ANSI_M] = FCK_PKEY_M,
	[kVK_ANSI_Period] = FCK_PKEY_PERIOD,
	[kVK_Tab] = FCK_PKEY_TAB,
	[kVK_Space] = FCK_PKEY_SPACE,
	[kVK_ANSI_Grave] = FCK_PKEY_GRAVE,
	[kVK_Delete] = FCK_PKEY_BACKSPACE,
	[0x34] = FCK_PKEY_KP_ENTER, // keyboard enter on portables
	[kVK_Escape] = FCK_PKEY_ESCAPE,
	[kVK_RightCommand] = FCK_PKEY_RGUI,
	[kVK_Command] = FCK_PKEY_LGUI,
	[kVK_Shift] = FCK_PKEY_LSHIFT,
	[kVK_RightShift] = FCK_PKEY_RSHIFT,
	[kVK_CapsLock] = FCK_PKEY_CAPSLOCK,
	[kVK_Option] = FCK_PKEY_LALT,
	[kVK_RightOption] = FCK_PKEY_RALT,
	[kVK_Control] = FCK_PKEY_LCTRL,
	[kVK_RightControl] = FCK_PKEY_RCTRL,
	[kVK_Function] = FCK_PKEY_UNKNOWN, // FUCK
	[kVK_F17] = FCK_PKEY_F17,
	[kVK_ANSI_KeypadDecimal] = FCK_PKEY_KP_PERIOD,
	[0x42] = FCK_PKEY_UNKNOWN, // unknown (unused?)
	[kVK_ANSI_KeypadMultiply] = FCK_PKEY_KP_MULTIPLY,
	[0x44] = FCK_PKEY_UNKNOWN, // unknown (unused?)
	[kVK_ANSI_KeypadPlus] = FCK_PKEY_KP_PLUS,
	[0x46] = FCK_PKEY_UNKNOWN, // unknown (unused?)
	[kVK_ANSI_KeypadClear] = FCK_PKEY_NUMLOCKCLEAR,
	[kVK_VolumeUp] = FCK_PKEY_VOLUMEUP,
	[kVK_VolumeDown] = FCK_PKEY_VOLUMEDOWN,
	[kVK_Mute] = FCK_PKEY_MUTE,
	[kVK_ANSI_KeypadDivide] = FCK_PKEY_KP_DIVIDE,
	[kVK_ANSI_KeypadEnter] = FCK_PKEY_KP_ENTER, // keypad enter on external keyboards, fn-return on portables
	[kVK_ANSI_KeypadMinus] = FCK_PKEY_KP_MINUS,
	[kVK_F18] = FCK_PKEY_F18,
	[kVK_F19] = FCK_PKEY_F19,
	[kVK_ANSI_KeypadEquals] = FCK_PKEY_KP_EQUALS,
	[kVK_ANSI_Keypad0] = FCK_PKEY_KP_0,
	[kVK_ANSI_Keypad1] = FCK_PKEY_KP_1,
	[kVK_ANSI_Keypad2] = FCK_PKEY_KP_2,
	[kVK_ANSI_Keypad3] = FCK_PKEY_KP_3,
	[kVK_ANSI_Keypad4] = FCK_PKEY_KP_4,
	[kVK_ANSI_Keypad5] = FCK_PKEY_KP_5,
	[kVK_ANSI_Keypad6] = FCK_PKEY_KP_6,
	[kVK_ANSI_Keypad7] = FCK_PKEY_KP_7,
	[kVK_ANSI_Keypad8] = FCK_PKEY_KP_8,
	[kVK_ANSI_Keypad9] = FCK_PKEY_KP_9,
	[kVK_JIS_Yen] = FCK_PKEY_INTERNATIONAL3,        // Cosmo_USB2ADB.c says "Yen (JIS)"
	[kVK_JIS_Underscore] = FCK_PKEY_INTERNATIONAL1, // Cosmo_USB2ADB.c says "Ro (JIS)"
	[kVK_JIS_KeypadComma] = FCK_PKEY_KP_COMMA,      // Cosmo_USB2ADB.c says ", JIS only"
	[kVK_F5] = FCK_PKEY_F5,
	[kVK_F6] = FCK_PKEY_F6,
	[kVK_F7] = FCK_PKEY_F7,
	[kVK_F3] = FCK_PKEY_F3,
	[kVK_F8] = FCK_PKEY_F8,
	[kVK_F9] = FCK_PKEY_F9,
	[kVK_JIS_Eisu] = FCK_PKEY_LANG2, // Cosmo_USB2ADB.c says "Eisu"
	[kVK_F11] = FCK_PKEY_F11,
	[kVK_JIS_Kana] = FCK_PKEY_LANG1, // Cosmo_USB2ADB.c says "Kana"
	[kVK_F13] = FCK_PKEY_PRINTSCREEN,
	[kVK_F16] = FCK_PKEY_F16,
	[kVK_F14] = FCK_PKEY_SCROLLLOCK, // F14/scroll lock, see comment about F13/print screen above
	[0x6C] = FCK_PKEY_UNKNOWN,       // unknown (unused?)
	[kVK_F10] = FCK_PKEY_F10,
	[0x6E] = FCK_PKEY_APPLICATION, // windows contextual menu key, fn-enter on portables
	[kVK_F12] = FCK_PKEY_F12,
	[0x70] = FCK_PKEY_UNKNOWN,  // unknown (unused?)
	[kVK_F15] = FCK_PKEY_PAUSE, // F15/pause, see comment about F13/print screen above
	[kVK_Help] = FCK_PKEY_INSERT,
	[kVK_Home] = FCK_PKEY_HOME,
	[kVK_PageUp] = FCK_PKEY_PAGEUP,
	[kVK_ForwardDelete] = FCK_PKEY_DELETE,
	[kVK_F4] = FCK_PKEY_F4,
	[kVK_End] = FCK_PKEY_END,
	[kVK_F2] = FCK_PKEY_F2,
	[kVK_PageDown] = FCK_PKEY_PAGEDOWN,
	[kVK_F1] = FCK_PKEY_F1,
	[kVK_LeftArrow] = FCK_PKEY_LEFT,
	[kVK_RightArrow] = FCK_PKEY_RIGHT,
	[kVK_DownArrow] = FCK_PKEY_DOWN,
	[kVK_UpArrow] = FCK_PKEY_UP,
	[0x7F] = FCK_PKEY_POWER,
};

typedef NS_ENUM(NSUInteger, NSEventType) { /* various types of events */
	NSEventTypeLeftMouseDown = 1,
	NSEventTypeLeftMouseUp = 2,
	NSEventTypeRightMouseDown = 3,
	NSEventTypeRightMouseUp = 4,
	NSEventTypeMouseMoved = 5,
	NSEventTypeLeftMouseDragged = 6,
	NSEventTypeRightMouseDragged = 7,
	NSEventTypeMouseEntered = 8,
	NSEventTypeMouseExited = 9,
	NSEventTypeKeyDown = 10,
	NSEventTypeKeyUp = 11,
	NSEventTypeFlagsChanged = 12,
	NSEventTypeAppKitDefined = 13,
	NSEventTypeSystemDefined = 14,
	NSEventTypeApplicationDefined = 15,
	NSEventTypePeriodic = 16,
	NSEventTypeCursorUpdate = 17,
	NSEventTypeScrollWheel = 22,
	NSEventTypeTabletPoint = 23,
	NSEventTypeTabletProximity = 24,
	NSEventTypeOtherMouseDown = 25,
	NSEventTypeOtherMouseUp = 26,
	NSEventTypeOtherMouseDragged = 27,
	/* The following event types are available on some hardware on 10.5.2 and later */
	NSEventTypeGesture API_AVAILABLE(macos(10.5)) = 29,
	NSEventTypeMagnify API_AVAILABLE(macos(10.5)) = 30,
	NSEventTypeSwipe API_AVAILABLE(macos(10.5)) = 31,
	NSEventTypeRotate API_AVAILABLE(macos(10.5)) = 18,
	NSEventTypeBeginGesture API_AVAILABLE(macos(10.5)) = 19,
	NSEventTypeEndGesture API_AVAILABLE(macos(10.5)) = 20,

	NSEventTypeSmartMagnify API_AVAILABLE(macos(10.8)) = 32,
	NSEventTypeQuickLook API_AVAILABLE(macos(10.8)) = 33,

	NSEventTypePressure API_AVAILABLE(macos(10.10.3)) = 34,
	NSEventTypeDirectTouch API_AVAILABLE(macos(10.10)) = 37,

	NSEventTypeChangeMode API_AVAILABLE(macos(10.15)) = 38,
};

typedef NS_ENUM(unsigned long long, NSEventMask) { /* masks for the types of events */
	NSEventMaskLeftMouseDown = 1ULL << NSEventTypeLeftMouseDown,
	NSEventMaskLeftMouseUp = 1ULL << NSEventTypeLeftMouseUp,
	NSEventMaskRightMouseDown = 1ULL << NSEventTypeRightMouseDown,
	NSEventMaskRightMouseUp = 1ULL << NSEventTypeRightMouseUp,
	NSEventMaskMouseMoved = 1ULL << NSEventTypeMouseMoved,
	NSEventMaskLeftMouseDragged = 1ULL << NSEventTypeLeftMouseDragged,
	NSEventMaskRightMouseDragged = 1ULL << NSEventTypeRightMouseDragged,
	NSEventMaskMouseEntered = 1ULL << NSEventTypeMouseEntered,
	NSEventMaskMouseExited = 1ULL << NSEventTypeMouseExited,
	NSEventMaskKeyDown = 1ULL << NSEventTypeKeyDown,
	NSEventMaskKeyUp = 1ULL << NSEventTypeKeyUp,
	NSEventMaskFlagsChanged = 1ULL << NSEventTypeFlagsChanged,
	NSEventMaskAppKitDefined = 1ULL << NSEventTypeAppKitDefined,
	NSEventMaskSystemDefined = 1ULL << NSEventTypeSystemDefined,
	NSEventMaskApplicationDefined = 1ULL << NSEventTypeApplicationDefined,
	NSEventMaskPeriodic = 1ULL << NSEventTypePeriodic,
	NSEventMaskCursorUpdate = 1ULL << NSEventTypeCursorUpdate,
	NSEventMaskScrollWheel = 1ULL << NSEventTypeScrollWheel,
	NSEventMaskTabletPoint = 1ULL << NSEventTypeTabletPoint,
	NSEventMaskTabletProximity = 1ULL << NSEventTypeTabletProximity,
	NSEventMaskOtherMouseDown = 1ULL << NSEventTypeOtherMouseDown,
	NSEventMaskOtherMouseUp = 1ULL << NSEventTypeOtherMouseUp,
	NSEventMaskOtherMouseDragged = 1ULL << NSEventTypeOtherMouseDragged,
};

/* The following event masks are available on some hardware on 10.5.2 and later */
#define NSEventMaskGesture API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeGesture)
#define NSEventMaskMagnify API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeMagnify)
#define NSEventMaskSwipe API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeSwipe)
#define NSEventMaskRotate API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeRotate)
#define NSEventMaskBeginGesture API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeBeginGesture)
#define NSEventMaskEndGesture API_AVAILABLE(macos(10.5))(1ULL << NSEventTypeEndGesture)

/* Note: You can only use these event masks on 64 bit. In other words, you cannot setup a local, nor global, event monitor for these event
 * types on 32 bit. Also, you cannot search the event queue for them (nextEventMatchingMask:...) on 32 bit. */
#define NSEventMaskSmartMagnify API_AVAILABLE(macos(10.8))(1ULL << NSEventTypeSmartMagnify)
#define NSEventMaskPressure API_AVAILABLE(macos(10.10.3))(1ULL << NSEventTypePressure)
#define NSEventMaskDirectTouch API_AVAILABLE(macos(10.12.2))(1ULL << NSEventTypeDirectTouch)
#define NSEventMaskChangeMode API_AVAILABLE(macos(10.15))(1ULL << NSEventTypeChangeMode)
#define NSEventMaskAny NSUIntegerMax

char *ns_strcat(register char *s, register const char *append)
{
	char *save = s;

	for (; *s; ++s)
		;
	while ((*s++ = *append++))
		;
	return save;
}

const char *NSEventModifierFlagsToChar(NSEventModifierFlags modifierFlags)
{
	static char result[100];
	result[0] = '\0';

	if ((modifierFlags & NSEventModifierFlagCapsLock) == NSEventModifierFlagCapsLock)
		ns_strcat(result, "CapsLock, ");
	if ((modifierFlags & NSEventModifierFlagShift) == NSEventModifierFlagShift)
		ns_strcat(result, "NShift, ");
	if ((modifierFlags & NSEventModifierFlagControl) == NSEventModifierFlagControl)
		ns_strcat(result, "Control, ");
	if ((modifierFlags & NSEventModifierFlagOption) == NSEventModifierFlagOption)
		ns_strcat(result, "Option, ");
	if ((modifierFlags & NSEventModifierFlagCommand) == NSEventModifierFlagCommand)
		ns_strcat(result, "Command, ");
	if ((modifierFlags & NSEventModifierFlagNumericPad) == NSEventModifierFlagNumericPad)
		ns_strcat(result, "NumericPad, ");
	if ((modifierFlags & NSEventModifierFlagHelp) == NSEventModifierFlagHelp)
		ns_strcat(result, "Help, ");
	if ((modifierFlags & NSEventModifierFlagFunction) == NSEventModifierFlagFunction)
		ns_strcat(result, "Function, ");

	return result;
}

const char *NSEventTypeToChar(NSEventType eventType)
{
	switch (eventType)
	{
	case NSEventTypeLeftMouseDown:
		return "LeftMouseDown";
	case NSEventTypeLeftMouseUp:
		return "LeftMouseUp";
	case NSEventTypeRightMouseDown:
		return "RightMouseDown";
	case NSEventTypeRightMouseUp:
		return "RightMouseUp";
	case NSEventTypeMouseMoved:
		return "MouseMoved";
	case NSEventTypeLeftMouseDragged:
		return "LeftMouseDragged";
	case NSEventTypeRightMouseDragged:
		return "RightMouseDragged";
	case NSEventTypeMouseEntered:
		return "MouseEntered";
	case NSEventTypeMouseExited:
		return "MouseExited";
	case NSEventTypeKeyDown:
		return "KeyDown";
	case NSEventTypeKeyUp:
		return "KeyUp";
	case NSEventTypeFlagsChanged:
		return "FlagsChanged";
	case NSEventTypeAppKitDefined:
		return "AppKitDefined";
	case NSEventTypeSystemDefined:
		return "SystemDefined";
	case NSEventTypeApplicationDefined:
		return "ApplicationDefined";
	case NSEventTypePeriodic:
		return "Periodic";
	case NSEventTypeCursorUpdate:
		return "CursorUpdate";
	case NSEventTypeScrollWheel:
		return "ScrollWheel";
	case NSEventTypeTabletPoint:
		return "TabletPoint";
	case NSEventTypeTabletProximity:
		return "TabletProximity";
	case NSEventTypeOtherMouseDown:
		return "OtherMouseDown";
	case NSEventTypeOtherMouseUp:
		return "OtherMouseUp";
	case NSEventTypeOtherMouseDragged:
		return "OtherMouseDragged";
	default:
		return "N/A";
	}
}

int fck_macos_mod_flag_check(NSEventModifierFlags flags, NSEventModifierFlags target)
{
	return (flags & target) == target;
}

void fck_macos_keyboard_event(fck_event_key *event, double timestamp, fck_keyboard_event_type type, NSUShort keycode,
                              NSEventModifierFlags mod, fck_event_unicode unicode)
{
	// Common
	event->common.size = sizeof(*event);
	event->common.timestamp = (fckc_u64)(timestamp * 1000); // TODO
	event->common.type = FCK_EVENT_TYPE_DEVICE;

	event->device_type = FCK_INPUT_DEVICE_TYPE_KEYBOARD;
	event->unicode = unicode;

	event->type = type;
	// TODO: FUCKING CHRIST, TRANSLATE THEM TO FCK! LOL
	event->mod = mod;
	event->pkey = keycode < fck_arraysize(fck_macos_keys) ? fck_macos_keys[keycode] : FCK_PKEY_UNKNOWN;
	// event->vkey = vkey; // No virtual keys for now!
}

void fck_macos_mouse_event(fck_event_mouse *event, double timestamp, fck_mouse_event_type type, int button_number, int is_down,
                           fckc_u32 clicks, fckc_f32 x, fckc_f32 y, fckc_f32 dx, fckc_f32 dy)
{
	switch (button_number)
	{
	case 0:
		type = FCK_MOUSE_EVENT_TYPE_BUTTON_LEFT;
		break;
	case 1:
		type = FCK_MOUSE_EVENT_TYPE_BUTTON_RIGHT;
		break;
	case 2:
		type = FCK_MOUSE_EVENT_TYPE_BUTTON_MIDDLE;
		break;
	case 3:
		type = FCK_MOUSE_EVENT_TYPE_BUTTON_4;
		break;
	case 4:
		type = FCK_MOUSE_EVENT_TYPE_BUTTON_5;
		break;
	default:
		break;
	}

	// Common
	event->common.size = sizeof(*event);
	event->common.timestamp = (fckc_u64)(timestamp * 1000); // TODO
	event->common.type = FCK_EVENT_TYPE_DEVICE;

	event->device_type = FCK_INPUT_DEVICE_TYPE_MOUSE;
	event->type = type;
	event->x = x;
	event->y = y;
	event->clicks = clicks; // ???
	event->is_down = is_down;
	event->dx = dx;
	event->dy = dy;
}

void fck_macos_unicode_create(const char *chars, fck_event_unicode *unicode)
{
	int len = strlen(chars);

	memset(unicode, 0, sizeof(*unicode));
	switch (len)
	{
	case 4:
		unicode->s.u3 = chars[3];
	case 3:
		unicode->s.u2 = chars[2];
	case 2:
		unicode->s.u1 = chars[1];
	case 1:
		unicode->s.u0 = chars[0];
	default:
		break;
	}
}

typedef struct fck_event_queue
{
	fckc_size_t head;
	fckc_size_t tail;
	fck_event events[1];
} fck_event_queue;

typedef struct fck_event_spsc
{
	// Producer 				| 	Consumer

	// (E) -> I -> M - > I -> S	| S -> I -> E -> I -> M
	fck_event_queue *consume;

	// M | M
	fck_event_queue *buffers[2];

	// E | S
	fckc_size_t active;

	// S | S
	kll_allocator *allocator;
	fckc_size_t capacity;
} fck_event_spsc;

static fckc_size_t fck_macos_align(fckc_size_t offset, fckc_size_t align)
{
	return (offset + align - 1) & ~(align - 1);
}

#define fck_macos_pointer_offset(type, ptr, offset) ((type *)(((fckc_u8 *)(ptr)) + (offset)))

fck_event_spsc *fck_event_spsc_create(kll_allocator *allocator, fckc_size_t capacity)
{
	fck_event_spsc *spsc;
	fckc_size_t spsc_size = sizeof(*spsc);
	fckc_size_t single_queue_size = offsetof(fck_event_queue, events[capacity]);
	fckc_size_t double_queue_size = single_queue_size * 2;

	spsc = (fck_event_spsc *)kll_malloc(allocator, spsc_size + double_queue_size);
	memset(spsc, 0, spsc_size + double_queue_size);

	spsc->allocator = allocator;
	spsc->active = 0;
	spsc->capacity = capacity;
	spsc->consume = NULL;
	spsc->buffers[0] = fck_macos_pointer_offset(fck_event_queue, spsc, spsc_size);
	spsc->buffers[1] = fck_macos_pointer_offset(fck_event_queue, spsc, spsc_size + single_queue_size);
	return spsc;
}

void fck_event_spsc_destroy(fck_event_spsc *spsc)
{
	kll_free(spsc->allocator, spsc);
}

int fck_event_spsc_enqueue(fck_event_spsc *ringbuffer, fck_event *event)
{
	fck_event_queue *active = ringbuffer->buffers[ringbuffer->active];
	fckc_size_t next = (active->tail + 1) % ringbuffer->capacity;
	active->events[active->tail] = *event;
	active->tail = next;
	if (next == active->head)
	{
		// When we run out of space, we are going to start overwriting
		// to achieve this, we need to push the front forward
		active->tail = (next + 1) % ringbuffer->capacity;
	}
	return 1;
}

int fck_event_spsc_submit(fck_event_spsc *ringbuffer)
{
	fck_event_queue *consumable = (fck_event_queue *)fckc_pointer_load((void **)&ringbuffer->consume);
	fckc_acquire();

	if (consumable != NULL)
	{
		return 0;
	}
	fck_event_queue *active = ringbuffer->buffers[ringbuffer->active];
	if (active->head == active->tail)
	{
		return 0;
	}

	ringbuffer->active = (ringbuffer->active + 1) % 2;
	fckc_release();
	fckc_pointer_store((void **)&ringbuffer->consume, active);
	return 1;
}

fckc_size_t fck_event_spsc_dequeue(fck_event_spsc *ringbuffer, fck_event *events, fckc_size_t capacity)
{
	if (capacity == 0)
	{
		return 0;
	}

	fck_event_queue *consumable = (fck_event_queue *)fckc_pointer_load((void **)&ringbuffer->consume);
	fckc_acquire();

	if (consumable == NULL)
	{
		return 0;
	}
	// Invalid permutation, if consumable != NULL, front and tail shall not be equal
	fck_assert(consumable->head != consumable->tail);

	{
		fckc_size_t used_capacity = 0;
		if (consumable->head > consumable->tail)
		{
			fck_event *src = &consumable->events[consumable->head];
			const fckc_size_t count_until_capacity = ringbuffer->capacity - consumable->head;
			if (capacity < count_until_capacity)
			{
				consumable->head = consumable->head + capacity;
				memcpy(events, src, sizeof(*events) * capacity);
				return capacity;
			}

			memcpy(events, src, sizeof(*events) * count_until_capacity);
			used_capacity = used_capacity + count_until_capacity;
			consumable->head = 0;
		}

		fck_assert(consumable->head <= consumable->tail);
		{
			fck_event *src = &consumable->events[consumable->head];
			fckc_size_t count = consumable->tail - consumable->head;
			fckc_size_t actual_capacity = capacity - used_capacity;
			fckc_size_t possible_count = fck_min(count, actual_capacity);
			memcpy(events, src, sizeof(*events) * possible_count);
			fck_assert(events->type != FCK_EVENT_TYPE_NONE);
			consumable->head = consumable->head + possible_count;
			fck_assert(consumable->head <= consumable->tail);
			if (consumable->head == consumable->tail)
			{
				ringbuffer->consume->head = 0;
				ringbuffer->consume->tail = 0;
				fckc_release();
				fckc_pointer_store((void **)&ringbuffer->consume, NULL);
			}
			return used_capacity + possible_count;
		}
	}
}

void fck_event_log(fck_event *event)
{
	printf("{\n\tCommon: [type: %s, size: %u, timestamp: %llu]\n", fck_event_type_to_string(event->common.type), event->common.size,
	       event->common.timestamp);

	switch (event->type)
	{
	case FCK_EVENT_TYPE_NONE:
		break;
	case FCK_EVENT_TYPE_DEVICE:
		printf("\tDevice: [type: %s]\n", fck_input_device_type_to_string(event->device.device_type));

		switch (event->device.device_type)
		{
		case FCK_INPUT_DEVICE_TYPE_NONE:
		case FCK_INPUT_DEVICE_TYPE_KEYBOARD:
			printf("\tKeyboard: [type: %s, pkey: %s, unicode: %s]\n}\n", fck_keyboard_event_type_to_string(event->key.type),
			       fck_pkey_tostring(event->key.pkey), (const char *)event->key.unicode.u);
			break;
		case FCK_INPUT_DEVICE_TYPE_MOUSE:
			printf("\tMouse: [type: %s, down: %u, clicks: %u, x: %f, y: %f, dx: %f, dy: %f]\n}\n",
			       fck_mouse_event_type_to_string(event->mouse.type), event->mouse.is_down, event->mouse.clicks, event->mouse.x,
			       event->mouse.y, event->mouse.dx, event->mouse.dy);
			break;
		}
	case FCK_EVENT_TYPE_TEXT:
		break;
	}
}

void fck_macos_poll_events(fck_event_spsc *spsc)
{
	id pool;

	NSApplication *ns_app;
	NSEvent *e;
	NSString *str;
	const char *chars;

	NSEventType type;
	NSPoint point;
	NSEventModifierFlags modifier_flags;
	NSTimeInterval timestamp;
	NSInteger click_count, button, is_down;
	NSUShort keycode;
	CGFloat dx, dy;

	fck_event event;
	fck_event_unicode unicode;
	fck_keyboard_event_type key_type;

	ns_app = objc_msgSend_id((id)objc_getClass("NSApplication"), sel_registerName("sharedApplication"));
	pool = objc_msgSend_id(NSAlloc(objc_getClass("NSAutoreleasePool")), sel_registerName("init"));

	while (true)
	{
		// This is a very sad one
		e = (NSEvent *)((id (*)(id, SEL, NSEventMask, void *, NSString *, bool))objc_msgSend)( //
			ns_app,                                                                            //
			sel_registerName("nextEventMatchingMask:untilDate:inMode:dequeue:"),               //
			ULONG_MAX,                                                                         //
			NULL,                                                                              //
			((id (*)(id, SEL, const char *))objc_msgSend)((id)objc_getClass("NSString"), sel_registerName("stringWithUTF8String:"),
		                                                  "kCFRunLoopDefaultMode"),
			true);

		type = (NSEventType)objc_msgSend_uint(e, sel_registerName("type"));
		if (type == 0 || type >= NSEventTypeGesture) // Let's skip new macos events
		{
			break;
		}

		point = ((NSPoint (*)(id, SEL))objc_msgSend)(e, sel_registerName("locationInWindow"));

		// static unsigned int previous_modifier_flags = 0;
		modifier_flags = objc_msgSend_uint(e, sel_registerName("modifierFlags"));
		timestamp = objc_msgSend_double(e, sel_registerName("timestamp"));

		event.type = FCK_EVENT_TYPE_NONE;

		switch (type)
		{
		case NSEventTypeMouseMoved:
			dx = objc_msgSend_cgfloat(e, sel_registerName("deltaX"));
			dy = objc_msgSend_cgfloat(e, sel_registerName("deltaY"));
			fck_macos_mouse_event(&event.mouse, timestamp, FCK_MOUSE_EVENT_TYPE_POSITION, -1, 0, 0, point.x, point.y, dx, dy);
			break;

		case NSEventTypeLeftMouseDown:
		case NSEventTypeRightMouseDown:
		case NSEventTypeOtherMouseDown:
			is_down = 1;
		case NSEventTypeLeftMouseUp:
		case NSEventTypeRightMouseUp:
		case NSEventTypeOtherMouseUp:
			click_count = objc_msgSend_int(e, sel_registerName("clickCount"));
			button = objc_msgSend_int(e, sel_registerName("buttonNumber"));
			fck_macos_mouse_event(&event.mouse, timestamp, FCK_MOUSE_EVENT_TYPE_BUTTON_NONE, button, is_down, click_count, point.x, point.y,
			                      0, 0);
			break;

		case NSEventTypeScrollWheel:
			dx = objc_msgSend_cgfloat(e, sel_registerName("scrollingDeltaX"));
			dy = objc_msgSend_cgfloat(e, sel_registerName("scrollingDeltaY"));
			if (!objc_msgSend_bool(e, sel_registerName("hasPreciseScrollingDeltas")))
			{
				// ... Idk - There is a possibility to give it a hardcoded higher value? but that is shit...
			}
			fck_macos_mouse_event(&event.mouse, timestamp, FCK_MOUSE_EVENT_TYPE_WHEEL, -1, 0, 0, point.x, point.y, dx, dy);
			break;

		case NSEventTypeKeyDown: {
			keycode = objc_msgSend_ushort(e, sel_registerName("keyCode"));
			str = objc_msgSend_string(e, sel_registerName("charactersIgnoringModifiers"));
			chars = (const char *)objc_msgSend_address(str, sel_registerName("UTF8String"));

			fck_macos_unicode_create(chars, &unicode);
			fck_macos_keyboard_event(&event.key, timestamp, FCK_KEYBOARD_EVENT_TYPE_DOWN, keycode, modifier_flags, unicode);
			break;
		}
		case NSEventTypeKeyUp: {
			keycode = objc_msgSend_ushort(e, sel_registerName("keyCode"));
			str = objc_msgSend_string(e, sel_registerName("charactersIgnoringModifiers"));
			chars = (const char *)objc_msgSend_address(str, sel_registerName("UTF8String"));

			fck_macos_unicode_create(chars, &unicode);
			fck_macos_keyboard_event(&event.key, timestamp, FCK_KEYBOARD_EVENT_TYPE_UP, keycode, modifier_flags, unicode);
			break;
		}
		case NSEventTypeFlagsChanged: {
			keycode = objc_msgSend_ushort(e, sel_registerName("keyCode"));
			is_down = (modifier_flags & (1 << fck_pkey_to_modifier[keycode])) == 1 << fck_pkey_to_modifier[keycode];
			key_type = is_down ? FCK_KEYBOARD_EVENT_TYPE_DOWN : FCK_KEYBOARD_EVENT_TYPE_UP;
			unicode = (fck_event_unicode){.s.u0 = 0, .s.u1 = 0, .s.u2 = 0, .s.u3 = 0};
			fck_macos_keyboard_event(&event.key, timestamp, key_type, keycode, modifier_flags, unicode);
			break;
		}
		default:
			break;
		}

		if (event.type != FCK_EVENT_TYPE_NONE)
		{
			if (!fck_event_spsc_enqueue(spsc, &event))
			{
				break;
			}
		}

		// Needed? Should I really do it now?
		objc_msgSend_void_id(ns_app, sel_registerName("sendEvent:"), e);
	}

	fck_event_spsc_submit(spsc);

	((void (*)(id, SEL))objc_msgSend)(ns_app, sel_registerName("updateWindows"));

	NSRelease(pool);
}

NSSize windowResize(void *self, SEL sel, NSSize frameSize)
{
	NSWindow *win = NULL;
	object_getInstanceVariable(self, name(NSWindow), (void **)&win);
	if (win == NULL)
		return frameSize;

	printf("window resized to %f %f\n", frameSize.width, frameSize.height);
	return frameSize;
}

// This is optional, but we’ll need it so the view accepts first responder
BOOL myAcceptsFirstResponder(void *self, SEL _cmd)
{
	return YES;
}

id fck_macos_on_event(void *context, NSEvent *event)
{
	return event;
}

NSWindow *NSWindow_create(NSRect rect)
{
	// I think this one adds to some other delegate...
	// class_addMethod(objc_getClass(name(NSObject)), sel_registerName("windowShouldClose:"), (IMP)windowShouldClose, NULL);
	NSApplication *app = objc_msgSend_id((id)objc_getClass(name(NSApplication)), sel_registerName("sharedApplication"));
	objc_msgSend_void_int(app, sel_registerName("setActivationPolicy:"), (int)NSApplicationActivationPolicyRegular);

	objc_msgSend_void_bool(app, sel_registerName("activateIgnoringOtherApps:"), true);
	objc_msgSend_void(app, sel_registerName("finishLaunching"));

	NSBackingStoreType macArgs =
		NSWindowStyleMaskClosable | NSWindowStyleMaskMiniaturizable | NSWindowStyleMaskTitled | NSWindowStyleMaskResizable;

	SEL initWithContentRect = sel_registerName("initWithContentRect:styleMask:backing:defer:");
	Class windowClass = objc_getClass(name(NSWindow));
	NSWindow *window = NSAlloc(windowClass);

	// frame
	typedef id (*window_init_with_content_rect)(id, SEL, NSRect, NSWindowStyleMask, NSBackingStoreType, bool);

	window = ((window_init_with_content_rect)objc_msgSend)(window, initWithContentRect, rect, macArgs, NSBackingStoreBuffered, NO);

	Class delegateClass = objc_allocateClassPair(objc_getClass(name(NSObject)), "WindowDelegate", 0);
	class_addIvar(delegateClass, name(NSWindow), sizeof(NSWindow *), rint(log2(sizeof(NSWindow *))), "L");

	SEL windowWillResize = sel_registerName("windowWillResize:toSize:");
	BOOL addResize = class_addMethod(delegateClass, windowWillResize, (IMP)windowResize, "{NSSize=ff}@:{NSSize=ff}");
	BOOL addFirstResponder = class_addMethod(delegateClass, sel_registerName("acceptsFirstResponder"), (IMP)myAcceptsFirstResponder, "v@:");

	id delegate = objc_msgSend_id(NSAlloc(delegateClass), sel_registerName("init"));

	object_setInstanceVariable(delegate, name(NSWindow), window);

	objc_msgSend_void_id(window, sel_registerName("setDelegate:"), delegate);
	((id (*)(id, SEL, SEL))objc_msgSend)(window, sel_registerName("makeKeyAndOrderFront:"), NULL);
	objc_msgSend_void_bool(window, sel_registerName("setIsVisible:"), true);

	// Block objects for callback:
	// static struct Block_literal
	//{
	//	void *isa;
	//	int flags;
	//	int reserved;
	//	void *invoke;
	//	struct Block_descriptor *descriptor;
	//} block_literal = {&_NSConcreteStackBlock,
	//                   0x40000000, // Flags: BLOCK_HAS_SIGNATURE | BLOCK_HAS_COPY_DISPOSE
	//                   0, (void *)fck_macos_on_event, NULL};
	// static struct Block_descriptor
	//{
	//	unsigned long reserved;
	//	unsigned long size;
	//	void (*copy_helper)(void *, const void *);
	//	void (*dispose_helper)(const void *);
	//	const char *signature;
	//} block_descriptor = {0, sizeof(struct Block_literal), NULL, NULL, "v@:@"};
	// const char *real_signature = "@?"
	//							 "@"
	//							 "24@0:8@16";
	// block_descriptor.signature = "@?32@0:8@16";
	// block_literal.descriptor = &block_descriptor;
	// struct Block_literal *block_obj = Block_copy(&block_literal);
	//
	// objc_msgSend_id_eventmask_block((id)objc_getClass(name(NSEvent)), sel_registerName("addLocalMonitorForEventsMatchingMask:handler:"),
	//                                NSEventMaskFlagsChanged, block_obj);

	// objc_msgSend_id((id)objc_getClass(name(NSEvent)), sel_registerName("addLocalMonitorForEvents"));
	//  objc_msgSend_void_id(window, sel_registerName("setContentView:"), view);

	return window;
};

static fck_window fck_window_api_macos_create(const char *name, int w, int h)
{
	NSRect rect = (NSRect){.origin = (NSPoint){0, 0}, .size = (NSSize){.width = w, .height = h}};
	fck_window window = (fck_window){.handle = (void *)(NSWindow_create(rect))};
	return window;
}

void fck_window_api_macos_destroy(fck_window window)
{
	objc_msgSend_void(window.handle, sel_registerName("close"));
	NSRelease(window.handle);
}

int fck_window_api_macos_is_valid(fck_window window)
{
	return window.handle != NULL;
}

int fck_window_api_macos_resize(fck_window window, int width, int height)
{
	// NSView *view = objc_msgSend_id(window.handle, sel_registerName("contentView"));
	NSRect rect = objc_msgSend_rect(window.handle, sel_registerName("frame"));
	rect.size.width = width;
	rect.size.height = height;
	objc_msgSend_void_rect_bool(window.handle, sel_registerName("setFrame:display:"), rect, TRUE);
	return 0;
}

int fck_window_api_macos_text_input_start(fck_window window)
{
	return 0;
}

int fck_window_api_macos_text_input_stop(fck_window window)
{
	return 0;
}

int fck_window_api_macos_size(fck_window window, int *width, int *height)
{
	// NSView *view = objc_msgSend_id(window.handle, sel_registerName("contentView"));
	NSRect rect = objc_msgSend_rect(window.handle, sel_registerName("frame"));
	*width = rect.size.width;
	*height = rect.size.height;
	return 1; // I think it can never fail? Let's fix it as it happens
}

int fck_window_api_macos_position(fck_window window, int *x, int *y)
{
	// NSView *view = objc_msgSend_id(window.handle, sel_registerName("contentView"));
	NSRect rect = objc_msgSend_rect(window.handle, sel_registerName("frame"));
	*x = rect.origin.x;
	*y = rect.origin.y;
	return 1; // I think it can never fail? Let's fix it as it happens
}

static fck_window_api window_macos_api = {
	.create = fck_window_api_macos_create,
	.destroy = fck_window_api_macos_destroy,
	.is_valid = fck_window_api_macos_is_valid,
	.resize = fck_window_api_macos_resize,
	.text_input_start = fck_window_api_macos_text_input_start,
	.text_input_stop = fck_window_api_macos_text_input_stop,
	.size = fck_window_api_macos_size,
	.position = fck_window_api_macos_position,
};

#include <string.h>

static fck_char_api char_api = {
	.isdigit = isdigit,
	.isspace = isspace,
	.isgraph = isgraph,
	.isprint = isprint,
	.iscntrl = iscntrl,
};

static fck_unsafe_string_api unsafe_string_api = {
	.cmp = strcmp,
	.dup = strdup,
	.len = strlen,
};

char *fck_string_find_graphical(char *str)
{
	if (str == NULL || *str == '\0')
	{
		return NULL;
	}
	while (*str != '\0')
	{
		if (!isgraph(*str))
		{
			str = str + 1;
			continue;
		}
		return str;
	}
	return NULL;
}

char *fck_string_find_printable(char *str)
{
	if (str == NULL || *str == '\0')
	{
		return NULL;
	}
	while (*str != '\0')
	{
		if (!isprint(*str))
		{
			str = str + 1;
			continue;
		}
		return str;
	}
	return NULL;
}

char *fck_string_find_control(char *str)
{
	if (str == NULL || *str == '\0')
	{
		return NULL;
	}
	while (*str != '\0')
	{
		if (!iscntrl(*str))
		{
			str = str + 1;
			continue;
		}
		return str;
	}
	return NULL;
}

char *fck_string_find_string(char *str, const char *other)
{
	return strstr(str, other);
}

char *fck_string_find_char(char *str, int ch)
{
	return strchr(str, ch);
}

static fck_string_find_api string_find_api = {
	.string = fck_string_find_string,
	.graphical = fck_string_find_graphical,
	.printable = fck_string_find_printable,
	.chr = fck_string_find_char,
	.control = fck_string_find_control,
};

static fck_string_api string_api = {
	.unsafe = &unsafe_string_api, //
	.find = &string_find_api,     //
	.cmp = strncmp,               //
	.dup = strndup,               //
	.len = strnlen,               //
	.toll = strtoll,              //
	.toull = strtoull,            //
	.tod = strtod,                //
};

static fck_memory_api memory_api = {
	.cpy = memcpy,
	.set = memset,
};

int fck_io_api_macos_log(const char *format, ...)
{
	va_list args;
	va_start(args, format);
	int result = vprintf(format, args);
	va_end(args);

	result = putchar('\n') || result;
	return result;
}

static fck_io_api io_api = {
	.format = snprintf,
	.log = fck_io_api_macos_log,
};

static int fck_shared_object_is_valid(fck_shared_object so)
{
	return so.handle != NULL;
}

static fck_shared_object fck_shared_object_load(const char *path)
{
	char real_path[256];

	// Portable code stinks
	// const char *path_delim_backslash = SDL_strrchr(path, '\\');
	// const char *path_delim_slash = SDL_strrchr(path, '/');
	// const char *path_delim = path_delim_backslash > path_delim_slash ? path_delim_backslash : path_delim_slash;
	const char *extension_dot = strchr(path, '.');

	int extension_found = extension_dot != NULL; //> path_delim;
	if (!extension_found)
	{
		int result = snprintf(real_path, sizeof(real_path), "%s.dylib", path);
		if (result < 0)
		{
			return (fck_shared_object){.handle = NULL};
		}
		path = real_path;
	}
	void *handle = dlopen(path, RTLD_NOW | RTLD_LOCAL);
	const char *loaderror = dlerror();
	if (!handle)
	{
		fck_io_api_macos_log("Failed loading %s: %s", path, loaderror);
		return (fck_shared_object){.handle = NULL};
	}
	return (fck_shared_object){.handle = (void *)handle};
}

static void fck_shared_object_unload(fck_shared_object so)
{
	if (so.handle)
	{
		dlclose(so.handle);
	}
}

static void *fck_shared_object_symbol(fck_shared_object so, const char *name)
{
	void *symbol = dlsym(so.handle, name);
	if (!symbol)
	{
		char small_buffer[128];
		char *underscore_name;
		// prepend an underscore for platforms that need that.
		size_t len = strlen(name) + 1;
		int is_large = len > 128;
		if (is_large)
		{
			underscore_name = malloc(len + 1);
		}
		else
		{
			underscore_name = small_buffer;
		}

		underscore_name[0] = '_';
		memcpy(&underscore_name[1], name, len);
		symbol = dlsym(so.handle, underscore_name);

		if (is_large)
		{
			free(underscore_name);
		}

		if (!symbol)
		{
			fck_io_api_macos_log("Failed loading %s: %s", name, (const char *)dlerror());
			return NULL;
		}
	}
	return symbol;
}

static fck_shared_object_api so_api = {
	.load = fck_shared_object_load,
	.symbol = fck_shared_object_symbol,
	.unload = fck_shared_object_unload,
	.is_valid = fck_shared_object_is_valid,
};

int fck_clipboard_api_set(const char *text)
{
	return (int)0;
}

int fck_clipboard_api_has(void)
{
	return (int)0;
}

fck_clipboard fck_clipboard_api_receive(void)
{
	return (fck_clipboard){.text = NULL};
}

void fck_clipboard_api_close(fck_clipboard clipboard)
{
	// TODO; Do clipboard stuff
}

int fck_clipboard_api_is_valid(fck_clipboard clipboard)
{
	if (clipboard.text == NULL)
	{
		return 0;
	}
	return strcmp("", clipboard.text);
}

fckc_u64 fck_chrono_api_ms()
{
	// TODO:
	return 0;
}

fck_file fck_filesystem_open(const char *path, const char *mode)
{
	FILE *file = fopen(path, mode);
	return (fck_file){.handle = (void *)file};
}

void fck_filesystem_close(fck_file file)
{
	(void)fclose(((FILE *)file.handle));
}

int fck_filesystem_is_valid(fck_file file)
{
	return file.handle != NULL;
}

fckc_i64 fck_filesystem_seek(fck_file file, fckc_i64 offset, fckc_u32 seek_mode)
{
	int whence;
	switch ((fck_stream_seek_mode)seek_mode)
	{
	case FCK_STREAM_CUR:
		whence = SEEK_CUR;
		break;
	case FCK_STREAM_END:
		whence = SEEK_END;
		break;
	case FCK_STREAM_SET:
		whence = SEEK_SET;
		break;
	default:
		return -1;
	}
	return (fckc_i64)fseek((FILE *)file.handle, offset, whence);
}

fckc_i64 fck_filesystem_size(fck_file file)
{
	long prev = ftell((FILE *)file.handle);
	if (fseek((FILE *)file.handle, 0, SEEK_END))
	{
		fck_io_api_macos_log("%s", "fseek failed to get file size");
		return -1;
	}
	// We return the sizze, -1 or the error gets propagated
	long size = ftell((FILE *)file.handle);
	(void)fseek((FILE *)file.handle, prev, SEEK_SET);
	return size;
}

fckc_size_t fck_filesystem_read(fck_file file, void *ptr, fckc_size_t size)
{
	return fread(ptr, 1, size, (FILE *)file.handle);
}

fckc_size_t fck_filesystem_write(fck_file file, const void *ptr, fckc_size_t size)
{
	return fwrite(ptr, 1, size, (FILE *)file.handle);
}

fckc_i64 fck_filesystem_flush(fck_file file)
{
	return (fckc_i64)fflush((FILE *)file.handle);
}

static fck_filesystem_api file_system_api = {
	.open = fck_filesystem_open,
	.close = fck_filesystem_close,
	.is_valid = fck_filesystem_is_valid,
	.size = fck_filesystem_size,
	.seek = fck_filesystem_seek,
	.read = fck_filesystem_read,
	.write = fck_filesystem_write,
	.flush = fck_filesystem_flush,
};

static fck_clipboard_api clipboard_api = {
	.set = fck_clipboard_api_set,
	.has = fck_clipboard_api_has,
	.receive = fck_clipboard_api_receive,
	.close = fck_clipboard_api_close,
	.is_valid = fck_clipboard_api_is_valid,
};

static fck_chrono_api chrono_api = {
	.ms = fck_chrono_api_ms,
};

fck_event_channel fck_event_channel_api_create(kll_allocator *allocator, fckc_size_t capacity)
{
	fck_event_spsc *spsc = fck_event_spsc_create(allocator, capacity);
	return (fck_event_channel){.handle = (void *)spsc};
}

void fck_event_channel_api_destroy(fck_event_channel channel)
{
	// TODO: It is useful to assert on "only destroy if empty"
	fck_event_spsc *spsc = (fck_event_spsc *)channel.handle;
	fck_event_spsc_destroy(spsc);
}

void fck_event_channel_api_pump(fck_event_channel channel)
{
	fck_event_spsc *spsc = (fck_event_spsc *)channel.handle;
	fck_macos_poll_events(spsc);
}

fckc_size_t fck_event_channel_api_poll(fck_event_channel channel, fck_event *events, fckc_size_t capacity)
{
	fck_event_spsc *spsc = (fck_event_spsc *)channel.handle;
	return fck_event_spsc_dequeue(spsc, events, capacity);
}

static fck_event_channel_api event_channel_api = {
	.create = fck_event_channel_api_create,
	.destroy = fck_event_channel_api_destroy,
	.pump = fck_event_channel_api_pump,
	.poll = fck_event_channel_api_poll,
};

static fck_os_api std_api = {
	.chr = &char_api,
	.str = &string_api,
	.mem = &memory_api,
	.io = &io_api,
	.so = &so_api,
	.win = &window_macos_api,
	.chrono = &chrono_api,
	.fs = &file_system_api,
	.event_channel = &event_channel_api,
};

fck_os_api *os = &std_api;
