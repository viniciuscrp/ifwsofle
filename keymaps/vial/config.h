/* Copyright 2020 Josef Adamcik
 * Modification for VIA support and RGB underglow by Jens Bonk-Wiltfang
 * Modification for Vial support by Drew Petersen
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

// clang-format off

#pragma once

// Vial Support
#define VIAL_KEYBOARD_UID {0x9E, 0xFE, 0x38, 0x9F, 0xFE, 0x9A, 0x11, 0x0D}

#define VIALRGB_NO_DIRECT

#define CHORDAL_HOLD
#define FLOW_TAP_TERM 130
#undef TAPPING_TERM
#define TAPPING_TERM 200
#define TAPPING_TERM_PER_KEY
#define PERMISSIVE_HOLD
#define TAPPING_TOGGLE 1

#define COMBO_COUNT 3

#define BOOTMAGIC_ROW    4
#define BOOTMAGIC_COLUMN 0
