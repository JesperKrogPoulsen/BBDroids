#include <Arduino.h>
#include "Config.h"

#include "RDisplay.h"
#include "RInput.h"

static const int CIRCLE_X = 40;
static const int TOP_CIRCLE_Y = 50;
static const int BOTTOM_CIRCLE_Y = 120;
static const int CIRCLE_RADIUS = 30;
static const int CURSOR_SIZE = 5;

#define SQ(a) ((a)*(a))
#define DEG2RAD(a) ((a)*M_PI/180.0)
#define RAD2DEG(a) ((a)*180.0/M_PI)

RDisplay RDisplay::display;

RDisplay::RDisplay():
  statusPixels_(2, P_D_NEOPIXEL, NEO_GRB+NEO_KHZ800)
#if defined(LEFT_REMOTE)
#if defined(ESP32_REMOTE)
, ser_(Serial2)
#else
, ser_(P_DISPLAY_RX, P_DISPLAY_TX) 
#endif
#endif // LEFT_REMOTE
{
  name_ = "display";
	description_ = "Display";
	help_ = "";
}

Result RDisplay::initialize() {
  pinMode(P_D_NEOPIXEL, OUTPUT);
  statusPixels_.begin();
  setLED(LED_LEFT, 0, 0, 150);
  setLED(LED_RIGHT, 150, 150, 150);
  showLEDs();

 
#if defined(LEFT_REMOTE)
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);
#endif
  
  return Subsystem::initialize();
}

Result RDisplay::start(ConsoleStream *stream) {
#if defined(LEFT_REMOTE)
  last_millis_ = millis();
  left_led_state_ = false;
  right_led_state_ = false;

  ser_.begin(921600);
  ser_.println("");
  while(ser_.available()) ser_.read(); // readEmpty
  if(sendStringAndWaitForOK("logo backlight") == false) {
    Console::console.printfBroadcast("Display failure\n");
    return RES_SUBSYS_COMM_ERROR;
  }
  Console::console.printfBroadcast("Display OK\n");

  operationStatus_ = RES_OK;
  started_ = true;

  return RES_OK;
#else
  return RES_SUBSYS_HW_DEPENDENCY_MISSING;
#endif
}
	
Result RDisplay::stop(ConsoleStream *stream) {
  Result res = RES_OK;

  if(sendStringAndWaitForOK("logo") == false) res = RES_SUBSYS_COMM_ERROR;

  started_ = false;
  return res;
}
	
Result RDisplay::step() {
  return RES_OK;
}

String RDisplay::sendStringAndWaitForResponse(const String& str, int predelay, bool nl) {
#if defined(LEFT_REMOTE)
  ser_.print(str);
  if(nl) {
    ser_.print("\n");
  }
  ser_.flush();


  if(predelay > 0) delay(predelay);

  String retval;
  if(readString(retval)) {
    return retval;
  }
#endif

  return "";
}
  
bool RDisplay::sendStringAndWaitForOK(const String& str, int predelay, bool nl) {
  String result = sendStringAndWaitForResponse(str, predelay, nl);
  //Serial.println(String("Received \"") + result + "\"");
  if(result.equals("OK.\r\n")) return true;
  return false;
}

bool RDisplay::readString(String& str, unsigned char terminator) {
#if defined(LEFT_REMOTE)
	while(true) {
		int to = 0;
		while(!ser_.available()) {
			delay(1);
			to++;
			if(to >= 1000) {
				return false;
			}
		}
		unsigned char c = (unsigned char)ser_.read();
		str += (char)c;
		if(c == terminator) return true;
	}
#else
  return false;
#endif
}

Result RDisplay::cls() {
  if(!sendStringAndWaitForOK("cls")) return RES_SUBSYS_COMM_ERROR;
  return RES_OK;
}

Result RDisplay::text(uint8_t x, uint8_t y, uint16_t color, const String& text) {
  String str = String("print ") + x + " " + y + " 0x" + String(color, HEX) + " \"" + text + "\"";
  if(!sendStringAndWaitForOK(str)) return RES_SUBSYS_COMM_ERROR; 
  return RES_OK;
}

Result RDisplay::hline(uint8_t x, uint8_t y, uint8_t width, uint16_t color) {
  String str = String("hline ") + x + " " + y + " " + width + " 0x" + String(color, HEX);
  if(!sendStringAndWaitForOK(str)) return RES_SUBSYS_COMM_ERROR;
  return RES_OK;
}

Result RDisplay::vline(uint8_t x, uint8_t y, uint8_t height, uint16_t color) {
  String str = String("vline ") + x + " " + y + " " + height + " 0x" + String(color, HEX);
  if(!sendStringAndWaitForOK(str)) return RES_SUBSYS_COMM_ERROR;
  return RES_OK;
}

Result RDisplay::line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color) {
  String str = String("line ") + x1 + " " + y1 + " " + x2 + " " + y2 + " 0x" + String(color, HEX);
  if(!sendStringAndWaitForOK(str)) return RES_SUBSYS_COMM_ERROR;
  return RES_OK;
}


Result RDisplay::rect(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint16_t color, bool filled) {
  String str;
  if(filled) str = String("rectfilled ") + x1 + " " + y1 + " " + x2 + " " + y2 + " " + " 0x" + String(color, HEX);
  else str = String("rect ") + x1 + " " + y1 + " " + x2 + " " + y2 + " " + " 0x" + String(color, HEX);
  if(!sendStringAndWaitForOK(str)) return RES_SUBSYS_COMM_ERROR;
  return RES_OK;
}

Result RDisplay::plot(uint8_t x, uint8_t y, uint16_t color) {
  String str = String("circle ") + x + " " + y + " 0 0x" + String(color, HEX);
  if(!sendStringAndWaitForOK(str)) return RES_SUBSYS_COMM_ERROR;
  return RES_OK;
}

Result RDisplay::setLED(RDisplay::WhichLED which, uint8_t r, uint8_t g, uint8_t b) {
  if(which == LED_LEFT || which == LED_BOTH) {
    statusPixels_.setPixelColor(1, r, g, b);
  } 
  if(which == LED_RIGHT || which == LED_BOTH) {
    statusPixels_.setPixelColor(0, r, g, b);
  }
  return RES_OK;
}

Result RDisplay::flashLED(WhichLED which, uint8_t iterations, uint8_t millisOn, uint8_t millisOff, uint8_t r, uint8_t g, uint8_t b) {
  for(int i=0; i<iterations; i++) {
    setLED(which, r, g, b);
    showLEDs();
    delay(millisOn);
    setLED(which, 0, 0, 0);
    showLEDs();
    delay(millisOff);
  }
  return RES_OK;
}

Result RDisplay::showLEDs() {
  statusPixels_.show();
  return RES_OK;
}
