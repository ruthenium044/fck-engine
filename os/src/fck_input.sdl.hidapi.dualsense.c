#include "fck_input.h"

// #include <SDL3/SDL_events.h>
// #include <SDL3/SDL_log.h>

#include <SDL3/SDL_hidapi.h>

#include "fck_os.h"
#include "fckc_apidef.h"
#include "fckc_assert.h"
#include "fckc_math.h"
#include "fck_gamepad_input.h"

#include <math.h>

// The maximum size of a USB packet for HID devices
#define fck_dualsense_usb_packet_length 64
#define fck_dualsense_bluetooth_extended_packet_length 78
#define fck_dualsense_largest_possible_packet_length 78

#define fck_vendor_sony 0x054C
#define fck_product_dualsense 0x0ce6
#define fck_product_dualsense_edge 0x0df2

#define fck_dualsense_discovery_rate 2000
#define fck_dualsense_timeout_rate 2000
#define fck_dualsense_input_report_usb_packet_size 64

#define fck_dualsense_input_report_usb_simple 0x01
#define fck_dualsense_input_report_bluetooth_simple 0x01
#define fck_dualsense_input_report_bluetooth_extended 0x31
#define fck_dualsense_input_report_bluetooth_extended_charging 0x32

#define fck_dualsense_feature_report_serial_number 9

#define fck_dualsense_load16(A, B) (fckc_i16)((fckc_u16)(A) | (((fckc_u16)(B)) << 8))
#define fck_dualsense_load32(A, B, C, D)                                                                                                   \
	((((fckc_u32)(A)) << 0) | (((fckc_u32)(B)) << 8) | (((fckc_u32)(C)) << 16) | (((fckc_u32)(D)) << 24))

/*
 * ============================================================================
 * DUALSENSE HID INPUT PROTOCOL MAP (USB / BLUETOOTH)
 * ============================================================================
 *
 * ----------------------------------------------------------------------------
 * REPORT ID: 0x01 (fck_dualsense_input_report_usb_simple)
 * Connection: USB Cable
 * Structure: 64-byte Raw HID Packet
 * ----------------------------------------------------------------------------
 * [0]      Report ID (0x01)
 * [1]      Left Stick X (0-255, 128 = Center)
 * [2]      Left Stick Y (0-255, 128 = Center)
 * [3]      Right Stick X (0-255, 128 = Center)
 * [4]      Right Stick Y (0-255, 128 = Center)
 * [5]      Left Trigger L2 (0-255)
 * [6]      Right Trigger R2 (0-255)
 * [7]      Sequence Number (0-63 Counter)
 * [8]      Buttons Cluster 1:
 *            - Bits 0-3: D-Pad (0:N, 1:NE, 2:E, 3:SE, 4:S, 5:SW, 6:W, 7:NW, 8:None)
 *            - Bit 4: Square
 *            - Bit 5: Cross
 *            - Bit 6: Circle
 *            - Bit 7: Triangle
 * [9]      Buttons Cluster 2:
 *            - Bit 0: L1
 *            - Bit 1: R1
 *            - Bit 2: L2 (Digital)
 *            - Bit 3: R2 (Digital)
 *            - Bit 4: Create (Share)
 *            - Bit 5: Options
 *            - Bit 6: L3
 *            - Bit 7: R3
 * [10]     Buttons Cluster 3:
 *            - Bit 0: PS Button
 *            - Bit 1: Touchpad Click
 *            - Bit 2: Mute
 *
 * ----------------------------------------------------------------------------
 * REPORT ID: 0x31 / 0x32 (fck_dualsense_input_report_bluetooth_extended)
 * Connection: Bluetooth (Wireless)
 * Structure: 78-byte Extended Packet
 * ----------------------------------------------------------------------------
 * [0]      Report ID (0x31 = BT / 0x32 = BT Charging)
 * [1]      Extended Header / Format Byte (Usually 0x01)
 * [2]      Left Stick X (0-255)
 * [3]      Left Stick Y (0-255)
 * [4]      Right Stick X (0-255)
 * [5]      Right Stick Y (0-255)
 * [6]      Left Trigger L2 (0-255)
 * [7]      Right Trigger R2 (0-255)
 * [8]      Sequence Number
 * [9]      Buttons Cluster 1:
 *            - Bits 0-3: D-Pad (0-7: Directions, 8: Neutral)
 *            - Bit 4: Square
 *            - Bit 5: Cross
 *            - Bit 6: Circle
 *            - Bit 7: Triangle
 * [10]     Buttons Cluster 2:
 *            - Bit 0: L1 | Bit 1: R1 | Bit 2: L2 (D) | Bit 3: R2 (D)
 *            - Bit 4: Create | Bit 5: Options | Bit 6: L3 | Bit 7: R3
 * [11]     Buttons Cluster 3:
 *            - Bit 0: PS Button | Bit 1: Touch Click | Bit 2: Mute
 * [12-15]  Adaptive Trigger Feedback Data
 * [16-21]  IMU: Gyroscope X/Y/Z (int16_t, Little Endian)
 * [22-27]  IMU: Accelerometer X/Y/Z (int16_t, Little Endian)
 * [33-41]  Touchpad Data (Contact 1 & 2 Coordinates + Finger State)
 * [74-77]  CRC32 Checksum (Bluetooth validation)
 */

/* Bitmask for Buttons Cluster 1 (Byte 8 USB / 9 BT) */
typedef struct fck_dualsense_buttons_1
{
	fckc_u8 dpad : 4; /* 0:N, 1:NE, 2:E, 3:SE, 4:S, 5:SW, 6:W, 7:NW, 8:None */
	fckc_u8 square : 1;
	fckc_u8 cross : 1;
	fckc_u8 circle : 1;
	fckc_u8 triangle : 1;
} fck_dualsense_buttons_1;

/* Bitmask for Buttons Cluster 2 (Byte 9 USB / 10 BT) */
typedef struct fck_dualsense_buttons_2
{
	fckc_u8 l1 : 1;
	fckc_u8 r1 : 1;
	fckc_u8 l2_digi : 1;
	fckc_u8 r2_digi : 1;
	fckc_u8 create : 1;
	fckc_u8 options : 1;
	fckc_u8 l3 : 1;
	fckc_u8 r3 : 1;
} fck_dualsense_buttons_2;

/* Bitmask for Buttons Cluster 3 (Byte 10 USB / 11 BT) */
typedef struct fck_dualsense_buttons_3
{
	fckc_u8 ps : 1;
	fckc_u8 touch : 1;
	fckc_u8 mute : 1;
	fckc_u8 reserved : 5;
} fck_dualsense_buttons_3;

/* USB Input Report (Report ID 0x01) - 64 Bytes */
typedef struct fck_dualsense_usb_report
{
	fckc_u8 report_id; /* Always 0x01 */
	fckc_u8 left_stick_x;
	fckc_u8 left_stick_y;
	fckc_u8 right_stick_x;
	fckc_u8 right_stick_y;
	fckc_u8 l2_analog;
	fckc_u8 r2_analog;
	fckc_u8 seq_number;

	fck_dualsense_buttons_1 buttons1;
	fck_dualsense_buttons_2 buttons2;
	fck_dualsense_buttons_3 buttons3;

	/* Remaining bytes for IMU, Touch, etc. */
	fckc_u8 padding[53];
} fck_dualsense_usb_report;

/* Bluetooth Input Report (Report ID 0x01) - 64 Bytes */
typedef struct fck_dualsense_bluetooth_report
{
	fckc_u8 report_id; /* Always 0x01 */
	fckc_u8 left_stick_x;
	fckc_u8 left_stick_y;
	fckc_u8 right_stick_x;
	fckc_u8 right_stick_y;
	fck_dualsense_buttons_1 buttons1;
	fck_dualsense_buttons_2 buttons2;
	fck_dualsense_buttons_3 buttons3;
	fckc_u8 l2_analog;
	fckc_u8 r2_analog;
} fck_dualsense_bluetooth_report;

/* BT Extended Input Report (Report ID 0x31/0x32) - 78 Bytes */
typedef struct fck_dualsense_bluetooth_extended_report
{
	fckc_u8 report_id; /* 0x31 or 0x32 */
	fckc_u8 format_id; /* The "Offset Shifter" byte (usually 0x01) */
	fckc_u8 left_stick_x;
	fckc_u8 left_stick_y;
	fckc_u8 right_stick_x;
	fckc_u8 right_stick_y;
	fckc_u8 l2_analog;
	fckc_u8 r2_analog;
	fckc_u8 seq_number;

	fck_dualsense_buttons_1 buttons1;
	fck_dualsense_buttons_2 buttons2;
	fck_dualsense_buttons_3 buttons3;

	/* Extended BT data */
	fckc_u8 reserved[4]; /* Trigger feedback / etc */
	fckc_u16 gyro[3];    /* X, Y, Z */
	fckc_u16 accel[3];   /* X, Y, Z */
	fckc_u8 sensor_padding[34];
	fckc_u32 crc32; /* Checksum at the end */
} fck_dualsense_bluetooth_extended_report;

#define fck_dualsense_input_report_usb_directional_pad_up_value 0
#define fck_dualsense_input_report_usb_directional_pad_up_right_value 1
#define fck_dualsense_input_report_usb_directional_pad_right_value 2
#define fck_dualsense_input_report_usb_directional_pad_down_right_value 3
#define fck_dualsense_input_report_usb_directional_pad_down_value 4
#define fck_dualsense_input_report_usb_directional_pad_down_left_value 5
#define fck_dualsense_input_report_usb_directional_pad_left_value 6
#define fck_dualsense_input_report_usb_directional_pad_up_left_value 7
#define fck_dualsense_input_report_usb_directional_pad_neutral_value 8

#define fck_dualsense_input_report_usb_square_bit 4
#define fck_dualsense_input_report_usb_cross_bit 5
#define fck_dualsense_input_report_usb_circle_bit 6
#define fck_dualsense_input_report_usb_triangle_bit 7

#define fck_dualsense_bittest(value, bit) ((value) & (1 << (bit)))

#define fck_input_dualsense_capacity 32
#define fck_input_dualsense_owner_unused 0LLU
#define fck_input_dualsense_owner_deleted ~0LLU

// https://github.com/nondebug/dualsense
// http://www.psdevwiki.com/ps4/DS4-USB
// http://www.psdevwiki.com/ps4/DS4-BT
// http://eleccelerator.com/wiki/index.php?title=DualShock_4
// and a little bit of https://github.com/chrippa/ds4drv
#define dual_shock_4_vendor 0x054C
#define dual_shock_4_usb 0x05C4
#define dual_shock_4_usb_v2 0x09CC
#define dual_shock_4_usb_dongle 0x0BA0
#define dual_shock_4_bluetooth 0x081F

// Dual Shock 4 compatible generic controllers
#define generic_dual_shock_4_vendor 0x0C12
#define generic_dual_shock_4_usb 0x0E20

#define fck_input_description_table_emplace(desc_id, desc_name, desc_data_type)                                                            \
	[(desc_id)] = {.id = (desc_id), .name = (desc_name), .data_type = (desc_data_type)}

typedef struct fck_dualsense_device
{
	// Duplicate, but maybe nice for double-checking
	fckc_u64 owner;
	fckc_u64 last_received;

	SDL_hid_device *hid;
	fck_input_data previous[fck_gamepad_count];
	fck_input_data current[fck_gamepad_count];
} fck_dualsense_device;

typedef struct fck_input_source_dualsense
{
	fck_input_source source;

	fckc_u64 last_discovery_ms;

	fckc_u64 owners[fck_input_dualsense_capacity]; // Always a number to double check stuff - Zero is nice, but make it something cool!
	fck_input_description descriptions[fck_gamepad_count];
	fck_dualsense_device devices[fck_input_dualsense_capacity];
} fck_input_source_dualsense;

static fckc_size_t fck_input_dualsense_owners(fckc_u64 **owners);
static fckc_size_t fck_input_dualsense_events(fck_input_event *events, fckc_size_t size);
static fckc_size_t fck_input_dualsense_descriptions(fck_input_description **descriptions);
static fckc_size_t fck_input_dualsense_states(fckc_u64 owner, fckc_u32 *ids, fck_input_data *states, fckc_size_t size);

static fck_input_source_dualsense input_source_dualsense = (fck_input_source_dualsense){
	.source =
		(fck_input_source){
			.name = "dualsense",
			.owners = fck_input_dualsense_owners,
			.events = fck_input_dualsense_events,
			.descriptions = fck_input_dualsense_descriptions,
			.states = fck_input_dualsense_states,
		},
	.last_discovery_ms = 0LLU,
	.owners = {0},
	.descriptions =
		{
			fck_input_description_table_emplace(fck_gamepad_left_stick, "left-stick", fck_input_data_float2),
			fck_input_description_table_emplace(fck_gamepad_right_stick, "right-stick", fck_input_data_float2),
			fck_input_description_table_emplace(fck_gamepad_left_trigger, "left-trigger", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_right_trigger, "right-trigger", fck_input_data_scalar),

			fck_input_description_table_emplace(fck_gamepad_north, "triangle", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_east, "circle", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_south, "cross", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_west, "square", fck_input_data_scalar),

			fck_input_description_table_emplace(fck_gamepad_left, "directional-pad-left", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_right, "directional-pad-right", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_up, "directional-pad-up", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_down, "directional-pad-down", fck_input_data_scalar),

			fck_input_description_table_emplace(fck_gamepad_left_1, "left-1", fck_input_data_float2),
			fck_input_description_table_emplace(fck_gamepad_right_1, "right-1", fck_input_data_float2),
			fck_input_description_table_emplace(fck_gamepad_left_2, "left-2", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_right_2, "right-2", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_left_3, "left-3", fck_input_data_float2),
			fck_input_description_table_emplace(fck_gamepad_right_3, "right-3", fck_input_data_float2),
			fck_input_description_table_emplace(fck_gamepad_select, "select", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_options, "options", fck_input_data_scalar),
			fck_input_description_table_emplace(fck_gamepad_home, "home", fck_input_data_scalar),
		},
	.devices = {0},
};

static uint64_t fck_input_dualsense_hash_serial_number(wchar_t *serial_number)
{
	// FNV
	uint64_t hash = 0xcbf29ce484222325ULL;
	const fckc_u64 fnv_prime = 0x100000001b3ULL;

	if (serial_number == NULL)
	{
		return 0;
	}

	while (*serial_number)
	{
		hash ^= (fckc_u64)(*serial_number++);
		hash *= fnv_prime;
	}
	return hash;
}

static void fck_input_source_dualsense_position(fck_dualsense_device *device, fck_gamepad_input_type event, fckc_u8 x, fckc_u8 y)
{
	fck_input_data *data = device->current + event;
	float vx = roundf((float)(x / 255.0f) * 100) / 100;
	float vy = roundf((float)(y / 255.0f) * 100) / 100;
	data->as_floats[0] = 1.0f - vx;
	data->as_floats[1] = vy;
}

static void fck_input_source_dualsense_axis(fck_dualsense_device *device, fck_gamepad_input_type event, fckc_u8 v)
{
	fck_input_data *data = device->current + event;
	data->as_scalar = (float)(v / 255.0f);
}

static void fck_input_source_dualsense_buttons_set_state_2(fck_dualsense_device *device, fck_dualsense_buttons_2 state)
{
	device->current[fck_gamepad_left_1].as_scalar = state.l1 ? 1.0f : 0.0f;
	device->current[fck_gamepad_right_1].as_scalar = state.r1 ? 1.0f : 0.0f;
	device->current[fck_gamepad_left_2].as_scalar = state.l2_digi ? 1.0f : 0.0f;
	device->current[fck_gamepad_right_2].as_scalar = state.r2_digi ? 1.0f : 0.0f;
	device->current[fck_gamepad_left_3].as_scalar = state.l3 ? 1.0f : 0.0f;
	device->current[fck_gamepad_right_3].as_scalar = state.r3 ? 1.0f : 0.0f;
	device->current[fck_gamepad_select].as_scalar = state.create ? 1.0f : 0.0f;
	device->current[fck_gamepad_options].as_scalar = state.options ? 1.0f : 0.0f;
}

static void fck_dualsense_device_set_state_3(fck_dualsense_device *device, fck_dualsense_buttons_3 state)
{
	device->current[fck_gamepad_home].as_scalar = state.ps ? 1.0f : 0.0f;
}

static void fck_dualsense_device_set_state_1(fck_dualsense_device *device, fck_dualsense_buttons_1 state)
{
	// Handle directional pad;
	fckc_u8 dpad = state.dpad;
	device->current[fck_gamepad_left].as_scalar = 0;
	device->current[fck_gamepad_right].as_scalar = 0;
	device->current[fck_gamepad_up].as_scalar = 0;
	device->current[fck_gamepad_down].as_scalar = 0;
	switch (dpad)
	{
	case fck_dualsense_input_report_usb_directional_pad_up_value:
		device->current[fck_gamepad_up].as_scalar = 1.0f;
		break;
	case fck_dualsense_input_report_usb_directional_pad_up_right_value:
		device->current[fck_gamepad_right].as_scalar = 1.0f;
		device->current[fck_gamepad_up].as_scalar = 1.0f;
		break;
	case fck_dualsense_input_report_usb_directional_pad_right_value:
		device->current[fck_gamepad_right].as_scalar = 1.0f;
		break;
	case fck_dualsense_input_report_usb_directional_pad_down_right_value:
		device->current[fck_gamepad_right].as_scalar = 1.0f;
		device->current[fck_gamepad_down].as_scalar = 1.0f;
		break;
	case fck_dualsense_input_report_usb_directional_pad_down_value:
		device->current[fck_gamepad_down].as_scalar = 1.0f;
		break;
	case fck_dualsense_input_report_usb_directional_pad_down_left_value:
		device->current[fck_gamepad_left].as_scalar = 1.0f;
		device->current[fck_gamepad_down].as_scalar = 1.0f;
		break;
	case fck_dualsense_input_report_usb_directional_pad_left_value:
		device->current[fck_gamepad_left].as_scalar = 1.0f;
		break;
	case fck_dualsense_input_report_usb_directional_pad_up_left_value:
		device->current[fck_gamepad_left].as_scalar = 1.0f;
		device->current[fck_gamepad_up].as_scalar = 1.0f;
		break;
	case fck_dualsense_input_report_usb_directional_pad_neutral_value:
	default:
		// Handled by default...
		break;
	}

	device->current[fck_gamepad_north].as_scalar = state.triangle ? 1.0f : 0.0f;
	device->current[fck_gamepad_east].as_scalar = state.circle ? 1.0f : 0.0f;
	device->current[fck_gamepad_south].as_scalar = state.cross ? 1.0f : 0.0f;
	device->current[fck_gamepad_west].as_scalar = state.square ? 1.0f : 0.0f;
}

static fckc_size_t fck_input_source_dualsense_find(fck_input_source_dualsense *input, fckc_u64 owner)
{
	fck_assert(owner != fck_input_dualsense_owner_unused || owner != fck_input_dualsense_owner_deleted);

	fckc_u64 hash = owner;
	fckc_u64 slot = hash % fck_arraysize(input->owners);
	for (fckc_size_t index = 0; index < fck_arraysize(input->owners); index++)
	{
		fckc_u64 *other = input->owners + slot;
		if (*other == fck_input_dualsense_owner_unused)
		{
			return 0;
		}
		if (*other != fck_input_dualsense_owner_deleted)
		{
			if (*other == owner)
			{
				return slot + 1;
			}
		}

		slot = (slot + 1) % fck_arraysize(input->owners);
	}
	return 0;
}

static fckc_size_t fck_input_source_dualsense_add(fck_input_source_dualsense *input, fckc_u64 owner)
{
	fck_assert(owner != fck_input_dualsense_owner_unused || owner != fck_input_dualsense_owner_deleted);

	fckc_u64 hash = owner;
	fckc_u64 slot = hash % fck_arraysize(input->owners);
	for (fckc_size_t index = 0; index < fck_arraysize(input->owners); index++)
	{
		fckc_u64 *other = input->owners + slot;
		if (*other == fck_input_dualsense_owner_unused || *other == fck_input_dualsense_owner_deleted)
		{
			*other = hash;
			return slot + 1;
		}

		slot = (slot + 1) % fck_arraysize(input->owners);
	}
	return 0;
}

static void fck_input_source_dualsense_remove(fck_input_source_dualsense *input, fckc_u64 owner)
{
	fck_assert(owner != fck_input_dualsense_owner_unused || owner != fck_input_dualsense_owner_deleted);

	fckc_u64 hash = owner;
	fckc_u64 slot = hash % fck_arraysize(input->owners);
	for (fckc_size_t index = 0; index < fck_arraysize(input->owners); index++)
	{
		fckc_u64 *other = input->owners + slot;
		if (*other == fck_input_dualsense_owner_unused)
		{
			return;
		}

		if (*other == owner)
		{
			*other = fck_input_dualsense_owner_deleted;
			return;
		}
		slot = (slot + 1) % fck_arraysize(input->owners);
	}
}

static wchar_t *fck_input_dualsense_serial_number_from_hid_device(SDL_hid_device *hid, wchar_t *out_string, size_t out_size)
{
	if (out_size < 14)
	{
		return NULL;
	}
	fckc_u8 report[fck_dualsense_usb_packet_length] = {0};
	report[0] = fck_dualsense_feature_report_serial_number;
	int report_result = SDL_hid_get_feature_report(hid, report, sizeof(report));
	if (report_result <= 0)
	{
		return NULL;
	}

	static const wchar_t hex_map[] = L"0123456789abcdef";
	out_string[0] = hex_map[report[6] >> 4];
	out_string[1] = hex_map[report[6] & 0x0F];
	out_string[2] = hex_map[report[5] >> 4];
	out_string[3] = hex_map[report[5] & 0x0F];
	out_string[4] = hex_map[report[4] >> 4];
	out_string[5] = hex_map[report[4] & 0x0F];
	out_string[6] = hex_map[report[3] >> 4];
	out_string[7] = hex_map[report[3] & 0x0F];
	out_string[8] = hex_map[report[2] >> 4];
	out_string[9] = hex_map[report[2] & 0x0F];
	out_string[10] = hex_map[report[1] >> 4];
	out_string[11] = hex_map[report[1] & 0x0F];

	out_string[12] = L'\0';
	return out_string;
}

static fckc_size_t fck_input_dualsense_owners(fckc_u64 **owners)
{
	(void)owners;
	return 0;
}

static void fck_input_dualsense_discovery(fck_input_source_dualsense *source)
{
	SDL_hid_device_info *sony_devices = SDL_hid_enumerate(fck_vendor_sony, 0);
	if (sony_devices)
	{
		SDL_hid_device_info *current = sony_devices;
		while (current)
		{
			if (current->product_id != fck_product_dualsense && current->product_id != fck_product_dualsense_edge)
			{
				continue;
			}
			if (current->bus_type != SDL_HID_API_BUS_USB && current->bus_type != SDL_HID_API_BUS_BLUETOOTH)
			{
				continue;
			}

			if (current->serial_number != NULL)
			{
				uint64_t hash = 0;
				uint64_t result = 0;

				if (current->serial_number == NULL || current->serial_number[0] == L'\0')
				{
					SDL_hid_device *hid = SDL_hid_open(current->vendor_id, current->product_id, current->serial_number);

					wchar_t serial_number[fck_dualsense_usb_packet_length] = {0};
					if (fck_input_dualsense_serial_number_from_hid_device(hid, serial_number, sizeof(serial_number)))
					{
						hash = fck_input_dualsense_hash_serial_number(serial_number);
						result = fck_input_source_dualsense_find(source, hash);
						if (result)
						{
							SDL_hid_close(hid);
						}
						else
						{
							result = fck_input_source_dualsense_add(source, hash);
							fck_assert(result);
							fck_dualsense_device *device = source->devices + result - 1;
							device->owner = hash;
							device->hid = hid;
						}
					}
					else
					{
						SDL_hid_close(hid);
					}
				}
				else
				{
					hash = fck_input_dualsense_hash_serial_number(current->serial_number);
					result = fck_input_source_dualsense_find(source, hash);
					if (result == 0)
					{
						result = fck_input_source_dualsense_add(source, hash);
						fck_assert(result);
						fck_dualsense_device *device = source->devices + result - 1;
						device->owner = hash;
						device->hid = SDL_hid_open(current->vendor_id, current->product_id, current->serial_number);
					}
				}
			}
			current = current->next;
		}

		SDL_hid_free_enumeration(sony_devices);
	}
}

typedef struct fckt_dualsense_events
{
	fck_input_event *events;
	fckc_size_t at;
	fckc_size_t capacity;
} fckt_dualsense_events;

static fckc_size_t fckt_dualsense_events_try_add(fckt_dualsense_events *queue, fck_dualsense_device *device, fck_gamepad_input_type type)
{
	if (queue->at >= queue->capacity)
	{
		return 0;
	}

	fck_input_data *previous = device->previous + type;
	fck_input_data *current = device->current + type;
	fck_input_description *desc = &input_source_dualsense.descriptions[type];
	// Maybe a is_same inline would be beneficial... 
	switch (desc->data_type)
	{
	case fck_input_data_scalar:
		if (previous->as_scalar == current->as_scalar)
		{
			return queue->at + 1;
		}
		break;
	case fck_input_data_float2:
		if (previous->as_floats[0] == current->as_floats[0] && previous->as_floats[1] == current->as_floats[1])
		{
			return queue->at + 1;
		}
		break;
	case fck_input_data_unicode:
		if (previous->as_unicode == current->as_unicode)
		{
			return queue->at + 1;
		}
		break;
	default:
		return queue->at + 1;
	}
	*previous = *current;

	fck_input_event *input_event = queue->events + queue->at;
	input_event->description = desc;
	input_event->time = device->last_received;
	input_event->owner = device->owner;
	input_event->source = &input_source_dualsense.source;
	input_event->userdata = NULL;
	input_event->data = device->current[type];
	queue->at = queue->at + 1;
	return queue->at + 1;
}

static fckc_size_t fck_input_dualsense_events(fck_input_event *events, fckc_size_t size)
{
	fckt_dualsense_events queue = (fckt_dualsense_events){.events = events, .at = 0, .capacity = size};

	fckc_u64 now = os->chrono->ms();
	fckc_u64 discovery_delta = now - input_source_dualsense.last_discovery_ms;

	if (discovery_delta >= fck_dualsense_discovery_rate)
	{
		input_source_dualsense.last_discovery_ms = now;
		fck_input_dualsense_discovery(&input_source_dualsense);
	}

	// TODO: Probably polling rate :)
	for (fckc_size_t index = 0; index < fck_arraysize(input_source_dualsense.owners); index++)
	{
		fckc_u64 owner = input_source_dualsense.owners[index];
		if (owner != fck_input_dualsense_owner_unused && owner != fck_input_dualsense_owner_deleted)
		{
			// Poll device state from HID
			fck_dualsense_device *device = input_source_dualsense.devices + index;
			fckc_u8 payload[fck_dualsense_largest_possible_packet_length];
			for (;;)
			{
				int result = SDL_hid_read_timeout(device->hid, payload, sizeof(payload), 0);
				if (result == -1)
				{
					fck_input_source_dualsense_remove(&input_source_dualsense, owner);
					memset(device, 0, sizeof(*device));
					break;
				}

				if (result == 0)
				{
					break;
				}

				fckc_u8 report_id = payload[0];
				switch (report_id)
				{
				case fck_dualsense_input_report_usb_simple: {
					if (result == fck_dualsense_usb_packet_length)
					{
						device->last_received = now;
						fck_dualsense_usb_report *report = (fck_dualsense_usb_report *)payload;
						memcpy(device->previous, device->current, sizeof(device->current));
						fck_input_source_dualsense_position(device, fck_gamepad_left_stick, report->left_stick_x, report->left_stick_y);
						fck_input_source_dualsense_position(device, fck_gamepad_right_stick, report->right_stick_x, report->right_stick_y);
						fck_input_source_dualsense_axis(device, fck_gamepad_left_trigger, report->l2_analog);
						fck_input_source_dualsense_axis(device, fck_gamepad_right_trigger, report->r2_analog);
						fck_dualsense_device_set_state_1(device, report->buttons1);
						fck_input_source_dualsense_buttons_set_state_2(device, report->buttons2);
						fck_dualsense_device_set_state_3(device, report->buttons3);
					}
				}
				break;
				case fck_dualsense_input_report_bluetooth_extended:
				case fck_dualsense_input_report_bluetooth_extended_charging: {
					device->last_received = now;
					fck_dualsense_bluetooth_extended_report *report = (fck_dualsense_bluetooth_extended_report *)payload;
					memcpy(device->previous, device->current, sizeof(device->current));
					fck_input_source_dualsense_position(device, fck_gamepad_left_stick, report->left_stick_x, report->left_stick_y);
					fck_input_source_dualsense_position(device, fck_gamepad_right_stick, report->right_stick_x, report->right_stick_y);
					fck_input_source_dualsense_axis(device, fck_gamepad_left_trigger, report->l2_analog);
					fck_input_source_dualsense_axis(device, fck_gamepad_right_trigger, report->r2_analog);
					fck_dualsense_device_set_state_1(device, report->buttons1);
					fck_input_source_dualsense_buttons_set_state_2(device, report->buttons2);
					fck_dualsense_device_set_state_3(device, report->buttons3);
				}
				break;
				default:
					break;
				}
			}

			for (fck_gamepad_input_type event_type = fck_gamepad_left_stick; event_type < fck_gamepad_count; event_type++)
			{
				if (!fckt_dualsense_events_try_add(&queue, device, event_type)) {
					return queue.at;
				}
			}
		}
	}

	return queue.at;
}
static fckc_size_t fck_input_dualsense_descriptions(fck_input_description **descriptions)
{
	*descriptions = input_source_dualsense.descriptions + 1;
	return fck_gamepad_count;
}

// TODO: Design flaw in all states functions. Why do I even guard against invalid ids??
static fckc_size_t fck_input_dualsense_states(fckc_u64 owner, fckc_u32 *ids, fck_input_data *states, fckc_size_t size)
{
	(void)owner, (void)ids, (void)states, (void)size;
	fckc_size_t result = fck_input_source_dualsense_find(&input_source_dualsense, owner);
	if (result)
	{
		fck_dualsense_device *device = input_source_dualsense.devices + result - 1;
		for (fckc_size_t index = 0; index < size; index++)
		{
			fckc_u32* id = ids + index;
			fck_input_data* state = states + index;
			if (*id >= fck_gamepad_count)
			{
				fckc_size_t last = size - 1;
				fck_input_data* last_state = states + last;
				fckc_u32* last_id = ids + last;
				*state = *last_state;

				*id = *last_id;
				size = size - 1;
				index = index - 1;
				continue;
			}

			const fck_input_data* data = device->current + *id;
			*state = *data;
		}
	}

	return 0;
}

FCK_EXPORT_API fck_input_source *dualsense_source = &input_source_dualsense.source;
