#include <Arduino.h>
#include <LoRa.h>
#include <ArduinoJson.h>
#include "config.h"
#include <WiFi.h>

/*****************************************************************/
/* GLOBAL CONSTS                                                 */
/*****************************************************************/
#define mS_TO_S_FACTOR 1000    // us
#define uS_TO_S_FACTOR 1000000 // us
#define SERIAL_BAUD 115200     // bps
#define RFM95_COMM_FREQ 868E6
#define RFM95_SEND_RATE 60 // s
#define SLEEP_TIME_FACTOR 0.8f
#define ESP_WAKE_UP_PERIOD_US (SLEEP_TIME_FACTOR * RFM95_SEND_RATE * uS_TO_S_FACTOR)
#define INACTIVITY_THRESHOLD_MS (5 * RFM95_SEND_RATE * mS_TO_S_FACTOR)
#define WIFI_CONNECTION_TIMEOUT_MS 10000 // ms
#define PAYLOAD_SIZE 23                  // bytes

#define RFM95_RESET_PIN 25
#define RFM95_DIO0_PIN 26
#define SPI_CS0_PIN 5
#define SPI_SCLK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23
#define BUZZER_PIN 27
#define DEBUG_MODE_EN_PIN 14

#define DEVICE_ID 0
#define BME_280_TEMPERATURE_SENSOR_IDENTIFIER 0
#define BME_280_HUMIDITY_SENSOR_IDENTIFIER 1
#define BME_280_PRESSURE_SENSOR_IDENTIFIER 2
#define UV_SENSOR_IDENTIFIER 3
#define SOIL_MOISTURE_SENSOR_IDENTIFIER 4
#define RAIN_SENSOR_IDENTIFIER 5
#define BAT_LEVEL_IDENTIFIER 6

/*****************************************************************/
/* GLOBAL VARIABLES                                              */
/*****************************************************************/
extern volatile bool is_packet_received;
extern char receivedMessage[];
extern volatile int16_t packetRSSI;

/*****************************************************************/
/* STRUCTURES                                                    */
/*****************************************************************/
struct MeasureData
{
  float temperature;
  float humidity;
  float pressure;
  uint8_t uvIndex;
  uint8_t soilMoisture;
  uint8_t rainPercent;
  uint8_t batLevel;
};

/*****************************************************************/
/* INIT FUNCTIONS                                                */
/*****************************************************************/

///////////////////////////////////////////////////////////////
/// Start the Serial communication at SERIAL_BAUD rate.
/// Wait for the serial port and the Monitor to be ready.
void initializeSerialCommunication();

///////////////////////////////////////////////////////////////
/// Configure the ESP32's pins.
void configureGPIO();

///////////////////////////////////////////////////////////////
/// Connect to a WiFi network with given ssid and password.
void connectToWifi(const char *ssid, const char *password);

/*****************************************************************/
/* WORKER FUNCTIONS                                              */
/*****************************************************************/

///////////////////////////////////////////////////////////////
/// Play a beeping sound using the Piezo Buzzer.
void playBeepingSound();

void convertHexStringToByteArray(const String &hexString, uint8_t *byteArray, size_t &byteArraySize);

///////////////////////////////////////////////////////////////
/// Convert the received hexadecimal encoded Low Power Payload
/// string to JSON array.
void decodePacketToJsonArray(const String &hexString, JsonArray &JSONArrayPacket);

///////////////////////////////////////////////////////////////
/// Send the decoded JSON packet via HTTP POST request.
void sendPayloadViaHTTPRequest(String &HTTPPayload);

void parseJsonArrayPacketToMeasureDataStruct(const JsonArray &JSONArrayPacket, MeasureData &measureData);

///////////////////////////////////////////////////////////////
/// Create a JSON string payload for the HTTP POST request.
void createPayloadForHTTPRequest(const JsonArray &JSONArrayPacket, String &HTTPPayload);

///////////////////////////////////////////////////////////////
/// Check if the debug mode switch is on.
bool isDebugMode();

///////////////////////////////////////////////////////////////
/// Prints the measureData to the Serial Monitor.
void printMeasureDataToSerialMonitor(MeasureData &measureData);

///////////////////////////////////////////////////////////////
/// Callback function called after channel activity was detected.
void onCadDone(boolean signalDetected);

///////////////////////////////////////////////////////////////
/// Callback function called when packet is received.
void onReceive(int packetSize);

///////////////////////////////////////////////////////////////
/// Get the last received packet's content.
String getReceivedMessage();

///////////////////////////////////////////////////////////////
/// Stop the used libraries.
void endLibraries();

///////////////////////////////////////////////////////////////
/// Enter deep sleep mode until user reset.
void enterDeepSleepForever();

///////////////////////////////////////////////////////////////
/// If not receiving packets, enter deep sleep until reset.
void handleInactivity(unsigned long &lastActivityTime);
