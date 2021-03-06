#include <ESP8266WiFi.h>          // https://github.com/esp8266/Arduino
#include <ESP8266HTTPClient.h>    // https://github.com/esp8266/Arduino
#include <ESP8266httpUpdate.h>    // https://github.com/esp8266/Arduino
#include <WiFiUdp.h>              // https://github.com/esp8266/Arduino

// for easy remote control
#include <TinyUPnP.h>             // https://github.com/ofekp/TinyUPnP

// needed for local file system working
#include <LittleFS.h>             // https://github.com/esp8266/Arduino
#include <ArduinoJson.h>          // https://github.com/bblanchon/ArduinoJson

// needed for library WiFiManager
#include <DNSServer.h>            // https://github.com/esp8266/Arduino
#include <ESP8266WebServer.h>     // https://github.com/esp8266/Arduino
#include <MyWiFiManager.h>        // https://github.com/tzapu/WiFiManager (modified, see local libraries)
// важно знать! используется изменённая библиотека WiFiManager 0.15, 
// с русским переводом, блокировкой сброса точки в случае длительного отсуствия и парой баг фиксов

// needed for sensors
#include <Wire.h>                 // https://github.com/esp8266/Arduino

// needed for statuses LED
#include <Ticker.h>               // https://github.com/esp8266/Arduino

// // // это был длииинный список библиотек для запуска этой штуки :)))

ADC_MODE(ADC_VCC); // чтобы измерять self-voltage level 3.3V

Ticker ticker1;
Ticker ticker2;

WiFiUDP udp;
TinyUPnP tinyUPnP(15000);

#define SERIAL_BAUD 115200 // скорость Serial порта, менять нет надобности
#define CHIP_TEST 0 // если нужно протестировать плату без подключения датчиков, задайте 1

#define MAIN_MODE_NORMAL 100 // всё нормально, связь и работа устройства в норме
#define MAIN_MODE_OFFLINE 200 // устройство работает, но испытывает проблемы с передачей данных
#define MAIN_MODE_FAIL 300 // что-то пошло не так, устройство не может функционировать без вмешательства прямых рук

#define OSMO_HTTP_SERVER_DEVICE "http://iot.osmo.mobi/device"
#define OSMO_HTTP_SERVER_SEND "https://iot.osmo.mobi/send"
#define OSMO_HTTP_SERVER_SEND_PACK "https://iot.osmo.mobi/sendPack"
#define OSMO_SERVER_HOST "osmo.mobi"
#define OSMO_SERVER_PORT 24827

//boolean STATUS_BME280_GOOD = true;
//boolean STATUS_GY21_GOOD = true;
boolean STATUS_REPORT_SEND = false;

boolean UDP_MODE = true; // переключение устройства в режим постоянной связи
boolean UPnP = false; // флаг того, что роутер открыл нам прямую связь на порт

int LOCAL_PORT = 10142; // локальный порт для UDP
int PING_INTERVAL = 1000; // интервал пинга сервера по UDP по умолчанию
int LED_BRIGHT = 200; // яркость внешнего светодиода в режиме ожидания
int REBOOT_INTERVAL = 6 * 60 * 60000; // интервал принудительной перезагрузки устройства, мы не перезагружаемся, если нет сети, чтобы не потерять время и возможность накапливать буфер
int RECONFIG_INTERVAL = 60 * 60000; // интервал обновления конфигурации устройства с сервера
int REPORT_INTERVAL = 60 * 60000; // интервал повтора отправки отчёта о проблемах (если проблема с чтением данных с сенсоров актуальна)

boolean NO_INTERNET = true; // флаг состояния, поднимается если отвалилась wifi сеть
boolean NO_SERVER = true; // флаг состояния, поднимается если отвалился сервер
boolean MODE_SEND_BUFFER = false; // флаг означающий, что необходимо сделать опустошение буфера
int MODE_RESET_WIFI = 0; // флаг означающий, что пользователем инициирован процесс очистки настроек WiFi

int BUFFER_COUNT = 0; // счётчик строк в буферном файле не отправленных на сервер

const char* DEVICE_MODEL = "IoT";
const char* DEVICE_REVISION = "alpha"; 
const char* DEVICE_FIRMWARE = "0.3.0";

const int RESET_WIFI = 0; // D3
const int LED_EXTERNAL = 2;

unsigned long previousMillis = 0;
unsigned long previousMillisConfig = 0;
unsigned long previousMillisPing = 0;
unsigned long previousMillisReboot = 0;
unsigned long previousMillisReport = 0;

String deviceName = String(DEVICE_MODEL) + "_" + String(DEVICE_FIRMWARE);

String OsMoSSLFingerprint = "";
String TOKEN = "";

int pingCount = 0;

// WifiManager callback
void configModeCallback(WiFiManager *myWiFiManager) 
{
  Serial.println("Entered config mode");
  Serial.println(WiFi.softAPIP());
  Serial.println(myWiFiManager->getConfigPortalSSID());
  Serial.println(WiFi.macAddress());
  ticker1.attach_ms(500, tickInternal);
  ticker2.attach_ms(1000, tickExternal, MAIN_MODE_NORMAL);
}
