#include <Arduino.h>
#include <LoRa.h>
#include <esp_task_wdt.h>
#include <ArduinoJson.h>
#include <CayenneLPP.h>

/*****************************************************************/
/* GLOBAL CONSTS                                                 */
/*****************************************************************/
#ifndef DEBUG_MODE
#define DEBUG_MODE 1
#endif

#define mS_TO_S_FACTOR 1000 // us
#define uS_TO_S_FACTOR 1000000 // us
#define SERIAL_BAUD 9600 // bps
#define RFM95_SEND_RATE 5 // s
#define SLEEP_TIME_FACTOR 0.9f
#define TWDT_TIMEOUT_MS (2 * RFM95_SEND_RATE * mS_TO_S_FACTOR)
#define ESP_WAKE_UP_PERIOD_US (SLEEP_TIME_FACTOR * RFM95_SEND_RATE * uS_TO_S_FACTOR)
#define PAYLOAD_SIZE 40 // bytes

#define RFM95_RESET_PIN 25
#define RFM95_DIO0_PIN 26
#define SPI_CS0_PIN 5
#define SPI_SCLK_PIN 18
#define SPI_MISO_PIN 19
#define SPI_MOSI_PIN 23

#define BME_280_SENSOR_IDENTIFIER 1
#define UV_SENSOR_IDENTIFIER 2
#define SOIL_MOISTURE_SENSOR_IDENTIFIER 3
#define RAIN_SENSOR_IDENTIFIER 4

/*****************************************************************/
/* GLOBAL VARIABLES                                              */
/*****************************************************************/
extern volatile bool is_packet_received;
extern char receivedMessage[];

/*****************************************************************/
/* STRUCTURES                                                    */
/*****************************************************************/
struct WeatherData {
  float temperature;
  float humidity;
  float pressure;
  uint8_t uvIndex;
  uint8_t soilMoisture;
  uint8_t rainPercent;
  int8_t packetRSSI;
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

/*****************************************************************/
/* WORKER FUNCTIONS                                              */
/*****************************************************************/

void convertHexStringToByteArray(const String& hexString, uint8_t* byteArray, size_t& byteArraySize);

  ///////////////////////////////////////////////////////////////
  /// Convert the received hexadecimal encoded Low Power Payload 
  /// string to JSON array.
void decodePacketToJsonArray(const String& hexString, JsonArray& JSONArrayPacket);

void parseJsonArrayPacketToWeatherDataStruct(const JsonArray& JSONArrayPacket, WeatherData& weatherData);

  ///////////////////////////////////////////////////////////////
  /// Prints the weatherData to the Serial Monitor.
void printWeatherDataToSerialMonitor(WeatherData& weatherData);

void onCadDone(boolean signalDetected);

  ///////////////////////////////////////////////////////////////
  /// Callback function called when packet is received.
void onReceive(int packetSize);

  ///////////////////////////////////////////////////////////////
  /// Get the last received packet's content.
String getReceivedMessage();  
