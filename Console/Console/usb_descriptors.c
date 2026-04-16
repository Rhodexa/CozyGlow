#include "tusb.h"
#include "pico/unique_id.h"
#include <string.h>

// ---------------------------------------------------------------------------
// Set this ONCE at boot (before tusb_init()) from the mode switch GPIO.
// All descriptor callbacks read it to decide which USB personality to present.
// ---------------------------------------------------------------------------
bool g_cdc_mode = false;

// ---------------------------------------------------------------------------
// String indices
// ---------------------------------------------------------------------------
enum {
    STR_IDX_LANG = 0,
    STR_IDX_MANUFACTURER,   // 1
    STR_IDX_PRODUCT,        // 2
    STR_IDX_SERIAL,         // 3
    STR_IDX_CDC_IFACE,      // 4  — CDC interface name shown in device manager
};

// ---------------------------------------------------------------------------
// Device descriptors
// MIDI mode:  bDeviceClass = 0x00 (class defined at interface level)
// CDC mode:   bDeviceClass = 0xEF (Misc, required for IAD composite device)
// Different product IDs so the host doesn't cache the wrong descriptor.
// ---------------------------------------------------------------------------
static const tusb_desc_device_t midi_device_desc = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = 0x00,
    .bDeviceSubClass    = 0x00,
    .bDeviceProtocol    = 0x00,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCAFE,
    .idProduct          = 0x4001,
    .bcdDevice          = 0x0100,
    .iManufacturer      = STR_IDX_MANUFACTURER,
    .iProduct           = STR_IDX_PRODUCT,
    .iSerialNumber      = STR_IDX_SERIAL,
    .bNumConfigurations = 0x01,
};

static const tusb_desc_device_t cdc_device_desc = {
    .bLength            = sizeof(tusb_desc_device_t),
    .bDescriptorType    = TUSB_DESC_DEVICE,
    .bcdUSB             = 0x0200,
    .bDeviceClass       = TUSB_CLASS_MISC,      // 0xEF — required for IAD
    .bDeviceSubClass    = 0x02,
    .bDeviceProtocol    = 0x01,
    .bMaxPacketSize0    = CFG_TUD_ENDPOINT0_SIZE,
    .idVendor           = 0xCAFE,
    .idProduct          = 0x4002,
    .bcdDevice          = 0x0100,
    .iManufacturer      = STR_IDX_MANUFACTURER,
    .iProduct           = STR_IDX_PRODUCT,
    .iSerialNumber      = STR_IDX_SERIAL,
    .bNumConfigurations = 0x01,
};

uint8_t const *tud_descriptor_device_cb(void) {
    return (uint8_t const *)(g_cdc_mode ? &cdc_device_desc : &midi_device_desc);
}

// ---------------------------------------------------------------------------
// Configuration descriptors
// ---------------------------------------------------------------------------

// MIDI: Audio Control interface (required by spec) + MIDI Streaming interface
// Endpoints: EP1 OUT (host→device), EP1 IN (device→host)
#define MIDI_ITF_COUNT      2
#define MIDI_CONFIG_LEN     (TUD_CONFIG_DESC_LEN + TUD_MIDI_DESC_LEN)

static const uint8_t midi_config_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, MIDI_ITF_COUNT, 0, MIDI_CONFIG_LEN, 0x00, 100),
    TUD_MIDI_DESCRIPTOR(0, 0, 0x01, 0x81, 64),
};

// CDC: Communication interface (with notification EP) + Data interface
// EP 0x81 = notification IN, EP 0x02 = data OUT, EP 0x82 = data IN
#define CDC_ITF_COUNT       2
#define CDC_CONFIG_LEN      (TUD_CONFIG_DESC_LEN + TUD_CDC_DESC_LEN)

static const uint8_t cdc_config_desc[] = {
    TUD_CONFIG_DESCRIPTOR(1, CDC_ITF_COUNT, 0, CDC_CONFIG_LEN, 0x00, 100),
    TUD_CDC_DESCRIPTOR(0, STR_IDX_CDC_IFACE, 0x81, 8, 0x02, 0x82, 64),
};

uint8_t const *tud_descriptor_configuration_cb(uint8_t index) {
    (void)index;
    return g_cdc_mode ? cdc_config_desc : midi_config_desc;
}

// ---------------------------------------------------------------------------
// String descriptors
// ---------------------------------------------------------------------------
static const char *string_descs[] = {
    NULL,               // 0: Language ID — handled inline below
    "CozyGlow",         // 1: Manufacturer
    "Console",          // 2: Product
    NULL,               // 3: Serial — generated from flash UID
    "Console MIDI CDC", // 4: CDC interface name
};

static uint16_t str_buf[32];

uint16_t const *tud_descriptor_string_cb(uint8_t index, uint16_t langid) {
    (void)langid;

    uint8_t chr_count;

    if (index == STR_IDX_LANG) {
        str_buf[1] = 0x0409; // English
        chr_count = 1;
    } else if (index == STR_IDX_SERIAL) {
        // Generate 16-char hex string from the Pico's unique 8-byte flash ID
        pico_unique_board_id_t uid;
        pico_get_unique_board_id(&uid);
        chr_count = 16;
        for (uint8_t i = 0; i < 8; i++) {
            str_buf[1 + i * 2]     = "0123456789ABCDEF"[(uid.id[i] >> 4) & 0xF];
            str_buf[1 + i * 2 + 1] = "0123456789ABCDEF"[uid.id[i] & 0xF];
        }
    } else if (index < (sizeof(string_descs) / sizeof(string_descs[0])) &&
               string_descs[index] != NULL) {
        const char *s = string_descs[index];
        chr_count = (uint8_t)strlen(s);
        if (chr_count > 31) chr_count = 31;
        for (uint8_t i = 0; i < chr_count; i++) {
            str_buf[1 + i] = (uint16_t)s[i];
        }
    } else {
        return NULL;
    }

    str_buf[0] = (uint16_t)((TUSB_DESC_STRING << 8) | (2 + 2 * chr_count));
    return str_buf;
}
