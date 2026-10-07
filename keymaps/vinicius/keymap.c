// clang-format off
#include QMK_KEYBOARD_H
#include "i18n.h"
#include "usb_util.h"
#include <transactions.h>

enum layer_names {
    _BASE = 0,
    _ALT,
    _NAV,
    _SYM,
    _ADJUST,
    _GAME,
    _FN,
    _MOUSE,
};

enum custom_keycodes {
    ST_MACRO_0 = SAFE_RANGE,
    ST_MACRO_1,
    MS_JIGGLER,
};

enum tap_dance_codes {
    DANCE_0,
};

// Declared before oled.c include so the OLED status display can reference them
static bool     ms_jiggler_enabled   = false;
static uint16_t ms_jiggler_timer     = 0;
static int8_t   ms_jiggler_direction = 5;
static bool     ms_snipe_enabled     = false;

#include "oled.c"

// ---------------------------------------------------------------------------
// Sofle hardware: USB sleep / RGB power management
// ---------------------------------------------------------------------------

uint8_t slave_sleep = 1;
typedef struct _master_to_slave_t { int m2s_data; } master_to_slave_t;
typedef struct _slave_to_master_t { int s2m_data; } slave_to_master_t;

void USER_SYNC_SLEEP_slave_handler(uint8_t in_buflen, const void *in_data,
                                   uint8_t out_buflen, void *out_data) {
    const master_to_slave_t *m2s = (const master_to_slave_t *)in_data;
    slave_sleep = (m2s->m2s_data == 0) ? 0 : 1;
}

void keyboard_post_init_user(void) {
    transaction_register_rpc(USER_SYNC_SLEEP, USER_SYNC_SLEEP_slave_handler);
    slave_sleep = 1;
}

// STM32 backup-register trick for clean bootloader entry
enum BackupRegister { BKP0 = 0 };
static const uint32_t CMD_BOOT = 0x544F4F42UL;

void backup_write(enum BackupRegister reg, uint32_t value) {
    (void)reg;
    *(volatile uint32_t *)0x20004000 = value;
}

static void wire_sleep(void) {
    static int first_time = -1;
    if (is_keyboard_master()) {
        if (!usb_connected_state()) {
            oled_off();
            writePinLow(RGB_LOW_MOS);
            writePinLow(RGB_MOS);
            master_to_slave_t m2s = {0};
            slave_to_master_t s2m = {0};
            transaction_rpc_exec(USER_SYNC_SLEEP, sizeof(m2s), &m2s, sizeof(s2m), &s2m);
        } else {
            if (first_time == -1) { oled_on(); first_time = 1; }
            writePinHigh(RGB_LOW_MOS);
            writePinHigh(RGB_MOS);
            master_to_slave_t m2s = {1};
            slave_to_master_t s2m = {1};
            transaction_rpc_exec(USER_SYNC_SLEEP, sizeof(m2s), &m2s, sizeof(s2m), &s2m);
        }
    } else {
        if (slave_sleep == 0) {
            oled_off();
            writePinLow(RGB_LOW_MOS);
            writePinLow(RGB_MOS);
        } else {
            if (first_time == -1) { oled_on(); first_time = 1; }
            writePinHigh(RGB_LOW_MOS);
            writePinHigh(RGB_MOS);
        }
    }
}

void suspend_power_down_user(void) { wire_sleep(); }
void suspend_wakeup_init_user(void) { wire_sleep(); }
void housekeeping_task_user(void) { wire_sleep(); }

// ---------------------------------------------------------------------------
// Dual-function key aliases
// ---------------------------------------------------------------------------

#define DUAL_FUNC_0 LT(1,  KC_F6)
#define DUAL_FUNC_1 LT(6,  KC_H)
#define DUAL_FUNC_2 LT(7,  KC_9)
#define DUAL_FUNC_3 LT(13, KC_S)

// ---------------------------------------------------------------------------
// Keymaps  (Sofle 60-key LAYOUT)
//
// Row 0–2: 6+6 keys  — identical to Helix
// Row 3:   6+1+1+6   — inner pair (L45, R40) carry KC_MEH
// Row 4:   5+5 thumbs — drop the 4 TRNS/NO from Helix's 7+7
//   Left:  [CAPS, JIGGLER, LGUI, CW_TOGG, LT(_FN,SPC)]
//   Right: [LT(_NAV,BSPC), TT(_MOUSE), LALT, NO, NO]
// ---------------------------------------------------------------------------

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
  [_BASE] = LAYOUT(
    MO(_ADJUST),    KC_1,                KC_2,           KC_3,              KC_4,              KC_5,                                         KC_6,             KC_7,              KC_8,              KC_9,              KC_0,                  KC_NO,
    KC_GRAVE,       KC_Q,                KC_W,           KC_E,              KC_R,              KC_T,                                         KC_Y,             KC_U,              KC_I,              KC_O,              KC_P,                  KC_DEL,
    KC_ESC,         LT(_SYM,KC_A),       KC_S,           MT(MOD_LCTL,KC_D), MT(MOD_LSFT,KC_F), KC_G,                                         KC_H,             MT(MOD_LSFT,KC_J), MT(MOD_LCTL,KC_K), KC_L,              LT(_SYM,BR_SCLN),      KC_ENTER,
    KC_CAPS,        MT(MOD_MEH, KC_Z),   KC_X,           KC_C,              KC_V,              KC_B,                  KC_NO,  KC_NO,         KC_N,             KC_M,              KC_COMMA,          KC_DOT,            MT(MOD_MEH, BR_SLSH),  CW_TOGG,
                    KC_NO,               KC_NO,          MS_JIGGLER,        KC_LEFT_GUI,       LT(_FN,KC_SPACE),                             LT(_NAV,KC_BSPC), MO(_MOUSE),        KC_LEFT_ALT,       KC_NO,             KC_NO
  ),

  [_ALT] = LAYOUT(
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,    KC_TRANSPARENT,    KC_TRANSPARENT,                               KC_TRANSPARENT, KC_TRANSPARENT,    KC_TRANSPARENT,    KC_TRANSPARENT,    KC_TRANSPARENT,      KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_W,           KC_F,              KC_P,              KC_G,                                         KC_J,           KC_L,              KC_U,              KC_Y,              KC_SCLN,             KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_R,           MT(MOD_LCTL,KC_S), MT(MOD_LSFT,KC_T), KC_D,                                         KC_TRANSPARENT, MT(MOD_LSFT,KC_N), MT(MOD_LCTL,KC_E), KC_I,              LT(_SYM,KC_O),       KC_TRANSPARENT,
    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,    KC_TRANSPARENT,    KC_TRANSPARENT,           KC_NO,  KC_NO,      KC_K,           KC_TRANSPARENT,    KC_TRANSPARENT,    KC_TRANSPARENT,    KC_TRANSPARENT,      KC_TRANSPARENT,
                    KC_TRANSPARENT, KC_TRANSPARENT, KC_TRANSPARENT,    KC_TRANSPARENT,    KC_TRANSPARENT,                               KC_TRANSPARENT, KC_TRANSPARENT,    KC_TRANSPARENT,    KC_TRANSPARENT,    KC_TRANSPARENT
  ),

  [_NAV] = LAYOUT(
    KC_NO,          KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_NO,                                        KC_NO,          KC_NO,             KC_NO,             KC_NO,             KC_NO,             KC_NO,
    KC_NO,          KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_NO,                                        KC_PAGE_UP,     LCTL(KC_LEFT),     KC_UP,             LCTL(KC_RIGHT),    KC_NO,             KC_TRANSPARENT,
    KC_NO,          KC_LEFT_GUI,    KC_LEFT_ALT,    KC_LEFT_CTRL,      KC_LEFT_SHIFT,     KC_NO,                                        KC_PGDN,        KC_LEFT,           KC_DOWN,           KC_RIGHT,          KC_NO,             KC_TRANSPARENT,
    KC_NO,          KC_NO,          KC_NO,          KC_NO,             KC_NO,             RALT(LSFT(RCTL(KC_B))),   KC_NO,  KC_NO,      KC_NO,          KC_HOME,           KC_NO,             KC_END,            KC_NO,             KC_NO,
                    KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_SPACE,                                     KC_BSPC,        KC_NO,             KC_NO,             KC_NO,             KC_NO
  ),

  [_SYM] = LAYOUT(
    KC_NO,          KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_NO,                                        KC_NO,          KC_NO,             KC_NO,             KC_NO,             KC_NO,               KC_NO,
    KC_NO,          KC_EXLM,        KC_AT,          KC_HASH,           KC_DLR,            KC_PERC,                                      BR_QUES,        KC_EQUAL,          BR_QUOT,           BR_DQT,            US_PLUS,             KC_NO,
    KC_NO,          BR_ACUT,        BR_GRAV,        BR_BSLS,           BR_PIPE,           BR_CIRC,                                      KC_ASTR,        KC_LPRN,           BR_LCBR,           BR_LBRC,           KC_MINUS,            KC_NO,
    KC_NO,          KC_NO,          KC_NO,          BR_CCDL,           BR_COLN,           KC_AMPR,                  KC_NO,  KC_NO,      BR_TILD,        KC_RPRN,           BR_RCBR,           BR_RBRC,           KC_UNDS,           KC_NO,
                    KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_SPACE,                                     KC_BSPC,        KC_NO,             KC_NO,             KC_NO,             KC_NO
  ),

  [_ADJUST] = LAYOUT(
    KC_NO,          KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_NO,                                        KC_NO,               KC_NO,                KC_NO,              KC_NO,                KC_NO,           KC_NO,
    KC_NO,          TO(_BASE),      KC_NO,          KC_NO,             KC_NO,             KC_NO,                                        KC_NO,               KC_BRIGHTNESS_DOWN,   KC_AUDIO_VOL_UP,    KC_BRIGHTNESS_UP,     KC_PSCR,         KC_NO,
    KC_NO,          KC_NO,          KC_NO,          KC_NO,             KC_NO,             TO(_GAME),                                    KC_NO,               KC_MEDIA_PREV_TRACK,  KC_AUDIO_VOL_DOWN,  KC_MEDIA_NEXT_TRACK,  KC_NO,           KC_NO,
    KC_NO,          KC_NO,          KC_NO,          TO(_ALT),          KC_NO,             KC_NO,                    KC_NO,  KC_NO,      KC_MEDIA_PLAY_PAUSE, KC_AUDIO_MUTE,        KC_NO,              KC_NO,                KC_NO,           KC_NO,
                    KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_NO,                                        KC_NO,               KC_NO,                KC_NO,              KC_NO,                KC_NO
  ),

  [_GAME] = LAYOUT(
    KC_ESC,         KC_1,           KC_2,           KC_3,              KC_4,              KC_5,                                         KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,    KC_TRANSPARENT,    TO(_BASE),
    KC_TAB,         KC_Q,           KC_W,           KC_E,              KC_R,              KC_T,                                         KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,    KC_TRANSPARENT,    KC_NO,
    KC_LSFT,        KC_A,           KC_S,           KC_D,              KC_F,              KC_G,                                         KC_TRANSPARENT,      KC_J,                 KC_K,                KC_TRANSPARENT,    BR_SCLN,           KC_ENTER,
    KC_LCTL,        KC_Z,           KC_X,           KC_C,              KC_V,              KC_B,                     KC_NO, KC_NO,       KC_TRANSPARENT,      KC_TRANSPARENT,       KC_TRANSPARENT,      KC_TRANSPARENT,    KC_TRANSPARENT,    KC_NO,
                    KC_NO,          KC_NO,          KC_LALT,           KC_SPACE,          KC_M,                                         KC_BSPC,             KC_ENTER,             KC_F5,               KC_F4,             KC_F3
  ),

  [_FN] = LAYOUT(
    KC_NO,          KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_NO,                                        KC_NO,               KC_NO,             KC_NO,               KC_NO,              KC_NO,                 KC_NO,
    KC_NO,          KC_NO,          KC_F7,          KC_F8,             DUAL_FUNC_0,       DUAL_FUNC_1,                                  KC_NO,               KC_7,              KC_8,                KC_9,               US_PLUS,               KC_NO,
    KC_NO,          KC_NO,          KC_F4,          TD(DANCE_0),       KC_F6,             DUAL_FUNC_2,                                  LSFT(KC_8),          MT(MOD_LSFT,KC_4), MT(MOD_LCTL,KC_5),   MT(MOD_LALT,KC_6),  MT(MOD_LGUI,KC_MINUS), KC_NO,
    KC_NO,          KC_NO,          KC_F1,          KC_F2,             KC_F3,             DUAL_FUNC_3,              KC_NO,  KC_NO,      KC_NO,               KC_1,              KC_2,                KC_3,               KC_SLASH,              KC_NO,
                    KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_SPACE,                                     KC_BSPC,             KC_0,              KC_DOT,              KC_COMMA,           KC_NO
  ),

  [_MOUSE] = LAYOUT(
    KC_NO,          KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_NO,                                        KC_NO,          KC_NO,             KC_NO,             KC_NO,             KC_NO,               TT(_MOUSE),
    KC_NO,          KC_NO,          LCTL(KC_W),     KC_NO,             LCTL(LSFT(KC_T)),  LCTL(KC_T),                                   KC_NO,          LALT(KC_LEFT),     KC_MS_UP,          LALT(KC_RIGHT),    KC_NO,               KC_NO,
    KC_ESC,         KC_MS_WH_LEFT,  KC_MS_BTN2,     KC_MS_BTN3,        KC_MS_BTN1,        KC_MS_WH_RIGHT,                               KC_NO,          KC_MS_LEFT,        KC_MS_DOWN,        KC_MS_RIGHT,       KC_NO,               KC_TRANSPARENT,
    KC_NO,          KC_HOME,        KC_MS_WH_UP,    KC_NO,             KC_MS_WH_DOWN,     KC_END,                   KC_NO,  KC_NO,      KC_NO,          ST_MACRO_0,        KC_NO,             ST_MACRO_1,        KC_NO,               KC_NO,
                    KC_NO,          KC_NO,          KC_NO,             KC_NO,             KC_RIGHT_CTRL,                                KC_NO,          KC_NO,             KC_NO,             KC_NO,             KC_TRANSPARENT
  ),
};

// ---------------------------------------------------------------------------
// Chordal hold layout (mirrors the LAYOUT macro shape)
// ---------------------------------------------------------------------------

const char chordal_hold_layout[MATRIX_ROWS][MATRIX_COLS] PROGMEM = LAYOUT(
  'L', 'L', 'L', 'L', 'L', 'L',           'R', 'R', 'R', 'R', 'R', 'R',
  'L', 'L', 'L', 'L', 'L', 'L',           'R', 'R', 'R', 'R', 'R', 'R',
  'L', 'L', 'L', 'L', 'L', 'L',           'R', 'R', 'R', 'R', 'R', 'R',
  'L', 'L', 'L', 'L', 'L', 'L', 'L', 'R', 'R', 'R', 'R', 'R', 'R', 'R',
            '*', '*', '*', '*', '*', '*', '*', '*', '*', '*'
);

// ---------------------------------------------------------------------------
// Combos
// ---------------------------------------------------------------------------

const uint16_t PROGMEM combo0[] = { MT(MOD_LCTL, KC_D), MT(MOD_LSFT, KC_F), COMBO_END };
const uint16_t PROGMEM combo1[] = { KC_X, KC_C, COMBO_END };
const uint16_t PROGMEM combo2[] = { KC_M, KC_COMMA, COMBO_END };

combo_t key_combos[COMBO_COUNT] = {
    COMBO(combo0, KC_TAB),
    COMBO(combo1, BR_CCDL),
    COMBO(combo2, BR_TILD),
};

// ---------------------------------------------------------------------------
// Tapping term per key
// ---------------------------------------------------------------------------

uint16_t get_tapping_term(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
        case KC_S:
            return TAPPING_TERM + 50;
        default:
            return TAPPING_TERM;
    }
}

// ---------------------------------------------------------------------------
// Tap-dance: DANCE_0 (F5 with shift/ctrl modifiers)
// ---------------------------------------------------------------------------

typedef struct { bool is_press_action; uint8_t step; } tap;

enum {
    SINGLE_TAP = 1,
    SINGLE_HOLD,
    DOUBLE_TAP,
    DOUBLE_HOLD,
    DOUBLE_SINGLE_TAP,
    MORE_TAPS,
};

static tap dance_state[1];

static uint8_t dance_step(tap_dance_state_t *state) {
    if (state->count == 1) {
        if (state->interrupted || !state->pressed) return SINGLE_TAP;
        else return SINGLE_HOLD;
    } else if (state->count == 2) {
        if (state->interrupted) return DOUBLE_SINGLE_TAP;
        else if (state->pressed) return DOUBLE_HOLD;
        else return DOUBLE_TAP;
    }
    return MORE_TAPS;
}

void on_dance_0(tap_dance_state_t *state, void *user_data) {
    if (state->count == 3) {
        tap_code16(KC_F5); tap_code16(KC_F5); tap_code16(KC_F5);
    }
    if (state->count > 3) { tap_code16(KC_F5); }
}

void dance_0_finished(tap_dance_state_t *state, void *user_data) {
    dance_state[0].step = dance_step(state);
    switch (dance_state[0].step) {
        case SINGLE_TAP:        register_code16(KC_F5);                  break;
        case SINGLE_HOLD:       register_code16(RSFT(KC_F5));            break;
        case DOUBLE_TAP:        register_code16(LCTL(LSFT(KC_F5)));      break;
        case DOUBLE_SINGLE_TAP: tap_code16(KC_F5); register_code16(KC_F5); break;
    }
}

void dance_0_reset(tap_dance_state_t *state, void *user_data) {
    wait_ms(10);
    switch (dance_state[0].step) {
        case SINGLE_TAP:        unregister_code16(KC_F5);             break;
        case SINGLE_HOLD:       unregister_code16(RSFT(KC_F5));       break;
        case DOUBLE_TAP:        unregister_code16(LCTL(LSFT(KC_F5))); break;
        case DOUBLE_SINGLE_TAP: unregister_code16(KC_F5);             break;
    }
    dance_state[0].step = 0;
}

tap_dance_action_t tap_dance_actions[] = {
    [DANCE_0] = ACTION_TAP_DANCE_FN_ADVANCED(on_dance_0, dance_0_finished, dance_0_reset),
};

// ---------------------------------------------------------------------------
// Mouse jiggler
// ---------------------------------------------------------------------------

void matrix_scan_user(void) {
    if (ms_jiggler_enabled && timer_elapsed(ms_jiggler_timer) > 2000) {
        ms_jiggler_timer = timer_read();
        report_mouse_t report = {0};
        report.x = ms_jiggler_direction;
        host_mouse_send(&report);
        ms_jiggler_direction = -ms_jiggler_direction;
    }
}

// ---------------------------------------------------------------------------
// Encoder map
// ---------------------------------------------------------------------------

const uint16_t PROGMEM encoder_map[][NUM_ENCODERS][2] = {
    [_BASE]   = { ENCODER_CCW_CW(KC_MS_WH_UP, KC_MS_WH_DOWN),  ENCODER_CCW_CW(KC_VOLD,    KC_VOLU)     },
    [_ALT]    = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS),            ENCODER_CCW_CW(KC_TRNS,    KC_TRNS)     },
    [_NAV]    = { ENCODER_CCW_CW(KC_PGUP, KC_PGDN),            ENCODER_CCW_CW(KC_HOME,    KC_END)      },
    [_SYM]    = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS),            ENCODER_CCW_CW(KC_TRNS,    KC_TRNS)     },
    [_ADJUST] = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS),            ENCODER_CCW_CW(KC_TRNS,    KC_TRNS)     },
    [_GAME]   = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS),            ENCODER_CCW_CW(KC_TRNS,    KC_TRNS)     },
    [_FN]     = { ENCODER_CCW_CW(KC_TRNS, KC_TRNS),            ENCODER_CCW_CW(KC_TRNS,    KC_TRNS)     },
    [_MOUSE]  = { ENCODER_CCW_CW(KC_MS_WH_UP, KC_MS_WH_DOWN),  ENCODER_CCW_CW(KC_MS_LEFT, KC_MS_RIGHT) },
};

// ---------------------------------------------------------------------------
// process_record_user
// ---------------------------------------------------------------------------

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    switch (keycode) {
    case QK_BOOTLOADER:
        backup_write(BKP0, CMD_BOOT);
        NVIC_SystemReset();
        return false;

    case QK_CLEAR_EEPROM:
        eeconfig_init();
        NVIC_SystemReset();
        return false;

    case QK_MODS ... QK_MODS_MAX:
        if (IS_MOUSE_KEYCODE(QK_MODS_GET_BASIC_KEYCODE(keycode)) ||
            IS_CONSUMER_KEYCODE(QK_MODS_GET_BASIC_KEYCODE(keycode))) {
            if (record->event.pressed) {
                add_mods(QK_MODS_GET_MODS(keycode));
                send_keyboard_report();
                wait_ms(2);
                register_code(QK_MODS_GET_BASIC_KEYCODE(keycode));
                return false;
            } else {
                wait_ms(2);
                del_mods(QK_MODS_GET_MODS(keycode));
            }
        }
        break;

    case ST_MACRO_0:
        if (record->event.pressed) {
            SEND_STRING(SS_LCTL(SS_LSFT(SS_TAP(X_TAB))));
        }
        break;

    case ST_MACRO_1:
        if (record->event.pressed) {
            SEND_STRING(SS_LCTL(SS_TAP(X_TAB)));
        }
        break;

    case DUAL_FUNC_0:
        if (record->tap.count > 0) {
            if (record->event.pressed) register_code16(KC_F9);
            else                       unregister_code16(KC_F9);
        } else {
            if (record->event.pressed) register_code16(LCTL(LSFT(KC_F9)));
            else                       unregister_code16(LCTL(LSFT(KC_F9)));
        }
        return false;

    case DUAL_FUNC_1:
        if (record->tap.count > 0) {
            if (record->event.pressed) register_code16(KC_F10);
            else                       unregister_code16(KC_F10);
        } else {
            if (record->event.pressed) register_code16(LSFT(KC_F10));
            else                       unregister_code16(LSFT(KC_F10));
        }
        return false;

    case DUAL_FUNC_2:
        if (record->tap.count > 0) {
            if (record->event.pressed) register_code16(KC_F11);
            else                       unregister_code16(KC_F11);
        } else {
            if (record->event.pressed) register_code16(LSFT(KC_F11));
            else                       unregister_code16(LSFT(KC_F11));
        }
        return false;

    case DUAL_FUNC_3:
        if (record->tap.count > 0) {
            if (record->event.pressed) register_code16(KC_F12);
            else                       unregister_code16(KC_F12);
        } else {
            if (record->event.pressed) register_code16(LCTL(KC_F12));
            else                       unregister_code16(LCTL(KC_F12));
        }
        return false;

    case MS_JIGGLER:
        if (record->event.pressed) {
            ms_jiggler_enabled = !ms_jiggler_enabled;
            ms_jiggler_timer   = timer_read();
        }
        return false;
    }

    return true;
}
