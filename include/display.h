#include <Arduino.h>

#define USED_LIB 0 // 0 - Adafruit, 1 - GyverOLED
#define DISPLAY_TYPE 1 // 0 - SSD1306, 1 - SH1106

#if (USED_LIB == 0)
    #include <SPI.h>
    #include <Wire.h>

    #if (DISPLAY_TYPE == 0)
        #include <Adafruit_SSD1306.h>
        #define WHITE SSD1306_WHITE
    #elif (DISPLAY_TYPE == 1)
        #include <Adafruit_SH1106.h>
    #endif
#elif (USED_LIB == 1)
    #include <GyverOLED.h>
#elif (USED_LIB == 2)

#endif

// Declaration for an SSD1306 display connected to I2C (SDA, SCL pins)
// The pins for I2C are defined by the Wire-library. 
// On an arduino UNO:       A4(SDA), A5(SCL)
// On an arduino MEGA 2560: 20(SDA), 21(SCL)
// On an arduino LEONARDO:   2(SDA),  3(SCL), ...
#define OLED_RESET -1 // Reset pin # (or -1 if sharing Arduino reset pin)
#define SCREEN_WIDTH 128 // OLED display width, in pixels
#define SCREEN_HEIGHT 64 // OLED display height, in pixels
#define SCREEN_ADDRESS 0x3C // See datasheet for Address; 0x3D for 128x64, 0x3C for 128x32

class DisplayHelper {
private:
#if (USED_LIB == 0)
    #if (DISPLAY_TYPE == 0)
        Adafruit_SSD1306 display = Adafruit_SSD1306(SCREEN_WIDTH, SCREEN_HEIGHT, &Wire, OLED_RESET);
    #elif (DISPLAY_TYPE == 1)
        Adafruit_SH1106 display = Adafruit_SH1106(OLED_RESET);
    #endif
#elif (USED_LIB == 1)
    #define USE_MICRO_WIRE
    #define OLED_SPI_SPEED 4000000ul

    #if (DISPLAY_TYPE == 0)
        #if (SCREEN_HEIGHT == 64)
            GyverOLED<SSD1306_128x64, OLED_NO_BUFFER> display = (SCREEN_ADDRESS);
        #else
            GyverOLED<SSD1306_128x32, OLED_NO_BUFFER> display = (SCREEN_ADDRESS);
        #endif
    #elif (DISPLAY_TYPE == 1)
        GyverOLED<SSH1106_128x64> display = (SCREEN_ADDRESS);
    #endif
#elif (USED_LIB == 2)

#endif
public:
    DisplayHelper() {}

    bool init() {
        bool displayState = false;

        #if (USED_LIB == 0)
            #if (DISPLAY_TYPE == 0)
                displayState = display.begin(SSD1306_SWITCHCAPVCC, SCREEN_ADDRESS);
            #elif (DISPLAY_TYPE == 1)
                displayState = true;
                display.begin(SH1106_SWITCHCAPVCC, SCREEN_ADDRESS);
            #endif

            display.setTextColor(WHITE);
        #elif (USED_LIB == 1)
            displayState = true;
            display.init();
        #elif (USED_LIB == 2)

        #endif

        return displayState;
    }

    void clear() {
        #if (USED_LIB == 0)
            display.clearDisplay();
        #elif (USED_LIB == 1)
            display.clear();
        #endif
    }

    void output() {
        #if (USED_LIB == 0)
            display.display();
        #elif (USED_LIB == 1)
            display.update();
        #endif
    }

    void setCursor(int x, int y) {
        #if (USED_LIB == 0)
            display.setCursor(x, y);
        #elif (USED_LIB == 1)
            display.setCursorXY(x, y);
        #endif
    }

    void drawPixel(int x, int y) {
        #if (USED_LIB == 0)
            display.drawPixel(x, y, WHITE);
        #elif (USED_LIB == 1)
            display.dot(x, y);
        #endif
    }

    void drawLine(int x0, int y0, int x1, int y1) {
        #if (USED_LIB == 0)
            display.drawLine(x0, y0, x1, y1, WHITE);
        #elif (USED_LIB == 1)
            display.line(x0, y0, x1, y1);
        #endif
    }

    void drawFastHLine(int x, int y, int h) {
        #if (USED_LIB == 0)
            display.drawFastHLine(x, y, h, WHITE);
        #elif (USED_LIB == 1)
            display.fastLineH(y, x, h);
        #elif (USED_LIB == 2)

        #endif
    }

    void drawFastVLine(int x, int y, int w) {
        #if (USED_LIB == 0)
            display.drawFastVLine(x, y, w, WHITE);
        #elif (USED_LIB == 1)
            display.fastLineV(x, y, w);
        #elif (USED_LIB == 2)

        #endif
    }

    void invertDisplay(bool state) {
        display.invertDisplay(state);
    }

    void setTextSize(int size) {
        #if (USED_LIB == 0)
            display.setTextSize(size);
        #elif (USED_LIB == 1)
            display.setScale(size);
        #endif
    }

    size_t write(uint8_t data) { return display.write(data); }

    #if (USED_LIB == 0)
        // size_t write(const char *str) { return display.write(str); }
        // size_t write(const uint8_t *buffer, size_t size) { return display.write(buffer, size); }
    #elif (USED_LIB == 1)

    #endif

    size_t print(const __FlashStringHelper *ifsh) { return display.print(ifsh); }
    size_t print(const String &s) { return display.print(s); }
    size_t print(const char str[]) { return display.print(str); }
    size_t print(char c) { return display.print(c); }
    size_t print(unsigned char b, int base = 10) { return display.print(b, base); }
    size_t print(int num, int base = 10) { return display.print(num, base); }
    size_t print(unsigned int num, int base = 10) { return display.print(num, base); }
    size_t print(long num, int base = 10) { return display.print(num, base); }
    size_t print(unsigned long num, int base = 10) { return display.print(num, base); }
    size_t print(double num, int digits = 2) { return display.print(num, digits); }
    size_t print(const Printable& x) { return display.print(x); }

    size_t println(const __FlashStringHelper *ifsh) { return display.println(ifsh); }
    size_t println(void) { return display.println(); }
    size_t println(const String &s) { return display.println(s); }
    size_t println(const char c[]) { return display.println(c); }
    size_t println(char c) { return display.println(c); }
    size_t println(unsigned char b, int base = 10) { return display.println(b, base); }
    size_t println(int num, int base = 10) { return display.println(num, base); }
    size_t println(unsigned int num, int base = 10) { return display.println(num, base); }
    size_t println(long num, int base = 10) { return display.println(num, base); }
    size_t println(unsigned long num, int base = 10) { return display.println(num, base); }
    size_t println(double num, int digits = 2) { return display.println(num, digits); }
    size_t println(const Printable& x) { return display.println(x); }
};