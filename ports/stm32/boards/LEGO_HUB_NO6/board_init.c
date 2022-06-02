/*
 * This file is part of the MicroPython project, http://micropython.org/
 *
 * The MIT License (MIT)
 *
 * Copyright (c) 2021 Damien P. George
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */

#include "py/mphal.h"
#include "irq.h"

void board_init(void) {
    if (query_irq() == IRQ_STATE_DISABLED) {
        enable_irq(IRQ_STATE_ENABLED);
    }

    // Enable 3V3 for all ports
    mp_hal_pin_output(pyb_pin_PORT_3V3_EN);
    mp_hal_pin_high(pyb_pin_PORT_3V3_EN);

    // Port A
    // Enable RX/TX buffer
    mp_hal_pin_output(pyb_pin_PORTA_EN);
    mp_hal_pin_low(pyb_pin_PORTA_EN);

    // Port B
    // Enable RX/TX buffer
    mp_hal_pin_output(pyb_pin_PORTB_EN);
    mp_hal_pin_low(pyb_pin_PORTB_EN);

    // Port C
    // Enable RX/TX buffer
    mp_hal_pin_output(pyb_pin_PORTC_EN);
    mp_hal_pin_low(pyb_pin_PORTC_EN);

    // Port D
    // Enable RX/TX buffer
    mp_hal_pin_output(pyb_pin_PORTD_EN);
    mp_hal_pin_low(pyb_pin_PORTD_EN);

    // Port E
    // Enable RX/TX buffer
    mp_hal_pin_output(pyb_pin_PORTE_EN);
    mp_hal_pin_low(pyb_pin_PORTE_EN);
    // Disable RS485 driver
    mp_hal_pin_output(pyb_pin_PORTE_RTS);
    mp_hal_pin_low(pyb_pin_PORTE_RTS);

    // Port F
    // Enable RX/TX buffer
    mp_hal_pin_output(pyb_pin_PORTF_EN);
    mp_hal_pin_low(pyb_pin_PORTF_EN);
    // Disable RS485 driver
    mp_hal_pin_output(pyb_pin_PORTF_RTS);
    mp_hal_pin_low(pyb_pin_PORTF_RTS);
}

#if BUILDING_MBOOT

#include "mboot/mboot.h"
#include "boardctrl.h"
#include "adc.h"
#include "hub_display.h"

#define RESET_MODE_NUM_STATES (4)
#define RESET_MODE_TIMEOUT_CYCLES (8)

#define PATTERN_B (0x00651946)
#define PATTERN_F (0x0021184e)
#define PATTERN_N (0x01296ad2)
#define PATTERN_S (0x0064104c)

static void board_led_pattern(int reset_mode, uint16_t brightness) {
    static const uint32_t pixels[] = {
        0,
        PATTERN_N,
        PATTERN_S,
        PATTERN_F,
        PATTERN_B,
    };
    uint32_t pixel = pixels[reset_mode];
    for (int i = 0; i < 25; ++i) {
        hub_display_set(i, brightness * ((pixel >> i) & 1));
    }
    hub_display_update();
}

static void board_button_init(void) {
    mp_hal_pin_config(pin_A1, MP_HAL_PIN_MODE_ADC, MP_HAL_PIN_PULL_NONE, 0);
    adc_config(ADC1, 12);
}

static int board_button_state(void) {
    uint16_t value = adc_config_and_read_u16(ADC1, 1, ADC_SAMPLETIME_15CYCLES);
    return value < 44000;
}

void board_mboot_cleanup(int reset_mode) {
    board_led_pattern(0, 0);
    hub_display_off();
}

void board_mboot_led_init(void) {
    hub_display_on();
}

void board_mboot_led_state(int led, int state) {
    if (state) {
        hub_display_set(28 + led, 0x7fff);
        hub_display_set(31 + led, 0x7fff);
    } else {
        hub_display_set(28 + led, 0);
        hub_display_set(31 + led, 0);
    }
    hub_display_update();
}

int board_mboot_get_reset_mode(uint32_t *initial_r0) {
    board_button_init();
    int reset_mode = BOARDCTRL_RESET_MODE_NORMAL;
    if (board_button_state()) {
        // Cycle through reset modes while USR is held.
        // Timeout is roughly 20s, where reset_mode=1.
        systick_init();
        hub_display_on();
        reset_mode = 0;
        for (int i = 0; i < (RESET_MODE_NUM_STATES * RESET_MODE_TIMEOUT_CYCLES + 1) * 32; i++) {
            if (i % 32 == 0) {
                if (++reset_mode > RESET_MODE_NUM_STATES) {
                    reset_mode = BOARDCTRL_RESET_MODE_NORMAL;
                }
                board_led_pattern(reset_mode, 0x7fff);
            }
            if (!board_button_state()) {
                break;
            }
            mp_hal_delay_ms(19);
        }
        // Flash the selected reset mode.
        for (int i = 0; i < 6; i++) {
            board_led_pattern(reset_mode, 0x0fff);
            mp_hal_delay_ms(50);
            board_led_pattern(reset_mode, 0x7fff);
            mp_hal_delay_ms(50);
        }
        mp_hal_delay_ms(300);
    }
    board_led_pattern(0, 0);
    return reset_mode;
}

#endif
