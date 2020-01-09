#include <Arduino.h>

#include "WiFi.h"
#include "SPIFFS.h"
#include "AsyncTCP.h"
#include "ESPAsyncWebServer.h"
#include "ESPmDNS.h"
#include <TimeLib.h>
#include "DHT.h"
#include <OneWire.h>
#include <DallasTemperature.h>

// https://github.com/espressif/arduino-esp32

// Stable release link: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json
// Development release link: https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_dev_index.json

// https://platformio.org/platformio-ide
// https://docs.platformio.org/en/latest/platforms/espressif32.html
// https://docs.platformio.org/en/latest/platforms/espressif32.html?utm_source=github&utm_medium=arduino-esp32
// https://docs.platformio.org/en/latest/platforms/espressif32.html?utm_source=github&utm_medium=arduino-esp32#examples

// Global
const char* hostname = "esp32-01";

bool WiFiClient  = true;
bool WiFiAccessPoint  = true;
bool WiFiSearchMyNetwork = false;

bool LEDenabled = true;
bool DSenabled = true;
bool DHTenabled = true;
bool WLANenabled = true;
bool WEBenabled = true;

String srcPrefix = "http://www.solopix.at/esp32/";

// ESP32
bool shouldReboot = false;

// LOG - https://de.wikipedia.org/wiki/Syslog
String logFunction = "";
const int logSeverityEmergency = 0;
const int logSeverityAlert = 1;
const int logSeverityCritical = 2;
const int logSeverityError = 3;
const int logSeverityWarning = 4;
const int logSeverityNotice = 5;
const int logSeverityInformational = 6;
const int logSeverityDebug = 7;
int64_t getMillisOfDay() {
	struct timeval tv;
	gettimeofday(&tv, NULL);
	return (tv.tv_sec * 1000LL + (tv.tv_usec / 1000LL));
}
String getTimeStamp() {
  char DateAndTimeString[20] = {0};
  struct tm timeinfo;
  int64_t MillisOfDay = getMillisOfDay();
  int64_t SecondsOfDay = MillisOfDay / 1000;
  int MillisOfSecond = MillisOfDay - (SecondsOfDay * 1000);
  char MillisOfSecondString[5] = {0};
  sprintf(MillisOfSecondString, "%03d", MillisOfSecond);
  if (!getLocalTime(&timeinfo)) { // Failed to obtain time
    time_t t = now(); 
    sprintf(DateAndTimeString, PSTR("%4d-%02d-%02d %02d:%02d:%02d"), year(t), month(t), day(t), hour(t), minute(t), second(t));
  } else {
    strftime(DateAndTimeString, sizeof(DateAndTimeString), "%Y-%m-%d %H:%M:%S", &timeinfo);
  }
  String returnTimeStamp=DateAndTimeString;
  returnTimeStamp += "." + String(MillisOfSecondString);
  return returnTimeStamp;
}
void logMessage(String function, int severity, String message) {
  String seperator = "\t";
  Serial.print(getTimeStamp()); //  + "." + String(millis())
  Serial.print(seperator);
  Serial.print(hostname);
  Serial.print(seperator);
  switch (severity) {
    case logSeverityEmergency:
      Serial.print("Emergency");
      break;
    case logSeverityAlert:
      Serial.print("Alert");
      break;
    case logSeverityCritical:
      Serial.print("Critical");
      break;
    case logSeverityError:
      Serial.print("Error");
      break;
    case logSeverityWarning:
      Serial.print("Warning");
      break;
    case logSeverityNotice:
      Serial.print("Notice");
      break;
    case logSeverityInformational:
      Serial.print("Info "); // !!!
      break;
    case logSeverityDebug:
      Serial.print("Debug");
      break;
    default:
      Serial.print("unknown");
      break;
  }
  Serial.print(seperator);
  Serial.print(function);
  Serial.print(seperator);
  Serial.println(message);
}

// WiFi - https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/scan-class.html
//        https://arduino-esp8266.readthedocs.io/en/latest/esp8266wifi/scan-class.html
//        https://randomnerdtutorials.com/micropython-wi-fi-manager-esp32-esp8266/
char WiFiCLssid[] = "wapsm4";
String WiFiCLip = "";
String WiFiCLmac = "";
char WiFiAPssid[] = "wapesp32-01";
String WiFiAPip = "";
String WiFiAPmac = "";
const char* WiFiCLpassword =  "espressif32";
const char* WiFiAPpassword =  "espressif32";
String WiFiCLssidPrefix = "wapsm";
bool WiFiCLconnected = false;
bool WiFiAPinitialized = false;
#define WLAN_MAX_NETWORKS 255
int WLANcount = 0;
String WLANrssi[WLAN_MAX_NETWORKS] = {};
String WLANssid[WLAN_MAX_NETWORKS] = {};
String WLANbssid[WLAN_MAX_NETWORKS] = {};
String WLANchannel[WLAN_MAX_NETWORKS] = {};
String WLANsecure[WLAN_MAX_NETWORKS] = {};
bool   WLANhidden[WLAN_MAX_NETWORKS] = {};
String WLANtimestamp = "";
void SortWlanNetworks() {
  bool sortiert = false;
  String tmpWLANrssi = "";
  String tmpWLANssid = "";
  String tmpWLANbssid = "";
  String tmpWLANchannel = "";
  String tmpWLANsecure = "";
  bool   tmpWLANhidden = false;  
  while (sortiert == false) {
    sortiert = true;
    for (int i = 1; i < WLANcount; ++i) {
      if (WLANssid[i] > WLANssid[i + 1]) {
        sortiert = false;
        tmpWLANrssi = WLANrssi[i];
        tmpWLANssid = WLANssid[i];
        tmpWLANbssid = WLANbssid[i];
        tmpWLANchannel = WLANchannel[i];
        tmpWLANsecure = WLANsecure[i];
        tmpWLANhidden = WLANhidden[i];  
        WLANrssi[i] = WLANrssi[i + 1];
        WLANssid[i] = WLANssid[i + 1];
        WLANbssid[i] = WLANbssid[i + 1];
        WLANchannel[i] = WLANchannel[i + 1];
        WLANsecure[i] = WLANsecure[i + 1];
        WLANhidden[i] = WLANhidden[i + 1];  
        WLANrssi[i + 1] = tmpWLANrssi;
        WLANssid[i + 1] = tmpWLANssid;
        WLANbssid[i + 1] = tmpWLANbssid;
        WLANchannel[i + 1] = tmpWLANchannel;
        WLANsecure[i + 1] = tmpWLANsecure;
        WLANhidden[i + 1] = tmpWLANhidden;  
      }
    }
  }
}
String GetMyNetwork() {
  String myNetwork = "";
  String ssid = "";
  int n = WiFi.scanNetworks(false, true);
  int i = 0;
  while (i < n && myNetwork == "") {
    ssid = WiFi.SSID(i);
    if (ssid.startsWith(WiFiCLssidPrefix)) {
      myNetwork = ssid;
      logMessage("WiFi",logSeverityInformational,"Found network: " + myNetwork);
    }
    i += 1;
  }
  if (myNetwork == "") {
    logMessage("WiFi",logSeverityError,"Network (ssid starting with \'" + WiFiCLssidPrefix + "\') not found");
  }
  return myNetwork;
}
bool GetWlanNetworksSync() {
  WLANcount = 0;
  String ssid;
  uint8_t encryptionType;
  int32_t RSSI;
  uint8_t* BSSID;
  int32_t channel;
  // bool isHidden;
  int n = WiFi.scanNetworks(false, true);
  logMessage("WiFi",logSeverityDebug,"Found " + String(n) + " networks");
  for (int i = 0; i < n; ++i) {
    WiFi.getNetworkInfo(i,ssid,encryptionType,RSSI,BSSID,channel);
    WLANcount += 1;
    WLANrssi[WLANcount] = String(WiFi.RSSI(i));
    WLANssid[WLANcount] = WiFi.SSID(i);
    WLANbssid[WLANcount] = WiFi.BSSIDstr(i);
    WLANchannel[WLANcount] = String(WiFi.channel(i));
    WLANsecure[WLANcount] = String(WiFi.encryptionType(i));
    // WLANhidden[WLANcount] = WiFi.isHidden(i);
    WLANhidden[WLANcount] = false;
    logMessage("WiFi",logSeverityDebug,"Found " + WiFi.SSID(i));
  }
  SortWlanNetworks();
  WLANtimestamp = getTimeStamp();
  return true;
}
String GetWlanNetworksJson() {
  String json = "[";
  if (GetWlanNetworksSync()) {
    for (int i = 1; i <= WLANcount; ++i) {
      if (i) json += ",";
      json += "{";
      json += "\"rssi\":" + WLANrssi[i];
      json += ",\"ssid\":\"" + WLANssid[i] + "\"";
      json += ",\"bssid\":\"" + WLANbssid[i] + "\"";
      json += ",\"channel\":" + WLANchannel[i];
      json += ",\"secure\":" + WLANsecure[i];
      json += ",\"hidden\":"+String(WLANhidden[i]?"true":"false");
      json += "}";
    }
  }
  json += "]";
  return json;
}

// NTP - https://lastminuteengineers.com/esp32-ntp-server-date-time-tutorial/
const char* ntpServer = "pool.ntp.org";
const long  gmtOffset_sec = 3600;
const int   daylightOffset_sec = 3600;

// SPIFFS - https://randomnerdtutorials.com/esp8266-web-server-spiffs-nodemcu/

// LEDs - rot:  Led1.de - WEBRD01-IM - 3mm diffused short Flat Top red DIP LED    - 2,1V/20mA - 3,3V: 47 Ω - 5V: 150 Ω
//        gelb: Led1.de - WEBYE01-IM - 3mm diffused short Flat Top yellow DIP LED - 2,1V/20mA - 3,3V: 47 Ω - 5V: 150 Ω
//        grün: Led1.de - WEBGN01-IM - 3mm diffused short Flat Top green DIP LED  - 3,2V/20mA - 3,3V:  4 Ω - 5V:  91 Ω
//        Ampel - https://www.wien.gv.at/verkehr/ampeln/ampelkunde.html
//                https://de.wikipedia.org/wiki/Ampel
//                https://starthardware.org/ampel-mit-arduino-fuer-autos-und-fussgaenger/
#define LEDcount 3
int LEDpin[LEDcount] = {18, 19, 21};
char LEDstatus[LEDcount] = {LOW, LOW, LOW};
char LEDmode[LEDcount] = {0, 0, 0};
bool LEDblink[LEDcount] = {false, false, false};
const char ledModeManual = 0;
const char ledModeBlink = 1;
const char ledModeTrafficlight  = 2;
int ledBlinkInterval = 500;
int ledLastTime = 0;
bool ledTrafficLights = false;
int tlLastTime = 0;
int tlPhase = -1;
char tlPhases[10][3] = {{HIGH, LOW,  LOW},  // rot
                        {HIGH, HIGH, LOW},  // rot-gelb
                        {LOW,  LOW,  HIGH},  // grün
                        {LOW,  LOW,  LOW},  // grün blinkend - aus
                        {LOW,  LOW,  HIGH},  // grün blinkend - ein
                        {LOW,  LOW,  LOW},  // grün blinkend - aus
                        {LOW,  LOW,  HIGH},  // grün blinkend - ein
                        {LOW,  LOW,  LOW},  // grün blinkend - aus
                        {LOW,  LOW,  HIGH},  // grün blinkend - ein
                        {LOW,  HIGH, LOW}   // gelb
                       };
int tlTimes[10] = {5000, 2500, 5000, 650, 650, 650, 650, 650, 650, 3500};
void led_toggle(int pin, char &status) {
  if (status == LOW) {
    digitalWrite(pin, HIGH);
    logMessage("GPIO",logSeverityDebug,"GPIO-" + String(pin) + " Status: ON");
    status = HIGH;
  } else {
    digitalWrite(pin, LOW);
    logMessage("GPIO",logSeverityDebug,"GPIO-" + String(pin) + " Status: OFF");
    status = LOW;
  }
}
void led_on(int pin, char &status) {
  if (digitalRead(pin) != HIGH) {
    digitalWrite(pin, HIGH);
    logMessage("GPIO",logSeverityDebug,"GPIO-" + String(pin) + " Status: ON");
  }
  status = HIGH;
}
void led_off(int pin, char &status) {
  if (digitalRead(pin) != LOW) {
    digitalWrite(pin, LOW);
    logMessage("GPIO",logSeverityDebug,"GPIO-" + String(pin) + " Status: OFF");
  }
  status = LOW;
}
void led_set(int pin, char &status, char LEDbit) {
  switch (LEDbit) {
    case HIGH:
      led_on(pin, status);
      break;
    case LOW:
      led_off(pin, status);
      break;
  }
}

// OneWire DSB1820 - https://randomnerdtutorials.com/esp32-ds18b20-temperature-arduino-ide/
//                   https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
//                   https://randomnerdtutorials.com/dht11-vs-dht22-vs-lm35-vs-ds18b20-vs-bme280-vs-bmp180/
//                   Connect pin 1 (on the left) of the sensor to GROUND
//                   Connect pin 2 to ONE_WIRE_BUS (PU-resistor: 3.3V: 4.7k / 5V: 10k)
//                   Connect pin 3 (on the right) to +3.3V
//                   Temperature range: -55 to 125°C
#define ONE_WIRE_BUS 4                      // GPIO-pin
OneWire oneWire(ONE_WIRE_BUS);              // OneWire instance to communicate with any OneWire devices
DallasTemperature oneWireSensors(&oneWire); // OneWire reference > Dallas Temperature sensor
int OneWireDevicesCount;
DeviceAddress tempDeviceAddress;
int DScount = 0;
String DSaddr[32] = {};
float  DStemp[32] = {};
String DStimestamp = "";
bool dsQuery = false;
const int dsQueryInterval = 10000;
int dsLastTime = 0;
String getOneWireDeviceAddress(DeviceAddress deviceAddress) {
  char devaddr[24] = {0};
  sprintf(devaddr,"%02X-%02X-%02X-%02X-%02X-%02X-%02X-%02X",deviceAddress[0],deviceAddress[1],deviceAddress[2],deviceAddress[3],deviceAddress[4],deviceAddress[5],deviceAddress[6],deviceAddress[7]);
  return devaddr;
}
String uint64ToString(uint64_t input) {
  String result = "";
  uint8_t base = 10;
  do {
    char c = input % base;
    input /= base;
    if (c < 10)
      c +='0';
    else
      c += 'A' - 10;
    result = c + result;
  } while (input);
  return result;
}

// DHT11/DHT22 - https://randomnerdtutorials.com/esp32-dht11-dht22-temperature-humidity-sensor-arduino-ide/
//               https://randomnerdtutorials.com/esp32-multiple-ds18b20-temperature-sensors/
//               https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
//               https://randomnerdtutorials.com/dht11-vs-dht22-vs-lm35-vs-ds18b20-vs-bme280-vs-bmp180/
//               Connect pin 1 (on the left) of the sensor to +3.3V
//               Connect pin 2 to DHTPIN (PU-resistor: 3.3V: 4.7k / 5V: 10k)
//               Connect pin 4 (on the right) to GROUND
//               Temperature range: DHT11: 0 to 50°C, DHT22: -40 to 80°C
#define DHTPIN 5      // GPIO-pin
#define DHTTYPE DHT22 // DHT11 or DHT22 or DHT21
DHT dht(DHTPIN, DHTTYPE);
float DHTh = 0;
float DHTt = 0;
String DHTtimestamp = "";
bool dhtQuery = false;
const int dhtQueryInterval = 30000;
int dhtLastTime = 0;

// BME280 -      https://randomnerdtutorials.com/esp32-web-server-with-bme280-mini-weather-station/
//               https://randomnerdtutorials.com/esp32-pinout-reference-gpios/
//               https://randomnerdtutorials.com/dht11-vs-dht22-vs-lm35-vs-ds18b20-vs-bme280-vs-bmp180/
//               https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
//               I2C:        SDI (SDA): GPIO 21
//                           SCK (SCL): GPIO 22
//               SPI:  SCK (SPI Clock): GPIO 18
//                          SDO (MISO): GPIO 19
//                          SDI (MOSI): GPIO 23
//                    CS (Chip Select): GPIO 5
// #define SEALEVELPRESSURE_HPA (1013.25)
// Adafruit_BME280 bme; // I2C
// Adafruit_BME280 bme(BME_CS); // hardware SPI
// Adafruit_BME280 bme(BME_CS, BME_MOSI, BME_MISO, BME_SCK); // software SPI
// #define BME_SCK 18
// #define BME_MISO 19
// #define BME_MOSI 23
// #define BME_CS 5

// I2C - https://randomnerdtutorials.com/esp32-i2c-communication-arduino-ide/
//       SDA: GPIO 21
//       SCL: GPIO 22
// #define I2C_SDA 21 // 27 // 33
// #define I2C_SCL 22 // 26 // 32


// WebServer - https://github.com/me-no-dev/ESPAsyncWebServer
//             https://libraries.io/github/me-no-dev/ESPAsyncWebServer
//             https://gitter.im/me-no-dev/ESPAsyncWebServer
//             https://randomnerdtutorials.com/esp8266-web-server-spiffs-nodemcu/
// 
//             http://192.168.1.86
//             http://192.168.1.86/ptr
//             http://192.168.1.86/test
//
//             Restart Prolem - https://github.com/me-no-dev/ESPAsyncWebServer/issues/544
//                              https://github.com/espressif/arduino-esp32/issues/922
//                              https://rntlab.com/question/error-task-watchdog-got-triggered-2/
//                              evtl.: Setup global and class functions as request handlers - https://github.com/me-no-dev/ESPAsyncWebServer#setup-global-and-class-functions-as-request-handlers

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");           // access at ws://[esp ip]/ws
AsyncEventSource events("/events"); // event source (Server-Sent events)
const char* http_username = "admin";
const char* http_password = "admin";
// Handle Unknown Request
void onRequest(AsyncWebServerRequest *request) {
  Serial.print(request->url());
  Serial.print(" requested ");
  Serial.println("- 404 - not found");
  request->send(SPIFFS, "/404.html", "text/html"); // request->send(404);
}
// Handle body
void onBody(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
  Serial.println("onBody");
}
// Handle upload
void onUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final) {
  Serial.println("onUpload");
}
// Handle WebSocket event
void onEvent(AsyncWebSocket * server, AsyncWebSocketClient * client, AwsEventType type, void * arg, uint8_t *data, size_t len) {
  Serial.println("onEvent");
}
// List all request parameter
void RequestParameter(AsyncWebServerRequest *request) {
  int params = request->params();
  for (int i = 0; i < params; i++) {
    AsyncWebParameter* p = request->getParam(i);
    if (p->isFile()) { //p->isPost() is also true
      Serial.printf("FILE[%s]: %s, size: %u\n", p->name().c_str(), p->value().c_str(), p->size());
    } else if (p->isPost()) {
      Serial.printf("POST[%s]: %s\n", p->name().c_str(), p->value().c_str());
    } else {
      Serial.printf("GET[%s]: %s\n", p->name().c_str(), p->value().c_str());
    }
  }
}
// HTML of /
String  GetHTML(String SPIFFSPrefix) {
  String title = "ESP32 Web Server";
  String h = "<!DOCTYPE html>\n";
  h += "<html>\n";
  h += "<head>\n";
  h += " <title>" + title + "</title>\n";
  h += " <script src=\"" + SPIFFSPrefix + "src/jquery-3.4.1.min.js\"></script>\n";
  h += " <script src=\"" + SPIFFSPrefix + "src/bootstrap.min.js\"></script>\n";
  h += " <link rel=\"stylesheet\" type=\"text/css\" href=\"" + SPIFFSPrefix + "src/bootstrap.min.css\">\n";
  h += " <link rel=\"stylesheet\" type=\"text/css\" href=\"" + SPIFFSPrefix + "src/my.css\">\n";
  h += " <link rel=\"shortcut icon\" type=\"image/x-icon\" href=\"favicon.ico\" />\n";
  h += " <link rel=\"icon\" type=\"image/x-icon\" href=\"favicon.ico\" />\n";
  h += "</head>\n";
  h += "<body style=\"background-color: #e9ecef;\">\n"; // calc(1rem + 0.5vw);
  h += "<div id=\"divAjaxLoader\" style=\"display: none; position: absolute; left: 50%; top: 50%; z-index: 999\">\n";
  h += " <img id=\"imgAjaxLoader\" src=\"" + SPIFFSPrefix + "src/ajax_loader.gif\" width=\"36\" height=\"36\"alt=\"\" >\n";
  h += "</div>\n";
  h += "<div id=\"divScreen\">\n";
  h += "<div class=\"jumbotron jumbotron-fluid\" style=\"padding-top:0px;padding-bottom:0px;\">\n";
  h += " <div class=\"container-fluid\">\n";
  h += "  <div class=\"d-flex justify-content-between\">\n";
  h += "   <div>\n";
  h += "    <h1 class=\"display-2\">" + title + "</h1>\n";
  h += "    <p class=\"lead\">AsyncWebServer with Bootstrap.</p>\n";
  h += "   </div>\n";
  h += "   <div>\n";
  h += "    <img src=\"" + SPIFFSPrefix + "src/esp32a.png\" class=\"float-right my-pic-header\" alt=\"ESP32\">\n";
  h += "   </div>\n";
  h += "  </div>\n";
  h += "  <hr class=\"my-4\" style=\"margin-top: 0.2rem !important; margin-bottom: 0.2rem !important;\">\n";
  // Control Buttons
  h += "  <div class=\"my-div-full my-div-full-right\">\n";
  h += "   <button type=\"button\" class=\"btn btn-md btn-outline-dark my-btn-5 my-btn-pic my-btn-pic-5 my-btn-5-gap\" onclick=\"javascript:__doPostBack('Refresh', 'true');\">\n";
  h += "    <img src=\"" + SPIFFSPrefix + "src/refresh.png\" alt=\"refresh\">\n";
  h += "    <div>REFRESH</div>";
  h += "   </button>";
  h += "   <button type=\"button\" class=\"btn btn-md btn-outline-dark my-btn-5 my-btn-pic my-btn-pic-5\" onclick=\"javascript:__doPostBack('Restart', 'true');\">\n";
  h += "    <img src=\"" + SPIFFSPrefix + "src/power_blue.png\" alt=\"restart\">\n";
  h += "    <div>RESTART</div>";
  h += "   </button>";
  h += "  </div>";
  // System Information
  h += "  <p class=\"my-p-header\" style=\"margin-top:-30px;\">System Information:</p>\n";
  h += "  <div class=\"my-div-full\">\n";
  h += "   <table class=\"table table-sm table-bordered\" style=\"width: auto; margin-top: 1rem;\">\n";
  h += "    <tr>\n";
  h += "     <td class=\"macid\" style=\"text-align:right\">Hostname:</td>\n";
  h += "     <td class=\"macid\" style=\"text-align:left\">" + String(hostname) + "</td>\n";
  h += "    </tr>\n";
  h += "    <tr>\n";
  h += "     <td class=\"macid\" style=\"text-align:right\">Date/Time:</td>\n";
  h += "     <td class=\"macid\" style=\"text-align:left\">" + getTimeStamp() + "</td>\n";
  h += "    </tr>\n";
  h += "    <tr>\n";
  h += "     <td class=\"macid\" style=\"text-align:right\">WiFi Client:</td>\n";
  if (WiFiCLconnected == true) {
    h += "     <td class=\"macid\" style=\"text-align:left\">SSID: " + String(WiFiCLssid) + "<br />&nbsp;&nbsp;IP: " + WiFiCLip + "<br />&nbsp;MAC: " + WiFiCLmac + "</td>\n";
  } else {
    h += "     <td class=\"macid\" style=\"text-align:left\"></td>\n";
  }
  h += "    </tr>\n";
  h += "    <tr>\n";
  h += "     <td class=\"macid\" style=\"text-align:right\">WiFi AccessPoint:</td>\n";
  if (WiFiAPinitialized == true) {
    h += "     <td class=\"macid\" style=\"text-align:left\">SSID: " + String(WiFiAPssid) + "<br />&nbsp;&nbsp;IP: " + WiFiAPip + "<br />&nbsp;MAC: " + WiFiAPmac + "</td>\n";
  } else {
    h += "     <td class=\"macid\" style=\"text-align:left\"></td>\n";
  }
  h += "    </tr>\n";
  h += "   </table>\n";
  h += "  </div>";
  // LED's
  if (LEDenabled == true) {
    h += "  <p class=\"my-p-header\" style=\"margin-top:-30px;\">LED's</p>\n";
    h += "  <div class=\"my-div-mobile\">\n";
    h += "   <button type=\"button\" class=\"btn btn-lg btn-outline-dark my-btn-3 my-btn-pic my-btn-pic-3 my-btn-3-gap\" onclick=\"javascript:__doPostBack('Led1', 'toggle');\">\n";
    if (LEDstatus[0] == HIGH || LEDmode[0] == ledModeBlink || ledTrafficLights == true) {
      h += "    <img src=\"" + SPIFFSPrefix + "src/led_red.png\" alt=\"red led\">\n";
    } else {
      h += "    <img src=\"" + SPIFFSPrefix + "src/led_grey.png\" alt=\"grey led\">\n";
    }
    h += "    <div>GPIO-" + String(LEDpin[0]) + "</div>\n";
    h += "   </button>\n";
    h += "   <button type=\"button\" class=\"btn btn-lg btn-outline-dark my-btn-3 my-btn-pic my-btn-pic-3 my-btn-3-gap\" onclick=\"javascript:__doPostBack('Led2', 'toggle');\">\n";
    if (LEDstatus[1] == HIGH || LEDmode[1] == ledModeBlink || ledTrafficLights == true) {
      h += "    <img src=\"" + SPIFFSPrefix + "src/led_yellow.png\" alt=\"yellow led\">\n";
    } else {
      h += "    <img src=\"" + SPIFFSPrefix + "src/led_grey.png\" alt=\"grey led\">\n";
    }
    h += "    <div>GPIO-" + String(LEDpin[1]) + "</div>\n";
    h += "   </button>\n";
    h += "   <button type=\"button\" class=\"btn btn-lg btn-outline-dark my-btn-3 my-btn-pic my-btn-pic-3\" onclick=\"javascript:__doPostBack('Led3', 'toggle');\">\n";
    if (LEDstatus[2] == HIGH || LEDmode[2] == ledModeBlink || ledTrafficLights == true) {
      h += "    <img src=\"" + SPIFFSPrefix + "src/led_green.png\" alt=\"green led\">\n";
    } else {
      h += "    <img src=\"" + SPIFFSPrefix + "src/led_grey.png\" alt=\"grey led\">\n";
    }
    h += "    <div>GPIO-" + String(LEDpin[2]) + "</div>\n";
    h += "   </button>\n";
    h += "  </div>\n";
    h += "  <div class=\"my-div-spacer\">\n";
    h += "  </div>\n";
    h += "  <div class=\"my-div-mobile\">\n";
    if (LEDmode[0] == ledModeBlink) {
      h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-3 my-btn-3-gap\" role=\"button\" onclick=\"javascript:__doPostBack('Led1', 'blinkstop');\">BLINK OFF</a>\n";
    } else {
      h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-3 my-btn-3-gap\" role=\"button\" onclick=\"javascript:__doPostBack('Led1', 'blinkstart');\">BLINK ON</a>\n";
    }
    if (LEDmode[1] == ledModeBlink) {
      h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-3 my-btn-3-gap\" role=\"button\" onclick=\"javascript:__doPostBack('Led2', 'blinkstop');\">BLINK OFF</a>\n";
    } else {
      h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-3 my-btn-3-gap\" role=\"button\" onclick=\"javascript:__doPostBack('Led2', 'blinkstart');\">BLINK ON</a>\n";
    }
    if (LEDmode[2] == ledModeBlink) {
      h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-3\"              role=\"button\" onclick=\"javascript:__doPostBack('Led3', 'blinkstop');\">BLINK OFF</a>\n";
    } else {
      h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-3\"              role=\"button\" onclick=\"javascript:__doPostBack('Led3', 'blinkstart');\">BLINK ON</a>\n";
    }
    h += "  </div>\n";
    h += "  <div class=\"my-div-spacer\">\n";
    h += "  </div>\n";
    h += "  <div class=\"my-div-mobile\">\n";
    if (ledTrafficLights == false) {
      h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-1\" role=\"button\" onclick=\"javascript:__doPostBack('Trafficlight', 'on');\">TRAFFIC LIGHTS ON</a>\n";
    } else {
      h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-1\" role=\"button\" onclick=\"javascript:__doPostBack('Trafficlight', 'off');\">TRAFFIC LIGHTS OFF</a>\n";
    }
    h += "  </div>\n";
  }
  // OneWire - DS18B20
  if (DSenabled == true) {
    while (dsQuery == true) {
      delay(100);
    }
    if (DScount > 0) {
      h += "  <p class=\"my-p-header\">DS18B20</p>\n";
      h += "  <div class=\"my-div-mobile\">\n";
      h += "   <p class=\"lead\">" + DStimestamp + "</p>\n";
      h += "   <table class=\"my-table-1\">\n";
      for(int i=1;i<=DScount; i++){
        h += "     <tr>\n";
        h += "      <td style=\"text-align:right\" class=\"macid\">" + DSaddr[i] + ":</td>\n";
        h += "      <td style=\"text-align:center\"><img src=\"" + SPIFFSPrefix + "src/temp_blue.png\" alt=\"temperature\"></td>\n";
        h += "      <td style=\"text-align:right\" class=\"sensorvalue\">";
        h += DStemp[i];
        h += "</td>\n";
        h += "      <td style=\"text-align:left\"  class=\"sensorvalue\">&deg;C</td>\n";
        h += "     </tr>\n";
      }
      h += "   </table>\n";
      h += "  </div>\n";
    }
  }
  // DHT
  if (DHTenabled == true) {
    while (dhtQuery == true) {
      delay(100);
    }  
    h += "  <p class=\"my-p-header\">DHT22</p>\n";
    h += "  <div class=\"my-div-mobile\">\n";
    h += "   <p class=\"lead\">" + DHTtimestamp + "</p>\n";
    h += "   <table class=\"my-table-1\">\n";
    h += "     <tr>\n";
    h += "      <td style=\"text-align:right\">Luftfeuchtigkeit:</td>\n";
    h += "      <td style=\"text-align:center\"><img src=\"" + SPIFFSPrefix + "src/humidy_blue.png\" alt=\"humidy\"></td>\n";
    h += "      <td style=\"text-align:right\" class=\"sensorvalue\">";
    h += DHTh;
    h += "</td>\n";
    h += "      <td style=\"text-align:left\"  class=\"sensorvalue\">%</td>\n";
    h += "     </tr>\n";
    h += "     <tr>\n";
    h += "      <td style=\"text-align:right\">Temperatur:</td>\n";
    h += "      <td style=\"text-align:center\"><img src=\"" + SPIFFSPrefix + "src/temp_blue.png\" alt=\"temperature\"></td>\n";
    h += "      <td style=\"text-align:right\" class=\"sensorvalue\">";
    h += DHTt;
    h += "</td>\n";
    h += "      <td style=\"text-align:left\"  class=\"sensorvalue\">&deg;C</td>\n";
    h += "     </tr>\n";
    h += "   </table>\n";
    h += "  </div>\n";
  }
  // WiFi
  if (WLANenabled == true) {
    h += "  <p class=\"my-p-header\">WiFi</p>\n";
    h += "  <div class=\"my-div-mobile\">\n";
    h += "   <p class=\"lead\">" + WLANtimestamp + "</p>\n";
    h += "  </div>\n";
    h += "  <div class=\"my-div-spacer\"></div>\n";
    h += "  <div class=\"my-div-mobile\">\n";
    h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-3 my-btn-3-gap\" role=\"button\" onclick=\"javascript:__doPostBack('WiFi', 'scan');\">SCAN</a>\n";
    h += "   <a class=\"btn btn-outline-secondary btn-lg my-btn-3 my-btn-3-gap\" role=\"button\" href=\"/scan\" target=\"_blank\">JSON</a>\n";
    h += "  </div>\n";
    h += "  <div class=\"my-div-mobile\">\n";
    h += "   <table class=\"table table-sm table-bordered table-hover\" style=\"margin-top: 1rem;\">\n";
    h += "    <thead>\n";
    h += "     <th class=\"macid\">RSSI</th>\n";
    h += "     <th class=\"macid\">SSID</th>\n";
    h += "     <th class=\"macid\">BSSID</th>\n";
    h += "     <th class=\"macid\">CH</th>\n";
    h += "     <th class=\"macid\">SECURE</th>\n";
    h += "     <th class=\"macid\">HIDDEN</th>\n";
    h += "    </thead>\n";
    for (int i = 1; i <= WLANcount; ++i) {
      h += "    <tr>\n";
      h += "     <td class=\"macid\" style=\"text-align:center\">" + WLANrssi[i] + "</td>\n";
      if (WLANssid[i].length() > 10) {
        h += "     <td class=\"macid\">" + WLANssid[i].substring(0,9) + "...</td>\n";
      } else {
        h += "     <td class=\"macid\">" + WLANssid[i] + "</td>\n";
      }
      h += "     <td class=\"macid\">" + WLANbssid[i] + "</td>\n";
      h += "     <td class=\"macid\" style=\"text-align:center\">" + WLANchannel[i] + "</td>\n";
      h += "     <td class=\"macid\" style=\"text-align:center\">" + WLANsecure[i] + "</td>\n";
      h += "     <td class=\"macid\" style=\"text-align:center\">" + String(WLANhidden[i]?"true":"false") + "</td>\n";
      h += "   </tr>\n";
    }
    h += "   </table>\n";
    h += "  </div>\n";
  }
  // CSS Info
  h += "  <p class=\"my-p-header\" style=\"margin-top:25rem;\">CSS</p>\n";
  h += "  <div>\n";
  h += "   <button type=\"button\" class=\"btn btn-lg btn-outline-secondary my-cssident\" onclick=\"getResolution();\"></button>\n";
  h += "  </div>\n";
  // PageEnd
  h += " </div>\n";
  h += "</div>\n";
  h += "</div>\n";
  h += "<form id=\"esp32form\" action=\"/\" method=\"POST\">\n";
  h += "<input type=\"hidden\" name=\"__EVENTTARGET\" id=\"__EVENTTARGET\" value=\"\" />\n";
  h += "<input type=\"hidden\" name=\"__EVENTARGUMENT\" id=\"__EVENTARGUMENT\" value=\"\" />\n";
  h += "</form>\n";
  h += "<script>\n";
  h += " if (typeof __doPostBack == 'undefined') {\n";
  h += "   __doPostBack = function (eventTarget, eventArgument) {\n";
  h += "     var ScreenDiv = document.getElementById('divScreen');\n";
  h += "     var AjaxLoaderDiv = document.getElementById('divAjaxLoader');\n";
  h += "     if (ScreenDiv) {\n";
  h += "         ScreenDiv.style.opacity = '0.3';\n";
  h += "     }\n";
  h += "     if (AjaxLoaderDiv) {\n";
  h += "         AjaxLoaderDiv.style.display = '';\n";
  h += "     }\n";
  h += "     var theForm = document.forms['esp32form'];\n";
  h += "     if (!theForm) {\n";
  h += "       theForm = document.forms.esp32form;\n";
  h += "     }\n";
  h += "     if (!theForm.onsubmit || (theForm.onsubmit() != false)) {\n";
  h += "       theForm.__EVENTTARGET.value = eventTarget;\n";
  h += "       theForm.__EVENTARGUMENT.value = eventArgument;\n";
  h += "       theForm.submit();\n";
  h += "     }\n";
  h += "   };\n";
  h += " }\n";
  h += "</script>\n";
  h += "<script>\n";
  h += " function getResolution() {\n";
  h += "  alert(\"screen resolution: \" + screen.width + \"x\" + screen.height);\n";
  h += " }\n";
  h += "</script>\n";
  h += "</body>\n";
  h += "</html>\n";
  return h;
}

void setup() {
  // put your setup code here, to run once:

  Serial.begin(115200);
  Serial.println("\n");

  // LEDs
  for (int i = 0; i < (sizeof(LEDpin) / sizeof(int)); ++i) {
    pinMode(LEDpin[i], OUTPUT);
    LEDstatus[i] = digitalRead(LEDpin[0]);
    LEDmode[i] = ledModeManual;
    LEDblink[i] = false;
  }
  ledTrafficLights = false;
  ledLastTime = millis();

  // Filesystem SPIFFS
  if (!SPIFFS.begin()) {
    Serial.println("An Error has occurred while mounting SPIFFS");
    return;
  }

  // Wifi Networks
  if (WiFiSearchMyNetwork == true) {
    WiFi.mode(WIFI_STA);
    WiFi.disconnect();
    delay(100);
    String myCLssid = GetMyNetwork();
    if (myCLssid == "") {
      delay (1000);
      ESP.restart();
    }
    myCLssid.toCharArray(WiFiCLssid,(myCLssid.length() + 1));
  }

  // WiFi Connect
  WiFi.persistent(false); // https://forum-raspberrypi.de/forum/thread/30941-esp8266-achtung-flash-speicher-schreibzugriff-bei-jedem-aufruf-von-u-a-wifi-begi/
                          // https://github.com/esp8266/Arduino/blob/4897e0006b5b0123a2fa31f67b14a3fff65ce561/doc/esp8266wifi/generic-class.md#persistent
  WiFi.begin(WiFiCLssid, WiFiCLpassword);
  delay(1000);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
    logMessage("WiFi",logSeverityDebug,"Connecting to WiFi (" + String(WiFiCLssid) + ") ...");
  }
  WiFiCLconnected = true;
  WiFiCLip = WiFi.localIP().toString();
  WiFiCLmac = WiFi.macAddress();
  logMessage("WiFi",logSeverityInformational,"Connected to " + String(WiFiCLssid));
  logMessage("WiFi",logSeverityInformational,"IP:  " + WiFiCLip);
  logMessage("WiFi",logSeverityInformational,"MAC: " + WiFiCLmac);

  // NTP
  configTime(gmtOffset_sec, daylightOffset_sec, ntpServer);
  logMessage("NTP",logSeverityInformational,"Time synchronized from " + String(ntpServer));

  // AP Mode
  if (WiFiAccessPoint == true) {
    IPAddress local_IP(192,168,4,1);
    IPAddress gateway(192,168,4,1);
    IPAddress subnet(255,255,255,0);
    Serial.print("Setting soft-AP configuration ... ");
    Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Ready" : "Failed!");
    Serial.print("Setting soft-AP ... ");
    Serial.println(WiFi.softAP(WiFiAPssid, WiFiAPpassword) ? "Ready" : "Failed!");
    WiFiAPip = WiFi.softAPIP().toString();
    WiFiAPmac = WiFi.softAPmacAddress().c_str();
    WiFiAPinitialized = true;
    logMessage("WiFi",logSeverityInformational,"AccessPoint " + String(WiFiAPssid) + " started");
    logMessage("WiFi",logSeverityInformational,"IP:  " + WiFiAPip);
    logMessage("WiFi",logSeverityInformational,"MAC: " + WiFiAPmac);
  }

  // DNS
  if (MDNS.begin(hostname)) {
    logMessage("Web",logSeverityInformational,"MDNS responder started - " + String(hostname));
  } else {
    MDNS.addService("http", "tcp", 80);
  }

  // Wifi Networks
  if (WLANenabled == true) {
    GetWlanNetworksSync();
  }

  // WebServer
  // WebServer - Events
  ws.onEvent(onEvent);
  // WebServer - Events - attach AsyncWebSocket
  server.addHandler(&ws);
  // WebServer - Events - attach AsyncEventSource
  server.addHandler(&events);
  // WebServer - Static Files
  //server.serveStatic("/src/", SPIFFS, "/www/src/").setCacheControl("max-age=600");

  // WebServer - Request - /ptr
  server.on("/ptr", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /ptr");
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->addHeader("Server","ESP Async Web Server");
    response->printf("<!DOCTYPE html><html><head><title>Webpage at %s</title></head><body>", request->url().c_str());
    response->print("<h2>Hello ");
    response->print(request->client()->remoteIP());
    response->print("</h2>");
    response->print("<h3>General</h3>");
    response->print("<ul>");
    response->printf("<li>Version: HTTP/1.%u</li>", request->version());
    response->printf("<li>Method: %s</li>", request->methodToString());
    response->printf("<li>URL: %s</li>", request->url().c_str());
    response->printf("<li>Host: %s</li>", request->host().c_str());
    response->printf("<li>ContentType: %s</li>", request->contentType().c_str());
    response->printf("<li>ContentLength: %u</li>", request->contentLength());
    response->printf("<li>Multipart: %s</li>", request->multipart()?"true":"false");
    response->print("</ul>");
    response->print("<h3>Headers</h3>");
    response->print("<ul>");
    int headers = request->headers();
    for(int i=0;i<headers;i++){
      AsyncWebHeader* h = request->getHeader(i);
      response->printf("<li>%s: %s</li>", h->name().c_str(), h->value().c_str());
    }
    response->print("</ul>");
    response->print("<h3>Parameters</h3>");
    response->print("<ul>");
    int params = request->params();
    for(int i=0;i<params;i++){
      AsyncWebParameter* p = request->getParam(i);
      if(p->isFile()){
        response->printf("<li>FILE[%s]: %s, size: %u</li>", p->name().c_str(), p->value().c_str(), p->size());
      } else if(p->isPost()){
        response->printf("<li>POST[%s]: %s</li>", p->name().c_str(), p->value().c_str());
      } else {
        response->printf("<li>GET[%s]: %s</li>", p->name().c_str(), p->value().c_str());
      }
    }
    response->print("</ul>");
    response->print("</body></html>");
    //send the response last
    request->send(response);
  });

  // WebServer - Requests - /
  server.on("/", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /");
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->addHeader("Server","ESP Async Web Server");
    response->print(GetHTML(srcPrefix));
    request->send(response);
  });
  server.on("/test", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /");
    AsyncResponseStream *response = request->beginResponseStream("text/html");
    response->addHeader("Server","ESP Async Web Server");
    response->print(GetHTML(""));
    request->send(response);
  });
  server.on("/", HTTP_POST,[](AsyncWebServerRequest *request) {
    if (request->hasArg("__EVENTTARGET") && request->hasArg("__EVENTARGUMENT")) {
      bool RequestHandled = false;
      AsyncWebParameter* awpTarget = request->getParam(0);
      String evTarget = awpTarget->value().c_str();
      evTarget.toUpperCase();
      AsyncWebParameter* awpArgument = request->getParam(1);
      String evArgument = awpArgument->value().c_str();
      evArgument.toUpperCase();
      logMessage("Web",logSeverityInformational,"Post: Target=" + evTarget + " Argument="+ evArgument);
      if (evTarget=="REFRESH" && evArgument=="TRUE") {
      }
      if (evTarget=="RESTART" && evArgument=="TRUE") {
        shouldReboot = true;
      }
      if (evTarget=="LED1" && evArgument=="TOGGLE") {
        ledTrafficLights = false;
        LEDmode[0] = ledModeManual;
        led_toggle(LEDpin[0],LEDstatus[0]);
      }
      if (evTarget=="LED2" && evArgument=="TOGGLE") {
        ledTrafficLights = false;
        LEDmode[1] = ledModeManual;
        led_toggle(LEDpin[1],LEDstatus[1]);
      }
      if (evTarget=="LED3" && evArgument=="TOGGLE") {
        ledTrafficLights = false;
        LEDmode[2] = ledModeManual;
        led_toggle(LEDpin[2],LEDstatus[2]);
      }
      if (evTarget=="LED1" && evArgument=="BLINKSTART") {
        ledTrafficLights = false;
        LEDmode[0] = ledModeBlink;
      }
      if (evTarget=="LED1" && evArgument=="BLINKSTOP") {
        ledTrafficLights = false;
        LEDmode[0] = ledModeManual;
        led_off(LEDpin[0],LEDstatus[0]);
      }
      if (evTarget=="LED2" && evArgument=="BLINKSTART") {
        ledTrafficLights = false;
        LEDmode[1] = ledModeBlink;
      }
      if (evTarget=="LED2" && evArgument=="BLINKSTOP") {
        ledTrafficLights = false;
        LEDmode[1] = ledModeManual;
        led_off(LEDpin[1],LEDstatus[1]);
      }
      if (evTarget=="LED3" && evArgument=="BLINKSTART") {
        ledTrafficLights = false;
        LEDmode[2] = ledModeBlink;
      }
      if (evTarget=="LED3" && evArgument=="BLINKSTOP") {
        ledTrafficLights = false;
        LEDmode[2] = ledModeManual;
        led_off(LEDpin[2],LEDstatus[2]);
      }
      if (evTarget=="TRAFFICLIGHT" && evArgument=="ON") {
        LEDmode[0] = ledModeTrafficlight;
        LEDmode[1] = ledModeTrafficlight;
        LEDmode[2] = ledModeTrafficlight;
        ledTrafficLights = true;
      }
      if (evTarget=="TRAFFICLIGHT" && evArgument=="OFF") {
        ledTrafficLights = false;
        LEDmode[0] = ledModeManual;
        LEDmode[1] = ledModeManual;
        LEDmode[2] = ledModeManual;
        led_off(LEDpin[0],LEDstatus[0]);
        led_off(LEDpin[1],LEDstatus[1]);
        led_off(LEDpin[2],LEDstatus[2]);
      }
      if (evTarget=="WIFI" && evArgument=="SCAN") {
        GetWlanNetworksSync();
        // RequestHandled = true;
        // request->send(200, "application/json", json);
      }
      if (RequestHandled == false) {
        request->send(200,"text/html",GetHTML(srcPrefix));
      }
    }
  });

  server.on("/led1", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[0] = ledModeManual;
    led_toggle(LEDpin[0],LEDstatus[0]);
    request->send(200,"text/html","OK");
  });
  server.on("/led2", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[1] = ledModeManual;
    led_toggle(LEDpin[1],LEDstatus[1]);
    request->send(200,"text/html","OK");
  });
  server.on("/led3", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[0] = ledModeManual;
    led_toggle(LEDpin[2],LEDstatus[2]);
    request->send(200,"text/html","OK");
  });
  server.on("/blinkstart1", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[0] = ledModeBlink;
    request->send(200,"text/html","OK");
  });
  server.on("/blinkstop1", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[0] = ledModeManual;
    led_off(LEDpin[0],LEDstatus[0]);
    request->send(200,"text/html","OK");
  });
  server.on("/blinkstart2", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[1] = ledModeBlink;
    request->send(200,"text/html","OK");
  });
  server.on("/blinkstop2", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[1] = ledModeManual;
    led_off(LEDpin[1],LEDstatus[1]);
    request->send(200,"text/html","OK");
  });
  server.on("/blinkstart3", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[2] = ledModeBlink;
    request->send(200,"text/html","OK");
  });
  server.on("/blinkstop3", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[2] = ledModeManual;
    led_off(LEDpin[2],LEDstatus[2]);
    request->send(200,"text/html","OK");
  });
  server.on("/ampelstart", HTTP_GET, [](AsyncWebServerRequest * request) {
    LEDmode[0] = ledModeTrafficlight;
    LEDmode[1] = ledModeTrafficlight;
    LEDmode[2] = ledModeTrafficlight;
    ledTrafficLights = true;
    request->send(200,"text/html","OK");
  });
  server.on("/ampelstop", HTTP_GET, [](AsyncWebServerRequest * request) {
    ledTrafficLights = false;
    LEDmode[0] = ledModeManual;
    LEDmode[1] = ledModeManual;
    LEDmode[2] = ledModeManual;
    led_off(LEDpin[0],LEDstatus[0]);
    led_off(LEDpin[1],LEDstatus[1]);
    led_off(LEDpin[2],LEDstatus[2]);
    request->send(200,"text/html","OK");
  });
  server.on("/restart", HTTP_GET, [](AsyncWebServerRequest * request) {
    request->send(200,"text/html","OK");
  });

  server.on("/index.html", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /index.html");
    RequestParameter(request);
    request->send(SPIFFS, "/index.html", "text/html");
  });

  // WebServer - Requests - JS, CSS
  server.on("/src/bootstrap.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/bootstrap.min.js");
    request->send(SPIFFS, "/src/bootstrap.min.js", "text/javascript");
  });
  server.on("/src/jquery-3.4.1.min.js", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/jquery-3.4.1.min.js");
    request->send(SPIFFS, "/src/jquery-3.4.1.min.js", "text/javascript");
  });
  server.on("/src/bootstrap.min.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/bootstrap.min.css");
    request->send(SPIFFS, "/src/bootstrap.min.css", "text/css");
  });
  server.on("/src/my.css", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/my.css");
    request->send(SPIFFS, "/src/my.css", "text/css");
  });

  // WebServer - Requests - ICO, JPG, PNG - https://wiki.selfhtml.org/wiki/MIME-Type/%C3%9Cbersicht
  server.on("/favicon.ico", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /favicon.ico");
    request->send(SPIFFS, "/favicon.ico", "image/x-icon");
  });
  server.on("/src/ajax_loader.gif", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/ajax_loader.gif");
    request->send(SPIFFS, "/src/ajax_loader.gif", "image/gif");
  });
  server.on("/src/power_blue.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/power_blue.png");
    request->send(SPIFFS, "/src/power_blue.png", "image/png");
  });
  server.on("/src/refresh.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/refresh.png");
    request->send(SPIFFS, "/src/refresh.png", "image/png");
  });
  server.on("/src/led_grey.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/led_grey.png");
    request->send(SPIFFS, "/src/led_grey.png", "image/png");
  });
  server.on("/src/led_red.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/led_red.png");
    request->send(SPIFFS, "/src/led_red.png", "image/png");
  });
  server.on("/src/led_yellow.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/led_yellow.png");
    request->send(SPIFFS, "/src/led_yellow.png", "image/png");
  });
  server.on("/src/led_green.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/led_green.png");
    request->send(SPIFFS, "/src/led_green.png", "image/png");
  });
  server.on("/src/temp_blue.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/temp_blue.png");
    request->send(SPIFFS, "/src/temp_blue.png", "image/png");
  });
  server.on("/src/humidy_blue.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/humidy_blue.png");
    request->send(SPIFFS, "/src/humidy_blue.png", "image/png");
  });
  server.on("/src/esp32a.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/esp32a.png");
    request->send(SPIFFS, "/src/esp32a.png", "image/png");
  });
  server.on("/src/esp32b.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/esp32b.png");
    request->send(SPIFFS, "/src/esp32b.png", "image/png");
  });
  server.on("/src/esp32c.png", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /src/esp32c.png");
    request->send(SPIFFS, "/src/esp32c.png", "image/png");
  });

  // WebServer - Request - Scan
  server.on("/scan", HTTP_GET, [](AsyncWebServerRequest * request) {
    logMessage("Web",logSeverityInformational,"Get: /scan ");
    String json = GetWlanNetworksJson();
    request->send(200, "application/json", json);
    json = String();
  });  

  // WebServer - Catch-All Handlers - Any request that can not find a Handler that canHandle it ends in the callbacks below.
  server.onNotFound(onRequest);
  server.onFileUpload(onUpload);
  server.onRequestBody(onBody);
  // WebServer - START
  server.begin();
  delay(1000);
  logMessage("Web",logSeverityInformational,"HTTP server started - http://" + WiFiCLip + " (http://" + hostname + ")");

  // OneWire DSB1820 - START
  if (DSenabled == true) {
    pinMode(ONE_WIRE_BUS, INPUT);
    oneWireSensors.begin();
    delay(1000);
    OneWireDevicesCount = oneWireSensors.getDeviceCount();
    logMessage("DS18B20",logSeverityInformational,"Found " + String(OneWireDevicesCount, DEC) + " devices");
    if (OneWireDevicesCount==0) {
      OneWireDevicesCount = 1;
    }
    for(int i=0;i<OneWireDevicesCount; i++){
      if(oneWireSensors.getAddress(tempDeviceAddress, i)){
        logMessage("DS18B20",logSeverityInformational,"Found device " + String(i + 1, DEC) + ": " + getOneWireDeviceAddress(tempDeviceAddress));
      } else {
        logMessage("DS18B20",logSeverityError,"Found ghost device at " + String(i + 1, DEC) + " but could not detect address. Check power and cabling.");
      }
    }
  }

  // DHT - START
  if (DHTenabled == true) {
    pinMode(DHTPIN, INPUT);
    dht.begin();
    delay(2000);
  }

  logMessage("setup()",logSeverityInformational,"ESP32 initialized");
}

void loop() {
  // put your main code here, to run repeatedly:

  int currenttime = millis();

  // Reboot
  if (shouldReboot) {
    logMessage("Loop()",logSeverityInformational,"Rebooting ...");
    delay(3000);
    ESP.restart();
  }

  // LED's
  if (LEDenabled == true) {
    currenttime = millis();
    if (ledTrafficLights == false) {
      tlPhase = -1;
      if (ledLastTime <=  (currenttime - ledBlinkInterval) || ledLastTime == 0) {
        if (LEDmode[0] == ledModeBlink) {
          led_toggle(LEDpin[0],LEDstatus[0]);
        }
        if (LEDmode[1] == ledModeBlink) {
          led_toggle(LEDpin[1],LEDstatus[1]);
        }
        if (LEDmode[2] == ledModeBlink) {
          led_toggle(LEDpin[2],LEDstatus[2]);
        }
        ledLastTime = currenttime;
      }
    } else {
      if (tlPhase == -1 || tlLastTime == 0) {
        tlPhase = 0;
        if (LEDstatus[0] != tlPhases[tlPhase][0]) {
          led_set(LEDpin[0],LEDstatus[0],tlPhases[tlPhase][0]);
        }
        if (LEDstatus[1] != tlPhases[tlPhase][1]) {
          led_set(LEDpin[1],LEDstatus[1],tlPhases[tlPhase][1]);
        }
        if (LEDstatus[2] != tlPhases[tlPhase][2]) {
          led_set(LEDpin[2],LEDstatus[2],tlPhases[tlPhase][2]);
        }
        tlLastTime = currenttime;
      } else {
        if (tlLastTime <=  (currenttime - tlTimes[tlPhase]) || tlLastTime == 0) {
          tlPhase += 1;
          if (tlPhase > 9) {
            tlPhase = 0;
          }
          if (LEDstatus[0] != tlPhases[tlPhase][0]) {
            led_set(LEDpin[0],LEDstatus[0],tlPhases[tlPhase][0]);
          }
          if (LEDstatus[1] != tlPhases[tlPhase][1]) {
            led_set(LEDpin[1],LEDstatus[1],tlPhases[tlPhase][1]);
          }
          if (LEDstatus[2] != tlPhases[tlPhase][2]) {
            led_set(LEDpin[2],LEDstatus[2],tlPhases[tlPhase][2]);
          }
          tlLastTime = currenttime;
        }
      }
    }
    delay(100);
  }

  // OneWire DSB1820
  if (DSenabled == true) {
    currenttime = millis();
    if (dsLastTime <=  (currenttime - dsQueryInterval) || dsLastTime == 0) {
      oneWireSensors.begin(); // again due to getDeviceCount()
      int currentOneWireDevicesCount = oneWireSensors.getDeviceCount();
      if (currentOneWireDevicesCount != OneWireDevicesCount) {
        logMessage("DS18B20",logSeverityAlert,"new/deleted devices - Found " + String(currentOneWireDevicesCount,DEC) +" devices.");
        OneWireDevicesCount = currentOneWireDevicesCount;
      }
      oneWireSensors.requestTemperatures();
      dsQuery = true;
      if (OneWireDevicesCount == 0) {
        oneWireSensors.getAddress(tempDeviceAddress, 0);
        float tempC = oneWireSensors.getTempCByIndex(0);
        DScount = 1;
        DSaddr[DScount]=getOneWireDeviceAddress(tempDeviceAddress);
        DStemp[DScount]=tempC;
        DStimestamp = getTimeStamp();
        logMessage("DS18B20",logSeverityDebug,"Sensor-1 (" + DSaddr[DScount] + ") Temperatur: " + String(tempC,DEC) + " °C");
        // float tempF = oneWireSensors.getTempFByIndex(0);
        // logMessage("DS18B20",logSeverityDebug,"DS18B20-1 (" + DSaddr[DScount] + "): Temperatur: " + String(tempF,DEC) + " °F");
      } else {
        DScount = 0;
        DStimestamp = getTimeStamp();
        for(int i=0;i<OneWireDevicesCount; i++){ // Loop through each device
          if(oneWireSensors.getAddress(tempDeviceAddress, i)){ // Search the wire for address
            float tempC = oneWireSensors.getTempC(tempDeviceAddress);
            DScount += 1;
            DSaddr[DScount]=getOneWireDeviceAddress(tempDeviceAddress);
            DStemp[DScount]=tempC;
            logMessage("DS18B20",logSeverityDebug,"Sensor-" + String(i + 1,DEC) + " (" + DSaddr[DScount] + ") Temperatur: " + String(tempC,2) + " °C");
            // float tempF = DallasTemperature::toFahrenheit(tempC);
            // logMessage("DS18B20",logSeverityDebug,"Sensor-" + String(i + 1,DEC) + " (" + DSaddr[DScount] + ") Temperatur: " + String(tempF,2) + " °F");
          }
        }
      }
      dsLastTime = currenttime;
      dsQuery = false;
      delay(100);
    }
  }

  // DHT
  if (DHTenabled == true) {
    currenttime = millis();
    if (dhtLastTime <=  (currenttime - dhtQueryInterval) || dhtLastTime == 0) {
      delay(100);
      float h = dht.readHumidity();
      float t = dht.readTemperature();
      int dhtRetryCount = 0;
      int dhtMaxRetryCount = 0; // 500;
      while (((isnan(h) || isnan(t))) && dhtRetryCount < dhtMaxRetryCount) {
        dhtRetryCount += 1;
        dht.begin();
        h = dht.readHumidity();
        t = dht.readTemperature();
      }
      if (isnan(h) || isnan(t)) { // Failed to read from DHT sensor
        // logMessage("DHT",logSeverityError,"Fehler beim Lesen vom DHT-Sensor");
        delay(100);
        return;
      } else {
        dhtQuery = true;
        DHTh = h;
        DHTt = t;
        DHTtimestamp = getTimeStamp();
        if (dhtRetryCount > 0) {
          logMessage("DHT",logSeverityDebug,"Luftfeuchtigkeit: " + String(h,2) +" %          Temperatur: " + String(t,2) + " °C (" + dhtRetryCount + " retries)");
        } else {
          logMessage("DHT",logSeverityDebug,"Luftfeuchtigkeit: " + String(h,2) +" %          Temperatur: " + String(t,2) + " °C");
        }
      }
      dhtLastTime = currenttime;
      dhtQuery = false;
      delay(100);
    }
  }
}