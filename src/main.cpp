/**
 * ReflowMate
 */

#include <Arduino.h>
#define FPSTR(pstr) (const __FlashStringHelper*)(pstr)

//
#include <core/EncButton.h>
#include <GyverNTC.h>

// Includes
#include <constants.h>
#include <helpers.h>
#include <display.h>

//
constexpr int LINE_HEIGHT = 10;
constexpr int TOP_LINE_Y = 2;
constexpr int BOTTOM_LINE_Y = SCREEN_HEIGHT - LINE_HEIGHT;
constexpr int LINE_1_Y = TOP_LINE_Y + LINE_HEIGHT + 5;
constexpr int LINE_2_Y = LINE_1_Y + LINE_HEIGHT + 2;
constexpr int LINE_3_Y = LINE_2_Y + LINE_HEIGHT + 2;

/*
 *
 */

int runningMode = 0; // 1: Reflow, 2: Custom, 3: Test (Heat/Fan)
int modeState = STATUS_NONE;

bool heaterState = false;
bool fanState = false;

int selectedMenu = 0;

const int tempArrayMax = 30;
int tempArray[tempArrayMax];

int temperature = 0;
float currentSetpoint = 0; // Current setpoint
int customSetpoint;	// Start setpoint for custom mode

unsigned long millis_before, millis_before_2, millis_before_3;
unsigned long millis_now = 0;
float seconds = 0;
float reflowModeSeconds = 0;

constexpr float refresh_rate = 500;
constexpr float pid_refresh_rate = 500;
constexpr float graph_refresh_rate = 1000;

//
DisplayHelper display;
GyverNTC therm(THERMISTOR_PIN, 10000, 3950, 25, 10000);
EncButton encoder(ENCODER_DT_PIN, ENCODER_CLK_PIN, ENCODER_BTN_PIN, INPUT_PULLUP);

#if USE_PWM
	#include "PWMrelay.h"
	#include "GyverPID.h"

	#define PID_OPTIMIZED_I false
	#define PID_INTEGRAL_WINDOW 10

	PWMrelay relay(SSR_PIN, SSR_ON_LEVEL);
	GyverPID regulator((float)PID_Kp, (float)PID_Ki, (float)PID_Kd);
#else
	#include <GyverRelay.h>
	GyverRelay regulator(SSR_ON_LEVEL == HIGH ? REVERSE : NORMAL);
#endif

/**
 * 
 */

void fanChangeState(bool state) {
	fanState = state;

	digitalWrite(FAN_PIN, state);
}

void heaterChangeState(bool state) {
	heaterState = (state == SSR_ON_LEVEL);

	digitalWrite(SSR_PIN, state);
}

/**
 * Draw
 */

void drawTop() {
	int lineHeight = LINE_HEIGHT + 3;
	int column2_x = 30;
	int column3_x = column2_x + 20 + 10;
	int column4_x = column3_x + 20 + 10;

	display.setTextSize(1);

	if (
		modeState == STATUS_START
		|| modeState == STATUS_PREHEAT
		|| modeState == STATUS_WORK
	) {
		display.setCursor(2, TOP_LINE_Y);
		display.println(runningMode);
	}

	display.setCursor(column2_x, TOP_LINE_Y);
	display.print(F("H:"));
	display.write(heaterState ? 0x2A : 0x09); // https://learn.adafruit.com/assets/103682

	display.setCursor(column3_x, TOP_LINE_Y);
	display.print(F("F:"));
	display.write(fanState ? 0x2A : 0x09);

	display.setCursor(column4_x, TOP_LINE_Y);
	display.print(F("C:"));
	display.println(temperature ?: 0);

	display.drawFastHLine(1, lineHeight, SCREEN_WIDTH);
	display.drawFastVLine(column2_x - 5, 0, lineHeight);
	display.drawFastVLine(column3_x - 5, 0, lineHeight);
	display.drawFastVLine(column4_x - 5, 0, lineHeight);
}

void drawBottom() {
	int line_y = BOTTOM_LINE_Y - 3;
	int column2_x = 45;
	int column3_x = column2_x + 40 + 5;

	display.setTextSize(1);

	if (
		modeState == STATUS_START
		|| modeState == STATUS_PREHEAT
		|| modeState == STATUS_WORK
	) {
		display.setCursor(2, BOTTOM_LINE_Y);
		display.print(F("TS:")); 
		display.print(currentSetpoint, 0);

		if (runningMode == 1) {
			display.setCursor(column2_x, BOTTOM_LINE_Y);

			if (currentSetpoint == SOAK_SETPOINT) {
				display.print(F("SOAK")); 
			} else if (currentSetpoint == REFLOW_SETPOINT) {
				display.print(F("REFLOW")); 
			}
		}

		display.setCursor(column3_x, BOTTOM_LINE_Y);

		String secondsStr = secondsToHMS(seconds);
		display.println(secondsStr); 
	} else {
		display.setCursor(column3_x, BOTTOM_LINE_Y);
		display.print(0); 
		display.println(F("s"));
	}

	display.drawFastHLine(1, line_y, SCREEN_WIDTH);
	display.drawFastVLine(column2_x - 5, line_y, SCREEN_HEIGHT);
	display.drawFastVLine(column3_x - 5, line_y, SCREEN_HEIGHT);
}

void drawGraph()
{
	display.drawFastVLine(13, LINE_1_Y, LINE_3_Y - LINE_1_Y);
	display.drawFastHLine(13, LINE_3_Y, SCREEN_WIDTH - 20);
}

void drawTempGraph()
{
	drawGraph();

	int leftPosition = 15;

	const int plotWidth = SCREEN_WIDTH - leftPosition - 5;
	const int plotHeight = LINE_3_Y - LINE_1_Y;

	const int elementSize = round(plotWidth / tempArrayMax);

	int minVal = 400;
	int maxVal = 0;
	for (int i = 0; i < tempArrayMax; i++) {
		maxVal = max(tempArray[i], maxVal);
      	minVal = min(tempArray[i], minVal);
    }

	const int period = maxVal - minVal;
	
	float peak;
	int drawY;
	int lastDrawY = LINE_3_Y;

	for (int i = 0; i < tempArrayMax; i++) {
		peak = ((float)(tempArray[i] - minVal) / period) * 100;
		drawY = LINE_3_Y - round(plotHeight * (peak / 100));

		if (drawY <= 0 && lastDrawY > 0) {
			drawY = lastDrawY;
		}

		display.drawLine(
			leftPosition, 
			lastDrawY,
			leftPosition + elementSize,
			drawY
		);

		lastDrawY = drawY;
		leftPosition += elementSize;
	}
}

// https://tchapi.github.io/Adafruit-GFX-Font-Customiser/
void updateOLED() {
	display.clear();

	drawTop();
	drawBottom();

	display.setTextSize(1);

	if (
		runningMode == 1 // Reflow mode
		|| runningMode == 2 // Custom mode
	) {
		if (modeState == STATUS_START) {

		} else if (modeState == STATUS_PREHEAT) {
			display.setCursor(2, LINE_1_Y);
			display.write(0x18);
			display.print(F(" PREHEATED..."));
		} else if (modeState == STATUS_WORK) {
			display.setCursor(2, LINE_1_Y);
			display.write(0x10);

			drawTempGraph();
		} else if (modeState == STATUS_COOLING) {
			display.setCursor(2, LINE_1_Y);
			display.write(0x19);
			display.print(F(" COOLDOWN..."));
		} else if (modeState == STATUS_COMPLETE) {
			display.setCursor(2, LINE_1_Y);
			display.print(F("COMPLETE"));
		} else if (modeState == STATUS_STOP) {
			display.setCursor(2, LINE_1_Y);
			display.print(F("STOP..."));
		} else if (modeState == STATUS_ERROR) {
			display.setCursor(2, LINE_1_Y);
			display.print(F("ERROR"));
		}
	} 
	// Test mode
	else if (runningMode == 3 ) {
		if (modeState == STATUS_PREHEAT) {
			display.setCursor(2, LINE_1_Y);
			display.write(0x18);
			display.print(F(" HEATER TEST..."));
		} else if (modeState == STATUS_COOLING) {
			display.setCursor(2, LINE_1_Y);
			display.print("+");
			display.print(F(" HEATER TEST"));

			display.setCursor(2, LINE_2_Y);
			display.write(0x19);
			display.print(F(" FAN TEST..."));
		} else if (modeState == STATUS_COMPLETE) {
			display.setCursor(2, LINE_1_Y);
			display.print("+");
			display.print(F(" HEATER TEST"));

			display.setCursor(2, LINE_2_Y);
			display.print("+");
			display.print(F(" FAN TEST"));

			display.setCursor(2, LINE_3_Y);
			display.print(F("COMPLETE"));
		}
	}
	// Service mode
	else { 
		if (modeState == STATUS_NONE) { 
			display.setCursor(2, LINE_1_Y + round(LINE_HEIGHT / 1.8));
			display.setTextSize(2);

			if (selectedMenu == 0) {
				display.print(F("- Select -"));
			} else {
				display.print(F("- "));
				display.print(FPSTR((PGM_P)pgm_read_ptr(&menu[selectedMenu - 1])));
			}
		}
	}

	display.output();
}

/**
 * 
 */

void storeTemp(int temp)
{
	static int i = 0;

	if (isnan(temp)) {
		if (i < tempArrayMax) {
			tempArray[i] = 0;
			i++;
		} else {
			for (int j = 0; j < tempArrayMax - 1; j++) {
				tempArray[j] = tempArray[j + 1];
				tempArray[tempArrayMax - 1] = 0;
			}
		}
	} else {
		if (i < tempArrayMax) {
			tempArray[i] = temp;
			i++;
		} else {
			for (int j = 0; j < tempArrayMax - 1; j++) {
				tempArray[j] = tempArray[j + 1];
				tempArray[tempArrayMax - 1] = temp;
			}
		}
	}
}

void clearTemperatureArray() {
	for (int i = 0; i < tempArrayMax; i++) {
		tempArray[i] = 0;
	}
}

/**
 * 
 */

void setStatus(int status) {
	if (status == STATUS_START) {
		heaterChangeState(SSR_ON_LEVEL);
		fanChangeState(LOW);

		tone(BUZZER_PIN, 2000, 150);
		delay(130);
		tone(BUZZER_PIN, 2200, 150);
		delay(130);
		tone(BUZZER_PIN, 2400, 150);
		delay(130);

		seconds = 0;
		reflowModeSeconds = 0;
	} else if (status == STATUS_PREHEAT) {
		heaterChangeState(SSR_ON_LEVEL);
		fanChangeState(LOW);
	} else if (status == STATUS_WORK) {
		
	} else if (status == STATUS_COOLING) {
		heaterChangeState(SSR_OFF_LEVEL);
		fanChangeState(HIGH);
		
		currentSetpoint = 0;
	} else if (status == STATUS_COMPLETE) {
		heaterChangeState(SSR_OFF_LEVEL);
		fanChangeState(LOW);

		tone(BUZZER_PIN, 1800, 1000);
	} else if (status == STATUS_NONE) {
		runningMode = 0;
		seconds = 0;
		reflowModeSeconds = 0;

		heaterChangeState(SSR_OFF_LEVEL);
		fanChangeState(LOW);

		clearTemperatureArray();
	} else if (status == STATUS_STOP) {
		heaterChangeState(SSR_OFF_LEVEL);
		fanChangeState(HIGH);

		if (runningMode == 2) {
			writeIntIntoEEPROM(2, customSetpoint); 
		}

		tone(BUZZER_PIN, 2500, 150);
		delay(130);
		tone(BUZZER_PIN, 2200, 150);
		delay(130);
		tone(BUZZER_PIN, 2000, 150);
		delay(130);
	} else if (status == STATUS_ERROR) {
		heaterChangeState(SSR_OFF_LEVEL);
		digitalWrite(FAN_PIN, HIGH);

		currentSetpoint = 0;
	}

	modeState = status;
	updateOLED();
}

void encoderTurn(bool isHigh) {
	if (modeState == STATUS_NONE) {
		if (isHigh) {
			selectedMenu++;
		} else {
			selectedMenu--;
		}

		tone(BUZZER_PIN, 2300, 40);

		const int menuLength = sizeof(menu) / sizeof(menu[0]);
		if (selectedMenu > menuLength) {
			selectedMenu = 0;
		} else if (selectedMenu < 0) {
			selectedMenu = menuLength;
		}
	} else if (
		modeState == STATUS_PREHEAT
		|| modeState == STATUS_WORK
	) {
		if (runningMode == 2) {
			if (isHigh) {
				customSetpoint++;
			} else {
				customSetpoint--;
			}
		}
	}
}

void buttonPress() {
	if((selectedMenu == 1 || selectedMenu == 2) && modeState == STATUS_NONE) { // Work Mode
		runningMode = selectedMenu;

		setStatus(STATUS_START);
	} else if (selectedMenu == 3) { // Test Mode
		runningMode = selectedMenu;

		if (modeState == STATUS_NONE) {
			setStatus(STATUS_PREHEAT);
		} else if (modeState == STATUS_PREHEAT) {
			setStatus(STATUS_COOLING);
		} else if (modeState == STATUS_COOLING) {
			setStatus(STATUS_STOP);
			delay(3000);
		}
	}
}

void buttonLongPress()
{
	if (
		modeState == STATUS_START 
		|| modeState == STATUS_PREHEAT 
		|| modeState == STATUS_WORK
	) { // Break work
		setStatus(STATUS_STOP);
		delay(3000);
	}
}

void readEncoderStatus() {
	if (encoder.tick()) {
		switch (encoder.action()) {
			case EB_PRESS:
				buttonPress();
				break;
			case EB_HOLD:
				buttonLongPress();
				break;
			case EB_TURN:
				encoderTurn(encoder.dir() == 1);
				break;
		}
	}
}

/**
 * 
 */

void temperatureMonitor() {
	if ((millis_now - millis_before_2) > pid_refresh_rate) {
		millis_before_2 = millis();
		temperature = therm.getTempAverage(20);

		if (runningMode == 0) {
			fanChangeState(temperature > COOLDOWN_TEMP);
		}

		if (
			modeState == STATUS_START 
			|| modeState == STATUS_PREHEAT 
			|| modeState == STATUS_WORK
		) {
			// Reflow Mode
			if (runningMode == 1) {
				reflowModeSeconds = reflowModeSeconds + (pid_refresh_rate / 1000);

				if (
					modeState == STATUS_START
					|| modeState == STATUS_PREHEAT
				) { 
					if (temperature < PREHEAT_SETPOINT) {
						if (currentSetpoint < PREHEAT_SETPOINT) {
							currentSetpoint = reflowModeSeconds * 1.555; // Reach 140ºC till 90s (150 / 90 = 1.555)
						}
						
						if (modeState == STATUS_START) {
							setStatus(STATUS_PREHEAT);
						} else if (reflowModeSeconds >= ERROR_SECONDS) {
							setStatus(STATUS_ERROR);
							delay(3000);
						}
					} else {
						currentSetpoint = SOAK_SETPOINT;
						reflowModeSeconds = 0;

						setStatus(STATUS_WORK);
					}
				}
				else if (modeState == STATUS_WORK) {
					if (currentSetpoint == SOAK_SETPOINT) { 
						if (temperature >= SOAK_SETPOINT && reflowModeSeconds >= HEAT_SECONDS) {
							currentSetpoint = REFLOW_SETPOINT;
							reflowModeSeconds = 0;
						}
						else if (temperature < SOAK_SETPOINT && reflowModeSeconds >= ERROR_SECONDS) {
							setStatus(STATUS_ERROR);
							delay(3000);
						}
					}
					else if (currentSetpoint == REFLOW_SETPOINT) { 
						if (temperature >= REFLOW_SETPOINT && reflowModeSeconds >= HEAT_SECONDS) {
							currentSetpoint = 0;
							reflowModeSeconds = 0;

							setStatus(STATUS_COOLING);
						}
						else if (temperature < REFLOW_SETPOINT && reflowModeSeconds >= ERROR_SECONDS) {
							setStatus(STATUS_ERROR);
							delay(3000);
						}
					}
				}
			}
			// Custom Mode
			else if (runningMode == 2) {
				currentSetpoint = customSetpoint;

				if (temperature < customSetpoint) {
					if (modeState == STATUS_START) {
						setStatus(STATUS_PREHEAT);
					}
				} else {
					if (
						modeState == STATUS_START
						|| modeState == STATUS_PREHEAT
					) {
						setStatus(STATUS_WORK);
					}
				}
			}
			// Test Mode
			else if (runningMode == 3) {
				if (modeState == STATUS_PREHEAT) {
					currentSetpoint = 220;
				}
			}

			regulator.setpoint = currentSetpoint;
			regulator.input = temperature;

			#if USE_PWM
				pidtype pid = regulator.getResultTimer();
				relay.setPWM((temperature < currentSetpoint) ? pid : 0);

				if (IS_DEBUG) {
					Serial.print(F("temperature: "));
					Serial.println(temperature);

					Serial.print(F("PID: "));
					Serial.println(pid);
				}
			#else
				bool state = regulator.compute(2000);
				heaterChangeState(state);

				if (IS_DEBUG) {
					Serial.print(temperature);
					Serial.print(' ');
					Serial.print(regulator.setpoint - regulator.hysteresis / 2);
					Serial.print(' ');
					Serial.print(regulator.setpoint + regulator.hysteresis / 2);
					Serial.print(' ');
					Serial.println(regulator.setpoint);
				}
			#endif

			fanChangeState(temperature > (currentSetpoint + 5));
		}

		if (
			modeState == STATUS_COOLING
			|| modeState == STATUS_STOP
			|| modeState == STATUS_ERROR
		) { // Before complete
			if (temperature < COOLDOWN_TEMP) {
				setStatus(STATUS_COMPLETE);
				delay(3000);
			}
		}

		if (modeState == STATUS_COMPLETE) { // Complete
			setStatus(STATUS_NONE);
		}
	}

	if (
		modeState == STATUS_START 
		|| modeState == STATUS_PREHEAT 
		|| modeState == STATUS_WORK
	) {
		if (
			runningMode == 1 
			|| runningMode == 2
			|| runningMode == 3
		) {
			#if USE_PWM
				relay.tick();   
			#endif
		}
	}

	if (runningMode == 1 || runningMode == 2) {
		if ((millis_now - millis_before_3) > graph_refresh_rate) {
			millis_before_3 = millis();

			if (modeState != STATUS_NONE) {
				storeTemp(temperature);
			}
		}
	}
}

void setup() {
	//
	if (IS_DEBUG) {
		Serial.begin(115200);
		while (!Serial); // Wait for serial monitor
	}

	// OLED init
	if (!display.init()) {
		if (IS_DEBUG) {
			Serial.println(F("DISPLAY allocation failed"));
		}

		for(;;); // Don't proceed, loop forever
	}

	//
	display.clear();
	display.output();

	// Encoder
	encoder.setEncType(EB_STEP2);

	// Pid
	#if USE_PWM
		regulator.setLimits(MIN_PID_VALUE, MAX_PID_VALUE);
	#else
		regulator.hysteresis = (float)HEAT_HYSTERESIS;
		regulator.k = (float)HEAT_KOOF; 
	#endif

	// Heater
	pinMode(SSR_PIN, OUTPUT);
	heaterChangeState(SSR_OFF_LEVEL);

	// Fan
	pinMode(FAN_PIN, OUTPUT);
	fanChangeState(LOW);

	// Buzzer
	pinMode(BUZZER_PIN, OUTPUT);
	digitalWrite(BUZZER_PIN, LOW);

	// Thermistor
	pinMode(THERMISTOR_PIN, INPUT);

	// Init buzz
	tone(BUZZER_PIN, 1800, 200);

	// EEPROM
	customSetpoint = readIntFromEEPROM(2, 180);
	if (customSetpoint > 300) {
		customSetpoint = 300;
	}

	if (IS_DEBUG) {
		Serial.print(F("CustomSetpoint: "));
		Serial.println(customSetpoint);
	}

	//
	millis_before = millis();
	millis_now = millis();

	// Initialise arrays
	clearTemperatureArray();
}

void loop() {
	millis_now = millis();

	temperatureMonitor();
	readEncoderStatus();

	if((millis_now - millis_before) > refresh_rate) {
		millis_before = millis();
		seconds = seconds + (refresh_rate / 1000);

		updateOLED();
	}
}
