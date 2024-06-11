#include <EEPROM.h>

void writeIntIntoEEPROM(int address, int number)
{ 
    EEPROM.write(address, number >> 8);
    EEPROM.write(address + 1, number & 0xFF);
}

int readIntFromEEPROM(int address, int def)
{
    int val = (EEPROM.read(address) << 8) + EEPROM.read(address + 1);

    return ((isnan(val) == 1 || val < 0) && def) ? def : val;
}

String secondsToHMS(const uint32_t seconds)
{
	uint32_t t = seconds;

	uint16_t s = t % 60;

	t = (t - s) / 60;
	uint16_t m = t % 60;

	// t = (t - m) / 60;
	// uint16_t h = t;

	return (m < 10 ? String(0) : "") + String(m) 
        + ":" + (s < 10 ? String(0) : "") + String(s);
}