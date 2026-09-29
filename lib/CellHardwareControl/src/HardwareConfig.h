#ifndef HARDWARE_CONFIG_H
#define HARDWARE_CONFIG_H

#include <Arduino.h>

/**
 * @file HardwareConfig.h
 * @brief Pin definitions, hardware constants, and system configuration for
 *        Role 4: Cell Hardware & Alert Control (4-Cell Configuration).
 *
 * Designed for ESP32 DevKit V1 with zero pin conflicts against I2C (OLED & RTC) or UART.
 */

// ============================================================================
// SYSTEM CAPACITY & SCALABILITY CONFIGURATION
// ============================================================================

// System configured for exactly 4 independent cells
#define MAX_CELLS 4
#define ACTIVE_CELL_COUNT 4

// ============================================================================
// PIN DEFINITIONS - I2C (RESERVED BY EXISTING MODULES - DO NOT ALTER)
// ============================================================================
// GPIO 21 -> I2C SDA (Used by SSD1306 OLED & DS1307 RTC)
// GPIO 22 -> I2C SCL (Used by SSD1306 OLED & DS1307 RTC)
// GPIO 1  -> UART TX0 (Used by Serial Monitor)
// GPIO 3  -> UART RX0 (Used by Serial Monitor)

// ============================================================================
// PIN DEFINITIONS - CELL 1 (COMPARTMENT 1)
// ============================================================================
#define PIN_CELL1_RED     25  // RGB Red channel (GPIO25)
#define PIN_CELL1_GREEN   26  // RGB Green channel (GPIO26)
#define PIN_CELL1_BLUE    -1  // Blue channel not needed (Red+Green = Yellow)
#define PIN_CELL1_BUTTON  32  // Push button (active LOW with internal pull-up)
#define PIN_CELL1_SERVO   18  // Compartment 1 Servo (PWM)

// ============================================================================
// PIN DEFINITIONS - CELL 2 (COMPARTMENT 2)
// ============================================================================
#define PIN_CELL2_RED     14  // RGB Red channel (GPIO14)
#define PIN_CELL2_GREEN   16  // RGB Green channel (GPIO16)
#define PIN_CELL2_BLUE    -1  // Blue channel not needed
#define PIN_CELL2_BUTTON  33  // Push button (active LOW with internal pull-up)
#define PIN_CELL2_SERVO   19  // Compartment 2 Servo (PWM)

// ============================================================================
// PIN DEFINITIONS - CELL 3 (COMPARTMENT 3)
// ============================================================================
#define PIN_CELL3_RED     13  // RGB Red channel (GPIO13)
#define PIN_CELL3_GREEN   12  // RGB Green channel (GPIO12)
#define PIN_CELL3_BLUE    -1  // Blue channel not needed
#define PIN_CELL3_BUTTON  27  // Push button (active LOW with internal pull-up)
#define PIN_CELL3_SERVO   5   // Compartment 3 Servo (PWM)

// ============================================================================
// PIN DEFINITIONS - CELL 4 (COMPARTMENT 4)
// ============================================================================
#define PIN_CELL4_RED     4   // RGB Red channel (GPIO4)
#define PIN_CELL4_GREEN   2   // RGB Green channel (GPIO2)
#define PIN_CELL4_BLUE    -1  // Blue channel not needed
#define PIN_CELL4_BUTTON  23  // Push button (active LOW with internal pull-up)
#define PIN_CELL4_SERVO   17  // Compartment 4 Servo (PWM)

// ============================================================================
// COMMON / SHARED ALERT ACTUATOR
// ============================================================================
#define PIN_COMMON_BUZZER    15  // Piezo Buzzer (PWM / Tone, GPIO15)
#define BUZZER_LEDC_CHANNEL  6   // Dedicated LEDC Channel 6 (Timer 3, no conflict with servos)

// ============================================================================
// TIMING CONSTANTS (NON-BLOCKING MILLIS)
// ============================================================================
#define DEBOUNCE_DELAY_MS           50    // Mechanical button debounce settling time (ms)
#define CONFIRM_GREEN_DURATION_MS   1000  // Duration to show GREEN after confirmation (ms)
#define BUZZER_FREQ_HZ              2500  // Resonant frequency for buzzer alert tone (Hz)
#define BUZZER_PULSE_ON_MS          300   // Buzzer intermittent pulse ON duration (ms)
#define BUZZER_PULSE_OFF_MS         300   // Buzzer intermittent pulse OFF duration (ms)

// ============================================================================
// SERVO CONFIGURATION
// ============================================================================
#define SERVO_CLOSED_ANGLE          0     // Closed / locked compartment position (degrees)
#define SERVO_OPEN_ANGLE            90    // Open / accessible compartment position (degrees)

// Structure for convenient per-cell pin bundling
struct CellPinConfig {
    int8_t pinRed;
    int8_t pinGreen;
    int8_t pinBlue;
    int8_t pinButton;
    int8_t pinServo;
};

// Default pin configurations for all 4 cells
static const CellPinConfig DEFAULT_CELL_PIN_CONFIGS[MAX_CELLS] = {
    { PIN_CELL1_RED, PIN_CELL1_GREEN, PIN_CELL1_BLUE, PIN_CELL1_BUTTON, PIN_CELL1_SERVO },
    { PIN_CELL2_RED, PIN_CELL2_GREEN, PIN_CELL2_BLUE, PIN_CELL2_BUTTON, PIN_CELL2_SERVO },
    { PIN_CELL3_RED, PIN_CELL3_GREEN, PIN_CELL3_BLUE, PIN_CELL3_BUTTON, PIN_CELL3_SERVO },
    { PIN_CELL4_RED, PIN_CELL4_GREEN, PIN_CELL4_BLUE, PIN_CELL4_BUTTON, PIN_CELL4_SERVO }
};

#endif // HARDWARE_CONFIG_H
