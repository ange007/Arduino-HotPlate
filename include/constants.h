#include <avr/pgmspace.h>

#define IS_DEBUG false

/**
 * Pins
 */

#define SSR_PIN 3
#define SSR_ON_LEVEL HIGH
#define SSR_OFF_LEVEL LOW

#define BUZZER_PIN 9
#define FAN_PIN 2
#define THERMISTOR_PIN A0

#define ENCODER_CLK_PIN 7
#define ENCODER_DT_PIN 6
#define ENCODER_BTN_PIN 5

/**
 * 
*/

#define PREHEAT_SETPOINT 140   // Mode 1 preheat ramp value is 140-150ºC
#define SOAK_SETPOINT 150      // Mode 1 soak is 150ºC for a few seconds
#define REFLOW_SETPOINT 230    // Mode 1 reflow peak is 230ºC
#define COOLDOWN_TEMP 40       // When is ok to touch the plate!

#define HEAT_SECONDS 90
#define ERROR_SECONDS 120

#define USE_FAN_IN_HEATING true
#define FAN_IN_HEATING_RANGE 10

/**
 * 
*/

#define USE_PWM true

#if USE_PWM
    #define MIN_PID_VALUE 0
    #define MAX_PID_VALUE 160

    #define PID_Kp 1.4
    #define PID_Ki 0.0025
    #define PID_Kd 6.4
#else
    #define HEAT_HYSTERESIS 5
    #define HEAT_KOOF 10.5
#endif

/**
 *
 */

#define STATUS_NONE 0
#define STATUS_START 1
#define STATUS_PREHEAT 2
#define STATUS_WORK 3
#define STATUS_COOLING 4
#define STATUS_COMPLETE 5
#define STATUS_STOP 6
#define STATUS_ERROR 7

/**
 * Menu
 */

const char mode_1[] PROGMEM = "Reflow";
const char mode_2[] PROGMEM = "Custom";
const char mode_3[] PROGMEM = "Test";

const char* const menu[] PROGMEM = {
    mode_1,
    mode_2,
    mode_3,
};